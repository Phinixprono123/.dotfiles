// main.c
// Nord-Notify — GTK4 notification daemon for Wayland/Hyprland
// Build: see instructions below

#define _GNU_SOURCE
#include <adwaita.h>
#include <gio/gdesktopappinfo.h> // for GDesktopAppInfo
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  guint id;
  GtkWidget *row;        // container card for this notification
  GtkRevealer *revealer; // slide-in/out
  gchar *desktop_id;     // from hints["desktop-entry"]
  gchar *app_name;       // Notify app_name
  guint timeout_ms;      // expiration
  guint timeout_src;     // g_timeout source id
} Popup;

typedef struct {
  GtkApplication *app;
  GtkWidget *window; // layer-shell window
  GtkWidget *vbox;   // stack of popups (top to bottom)
  GDBusConnection *bus;
  guint reg_id;       // dbus object registration
  guint own_id;       // bus name owner
  GHashTable *popups; // id -> Popup*
  guint next_id;      // incremental notification ids
} Server;

static Server gsv = {0};

// ----- CSS loading (defaults, with optional override) -----

static const char *DEFAULT_CSS =
    "window.layer-shell {"
    "  background: transparent;"
    "}"
    ".nord-card {"
    "  background-color: #2E3440;" /* Polar Night */
    "  border-radius: 18px;"
    "  border: 1px solid #4C566A;" /* Frost border */
    "  padding: 12px 14px;"
    "  margin: 8px;"
    "  box-shadow: 0 10px 30px 0 rgba(0,0,0,0.35);"
    "}"
    ".nord-title {"
    "  font-weight: 700;"
    "  font-size: 14.5pt;"
    "  color: #ECEFF4;" /* Snow Storm */
    "}"
    ".nord-body {"
    "  margin-top: 4px;"
    "  color: #D8DEE9;"
    "  font-size: 11.5pt;"
    "}"
    ".nord-accent {"
    "  background: #88C0D0;" /* Frost blue accent */
    "  min-width: 3px;"
    "  border-radius: 2px;"
    "}"
    ".nord-app {"
    "  color: #A3BE8C;" /* green hint for app name */
    "  font-weight: 600;"
    "  opacity: 0.8;"
    "}";

static void load_css(void) {
  GtkCssProvider *prov = gtk_css_provider_new();
  // Try ~/.config/nordnotify/style.css first
  const char *xdg = g_get_user_config_dir();
  char *path = g_build_filename(xdg, "nordnotify", "style.css", NULL);
  if (g_file_test(path, G_FILE_TEST_EXISTS)) {
    gtk_css_provider_load_from_path(prov, path);
  } else {
    gtk_css_provider_load_from_data(prov, DEFAULT_CSS, -1);
  }
  g_free(path);

  gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                             GTK_STYLE_PROVIDER(prov),
                                             GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref(prov);
}

// ----- Helper: launch desktop app -----

static void launch_desktop(const char *desktop_id,
                           const char *fallback_app_name) {
  if (desktop_id && *desktop_id) {
    GDesktopAppInfo *info = g_desktop_app_info_new(desktop_id);
    if (info) {
      GError *err = NULL;
      if (!g_app_info_launch(G_APP_INFO(info), NULL, NULL, &err)) {
        g_warning("Launch failed for %s: %s", desktop_id, err->message);
        g_clear_error(&err);
      }
      g_object_unref(info);
      return;
    }
  }
  // Fallback: try common desktop-id heuristics from app_name
  if (fallback_app_name && *fallback_app_name) {
    // Try lowercase + .desktop
    char *lower = g_ascii_strdown(fallback_app_name, -1);
    char *guess = g_strdup_printf("%s.desktop", lower);
    GDesktopAppInfo *info = g_desktop_app_info_new(guess);
    if (!info) {
      // Try org.%s.%s style: org.%s.%s.desktop
      char *guess2 = g_strdup_printf("org.%s.%s.desktop", lower, lower);
      info = g_desktop_app_info_new(guess2);
      g_free(guess2);
    }
    if (info) {
      GError *err = NULL;
      if (!g_app_info_launch(G_APP_INFO(info), NULL, NULL, &err)) {
        g_warning("Fallback launch failed: %s", err->message);
        g_clear_error(&err);
      }
      g_object_unref(info);
    } else {
      g_warning("No desktop entry found for app_name=%s", fallback_app_name);
    }
    g_free(guess);
    g_free(lower);
  }
}

// ----- Popup lifecycle -----

static void popup_destroy(Popup *p);

static gboolean popup_auto_close_cb(gpointer user_data) {
  Popup *p = user_data;
  // slide out, then destroy on transition end
  gtk_revealer_set_reveal_child(p->revealer, FALSE);
  // schedule destroy a bit after transition
  g_timeout_add(220, (GSourceFunc)popup_destroy, p);
  return G_SOURCE_REMOVE;
}

static Popup *popup_new(guint id, const char *app_name, const char *summary,
                        const char *body, const char *desktop_id,
                        int expire_timeout_ms) {
  Popup *p = g_new0(Popup, 1);
  p->id = id;
  p->timeout_ms =
      (expire_timeout_ms <= 0 ? 6000 : (guint)expire_timeout_ms); // default 6s
  p->desktop_id = g_strdup(desktop_id ? desktop_id : "");
  p->app_name = g_strdup(app_name ? app_name : "");

  // Row content: [accent] [text column]
  GtkWidget *card = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_widget_add_css_class(card, "nord-card");

  GtkWidget *accent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(accent, "nord-accent");
  gtk_widget_set_size_request(accent, 3, -1);
  gtk_box_append(GTK_BOX(card), accent);

  GtkWidget *col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_append(GTK_BOX(card), col);

  // App line (subtle)
  if (app_name && *app_name) {
    GtkWidget *app = gtk_label_new(app_name);
    gtk_widget_add_css_class(app, "nord-app");
    gtk_widget_set_halign(app, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(app), PANGO_ELLIPSIZE_END);
    gtk_box_append(GTK_BOX(col), app);
  }

  GtkWidget *title =
      gtk_label_new(summary && *summary ? summary : "(no title)");
  gtk_widget_add_css_class(title, "nord-title");
  gtk_widget_set_halign(title, GTK_ALIGN_START);
  gtk_label_set_wrap(GTK_LABEL(title), TRUE);
  gtk_box_append(GTK_BOX(col), title);

  if (body && *body) {
    GtkWidget *bodyw = gtk_label_new(body);
    gtk_widget_add_css_class(bodyw, "nord-body");
    gtk_widget_set_halign(bodyw, GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(bodyw), TRUE);
    gtk_box_append(GTK_BOX(col), bodyw);
  }

  // Gestures: click to open app
  GtkGesture *click = gtk_gesture_click_new();
  g_signal_connect(click, "released",
                   G_CALLBACK(+[](GtkGestureClick *g, gint n_press, gdouble x,
                                  gdouble y, gpointer user_data) {
                     Popup *pp = user_data;
                     launch_desktop(pp->desktop_id, pp->app_name);
                     // Close immediately after click
                     gtk_revealer_set_reveal_child(pp->revealer, FALSE);
                     g_timeout_add(180, (GSourceFunc)popup_destroy, pp);
                   }),
                   p);
  gtk_widget_add_controller(card, GTK_EVENT_CONTROLLER(click));

  // Wrap in revealer for slide animation
  GtkWidget *rev = gtk_revealer_new();
  p->revealer = GTK_REVEALER(rev);
  gtk_revealer_set_transition_type(p->revealer,
                                   GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
  gtk_revealer_set_transition_duration(p->revealer, 180);
  gtk_revealer_set_reveal_child(p->revealer, TRUE);
  gtk_revealer_set_child(p->revealer, card);

  p->row = rev;

  // Auto-close
  p->timeout_src = g_timeout_add(p->timeout_ms, popup_auto_close_cb, p);
  return p;
}

static void popup_add_to_ui(Popup *p) {
  gtk_box_prepend(GTK_BOX(gsv.vbox), p->row); // newest on top
  // Present window if hidden
  gtk_window_present(GTK_WINDOW(gsv.window));
}

static void popup_remove_from_ui(Popup *p) {
  if (GTK_IS_WIDGET(p->row)) {
    gtk_widget_unparent(p->row);
  }
}

static void popup_free(Popup *p) {
  if (!p)
    return;
  if (p->timeout_src) {
    g_source_remove(p->timeout_src);
    p->timeout_src = 0;
  }
  g_free(p->desktop_id);
  g_free(p->app_name);
  g_free(p);
}

static void popup_destroy_delayed(gpointer data) {
  Popup *p = data;
  popup_remove_from_ui(p);
  g_hash_table_remove(
      gsv.popups, GUINT_TO_POINTER(p->id)); // will free via value destroy func
}

static void popup_destroy(Popup *p) { popup_destroy_delayed(p); }

// ----- DBus org.freedesktop.Notifications -----

static const char *IFACE_XML =
    "<node>"
    "  <interface name='org.freedesktop.Notifications'>"
    "    <method name='GetServerInformation'>"
    "      <arg type='s' name='name' direction='out'/>"
    "      <arg type='s' name='vendor' direction='out'/>"
    "      <arg type='s' name='version' direction='out'/>"
    "      <arg type='s' name='spec_version' direction='out'/>"
    "    </method>"
    "    <method name='GetCapabilities'>"
    "      <arg type='as' name='caps' direction='out'/>"
    "    </method>"
    "    <method name='Notify'>"
    "      <arg type='s' name='app_name' direction='in'/>"
    "      <arg type='u' name='replaces_id' direction='in'/>"
    "      <arg type='s' name='app_icon' direction='in'/>"
    "      <arg type='s' name='summary' direction='in'/>"
    "      <arg type='s' name='body' direction='in'/>"
    "      <arg type='as' name='actions' direction='in'/>"
    "      <arg type='a{sv}' name='hints' direction='in'/>"
    "      <arg type='i' name='expire_timeout' direction='in'/>"
    "      <arg type='u' name='id' direction='out'/>"
    "    </method>"
    "    <method name='CloseNotification'>"
    "      <arg type='u' name='id' direction='in'/>"
    "    </method>"
    "    <signal name='NotificationClosed'>"
    "      <arg type='u' name='id'/>"
    "      <arg type='u' name='reason'/>"
    "    </signal>"
    "    <signal name='ActionInvoked'>"
    "      <arg type='u' name='id'/>"
    "      <arg type='s' name='action_key'/>"
    "    </signal>"
    "  </interface>"
    "</node>";

static GDBusNodeInfo *introspection = NULL;

static void emit_closed_signal(guint id, guint reason) {
  // reason: 1=expired, 2=dismissed, 3=closed, 4=undefined
  GVariant *params = g_variant_new("(uu)", id, reason);
  g_dbus_connection_emit_signal(gsv.bus, NULL, "/org/freedesktop/Notifications",
                                "org.freedesktop.Notifications",
                                "NotificationClosed", params, NULL);
}

static void handle_method_call(GDBusConnection *conn, const char *sender,
                               const char *obj_path, const char *iface,
                               const char *method, GVariant *params,
                               GDBusMethodInvocation *invoc,
                               gpointer user_data) {
  if (g_strcmp0(method, "GetServerInformation") == 0) {
    g_dbus_method_invocation_return_value(
        invoc, g_variant_new("(ssss)", "NordNotify", "Prono", "0.1.0", "1.2"));
    return;
  }
  if (g_strcmp0(method, "GetCapabilities") == 0) {
    const char *caps[] = {"body", "body-markup", "body-hyperlinks",
                          "icon-static"};
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
    for (guint i = 0; i < G_N_ELEMENTS(caps); i++)
      g_variant_builder_add(&b, "s", caps[i]);
    g_dbus_method_invocation_return_value(invoc, g_variant_new("(as)", &b));
    return;
  }
  if (g_strcmp0(method, "CloseNotification") == 0) {
    guint32 id;
    g_variant_get(params, "(u)", &id);
    Popup *p = g_hash_table_lookup(gsv.popups, GUINT_TO_POINTER(id));
    if (p) {
      gtk_revealer_set_reveal_child(p->revealer, FALSE);
      g_timeout_add(180, (GSourceFunc)popup_destroy, p);
      emit_closed_signal(id, 3); // 3=closed by close call
    }
    g_dbus_method_invocation_return_value(invoc, NULL);
    return;
  }
  if (g_strcmp0(method, "Notify") == 0) {
    const char *app_name, *app_icon, *summary, *body;
    guint32 replaces_id;
    GVariant *actions;
    GVariant *hints;
    gint32 expire_timeout;
    g_variant_get(params, "(&su&ss&sas@a{sv}i)", &app_name, &replaces_id,
                  &app_icon, &summary, &body, &actions, &hints,
                  &expire_timeout);

    // Reuse ID if provided and exists, else assign new
    guint id = replaces_id ? replaces_id : ++gsv.next_id;

    // Extract desktop-entry hint if present
    const char *desktop_id = NULL;
    if (hints) {
      GVariant *v =
          g_variant_lookup_value(hints, "desktop-entry", G_VARIANT_TYPE_STRING);
      if (v) {
        desktop_id = g_variant_get_string(v, NULL);
        // Note: v borrowed; do not free
      }
    }

    // If replacing, close previous
    Popup *prev = g_hash_table_lookup(gsv.popups, GUINT_TO_POINTER(id));
    if (prev) {
      gtk_revealer_set_reveal_child(prev->revealer, FALSE);
      g_timeout_add(140, (GSourceFunc)popup_destroy, prev);
    }

    Popup *p =
        popup_new(id, app_name, summary, body, desktop_id, expire_timeout);
    g_hash_table_insert(gsv.popups, GUINT_TO_POINTER(id), p);
    popup_add_to_ui(p);

    g_dbus_method_invocation_return_value(invoc, g_variant_new("(u)", id));
    return;
  }
  // Unknown method
  g_dbus_method_invocation_return_dbus_error(
      invoc, "org.freedesktop.DBus.Error.NotSupported", "Unknown method");
}

static const GDBusInterfaceVTable vtable = {.method_call = handle_method_call,
                                            .get_property = NULL,
                                            .set_property = NULL};

static void on_bus_acquired(GDBusConnection *conn, const char *name,
                            gpointer user_data) {
  gsv.bus = conn;
  introspection = g_dbus_node_info_new_for_xml(IFACE_XML, NULL);
  gsv.reg_id = g_dbus_connection_register_object(
      conn, "/org/freedesktop/Notifications", introspection->interfaces[0],
      &vtable, NULL, NULL, NULL);
  if (!gsv.reg_id) {
    g_error("Failed to register org.freedesktop.Notifications object");
  }
}

static void on_name_acquired(GDBusConnection *conn, const char *name,
                             gpointer user_data) {
  // Ready
}

static void on_name_lost(GDBusConnection *conn, const char *name,
                         gpointer user_data) {
  g_error("Failed to acquire name %s; another daemon may be running", name);
}

// ----- UI setup (layer-shell window, top-right anchor) -----

static void setup_window(GtkApplication *app) {
  gsv.window = gtk_window_new();
  gtk_window_set_application(GTK_WINDOW(gsv.window), app);
  gtk_window_set_decorated(GTK_WINDOW(gsv.window), FALSE);
  gtk_window_set_title(GTK_WINDOW(gsv.window), "NordNotify");
  gtk_widget_add_css_class(gsv.window, "layer-shell");

  // Layer-shell: overlay, top-right, no exclusive zone
  gtk_layer_init_for_window(GTK_WINDOW(gsv.window));
  gtk_layer_set_layer(GTK_WINDOW(gsv.window), GTK_LAYER_SHELL_LAYER_OVERLAY);
  gtk_layer_set_namespace(GTK_WINDOW(gsv.window), "nordnotify");
  gtk_layer_set_anchor(GTK_WINDOW(gsv.window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(gsv.window), GTK_LAYER_SHELL_EDGE_RIGHT,
                       TRUE);
  gtk_layer_set_margin(GTK_WINDOW(gsv.window), GTK_LAYER_SHELL_EDGE_TOP, 18);
  gtk_layer_set_margin(GTK_WINDOW(gsv.window), GTK_LAYER_SHELL_EDGE_RIGHT, 18);
  gtk_layer_set_exclusive_zone(GTK_WINDOW(gsv.window), 0);

  // Vertical box of notifications (newest prepended)
  gsv.vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(GTK_WINDOW(gsv.window), gsv.vbox);
}

static void on_activate(GtkApplication *app, gpointer user_data) {
  adw_init();
  load_css();
  setup_window(app);
  // Keep hidden until first notification; still needs a surface
  gtk_window_present(GTK_WINDOW(gsv.window));
  gtk_window_set_visible(GTK_WINDOW(gsv.window), TRUE);
}

int main(int argc, char **argv) {
  // App + state
  gsv.popups = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
                                     (GDestroyNotify)popup_free);
  gsv.next_id = 1;

  gsv.own_id =
      g_bus_own_name(G_BUS_TYPE_SESSION, "org.freedesktop.Notifications",
                     G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired,
                     on_name_acquired, on_name_lost, NULL, NULL);

  GtkApplication *app =
      gtk_application_new("com.prono.nordnotify", G_APPLICATION_NON_UNIQUE);
  gsv.app = app;
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);

  // Cleanup
  if (gsv.reg_id && gsv.bus)
    g_dbus_connection_unregister_object(gsv.bus, gsv.reg_id);
  if (introspection)
    g_dbus_node_info_unref(introspection);
  if (gsv.own_id)
    g_bus_unown_name(gsv.own_id);
  g_clear_object(&app);
  g_hash_table_destroy(gsv.popups);
  return status;
}

#include <gtk-layer-shell.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHARS 50
#define VERTICAL_OFFSET 35
#define UPDATE_INTERVAL 1000

// Default variables
static char text_color[32] = "#ffffff";
static int font_size = 14;
static char symbol_str[32] = "♪";

static GtkWidget *label;
static char last_text[512] = "";

// Decorate text with inputted symbols (sorry for grammer)
static char *decorate_text(const char *text) {
  size_t total_len =
      strlen(symbol_str) + 1 + strlen(text) + 1 + strlen(symbol_str) + 1;
  char *decorated = g_malloc(total_len);
  snprintf(decorated, total_len, "%s %s %s", symbol_str, text, symbol_str);
  return decorated;
}

static char *truncate_text(const char *text, int max_chars) {
  size_t len = strlen(text);
  if (len <= (size_t)max_chars)
    return g_strdup(text);
  char *out = g_malloc(max_chars + 4);
  strncpy(out, text, max_chars);
  strcpy(out + max_chars, "…");
  return out;
}

static gboolean change_text(gpointer data) {
  gtk_label_set_text(GTK_LABEL(label), last_text);
  gtk_widget_set_opacity(label, 1.0);
  return FALSE;
}

static gboolean update_now_playing(gpointer user_data) {
  FILE *fp = popen(
      "playerctl metadata --format '{{artist}} – {{title}}' 2>/dev/null", "r");
  char buf[512] = {0};

  if (fp && fgets(buf, sizeof(buf), fp)) {
    buf[strcspn(buf, "\n")] = '\0';
    if (strlen(buf) == 0)
      strcpy(buf, "Nothing playing");
  } else {
    strcpy(buf, "Nothing playing");
  }
  if (fp)
    pclose(fp);

  char *truncated = truncate_text(buf, MAX_CHARS);
  char *decorated = decorate_text(truncated);

  if (strcmp(decorated, last_text) != 0) {
    strcpy(last_text, decorated);
    gtk_widget_set_opacity(label, 0.0);
    g_timeout_add(150, change_text, NULL);
  }

  g_free(truncated);
  g_free(decorated);
  return TRUE;
}

int main(int argc, char *argv[]) {
  // Do the arguments
  if (argc > 1)
    strncpy(text_color, argv[1], sizeof(text_color) - 1);
  if (argc > 2)
    font_size = atoi(argv[2]);
  if (argc > 3)
    strncpy(symbol_str, argv[3], sizeof(symbol_str) - 1);

  gtk_init(&argc, &argv);

  GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
  gtk_widget_set_app_paintable(win, TRUE);

  gtk_layer_init_for_window(GTK_WINDOW(win));
  gtk_layer_set_layer(GTK_WINDOW(win), GTK_LAYER_SHELL_LAYER_OVERLAY);
  gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
  gtk_layer_set_exclusive_zone(GTK_WINDOW(win), -1);

  gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_BOTTOM,
                       VERTICAL_OFFSET);
  gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_TOP, 0);
  gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_LEFT, 0);
  gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_RIGHT, 0);

  GdkScreen *screen = gdk_screen_get_default();
  GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
  if (visual)
    gtk_widget_set_visual(win, visual);

  label = gtk_label_new("Nothing playing");
  gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
  gtk_widget_set_opacity(label, 1.0);

  char css[256];
  snprintf(css, sizeof(css),
           "label {"
           " color: %s;"
           " text-shadow: 2px 2px 4px rgba(0,0,0,0.7);"
           " font-weight: 500;"
           " font-size: %dpt;"
           "}",
           text_color, font_size);

  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(provider, css, -1, NULL);
  gtk_style_context_add_provider(gtk_widget_get_style_context(label),
                                 GTK_STYLE_PROVIDER(provider),
                                 GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref(provider);

  gtk_container_add(GTK_CONTAINER(win), label);

  g_timeout_add(UPDATE_INTERVAL, update_now_playing, NULL);
  g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show_all(win);
  gtk_main();
  return 0;
}

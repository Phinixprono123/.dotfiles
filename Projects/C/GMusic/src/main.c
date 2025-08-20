// src/main.c

#include "track.h"
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <gst/player/player.h>
#include <gtk/gtk.h>

typedef struct {
  GtkApplication *app;
  GtkWindow *win;
  GtkListView *list;
  GtkEntry *search;
  GtkImage *cover;
  GtkLabel *title_lbl;
  GtkLabel *meta_lbl;
  GtkScale *progress;
  GtkToggleButton *shuffle_btn;
  GtkToggleButton *repeat_btn;
  GtkButton *play_btn;
  GtkButton *prev_btn;
  GtkButton *next_btn;

  GListStore *store;
  GtkSingleSelection *sel;
  GstPlayer *player;
  gboolean playing;
  gboolean user_scrubbing;
  gboolean shuffle;
  gboolean repeat_one;

  GPtrArray *order;
  int current_index;
  guint pos_update_id;
} AppState;

/* Forward declarations */
static gboolean on_new_track(gpointer user_data);
static gboolean on_metadata_ready(gpointer user_data);
static void on_items_changed(GListModel *m, guint pos, guint removed,
                             guint added, gpointer u);
static void on_scrub_pressed(GtkGestureClick *g, gint n, gdouble x, gdouble y,
                             gpointer u);
static void on_scrub_released(GtkGestureClick *g, gint n, gdouble x, gdouble y,
                              gpointer u);
static void on_row_activated(GtkListView *list, guint pos, gpointer u);
static void on_play_clicked(GtkButton *btn, gpointer u);
static void on_next_clicked(GtkButton *btn, gpointer u);
static void on_prev_clicked(GtkButton *btn, gpointer u);
static void on_shuffle_toggled(GtkToggleButton *b, gpointer u);
static void on_repeat_toggled(GtkToggleButton *b, gpointer u);
static void on_search_changed(GtkEditable *ed, gpointer u);
static gboolean custom_filter_func(gpointer item, gpointer u);
static gboolean on_key_pressed(GtkEventControllerKey *key, guint keyval,
                               guint keycode, GdkModifierType state,
                               gpointer u);
static void on_eos(GstPlayer *p, gpointer u);
static void on_state_changed(GstPlayer *p, GstPlayerState state, gpointer u);
static void row_setup(GtkSignalListItemFactory *fac, GtkListItem *item,
                      gpointer d);
static void row_bind(GtkSignalListItemFactory *fac, GtkListItem *item,
                     gpointer d);
static GtkListItemFactory *create_row_factory_code(void);
static void load_css(void);
static GtkWidget *build_ui(AppState *S);
static void on_activate(GApplication *app, gpointer dir);
static void scan_dir_recursive(AppState *S, const char *root);
static void discover_track(AppState *S, Track *t);
static void scan_worker(GTask *task, gpointer source_object, gpointer task_data,
                        GCancellable *c);
static void start_scan(AppState *S, const char *root);

/* Utility functions */
static gboolean is_audio_ext(const char *name) {
  const char *dot = strrchr(name, '.');
  if (!dot)
    return FALSE;
  gchar *ext = g_ascii_strdown(dot + 1, -1);
  gboolean ok = g_strv_contains((const char *[]){"mp3", "flac", "ogg", "oga",
                                                 "opus", "m4a", "aac", "wav",
                                                 "wma", "alac", NULL},
                                ext);
  g_free(ext);
  return ok;
}

static char *file_to_uri(const char *path) {
  GFile *f = g_file_new_for_path(path);
  char *uri = g_file_get_uri(f);
  g_object_unref(f);
  return uri;
}

static GdkTexture *texture_from_bytes(GBytes *bytes) {
  if (!bytes)
    return NULL;
  GInputStream *in = g_memory_input_stream_new_from_bytes(bytes);
  GdkPixbuf *pb = gdk_pixbuf_new_from_stream(in, NULL, NULL);
  g_object_unref(in);
  if (!pb)
    return NULL;
  GdkTexture *tex = gdk_texture_new_for_pixbuf(pb);
  g_object_unref(pb);
  return tex;
}

/* Play‐order management */
static void ensure_order(AppState *S) {
  if (S->order)
    g_ptr_array_free(S->order, TRUE);
  guint n = g_list_model_get_n_items(G_LIST_MODEL(S->store));
  S->order = g_ptr_array_new_with_free_func(NULL);
  for (guint i = 0; i < n; i++)
    g_ptr_array_add(S->order, GINT_TO_POINTER((int)i));
  if (S->shuffle) {
    for (guint i = 0; i < n; i++) {
      guint j = g_random_int_range(i, n);
      gpointer tmp = S->order->pdata[i];
      S->order->pdata[i] = S->order->pdata[j];
      S->order->pdata[j] = tmp;
    }
  }
  S->current_index = 0;
}

/* items-changed callback */
static void on_items_changed(GListModel *m, guint pos, guint removed,
                             guint added, gpointer u) {
  ensure_order((AppState *)u);
}

/* Now-playing UI update */
static void set_now_playing(AppState *S, Track *t) {
  const char *title = track_get_title(t);
  if (!title || !*title) {
    char *b = g_path_get_basename(track_get_path(t));
    gtk_label_set_text(S->title_lbl, b);
    g_free(b);
  } else {
    gtk_label_set_text(S->title_lbl, title);
  }
  gchar *meta = g_strdup_printf(
      "%s • %s", track_get_artist(t)[0] ? track_get_artist(t) : "Unknown",
      track_get_album(t)[0] ? track_get_album(t) : "");
  gtk_label_set_text(S->meta_lbl, meta);
  g_free(meta);
  GdkTexture *tex = track_get_texture(t);
  if (tex)
    gtk_image_set_from_paintable(S->cover, GDK_PAINTABLE(tex));
  else
    gtk_image_set_from_icon_name(S->cover, "audio-x-generic-symbolic");
}

/* Scanning & metadata */
typedef struct {
  AppState *S;
  Track *t;
} NewTrack;
typedef struct {
  AppState *S;
  Track *t;
  gchar *title, *artist, *album;
  gint64 dur;
} MetaData;

static gboolean on_new_track(gpointer u) {
  NewTrack *nt = u;
  g_list_store_append(nt->S->store, nt->t);
  g_object_unref(nt->t);
  g_free(nt);
  return G_SOURCE_REMOVE;
}

static gboolean on_metadata_ready(gpointer u) {
  MetaData *md = u;
  track_set_metadata(md->t, md->title, md->artist, md->album, md->dur);
  g_object_unref(md->t);
  g_free(md->title);
  g_free(md->artist);
  g_free(md->album);
  g_free(md);
  return G_SOURCE_REMOVE;
}

static void discover_track(AppState *S, Track *t) {
  GstDiscoverer *disc = gst_discoverer_new(5 * GST_SECOND, NULL);
  if (!disc)
    return;
  GstDiscovererInfo *info =
      gst_discoverer_discover_uri(disc, track_get_uri(t), NULL);
  if (!info) {
    g_object_unref(disc);
    return;
  }

  const GstTagList *tags = gst_discoverer_info_get_tags(info);
  gchar *title = NULL, *artist = NULL, *album = NULL;
  if (tags) {
    gst_tag_list_get_string(tags, GST_TAG_TITLE, &title);
    gst_tag_list_get_string(tags, GST_TAG_ARTIST, &artist);
    gst_tag_list_get_string(tags, GST_TAG_ALBUM, &album);
    GValue img = G_VALUE_INIT;
    if (gst_tag_list_copy_value(&img, tags, GST_TAG_IMAGE) ||
        gst_tag_list_copy_value(&img, tags, GST_TAG_PREVIEW_IMAGE)) {
      GstSample *s = g_value_get_boxed(&img);
      if (s) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        GstMapInfo map;
        if (buf && gst_buffer_map(buf, &map, GST_MAP_READ)) {
          GBytes *b = g_bytes_new(map.data, map.size);
          GdkTexture *tex = texture_from_bytes(b);
          if (tex)
            track_set_texture(t, tex);
          g_bytes_unref(b);
          gst_buffer_unmap(buf, &map);
        }
      }
      g_value_unset(&img);
    }
  }

  gint64 dur = gst_discoverer_info_get_duration(info);
  MetaData *md = g_new0(MetaData, 1);
  md->S = S;
  md->t = g_object_ref(t);
  md->title = title;
  md->artist = artist;
  md->album = album;
  md->dur = dur;
  g_main_context_invoke(NULL, on_metadata_ready, md);

  g_object_unref(info);
  g_object_unref(disc);
}

static void scan_dir_recursive(AppState *S, const char *root) {
  GQueue q = G_QUEUE_INIT;
  g_queue_push_tail(&q, g_strdup(root));
  while (!g_queue_is_empty(&q)) {
    char *dir = g_queue_pop_head(&q);
    GDir *d = g_dir_open(dir, 0, NULL);
    if (!d) {
      g_free(dir);
      continue;
    }
    const char *name;
    while ((name = g_dir_read_name(d))) {
      if (g_str_has_prefix(name, "."))
        continue;
      char *full = g_build_filename(dir, name, NULL);
      if (g_file_test(full, G_FILE_TEST_IS_DIR)) {
        g_queue_push_tail(&q, full);
      } else if (is_audio_ext(name)) {
        char *uri = file_to_uri(full);
        Track *t = track_new(uri, full);
        g_free(uri);
        NewTrack *nt = g_new0(NewTrack, 1);
        nt->S = S;
        nt->t = g_object_ref(t);
        g_main_context_invoke(NULL, on_new_track, nt);
        discover_track(S, t);
        g_object_unref(t);
      }
      g_free(full);
    }
    g_dir_close(d);
    g_free(dir);
  }
}

static void scan_worker(GTask *task, gpointer so, gpointer td,
                        GCancellable *c) {
  AppState *S = so;
  char *root = td;
  scan_dir_recursive(S, root);
}

static void start_scan(AppState *S, const char *root) {
  GTask *task = g_task_new(S, NULL, NULL, NULL);
  g_task_set_task_data(task, g_strdup(root), g_free);
  g_task_run_in_thread(task, scan_worker);
  g_object_unref(task);
}

/* Playback control */
static int selected_index(AppState *S) {
  guint pos = gtk_single_selection_get_selected(S->sel);
  return pos == GTK_INVALID_LIST_POSITION ? -1 : (int)pos;
}

static void play_track(AppState *S, int idx) {
  if (idx < 0)
    return;
  guint n = g_list_model_get_n_items(G_LIST_MODEL(S->store));
  if ((guint)idx >= n)
    return;
  Track *t = g_list_model_get_item(G_LIST_MODEL(S->store), idx);
  gst_player_set_uri(S->player, track_get_uri(t));
  gst_player_play(S->player);
  set_now_playing(S, t);
  S->playing = TRUE;
  gtk_button_set_icon_name(S->play_btn, "media-playback-pause-symbolic");
  g_object_unref(t);
}

static void next_track(AppState *S) {
  if (!S->order)
    return;
  if (S->repeat_one) {
    play_track(S, GPOINTER_TO_INT(S->order->pdata[S->current_index]));
    return;
  }
  S->current_index = (S->current_index + 1) % S->order->len;
  play_track(S, GPOINTER_TO_INT(S->order->pdata[S->current_index]));
}

static void prev_track(AppState *S) {
  if (!S->order)
    return;
  if (S->repeat_one) {
    play_track(S, GPOINTER_TO_INT(S->order->pdata[S->current_index]));
    return;
  }
  S->current_index = (S->current_index - 1 + S->order->len) % S->order->len;
  play_track(S, GPOINTER_TO_INT(S->order->pdata[S->current_index]));
}

static gboolean on_pos_tick(gpointer data) {
  AppState *S = data;
  if (S->user_scrubbing)
    return G_SOURCE_CONTINUE;
  gint64 pos = gst_player_get_position(S->player);
  gint64 dur = gst_player_get_duration(S->player);
  double frac = dur > 0 ? (double)pos / (double)dur : 0.0;
  gtk_range_set_value(GTK_RANGE(S->progress), frac);
  return G_SOURCE_CONTINUE;
}

/* UI callbacks */
static void on_row_activated(GtkListView *list, guint pos, gpointer u) {
  AppState *S = u;
  ensure_order(S);
  for (guint i = 0; i < S->order->len; i++) {
    if (GPOINTER_TO_INT(S->order->pdata[i]) == (int)pos) {
      S->current_index = i;
      break;
    }
  }
  play_track(S, pos);
}

static void on_play_clicked(GtkButton *b, gpointer u) {
  AppState *S = u;
  if (S->playing) {
    gst_player_pause(S->player);
    S->playing = FALSE;
    gtk_button_set_icon_name(S->play_btn, "media-playback-start-symbolic");
  } else {
    gst_player_play(S->player);
    S->playing = TRUE;
    gtk_button_set_icon_name(S->play_btn, "media-playback-pause-symbolic");
  }
}

static void on_next_clicked(GtkButton *b, gpointer u) {
  next_track((AppState *)u);
}
static void on_prev_clicked(GtkButton *b, gpointer u) {
  prev_track((AppState *)u);
}

static void on_shuffle_toggled(GtkToggleButton *b, gpointer u) {
  AppState *S = u;
  S->shuffle = gtk_toggle_button_get_active(b);
  ensure_order(S);
}

static void on_repeat_toggled(GtkToggleButton *b, gpointer u) {
  AppState *S = u;
  S->repeat_one = gtk_toggle_button_get_active(b);
}

static void on_search_changed(GtkEditable *ed, gpointer u) {
  AppState *S = u;
  const char *q = gtk_editable_get_text(ed);
  GtkFilter *f = NULL;
  if (q && *q) {
    f = GTK_FILTER(
        gtk_custom_filter_new(custom_filter_func, g_strdup(q), g_free));
  }
  GtkFilterListModel *flm =
      GTK_FILTER_LIST_MODEL(gtk_single_selection_get_model(S->sel));
  gtk_filter_list_model_set_filter(flm, f);
}

static gboolean custom_filter_func(gpointer item, gpointer u) {
  Track *t = item;
  const char *needle = u;
  char *a = g_utf8_strdown(track_get_title(t), -1);
  char *b = g_utf8_strdown(track_get_artist(t), -1);
  char *c = g_utf8_strdown(track_get_album(t), -1);
  gboolean ok = (a && strstr(a, needle)) || (b && strstr(b, needle)) ||
                (c && strstr(c, needle));
  g_free(a);
  g_free(b);
  g_free(c);
  return ok;
}

static gboolean on_key_pressed(GtkEventControllerKey *k, guint keyval,
                               guint code, GdkModifierType st, gpointer u) {
  AppState *S = u;
  switch (keyval) {
  case GDK_KEY_Up:
  case GDK_KEY_Down:
    gtk_widget_grab_focus(GTK_WIDGET(S->list));
    return FALSE;
  case GDK_KEY_Return:
  case GDK_KEY_KP_Enter: {
    int idx = selected_index(S);
    if (idx >= 0) {
      ensure_order(S);
      for (guint i = 0; i < S->order->len; i++) {
        if (GPOINTER_TO_INT(S->order->pdata[i]) == idx)
          S->current_index = i;
      }
      play_track(S, idx);
    }
  }
    return TRUE;
  case GDK_KEY_space:
    on_play_clicked(S->play_btn, S);
    return TRUE;
  case GDK_KEY_Left:
    gst_player_seek(S->player, gst_player_get_position(S->player) -
                                   ((st & GDK_SHIFT_MASK) ? 15 * GST_SECOND
                                                          : 5 * GST_SECOND));
    return TRUE;
  case GDK_KEY_Right:
    gst_player_seek(S->player, gst_player_get_position(S->player) +
                                   ((st & GDK_SHIFT_MASK) ? 15 * GST_SECOND
                                                          : 5 * GST_SECOND));
    return TRUE;
  case GDK_KEY_n:
    next_track(S);
    return TRUE;
  case GDK_KEY_p:
    prev_track(S);
    return TRUE;
  case GDK_KEY_r:
    gtk_toggle_button_set_active(S->repeat_btn,
                                 !gtk_toggle_button_get_active(S->repeat_btn));
    return TRUE;
  case GDK_KEY_s:
    gtk_toggle_button_set_active(S->shuffle_btn,
                                 !gtk_toggle_button_get_active(S->shuffle_btn));
    return TRUE;
  case GDK_KEY_slash:
    gtk_widget_grab_focus(GTK_WIDGET(S->search));
    return TRUE;
  case GDK_KEY_Escape:
    gtk_editable_set_text(GTK_EDITABLE(S->search), "");
    gtk_widget_grab_focus(GTK_WIDGET(S->list));
    return TRUE;
  }
  return FALSE;
}

/* GStreamer events */
static void on_eos(GstPlayer *p, gpointer u) { next_track((AppState *)u); }
static void on_state_changed(GstPlayer *p, GstPlayerState s, gpointer u) {
  AppState *S = u;
  S->playing = (s == GST_PLAYER_STATE_PLAYING);
  gtk_button_set_icon_name(S->play_btn, S->playing
                                            ? "media-playback-pause-symbolic"
                                            : "media-playback-start-symbolic");
}

/* Row factory */
static void row_setup(GtkSignalListItemFactory *fac, GtkListItem *item,
                      gpointer d) {
  GtkBuilder *b = gtk_builder_new_from_string(
      "<interface>"
      " <object class='GtkBox' id='row'>"
      "  <property name='spacing'>12</property>"
      "  <property name='margin-start'>12</property>"
      "  <property name='margin-end'>12</property>"
      "  <property name='margin-top'>6</property>"
      "  <property name='margin-bottom'>6</property>"
      "  <child><object class='GtkImage' id='img'>"
      "   <property name='icon-name'>audio-x-generic-symbolic</property>"
      "   <property name='pixel-size'>48</property>"
      "   <style><class name='row-cover'/></style>"
      "  </object></child>"
      "  <child><object class='GtkBox'>"
      "   <property name='orientation'>vertical</property>"
      "   <property name='spacing'>2</property>"
      "   <child><object class='GtkLabel' id='title'>"
      "    <property name='xalign'>0</property>"
      "    <property name='ellipsize'>end</property>"
      "    <style><class name='row-title'/></style>"
      "   </object></child>"
      "   <child><object class='GtkLabel' id='sub'>"
      "    <property name='xalign'>0</property>"
      "    <property name='ellipsize'>end</property>"
      "    <style><class name='row-subtitle'/></style>"
      "   </object></child>"
      "  </object></child>"
      "</object>"
      "</interface>",
      -1);
  GtkWidget *row = GTK_WIDGET(gtk_builder_get_object(b, "row"));
  gtk_list_item_set_child(item, row);
  g_object_unref(b);
}

static void row_bind(GtkSignalListItemFactory *fac, GtkListItem *item,
                     gpointer d) {
  Track *t = gtk_list_item_get_item(item);
  GtkWidget *row = gtk_list_item_get_child(item);
  GtkWidget *img = gtk_widget_get_first_child(row);
  GtkWidget *vbox = gtk_widget_get_next_sibling(img);
  GtkLabel *title = GTK_LABEL(gtk_widget_get_first_child(vbox));
  GtkLabel *sub = GTK_LABEL(gtk_widget_get_next_sibling(GTK_WIDGET(title)));

  const char *ttl = track_get_title(t);
  if (!ttl || !*ttl) {
    char *b = g_path_get_basename(track_get_path(t));
    gtk_label_set_text(title, b);
    g_free(b);
  } else
    gtk_label_set_text(title, ttl);

  gchar *txt = g_strdup_printf(
      "%s — %s", track_get_artist(t)[0] ? track_get_artist(t) : "Unknown",
      track_get_album(t)[0] ? track_get_album(t) : "");
  gtk_label_set_text(sub, txt);
  g_free(txt);

  GdkTexture *tex = track_get_texture(t);
  if (tex)
    gtk_image_set_from_paintable(GTK_IMAGE(img), GDK_PAINTABLE(tex));
  else
    gtk_image_set_from_icon_name(GTK_IMAGE(img), "audio-x-generic-symbolic");
}

static GtkListItemFactory *create_row_factory_code(void) {
  GtkListItemFactory *fac = gtk_signal_list_item_factory_new();
  g_signal_connect(fac, "setup", G_CALLBACK(row_setup), NULL);
  g_signal_connect(fac, "bind", G_CALLBACK(row_bind), NULL);
  return fac;
}

/* Theming */
static void load_css(void) {
  const char *c[] = {g_getenv("HYPERMUSIC_CSS"), "theme.css",
                     g_build_filename(g_get_user_config_dir(), "hyper-music",
                                      "theme.css", NULL),
                     NULL};
  GtkCssProvider *prov = gtk_css_provider_new();
  gboolean ok = FALSE;
  for (int i = 0; c[i]; i++) {
    if (c[i] && g_file_test(c[i], G_FILE_TEST_EXISTS)) {
      gtk_css_provider_load_from_path(prov, c[i]);
      ok = TRUE;
      break;
    }
  }
  if (!ok) {
    /* load the built-in CSS blob (must be zero-terminated) */
    GBytes *b = g_resources_lookup_data("/css/style.css",
                                        G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
    const char *css = (const char *)g_bytes_get_data(b, NULL);
    gtk_css_provider_load_from_string(prov, css);
    g_bytes_unref(b);
  }

  gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                             GTK_STYLE_PROVIDER(prov),
                                             GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref(prov);
}

/* UI construction */
static GtkWidget *build_ui(AppState *S) {
  GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(root, "app-root");

  GtkWidget *hdr = gtk_header_bar_new();
  gtk_widget_add_css_class(hdr, "app-header");
  gtk_box_append(GTK_BOX(root), hdr);

  S->search = GTK_ENTRY(gtk_entry_new());
  gtk_widget_add_css_class(GTK_WIDGET(S->search), "search-entry");
  gtk_entry_set_placeholder_text(S->search, "Search title, artist, album");
  g_signal_connect(S->search, "changed", G_CALLBACK(on_search_changed), S);

  S->store = g_list_store_new(TYPE_TRACK);
  GtkFilterListModel *flm =
      gtk_filter_list_model_new(G_LIST_MODEL(S->store), NULL);
  S->sel = gtk_single_selection_new(G_LIST_MODEL(flm));
  S->list = GTK_LIST_VIEW(gtk_list_view_new(GTK_SELECTION_MODEL(S->sel),
                                            create_row_factory_code()));
  gtk_list_view_set_enable_rubberband(S->list, FALSE);
  gtk_list_view_set_single_click_activate(S->list, TRUE);
  g_signal_connect(S->list, "activate", G_CALLBACK(on_row_activated), S);

  GtkWidget *sc = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sc), GTK_WIDGET(S->list));
  gtk_widget_add_css_class(sc, "track-list");

  GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_top(right, 12);
  gtk_widget_set_margin_bottom(right, 12);
  gtk_widget_set_margin_start(right, 12);
  gtk_widget_set_margin_end(right, 12);

  S->cover = GTK_IMAGE(gtk_image_new());
  gtk_widget_set_size_request(GTK_WIDGET(S->cover), 256, 256);
  gtk_widget_add_css_class(GTK_WIDGET(S->cover), "cover");
  gtk_box_append(GTK_BOX(right), GTK_WIDGET(S->cover));

  S->title_lbl = GTK_LABEL(gtk_label_new("Select a track"));
  gtk_widget_add_css_class(GTK_WIDGET(S->title_lbl), "title");
  gtk_label_set_xalign(S->title_lbl, 0.0);
  gtk_box_append(GTK_BOX(right), GTK_WIDGET(S->title_lbl));

  S->meta_lbl = GTK_LABEL(gtk_label_new(""));
  gtk_widget_add_css_class(GTK_WIDGET(S->meta_lbl), "meta");
  gtk_label_set_xalign(S->meta_lbl, 0.0);
  gtk_box_append(GTK_BOX(right), GTK_WIDGET(S->meta_lbl));

  S->progress = GTK_SCALE(
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.001));
  gtk_scale_set_draw_value(S->progress, FALSE);
  gtk_widget_add_css_class(GTK_WIDGET(S->progress), "progress");
  gtk_box_append(GTK_BOX(right), GTK_WIDGET(S->progress));

  GtkGesture *scrub = gtk_gesture_click_new();
  g_signal_connect(scrub, "pressed", G_CALLBACK(on_scrub_pressed), S);
  g_signal_connect(scrub, "released", G_CALLBACK(on_scrub_released), S);
  gtk_widget_add_controller(GTK_WIDGET(S->progress),
                            GTK_EVENT_CONTROLLER(scrub));

  GtkWidget *ctrls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  S->prev_btn =
      GTK_BUTTON(gtk_button_new_from_icon_name("media-skip-backward-symbolic"));
  S->play_btn = GTK_BUTTON(
      gtk_button_new_from_icon_name("media-playback-start-symbolic"));
  S->next_btn =
      GTK_BUTTON(gtk_button_new_from_icon_name("media-skip-forward-symbolic"));
  S->shuffle_btn =
      GTK_TOGGLE_BUTTON(gtk_toggle_button_new_with_label("Shuffle"));
  S->repeat_btn = GTK_TOGGLE_BUTTON(gtk_toggle_button_new_with_label("Repeat"));

  g_signal_connect(S->prev_btn, "clicked", G_CALLBACK(on_prev_clicked), S);
  g_signal_connect(S->play_btn, "clicked", G_CALLBACK(on_play_clicked), S);
  g_signal_connect(S->next_btn, "clicked", G_CALLBACK(on_next_clicked), S);
  g_signal_connect(S->shuffle_btn, "toggled", G_CALLBACK(on_shuffle_toggled),
                   S);
  g_signal_connect(S->repeat_btn, "toggled", G_CALLBACK(on_repeat_toggled), S);

  gtk_box_append(GTK_BOX(ctrls), GTK_WIDGET(S->prev_btn));
  gtk_box_append(GTK_BOX(ctrls), GTK_WIDGET(S->play_btn));
  gtk_box_append(GTK_BOX(ctrls), GTK_WIDGET(S->next_btn));
  gtk_box_append(GTK_BOX(ctrls), GTK_WIDGET(S->shuffle_btn));
  gtk_box_append(GTK_BOX(ctrls), GTK_WIDGET(S->repeat_btn));
  gtk_box_append(GTK_BOX(right), ctrls);

  GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_hexpand(paned, TRUE);
  gtk_widget_set_vexpand(paned, TRUE);
  gtk_paned_set_start_child(GTK_PANED(paned), sc);

  GtkWidget *rc = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_append(GTK_BOX(rc), GTK_WIDGET(S->search));
  gtk_box_append(GTK_BOX(rc), right);
  gtk_paned_set_end_child(GTK_PANED(paned), rc);

  gtk_box_append(GTK_BOX(root), paned);

  GtkEventController *key = gtk_event_controller_key_new();
  g_signal_connect(key, "key-pressed", G_CALLBACK(on_key_pressed), S);
  gtk_widget_add_controller(root, key);

  return root;
}

/* Application entry */
static void on_activate(GApplication *app, gpointer dir) {
  AppState *S = g_new0(AppState, 1);
  S->app = GTK_APPLICATION(app);

  load_css();

  S->win = GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(app)));
  gtk_window_set_default_size(S->win, 1100, 680);
  gtk_window_set_title(S->win, "Hyper Music");
  gtk_window_set_child(S->win, build_ui(S));

  gst_init(NULL, NULL);
  S->player = gst_player_new(NULL, NULL);
  g_signal_connect(S->player, "end-of-stream", G_CALLBACK(on_eos), S);
  g_signal_connect(S->player, "state-changed", G_CALLBACK(on_state_changed), S);

  S->pos_update_id = g_timeout_add(200, on_pos_tick, S);
  g_signal_connect(S->store, "items-changed", G_CALLBACK(on_items_changed), S);

  gtk_window_present(S->win);

  const char *music_dir =
      dir ? (const char *)dir
          : g_build_filename(g_get_home_dir(), "Music", NULL);
  start_scan(S, music_dir);
}

int main(int argc, char **argv) {
  GtkApplication *app =
      gtk_application_new("dev.hyper.music", G_APPLICATION_HANDLES_OPEN);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate),
                   (gpointer)(argc > 1 ? argv[1] : NULL));
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}

/* Scrub gesture callbacks */
static void on_scrub_pressed(GtkGestureClick *g, gint n, gdouble x, gdouble y,
                             gpointer u) {
  AppState *S = u;
  S->user_scrubbing = TRUE;
}

static void on_scrub_released(GtkGestureClick *g, gint n, gdouble x, gdouble y,
                              gpointer u) {
  AppState *S = u;
  double frac = gtk_range_get_value(GTK_RANGE(S->progress));
  gint64 dur = gst_player_get_duration(S->player);
  if (dur > 0)
    gst_player_seek(S->player, (gint64)(frac * dur));
  S->user_scrubbing = FALSE;
}

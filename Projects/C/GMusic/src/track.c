#include "track.h"

struct _Track {
  GObject parent_instance;

  char *uri;
  char *path;
  char *title;
  char *artist;
  char *album;
  gint64 duration_usec;
  GdkTexture *texture;
};

G_DEFINE_TYPE(Track, track, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_URI,
  PROP_PATH,
  PROP_TITLE,
  PROP_ARTIST,
  PROP_ALBUM,
  PROP_DURATION,
  PROP_TEXTURE,
  N_PROPS
};

static GParamSpec *props[N_PROPS];

static void track_finalize(GObject *obj) {
  Track *self = (Track *)obj;
  g_clear_pointer(&self->uri, g_free);
  g_clear_pointer(&self->path, g_free);
  g_clear_pointer(&self->title, g_free);
  g_clear_pointer(&self->artist, g_free);
  g_clear_pointer(&self->album, g_free);
  g_clear_object(&self->texture);
  G_OBJECT_CLASS(track_parent_class)->finalize(obj);
}

static void track_get_property(GObject *obj, guint prop_id, GValue *val,
                               GParamSpec *pspec) {
  Track *self = (Track *)obj;
  switch (prop_id) {
  case PROP_URI:
    g_value_set_string(val, self->uri);
    break;
  case PROP_PATH:
    g_value_set_string(val, self->path);
    break;
  case PROP_TITLE:
    g_value_set_string(val, self->title);
    break;
  case PROP_ARTIST:
    g_value_set_string(val, self->artist);
    break;
  case PROP_ALBUM:
    g_value_set_string(val, self->album);
    break;
  case PROP_DURATION:
    g_value_set_int64(val, self->duration_usec);
    break;
  case PROP_TEXTURE:
    g_value_set_object(val, self->texture);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
  }
}

static void track_set_property(GObject *obj, guint prop_id, const GValue *val,
                               GParamSpec *pspec) {
  Track *self = (Track *)obj;
  switch (prop_id) {
  case PROP_URI:
    g_free(self->uri);
    self->uri = g_value_dup_string(val);
    break;
  case PROP_PATH:
    g_free(self->path);
    self->path = g_value_dup_string(val);
    break;
  case PROP_TITLE:
    g_free(self->title);
    self->title = g_value_dup_string(val);
    break;
  case PROP_ARTIST:
    g_free(self->artist);
    self->artist = g_value_dup_string(val);
    break;
  case PROP_ALBUM:
    g_free(self->album);
    self->album = g_value_dup_string(val);
    break;
  case PROP_DURATION:
    self->duration_usec = g_value_get_int64(val);
    break;
  case PROP_TEXTURE:
    g_set_object(&self->texture, g_value_get_object(val));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
  }
}

static void track_class_init(TrackClass *klass) {
  GObjectClass *oc = G_OBJECT_CLASS(klass);
  oc->finalize = track_finalize;
  oc->get_property = track_get_property;
  oc->set_property = track_set_property;

  props[PROP_URI] = g_param_spec_string("uri", "Uri", "File URI", NULL,
                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  props[PROP_PATH] =
      g_param_spec_string("path", "Path", "Filesystem path", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  props[PROP_TITLE] = g_param_spec_string("title", "Title", "Track title", NULL,
                                          G_PARAM_READWRITE);
  props[PROP_ARTIST] = g_param_spec_string("artist", "Artist", "Artist", NULL,
                                           G_PARAM_READWRITE);
  props[PROP_ALBUM] =
      g_param_spec_string("album", "Album", "Album", NULL, G_PARAM_READWRITE);
  props[PROP_DURATION] =
      g_param_spec_int64("duration", "Duration", "Microseconds", 0, G_MAXINT64,
                         0, G_PARAM_READWRITE);
  props[PROP_TEXTURE] =
      g_param_spec_object("texture", "Texture", "Cover texture",
                          GDK_TYPE_TEXTURE, G_PARAM_READWRITE);

  g_object_class_install_properties(oc, N_PROPS, props);
}

static void track_init(Track *self) {}

Track *track_new(const char *uri, const char *path) {
  return g_object_new(TYPE_TRACK, "uri", uri, "path", path, NULL);
}

void track_set_metadata(Track *self, const char *title, const char *artist,
                        const char *album, gint64 duration_usec) {
  g_object_set(self, "title", title ? title : "", "artist",
               artist ? artist : "", "album", album ? album : "", "duration",
               duration_usec, NULL);
}

void track_set_texture(Track *self, GdkTexture *tex) {
  g_object_set(self, "texture", tex, NULL);
}

const char *track_get_uri(Track *self) { return self->uri; }
const char *track_get_path(Track *self) { return self->path; }
const char *track_get_title(Track *self) {
  return self->title ? self->title : "";
}
const char *track_get_artist(Track *self) {
  return self->artist ? self->artist : "";
}
const char *track_get_album(Track *self) {
  return self->album ? self->album : "";
}
gint64 track_get_duration(Track *self) { return self->duration_usec; }
GdkTexture *track_get_texture(Track *self) { return self->texture; }

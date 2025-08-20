#pragma once
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_TRACK (track_get_type())
G_DECLARE_FINAL_TYPE(Track, track, TRACK, OBJECT, GObject)

Track *track_new(const char *uri, const char *path);
void track_set_metadata(Track *self, const char *title, const char *artist,
                        const char *album, gint64 duration_usec);
void track_set_texture(Track *self, GdkTexture *tex);

const char *track_get_uri(Track *self);
const char *track_get_path(Track *self);
const char *track_get_title(Track *self);
const char *track_get_artist(Track *self);
const char *track_get_album(Track *self);
gint64 track_get_duration(Track *self);
GdkTexture *track_get_texture(Track *self);

G_END_DECLS

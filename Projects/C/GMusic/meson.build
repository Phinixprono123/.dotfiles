project('hyper-music', 'c', version: '0.1',
  default_options: ['c_std=c11'])

gtk4        = dependency('gtk4')
gst_player  = dependency('gstreamer-player-1.0')
gst_pbutils = dependency('gstreamer-pbutils-1.0')
glib        = dependency('glib-2.0')
gio         = dependency('gio-2.0')
gdkpixbuf   = dependency('gdk-pixbuf-2.0')

src = [
  'src/main.c',
  'src/track.c',
]

executable('hyper-music', src,
  dependencies: [gtk4, gst_player, gst_pbutils, glib, gio, gdkpixbuf],
  install: true)

install_data('data/style.css',
  install_dir: join_paths(get_option('prefix'),
                          'share', 'hyper-music'))


from gi.repository import Gtk, GLib
import os

THEME_DIR = os.path.expanduser("~/.config/themes")

class HyprThemeSwitcher(Gtk.ApplicationWindow):
    def __init__(self, app):
        Gtk.ApplicationWindow.__init__(self, application=app)
        self.set_title("Theme Switcher")
        self.set_default_size(460, 280)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12, margin=16)
        self.set_child(box)

        self.dropdown = Gtk.DropDown.new_from_strings(self.get_themes())
        box.append(Gtk.Label(label="Select Theme", halign=Gtk.Align.START))
        box.append(self.dropdown)

        apply_btn = Gtk.Button(label="Apply Theme")
        apply_btn.connect("clicked", self.apply_theme)
        box.append(apply_btn)

    def get_themes(self):
        return [f for f in os.listdir(THEME_DIR) if os.path.isdir(os.path.join(THEME_DIR, f))]

    def apply_theme(self, btn):
        theme = self.dropdown.get_selected_item().get_string()
        theme_path = os.path.join(THEME_DIR, theme)

        # Use GLib.spawn_async for non-blocking calls
        def run_cmd(cmd): GLib.spawn_async(cmd, flags=GLib.SpawnFlags.DO_NOT_REAP_CHILD)

        # Hyprland
        run_cmd(["cp", f"{theme_path}/hyprland.conf", os.path.expanduser("~/.config/hypr/hyprland.conf")])
        run_cmd(["hyprctl", "reload"])

        # Waybar (check for modified CSS first)
        css_src = os.path.join(theme_path, "waybar.css")
        css_dst = os.path.expanduser("~/.config/waybar/style.css")
        if os.path.exists(css_src):
            run_cmd(["cp", css_src, css_dst])
            run_cmd(["pkill", "-f", "waybar && waybar &"])

        # Wallpaper
        run_cmd(["swww", "img", os.path.join(theme_path, "wallpaper.jpg"), "--transition-type", "fade"])

        # GTK Theme (auto-detect accent from current)
        gtk_file = os.path.join(theme_path, "gtk-theme.txt")
        if os.path.exists(gtk_file):
            with open(gtk_file) as f:
                gtk_theme = f.read().strip()
                run_cmd(["gsettings", "set", "org.gnome.desktop.interface", "gtk-theme", gtk_theme])

        run_cmd(["notify-send", f"Theme switched to {theme}"])

class App(Gtk.Application):
    def __init__(self):
        Gtk.Application.__init__(self, application_id="com.prono.theme-switcher")

    def do_activate(self):
        win = HyprThemeSwitcher(self)
        win.present()

App().run()


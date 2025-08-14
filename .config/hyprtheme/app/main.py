#!/usr/bin/env python3
import os
import sys
import threading
from pathlib import Path

sys.path.append(os.path.dirname(__file__))

import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
from engine import THEME_ROOT, apply_theme, get_turbo
from gi.repository import Adw, Gio, GLib, Gtk

APP_TITLE = "HyprTheme"

# ---- ThemeCard ----

class ThemeCard(Gtk.Button):
    def __init__(self, appwin, theme, slide_in=False):
        super().__init__()
        self.appwin = appwin
        self.theme = theme
        self.busy = False

        self.set_can_focus(True)
        self.set_focus_on_click(True)
        self.add_css_class("card")
        self.add_css_class("clickable")
        if slide_in:
            self.add_css_class("slide-in")

        pic = Gtk.Picture()
        pic.set_content_fit(Gtk.ContentFit.COVER)
        wp = theme.get("preview") or theme.get("wallpaper", "")
        if wp and Path(wp).exists():
            pic.set_filename(wp)
        pic.set_hexpand(True)
        pic.set_vexpand(True)
        pic.add_css_class("card-image")

        overlay = Gtk.Overlay()
        overlay.set_child(pic)

        gradient = Gtk.Box()
        gradient.add_css_class("gradient-bottom")
        gradient.set_hexpand(True)
        gradient.set_halign(Gtk.Align.FILL)
        gradient.set_valign(Gtk.Align.END)
        gradient.set_size_request(-1, 96)
        overlay.add_overlay(gradient)

        header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        header.add_css_class("card-header")
        header.set_hexpand(True)
        header.set_halign(Gtk.Align.FILL)
        header.set_valign(Gtk.Align.END)
        header.set_margin_start(12)
        header.set_margin_end(12)
        header.set_margin_bottom(12)

        title = Gtk.Label(label=theme.get("name", theme["id"]))
        title.add_css_class("title")
        title.set_xalign(0.0)

        apply_btn = Gtk.Button(label="Apply")
        apply_btn.add_css_class("pill")
        apply_btn.connect("clicked", self.on_apply_clicked)

        header.append(title)
        header.append(apply_btn)
        overlay.add_overlay(header)

        self.apply_overlay = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        self.apply_overlay.set_halign(Gtk.Align.CENTER)
        self.apply_overlay.set_valign(Gtk.Align.CENTER)

        spinner = Gtk.Spinner()
        spinner.set_spinning(True)
        self.apply_overlay.append(spinner)

        label = Gtk.Label(label="Applying…")
        label.add_css_class("title")
        self.apply_overlay.append(label)

        self.apply_overlay.set_visible(False)
        overlay.add_overlay(self.apply_overlay)

        self.set_child(overlay)
        self.connect("clicked", self.on_apply_clicked)

    def set_busy(self, busy: bool):
        self.busy = busy
        self.set_sensitive(not busy)
        self.apply_overlay.set_visible(busy)

    def on_apply_clicked(self, _btn):
        if self.busy:
            return
        self.set_busy(True)
        self.appwin.toast(f"Applying {self.theme.get('name', self.theme['id'])}…")

        def worker():
            err = None
            try:
                apply_theme(self.theme["id"])
            except Exception as e:
                err = e

            def done():
                self.set_busy(False)
                if err:
                    self.appwin.toast(f"Failed: {err}")
                else:
                    self.appwin.toast(f"Applied {self.theme.get('name', self.theme['id'])}")
                return False
            GLib.idle_add(done)

        threading.Thread(target=worker, daemon=True).start()

# ---- ThemeWatcher ----

class ThemeWatcher:
    def __init__(self, on_change):
        self.on_change = on_change
        self.monitors = []
        self.debounce_id = 0
        self.setup()

    def setup(self):
        self.dispose()
        root_file = Gio.File.new_for_path(str(THEME_ROOT))
        self.monitors.append(self._make_monitor(root_file))
        if THEME_ROOT.exists():
            for p in sorted([x for x in THEME_ROOT.iterdir() if x.is_dir()]):
                self.monitors.append(self._make_monitor(Gio.File.new_for_path(str(p))))

    def _make_monitor(self, gio_file: Gio.File):
        try:
            m = gio_file.monitor_directory(Gio.FileMonitorFlags.NONE, None)
            m.connect("changed", self._on_changed)
            return m
        except Exception:
            return None

    def _on_changed(self, *_args):
        if self.debounce_id:
            return
        def fire():
            self.debounce_id = 0
            self.on_change()
            self.setup()
            return False
        self.debounce_id = GLib.timeout_add(220, fire)

    def dispose(self):
        for m in self.monitors:
            try:
                if m:
                    m.cancel()
            except Exception:
                pass
        self.monitors = []
        if self.debounce_id:
            GLib.source_remove(self.debounce_id)
            self.debounce_id = 0

# ---- AppWindow ----

class AppWindow(Adw.ApplicationWindow):
    def __init__(self, app):
        super().__init__(application=app, title=APP_TITLE, default_width=960, default_height=680)
        self.set_resizable(False)
        self.set_decorated(False)
        self.add_css_class("surface")

        self.known_ids = set()
        self.filter_text = ""
        self._refresh_icon = None
        self._refresh_spin_off_id = 0
        self._slider_handler_id = 0

        hb = Adw.HeaderBar()
        hb.add_css_class("flat")
        title = Gtk.Label(label=APP_TITLE)
        title.add_css_class("hb-title")
        hb.set_title_widget(title)

        self.search = Gtk.Entry()
        self.search.set_placeholder_text("Filter themes…")
        self.search.set_width_chars(18)
        self.search.connect("changed", self.on_filter_changed)
        hb.pack_start(self.search)

        self.turbo_btn = Gtk.ToggleButton.new()
        self.turbo_btn.set_icon_name("battery-full-charged-symbolic")
        self.turbo_btn.set_tooltip_text("Turbo apply: hardlinks + parallel I/O")
        self.turbo_btn.set_active(get_turbo())
        self.turbo_btn.connect("toggled", self.on_turbo_toggled)
        hb.pack_end(self.turbo_btn)

        refresh_btn = Gtk.Button.new_from_icon_name("view-refresh-symbolic")
        refresh_btn.set_tooltip_text("Reload themes")
        refresh_btn.connect("clicked", self.on_refresh_clicked)
        hb.pack_end(refresh_btn)
        self._refresh_icon = refresh_btn

        self.flow = Gtk.FlowBox()
        self.flow.set_selection_mode(Gtk.SelectionMode.NONE)
        self.flow.set_column_spacing(16)
        self.flow.set_row_spacing(16)
        self.flow.set_valign(Gtk.Align.START)
        self.flow.set_max_children_per_line(3)
        self.flow.set_min_children_per_line(2)

        scroll_ctrl = Gtk.EventControllerScroll.new(Gtk.EventControllerScrollFlags.VERTICAL)
        scroll_ctrl.connect("scroll", self.on_scroll)
        self.flow.add_controller(scroll_ctrl)

        scroller = Gtk.ScrolledWindow()
        scroller.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scroller.set_child(self.flow)
        self.scroller = scroller

        nav = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        nav.add_css_class("navbar")
        self.slider = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL, 0.0, 1.0, 0.001)
        self.slider.set_hexpand(True)
        nav.append(self.slider)

        self.toast_overlay = Adw.ToastOverlay()
        root = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        root.append(hb)
        root.append(scroller)
        root.append(nav)
        self.toast_overlay.set_child(root)
        self.set_content(self.toast_overlay)

        key_controller = Gtk.EventControllerKey.new()
        key_controller.connect("key-pressed", self.on_key_pressed)
        self.add_controller(key_controller)

        self.load_css()
        self.populate()

        self.watcher = ThemeWatcher(self.on_external_change)
        self.bind_slider()

    def add_refresh_spin(self, on=True):
        if not self._refresh_icon:
            return
        if on:
            self._refresh_icon.add_css_class("refreshing")
        else:
            self._refresh_icon.remove_css_class("refreshing")

    def on_external_change(self):
        self.add_refresh_spin(True)
        self.toast("Theme folder changed — updating…")
        self.populate(slide_new=True)
        if self._refresh_spin_off_id:
            GLib.source_remove(self._refresh_spin_off_id)
        self._refresh_spin_off_id = GLib.timeout_add(600, lambda: (self.add_refresh_spin(False), False)[1])

    def load_css(self):
        css_dir = Path(__file__).parent
        provider = Gtk.CssProvider()
        css = ""
        vars_file


if __name__ == "__main__":
    app = App()
    app.run()


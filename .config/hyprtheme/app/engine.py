import json
import shutil
import subprocess
import time
import os
import hashlib
import threading
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor

# Optional: Gio for fast, in-process gsettings
try:
    import gi
    gi.require_version("Gio", "2.0")
    from gi.repository import Gio  # type: ignore
    _HAVE_GIO = True
except Exception:
    _HAVE_GIO = False

HOME = Path.home()
CONFIG = HOME / ".config"
THEME_ROOT = CONFIG / "hyprtheme" / "themes"
STATE_DIR = CONFIG / "hyprtheme"
STATE_FILE = STATE_DIR / "state.json"

HYPR_DIR = CONFIG / "hypr"
HYPR_CONF = HYPR_DIR / "hyprland.conf"

WAYBAR_DIR = CONFIG / "waybar"
WAYBAR_THEME = WAYBAR_DIR / "theme.css"

GHOSTTY_DIR = CONFIG / "ghostty"
GHOSTTY_CONFIG_DEFAULT = GHOSTTY_DIR / "config"

APP_ID = "com.prono.HyprTheme"

# In-memory theme cache
_THEME_CACHE_SIG = None
_THEME_CACHE = None

# Turbo flag (persisted in state)
_TURBO = True


def sh(cmd, check=True):
    return subprocess.run(
        cmd, shell=True, check=check,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )


# ---------- prefs ----------

def _ensure_state_dir():
    STATE_DIR.mkdir(parents=True, exist_ok=True)


def load_state() -> dict:
    try:
        if STATE_FILE.exists():
            return json.loads(STATE_FILE.read_text(encoding="utf-8"))
    except Exception:
        pass
    return {}


def save_state(state: dict):
    try:
        _ensure_state_dir()
        STATE_FILE.write_text(json.dumps(state, indent=2), encoding="utf-8")
    except Exception as e:
        print(f"[state] failed to save: {e}")


def set_turbo(enabled: bool):
    global _TURBO
    _TURBO = bool(enabled)
    st = load_state()
    st["turbo"] = _TURBO
    save_state(st)


def get_turbo() -> bool:
    global _TURBO
    st = load_state()
    if "turbo" in st:
        _TURBO = bool(st["turbo"])
    return _TURBO


# ---------- perf helpers ----------

def fast_hash(s: str) -> str:
    return hashlib.sha1(s.encode("utf-8")).hexdigest()


def file_checksum(path: Path) -> str:
    try:
        h = hashlib.sha256()
        with path.open("rb") as f:
            for chunk in iter(lambda: f.read(262144), b""):
                h.update(chunk)
        return h.hexdigest()
    except Exception:
        return ""


def files_are_same(src: Path, dst: Path) -> bool:
    if not (src.exists() and dst.exists() and src.is_file() and dst.is_file()):
        return False
    ss, ds = src.stat(), dst.stat()
    if ss.st_size != ds.st_size:
        return False
    if abs(ss.st_mtime - ds.st_mtime) < 0.001:
        return True
    return file_checksum(src) == file_checksum(dst)


def ensure_dirs():
    HYPR_DIR.mkdir(parents=True, exist_ok=True)
    WAYBAR_DIR.mkdir(parents=True, exist_ok=True)
    GHOSTTY_DIR.mkdir(parents=True, exist_ok=True)
    STATE_DIR.mkdir(parents=True, exist_ok=True)


def backup(path: Path):
    if path.exists() and path.is_file():
        bak = path.with_suffix(path.suffix + ".bak")
        try:
            shutil.copy2(path, bak)
            print(f"[backup] {path} → {bak}")
        except Exception:
            pass


def link_or_copy_file(src: Path, dst: Path, skip_when_same=True) -> bool:
    """Prefer hardlink for instant apply; fallback to copy2."""
    src = Path(src); dst = Path(dst)
    if not src.exists() or not src.is_file():
        print(f"[missing] {src}")
        return False
    dst.parent.mkdir(parents=True, exist_ok=True)

    try:
        if skip_when_same and dst.exists() and files_are_same(src, dst):
            print(f"[skip] identical: {src} == {dst}")
            return False

        if _TURBO:
            # Remove before linking to avoid EXDEV/EEXIST noise
            if dst.exists():
                try:
                    dst.unlink()
                except Exception:
                    pass
            try:
                os.link(src, dst)
                print(f"[link] {src} ⟶ {dst}")
                return True
            except OSError:
                pass  # cross-device or fs not supporting hardlinks -> fallback

        if dst.exists():
            backup(dst)
        shutil.copy2(str(src), str(dst))
        print(f"[copied] {src} → {dst}")
        return True
    except Exception as e:
        print(f"[error] file {src} → {dst}: {e}")
        return False


def copy_tree_fast(src: Path, dst: Path) -> bool:
    """Copy directory with skip-identical; hardlink new files if turbo."""
    if not src.exists():
        return False
    dst.mkdir(parents=True, exist_ok=True)
    changed = False
    for root, _dirs, files in os.walk(src):
        rel = os.path.relpath(root, src)
        dstdir = dst / rel if rel != "." else dst
        dstdir.mkdir(parents=True, exist_ok=True)
        for f in files:
            s = Path(root) / f
            d = dstdir / f
            if link_or_copy_file(s, d):
                changed = True
    return changed


# ---------- hypr/waybar/wallpaper ----------

def hypr_reload():
    sh("hyprctl reload", check=False)


def waybar_reload():
    sh("pkill -SIGUSR2 waybar || true", check=False)


def swww_ensure():
    res = sh("pgrep -x swww-daemon || true", check=False)
    if res.stdout.strip() == "":
        sh("swww-daemon --format xrgb", check=False)
        time.sleep(0.15 if _TURBO else 0.25)


def set_wallpaper(img: Path):
    if not img.exists():
        return
    swww_ensure()
    # Shorter transition when turbo
    duration = 0.18 if _TURBO else 0.42
    sh(
        f"swww img '{img}' --transition-type any "
        "--transition-fps 144 --transition-step 144 "
        f"--invert-y --transition-duration {duration}",
        check=False
    )


# ---------- palette/css for the app (async on turbo) ----------

def extract_palette(img_path: Path):
    try:
        from colorthief import ColorThief
        from PIL import Image

        if not img_path.exists():
            return None

        with Image.open(img_path) as im:
            im = im.convert("RGB")
            im.thumbnail((340, 340))
            tmp = img_path.with_suffix(".m3thumb.jpg")
            im.save(tmp, "JPEG", quality=68)

        ct = ColorThief(str(tmp))
        palette = ct.get_palette(color_count=6)

        def hexify(rgb):
            return "#{:02x}{:02x}{:02x}".format(*rgb)

        return {
            "primary": hexify(palette[0]),
            "secondary": hexify(palette[1]),
            "tertiary": hexify(palette[2]),
            "neutral": hexify(palette[3]),
            "surface": hexify(palette[4]),
            "outline": hexify(palette[5])
        }
    except Exception:
        return None


def generate_css_vars(palette):
    if not palette:
        return ""
    return f"""
:root {{
  --m3-primary: {palette['primary']};
  --m3-secondary: {palette['secondary']};
  --m3-tertiary: {palette['tertiary']};
  --m3-neutral: {palette['neutral']};
  --m3-surface: {palette['surface']};
  --m3-outline: {palette['outline']};
}}
"""


def write_runtime_css_vars(palette):
    css = generate_css_vars(palette)
    target = Path(__file__).parent / "style-vars.css"
    try:
        if target.exists():
            old = target.read_text(encoding="utf-8")
            if old == css:
                return
        target.write_text(css, encoding="utf-8")
        print(f"[css] wrote {target}")
    except Exception as e:
        print(f"[css] failed to write {target}: {e}")


def update_palette_async(img_path: Path):
    def worker():
        pal = extract_palette(img_path)
        if pal:
            write_runtime_css_vars(pal)
    threading.Thread(target=worker, daemon=True).start()


# ---------- GTK / cursor ----------

def set_gsettings(theme_name="", icon_theme="", cursor_theme="", font_name="", adw_scheme=""):
    if _HAVE_GIO:
        try:
            s = Gio.Settings.new("org.gnome.desktop.interface")
            if theme_name:
                s.set_string("gtk-theme", theme_name)
            if icon_theme:
                s.set_string("icon-theme", icon_theme)
            if cursor_theme:
                s.set_string("cursor-theme", cursor_theme)
            if font_name:
                s.set_string("font-name", font_name)
            if adw_scheme in ("default", "prefer-dark", "prefer-light"):
                s.set_string("color-scheme", adw_scheme)
            s.apply()
            return
        except Exception as e:
            print(f"[gio] settings failed, falling back: {e}")

    def gset(schema, key, value):
        if not value:
            return
        try:
            subprocess.run(
                f"gsettings set {schema} {key} \"{value}\"",
                shell=True, check=False,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            )
        except Exception as e:
            print(f"[gsettings] failed for {key}: {e}")

    gset("org.gnome.desktop.interface", "gtk-theme", theme_name)
    gset("org.gnome.desktop.interface", "icon-theme", icon_theme)
    gset("org.gnome.desktop.interface", "cursor-theme", cursor_theme)
    gset("org.gnome.desktop.interface", "font-name", font_name)
    if adw_scheme in ("default", "prefer-dark", "prefer-light"):
        gset("org.gnome.desktop.interface", "color-scheme", adw_scheme)


def set_wayland_cursor(cursor_theme: str = "", size: int = 24):
    if not cursor_theme:
        return
    try:
        subprocess.run(f"hyprctl setcursor '{cursor_theme}' {size}", shell=True, check=False)
        print(f"[cursor] set via hyprctl: {cursor_theme} ({size})")
    except Exception as e:
        print(f"[cursor] failed to apply: {e}")


# ---------- Ghostty ----------

def ghostty_reload(config_path: Path | None = None):
    sh("ghostty +action=reload-config || true", check=False)
    if config_path and config_path.exists():
        try:
            os.utime(config_path, None)
            print(f"[ghostty] touched {config_path}")
        except Exception as e:
            print(f"[ghostty] touch failed: {e}")


# ---------- ZenBrowser ----------

def apply_zen_userstyles(theme_dir: Path, profile_dir: Path,
                         userchrome_rel: str, usercontent_rel: str):
    if not profile_dir:
        return
    profile_dir = Path(os.path.expanduser(str(profile_dir)))
    chrome_dir = profile_dir / "chrome"
    chrome_dir.mkdir(parents=True, exist_ok=True)

    if userchrome_rel:
        link_or_copy_file(theme_dir / userchrome_rel, chrome_dir / "userChrome.css")
    if usercontent_rel:
        link_or_copy_file(theme_dir / usercontent_rel, chrome_dir / "userContent.css")


# ---------- theme listing with caching ----------

def _themes_signature() -> str:
    if not THEME_ROOT.exists():
        return "empty"
    parts = []
    try:
        for tdir in sorted([p for p in THEME_ROOT.iterdir() if p.is_dir()]):
            tj = tdir / "theme.json"
            st = tj.stat() if tj.exists() else None
            parts.append(f"{tdir.name}:{st.st_mtime if st else 0}:{st.st_size if st else 0}")
    except Exception:
        pass
    return fast_hash("|".join(parts))


def list_themes():
    global _THEME_CACHE_SIG, _THEME_CACHE
    sig = _themes_signature()
    if sig == _THEME_CACHE_SIG and _THEME_CACHE is not None:
        return _THEME_CACHE

    themes = []
    if not THEME_ROOT.exists():
        _THEME_CACHE_SIG, _THEME_CACHE = sig, themes
        return themes

    for tdir in sorted([p for p in THEME_ROOT.iterdir() if p.is_dir()]):
        tj = tdir / "theme.json"
        if not tj.exists():
            continue
        try:
            meta = json.loads(tj.read_text(encoding="utf-8"))

            wallpaper_rel = meta.get("wallpaper", "")
            preview_rel = meta.get("preview") or meta.get("preview_image") or wallpaper_rel

            item = {
                "id": tdir.name,
                "name": meta.get("name", tdir.name),
                "dir": str(tdir),

                "hypr": str(tdir / meta.get("hyprland_conf", "hyprland.conf")),
                "waybar": str(tdir / meta.get("waybar_css", "waybar.css")),
                "wallpaper": str(tdir / wallpaper_rel) if wallpaper_rel else "",
                "preview": str(tdir / preview_rel) if preview_rel else "",

                "gtk_theme": meta.get("gtk_theme", ""),
                "gtk_icon_theme": meta.get("gtk_icon_theme", ""),
                "gtk_cursor_theme": meta.get("gtk_cursor_theme", ""),
                "gtk_font_name": meta.get("gtk_font_name", ""),
                "adw_color_scheme": meta.get("adw_color_scheme", ""),

                "ghostty_src": meta.get("ghostty_src", "ghostty"),
                "ghostty_target": meta.get("ghostty_target", "config"),

                "zen_profile_dir": meta.get("zen_profile_dir", ""),
                "zen_userchrome": meta.get("zen_userchrome", ""),
                "zen_usercontent": meta.get("zen_usercontent", ""),
            }
            themes.append(item)
        except Exception as e:
            print(f"[theme] failed to parse {tj}: {e}")
            continue

    _THEME_CACHE_SIG, _THEME_CACHE = sig, themes
    return themes


def runtime_float_rule():
    rules = [
        f"windowrulev2 float, app-id:^(?:{APP_ID})$",
        f"windowrulev2 size 960 620, app-id:^(?:{APP_ID})$",
        f"windowrulev2 move 5% 7%, app-id:^(?:{APP_ID})$"
    ]
    for r in rules:
        sh(f"hyprctl keyword '{r}'", check=False)


# ---------- apply theme (turbo, parallel) ----------

def _apply_ghostty(t):
    theme_path = Path(t["dir"])
    ghostty_src = theme_path / t.get("ghostty_src", "ghostty")
    ghostty_target_name = t.get("ghostty_target", "config").strip("/") or "config"
    ghostty_dst_file = GHOSTTY_DIR / ghostty_target_name
    changed = False

    if ghostty_src.exists():
        if ghostty_src.is_dir():
            changed = copy_tree_fast(ghostty_src, GHOSTTY_DIR)
            cfg_to_touch = ghostty_dst_file if ghostty_dst_file.exists() else GHOSTTY_CONFIG_DEFAULT
            if changed:
                ghostty_reload(cfg_to_touch)
        else:
            if link_or_copy_file(ghostty_src, ghostty_dst_file):
                changed = True
                ghostty_reload(ghostty_dst_file)
    else:
        legacy_file = theme_path / "ghostty"
        if legacy_file.exists() and legacy_file.is_file():
            if link_or_copy_file(legacy_file, GHOSTTY_CONFIG_DEFAULT):
                changed = True
                ghostty_reload(GHOSTTY_CONFIG_DEFAULT)
    return changed


def apply_theme(theme_id: str):
    ensure_dirs()
    state = load_state()
    get_turbo()  # ensure _TURBO reflects persisted pref

    themes = {t["id"]: t for t in list_themes()}
    if theme_id not in themes:
        raise RuntimeError(f"Theme '{theme_id}' not found.")
    t = themes[theme_id]
    theme_path = Path(t["dir"])

    hypr_src = theme_path / Path(t["hypr"])
    waybar_src = theme_path / Path(t["waybar"])

    results = {}

    with ThreadPoolExecutor(max_workers=4) as ex:
        futs = {
            "hypr": ex.submit(link_or_copy_file, hypr_src, HYPR_CONF),
            "waybar": ex.submit(link_or_copy_file, waybar_src, WAYBAR_THEME) if waybar_src.exists() else None,
            "ghostty": ex.submit(_apply_ghostty, t),
            "zen": ex.submit(
                apply_zen_userstyles, theme_path,
                Path(os.path.expanduser(t.get("zen_profile_dir", ""))),
                t.get("zen_userchrome", ""), t.get("zen_usercontent", "")
            ) if t.get("zen_profile_dir", "") else None,
            "settings": ex.submit(
                set_gsettings,
                t.get("gtk_theme", ""),
                t.get("gtk_icon_theme", ""),
                t.get("gtk_cursor_theme", ""),
                t.get("gtk_font_name", ""),
                t.get("adw_color_scheme", ""),
            ),
        }
        for k, f in futs.items():
            if f is None:
                continue
            try:
                results[k] = f.result()
            except Exception as e:
                print(f"[apply:{k}] {e}")

    # Cursor after settings (non-blocking)
    cursor = t.get("gtk_cursor_theme", "")
    if cursor:
        set_wayland_cursor(cursor, 24)

    # Wallpaper + palette (palette async if turbo)
    wp_path = Path(t.get("wallpaper", "")).expanduser()
    last_wp = state.get("last_wallpaper", "")
    if str(wp_path) and str(wp_path) != last_wp and wp_path.exists():
        set_wallpaper(wp_path)
        if _TURBO:
            update_palette_async(wp_path)
        else:
            pal = extract_palette(wp_path)
            write_runtime_css_vars(pal)
        state["last_wallpaper"] = str(wp_path)

    # Waybar signal and Hypr reload only if needed
    if results.get("waybar"):
        waybar_reload()
    if results.get("hypr"):
        hypr_reload()

    # Save state
    state["last_theme"] = theme_id
    state["turbo"] = _TURBO
    save_state(state)


import threading
import queue
import os

import PySimpleGUI as sg
from ursina import *
from ursina.mesh import Mesh
from ursina.lights import DirectionalLight
import math

info_queue = queue.Queue()


def gui_thread():
    """
    Runs in its own thread. Creates a PySimpleGUI window and
    listens for planet names on info_queue. When a name arrives,
    updates title, image, and info text.
    """
    sg.theme("DarkBlue3")
    layout = [
        [sg.Text("Solar System Info", font=("Any", 18), key="-TITLE-")],
        [sg.Image(key="-IMAGE-")],
        [
            sg.Multiline(
                default_text="",
                size=(30, 15),
                key="-INFO-",
                disabled=True,
                autoscroll=True,
            )
        ],
    ]
    window = sg.Window("Planet Info Panel", layout, finalize=True)

    while True:
        event, values = window.read(timeout=100)
        if event == sg.WIN_CLOSED:
            break

        try:
            planet_name = info_queue.get_nowait()
        except queue.Empty:
            continue

        # update title
        window["-TITLE-"].update(f" {planet_name} ")
        data = PLANETS[planet_name]

        # update image
        thumb_path = data.get("thumb", "")
        if os.path.isfile(thumb_path):
            window["-IMAGE-"].update(filename=thumb_path)
        else:
            window["-IMAGE-"].update(data=None)

        # update info text
        window["-INFO-"].update(data["info"])

    window.close()


# start GUI thread
threading.Thread(target=gui_thread, daemon=True).start()


# ── Ursina setup ─────────────────────────────────────────────────────────────
app = Ursina()
window.title = "Interactive Solar System"
window.color = color.rgb(10, 10, 30)
window.fps_counter.enabled = False

Sky(texture="textures/stars.jpg")
DirectionalLight().look_at(Vec3(1, -1, -1))


# ── OrbitCamera ─────────────────────────────────────────────────────────────
class OrbitCamera:
    def __init__(self):
        self.min_distance = 5
        self.max_distance = 80
        self.default_distance = 40
        self.distance = self.default_distance
        self.pitch = 10
        self.yaw = 50
        self.center = Vec3(0, 0, 0)
        self.drag_speed = 200
        self.zoom_step = 2
        self.zoom_reset_speed = 3

    def update(self):
        if held_keys["right mouse"]:
            self.yaw += mouse.velocity[0] * self.drag_speed
            self.pitch -= mouse.velocity[1] * self.drag_speed
            self.pitch = clamp(self.pitch, -80, 80)

        p = math.radians(self.pitch)
        y = math.radians(self.yaw)
        x = self.distance * math.cos(p) * math.sin(y)
        z = self.distance * math.cos(p) * math.cos(y)
        camera.position = self.center + Vec3(x, self.distance * math.sin(p), z)
        camera.look_at(self.center)


orbit_cam = OrbitCamera()


# ── Planet data (256×256 thumbnails in textures/) ──────────────────────────
PLANETS = {
    "Sun": dict(
        r=1.5,
        d=0,
        tex="textures/sun.jpg",
        thumb="textures/sun_thumb.png",
        info="Type: G2V\nAge: 4.6 Gyr\nTemp: 5778 K",
    ),
    "Mercury": dict(
        r=0.3,
        d=2,
        tex="textures/mercury.jpg",
        thumb="textures/mercury_thumb.png",
        info="Radius: 2440 km\nOrbit: 88 days\nMoons: 0",
    ),
    "Venus": dict(
        r=0.5,
        d=3,
        tex="textures/venus.jpg",
        thumb="textures/venus_thumb.png",
        info="Radius: 6052 km\nSurface: 464 °C\nMoons: 0",
    ),
    "Earth": dict(
        r=0.6,
        d=4,
        tex="textures/earth.jpg",
        thumb="textures/earth_thumb.png",
        info="Radius: 6371 km\nWater: 71 %\nMoons: 1",
    ),
    "Mars": dict(
        r=0.4,
        d=6,
        tex="textures/mars.jpg",
        thumb="textures/mars_thumb.png",
        info="Radius: 3390 km\nMoons: Phobos, Deimos",
    ),
    "Jupiter": dict(
        r=1.2,
        d=8,
        tex="textures/jupiter.jpg",
        thumb="textures/jupiter_thumb.png",
        info="Radius: 69911 km\nGreat Red Spot\nMoons: 79",
    ),
    "Saturn": dict(
        r=1.1,
        d=10,
        tex="textures/saturn.jpg",
        thumb="textures/saturn_thumb.png",
        ring_tex="textures/saturn_ring.png",
        info="Radius: 58232 km\nRings: Yes\nMoons: 82",
    ),
    "Uranus": dict(
        r=0.7,
        d=12,
        tex="textures/uranus.jpg",
        thumb="textures/uranus_thumb.png",
        info="Radius: 25362 km\nTilt: 98°\nMoons: 27",
    ),
    "Neptune": dict(
        r=0.7,
        d=14,
        tex="textures/neptune.jpg",
        thumb="textures/neptune_thumb.png",
        info="Radius: 24622 km\nFastest winds\nMoons: 14",
    ),
}

# draw orbits
for data in PLANETS.values():
    if data["d"] > 0:
        pts = [
            Vec3(math.cos(a) * data["d"], 0, math.sin(a) * data["d"])
            for a in [2 * math.pi * i / 60 for i in range(61)]
        ]
        Entity(model=Mesh(vertices=pts, mode="line"), color=color.gray)


# create planets + rings
planet_entities = {}
for name, data in PLANETS.items():
    tex = load_texture(data["tex"]) if os.path.isfile(data["tex"]) else None
    ent = Entity(
        name=name,
        model="sphere",
        scale=data["r"],
        texture=tex,
        color=color.white,
        position=(data["d"], 0, 0),
        collider="sphere",
    )
    planet_entities[name] = ent

    # Saturn’s ring
    if name == "Saturn" and data.get("ring_tex"):
        inner, outer = data["r"] * 1.3, data["r"] * 2
        seg, verts, tris, uvs = 32, [], [], []
        for i in range(seg + 1):
            θ = 2 * math.pi * i / seg
            verts += [
                Vec3(math.cos(θ) * inner, 0, math.sin(θ) * inner),
                Vec3(math.cos(θ) * outer, 0, math.sin(θ) * outer),
            ]
            uvs += [(i / seg, 0), (i / seg, 1)]
        for i in range(seg):
            b = i * 2
            tris += [(b, b + 1, b + 3), (b, b + 3, b + 2)]
        mesh = Mesh(vertices=verts, triangles=tris, uvs=uvs)
        mesh.generate()
        Entity(
            model=mesh,
            texture=load_texture(data["ring_tex"]),
            parent=ent,
            position=(0, 0.02, 0),
            rotation_x=20,
            double_sided=True,
        )


# ── Focus & input logic ───────────────────────────────────────────────────────
focus_target = None
focus_offset = Vec3()
time_passed = 0
resetting_zoom = False


def focus_on(name):
    global focus_target, focus_offset, resetting_zoom
    focus_target = planet_entities[name]
    resetting_zoom = False
    s = focus_target.scale_x
    focus_offset = Vec3(0, s * 2, -s * 3)
    # push to GUI thread
    info_queue.put(name)


def clear_focus():
    global focus_target, resetting_zoom
    focus_target = None
    resetting_zoom = True


def input(key):
    if key == "left mouse down":
        hit = mouse.hovered_entity
        if hit and hit.name in planet_entities:
            focus_on(hit.name)
        else:
            clear_focus()
    if key == "scroll up":
        orbit_cam.distance = clamp(
            orbit_cam.distance - orbit_cam.zoom_step,
            orbit_cam.min_distance,
            orbit_cam.max_distance,
        )
    if key == "scroll down":
        orbit_cam.distance = clamp(
            orbit_cam.distance + orbit_cam.zoom_step,
            orbit_cam.min_distance,
            orbit_cam.max_distance,
        )


def update():
    global time_passed, resetting_zoom
    dt = time.dt
    time_passed += dt

    # spin & orbit
    for name, ent in planet_entities.items():
        ent.rotation_y += 20 * dt
        d = PLANETS[name]["d"]
        if d > 0:
            ang = time_passed * (2.2 / d)
            ent.x = math.cos(ang) * d
            ent.z = math.sin(ang) * d

    # camera movement
    if focus_target:
        orbit_cam.center = lerp(orbit_cam.center, focus_target.world_position, dt * 3)
        orbit_cam.distance = lerp(orbit_cam.distance, focus_target.scale_x * 5, dt * 3)
        tgt = focus_target.world_position + focus_offset
        camera.position = lerp(camera.position, tgt, dt * 3)
        camera.look_at(focus_target.world_position)
    else:
        orbit_cam.center = lerp(orbit_cam.center, Vec3(0, 0, 0), dt * 3)
        if resetting_zoom:
            orbit_cam.distance = lerp(
                orbit_cam.distance,
                orbit_cam.default_distance,
                dt * orbit_cam.zoom_reset_speed,
            )
            if abs(orbit_cam.distance - orbit_cam.default_distance) < 0.01:
                resetting_zoom = False

    orbit_cam.update()


app.run()

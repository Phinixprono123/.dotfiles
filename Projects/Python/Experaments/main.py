#!/usr/bin/env python3
import tkinter as tk
import os


def perform(action):
    os.system(action)
    root.destroy()


# Hide base window
root = tk.Tk()
root.withdraw()

# Create a Toplevel popup
popup = tk.Toplevel(root)
popup.overrideredirect(True)
popup.attributes("-topmost", True)
popup.configure(bg="#181926")

# Try setting WM type (some systems ignore this silently)
try:
    popup.wm_attributes("-type", "dialog")
except:
    pass

# Geometry (adjust as needed)
width, height = 220, 240
x_pos, y_pos = 1650, 40
popup.geometry(f"{width}x{height}+{x_pos}+{y_pos}")

# Add power menu buttons
actions = {
    "😴 Suspend": "systemctl suspend",
    "🔒 Lock": "hyprlock",
    "🔁 Reboot": "systemctl reboot",
    "🧬 UEFI Firmware": "systemctl reboot --firmware-setup",
    "⏻ Shutdown": "systemctl poweroff",
}

for label, cmd in actions.items():
    tk.Button(
        popup,
        text=label,
        command=lambda c=cmd: perform(c),
        font=("Hack", 11),
        bg="#282c34",
        fg="#a5adcb",
        activebackground="#363a4f",
        relief="flat",
    ).pack(fill="x", padx=10, pady=4)

popup.update_idletasks()
popup.deiconify()
popup.mainloop()

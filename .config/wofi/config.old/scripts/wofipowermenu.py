#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os


def run_menu():
    keys = (
        "😴 Suspend",
        "🔒 Lock",
        "🔁 Reboot",
        "🧬 UEFI Firmware",
        "⏻ Shutdown",
    )

    actions = (
        "systemctl suspend",
        "hyprlock",
        "systemctl reboot",
        "systemctl reboot --firmware-setup",
        "systemctl poweroff",
    )

    options = "\n".join(keys)
    choice = (
        os.popen(
            "echo -e '"
            + options
            + "' | wofi --dmenu --no-filter --location 4 -W 300 -H 250 --style ~/.config/wofi/hideinput.css"
        )
        .readline()
        .strip()
    )
    if choice in keys:
        os.system(actions[keys.index(choice)])


run_menu()

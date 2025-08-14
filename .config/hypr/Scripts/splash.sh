#!/bin/bash

# Wait briefly to make sure Wayland is ready but don't block Hyprland
(
  sleep 1

  # Show splash — replace with preferred tool or method
  yad --splash \
    --timeout=3 \
    --no-buttons \
    --undecorated \
    --center \
    --image="~/.config/hypr/Assets/cat.gif"
)

#!/bin/bash

# Reload hyprland config
hyprctl reload

# KIll and reload waybar
killall waybar
waybar &


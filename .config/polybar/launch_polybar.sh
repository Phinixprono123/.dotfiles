#!/bin/bash

# Enable debug mode to see every command being executed.
# This is a powerful tool to understand what your script is doing.
set -x

# --- Section 1: Cleanup ---
killall -q polybar

# Wait until the processes have been shut down.
while pgrep -x polybar >/dev/null; do sleep 1; done
# Check if a monitor is connected using the 'xrandr' command.
if type "xrandr" > /dev/null; then
  # If xrandr is available, launch polybar on each connected monitor.
  for m in $(xrandr --query | grep " connected" | cut -d" " -f1); do
    # Pass the monitor name to the polybar command.
    MONITOR=$m polybar &
  done
else
  # If no monitors are detected with xrandr, launch polybar on the default monitor.
  polybar &
fi

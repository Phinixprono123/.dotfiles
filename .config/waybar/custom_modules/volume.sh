#!/bin/bash
MAX=200
STEP=5

# Get current volume as integer
VOLUME=$(pactl get-sink-volume @DEFAULT_SINK@ | grep -oP '\d+(?=%)' | head -1)

# Calculate new volume
NEW_VOLUME=$((VOLUME + STEP))
if [ "$NEW_VOLUME" -gt "$MAX" ]; then
  NEW_VOLUME=$MAX
fi

# Apply new volume
pactl set-sink-volume @DEFAULT_SINK@ "${NEW_VOLUME}%"

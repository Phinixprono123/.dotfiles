#!/bin/bash

ZOOM_FILE="/tmp/hypr-current-zoom"
DEFAULT=1.0
STEP=0.05

# Safely read current zoom level
if [[ -f "$ZOOM_FILE" ]]; then
  CURRENT=$(cat "$ZOOM_FILE")
else
  CURRENT=$DEFAULT
fi

# Use awk for precision math
if [[ $1 == "in" ]]; then
  TARGET=$(awk -v val="$CURRENT" -v step="$STEP" 'BEGIN {print val + step}')
elif [[ $1 == "out" ]]; then
  TARGET=$(awk -v val="$CURRENT" -v step="$STEP" 'BEGIN {print val - step}')
else
  TARGET=$DEFAULT
fi

# Clamp zoom level
TARGET=$(awk -v val="$TARGET" 'BEGIN {if (val < 0.5) val = 0.5; if (val > 3.0) val = 3.0; print val}')

# Save and apply
echo "$TARGET" >"$ZOOM_FILE"
hypr-zoom -target "$TARGET"

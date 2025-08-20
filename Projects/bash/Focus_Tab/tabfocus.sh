#!/usr/bin/env bash
APP_NAME="$1"       # e.g. "Firefox"
TAB_MATCH="$2"      # e.g. part of the tab title or URL

# Step 1: Focus the browser window in Hyprland
hyprctl dispatch focuswindow "$APP_NAME"

# Step 2: Tell Firefox to switch to the matching tab
# Using mozrepl / websocket / remote protocol (example uses socat & JSON-RPC)

# Simple example via websocket (needs websocat installed)
if command -v websocat &>/dev/null; then
    # Get list of tabs from Firefox
    TABS_JSON=$(websocat ws://localhost:6000 | jq '.tabs')
    TARGET_TAB_ID=$(echo "$TABS_JSON" | jq -r --arg match "$TAB_MATCH" '.[] | select(.title | test($match; "i")) | .id' | head -n1)
    if [ -n "$TARGET_TAB_ID" ]; then
        websocat ws://localhost:6000 <<< "{\"to\":\"root\",\"type\":\"selectTab\",\"tabId\":$TARGET_TAB_ID}"
    fi
fi


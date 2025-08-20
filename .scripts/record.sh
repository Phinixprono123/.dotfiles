#!/bin/bash

RECORDING_PID=$(pgrep -x wf-recorder)

if [ -n "$RECORDING_PID" ]; then
  pkill -INT wf-recorder
  notify-send "🎬 Recording stopped"
else
  TIMESTAMP=$(date +%Y-%m-%d_%H-%M-%S)
  wf-recorder -f "$HOME/Videos/recording_$TIMESTAMP.mp4" &
  notify-send "🎥 Recording started"
fi

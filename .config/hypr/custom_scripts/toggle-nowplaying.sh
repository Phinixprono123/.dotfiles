#!/bin/bash
COLOR="#8FBCBB"
SIZE=12
SYMBOL=""

if pgrep -f NPlay > /dev/null; then
    pkill -f NPlay
else
    NPlay "$COLOR" "$SIZE" "$SYMBOL" &
fi


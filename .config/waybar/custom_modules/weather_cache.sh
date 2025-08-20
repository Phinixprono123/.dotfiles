#!/usr/bin/env bash
cache_file="$HOME/.cache/weather.txt"

if [[ -f "$cache_file" ]]; then
    cat "$cache_file"
else
    echo "…"
    echo "Updating weather..."
fi


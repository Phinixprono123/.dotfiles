#!/bin/bash
# Get the title
title=$(hyprctl activewindow | grep -i 'title:' | cut -d '"' -f2)

# Remove first 7 characters
title="${title:8}"

max_length=50

# Handle empty titles
if [[ -z "$title" ]]; then
  echo "No window title found"
elif [[ ${#title} -gt $max_length ]]; then
  echo "${title:0:$((max_length - 3))}..."
else
  echo "$title"
fi

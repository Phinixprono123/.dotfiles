#!/usr/bin/env bash
export LC_ALL=C.UTF-8
City="Dhaka"

cache_file="$HOME/.cache/weather.txt"

main=$(curl --keepalive-time 2 -s "http://wttr.in/${City}?format=1" \
  | sed 's/\xC2\xA0//g' \
  | xargs)

details=$(curl --keepalive-time 2 -s "http://wttr.in/${City}?format=%l:+%C")

{
  echo "$main"
  echo -e "$details"
} > "$cache_file"


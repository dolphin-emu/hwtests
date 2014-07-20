#!/bin/sh

"$DEVKITPPC/bin/wiiload" "$1"
# empiric value, no idea if this differs for large executables
sleep 2
netcat ${WIILOAD#tcp:} 16784

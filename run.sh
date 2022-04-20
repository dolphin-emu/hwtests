#!/bin/sh

wiiload "$1"
# empiric value, no idea if this differs for large executables
sleep 4
netcat ${WIILOAD#tcp:} 16784

#!/bin/bash
set -e

# Build the project
make

# Locate Xephyr reliably
XEPHYR=$(command -v Xephyr)
if [ -z "$XEPHYR" ]; then
    echo "Xephyr not found in PATH, exiting."
    exit 1
fi

# Launch Xephyr with xinit
xinit ./xinitrc -- "$XEPHYR" :100 -ac -screen 844x644 -host-cursor


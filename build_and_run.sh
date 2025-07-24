#!/bin/bash
set -e

make

XEPHYR=$(whereis -b Xephyr | sed -E 's/^.*: ?//')
if [ -z "$XEPHYR" ]; then
    echo "Xephyr not found, exiting"
    exit 1
fi
 xinit ./xinitrc -- \
        "$XEPHYR" \
        :100 \
        -ac \
        -screen 600x400 \
        -host-cursor

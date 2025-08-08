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
		-screen 1920x1080 \
        # -screen 820x460 \
        -host-cursor

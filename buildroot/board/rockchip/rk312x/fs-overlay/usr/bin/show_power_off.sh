#!/bin/sh

echo "SDL image show"

export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/var/run}
/usr/bin/showImage /usr/share/menu/images/powering-off.jpg > /dev/kmsg 2>&1 &

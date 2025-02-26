#!/bin/sh

echo "SDL image show"

export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/var/run}
weston --tty=2 --idle-time=0&
{
        while [ ! -e ${XDG_RUNTIME_DIR}/wayland-0 ]; do
                sleep .1
        done
}&

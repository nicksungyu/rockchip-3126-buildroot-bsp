#!/bin/sh

game_name=$(cat /tmp/game_cmdline | cut -d ' ' -f 1)
killall -9 $game_name

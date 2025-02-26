#!/bin/sh

echo "kill all process..."
killall -9 hp_spk_switch
killall -9 adbd
killall -9 ash
killall -15 OnlineHub
killall -15 weston
killall -15 syslogd
killall -15 klogd
killall -15 udevd
killall -15 input-event-daemon
killall -15 dbus-daemon

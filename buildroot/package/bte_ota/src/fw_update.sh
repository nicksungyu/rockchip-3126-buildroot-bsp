#!/bin/sh
echo 3 > /proc/sys/vm/drop_caches
echo 1 > /proc/unlock

# kill daemon
/usr/bin/kill_daemon.sh
# do chroot
/usr/bin/prepare_chroot.sh
# drop cache
echo 3 > /proc/sys/vm/drop_caches
# do ota
chroot /tmp/chroot_bte/ /usr/bin/fw_chroot.sh

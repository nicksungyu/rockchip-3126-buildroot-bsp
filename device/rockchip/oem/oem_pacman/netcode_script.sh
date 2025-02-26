mkdir /tmp/usb
mount /dev/sda1 -t vfat /tmp/usb
if [ $? == 0 ]
then
	cp /tmp/netcode.log /tmp/usb
	cat /tmp/ssids >> /tmp/usb/netcode.log
	cat /tmp/wifiscan >> /tmp/usb/netcode.log
else
	exit 1
fi
/bin/sync

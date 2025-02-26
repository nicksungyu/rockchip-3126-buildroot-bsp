mkdir /tmp/usb
mount /dev/sda1 -t vfat /tmp/usb
if [ $? == 0 ]
then
	mkdir /tmp/usb/tmp
	cp /tmp/* /tmp/usb/tmp
	mkdir /tmp/usb/docs
	cp docs/* /tmp/usb/docs
	umount /tmp/usb
	rmdir /tmp/usb
else
	exit 1
fi
/bin/sync

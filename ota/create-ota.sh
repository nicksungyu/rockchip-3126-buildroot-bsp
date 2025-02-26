#!/bin/bash
## ./create-ota.sh ota boot.img rootfs.img
OS_PUBLIC_KEY="./ota-key/ota_key.pub"
OTA_KEY="./ota-key/ota.key"
OTA_ENC_KEY="ota.key.enc"
LIST="list.txt"
ZIP_FILE_LIST="$LIST"
OTA_FILE="ota.zip"
if [ $# -eq 0 ] # $# is parameter counter value
then
	echo "Please input filename"
	exit
else
	rm -rf ./ota.zip
	echo "counter=$(($# + 2))" > $LIST
	cat ./model >> $LIST
	#use public to encrypt ota key
	LD_LIBRARY_PATH=./ ./openssl rsautl -encrypt -inkey $OS_PUBLIC_KEY -pubin -in $OTA_KEY -out $OTA_ENC_KEY
	md5hash=($(md5sum $OTA_ENC_KEY | cut -d ' ' -f 1))
	echo "$OTA_ENC_KEY,$md5hash" >> $LIST
	ZIP_FILE_LIST="$ZIP_FILE_LIST $OTA_ENC_KEY"
	#encrypt all input file
	for var in "$@"
	do
		echo "Filename=>$var"
		LD_LIBRARY_PATH=./ ./openssl enc -aes-256-cbc -salt -pbkdf2 -in $var -out $var.enc -pass file:$OTA_KEY
		echo "Encrypted Filename=>$var.enc"
		md5hash=($(md5sum $var.enc | cut -d ' ' -f 1))
		echo "$var.enc,$md5hash" >> $LIST
		ZIP_FILE_LIST="$ZIP_FILE_LIST $var.enc"
	done
	#zip ota file
	zip -9r $OTA_FILE $ZIP_FILE_LIST
fi

WPA_CONF="docs/wpa_supplicant.conf"
echo "#ctrl_interface=/var/run/wpa_supplicant" > $WPA_CONF
echo "update_config=1" >> $WPA_CONF
echo "network={" >> $WPA_CONF
if [ ! -z $4 ] && [ "$4" != "any" ]
then 
	echo "        bssid=$4" >> $WPA_CONF
fi
echo "        ssid=\"$1\"" >> $WPA_CONF
echo "        scan_ssid=1" >> $WPA_CONF
if [ "$2" = "none" ]; then
	echo "        key_mgmt=NONE" >> $WPA_CONF
elif [ "$2" = "wep" ]; then
	echo "        key_mgmt=NONE" >> $WPA_CONF
	echo "        wep_key0=$3" >> $WPA_CONF
#	echo "        wep_tx_keyid=0" >> $WPA_CONF
elif [ -f /tmp/$3 ]; then
#	wpa_passphrase $1 $3 |grep -v "#psk" |grep psk >> $WPA_CONF
	cat /tmp/$3 |grep -v "#psk" |grep psk >> $WPA_CONF
fi
echo "}" >> $WPA_CONF
/bin/sync

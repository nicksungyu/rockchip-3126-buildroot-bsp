WIRELESS_CONF="docs/wpa_supplicant.conf"
DNS_CONF=`realpath /etc/resolv.conf`
if [ ! -f "$DNS_CONF" ]
then
	DNS_CONF="docs/resolv.conf"
fi
NETWORK_CONF="docs/network.conf"
OUTFILE="/tmp/net.out"
WPA_LOG="/tmp/wpa_supplicant.log"

wpa_device="wext"
killall="killall"
eval `grep 'killall=' config.ini`
eval `grep 'wpa_device=' config.ini`
eval `grep 'DNS_CONF=' config.ini`
eval `grep 'admin_prefix=' config.ini`
if [ -f OnlineHub.ini ]
then
	eval `grep 'killall=' OnlineHub.ini`
	eval `grep 'wpa_device=' OnlineHub.ini`
	eval `grep 'DNS_CONF=' OnlineHub.ini`
	eval `grep 'admin_prefix=' OnlineHub.ini`
fi

echo "# net.sh results" > $OUTFILE

if [ -f $NETWORK_CONF ]
then
	. $NETWORK_CONF
else
	echo "Missing $NETWORK_CONF"
	exit
fi

ethernet_stop(){
	ifconfig eth0 down
}

ethernet_start(){
	for i in `seq 15`
	do
		ifconfig eth0 up
		if [ $? == 0 ]
		then
			return
		fi
		sleep 1
	done
}

wireless_stop(){
	$killall -9 wpa_supplicant
	ifconfig wlan0 down
}

wireless_start(){
	if [ -f $WIRELESS_CONF ]
	then
		echo "Starting wpa_supplicant..."
		wpa_supplicant -D$wpa_device -iwlan0 -c $WIRELESS_CONF > $WPA_LOG &
	else
		echo "Please check $WIRELESS_CONF"
		echo "wpa=incomplete" >> $OUTFILE
		ifconfig wlan0 up
		return
	fi

	echo "Waiting for wpa_supplicant result..."
	for i in `seq 30`
	do
		if grep -q "CTRL-EVENT-CONNECTED" $WPA_LOG
		then
			echo "wpa=success" >> $OUTFILE
			return
		fi
		if grep -q "WRONG_KEY" $WPA_LOG
		then
			echo "wpa=failed" >> $OUTFILE
			$killall -9 wpa_supplicant
			ifconfig wlan0 up
			return
		fi
		if grep -q "Terminated by OnlineHub" $WPA_LOG
		then
			echo "wpa=cancel" >> $OUTFILE
			$killall -9 wpa_supplicant
			ifconfig wlan0 up
			return
		fi
		if grep -q "Failed to initialize" $WPA_LOG
		then
			echo "wpa_supplicant init failure; retrying..."
			ethernet_stop
			wireless_stop
			sleep 1
			wpa_supplicant -D$wpa_device -iwlan0 -c $WIRELESS_CONF > $WPA_LOG &
		fi
		sleep 1
	done
	echo "wpa=timeout" >> $OUTFILE
}

dhcp_stop(){
	#-12 = USR2, send release ip to udhcpc
	$killall -12 udhcpc
	$killall -9 udhcpc
	route del default
}

get_mac_address(){
	LABEL=""
	for VALUE in `ifconfig $interface`
	do
		if [ "$LABEL" = "HWaddr" ]
		then
			echo "mac_address=$VALUE" >> $OUTFILE
			return
		fi
		LABEL="$VALUE"
	done
	for VALUE in `ifconfig $interface`
	do
		if [ "$LABEL" = "ether" ]
		then
			echo "mac_address=$VALUE" >> $OUTFILE
			return
		fi
		LABEL="$VALUE"
	done
}

get_route_settings(){
	# Parse subnet and IP from ifconfig
	FOUND_IP=0
	FOUND_SUBNET=0
	for FIELD in `ifconfig $interface`
	do
		LABEL=`echo $FIELD | cut -d':' -f1`
		VALUE=`echo $FIELD | cut -d':' -f2`
		if [ ! -z "$VALUE" ]
		then
			if [ "$LABEL" = "addr" ]
			then
				echo "ip=$VALUE" >> $OUTFILE
				FOUND_IP=1
			fi
			if [ "$LABEL" = "Mask" ]
			then
				echo "subnet=$VALUE" >> $OUTFILE
				FOUND_SUBNET=1
			fi
		fi
	done

	LABEL=""
	for VALUE in `ifconfig $interface`
	do
		if [ $FOUND_IP -eq 0 ] 
		then
			if [ "$LABEL" = "inet" ]
			then
				echo "ip=$VALUE" >> $OUTFILE
			fi
		fi
		if [ $FOUND_SUBNET -eq 0 ]
		then
			if [ "$LABEL" = "netmask" ]
			then
				echo "subnet=$VALUE" >> $OUTFILE
			fi
		fi
		LABEL="$VALUE"
	done

	# Parse gateway from ip route

	LABEL=""
	for VALUE in `ip route | grep $interface | grep default`
	do
		if [ "$LABEL" = "via" ]
		then
			echo "gateway=$VALUE" >> $OUTFILE
		fi
		LABEL="$VALUE"
	done

	# Consolidate DNS from /etc/resolv.conf

	DNS=1
	cat /etc/resolv.conf | while read LINE 
	do
		LABEL=`echo $LINE | cut -d' ' -f1`
		VALUE=`echo $LINE | cut -d' ' -f2-`
		if [ "$LABEL" = "nameserver" ]
		then
			echo "dns$DNS=$VALUE" >> $OUTFILE
			let DNS=$DNS+1
		fi
	done
}

dhcp_start(){
	echo "Starting DHCP..."
	$admin_prefix udhcpc -i $interface -b -T 2 -t 15
}

static_start(){
	echo "Setting up static configuration..."
	$admin_prefix ifconfig $interface $ip_address netmask $ip_netmask
	$admin_prefix route add default gw $gateway $interface
	echo "nameserver $dns" > $DNS_CONF
	if [ ! -z "$dns2" ]
	then
		echo "nameserver $dns2" >> $DNS_CONF
	fi
}

dhcp_stop

if [ "$1" != "method" ] # change interface unless "method" only
then
	ethernet_stop
	wireless_stop
	if [ "$interface" = "wlan0" ] #wireless connection
	then
		wireless_start
		get_mac_address
		echo "wpa_supplicant result: `grep wpa $OUTFILE`"
	elif [ "$interface" = "eth0" ] #ethernet connection
	then
		ethernet_start
		get_mac_address
	else
		echo "No Network"
		exit
	fi
fi

if [ "$1" != "interface" ] # change method unless "interface" only
then
	case $method in
		"static") static_start;;
		"dhcp") dhcp_start;;
	esac
	get_route_settings
fi
/bin/sync

SCAN_FILE="/tmp/wifiscan"
SSIDS_FILE="/tmp/ssids"
ONECELL_FILE="/tmp/onecell"
ONECELL_INFO_FILE="/tmp/onecell_info"
rm -rf $SCAN_FILE $SSIDS_FILE
#save scan results to a temp file
ifconfig wlan0 up

scan_loop(){
	for i in `seq 10`
	do
		if [ -z "$1" ]
		then
			iwlist wlan0 scanning > $SCAN_FILE 2> $ONECELL_FILE
		else
			chmod a+x /tmp/$1
			/tmp/$1 > $SCAN_FILE 2> $ONECELL_FILE
		fi
		if grep -q Killed $ONECELL_FILE
		then
			echo "Terminated"
			exit
		fi
		scan_ok=$(grep "wlan" $SCAN_FILE)
		if [ -z "$scan_ok" ];
		then
			echo "WIFI scanning failed - try $i"
		else
			echo "WIFI scanning succeeded"
			return
		fi
		sleep 1
	done
}

scan_loop $1

#save number of scanned cell
num_results=$(grep -c "ESSID:" $SCAN_FILE)
i=1
while [ "$i" -le "$num_results" ];
do
	if [ $i -lt 10 ]; then
		cell=$(echo "Cell 0$i - Address:")
	else
		cell=$(echo "Cell $i - Address:")
	fi
	j=`expr $i + 1`
	if [ $j -lt 10 ]; then
		nextcell=$(echo "Cell 0$j - Address:")
	else
		nextcell=$(echo "Cell $j - Address:")
	fi
	awk -v v1="$cell" '$0 ~ v1 {p=1}p' $SCAN_FILE | awk -v v2="$nextcell" '$0 ~ v2 {exit}1' > $ONECELL_FILE
	onessid=$(grep "ESSID:" $ONECELL_FILE | awk '{ sub(/^[ \t]+/, ""); print }' | awk '{gsub("ESSID:", "");print}')
	#remove SSID line to avoid grep mistakes
	grep -v ESSID $ONECELL_FILE > $ONECELL_INFO_FILE
	onemac=$(grep "Address:" $ONECELL_INFO_FILE | awk '{ gsub(/[ \t]/, ""); print }' | awk '{sub(/^[^:]*:/, "");print}')
	oneencryption=$(grep "Encryption key:" $ONECELL_INFO_FILE | awk '{ sub(/^[ \t]+/, ""); print }' | awk '{gsub("Encryption key:on", "secure");print}' | awk '{gsub("Encryption key:off", "open");print}')
	onepower=$(grep "Quality=" $ONECELL_INFO_FILE | awk '{ sub(/^[ \t]+/, ""); print }' | awk '{gsub("Quality=", "");print}' | awk -F ' ' '{print $1}')
	onepower=$((100 * $onepower))
	onepower=${onepower%.*}
	onepower="$onepower%"
	wifimode=$(grep "WPA2 " $ONECELL_INFO_FILE)
	if [ -n "$wifimode" ]; then
		wifimode="wpa2"
	else
		wifimode=$(grep "WPA " $ONECELL_INFO_FILE)
		if [ -n "$wifimode" ]; then
			wifimode="wpa"
		else
			wifimode=$(grep "Encryption key:on" $ONECELL_INFO_FILE)
			if [ -n "$wifimode" ]; then
				wifimode="wep"
			else
				wifimode="none"
			fi
		fi
	fi
	if [ -n "$oneaddress" ]; then
		echo "$onessid,$oneaddress,$oneencryption,$onepower,$onemac" >> $SSIDS_FILE
	else
		echo "$onessid,$oneencryption,$wifimode,$onepower,$onemac" >> $SSIDS_FILE
    	fi
	i=`expr $i + 1`
done

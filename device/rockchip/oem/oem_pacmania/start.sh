export LD_LIBRARY_PATH=`pwd`/lib
system_date=20210213
while read LINE; do
	if [[ "`echo $LINE | cut -d' ' -f1`" == Date ]]; then
		system_date="${LINE//[!0-9]/}"
	fi
done < version
eval `grep 'system_date=' docs/network.ini`
system_date_suffix=0000
eval `grep 'system_date_suffix=' config.ini`
eval `grep 'system_date_suffix=' OnlineHub.ini`
date -s ${system_date}${system_date_suffix}
hwclock -w
patched=0
if [ -f "$patch_destination" ]; then
	patched=1
else
	./patch.sh $@
fi
if [ "$1" != "skip" ]; then
	hub_options="-init=menu $@"
else
	shift
	hub_options="$@"
fi
eval `grep 'preamble=' config.ini`
eval `grep 'preamble=' OnlineHub.ini`

UseExternalDownloader(){
	$preamble
	./Downloader $@
	case $? in
		88) ./patch.sh skip;;
		99) /sbin/fw_update.sh;;
	esac
}

while true; do
	launch_cmd=""
	$preamble
	./OnlineHub $hub_options
	game=$?
	if [ $game -gt 0 ]; then
		eval `grep 'launch_cmd=' /tmp/network.tmp`
		if [ $game -ge 50 ]; then
			if [[ $patched -ne 0 && $game -ge 100 ]]; then
				echo Patch failure detected
				$preamble
				./Downloader -crash
				if [[ $? -ge 100 || ! -f "$patch_destination" ]]; then
					echo Patch removal
					exit
				fi
			else
				case $game in
					98) UseExternalDownloader $launch_cmd;;
					99) exit;;
				esac
			fi
			launch_cmd=""
		fi
	fi

	hub_options="$@"
	if [ ! -z "$launch_cmd" ]; then
		$preamble
		$launch_cmd $@
		last_exit_code=$?
		if [ $last_exit_code == 3 ]; then
			hub_options="-summon_hub $@"
		else
			hub_options="-last_context=$last_exit_code,app$game $@"
		fi
	fi
done

scores="/tmp/$1.sav"			# The path for the score file Vortex sees
vortex="`pwd`/vortex"			# The path to Vortex's directory
default="$vortex/$1.org.sav"	# Default scores

# Hack: HDMI systems perplexingly seem to shut down HDMI if net.sh is run.
# This brings it back up if "rockchip-hdmi=true" is set in config.ini
eval `grep 'rockchip_hdmi=' config.ini`
RestoreHDMI(){
	if [[ "$rockchip_hdmi" == true || "$rockchip_hdmi" == 1 ]]; then
		echo "U:1280x720p-0" > /sys/class/graphics/fb0/mode
	fi
}
RestoreHDMI

# Create default high score table if it doesn't exist
for bank in 0 1; do
	if [[ ! -f "docs/$1_$bank.sav" && -f "$default" ]]; then
		cp "$default" "docs/$1_$bank.sav"
	fi
done

# Look up launch command when it's not the same as the save file name
# Also look up EEROM checksum parameters, if needed
game="$1"
options=""
case "$1" in
	MsPacMan)			game="PacMan m";;
	DigDugII)			game="Mappy d";;
	DragonSpirit)		game="NS1 d";		options="-base=86 -start=80 -check=96";;
	Rompers)			game="NS1 r";		options="-base=86 -start=80 -check=96";;
	Galaga88)			game="NS1 g";		options="-base=86 -start=80 -check=96";;
	KingandBalloon)		game="Galaxian k";;
	RollingThunder)		game="NS86";;
	TheTowerofDruaga)	game="Mappy t";;
	Tron)									options="-base=0";;
	DiscsOfTron)							options="-base=0";;
esac

# Convert Gemini .opt file to Vortex .cfg file and identify whether
# we're using the "arcade" or "any" score bank
./VortexSettings "$1" $options
bank=$?

# Remove old link
if [ -f "$scores" ]; then
	rm "$scores"
fi

# Link the selected score bank to the path Vortex expects
ln -s "`pwd`/docs/$1_$bank.sav" "$scores"

# Launch Vortex
cd $vortex
if [ "$2" == "-attract" ]; then
	../AttractMode ./$game
else
	./$game
fi
RestoreHDMI

if [ -z "$patch_destination" ]; then
	export install_base=`pwd`
fi

patch_base=/tmp/patch
export patch_destination=docs/patch.zip
eval "export `grep 'patch_destination=' docs/network.ini` >/dev/null"

# If there's a patch file, set it up
if [ -f "$patch_destination" ]; then

	# Clean up previous patch
	if [ -d "$patch_base" ]; then
		rm -r "$patch_base"
	fi
	mkdir "$patch_base"

	# Expand the contents of the patch
	cd $install_base
	unzip -o "$patch_destination" -x docs/\* -d"$patch_base"

	# Recurse the base installation and, for anything that isn't
	# already there, make symlinks to it in $patch_base
	RecurseSymLink(){
		if [ ! -d "$patch_base/$1" ]; then
			ln -s "`pwd`/$f" "$patch_base/$f"
		else
			for f in $1*; do
				if [ -d "$f" ]; then
					RecurseSymLink "$f/"
				else
					if [[ -f "`pwd`/$f" && ! -f "$patch_base/$f" ]]; then
						ln -s "`pwd`/$f" "$patch_base/$f"
					fi
				fi
			done
		fi
	}
	RecurseSymLink

	echo Running patched version
	cd $patch_base
	./start.sh $@
	rm -r "$patch_base"
	echo Leaving patched version
fi

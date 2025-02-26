#/bin/bash

export LC_ALL=C
unset RK_CFG_TOOLCHAIN

function choose_target_board()
{
	echo
	echo "You're building on Linux"
	echo "Lunch menu...pick a combo:"
	echo ""

	echo "0. default BoardConfig.mk"
	echo ${RK_TARGET_BOARD_ARRAY[@]} | xargs -n 1 | sed "=" | sed "N;s/\n/. /"

	local INDEX
	read -p "Which would you like? [0]: " INDEX
	INDEX=$((${INDEX:-0} - 1))

	if echo $INDEX | grep -vq [^0-9]; then
		RK_BUILD_TARGET_BOARD="${RK_TARGET_BOARD_ARRAY[$INDEX]}"
	else
		echo "Lunching for Default BoardConfig.mk boards..."
		RK_BUILD_TARGET_BOARD=BoardConfig.mk
	fi
}

function build_select_board()
{
	TARGET_PRODUCT="device/rockchip/.target_product"
	TARGET_PRODUCT_DIR=$(realpath ${TARGET_PRODUCT})

	RK_TARGET_BOARD_ARRAY=( $(cd ${TARGET_PRODUCT_DIR}/; ls BoardConfig*.mk | sort) )

	RK_TARGET_BOARD_ARRAY_LEN=${#RK_TARGET_BOARD_ARRAY[@]}
	if [ $RK_TARGET_BOARD_ARRAY_LEN -eq 0 ]; then
		echo "No available Board Config"
		return
	fi

	choose_target_board

	ln -rfs $TARGET_PRODUCT_DIR/$RK_BUILD_TARGET_BOARD device/rockchip/.BoardConfig.mk
	echo "switching to board: `realpath $BOARD_CONFIG`"
}

function unset_board_config_all()
{
	local tmp_file=`mktemp`
	grep -o "^export.*RK_.*=" `find $TOP_DIR/device/rockchip -name "Board*.mk" -type f` -h | sort | uniq > $tmp_file
	source $tmp_file
	rm -f $tmp_file
}

CMD=`realpath $0`
COMMON_DIR=`dirname $CMD`
TOP_DIR=$(realpath $COMMON_DIR/../../..)

BOARD_CONFIG=$TOP_DIR/device/rockchip/.BoardConfig.mk

if [ ! -L "$BOARD_CONFIG" -a  "$1" != "lunch" ]; then
	build_select_board
fi
unset_board_config_all
[ -L "$BOARD_CONFIG" ] && source $BOARD_CONFIG
source $TOP_DIR/device/rockchip/common/Version.mk
TARGET_OUTPUT_DIR=$TOP_DIR/buildroot/output/$RK_CFG_BUILDROOT/target

function usagekernel()
{
	echo "cd kernel"
	echo "make ARCH=$RK_ARCH $RK_KERNEL_DEFCONFIG $RK_KERNEL_DEFCONFIG_FRAGMENT"
	echo "make ARCH=$RK_ARCH $RK_KERNEL_DTS.img -j$RK_JOBS"
}

function usageuboot()
{
	echo "cd u-boot"
	echo "./make.sh $RK_UBOOT_DEFCONFIG" \
		"${RK_TRUST_INI_CONFIG:+../rkbin/RKTRUST/$RK_TRUST_INI_CONFIG}" \
		"${RK_SPL_INI_CONFIG:+../rkbin/RKBOOT/$RK_SPL_INI_CONFIG}" \
		"${RK_UBOOT_SIZE_CONFIG:+--sz-uboot $RK_UBOOT_SIZE_CONFIG}" \
		"${RK_TRUST_SIZE_CONFIG:+--sz-trust $RK_TRUST_SIZE_CONFIG}"
}

function usagerootfs()
{
	echo "source envsetup.sh $RK_CFG_BUILDROOT"

	case "${RK_ROOTFS_SYSTEM:-buildroot}" in
		yocto)
			;;
		debian)
			;;
		distro)
			;;
		*)
			echo "make"
			;;
	esac
}

function usagerecovery()
{
	echo "source envsetup.sh $RK_CFG_RECOVERY"
	echo "$COMMON_DIR/mk-ramdisk.sh recovery.img $RK_CFG_RECOVERY"
}

function usageramboot()
{
	echo "source envsetup.sh $RK_CFG_RAMBOOT"
	echo "$COMMON_DIR/mk-ramdisk.sh ramboot.img $RK_CFG_RAMBOOT"
}

function usagemodules()
{
	echo "cd kernel"
	echo "make ARCH=$RK_ARCH $RK_KERNEL_DEFCONFIG"
	echo "make ARCH=$RK_ARCH modules -j$RK_JOBS"
}

function usage()
{
	echo "Usage: build.sh [OPTIONS]"
	echo "Available options:"
	echo "BoardConfig*.mk    -switch to specified board config"
	echo "lunch              -list current SDK boards and switch to specified board config"
	echo "uboot              -build uboot"
	echo "spl                -build spl"
	echo "loader             -build loader"
	echo "kernel             -build kernel"
	echo "modules            -build kernel modules"
	echo "toolchain          -build toolchain"
	echo "rootfs             -build default rootfs, currently build buildroot as default"
	echo "buildroot          -build buildroot rootfs"
	echo "ramboot            -build ramboot image"
	echo "multi-npu_boot     -build boot image for multi-npu board"
	echo "yocto              -build yocto rootfs"
	echo "debian             -build debian9 stretch rootfs"
	echo "distro             -build debian10 buster rootfs"
	echo "pcba               -build pcba"
	echo "recovery           -build recovery"
	echo "all                -build uboot, kernel, rootfs, recovery image"
	echo "cleanall           -clean uboot, kernel, rootfs, recovery"
	echo "firmware           -pack all the image we need to boot up system"
	echo "updateimg          -pack update image"
	echo "otapackage         -pack ab update otapackage image"
	echo "save               -save images, patches, commands used to debug"
	echo "debug              -enable debug uart"
	echo "release            -disable debug uart"
	echo "allsave            -build all & firmware & updateimg & save"
	echo ""
	echo "Default option is 'allsave'."
}

function uart_enable(){
	printf "UART enable\n"

	# enable adb 
	if [ ! -f ${TARGET_OUTPUT_DIR}/usr/bin/adbd ]; then
		rm -rf ${TARGET_OUTPUT_DIR}/../build/android-tools-4.2.2+git20130218
		rm -rf ${TARGET_OUTPUT_DIR}/../build/rkscript
	fi
	if [ ! -f ${TARGET_OUTPUT_DIR}/usr/bin/gdb  ]; then
		rm -rf ${TARGET_OUTPUT_DIR}/../build/gdb-8.1.1/
	fi

        if [ -f $TOP_DIR/u-boot/.config ]; then
                rm $TOP_DIR/u-boot/.config
        fi	
}

function uart_disable(){
        printf "UART disable\n"

	# disable adb
	if [ -f ${TARGET_OUTPUT_DIR}/usr/bin/adbd ]; then
		rm ${TARGET_OUTPUT_DIR}/usr/bin/adbd
		rm -rf ${TARGET_OUTPUT_DIR}/../build/rkscript
	fi
	if [ -f ${TARGET_OUTPUT_DIR}/usr/bin/gdb  ]; then
		rm -rf ${TARGET_OUTPUT_DIR}/usr/bin/gdb
	fi

	if [ -f $TOP_DIR/u-boot/.config ]; then
		rm $TOP_DIR/u-boot/.config
	fi  	
}

function build_uboot(){
	if [ -z $RK_UBOOT_DEFCONFIG ]; then
		return;
	fi
	echo "============Start build uboot============"
	echo "TARGET_UBOOT_CONFIG=$RK_UBOOT_DEFCONFIG"
	echo "========================================="
	if [ -f u-boot/*_loader_*.bin ]; then
		rm u-boot/*_loader_*.bin
	fi

	cd u-boot && PACK_DEBUG=$PACK_DEBUG ./make.sh $RK_UBOOT_DEFCONFIG \
		${RK_TRUST_INI_CONFIG:+../rkbin/RKTRUST/$RK_TRUST_INI_CONFIG} \
		${RK_SPL_INI_CONFIG:+../rkbin/RKBOOT/$RK_SPL_INI_CONFIG} \
		${RK_UBOOT_SIZE_CONFIG:+--sz-uboot $RK_UBOOT_SIZE_CONFIG} \
		${RK_TRUST_SIZE_CONFIG:+--sz-trust $RK_TRUST_SIZE_CONFIG} \
		&& cd -

	if [ $? -eq 0 ]; then
		echo "====Build uboot ok!===="
	else
		echo "====Build uboot failed!===="
		exit 1
	fi
}

function build_spl(){
	echo "============Start build spl============"
	echo "TARGET_SPL_CONFIG=$RK_SPL_DEFCONFIG"
	echo "========================================="
	if [ -f u-boot/*spl.bin ]; then
		rm u-boot/*spl.bin
	fi
	cd u-boot && ./make.sh $RK_SPL_DEFCONFIG && ./make.sh spl-s && cd -
	if [ $? -eq 0 ]; then
		echo "====Build spl ok!===="
	else
		echo "====Build spl failed!===="
		exit 1
	fi
}

function build_loader(){
	if [ -z $RK_LOADER_BUILD_TARGET ]; then
		return;
	fi
	echo "============Start build loader============"
	echo "RK_LOADER_BUILD_TARGET=$RK_LOADER_BUILD_TARGET"
	echo "=========================================="
	cd loader && ./build.sh $RK_LOADER_BUILD_TARGET && cd -
	if [ $? -eq 0 ]; then
		echo "====Build loader ok!===="
	else
		echo "====Build loader failed!===="
		exit 1
	fi
}

function build_kernel(){
	echo "============Start build kernel ============"
	echo "TARGET_ARCH          =$RK_ARCH"
	echo "TARGET_KERNEL_CONFIG =$RK_KERNEL_DEFCONFIG"
	echo "TARGET_KERNEL_DTS    =$RK_KERNEL_DTS"
	echo "TARGET_KERNEL_CONFIG_FRAGMENT =$RK_KERNEL_DEFCONFIG_FRAGMENT"
	echo "TARGET_OUTPUT_DIR    =$TARGET_OUTPUT_DIR"
	echo "RK_OEM_DIR           =$RK_OEM_DIR"
	echo "=========================================="

	if [  "x$RK_OEM_DIR" == "xoem_pacman" ]; then
		GAME="-DGAME_H2H_PACMAN"
		GAME_TYPE="PacMan"
	elif [ "x$RK_OEM_DIR" == "xoem_pacmania" ]; then
		GAME="-DGAME_PACMANIA"
		GAME_TYPE="Pacmania"
	elif [ "x$RK_OEM_DIR" == "xoem_classof81" ]; then
		GAME="-DGAME_PACMANIA"
		GAME_TYPE="ClassOf81"
	fi

	# make recovery-kernel.img(zboot_recovery.img) for recovery-rootfs uuid using
	sed -i 's/bootargs = "root=PARTUUID=614e0000-0000-4b53-8000-1d28000054a9 rootwait";/bootargs = "root=PARTUUID=614e0000-0000-4b53-8000-1d28000054aa rootwait";/' kernel/arch/arm/boot/dts/rk3126-linux.dts
	cd $TOP_DIR/kernel && make ARCH=$RK_ARCH $RK_KERNEL_DEFCONFIG $RK_KERNEL_DEFCONFIG_FRAGMENT
	if [ $PACK_DEBUG == "release" ]; then
		sed -i 's/CONFIG_FIQ_DEBUGGER=y/\# CONFIG_FIQ_DEBUGGER is not set/' $TOP_DIR/kernel/.config
	fi
	make KCPPFLAGS="${KCPPFLAGS} ${GAME}" ARCH=$RK_ARCH $RK_KERNEL_DTS.img -j$RK_JOBS && cd -
	mv kernel/zboot.img kernel/zboot_recovery.img
	if [ $? -eq 0 ]; then
		echo "====Build recovery-kernel ok!===="
	else
		echo "====Build recovery-kernel failed!===="
		exit 1
	fi

	# for original rootfs uuid using 
	sed -i 's/bootargs = "root=PARTUUID=614e0000-0000-4b53-8000-1d28000054aa rootwait";/bootargs = "root=PARTUUID=614e0000-0000-4b53-8000-1d28000054a9 rootwait";/' kernel/arch/arm/boot/dts/rk3126-linux.dts
	cd $TOP_DIR/kernel && make ARCH=$RK_ARCH $RK_KERNEL_DEFCONFIG $RK_KERNEL_DEFCONFIG_FRAGMENT
	if [ $PACK_DEBUG == "release" ]; then
		sed -i 's/CONFIG_FIQ_DEBUGGER=y/\# CONFIG_FIQ_DEBUGGER is not set/' $TOP_DIR/kernel/.config
	fi
	make KCPPFLAGS="${KCPPFLAGS} ${GAME}" ARCH=$RK_ARCH $RK_KERNEL_DTS.img -j$RK_JOBS && cd -
	# make boot_backup.img for dd to original kernel
	cp kernel/zboot.img kernel/zboot_backup.img	

	if [ $? -eq 0 ]; then
		if [ -f "$TOP_DIR/device/rockchip/$RK_TARGET_PRODUCT/$RK_KERNEL_FIT_ITS" ];then
			$COMMON_DIR/mk-fitimage.sh $TOP_DIR/kernel/$RK_BOOT_IMG $TOP_DIR/device/rockchip/$RK_TARGET_PRODUCT/$RK_KERNEL_FIT_ITS
		fi
		echo "====Build kernel ok!===="		
	else
		echo "====Build kernel failed!===="
		exit 1
	fi
	echo "====Build RTL8188FU===="
	cd $TOP_DIR/kernel/ && make ARCH=$RK_ARCH KSRC=$TOP_DIR/kernel CROSS_COMPILE=arm-linux-gnueabihf- -C $TOP_DIR/kernel/modules/rtl8188FU_linux_v5.7.4_33085.20190419  && cd -
	if [ $? -eq 0 ]; then
		echo "====Build rtl8188fu ok!===="
	else
		echo "====Build rtl8188fu failed!===="
		exit 1
	fi
}

function build_modules(){
	echo "============Start build kernel modules============"
	echo "TARGET_ARCH          =$RK_ARCH"
	echo "TARGET_KERNEL_CONFIG =$RK_KERNEL_DEFCONFIG"
	echo "TARGET_KERNEL_CONFIG_FRAGMENT =$RK_KERNEL_DEFCONFIG_FRAGMENT"
	echo "=================================================="
	cd $TOP_DIR/kernel && make ARCH=$RK_ARCH $RK_KERNEL_DEFCONFIG $RK_KERNEL_DEFCONFIG_FRAGMENT
	if [ $PACK_DEBUG == "release" ]; then
		sed -i 's/CONFIG_FIQ_DEBUGGER=y/\# CONFIG_FIQ_DEBUGGER is not set/' $TOP_DIR/kernel/.config
	fi
	make ARCH=$RK_ARCH modules -j$RK_JOBS && cd -
	if [ $? -eq 0 ]; then
		echo "====Build kernel ok!===="
	else
		echo "====Build kernel failed!===="
		exit 1
	fi
}

function build_toolchain(){
	echo "==========Start build toolchain =========="
	echo "TARGET_TOOLCHAIN_CONFIG=$RK_CFG_TOOLCHAIN"
	echo "========================================="
	[[ $RK_CFG_TOOLCHAIN ]] \
		&& /usr/bin/time -f "you take %E to build toolchain" $COMMON_DIR/mk-toolchain.sh $BOARD_CONFIG \
		|| echo "No toolchain step, skip!"
	if [ $? -eq 0 ]; then
		echo "====Build toolchain ok!===="
	else
		echo "====Build toolchain failed!===="
		exit 1
	fi
}

function build_buildroot(){
	echo "==========Start build buildroot=========="
	echo "TARGET_BUILDROOT_CONFIG=$RK_CFG_BUILDROOT"
	echo "========================================="
	if [ -d $TARGET_OUTPUT_DIR ]; then
		# add bte driver to target_output_dir
		md5sum $TOP_DIR/kernel/drivers/input/keyboard/bte-uart-kb.ko
		md5sum $TARGET_OUTPUT_DIR/bte-uart-kb.ko
		cp $TOP_DIR/kernel/drivers/input/keyboard/bte-uart-kb.ko $TARGET_OUTPUT_DIR/
		cp $TOP_DIR/kernel/modules/rtl8188FU_linux_v5.7.4_33085.20190419/8188fu.ko $TARGET_OUTPUT_DIR/

		PACK_DEBUG=$PACK_DEBUG /usr/bin/time -f "you take %E to build builroot" $COMMON_DIR/mk-buildroot.sh $BOARD_CONFIG
	
		# fill up rootfs size to multiples of 512	
		ROOTFS_SIZE=`ls -l $TOP_DIR/buildroot/output/rockchip_rk312x/images/rootfs.$RK_ROOTFS_TYPE | awk -F ' ' {'print $5'}`
		echo "*** ROOTFS SIZE(type : $RK_ROOTFS_TYPE) : $ROOTFS_SIZE ***"
		part_name=` grep "CMDLINE: mtdparts=rk29xxnand:" $TOP_DIR/device/rockchip/rk3126c/parameter-buildroot.txt | awk -F ',' {'print $4'} | cut -d '(' -f2|cut -d ')' -f1`
		echo "fillup target partition : $part_name"
		if [ "$part_name" == "rootfs" ];then
			fillup_bytes=$[$ROOTFS_SIZE%512]
			echo -e '\033[0;32;1m'
			echo "part_name=$part_name rootfs_size=$ROOTFS_SIZE fillup(mod 512)=$fillup_bytes"
			echo -e '\033[0m'

			dd if=/dev/zero bs=1 count=$fillup_bytes >> $TOP_DIR/buildroot/output/rockchip_rk312x/images/rootfs.$RK_ROOTFS_TYPE
			cksum=`cksum $TOP_DIR/buildroot/output/rockchip_rk312x/images/rootfs.$RK_ROOTFS_TYPE | awk -F ' ' {'print $1'}`
			size=`cksum $TOP_DIR/buildroot/output/rockchip_rk312x/images/rootfs.$RK_ROOTFS_TYPE | awk -F ' ' {'print $2'}`
			echo "cksum=$cksum size=$size"
		else
			echo -e '\033[0;31;1m'
			echo "*** fillup target parition isn't rootfs ***"
			echo -e '\033[0m'
			exit 1
		fi
	else
		# for first build
		PACK_DEBUG=$PACK_DEBUG /usr/bin/time -f "you take %E to build builroot" $COMMON_DIR/mk-buildroot.sh $BOARD_CONFIG
		md5sum $TOP_DIR/kernel/drivers/input/keyboard/bte-uart-kb.ko
		md5sum $TARGET_OUTPUT_DIR/bte-uart-kb.ko
		cp $TOP_DIR/kernel/drivers/input/keyboard/bte-uart-kb.ko $TARGET_OUTPUT_DIR/
		cp $TOP_DIR/kernel/modules/rtl8188FU_linux_v5.7.4_33085.20190419/8188fu.ko $TARGET_OUTPUT_DIR/
		PACK_DEBUG=$PACK_DEBUG /usr/bin/time -f "you take %E to build builroot" $COMMON_DIR/mk-buildroot.sh $BOARD_CONFIG

		# fill up rootfs size to multiples of 512
		ROOTFS_SIZE=`ls -l $TOP_DIR/buildroot/output/rockchip_rk312x/images/rootfs.$RK_ROOTFS_TYPE | awk -F ' ' {'print $5'}`
		echo "*** ROOTFS SIZE(type : $RK_ROOTFS_TYPE) : $ROOTFS_SIZE ***"
		part_name=` grep "CMDLINE: mtdparts=rk29xxnand:" $TOP_DIR/device/rockchip/rk3126c/parameter-buildroot.txt | awk -F ',' {'print $4'} | cut -d '(' -f2|cut -d ')' -f1`
		echo "fillup target partition : $part_name"
		if [ "$part_name" == "rootfs" ];then
			fillup_bytes=$[$ROOTFS_SIZE%512]
			echo -e '\033[0;32;1m'
			echo "part_name=$part_name rootfs_size=$ROOTFS_SIZE fillup=$fillup_bytes"
			echo -e '\033[0m'

			dd if=/dev/zero bs=1 count=$fillup_bytes >> $TOP_DIR/buildroot/output/rockchip_rk312x/images/rootfs.$RK_ROOTFS_TYPE
			cksum=`cksum $TOP_DIR/buildroot/output/rockchip_rk312x/images/rootfs.$RK_ROOTFS_TYPE | awk -F ' ' {'print $1'}`
			size=`cksum $TOP_DIR/buildroot/output/rockchip_rk312x/images/rootfs.$RK_ROOTFS_TYPE | awk -F ' ' {'print $2'}`
			
			echo "cksum=$cksum size=$size"
		else
			echo -e '\033[0;31;1m'
			echo "*** fillup target parition isn't rootfs ***"
			echo -e '\033[0m'
			exit 1
		fi
	fi

	if [ $? -eq 0 ]; then
		echo "====Build buildroot ok!===="
	else
		echo "====Build buildroot failed!===="
		exit 1
	fi
}

function build_ramboot(){
	echo "=========Start build ramboot========="
	echo "TARGET_RAMBOOT_CONFIG=$RK_CFG_RAMBOOT"
	echo "====================================="
	/usr/bin/time -f "you take %E to build ramboot" $COMMON_DIR/mk-ramdisk.sh ramboot.img $RK_CFG_RAMBOOT
	if [ $? -eq 0 ]; then
		rm $TOP_DIR/rockdev/boot.img
		ln -s $TOP_DIR/buildroot/output/$RK_CFG_RAMBOOT/images/ramboot.img $TOP_DIR/rockdev/boot.img
		echo "====Build ramboot ok!===="
	else
		echo "====Build ramboot failed!===="
		exit 1
	fi
}

function build_multi-npu_boot(){
	if [ -z "$RK_MULTINPU_BOOT" ]; then
		echo "=========Please set 'RK_MULTINPU_BOOT=y' in BoardConfig.mk========="
		exit 1
	fi
	echo "=========Start build multi-npu boot========="
	echo "TARGET_RAMBOOT_CONFIG=$RK_CFG_RAMBOOT"
	echo "====================================="
	/usr/bin/time -f "you take %E to build multi-npu boot" $COMMON_DIR/mk-multi-npu_boot.sh
	if [ $? -eq 0 ]; then
		echo "====Build multi-npu boot ok!===="
	else
		echo "====Build multi-npu boot failed!===="
		exit 1
	fi
}

function build_yocto(){
	if [ -z "$RK_YOCTO_MACHINE" ]; then
		echo "This board doesn't support yocto!"
		exit 1
	fi

	echo "=========Start build ramboot========="
	echo "TARGET_MACHINE=$RK_YOCTO_MACHINE"
	echo "====================================="

	export LANG=en_US.UTF-8 LANGUAGE=en_US.en LC_ALL=en_US.UTF-8

	cd yocto
	ln -sf $RK_YOCTO_MACHINE.conf build/conf/local.conf
	source oe-init-build-env
	cd ..
	bitbake core-image-minimal -r conf/include/rksdk.conf

	if [ $? -eq 0 ]; then
		echo "====Build yocto ok!===="
	else
		echo "====Build yocto failed!===="
		exit 1
	fi
}

function build_debian(){
	cd debian

	if [ "$RK_ARCH" == "arm" ]; then
		ARCH=armhf
	fi
	if [ "$RK_ARCH" == "arm64" ]; then
		ARCH=arm64
	fi

	if [ ! -e linaro-stretch-alip-*.tar.gz ]; then
		echo "\033[36m Run mk-base-debian.sh first \033[0m"
		RELEASE=stretch TARGET=desktop ARCH=$ARCH ./mk-base-debian.sh
	fi

	VERSION=debug ARCH=$ARCH ./mk-rootfs-stretch.sh

	./mk-image.sh
	cd ..
	if [ $? -eq 0 ]; then
		echo "====Build Debian9 ok!===="
	else
		echo "====Build Debian9 failed!===="
		exit 1
	fi
}

function build_distro(){
	echo "===========Start build debian==========="
	echo "TARGET_ARCH=$RK_ARCH"
	echo "RK_DISTRO_DEFCONFIG=$RK_DISTRO_DEFCONFIG"
	echo "========================================"
	cd distro && make $RK_DISTRO_DEFCONFIG && /usr/bin/time -f "you take %E to build debian" $TOP_DIR/distro/make.sh && cd -
	if [ $? -eq 0 ]; then
		echo "====Build debian ok!===="
	else
		echo "====Build debian failed!===="
		exit 1
	fi
}

function build_rootfs(){
	rm -f ${TOP_DIR}/buildroot/output/$RK_CFG_BUILDROOT/images/rootfs.squashfs

	case "$1" in
		yocto)
			build_yocto
			ROOTFS_IMG=yocto/build/tmp/deploy/images/$RK_YOCTO_MACHINE/rootfs.img
			;;
		debian)
			build_debian
			ROOTFS_IMG=debian/linaro-rootfs.img
			;;
		distro)
			build_distro
			ROOTFS_IMG=distro/output/images/rootfs.$RK_ROOTFS_TYPE
			;;
		*)
			build_buildroot
			ROOTFS_IMG=buildroot/output/$RK_CFG_BUILDROOT/images/rootfs.$RK_ROOTFS_TYPE
			;;
	esac

	[ -z "$ROOTFS_IMG" ] && return

	if [ ! -f "$ROOTFS_IMG" ]; then
		echo "$ROOTFS_IMG not generated?"
	else
		mkdir -p ${RK_ROOTFS_IMG%/*}
		ln -rsf $TOP_DIR/$ROOTFS_IMG $RK_ROOTFS_IMG
	fi
}

function build_recovery(){
	echo "==========Start build recovery=========="
	echo "TARGET_RECOVERY_CONFIG=$RK_CFG_RECOVERY"
	echo "========================================"
	/usr/bin/time -f "you take %E to build recovery" $COMMON_DIR/mk-ramdisk.sh recovery.img $RK_CFG_RECOVERY
	if [ $? -eq 0 ]; then
		echo "====Build recovery ok!===="
	else
		echo "====Build recovery failed!===="
		exit 1
	fi
}

function build_pcba(){
	echo "==========Start build pcba=========="
	echo "TARGET_PCBA_CONFIG=$RK_CFG_PCBA"
	echo "===================================="
	/usr/bin/time -f "you take %E to build pcba" $COMMON_DIR/mk-ramdisk.sh pcba.img $RK_CFG_PCBA
	if [ $? -eq 0 ]; then
		echo "====Build pcba ok!===="
	else
		echo "====Build pcba failed!===="
		exit 1
	fi
}

function build_all(){
	echo "============================================"
	echo "TARGET_ARCH=$RK_ARCH"
	echo "TARGET_PLATFORM=$RK_TARGET_PRODUCT"
	echo "TARGET_UBOOT_CONFIG=$RK_UBOOT_DEFCONFIG"
	echo "TARGET_SPL_CONFIG=$RK_SPL_DEFCONFIG"
	echo "TARGET_KERNEL_CONFIG=$RK_KERNEL_DEFCONFIG"
	echo "TARGET_KERNEL_DTS=$RK_KERNEL_DTS"
	echo "TARGET_TOOLCHAIN_CONFIG=$RK_CFG_TOOLCHAIN"
	echo "TARGET_BUILDROOT_CONFIG=$RK_CFG_BUILDROOT"
	echo "TARGET_RECOVERY_CONFIG=$RK_CFG_RECOVERY"
	echo "TARGET_PCBA_CONFIG=$RK_CFG_PCBA"
	echo "TARGET_RAMBOOT_CONFIG=$RK_CFG_RAMBOOT"
	echo "============================================"

	#note: if build spl, it will delete loader.bin in uboot directory,
	# so can not build uboot and spl at the same time.
	if [ -z $RK_SPL_DEFCONFIG ]; then
		build_uboot
	else
		build_spl
	fi

	build_loader
	build_kernel
	build_toolchain && \
	build_rootfs ${RK_ROOTFS_SYSTEM:-buildroot}
	build_recovery
	build_ramboot
}

function build_cleanall(){
	echo "clean uboot, kernel, rootfs, recovery"
	cd $TOP_DIR/u-boot/ && make distclean && cd -
	cd $TOP_DIR/kernel && make distclean && cd -
	rm -rf $TOP_DIR/buildroot/output
	rm -rf $TOP_DIR/yocto/build/tmp
	rm -rf $TOP_DIR/distro/output
	rm -rf $TOP_DIR/debian/binary
}

function build_firmware(){
	./mkfirmware.sh $BOARD_CONFIG
	if [ $? -eq 0 ]; then
		echo "==== Make image ok! ===="
	else
		echo "==== Make image failed! ===="
		exit 1
	fi
}

function build_updateimg(){
	LCD_FREQ_BYTE=`grep -i 'clock-frequency' kernel/arch/arm/boot/dts/rk3126-linux.dts | head -n1 | awk -F '[<>]' {'print $2'}`
	LCD_FREQ_MB=$(($LCD_FREQ_BYTE/1000000))
	LCD_H=`grep -i 'hactive' kernel/arch/arm/boot/dts/rk3126-linux.dts | head -n1 | awk -F '[<>]' {'print $2'}`
	LCD_V=`grep -i 'vactive' kernel/arch/arm/boot/dts/rk3126-linux.dts | head -n1 | awk -F '[<>]' {'print $2'}`
	CUR_DATE=`date +"%Y%m%d"`
	IMG_NAME="${CUR_DATE}_${GAME_TYPE}_${PACK_DEBUG}_lvds17_${LCD_H}x${LCD_V}_256M_${LCD_FREQ_MB}mhz-6b.img"
	IMG_ZIP_NAME="${CUR_DATE}_${GAME_TYPE}_${PACK_DEBUG}_lvds17_${LCD_H}x${LCD_V}_256M_${LCD_FREQ_MB}mhz-6b.zip"
	IMAGE_PATH=$TOP_DIR/rockdev
	PACK_TOOL_DIR=$TOP_DIR/tools/linux/Linux_Pack_Firmware
	if [ "$RK_LINUX_AB_ENABLE"x = "true"x ];then
		echo "Make Linux a/b update.img."
		build_otapackage
		source_package_file_name=`ls -lh $PACK_TOOL_DIR/rockdev/package-file | awk -F ' ' '{print $NF}'`
		cd $PACK_TOOL_DIR/rockdev && ln -fs "$source_package_file_name"-ab package-file && ./mkupdate.sh && cd -
		mv $PACK_TOOL_DIR/rockdev/update.img $IMAGE_PATH/update_ab.img
		cd $PACK_TOOL_DIR/rockdev && ln -fs $source_package_file_name package-file && cd -
		if [ $? -eq 0 ]; then
			echo "Make Linux a/b update image ok!"
		else
			echo "Make Linux a/b update image failed!"
			exit 1
		fi

	else
		mkdir -p ${IMAGE_PATH}/tmp
		echo "Make ckeck.img"
		if [ ! -f ${IMAGE_PATH}/check.img ]; then
			dd if=/dev/zero of=${IMAGE_PATH}/check.img bs=1M count=1
			mkfs.fat ${IMAGE_PATH}/check.img
		fi

		# mount ckeck.img to store cksum.txt & size.txt
		sudo mount ${IMAGE_PATH}/check.img ${IMAGE_PATH}/tmp
		sudo sh -c "printf "%010d" $cksum > ${IMAGE_PATH}/tmp/cksum.txt"
		sudo sh -c "printf "%010d" $cksum > ${IMAGE_PATH}/cksum.txt"
		rootfs_of_512_multiples=$[$size/512]
		sudo sh -c "echo "$rootfs_of_512_multiples" > ${IMAGE_PATH}/tmp/size.txt"
		sudo umount ${IMAGE_PATH}/tmp
		sudo rm -r ${IMAGE_PATH}/tmp

		echo "Make  update.img"
		if [ -f "$PACK_TOOL_DIR/rockdev/$RK_PACKAGE_FILE" ]; then
			source_package_file_name=`ls -lh $PACK_TOOL_DIR/rockdev/package-file | awk -F ' ' '{print $NF}'`

			cd $PACK_TOOL_DIR/rockdev && \
				ln -fs "$PACK_TOOL_DIR/rockdev/$RK_PACKAGE_FILE" package-file && \
				./mkupdate.sh && cd -

			cd $PACK_TOOL_DIR/rockdev && \
				ln -fs $source_package_file_name package-file && cd -
		else
			cd $PACK_TOOL_DIR/rockdev && ./mkupdate.sh && cd -
		fi
		
		mv $PACK_TOOL_DIR/rockdev/update.img $IMAGE_PATH
		mv $IMAGE_PATH/update.img $IMAGE_PATH/${IMG_NAME}
		cd ${IMAGE_PATH}
		zip ${IMG_ZIP_NAME} ${IMG_NAME} cksum.txt && cd -
		if [ $? -eq 0 ]; then
			echo -e '\033[0;32;1m'
			echo "Make update image ok!"
			echo "----------programmer image is at----------"
			echo 
			echo ${IMAGE_PATH}/${IMG_NAME}
			echo 
			echo "------------------------------------------"
                        echo -e '\033[0m'
		else
			echo -e '\033[0;31;1m'
			echo "Make update image failed!"
			echo -e '\033[0m'
			exit 1
		fi
	fi
}

function build_otapackage(){
	IMAGE_PATH=$TOP_DIR/rockdev
	PACK_TOOL_DIR=$TOP_DIR/tools/linux/Linux_Pack_Firmware

	echo "Make ota ab update.img"
	source_package_file_name=`ls -lh $PACK_TOOL_DIR/rockdev/package-file | awk -F ' ' '{print $NF}'`
	cd $PACK_TOOL_DIR/rockdev && ln -fs "$source_package_file_name"-ota package-file && ./mkupdate.sh && cd -
	mv $PACK_TOOL_DIR/rockdev/update.img $IMAGE_PATH/update_ota.img
	cd $PACK_TOOL_DIR/rockdev && ln -fs $source_package_file_name package-file && cd -
	if [ $? -eq 0 ]; then
		echo "Make update ota ab image ok!"
	else
		echo "Make update ota ab image failed!"
		exit 1
	fi
}

function build_save(){
	IMAGE_PATH=$TOP_DIR/rockdev
	DATE=$(date  +%Y%m%d.%H%M)
	STUB_PATH=Image/"$RK_KERNEL_DTS"_"$DATE"_RELEASE_TEST
	STUB_PATH="$(echo $STUB_PATH | tr '[:lower:]' '[:upper:]')"
	export STUB_PATH=$TOP_DIR/$STUB_PATH
	export STUB_PATCH_PATH=$STUB_PATH/PATCHES
	mkdir -p $STUB_PATH

	#Generate patches
	#$TOP_DIR/.repo/repo/repo forall -c "$TOP_DIR/device/rockchip/common/gen_patches_body.sh"

	#Copy stubs
	#$TOP_DIR/.repo/repo/repo manifest -r -o $STUB_PATH/manifest_${DATE}.xml
	mkdir -p $STUB_PATCH_PATH/kernel
	cp $TOP_DIR/kernel/.config $STUB_PATCH_PATH/kernel
	cp $TOP_DIR/kernel/vmlinux $STUB_PATCH_PATH/kernel
	mkdir -p $STUB_PATH/IMAGES/
	#cp $IMAGE_PATH/* $STUB_PATH/IMAGES/

	#Save build command info
	echo "UBOOT:  defconfig: $RK_UBOOT_DEFCONFIG" >> $STUB_PATH/build_cmd_info
	echo "KERNEL: defconfig: $RK_KERNEL_DEFCONFIG, dts: $RK_KERNEL_DTS" >> $STUB_PATH/build_cmd_info
	echo "BUILDROOT: $RK_CFG_BUILDROOT" >> $STUB_PATH/build_cmd_info

}

function build_allsave(){
	build_all
	build_firmware
	build_updateimg
	build_save
}

function build_debug(){
        build_all
        build_firmware
        build_updateimg
        build_save
}

function build_release(){
        build_all
        build_firmware
        build_updateimg
        build_save
}

#=========================
# build targets
#=========================

if echo $@|grep -wqE "help|-h"; then
	if [ -n "$2" -a "$(type -t usage$2)" == function ]; then
		echo "###Current SDK Default [ $2 ] Build Command###"
		eval usage$2
	else
		usage
	fi
	exit 0
fi

OPTIONS="${@:-allsave}"

[ -f "$TOP_DIR/device/rockchip/$RK_TARGET_PRODUCT/$RK_BOARD_PRE_BUILD_SCRIPT" ] \
	&& source "$TOP_DIR/device/rockchip/$RK_TARGET_PRODUCT/$RK_BOARD_PRE_BUILD_SCRIPT"  # board hooks

for option in ${OPTIONS}; do
	echo "processing option: $option"
	case $option in
		BoardConfig*.mk)
			option=$TOP_DIR/device/rockchip/$RK_TARGET_PRODUCT/$option
			;&
		*.mk)
			CONF=$(realpath $option)
			echo "switching to board: $CONF"
			if [ ! -f $CONF ]; then
				echo "not exist!"
				exit 1
			fi

			ln -sf $CONF $BOARD_CONFIG
			;;
		buildroot|debian|distro|yocto)
			build_rootfs $option
			;;
		lunch)
			build_select_board
			;;
		recovery)
			build_kernel
			;;
		debug)
			PACK_DEBUG="debug"
			uart_enable
			eval build_$option || usage
			;;
		release)
			PACK_DEBUG="release"
			uart_disable
			eval build_$option || usage
			;;
		*)
			PACK_DEBUG="debug"
			uart_enable
			eval build_$option || usage
			;;
	esac
done

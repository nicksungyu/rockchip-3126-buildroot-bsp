#!/bin/bash

# Target arch
export RK_ARCH=arm
# Uboot defconfig
export RK_UBOOT_DEFCONFIG=rk3126
# Trust ini config
export RK_TRUST_INI_CONFIG=RK3126TOS_LADDR.ini
# Uboot size
export RK_UBOOT_SIZE_CONFIG=1024\ 2
# Trust size
export RK_TRUST_SIZE_CONFIG=1024\ 2
# Kernel defconfig
export RK_KERNEL_DEFCONFIG=rockchip_linux_defconfig
# Kernel dts
export RK_KERNEL_DTS=rk3126-linux
# boot image type
export RK_BOOT_IMG=zboot.img
# kernel image path
export RK_KERNEL_IMG=kernel/arch/arm/boot/zImage
# parameter for GPT table
export RK_PARAMETER=parameter-buildroot.txt
# Buildroot config
export RK_CFG_BUILDROOT=rockchip_rk312x
# Recovery config
#export RK_CFG_RECOVERY=rockchip_rk312x_recovery
# ramboot config
export RK_CFG_RAMBOOT=
# Pcba config
export RK_CFG_PCBA=rockchip_rk3126c_pcba
# Build jobs
export RK_JOBS=12
# target chip
export RK_TARGET_PRODUCT=rk3126c
# Set rootfs type, including ext2 ext4 squashfs
export RK_ROOTFS_TYPE=squashfs
# rootfs image path
export RK_ROOTFS_IMG=rockdev/rootfs.${RK_ROOTFS_TYPE}
# Set oem partition type, including ext2 squashfs
export RK_OEM_FS_TYPE=ext2
# Set userdata partition type, including ext2, fat
export RK_USERDATA_FS_TYPE=ext2
#OEM config
export RK_OEM_DIR=oem_classof81
#userdata config
export RK_USERDATA_DIR=userdata_empty
#misc image
export RK_MISC=wipe_all-misc.img
#choose enable distro module
export RK_DISTRO_MODULE=
# recovery-rootfs image path
export RK_RECOVERY_KERENL_IMG=rockdev/recovery-kernel.img
# recovery-rootfs image path 
export RK_RECOVERY_ROOTFS_IMG=rockdev/recovery-rootfs.img
# boot_backup image path
export RK_KERNEL_BACKUP_IMG=rockdev/boot_backup.img

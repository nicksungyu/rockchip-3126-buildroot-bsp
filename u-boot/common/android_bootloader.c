/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <android_bootloader.h>
#include <android_bootloader_message.h>
#include <android_avb/avb_slot_verify.h>
#include <android_avb/avb_ops_user.h>
#include <android_avb/rk_avb_ops_user.h>
#include <android_image.h>
#include <bootm.h>
#include <asm/arch/hotkey.h>
#include <cli.h>
#include <common.h>
#include <dt_table.h>
#include <image-android-dt.h>
#include <malloc.h>
#include <fdt_support.h>
#include <fs.h>
#include <boot_rkimg.h>
#include <attestation_key.h>
#include <keymaster.h>
#include <linux/libfdt_env.h>
#include <optee_include/OpteeClientInterface.h>
#include <bidram.h>
#include <console.h>
#include <sysmem.h>

DECLARE_GLOBAL_DATA_PTR;

#define ANDROID_PARTITION_BOOT "boot"
#define ANDROID_PARTITION_MISC "misc"
#define ANDROID_PARTITION_OEM  "oem"
#define ANDROID_PARTITION_RECOVERY  "recovery"
#define ANDROID_PARTITION_SYSTEM "system"
#define ANDROID_PARTITION_VBMETA "vbmeta"
#define ANDROID_PARTITION_SUPER "super"

#define BTE_RECOVERY_PARTITION "recovery-kernel"

#define ANDROID_ARG_SLOT_SUFFIX "androidboot.slot_suffix="
#define ANDROID_ARG_ROOT "root="
#define ANDROID_ARG_SERIALNO "androidboot.serialno="
#define ANDROID_VERIFY_STATE "androidboot.verifiedbootstate="
#ifdef CONFIG_ROCKCHIP_RESOURCE_IMAGE
#define ANDROID_ARG_FDT_FILENAME "rk-kernel.dtb"
#else
#define ANDROID_ARG_FDT_FILENAME "kernel.dtb"
#endif
#define OEM_UNLOCK_ARG_SIZE 30
#define UUID_SIZE 37

#ifdef CONFIG_ANDROID_AB
static int is_support_dynamic_partition(struct blk_desc *dev_desc)
{
	disk_partition_t super_part_info;
	disk_partition_t boot_part_info;
	int part_num;
	int is_dp = 0;
	char *super_dp = NULL;
	char *super_info = "androidboot.super_partition=";

	memset(&super_part_info, 0x0, sizeof(super_part_info));
	part_num = part_get_info_by_name(dev_desc, ANDROID_PARTITION_SUPER,
					 &super_part_info);
	if (part_num < 0) {
		memset(&boot_part_info, 0x0, sizeof(boot_part_info));
		part_num = part_get_info_by_name(dev_desc, ANDROID_PARTITION_BOOT,
					 &boot_part_info);
		if (part_num < 0) {
			is_dp = 0;
		} else {
			andr_img_hdr hdr;
			ulong hdr_blocks = sizeof(struct andr_img_hdr) /
			boot_part_info.blksz;

			memset(&hdr, 0x0, sizeof(hdr));
			if (blk_dread(dev_desc, boot_part_info.start, hdr_blocks, &hdr) !=
				hdr_blocks) {
				is_dp = 0;
			} else {
				debug("hdr cmdline=%s\n", hdr.cmdline);
				super_dp = strstr(hdr.cmdline, super_info);
				if (super_dp != NULL) {
					is_dp = 1;
				} else {
					is_dp = 0;
				}
			}
		}
	} else {
		debug("Find super partition, the firmware support dynamic partition\n");
		is_dp = 1;
	}

	debug("%s is_dp=%d\n", __func__, is_dp);
	return is_dp;
}

static int get_partition_unique_uuid(char *partition,
				     char *guid_buf,
				     size_t guid_buf_size)
{
	struct blk_desc *dev_desc;
	disk_partition_t part_info;

	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: Could not find device\n", __func__);
		return -1;
	}

	if (part_get_info_by_name(dev_desc, partition, &part_info) < 0) {
		printf("Could not find \"%s\" partition\n", partition);
		return -1;
	}

	if (guid_buf && guid_buf_size > 0)
		memcpy(guid_buf, part_info.uuid, guid_buf_size);

	return 0;
}

static void update_root_uuid_if_android_ab(void)
{
	/*
	 * In android a/b & avb process, the system.img is mandory and the
	 * "root=" will be added in vbmeta.img.
	 *
	 * In linux a/b & avb process, the system is NOT mandory and the
	 * "root=" will not be added in vbmeta.img but in kernel dts bootargs.
	 * (Parsed and droped late, i.e. "root=" is not available now/always).
	 *
	 * To compatible with the above two processes, test the existence of
	 * "root=" and create it for linux ab & avb.
	 */

	char root_partuuid[70] = "root=PARTUUID=";
	char *boot_args = env_get("bootargs");
	char guid_buf[UUID_SIZE] = {0};
	struct blk_desc *dev_desc;

	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: Could not find device\n", __func__);
		return;
	}

	if (is_support_dynamic_partition(dev_desc)) {
		return;
	}

	if (!strstr(boot_args, "root=")) {
		get_partition_unique_uuid(ANDROID_PARTITION_SYSTEM,
					  guid_buf, UUID_SIZE);
		strcat(root_partuuid, guid_buf);
		env_update("bootargs", root_partuuid);
	}
}

static int decrease_tries_if_android_ab(char *slot_suffix)
{
	AvbABData ab_data_orig;
	AvbABData ab_data;
	AvbOps *ops;
	size_t slot_index = 0;

	/* TODO: get from pre-loader or misc partition */
	if (rk_avb_get_current_slot(slot_suffix)) {
		printf("rk_avb_get_current_slot() failed\n");
		return -1;
	}

	if (!strncmp(slot_suffix, "_a", 2))
		slot_index = 0;
	else if (!strncmp(slot_suffix, "_b", 2))
		slot_index = 1;
	else
		slot_index = 0;

	ops = avb_ops_user_new();
	if (!ops) {
		printf("avb_ops_user_new() failed!\n");
		return -1;
	}

	if (load_metadata(ops->ab_ops, &ab_data, &ab_data_orig)) {
		printf("Can not load metadata\n");
		return -1;
	}

	/* ... and decrement tries remaining, if applicable. */
	if (!ab_data.slots[slot_index].successful_boot &&
	    ab_data.slots[slot_index].tries_remaining > 0)
		ab_data.slots[slot_index].tries_remaining -= 1;

	if (save_metadata_if_changed(ops->ab_ops, &ab_data, &ab_data_orig)) {
		printf("Can not save metadata\n");
		return -1;
	}

	if (slot_suffix[0] != '_') {
#ifndef CONFIG_ANDROID_AVB
		printf("###There is no bootable slot, bring up lastboot!###\n");
		if (rk_get_lastboot() == 1)
			memcpy(slot_suffix, "_b", 2);
		else if (rk_get_lastboot() == 0)
			memcpy(slot_suffix, "_a", 2);
		else
#endif
			return -1;
	}

	return 0;
}
#else
static inline void update_root_uuid_if_android_ab(void) {}
static inline int decrease_tries_if_android_ab(char *slot_suffix) { return 0; }
#endif

#if defined(CONFIG_ANDROID_AB) && defined(CONFIG_ANDROID_AVB)
static void reset_cpu_if_android_ab(void)
{
	printf("Reset in AB system.\n");
	flushc();
	/*
	 * Since we use the retry-count in ab system, then can
	 * try reboot if verify fail until the retry-count is
	 * equal to zero.
	 */
	reset_cpu(0);
}
#else
static inline void reset_cpu_if_android_ab(void) {}
#endif

int android_bootloader_message_load(
	struct blk_desc *dev_desc,
	const disk_partition_t *part_info,
	struct android_bootloader_message *message)
{
	ulong message_blocks = sizeof(struct android_bootloader_message) /
	    part_info->blksz;
	if (message_blocks > part_info->size) {
		printf("misc partition too small.\n");
		return -1;
	}

	if (blk_dread(dev_desc, part_info->start + android_bcb_msg_sector_offset(),
	     message_blocks, message) !=
	    message_blocks) {
		printf("Could not read from misc partition\n");
		return -1;
	}
	debug("ANDROID: Loaded BCB, %lu blocks.\n", message_blocks);
	return 0;
}

static int android_bootloader_message_write(
	struct blk_desc *dev_desc,
	const disk_partition_t *part_info,
	struct android_bootloader_message *message)
{
	ulong message_blocks = sizeof(struct android_bootloader_message) /
	    part_info->blksz + android_bcb_msg_sector_offset();

	if (message_blocks > part_info->size) {
		printf("misc partition too small.\n");
		return -1;
	}

	if (blk_dwrite(dev_desc, part_info->start, message_blocks, message) !=
	    message_blocks) {
		printf("Could not write to misc partition\n");
		return -1;
	}
	debug("ANDROID: Wrote new BCB, %lu blocks.\n", message_blocks);
	return 0;
}

static enum android_boot_mode android_bootloader_load_and_clear_mode(
	struct blk_desc *dev_desc,
	const disk_partition_t *misc_part_info)
{
	struct android_bootloader_message bcb;
#ifdef CONFIG_FASTBOOT
	char *bootloader_str;

	/* Check for message from bootloader stored in RAM from a previous boot.
	 */
	bootloader_str = (char *)CONFIG_FASTBOOT_BUF_ADDR;
	if (!strcmp("reboot-bootloader", bootloader_str)) {
		bootloader_str[0] = '\0';
		return ANDROID_BOOT_MODE_BOOTLOADER;
	}
#endif

	/* Check and update the BCB message if needed. */
	if (android_bootloader_message_load(dev_desc, misc_part_info, &bcb) <
	    0) {
		printf("WARNING: Unable to load the BCB.\n");
		return ANDROID_BOOT_MODE_NORMAL;
	}

	if (!strcmp("bootonce-bootloader", bcb.command)) {
		/* Erase the message in the BCB since this value should be used
		 * only once.
		 */
		memset(bcb.command, 0, sizeof(bcb.command));
		android_bootloader_message_write(dev_desc, misc_part_info,
						 &bcb);
		return ANDROID_BOOT_MODE_BOOTLOADER;
	}

	if (!strcmp("boot-recovery", bcb.command))
		return ANDROID_BOOT_MODE_RECOVERY;

	if (!strcmp("boot-fastboot", bcb.command))
		return ANDROID_BOOT_MODE_RECOVERY;

	return ANDROID_BOOT_MODE_NORMAL;
}

int android_bcb_write(char *cmd)
{
	struct android_bootloader_message message = {0};
	disk_partition_t part_info;
	struct blk_desc *dev_desc;
	int ret;

	if (!cmd)
		return -ENOMEM;

	if (strlen(cmd) >= 32)
		return -ENOMEM;

	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: dev_desc is NULL!\n", __func__);
		return -ENODEV;
	}

	ret = part_get_info_by_name(dev_desc, ANDROID_PARTITION_MISC, &part_info);
	if (ret < 0) {
		printf("%s: Could not found misc partition, just run recovery\n",
		       __func__);
		return -ENODEV;
	}

	strcpy(message.command, cmd);
	return android_bootloader_message_write(dev_desc, &part_info, &message);
}

/**
 * Return the reboot reason string for the passed boot mode.
 *
 * @param mode	The Android Boot mode.
 * @return a pointer to the reboot reason string for mode.
 */
static const char *android_boot_mode_str(enum android_boot_mode mode)
{
	switch (mode) {
	case ANDROID_BOOT_MODE_NORMAL:
		return "(none)";
	case ANDROID_BOOT_MODE_RECOVERY:
		return "recovery";
	case ANDROID_BOOT_MODE_BOOTLOADER:
		return "bootloader";
	case ANDROID_BOOT_MODE_BTE_RECOVERY:
		return "bte_recovery";
	}
	return NULL;
}

static int android_bootloader_boot_bootloader(void)
{
	const char *fastboot_cmd = env_get("fastbootcmd");

	if (fastboot_cmd == NULL) {
		printf("fastboot_cmd is null, run default fastboot_cmd!\n");
		fastboot_cmd = "fastboot usb 0";
	}

	return run_command(fastboot_cmd, CMD_FLAG_ENV);
}

#ifdef CONFIG_SUPPORT_OEM_DTB
static int android_bootloader_get_fdt(const char *part_name,
		const char *load_file_name)
{
	struct blk_desc *dev_desc;
	disk_partition_t part_info;
	char *fdt_addr = NULL;
	char dev_part[3] = {0};
	loff_t bytes = 0;
	loff_t pos = 0;
	loff_t len_read;
	unsigned long addr = 0;
	int part_num = -1;
	int ret;

	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: dev_desc is NULL!\n", __func__);
		return -1;
	}

	part_num = part_get_info_by_name(dev_desc, part_name, &part_info);
	if (part_num < 0) {
		printf("ANDROID: Could not find partition \"%s\"\n", part_name);
		return -1;
	}

	snprintf(dev_part, ARRAY_SIZE(dev_part), ":%x", part_num);
	if (fs_set_blk_dev_with_part(dev_desc, part_num))
		return -1;

	fdt_addr = env_get("fdt_addr_r");
	if (!fdt_addr) {
		printf("ANDROID: No Found FDT Load Address.\n");
		return -1;
	}
	addr = simple_strtoul(fdt_addr, NULL, 16);

	ret = fs_read(load_file_name, addr, pos, bytes, &len_read);
	if (ret < 0)
		return -1;

	return 0;
}
#endif

/*
 *   Test on RK3308 AARCH64 mode (Cortex A35 816 MHZ) boot with eMMC:
 *
 *   |-------------------------------------------------------------------|
 *   | Format    |  Size(Byte) | Ratio | Decomp time(ms) | Boot time(ms) |
 *   |-------------------------------------------------------------------|
 *   | Image     | 7720968     |       |                 |     488       |
 *   |-------------------------------------------------------------------|
 *   | Image.lz4 | 4119448     | 53%   |       59        |     455       |
 *   |-------------------------------------------------------------------|
 *   | Image.lzo | 3858322     | 49%   |       141       |     536       |
 *   |-------------------------------------------------------------------|
 *   | Image.gz  | 3529108     | 45%   |       222       |     609       |
 *   |-------------------------------------------------------------------|
 *   | Image.bz2 | 3295914     | 42%   |       2940      |               |
 *   |-------------------------------------------------------------------|
 *   | Image.lzma| 2683750     | 34%   |                 |               |
 *   |-------------------------------------------------------------------|
 */
static int sysmem_alloc_uncomp_kernel(ulong andr_hdr,
				      ulong uncomp_kaddr, u32 comp)
{
	struct andr_img_hdr *hdr = (struct andr_img_hdr *)andr_hdr;
	ulong ksize, kaddr;

	if (comp != IH_COMP_NONE) {
		/* Release compressed sysmem */
		kaddr = env_get_hex("kernel_addr_c", 0);
		if (!kaddr)
			kaddr = env_get_hex("kernel_addr_r", 0);
		kaddr -= hdr->page_size;
		if (sysmem_free((phys_addr_t)kaddr))
			return -EINVAL;

		/*
		 * Use smaller Ratio to get larger estimated uncompress
		 * kernel size.
		 */
		if (comp == IH_COMP_ZIMAGE)
			ksize = hdr->kernel_size * 100 / 45;
		else if (comp == IH_COMP_LZ4)
			ksize = hdr->kernel_size * 100 / 50;
		else if (comp == IH_COMP_LZO)
			ksize = hdr->kernel_size * 100 / 45;
		else if (comp == IH_COMP_GZIP)
			ksize = hdr->kernel_size * 100 / 40;
		else if (comp == IH_COMP_BZIP2)
			ksize = hdr->kernel_size * 100 / 40;
		else if (comp == IH_COMP_LZMA)
			ksize = hdr->kernel_size * 100 / 30;
		else
			ksize = hdr->kernel_size;

		kaddr = uncomp_kaddr;
		ksize = ALIGN(ksize, 512);
		if (!sysmem_alloc_base(MEM_UNCOMP_KERNEL,
				       (phys_addr_t)kaddr, ksize))
			return -ENOMEM;
	}

	return 0;
}

int android_bootloader_boot_kernel(unsigned long kernel_address)
{
	char *kernel_addr_r = env_get("kernel_addr_r");
	char *kernel_addr_c = env_get("kernel_addr_c");
	char *fdt_addr = env_get("fdt_addr_r");
	char kernel_addr_str[12];
	char comp_str[32] = {0};
	ulong comp_type;
	const char *comp_name[] = {
		[IH_COMP_NONE]  = "IMAGE",
		[IH_COMP_GZIP]  = "GZIP",
		[IH_COMP_BZIP2] = "BZIP2",
		[IH_COMP_LZMA]  = "LZMA",
		[IH_COMP_LZO]   = "LZO",
		[IH_COMP_LZ4]   = "LZ4",
		[IH_COMP_ZIMAGE]= "ZIMAGE",
	};
	char *bootm_args[] = {
		kernel_addr_str, kernel_addr_str, fdt_addr, NULL };

	comp_type = env_get_ulong("os_comp", 10, 0);
	sprintf(kernel_addr_str, "0x%08lx", kernel_address);

	if (comp_type != IH_COMP_NONE) {
		if (comp_type == IH_COMP_ZIMAGE &&
		    kernel_addr_r && !kernel_addr_c) {
			kernel_addr_c = kernel_addr_r;
			kernel_addr_r = __stringify(CONFIG_SYS_SDRAM_BASE);
		}
		snprintf(comp_str, 32, "%s%s%s",
			 "(Uncompress to ", kernel_addr_r, ")");
	}

	printf("Booting %s kernel at %s%s with fdt at %s...\n\n\n",
	       comp_name[comp_type],
	       comp_type != IH_COMP_NONE ? kernel_addr_c : kernel_addr_r,
	       comp_str, fdt_addr);

	hotkey_run(HK_SYSMEM);

	/*
	 * Check whether there is enough space for uncompress kernel,
	 * Actually, here only gives a sysmem warning message when failed
	 * but never return -1.
	 */
	if (sysmem_alloc_uncomp_kernel(kernel_address,
				       simple_strtoul(kernel_addr_r, NULL, 16),
				       comp_type))
		return -1;

	return do_bootm_states(NULL, 0, ARRAY_SIZE(bootm_args), bootm_args,
		BOOTM_STATE_START |
		BOOTM_STATE_FINDOS | BOOTM_STATE_FINDOTHER |
		BOOTM_STATE_LOADOS |
#ifdef CONFIG_SYS_BOOT_RAMDISK_HIGH
		BOOTM_STATE_RAMDISK |
#endif
		BOOTM_STATE_OS_PREP | BOOTM_STATE_OS_FAKE_GO |
		BOOTM_STATE_OS_GO, &images, 1);
}

static char *strjoin(const char **chunks, char separator)
{
	int len, joined_len = 0;
	char *ret, *current;
	const char **p;

	for (p = chunks; *p; p++)
		joined_len += strlen(*p) + 1;

	if (!joined_len) {
		ret = malloc(1);
		if (ret)
			ret[0] = '\0';
		return ret;
	}

	ret = malloc(joined_len);
	current = ret;
	if (!ret)
		return ret;

	for (p = chunks; *p; p++) {
		len = strlen(*p);
		memcpy(current, *p, len);
		current += len;
		*current = separator;
		current++;
	}
	/* Replace the last separator by a \0. */
	current[-1] = '\0';
	return ret;
}

/** android_assemble_cmdline - Assemble the command line to pass to the kernel
 * @return a newly allocated string
 */
char *android_assemble_cmdline(const char *slot_suffix,
				      const char *extra_args)
{
	const char *cmdline_chunks[16];
	const char **current_chunk = cmdline_chunks;
	char *env_cmdline, *cmdline, *rootdev_input, *serialno;
	char *allocated_suffix = NULL;
	char *allocated_serialno = NULL;
	char *allocated_rootdev = NULL;
	unsigned long rootdev_len;

	env_cmdline = env_get("bootargs");
	if (env_cmdline)
		*(current_chunk++) = env_cmdline;

	/* The |slot_suffix| needs to be passed to the kernel to know what
	 * slot to boot from.
	 */
	if (slot_suffix) {
		allocated_suffix = malloc(strlen(ANDROID_ARG_SLOT_SUFFIX) +
					  strlen(slot_suffix) + 1);
		memset(allocated_suffix, 0, strlen(ANDROID_ARG_SLOT_SUFFIX)
		       + strlen(slot_suffix) + 1);
		strcpy(allocated_suffix, ANDROID_ARG_SLOT_SUFFIX);
		strcat(allocated_suffix, slot_suffix);
		*(current_chunk++) = allocated_suffix;
	}

	serialno = env_get("serial#");
	if (serialno) {
		allocated_serialno = malloc(strlen(ANDROID_ARG_SERIALNO) +
					  strlen(serialno) + 1);
		memset(allocated_serialno, 0, strlen(ANDROID_ARG_SERIALNO) +
				strlen(serialno) + 1);
		strcpy(allocated_serialno, ANDROID_ARG_SERIALNO);
		strcat(allocated_serialno, serialno);
		*(current_chunk++) = allocated_serialno;
	}

	rootdev_input = env_get("android_rootdev");
	if (rootdev_input) {
		rootdev_len = strlen(ANDROID_ARG_ROOT) + CONFIG_SYS_CBSIZE + 1;
		allocated_rootdev = malloc(rootdev_len);
		strcpy(allocated_rootdev, ANDROID_ARG_ROOT);
		cli_simple_process_macros(rootdev_input,
					  allocated_rootdev +
					  strlen(ANDROID_ARG_ROOT));
		/* Make sure that the string is null-terminated since the
		 * previous could not copy to the end of the input string if it
		 * is too big.
		 */
		allocated_rootdev[rootdev_len - 1] = '\0';
		*(current_chunk++) = allocated_rootdev;
	}

	if (extra_args)
		*(current_chunk++) = extra_args;

	*(current_chunk++) = NULL;
	cmdline = strjoin(cmdline_chunks, ' ');
	free(allocated_suffix);
	free(allocated_rootdev);
	return cmdline;
}

#ifdef CONFIG_ANDROID_AVB
static void slot_set_unbootable(AvbABSlotData* slot)
{
	slot->priority = 0;
	slot->tries_remaining = 0;
	slot->successful_boot = 0;
}

static AvbSlotVerifyResult android_slot_verify(char *boot_partname,
			       unsigned long *android_load_address,
			       char *slot_suffix)
{
	const char *requested_partitions[1] = {NULL};
	uint8_t unlocked = true;
	AvbOps *ops;
	AvbSlotVerifyFlags flags;
	AvbSlotVerifyData *slot_data[1] = {NULL};
	AvbSlotVerifyResult verify_result;
	AvbABData ab_data, ab_data_orig;
	size_t slot_index_to_boot = 0;
	char verify_state[38] = {0};
	char can_boot = 1;
	unsigned long load_address = *android_load_address;
	struct andr_img_hdr *hdr;

	requested_partitions[0] = boot_partname;
	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!\n");
		return AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
	}

	if (ops->read_is_device_unlocked(ops, (bool *)&unlocked) != AVB_IO_RESULT_OK)
		printf("Error determining whether device is unlocked.\n");

	printf("read_is_device_unlocked() ops returned that device is %s\n",
	       (unlocked & LOCK_MASK)? "UNLOCKED" : "LOCKED");

	flags = AVB_SLOT_VERIFY_FLAGS_NONE;
	if (unlocked & LOCK_MASK)
		flags |= AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR;

	if(load_metadata(ops->ab_ops, &ab_data, &ab_data_orig)) {
		printf("Can not load metadata\n");
		return AVB_SLOT_VERIFY_RESULT_ERROR_IO;
	}

	if (!strncmp(slot_suffix, "_a", 2))
		slot_index_to_boot = 0;
	else if (!strncmp(slot_suffix, "_b", 2))
		slot_index_to_boot = 1;
	else
		slot_index_to_boot = 0;

	verify_result =
	avb_slot_verify(ops,
			requested_partitions,
			slot_suffix,
			flags,
			AVB_HASHTREE_ERROR_MODE_RESTART,
			&slot_data[0]);

	strcat(verify_state, ANDROID_VERIFY_STATE);
	switch (verify_result) {
	case AVB_SLOT_VERIFY_RESULT_OK:
		if (unlocked & LOCK_MASK)
			strcat(verify_state, "orange");
		else
			strcat(verify_state, "green");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED:
		if (unlocked & LOCK_MASK)
			strcat(verify_state, "orange");
		else
			strcat(verify_state, "yellow");
		break;
	case AVB_SLOT_VERIFY_RESULT_ERROR_OOM:
	case AVB_SLOT_VERIFY_RESULT_ERROR_IO:
	case AVB_SLOT_VERIFY_RESULT_ERROR_INVALID_METADATA:
	case AVB_SLOT_VERIFY_RESULT_ERROR_UNSUPPORTED_VERSION:
	case AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION:
	case AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX:
	default:
		if (unlocked & LOCK_MASK)
			strcat(verify_state, "orange");
		else
			strcat(verify_state, "red");
		break;
	}

	if (!slot_data[0]) {
		can_boot = 0;
		goto out;
	}

	if (verify_result == AVB_SLOT_VERIFY_RESULT_OK ||
	    verify_result == AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED ||
	    (unlocked & LOCK_MASK)) {
		int len = 0;
		char *bootargs, *newbootargs;

		if (*slot_data[0]->cmdline) {
			debug("Kernel command line: %s\n", slot_data[0]->cmdline);
			len += strlen(slot_data[0]->cmdline);
		}

		bootargs = env_get("bootargs");
		if (bootargs)
			len += strlen(bootargs);

		newbootargs = malloc(len + 2);

		if (!newbootargs) {
			puts("Error: malloc in android_slot_verify failed!\n");
			return AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
		}
		*newbootargs = '\0';

		if (bootargs) {
			strcpy(newbootargs, bootargs);
			strcat(newbootargs, " ");
		}
		if (*slot_data[0]->cmdline)
			strcat(newbootargs, slot_data[0]->cmdline);
		env_set("bootargs", newbootargs);

		/* Reserve page_size */
		hdr = (void *)slot_data[0]->loaded_partitions->data;
		load_address -= hdr->page_size;
		if (android_image_memcpy_separate(hdr, &load_address)) {
			printf("Failed to separate copy android image\n");
			return AVB_SLOT_VERIFY_RESULT_ERROR_IO;
		}
		*android_load_address = load_address;
	} else {
		slot_set_unbootable(&ab_data.slots[slot_index_to_boot]);
	}

out:
#if defined(CONFIG_ANDROID_AB) && !defined(CONFIG_ANDROID_AVB)
	/*
	 * In ab & avb process, the tries_remaining minus one in function
	 * android_slot_verify, shield this function here.
	 */
	/* ... and decrement tries remaining, if applicable. */
	if (!ab_data.slots[slot_index_to_boot].successful_boot &&
	    ab_data.slots[slot_index_to_boot].tries_remaining > 0) {
		ab_data.slots[slot_index_to_boot].tries_remaining -= 1;
	}
#endif
	env_update("bootargs", verify_state);
	if (save_metadata_if_changed(ops->ab_ops, &ab_data, &ab_data_orig)) {
		printf("Can not save metadata\n");
		verify_result = AVB_SLOT_VERIFY_RESULT_ERROR_IO;
	}

	if (slot_data[0] != NULL)
		avb_slot_verify_data_free(slot_data[0]);

	if ((unlocked & LOCK_MASK) && can_boot)
		return 0;
	else
		return verify_result;
}
#endif

#if defined(CONFIG_CMD_DTIMG) && defined(CONFIG_OF_LIBFDT_OVERLAY)

/*
 * Default return index 0.
 */
__weak int board_select_fdt_index(ulong dt_table_hdr)
{
/*
 * User can use "dt_for_each_entry(entry, hdr, idx)" to iterate
 * over all dt entry of DT image and pick up which they want.
 *
 * Example:
 *	struct dt_table_entry *entry;
 *	int index;
 *
 *	dt_for_each_entry(entry, dt_table_hdr, index) {
 *
 *		.... (use entry)
 *	}
 *
 *	return index;
 */
	return 0;
}

static int android_get_dtbo(ulong *fdt_dtbo,
			    const struct andr_img_hdr *hdr,
			    int *index, int boot_mode)
{
	struct dt_table_header *dt_hdr = NULL;
	struct blk_desc *dev_desc;
	const char *part_name;
	disk_partition_t part_info;
	u32 blk_offset, blk_cnt;
	void *buf;
	ulong e_addr;
	u32 e_size;
	int e_idx;
	int ret;

	/* Get partition according to boot mode */
	if (boot_mode == BOOT_MODE_RECOVERY)
		part_name = PART_RECOVERY;
	else
		part_name = PART_DTBO;

	/* Get partition info */
	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: dev_desc is NULL!\n", __func__);
		return -ENODEV;
	}

	ret = part_get_info_by_name(dev_desc, part_name, &part_info);
	if (ret < 0) {
		printf("%s: failed to get %s part info, ret=%d\n",
		       __func__, part_name, ret);
		return ret;
	}

	/* Check dt table header */
	if (!strcmp(part_name, PART_RECOVERY))
		blk_offset = part_info.start +
			     (hdr->recovery_dtbo_offset / part_info.blksz);
	else
		blk_offset = part_info.start;

	dt_hdr = memalign(ARCH_DMA_MINALIGN, part_info.blksz);
	if (!dt_hdr) {
		printf("%s: out of memory for dt header!\n", __func__);
		return -ENOMEM;
	}

	ret = blk_dread(dev_desc, blk_offset, 1, dt_hdr);
	if (ret != 1) {
		printf("%s: failed to read dt table header\n",
		       __func__);
		goto out1;
	}

	if (!android_dt_check_header((ulong)dt_hdr)) {
		printf("%s: Error: invalid dt table header: 0x%x\n",
		       __func__, dt_hdr->magic);
		ret = -EINVAL;
		goto out1;
	}

#ifdef DEBUG
	android_dt_print_contents((ulong)dt_hdr);
#endif

	blk_cnt = DIV_ROUND_UP(fdt32_to_cpu(dt_hdr->total_size),
			       part_info.blksz);
	/* Read all DT Image */
	buf = memalign(ARCH_DMA_MINALIGN, part_info.blksz * blk_cnt);
	if (!buf) {
		printf("%s: out of memory for %s part!\n", __func__, part_name);
		ret = -ENOMEM;
		goto out1;
	}

	ret = blk_dread(dev_desc, blk_offset, blk_cnt, buf);
	if (ret != blk_cnt) {
		printf("%s: failed to read dtbo, blk_cnt=%d, ret=%d\n",
		       __func__, blk_cnt, ret);
		goto out2;
	}

	e_idx = board_select_fdt_index((ulong)buf);
	if (e_idx < 0) {
		printf("%s: failed to select board fdt index\n", __func__);
		ret = -EINVAL;
		goto out2;
	}

	ret = android_dt_get_fdt_by_index((ulong)buf, e_idx, &e_addr, &e_size);
	if (!ret) {
		printf("%s: failed to get fdt, index=%d\n", __func__, e_idx);
		ret = -EINVAL;
		goto out2;
	}

	if (fdt_dtbo)
		*fdt_dtbo = e_addr;
	if (index)
		*index = e_idx;

	free(dt_hdr);
	debug("ANDROID: Loading dt entry to 0x%lx size 0x%x idx %d from \"%s\" part\n",
	      e_addr, e_size, e_idx, part_name);

	return 0;

out2:
	free(buf);
out1:
	free(dt_hdr);

	return ret;
}

int android_fdt_overlay_apply(void *fdt_addr)
{
	struct andr_img_hdr *hdr;
	struct blk_desc *dev_desc;
	const char *part_name;
	disk_partition_t part_info;
	char buf[32] = {0};
	u32 blk_cnt;
	ulong fdt_dtbo = -1;
	int boot_mode;
	int index = -1;
	int ret;

	boot_mode = rockchip_get_boot_mode();
#ifdef CONFIG_ANDROID_AB
	if (boot_mode == BOOT_MODE_RECOVERY)
		boot_mode = BOOT_MODE_NORMAL;
#endif
	if (boot_mode == BOOT_MODE_RECOVERY)
		part_name = PART_RECOVERY;
	else
		part_name = PART_BOOT;

	/* Get partition info */
	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: dev_desc is NULL!\n", __func__);
		return -ENODEV;
	}

	ret = part_get_info_by_name(dev_desc, part_name, &part_info);
	if (ret < 0) {
		printf("%s: failed to get %s part info, ret=%d\n",
		       __func__, part_name, ret);
		return ret;
	}

	blk_cnt = DIV_ROUND_UP(sizeof(*hdr), part_info.blksz);
	hdr = memalign(ARCH_DMA_MINALIGN, part_info.blksz * blk_cnt);
	if (!hdr) {
		printf("%s: out of memory!\n", __func__);
		return -ENOMEM;
	}

	ret = blk_dread(dev_desc, part_info.start, blk_cnt, hdr);
	if (ret != blk_cnt) {
		printf("%s: failed to read %s hdr!\n", __func__, part_name);
		goto out;
	}

#ifdef DEBUG
	android_print_contents(hdr);
#endif

	if (android_image_check_header(hdr))
		return -EINVAL;

	/* Check header version */
	if (!hdr->header_version) {
		printf("Android header version 0\n");
		ret = -EINVAL;
		goto out;
	}

	ret = android_get_dtbo(&fdt_dtbo, (void *)hdr, &index, boot_mode);
	if (!ret) {
		phys_size_t fdt_size;
		/* Must incease size before overlay */
		fdt_size = fdt_totalsize((void *)fdt_addr) +
				fdt_totalsize((void *)fdt_dtbo);
		if (sysmem_free((phys_addr_t)fdt_addr))
			goto out;

		if (!sysmem_alloc_base(MEM_FDT_DTBO,
				       (phys_addr_t)fdt_addr,
					fdt_size + CONFIG_SYS_FDT_PAD))
			goto out;
		fdt_increase_size(fdt_addr, fdt_totalsize((void *)fdt_dtbo));
		ret = fdt_overlay_apply(fdt_addr, (void *)fdt_dtbo);
		if (!ret) {
			snprintf(buf, 32, "%s%d", "androidboot.dtbo_idx=", index);
			env_update("bootargs", buf);
			printf("ANDROID: fdt overlay OK\n");
		} else {
			printf("ANDROID: fdt overlay failed, ret=%d\n", ret);
		}
	}

out:
	free(hdr);

	return 0;
}
#endif

int android_image_load_by_partname(struct blk_desc *dev_desc,
				   const char *boot_partname,
				   unsigned long *load_address)
{
	disk_partition_t boot_part;
	int ret, part_num;

	part_num = part_get_info_by_name(dev_desc, boot_partname, &boot_part);
	if (part_num < 0) {
		printf("%s: Can't find part: %s\n", __func__, boot_partname);
		return -1;
	}
	printf("ANDROID: Loading kernel from \"%s\", partition %d.\n",
	      boot_part.name, part_num);

	ret = android_image_load(dev_desc, &boot_part, *load_address, -1UL);
	if (ret < 0) {
		printf("%s: %s part load fail, ret=%d\n",
		      __func__, boot_part.name, ret);
		return ret;
	}
	*load_address = ret;

	return 0;
}

int android_bootloader_boot_flow(struct blk_desc *dev_desc,
				 unsigned long load_address)
{
	enum android_boot_mode mode = ANDROID_BOOT_MODE_NORMAL;
	disk_partition_t misc_part_info;
	int part_num;
	char *command_line;
	char slot_suffix[3] = {0};
	const char *mode_cmdline = NULL;
	char *boot_partname = ANDROID_PARTITION_BOOT;
	int bte_recovery_flag = 0;

	/*
	 * 1. Load MISC partition and determine the boot mode
	 *   clear its value for the next boot if needed.
	 */
	part_num = part_get_info_by_name(dev_desc, ANDROID_PARTITION_MISC,
					 &misc_part_info);
	if (part_num < 0) {
		printf("Could not find misc partition\n");
	} else {
#ifdef CONFIG_ANDROID_KEYMASTER_CA
		/* load attestation key from misc partition. */
		load_attestation_key(dev_desc, &misc_part_info);
#endif

		mode = android_bootloader_load_and_clear_mode(dev_desc,
							      &misc_part_info);
#ifdef CONFIG_RKIMG_BOOTLOADER
		if (mode == ANDROID_BOOT_MODE_NORMAL) {
			if (rockchip_get_boot_mode() == BOOT_MODE_RECOVERY)
				mode = ANDROID_BOOT_MODE_RECOVERY;
		}
#endif
	}

	if( rockchip_get_boot_mode() == BOOT_BTE_RECOVERY ) {
		mode = ANDROID_BOOT_MODE_BTE_RECOVERY;
	} // if

	printf("ANDROID: reboot reason: \"%s\"\n", android_boot_mode_str(mode));

	/* Get current slot_suffix */
	if (decrease_tries_if_android_ab(slot_suffix))
		return -1;

	switch (mode) {
	case ANDROID_BOOT_MODE_NORMAL:
		/* In normal mode, we load the kernel from "boot" but append
		 * "skip_initramfs" to the cmdline to make it ignore the
		 * recovery initramfs in the boot partition.
		 */
#ifdef CONFIG_ANDROID_AB
		/*  In A/B, the recovery image is built as boot.img, containing the
		* recovery's ramdisk. Previously, bootloader used the skip_initramfs
		* kernel command line parameter to decide which mode to boot into.
		* For Android >=10 and with dynamic partition support, the bootloader
		* MUST NOT pass skip_initramfs to the kernel command-line.
		* Instead, bootloader should pass androidboot.force_normal_boot=1
		* and then Android's first-stage init in ramdisk
		* will skip recovery and boot normal Android.
		*/
		if (is_support_dynamic_partition(dev_desc)) {
			mode_cmdline = "androidboot.force_normal_boot=1";
		} else {
			mode_cmdline = "skip_initramfs";
		}
#endif
		break;
	case ANDROID_BOOT_MODE_RECOVERY:
		/* In recovery mode we still boot the kernel from "boot" but
		 * don't skip the initramfs so it boots to recovery.
		 */
#ifndef CONFIG_ANDROID_AB
		boot_partname = ANDROID_PARTITION_RECOVERY;
#endif
		break;
	case ANDROID_BOOT_MODE_BOOTLOADER:
		/* Bootloader mode enters fastboot. If this operation fails we
		 * simply return since we can't recover from this situation by
		 * switching to another slot.
		 */
		return android_bootloader_boot_bootloader();
	case ANDROID_BOOT_MODE_BTE_RECOVERY:
		boot_partname = BTE_RECOVERY_PARTITION;
		bte_recovery_flag = 1;
		break;
	}

#ifdef CONFIG_ANDROID_AVB
	uint8_t vboot_flag = 0;
	disk_partition_t vbmeta_part_info;

	if (trusty_read_vbootkey_enable_flag(&vboot_flag)) {
		printf("Can't read vboot flag\n");
		return -1;
	}

	if (vboot_flag) {
		printf("Vboot=1, SecureBoot enabled, AVB verify\n");
		if (android_slot_verify(boot_partname, &load_address,
					slot_suffix)) {
			printf("AVB verify failed\n");
			reset_cpu_if_android_ab();

			return -1;
		}
	} else {
		part_num = part_get_info_by_name(dev_desc,
						 ANDROID_PARTITION_VBMETA,
						 &vbmeta_part_info);
		if (part_num < 0) {
			printf("Not AVB images, AVB skip\n");
			env_update("bootargs",
				   "androidboot.verifiedbootstate=orange");
			if (android_image_load_by_partname(dev_desc,
							   boot_partname,
							   &load_address)) {
				printf("Android image load failed\n");
				return -1;
			}
			else {
				if( bte_recovery_flag == 1 ) 
					printf("find recovery-kernel partition\n");
			} // else
		} else {
			printf("Vboot=0, AVB images, AVB verify\n");
			if (android_slot_verify(boot_partname, &load_address,
						slot_suffix)) {
				printf("AVB verify failed\n");
				reset_cpu_if_android_ab();

				return -1;
			}
		}
	}
#else
	/*
	 * 2. Load the boot/recovery from the desired "boot" partition.
	 * Determine if this is an AOSP image.
	 */
	if (android_image_load_by_partname(dev_desc,
					   boot_partname,
					   &load_address)) {
		printf("Android image load failed\n");
		return -1;
	}
#endif
	update_root_uuid_if_android_ab();

	/* Set Android root variables. */
	env_set_ulong("android_root_devnum", dev_desc->devnum);
	env_set("android_slotsufix", slot_suffix);

#ifdef CONFIG_FASTBOOT_OEM_UNLOCK
	/* read oem unlock status and attach to bootargs */
	uint8_t unlock = 0;
	TEEC_Result result;
	char oem_unlock[OEM_UNLOCK_ARG_SIZE] = {0};
	result = trusty_read_oem_unlock(&unlock);
	if (result) {
		printf("read oem unlock status with error : 0x%x\n", result);
	} else {
		snprintf(oem_unlock, OEM_UNLOCK_ARG_SIZE, "androidboot.oem_unlocked=%d", unlock);
		env_update("bootargs", oem_unlock);
	}
#endif

	/* Assemble the command line */
	command_line = android_assemble_cmdline(slot_suffix, mode_cmdline);
	env_update("bootargs", command_line);

	debug("ANDROID: bootargs: \"%s\"\n", command_line);

#ifdef CONFIG_SUPPORT_OEM_DTB
	if (android_bootloader_get_fdt(ANDROID_PARTITION_OEM,
				       ANDROID_ARG_FDT_FILENAME)) {
		printf("Can not get the fdt data from oem!\n");
	}
#endif
#ifdef CONFIG_OPTEE_CLIENT
	if (trusty_notify_optee_uboot_end())
		printf("Close optee client failed!\n");
#endif
	android_bootloader_boot_kernel(load_address);

	/* TODO: If the kernel doesn't boot mark the selected slot as bad. */
	return -1;
}

int android_avb_boot_flow(unsigned long kernel_address)
{
	struct blk_desc *dev_desc;
	disk_partition_t boot_part_info;
	int ret;

	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: dev_desc is NULL!\n", __func__);
		return -1;
	}

	/* Load the kernel from the desired "boot" partition. */
	ret = part_get_info_by_name(dev_desc, ANDROID_PARTITION_BOOT,
				    &boot_part_info);
	if (ret < 0) {
		printf("%s: failed to get boot part\n", __func__);
		return ret;
	}

	ret = android_image_load(dev_desc, &boot_part_info,
				 kernel_address, -1UL);
	if (ret < 0) {
		printf("Android avb boot failed, error %d.\n", ret);
		return ret;
	}

	android_bootloader_boot_kernel(kernel_address);

	/* TODO: If the kernel doesn't boot mark the selected slot as bad. */
	return -1;
}

int android_boot_flow(unsigned long kernel_address)
{
	struct blk_desc *dev_desc;
	disk_partition_t boot_part_info;
	int ret;

	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: dev_desc is NULL!\n", __func__);
		return -1;
	}
	/* Load the kernel from the desired "boot" partition. */
	ret = part_get_info_by_name(dev_desc, ANDROID_PARTITION_BOOT,
				    &boot_part_info);
	if (ret < 0) {
		printf("%s: failed to get boot part\n", __func__);
		return ret;
	}

	ret = android_image_load(dev_desc, &boot_part_info, kernel_address,
				 -1UL);
	if (ret < 0)
		return ret;

	android_bootloader_boot_kernel(kernel_address);

	/* TODO: If the kernel doesn't boot mark the selected slot as bad. */
	return -1;
}

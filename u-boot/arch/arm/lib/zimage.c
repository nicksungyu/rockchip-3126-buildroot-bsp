/*
 * Copyright (C) 2016
 * Ladislav Michl <ladis@linux-mips.org>
 *
 * bootz code:
 * Copyright (C) 2012 Marek Vasut <marek.vasut@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>

#define	LINUX_ARM_ZIMAGE_MAGIC	0x016f2818

struct arm_z_header {
	uint32_t	code[9];
	uint32_t	zi_magic;
	uint32_t	zi_start;
	uint32_t	zi_end;
} __attribute__ ((__packed__));

int bootz_setup(ulong image, ulong *start, ulong *end)
{
	struct arm_z_header *zi = (struct arm_z_header *)image;

	if (zi->zi_magic != LINUX_ARM_ZIMAGE_MAGIC) {
#ifndef CONFIG_SPL_FRAMEWORK
		puts("Bad Linux ARM zImage magic!\n");
#endif
		return 1;
	}

	*start = zi->zi_start;
	*end = zi->zi_end;
#ifndef CONFIG_SPL_FRAMEWORK
	printf("Kernel image (BTE) @ %#08lx [ %#08lx - %#08lx ]\n",
	       image, *start, *end);
#endif

	return 0;
}

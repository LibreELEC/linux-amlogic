/*
 * arch/arm/mach-mesong9tv/include/mach/memory.h
 *
 * Copyright (C) 2014 Amlogic, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MACH_MESONG9TV_MEMORY_H
#define __MACH_MESONG9TV_MEMORY_H

/*
 * Physical DRAM offset.
 */
#define PHYS_OFFSET		UL(0x00200000)

#define BOOT_PARAMS_OFFSET	(PHYS_OFFSET + 0x100)

#endif //__MACH_MESONG9TV_MEMORY_H
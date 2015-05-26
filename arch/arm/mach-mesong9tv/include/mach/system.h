/*
 * arch/arm/mach-mesong9tv/include/mach/system.h
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

#ifndef __MACH_MESONG9TV_SYSTEM_H
#define __MACH_MESONG9TV_SYSTEM_H

#include <linux/io.h>
#include <plat/io.h>
#include <mach/io.h>
#include <mach/register.h>

static inline void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks
	 */
	cpu_do_idle();
}

#define WATCHDOG_ENABLE_BIT		(1<<19)
#define DUAL_CORE_RESET			(3<<24)

static inline void arch_reset(char mode, const char *cmd)
{
	WRITE_MPEG_REG(VENC_VDAC_SETTING, 0xf);
	WRITE_MPEG_REG(WATCHDOG_RESET, 0);
	WRITE_MPEG_REG(WATCHDOG_TC, DUAL_CORE_RESET| WATCHDOG_ENABLE_BIT | 100);
	while(1)
		arch_idle();
}

#endif //__MACH_MESONG9TV_SYSTEM_H

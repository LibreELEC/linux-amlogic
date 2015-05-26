/*
 * arch/arm/mach-mesong9tv/include/mach/watchdog.h
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

#ifndef __MACH_MESONG9TV_WATCHDOG_H
#define __MACH_MESONG9TV_WATCHDOG_H

#include <linux/io.h>
#include <plat/io.h>
#include <mach/hardware.h>
#include <mach/register.h>

#define WATCHDOG_ENABLE_BIT		19
#define WATCHDOG_COUNT_BIT		15
#define WATCHDOG_COUNT_MASK		((1<<(WATCHDOG_COUNT_BIT+1))-1)
#define WDT_ONE_SECOND 7812

#define MAX_TIMEOUT (WATCHDOG_COUNT_MASK/WDT_ONE_SECOND)
#define MIN_TIMEOUT			1

#endif

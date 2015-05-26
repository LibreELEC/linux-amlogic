/*
 * arch/arm/mach-mesong9tv/include/mach/timex.h
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

#ifndef __MACH_MESONG9TV_TIMEX_H
#define __MACH_MESONG9TV_TIMEX_H

#ifndef CONFIG_MESON_CLOCK_TICK_RATE
#define CLOCK_TICK_RATE		(24000000)
#else
#define CLOCK_TICK_RATE		(CONFIG_MESON_CLOCK_TICK_RATE)
#endif

extern struct sys_timer meson_sys_timer;

#endif //__MACH_MESONG9TV_TIMEX_H

/*
 * arch/arm/plat-meson/plat-hibernate.c
 *
 * Copyright (C) 2015 Amlogic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/kernel.h>
#include <mach/am_regs.h>

typedef struct {
	const unsigned int reg_addr;
	unsigned int reg_val;
} plat_reg_rcd_t;

#define PLATE_REG_RECORD(_REG) \
static plat_reg_rcd_t PLAT_RCD_##_REG = { \
	.reg_addr = P_##_REG, \
	.reg_val = 0, \
}

#define SAVE_PLAT_REG(_REG) \
do { \
PLAT_RCD_##_REG.reg_val = aml_read_reg32(P_##_REG); \
} while(0)


#define RESTORE_PLAT_REG(_REG) \
do { \
aml_write_reg32(PLAT_RCD_##_REG.reg_addr, PLAT_RCD_##_REG.reg_val); \
} while(0);


PLATE_REG_RECORD(ISA_TIMERD);
PLATE_REG_RECORD(ISA_TIMER_MUX);

void platform_reg_save(void)
{
	SAVE_PLAT_REG(ISA_TIMERD);
	SAVE_PLAT_REG(ISA_TIMER_MUX);
}

void platform_reg_restore(void)
{
	RESTORE_PLAT_REG(ISA_TIMERD);
	RESTORE_PLAT_REG(ISA_TIMER_MUX);
}
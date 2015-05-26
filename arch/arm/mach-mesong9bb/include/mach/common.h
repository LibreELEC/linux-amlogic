/*
 * common code header to arch and all modules.
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


#ifndef _MACH_MESONG9TV_PM_H
#define _MACH_MESONG9TV_PM_H

/*
 * These led and vghl pwm registers are no longer used in g9tv.
 * They are defined here to avoid some code compiling error.
 * For example meson_cs_dcdc_regulator.c
 */

#define LED_PWM_REG0			0x21da
#define P_LED_PWM_REG0			CBUS_REG_ADDR(LED_PWM_REG0)
#define LED_PWM_REG1			0x21db
#define P_LED_PWM_REG1			CBUS_REG_ADDR(LED_PWM_REG1)
#define LED_PWM_REG2			0x21dc
#define P_LED_PWM_REG2			CBUS_REG_ADDR(LED_PWM_REG2)
#define LED_PWM_REG3			0x21dd
#define P_LED_PWM_REG3			CBUS_REG_ADDR(LED_PWM_REG3)
#define LED_PWM_REG4			0x21de
#define P_LED_PWM_REG4			CBUS_REG_ADDR(LED_PWM_REG4)
#define LED_PWM_REG5			0x21df
#define P_LED_PWM_REG5			CBUS_REG_ADDR(LED_PWM_REG5)
#define LED_PWM_REG6			0x21e0
#define P_LED_PWM_REG6			CBUS_REG_ADDR(LED_PWM_REG6)

#define VGHL_PWM_REG0			0x21e1
#define P_VGHL_PWM_REG0 		CBUS_REG_ADDR(VGHL_PWM_REG0)
#define VGHL_PWM_REG1			0x21e2
#define P_VGHL_PWM_REG1 		CBUS_REG_ADDR(VGHL_PWM_REG1)
#define VGHL_PWM_REG2			0x21e3
#define P_VGHL_PWM_REG2 		CBUS_REG_ADDR(VGHL_PWM_REG2)
#define VGHL_PWM_REG3			0x21e4
#define P_VGHL_PWM_REG3 		CBUS_REG_ADDR(VGHL_PWM_REG3)
#define VGHL_PWM_REG4			0x21e5
#define P_VGHL_PWM_REG4 		CBUS_REG_ADDR(VGHL_PWM_REG4)
#define VGHL_PWM_REG5			0x21e6
#define P_VGHL_PWM_REG5 		CBUS_REG_ADDR(VGHL_PWM_REG5)
#define VGHL_PWM_REG6			0x21e7
#define P_VGHL_PWM_REG6			CBUS_REG_ADDR(VGHL_PWM_REG6)


#endif


/*
 * arch/arm/mach-mesong9bb/gpio.c
 * Amlogic GPIO Driver
 *
 * Copyright (C) 2015 Amlogic, Inc.
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


#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/amlogic/gpio-amlogic.h>

#include <mach/am_regs.h>
#include <plat/io.h>

extern int meson_pin_to_pullup(unsigned int pin ,unsigned int *reg,unsigned int *bit,unsigned int *bit_en);
extern struct amlogic_set_pullup pullup_ops;
extern unsigned p_pull_up_addr[];
extern unsigned p_pull_upen_addr[];
extern unsigned int p_pin_mux_reg_addr[];
extern int gpio_irq;
extern int gpio_flag;


//#define debug
#ifdef debug
	#define gpio_print(...) printk(__VA_ARGS__)
#else
	#define gpio_print(...)
#endif


unsigned p_gpio_oen_addr[] = {
	P_PREG_PAD_GPIO0_EN_N,
	P_PREG_PAD_GPIO1_EN_N,
	P_PREG_PAD_GPIO2_EN_N,
	P_PREG_PAD_GPIO3_EN_N,
	P_PREG_PAD_GPIO4_EN_N,
	P_PREG_PAD_GPIO5_EN_N,
	P_AO_GPIO_O_EN_N,
};

static unsigned p_gpio_output_addr[] = {
	P_PREG_PAD_GPIO0_O,
	P_PREG_PAD_GPIO1_O,
	P_PREG_PAD_GPIO2_O,
	P_PREG_PAD_GPIO3_O,
	P_PREG_PAD_GPIO4_O,
	P_PREG_PAD_GPIO5_O,
	P_AO_GPIO_O_EN_N,
};

static unsigned p_gpio_input_addr[] = {
	P_PREG_PAD_GPIO0_I,
	P_PREG_PAD_GPIO1_I,
	P_PREG_PAD_GPIO2_I,
	P_PREG_PAD_GPIO3_I,
	P_PREG_PAD_GPIO4_I,
	P_PREG_PAD_GPIO5_I,
	P_AO_GPIO_I,
};


#define PMUX(reg, bit)		((reg << 5) | bit)
#define PMUX_NONE		(0xFFFF)
#define PMUX_SIZE		9

static unsigned int meson_pinmux_table[][PMUX_SIZE] = {

	[GPIOX_0]	=	{PMUX(5,14),	PMUX(8,5),	PMUX(0,1),	PMUX(0,6),	PMUX(6,17),	PMUX(7,0),	PMUX(8,27),	PMUX(3,17),	PMUX(9,18)},
	[GPIOX_1]	=	{PMUX(5,13),	PMUX(8,4),	PMUX(0,1),	PMUX(0,6),	PMUX(6,16),	PMUX(7,1),	PMUX_NONE,	PMUX(3,16),	PMUX(9,17)},
	[GPIOX_2]	=	{PMUX(5,13),	PMUX(8,3),	PMUX(0,0),	PMUX(0,6),	PMUX_NONE,	PMUX(7,2),	PMUX_NONE,	PMUX(3,15),	PMUX_NONE},
	[GPIOX_3]	=	{PMUX(5,13),	PMUX(8,2),	PMUX(0,0),	PMUX(0,6),	PMUX_NONE,	PMUX(7,3),	PMUX_NONE,	PMUX(3,14),	PMUX_NONE},
	[GPIOX_4]	=	{PMUX(5,12),	PMUX_NONE,	PMUX(0,0),	PMUX(0,6),	PMUX(3,30),	PMUX(7,4),	PMUX(4,17),	PMUX(3,13),	PMUX_NONE},
	[GPIOX_5]	=	{PMUX(5,12),	PMUX_NONE,	PMUX(0,0),	PMUX(0,6),	PMUX(3,29),	PMUX(7,5),	PMUX(4,16),	PMUX(3,12),	PMUX_NONE},
	[GPIOX_6]	=	{PMUX(5,12),	PMUX_NONE,	PMUX(0,0),	PMUX(0,6),	PMUX(3,28),	PMUX(7,6),	PMUX(4,15),	PMUX(3,12),	PMUX_NONE},
	[GPIOX_7]	=	{PMUX(5,12),	PMUX_NONE,	PMUX(0,0),	PMUX(0,6),	PMUX(3,27),	PMUX(7,7),	PMUX(4,14),	PMUX(3,12),	PMUX_NONE},
	[GPIOX_8]	=	{PMUX(5,11),	PMUX(8,1),	PMUX(0,3),	PMUX(0,6),	PMUX_NONE,	PMUX(7,8),	PMUX(8,26),	PMUX(3,12),	PMUX_NONE},
	[GPIOX_9]	=	{PMUX(5,10),	PMUX(8,0),	PMUX(0,3),	PMUX(0,6),	PMUX_NONE,	PMUX(7,9),	PMUX(9,14),	PMUX(3,12),	PMUX_NONE},
	[GPIOX_10]	=	{PMUX_NONE,	PMUX(3,22),	PMUX(0,2),	PMUX(0,6),	PMUX_NONE,	PMUX(7,10),	PMUX(7,31),	PMUX(3,12),	PMUX(9,19)},
	[GPIOX_11]	=	{PMUX_NONE,	PMUX(3,18),	PMUX(0,2),	PMUX(0,6),	PMUX_NONE,	PMUX(7,11),	PMUX(2,3),	PMUX(3,12),	PMUX_NONE},
	[GPIOX_12]	=	{PMUX_NONE,	PMUX_NONE,	PMUX(0,2),	PMUX(0,6),	PMUX(3,7),	PMUX(7,12),	PMUX_NONE,	PMUX_NONE,	PMUX(4,13)},
	[GPIOX_13]	=	{PMUX_NONE,	PMUX_NONE,	PMUX(0,2),	PMUX(0,6),	PMUX(3,11),	PMUX(7,13),	PMUX_NONE,	PMUX_NONE,	PMUX(4,12)},
	[GPIOX_14]	=	{PMUX_NONE,	PMUX_NONE,	PMUX(0,2),	PMUX(0,6),	PMUX(3,11),	PMUX(7,14),	PMUX_NONE,	PMUX_NONE,	PMUX(4,11)},
	[GPIOX_15]	=	{PMUX_NONE,	PMUX_NONE,	PMUX(0,2),	PMUX(0,6),	PMUX(3,11),	PMUX(7,15),	PMUX_NONE,	PMUX_NONE,	PMUX(4,10)},
	[GPIOX_16]	=	{PMUX_NONE,	PMUX(9,20),	PMUX(0,5),	PMUX(0,6),	PMUX(3,11),	PMUX(7,16),	PMUX(8,25),	PMUX_NONE,	PMUX_NONE},
	[GPIOX_17]	=	{PMUX_NONE,	PMUX(9,20),	PMUX(0,5),	PMUX(0,6),	PMUX(3,11),	PMUX(7,17),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOX_18]	=	{PMUX_NONE,	PMUX(9,20),	PMUX(0,4),	PMUX(0,6),	PMUX(3,11),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOX_19]	=	{PMUX_NONE,	PMUX(9,20),	PMUX(0,4),	PMUX(0,6),	PMUX(3,11),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOX_20]	=	{PMUX_NONE,	PMUX(9,20),	PMUX(0,4),	PMUX(0,6),	PMUX(3,9),	PMUX_NONE,	PMUX(1,17),	PMUX(1,0),	PMUX_NONE},
	[GPIOX_21]	=	{PMUX_NONE,	PMUX(9,20),	PMUX(0,4),	PMUX(0,6),	PMUX(3,8),	PMUX(1,3),	PMUX(1,16),	PMUX(1,4),	PMUX_NONE},
	[GPIOX_22]	=	{PMUX_NONE,	PMUX(9,20),	PMUX(0,4),	PMUX(0,6),	PMUX(3,10),	PMUX(1,1),	PMUX_NONE,	PMUX(1,2),	PMUX_NONE},
	[GPIOX_23]	=	{PMUX_NONE,	PMUX(9,20),	PMUX(0,4),	PMUX(0,6),	PMUX(3,6),	PMUX_NONE,	PMUX_NONE,	PMUX(1,6),	PMUX_NONE},
	[GPIOX_24]	=	{PMUX_NONE,	PMUX_NONE,	PMUX(0,19),	PMUX(0,9),	PMUX_NONE,	PMUX(1,7),	PMUX(8,24),	PMUX(1,9),	PMUX(4,21)},
	[GPIOX_25]	=	{PMUX_NONE,	PMUX_NONE,	PMUX(0,18),	PMUX(0,8),	PMUX_NONE,	PMUX(1,8),	PMUX(8,23),	PMUX(1,5),	PMUX(4,20)},
	[GPIOX_26]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(0,7),	PMUX(7,30),	PMUX(3,25),	PMUX(8,22),	PMUX_NONE,	PMUX(4,19)},
	[GPIOX_27]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(0,10),	PMUX_NONE,	PMUX(8,19),	PMUX(8,28),	PMUX_NONE,	PMUX(4,18)},

	[BOOT_0]	=	{PMUX_NONE,	PMUX(4,30),	PMUX_NONE,	PMUX(6,29),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_1]	=	{PMUX_NONE,	PMUX(4,29),	PMUX_NONE,	PMUX(6,28),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_2]	=	{PMUX_NONE,	PMUX(4,29),	PMUX_NONE,	PMUX(6,27),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_3]	=	{PMUX_NONE,	PMUX(4,29),	PMUX_NONE,	PMUX(6,26),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_4]	=	{PMUX_NONE,	PMUX(4,28),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_5]	=	{PMUX_NONE,	PMUX(4,28),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_6]	=	{PMUX_NONE,	PMUX(4,28),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_7]	=	{PMUX_NONE,	PMUX(4,28),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_8]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_9]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_10]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_11]	=	{PMUX_NONE,	PMUX_NONE,	PMUX(5,1),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_12]	=	{PMUX_NONE,	PMUX_NONE,	PMUX(5,3),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_13]	=	{PMUX_NONE,	PMUX_NONE,	PMUX(5,2),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_14]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_15]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_16]	=	{PMUX_NONE,	PMUX(4,27),	PMUX_NONE,	PMUX(6,25),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_17]	=	{PMUX_NONE,	PMUX(4,26),	PMUX_NONE,	PMUX(6,24),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[BOOT_18]	=	{PMUX_NONE,	PMUX_NONE,	PMUX(5,0),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},

	[GPIOH_0]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(7,20),	PMUX_NONE},
	[GPIOH_1]	=	{PMUX_NONE,	PMUX(4,9),	PMUX_NONE,	PMUX(3,26),	PMUX(7,29),	PMUX_NONE,	PMUX_NONE,	PMUX(7,19),	PMUX_NONE},
	[GPIOH_2]	=	{PMUX_NONE,	PMUX(4,8),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(7,28),	PMUX_NONE},
	[GPIOH_3]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(1,25),	PMUX_NONE},
	[GPIOH_4]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(1,24),	PMUX_NONE},
	[GPIOH_5]	=	{PMUX_NONE,	PMUX(4,7),	PMUX_NONE,	PMUX(4,3),	PMUX_NONE,	PMUX(1,19),	PMUX_NONE,	PMUX(1,26),	PMUX_NONE},
	[GPIOH_6]	=	{PMUX_NONE,	PMUX(4,6),	PMUX_NONE,	PMUX(4,2),	PMUX_NONE,	PMUX(1,18),	PMUX_NONE,	PMUX(1,23),	PMUX_NONE},
	[GPIOH_7]	=	{PMUX_NONE,	PMUX(6,23),	PMUX_NONE,	PMUX(5,8),	PMUX_NONE,	PMUX(3,24),	PMUX_NONE,	PMUX(7,24),	PMUX_NONE},
	[GPIOH_8]	=	{PMUX_NONE,	PMUX(6,22),	PMUX_NONE,	PMUX(5,9),	PMUX_NONE,	PMUX(9,15),	PMUX_NONE,	PMUX(7,23),	PMUX_NONE},
	[GPIOH_9]	=	{PMUX_NONE,	PMUX(9,12),	PMUX_NONE,	PMUX(9,13),	PMUX_NONE,	PMUX(9,16),	PMUX_NONE,	PMUX(7,22),	PMUX_NONE},
	[GPIOH_10]	=	{PMUX_NONE,	PMUX(9,10),	PMUX_NONE,	PMUX(9,11),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},

	[GPIOZ_0]	=	{PMUX_NONE,	PMUX(2,23),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_1]	=	{PMUX_NONE,	PMUX(2,22),	PMUX_NONE,	PMUX(3,19),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_2]	=	{PMUX_NONE,	PMUX(2,21),	PMUX_NONE,	PMUX(2,20),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_3]	=	{PMUX_NONE,	PMUX(2,19),	PMUX_NONE,	PMUX(2,18),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_4]	=	{PMUX_NONE,	PMUX(2,17),	PMUX_NONE,	PMUX_NONE,	PMUX(11,16),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_5]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(2,16),	PMUX(11,15),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_6]	=	{PMUX_NONE,	PMUX(6,14),	PMUX_NONE,	PMUX(3,21),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_7]	=	{PMUX_NONE,	PMUX(6,13),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_8]	=	{PMUX_NONE,	PMUX(6,12),	PMUX_NONE,	PMUX_NONE,	PMUX(11,14),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_9]	=	{PMUX_NONE,	PMUX(6,11),	PMUX_NONE,	PMUX_NONE,	PMUX(11,13),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_10]	=	{PMUX_NONE,	PMUX(6,10),	PMUX_NONE,	PMUX_NONE,	PMUX(11,12),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_11]	=	{PMUX_NONE,	PMUX(6,9),	PMUX_NONE,	PMUX(1,28),	PMUX(11,11),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_12]	=	{PMUX_NONE,	PMUX(6,8),	PMUX_NONE,	PMUX(1,27),	PMUX(11,10),	PMUX_NONE,	PMUX(2,2),	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_13]	=	{PMUX_NONE,	PMUX(6,7),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_14]	=	{PMUX_NONE,	PMUX(6,6),	PMUX_NONE,	PMUX(3,20),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_15]	=	{PMUX_NONE,	PMUX(6,5),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_16]	=	{PMUX_NONE,	PMUX(6,4),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_17]	=	{PMUX_NONE,	PMUX(6,3),	PMUX_NONE,	PMUX(2,27),	PMUX_NONE,	PMUX(7,28),	PMUX(2,1),	PMUX(5,31),	PMUX_NONE},
	[GPIOZ_18]	=	{PMUX_NONE,	PMUX(6,2),	PMUX_NONE,	PMUX(2,26),	PMUX_NONE,	PMUX(7,27),	PMUX(2,0),	PMUX(5,30),	PMUX_NONE},
	[GPIOZ_19]	=	{PMUX_NONE,	PMUX(6,1),	PMUX_NONE,	PMUX(2,25),	PMUX(11,9),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOZ_20]	=	{PMUX_NONE,	PMUX(6,0),	PMUX_NONE,	PMUX(2,24),	PMUX(11,8),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},

	[GPIOW_0]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(7,26),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(6,31),	PMUX_NONE},
	[GPIOW_1]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(7,25),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(6,30),	PMUX_NONE},
	[GPIOW_2]	=	{PMUX_NONE,	PMUX(8,16),	PMUX_NONE,	PMUX(8,15),	PMUX(11,18),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_3]	=	{PMUX_NONE,	PMUX(8,13),	PMUX_NONE,	PMUX(8,12),	PMUX(11,17),	PMUX(7,21),	PMUX(9,21),	PMUX_NONE,	PMUX_NONE},
	[GPIOW_4]	=	{PMUX_NONE,	PMUX(10,12),	PMUX_NONE,	PMUX(11,22),	PMUX(11,7),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_5]	=	{PMUX_NONE,	PMUX(10,11),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_6]	=	{PMUX_NONE,	PMUX(10,10),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_7]	=	{PMUX_NONE,	PMUX(10,9),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_8]	=	{PMUX_NONE,	PMUX(10,9),	PMUX_NONE,	PMUX(11,21),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_9]	=	{PMUX_NONE,	PMUX(10,8),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_10]	=	{PMUX_NONE,	PMUX(10,7),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(10,13),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_11]	=	{PMUX_NONE,	PMUX(10,6),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_12]	=	{PMUX_NONE,	PMUX(10,6),	PMUX_NONE,	PMUX(11,19),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_13]	=	{PMUX_NONE,	PMUX(10,5),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_14]	=	{PMUX_NONE,	PMUX(10,4),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_15]	=	{PMUX_NONE,	PMUX(10,3),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_16]	=	{PMUX_NONE,	PMUX(10,3),	PMUX_NONE,	PMUX(11,23),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOW_17]	=	{PMUX_NONE,	PMUX(10,2),	PMUX_NONE,	PMUX(6,21),	PMUX(11,3),	PMUX_NONE,	PMUX_NONE,	PMUX(10,25),	PMUX_NONE},
	[GPIOW_18]	=	{PMUX_NONE,	PMUX(10,1),	PMUX_NONE,	PMUX(6,20),	PMUX(11,2),	PMUX_NONE,	PMUX_NONE,	PMUX(10,24),	PMUX_NONE},
	[GPIOW_19]	=	{PMUX_NONE,	PMUX(10,0),	PMUX_NONE,	PMUX(6,19),	PMUX(11,1),	PMUX(10,29),	PMUX(10,28),	PMUX(10,23),	PMUX_NONE},
	[GPIOW_20]	=	{PMUX_NONE,	PMUX(10,0),	PMUX_NONE,	PMUX(6,18),	PMUX(11,0),	PMUX(10,27),	PMUX(10,26),	PMUX(10,22),	PMUX_NONE},

	[GPIOAO_0]	=	{PMUX_NONE,	PMUX(13,12),	PMUX_NONE,	PMUX(13,26),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_1]	=	{PMUX_NONE,	PMUX(13,11),	PMUX_NONE,	PMUX(13,25),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_2]	=	{PMUX_NONE,	PMUX(13,10),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_3]	=	{PMUX_NONE,	PMUX(13,9),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_4]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(13,24),	PMUX(13,2),	PMUX(13,6),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_5]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(13,23),	PMUX(13,1),	PMUX(13,5),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_6]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_7]	=	{PMUX_NONE,	PMUX(13,0),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_8]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(13,17),	PMUX(13,14),	PMUX(13,16),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_9]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(13,27),	PMUX(13,13),	PMUX(13,15),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_10]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_11]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(13,22),	PMUX(13,28),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_12]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(13,21),	PMUX(13,29),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOAO_13]	=	{PMUX_NONE,	PMUX(13,31),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},

	[CARD_0]	=	{PMUX_NONE,	PMUX(2,14),	PMUX_NONE,	PMUX(2,7),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[CARD_1]	=	{PMUX_NONE,	PMUX(2,15),	PMUX_NONE,	PMUX(2,6),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[CARD_2]	=	{PMUX_NONE,	PMUX(2,11),	PMUX_NONE,	PMUX(2,5),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[CARD_3]	=	{PMUX_NONE,	PMUX(2,10),	PMUX_NONE,	PMUX(2,4),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[CARD_4]	=	{PMUX_NONE,	PMUX(2,12),	PMUX_NONE,	PMUX(2,7),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(8,10),	PMUX_NONE},
	[CARD_5]	=	{PMUX_NONE,	PMUX(2,13),	PMUX_NONE,	PMUX(2,7),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(8,9),	PMUX_NONE},
	[CARD_6]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[CARD_7]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[CARD_8]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},

	[GPIOY_0]	=	{PMUX_NONE,	PMUX(3,3),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(1,15),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE},
	[GPIOY_1]	=	{PMUX_NONE,	PMUX(3,2),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(1,14),	PMUX_NONE,	PMUX_NONE,	PMUX(9,3)},
	[GPIOY_2]	=	{PMUX_NONE,	PMUX(3,1),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(1,13),	PMUX_NONE,	PMUX_NONE,	PMUX(9,2)},
	[GPIOY_3]	=	{PMUX_NONE,	PMUX(3,0),	PMUX_NONE,	PMUX(9,4),	PMUX(9,5),	PMUX(1,12),	PMUX_NONE,	PMUX_NONE,	PMUX(9,1)},
	[GPIOY_4]	=	{PMUX_NONE,	PMUX(3,4),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(5,29),	PMUX_NONE,	PMUX_NONE,	PMUX(9,0)},
	[GPIOY_5]	=	{PMUX_NONE,	PMUX(3,5),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(5,28),	PMUX_NONE,	PMUX_NONE,	PMUX(9,0)},
	[GPIOY_6]	=	{PMUX_NONE,	PMUX(3,5),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(5,27),	PMUX_NONE,	PMUX_NONE,	PMUX(9,0)},
	[GPIOY_7]	=	{PMUX_NONE,	PMUX(3,5),	PMUX_NONE,	PMUX(9,26),	PMUX(9,25),	PMUX(5,26),	PMUX(10,19),	PMUX(10,21),	PMUX(9,0)},
	[GPIOY_8]	=	{PMUX_NONE,	PMUX(3,5),	PMUX_NONE,	PMUX_NONE,	PMUX(9,24),	PMUX(5,25),	PMUX(10,18),	PMUX(10,20),	PMUX(9,0)},
	[GPIOY_9]	=	{PMUX_NONE,	PMUX(3,5),	PMUX_NONE,	PMUX(9,22),	PMUX(9,23),	PMUX(5,24),	PMUX_NONE,	PMUX_NONE,	PMUX(9,0)},
	[GPIOY_10]	=	{PMUX_NONE,	PMUX(3,5),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(10,17),	PMUX(9,9),	PMUX(9,0)},
	[GPIOY_11]	=	{PMUX_NONE,	PMUX(3,5),	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(10,16),	PMUX(9,8),	PMUX(9,0)},
	[GPIOY_12]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(9,31),	PMUX(9,30),	PMUX_NONE,	PMUX(10,15),	PMUX(9,7),	PMUX_NONE},
	[GPIOY_13]	=	{PMUX_NONE,	PMUX_NONE,	PMUX_NONE,	PMUX(9,29),	PMUX(9,28),	PMUX_NONE,	PMUX(10,14),	PMUX(9,6),	PMUX_NONE},
};


#define PIN_MAP(pin,reg,bit) \
{ \
	.num=pin, \
	.name=#pin, \
	.out_en_reg_bit=GPIO_REG_BIT(reg,bit), \
	.out_value_reg_bit=GPIO_REG_BIT(reg,bit), \
	.input_value_reg_bit=GPIO_REG_BIT(reg,bit), \
}

#define PIN_AOMAP(pin,en_reg,en_bit,out_reg,out_bit,in_reg,in_bit) \
{ \
	.num=pin, \
	.name=#pin, \
	.out_en_reg_bit=GPIO_REG_BIT(en_reg,en_bit), \
	.out_value_reg_bit=GPIO_REG_BIT(out_reg,out_bit), \
	.input_value_reg_bit=GPIO_REG_BIT(in_reg,in_bit), \
	.gpio_owner=NULL, \
}

struct amlogic_gpio_desc amlogic_pins[] = {

	PIN_MAP(GPIOX_0,	4,	0),
	PIN_MAP(GPIOX_1,	4,	1),
	PIN_MAP(GPIOX_2,	4,	2),
	PIN_MAP(GPIOX_3,	4,	3),
	PIN_MAP(GPIOX_4,	4,	4),
	PIN_MAP(GPIOX_5,	4,	5),
	PIN_MAP(GPIOX_6,	4,	6),
	PIN_MAP(GPIOX_7,	4,	7),
	PIN_MAP(GPIOX_8,	4,	8),
	PIN_MAP(GPIOX_9,	4,	9),
	PIN_MAP(GPIOX_10,	4,	10),
	PIN_MAP(GPIOX_11,	4,	11),
	PIN_MAP(GPIOX_12,	4,	12),
	PIN_MAP(GPIOX_13,	4,	13),
	PIN_MAP(GPIOX_14,	4,	14),
	PIN_MAP(GPIOX_15,	4,	15),
	PIN_MAP(GPIOX_16,	4,	16),
	PIN_MAP(GPIOX_17,	4,	17),
	PIN_MAP(GPIOX_18,	4,	18),
	PIN_MAP(GPIOX_19,	4,	19),
	PIN_MAP(GPIOX_20,	4,	20),
	PIN_MAP(GPIOX_21,	4,	21),
	PIN_MAP(GPIOX_22,	4,	22),
	PIN_MAP(GPIOX_23,	4,	23),
	PIN_MAP(GPIOX_24,	4,	24),
	PIN_MAP(GPIOX_25,	4,	25),
	PIN_MAP(GPIOX_26,	4,	26),
	PIN_MAP(GPIOX_27,	4,	27),

	PIN_MAP(BOOT_0,		2,	0),
	PIN_MAP(BOOT_1,		2,	1),
	PIN_MAP(BOOT_2,		2,	2),
	PIN_MAP(BOOT_3,		2,	3),
	PIN_MAP(BOOT_4,		2,	4),
	PIN_MAP(BOOT_5,		2,	5),
	PIN_MAP(BOOT_6,		2,	6),
	PIN_MAP(BOOT_7,		2,	7),
	PIN_MAP(BOOT_8,		2,	8),
	PIN_MAP(BOOT_9,		2,	9),
	PIN_MAP(BOOT_10,	2,	10),
	PIN_MAP(BOOT_11,	2,	11),
	PIN_MAP(BOOT_12,	2,	12),
	PIN_MAP(BOOT_13,	2,	13),
	PIN_MAP(BOOT_14,	2,	14),
	PIN_MAP(BOOT_15,	2,	15),
	PIN_MAP(BOOT_16,	2,	16),
	PIN_MAP(BOOT_17,	2,	17),
	PIN_MAP(BOOT_18,	2,	18),

	PIN_MAP(GPIOH_0,	1,	16),
	PIN_MAP(GPIOH_1,	1,	17),
	PIN_MAP(GPIOH_2,	1,	18),
	PIN_MAP(GPIOH_3,	1,	19),
	PIN_MAP(GPIOH_4,	1,	20),
	PIN_MAP(GPIOH_5,	1,	21),
	PIN_MAP(GPIOH_6,	1,	22),
	PIN_MAP(GPIOH_7,	1,	23),
	PIN_MAP(GPIOH_8,	1,	24),
	PIN_MAP(GPIOH_9,	1,	25),
	PIN_MAP(GPIOH_10,	1,	26),

	PIN_MAP(GPIOZ_0,	3,	0),
	PIN_MAP(GPIOZ_1,	3,	1),
	PIN_MAP(GPIOZ_2,	3,	2),
	PIN_MAP(GPIOZ_3,	3,	3),
	PIN_MAP(GPIOZ_4,	3,	4),
	PIN_MAP(GPIOZ_5,	3,	5),
	PIN_MAP(GPIOZ_6,	3,	6),
	PIN_MAP(GPIOZ_7,	3,	7),
	PIN_MAP(GPIOZ_8,	3,	8),
	PIN_MAP(GPIOZ_9,	3,	9),
	PIN_MAP(GPIOZ_10,	3,	10),
	PIN_MAP(GPIOZ_11,	3,	11),
	PIN_MAP(GPIOZ_12,	3,	12),
	PIN_MAP(GPIOZ_13,	3,	13),
	PIN_MAP(GPIOZ_14,	3,	14),
	PIN_MAP(GPIOZ_15,	3,	15),
	PIN_MAP(GPIOZ_16,	3,	16),
	PIN_MAP(GPIOZ_17,	3,	17),
	PIN_MAP(GPIOZ_18,	3,	18),
	PIN_MAP(GPIOZ_19,	3,	19),
	PIN_MAP(GPIOZ_20,	3,	20),

	PIN_MAP(GPIOW_0,	0,	0),
	PIN_MAP(GPIOW_1,	0,	1),
	PIN_MAP(GPIOW_2,	0,	2),
	PIN_MAP(GPIOW_3,	0,	3),
	PIN_MAP(GPIOW_4,	0,	4),
	PIN_MAP(GPIOW_5,	0,	5),
	PIN_MAP(GPIOW_6,	0,	6),
	PIN_MAP(GPIOW_7,	0,	7),
	PIN_MAP(GPIOW_8,	0,	8),
	PIN_MAP(GPIOW_9,	0,	9),
	PIN_MAP(GPIOW_10,	0,	10),
	PIN_MAP(GPIOW_11,	0,	11),
	PIN_MAP(GPIOW_12,	0,	12),
	PIN_MAP(GPIOW_13,	0,	13),
	PIN_MAP(GPIOW_14,	0,	14),
	PIN_MAP(GPIOW_15,	0,	15),
	PIN_MAP(GPIOW_16,	0,	16),
	PIN_MAP(GPIOW_17,	0,	17),
	PIN_MAP(GPIOW_18,	0,	18),
	PIN_MAP(GPIOW_19,	0,	19),
	PIN_MAP(GPIOW_20,	0,	20),

	PIN_AOMAP(GPIOAO_0,	6,	0,	6,	16,	6,	0),
	PIN_AOMAP(GPIOAO_1,	6,	1,	6,	17,	6,	1),
	PIN_AOMAP(GPIOAO_2,	6,	2,	6,	18,	6,	2),
	PIN_AOMAP(GPIOAO_3,	6,	3,	6,	19,	6,	3),
	PIN_AOMAP(GPIOAO_4,	6,	4,	6,	20,	6,	4),
	PIN_AOMAP(GPIOAO_5,	6,	5,	6,	21,	6,	5),
	PIN_AOMAP(GPIOAO_6,	6,	6,	6,	22,	6,	6),
	PIN_AOMAP(GPIOAO_7,	6,	7,	6,	23,	6,	7),
	PIN_AOMAP(GPIOAO_8,	6,	8,	6,	24,	6,	8),
	PIN_AOMAP(GPIOAO_9,	6,	9,	6,	25,	6,	9),
	PIN_AOMAP(GPIOAO_10,	6,	10,	6,	26,	6,	10),
	PIN_AOMAP(GPIOAO_11,	6,	11,	6,	27,	6,	11),
	PIN_AOMAP(GPIOAO_12,	6,	12,	6,	28,	6,	12),
	PIN_AOMAP(GPIOAO_13,	6,	13,	6,	29,	6,	13),

	PIN_MAP(CARD_0,		2,	20),
	PIN_MAP(CARD_1,		2,	21),
	PIN_MAP(CARD_2,		2,	22),
	PIN_MAP(CARD_3,		2,	23),
	PIN_MAP(CARD_4,		2,	24),
	PIN_MAP(CARD_5,		2,	25),
	PIN_MAP(CARD_6,		2,	26),
	PIN_MAP(CARD_7,		2,	27),
	PIN_MAP(CARD_8,		2,	28),

	PIN_MAP(GPIOY_0,	1,	0),
	PIN_MAP(GPIOY_1,	1,	1),
	PIN_MAP(GPIOY_2,	1,	2),
	PIN_MAP(GPIOY_3,	1,	3),
	PIN_MAP(GPIOY_4,	1,	4),
	PIN_MAP(GPIOY_5,	1,	5),
	PIN_MAP(GPIOY_6,	1,	6),
	PIN_MAP(GPIOY_7,	1,	7),
	PIN_MAP(GPIOY_8,	1,	8),
	PIN_MAP(GPIOY_9,	1,	9),
	PIN_MAP(GPIOY_10,	1,	10),
	PIN_MAP(GPIOY_11,	1,	11),
	PIN_MAP(GPIOY_12,	1,	12),
	PIN_MAP(GPIOY_13,	1,	13)
};

/* amlogic request gpio interface*/
int gpio_amlogic_requst(struct gpio_chip *chip,unsigned offset)
{
	int ret;
	unsigned int i,reg,bit;
	unsigned int *gpio_reg = &meson_pinmux_table[offset][0];
	ret=pinctrl_request_gpio(offset);
	gpio_print("==%s==%d\n",__FUNCTION__,__LINE__);
	if (!ret) {
		for (i=0;i<sizeof(meson_pinmux_table[offset])/sizeof(meson_pinmux_table[offset][0]);i++) {
			if (gpio_reg[i] != PMUX_NONE)
			{
				reg=GPIO_REG(gpio_reg[i]);
				bit=GPIO_BIT(gpio_reg[i]);
				aml_clr_reg32_mask(p_pin_mux_reg_addr[reg],1<<bit);
				gpio_print("clr reg=%d,bit =%d\n",reg,bit);
			}
		}
	}
	return ret;
}
/* amlogic request gpio interface*/

void	 gpio_amlogic_free(struct gpio_chip *chip,unsigned offset)
{
	 pinctrl_free_gpio(offset);
	return;
}
int gpio_amlogic_to_irq(struct gpio_chip *chip,unsigned offset)
{
	unsigned reg,start_bit;
	unsigned irq_bank=gpio_flag&0x7;
	unsigned filter=(gpio_flag>>8)&0x7;
	unsigned irq_type=(gpio_flag>>16)&0x3;
	unsigned type[]={0x0, 	/*GPIO_IRQ_HIGH*/
				0x10000, /*GPIO_IRQ_LOW*/
				0x1,  	/*GPIO_IRQ_RISING*/
				0x10001, /*GPIO_IRQ_FALLING*/
				};
	 /*set trigger type*/
	if (offset>GPIOX_21)
		return -1;
	aml_clrset_reg32_bits(P_GPIO_INTR_EDGE_POL,0x10001<<irq_bank,type[irq_type]<<irq_bank);
	printk(" reg:%x,clearmask=%x,setmask=%x\n",(P_GPIO_INTR_EDGE_POL&0xffff)>>2,0x10001<<irq_bank,(aml_read_reg32(P_GPIO_INTR_EDGE_POL)>>irq_bank)&0x10001);
	/*select pin*/
	reg=irq_bank<4?P_GPIO_INTR_GPIO_SEL0:P_GPIO_INTR_GPIO_SEL1;
	start_bit=(irq_bank&3)*8;
	aml_clrset_reg32_bits(reg,0xff<<start_bit,amlogic_pins[offset].num<<start_bit);
	printk("reg:%x,clearmask=%x,set pin=%d\n",(reg&0xffff)>>2,0xff<<start_bit,(aml_read_reg32(reg)>>start_bit)&0xff);
	/*set filter*/
	start_bit=(irq_bank)*4;
	aml_clrset_reg32_bits(P_GPIO_INTR_FILTER_SEL0,0x7<<start_bit,filter<<start_bit);
	printk("reg:%x,clearmask=%x,setmask=%x\n",(P_GPIO_INTR_FILTER_SEL0&0xffff)>>2,0x7<<start_bit,(aml_read_reg32(P_GPIO_INTR_FILTER_SEL0)>>start_bit)&0x7);
	return 0;
}

int gpio_amlogic_direction_input(struct gpio_chip *chip,unsigned offset)
{
	unsigned int reg,bit;
	gpio_print("==%s==%d\n",__FUNCTION__,__LINE__);
	reg=GPIO_REG(amlogic_pins[offset].out_en_reg_bit);
	bit=GPIO_BIT(amlogic_pins[offset].out_en_reg_bit);
	aml_set_reg32_mask(p_gpio_oen_addr[reg],1<<bit);
	return 0;
}

int gpio_amlogic_get(struct gpio_chip *chip,unsigned offset)
{
	unsigned int reg,bit;
	gpio_print("==%s==%d\n",__FUNCTION__,__LINE__);
	reg=GPIO_REG(amlogic_pins[offset].input_value_reg_bit);
	bit=GPIO_BIT(amlogic_pins[offset].input_value_reg_bit);
	return aml_get_reg32_bits(p_gpio_input_addr[reg],bit,1);
}

int gpio_amlogic_direction_output(struct gpio_chip *chip,unsigned offset, int value)
{
	unsigned int reg,bit;

	if (value) {
		reg=GPIO_REG(amlogic_pins[offset].out_value_reg_bit);
		bit=GPIO_BIT(amlogic_pins[offset].out_value_reg_bit);
		aml_set_reg32_mask(p_gpio_output_addr[reg],1<<bit);
		gpio_print("out reg=%x,value=%x\n",p_gpio_output_addr[reg],aml_read_reg32(p_gpio_output_addr[reg]));
	}
	else {
		reg=GPIO_REG(amlogic_pins[offset].out_value_reg_bit);
		bit=GPIO_BIT(amlogic_pins[offset].out_value_reg_bit);
		aml_clr_reg32_mask(p_gpio_output_addr[reg],1<<bit);
		gpio_print("out reg=%x,value=%x\n",p_gpio_output_addr[reg],aml_read_reg32(p_gpio_output_addr[reg]));
	}
	reg=GPIO_REG(amlogic_pins[offset].out_en_reg_bit);
	bit=GPIO_BIT(amlogic_pins[offset].out_en_reg_bit);
	aml_clr_reg32_mask(p_gpio_oen_addr[reg],1<<bit);
	gpio_print("==%s==%d\n",__FUNCTION__,__LINE__);
	gpio_print("oen reg=%x,value=%x\n",p_gpio_oen_addr[reg],aml_read_reg32(p_gpio_oen_addr[reg]));
	gpio_print("value=%d\n",value);
	return 0;
}
void	gpio_amlogic_set(struct gpio_chip *chip,unsigned offset, int value)
{
	unsigned int reg,bit;
	reg=GPIO_REG(amlogic_pins[offset].out_value_reg_bit);
	bit=GPIO_BIT(amlogic_pins[offset].out_value_reg_bit);
	gpio_print("==%s==%d\n",__FUNCTION__,__LINE__);
	if (value)
		aml_set_reg32_mask(p_gpio_output_addr[reg],1<<bit);
	else
		aml_clr_reg32_mask(p_gpio_output_addr[reg],1<<bit);
}
int gpio_amlogic_name_to_num(const char *name)
{
	int i,tmp=100,num=0;
	int len=0;
	char *p=NULL;
	char *start=NULL;
	if (!name)
		return -1;

	len=strlen(name);
	p=kzalloc(len+1,GFP_KERNEL);
	start=p;
	if (!p)
	{
		printk("%s:malloc error\n",__func__);
		return -1;
	}
	p=strcpy(p,name);
	for (i=0; i < len; p++, i++) {
		if (*p == '_') {
			*p='\0';
			tmp=i;
		}
		if (i > tmp && *p >= '0' && *p <= '9')
			num=num*10+*p-'0';
	}
	p = start;
	if (!strcmp(p, "GPIOX"))
		num = num + 0;
	else if (!strcmp(p, "BOOT"))
		num = num + 28;
	else if (!strcmp(p, "GPIOH"))
		num = num + 47;
	else if (!strcmp(p, "GPIOZ"))
		num = num + 58;
	else if (!strcmp(p, "GPIOW"))
		num = num + 79;
	else if (!strcmp(p, "GPIOAO"))
		num = num + 100;
	else if (!strcmp(p, "CARD"))
		num = num + 114;
	else if (!strcmp(p, "GPIOY"))
		num = num + 123;
	else
		num= -1;
	kzfree(start);
	return num;
}

static struct gpio_chip amlogic_gpio_chip={
	.request=gpio_amlogic_requst,
	.free=gpio_amlogic_free,
	.direction_input=gpio_amlogic_direction_input,
	.get=gpio_amlogic_get,
	.direction_output=gpio_amlogic_direction_output,
	.set=gpio_amlogic_set,
	.to_irq=gpio_amlogic_to_irq,
};


static const struct of_device_id amlogic_gpio_match[] =
{
	{
	.compatible = "amlogic,g9bb-gpio",
	},
	{ },
};
struct amlogic_gpio_platform_data
{
	unsigned int base;
	unsigned ngpios;
	struct device_node	*of_node; /* associated device tree node */
};

static int m8b_set_pullup(unsigned int pin,unsigned int val,unsigned int pullen)
{
	unsigned int reg=0,bit=0,bit_en=0,ret;
	ret=meson_pin_to_pullup(pin,&reg,&bit,&bit_en);
	if (!ret)
	{
		if (pullen) {
			if (!ret)
			{
				if (val)
					aml_set_reg32_mask(p_pull_up_addr[reg],1<<bit);
				else
					aml_clr_reg32_mask(p_pull_up_addr[reg],1<<bit);
			}
			aml_set_reg32_mask(p_pull_upen_addr[reg],1<<bit_en);
		}
		else
			aml_clr_reg32_mask(p_pull_upen_addr[reg],1<<bit_en);
	}
	return ret;
}

//#define gpio_dump
//#define pull_dump
//#define dire_dump
static int amlogic_gpio_probe(struct platform_device *pdev)
{
#ifdef CONFIG_OF_GPIO
		amlogic_gpio_chip.of_node = pdev->dev.of_node;
#endif

	amlogic_gpio_chip.base=0;
	amlogic_gpio_chip.ngpio=ARRAY_SIZE(amlogic_pins);
	gpiochip_add(&amlogic_gpio_chip);
	pullup_ops.meson_set_pullup=m8b_set_pullup;
	dev_info(&pdev->dev, "Probed amlogic GPIO driver\n");
#ifdef gpio_dump
	int gi;
	for (gi=0;gi<GPIO_MAX;gi++)
		printk("%s,amlogic_pins[%d]=%d,%d,out en reg=%x,bit=%d,out val reg=%x,bit=%d,input reg=%x,bit=%d\n",
		amlogic_pins[gi].name,gi,amlogic_pins[gi].num,
		gpio_amlogic_name_to_num(amlogic_pins[gi].name),
		(p_gpio_oen_addr[GPIO_REG(amlogic_pins[gi].out_en_reg_bit)]&0xffff)>>2,
		GPIO_BIT(amlogic_pins[gi].out_en_reg_bit),
		(p_gpio_output_addr[GPIO_REG(amlogic_pins[gi].out_value_reg_bit)]&0xffff)>>2,
		GPIO_BIT(amlogic_pins[gi].out_value_reg_bit),
		(p_gpio_input_addr[GPIO_REG(amlogic_pins[gi].input_value_reg_bit)]&0xffff)>>2,
		GPIO_BIT(amlogic_pins[gi].input_value_reg_bit)
	);
#endif
#ifdef irq_dump

	for (i=GPIO_IRQ0;i<GPIO_IRQ7+1;i++) {
		gpio_flag=AML_GPIO_IRQ(i,FILTER_NUM7,GPIO_IRQ_HIGH);
		gpio_amlogic_to_irq(NULL,50);
	}
	for (i=GPIO_IRQ_HIGH;i<GPIO_IRQ_FALLING+1;i++) {
		gpio_flag=AML_GPIO_IRQ(GPIO_IRQ0,FILTER_NUM7,i);
		gpio_amlogic_to_irq(NULL,50);
	}

#endif
#ifdef pull_dump

	int preg,pbit,penbit,pi;
	for (pi=0;pi<GPIO_TEST_N;pi++) {
		m8b_pin_to_pullup(pi,&preg,&pbit,&penbit);
		printk("%s \t,pull up en reg:%x \t,enbit:%d \t,==,pull up reg:%x \t,bit:%d \t\n",
			amlogic_pins[pi].name,
			(p_pull_upen_addr[preg]&0xffff)>>2,
			penbit,
			(p_pull_up_addr[preg]&0xffff)>>2,
			pbit);
	}
#endif
#ifdef dire_dump
extern int m8b_pin_map_to_direction(unsigned int pin,unsigned int *reg,unsigned int *bit);
	int dreg,dbit,di;
	for (di=0;di<GPIO_TEST_N;di++) {
		m8b_pin_map_to_direction(di,&dreg,&dbit);
		printk("%s \t,output en reg:%x \t,enbit:%d \t\n",
			amlogic_pins[di].name,
			(p_gpio_oen_addr[dreg]&0xffff)>>2,
			dbit);
	}
#endif
	return 0;
}



static struct platform_driver amlogic_gpio_driver = {
	.probe		= amlogic_gpio_probe,
	.driver		= {
		.name	= "amlogic_gpio",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(amlogic_gpio_match),
	},
};

/*
 * gpio driver register needs to be done before
 * machine_init functions access gpio APIs.
 * Hence amlogic_gpio_drv_reg() is a postcore_initcall.
 */
static int __init amlogic_gpio_drv_reg(void)
{
	return platform_driver_register(&amlogic_gpio_driver);
}
postcore_initcall(amlogic_gpio_drv_reg);

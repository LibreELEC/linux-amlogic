/*
 * arch/arm/mach-mesong9tv/pinctrl.c
 * Amlogic Pin Controller Driver
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>

#include <plat/io.h>
#include <mach/am_regs.h>
#include <linux/amlogic/pinctrl-amlogic.h>
#include <linux/amlogic/aml_gpio_consumer.h>

DEFINE_MUTEX(spi_nand_mutex);

unsigned p_pull_up_addr[] = {
	P_PAD_PULL_UP_REG0,
	P_PAD_PULL_UP_REG1,
	P_PAD_PULL_UP_REG2,
	P_PAD_PULL_UP_REG3,
	P_PAD_PULL_UP_REG4,
	P_AO_RTI_PULL_UP_REG,
};

unsigned int p_pin_mux_reg_addr[] = {
	P_PERIPHS_PIN_MUX_0,
	P_PERIPHS_PIN_MUX_1,
	P_PERIPHS_PIN_MUX_2,
	P_PERIPHS_PIN_MUX_3,
	P_PERIPHS_PIN_MUX_4,
	P_PERIPHS_PIN_MUX_5,
	P_PERIPHS_PIN_MUX_6,
	P_PERIPHS_PIN_MUX_7,
	P_PERIPHS_PIN_MUX_8,
	P_PERIPHS_PIN_MUX_9,
	P_PERIPHS_PIN_MUX_10,
	P_PERIPHS_PIN_MUX_11,
	P_PERIPHS_PIN_MUX_12,
	P_AO_RTI_PIN_MUX_REG,
};

unsigned p_pull_upen_addr[] = {
	P_PAD_PULL_UP_EN_REG0,
	P_PAD_PULL_UP_EN_REG1,
	P_PAD_PULL_UP_EN_REG2,
	P_PAD_PULL_UP_EN_REG3,
	P_PAD_PULL_UP_EN_REG4,
	P_AO_RTI_PULL_UP_REG,
};

/* Pad names for the pinmux subsystem */
const static struct pinctrl_pin_desc g9tv_pads[] = {

	PINCTRL_PIN(GPIOAO_0,	"GPIOAO_0"),
	PINCTRL_PIN(GPIOAO_1,	"GPIOAO_1"),
	PINCTRL_PIN(GPIOAO_2,	"GPIOAO_2"),
	PINCTRL_PIN(GPIOAO_3,	"GPIOAO_3"),
	PINCTRL_PIN(GPIOAO_4,	"GPIOAO_4"),
	PINCTRL_PIN(GPIOAO_5,	"GPIOAO_5"),
	PINCTRL_PIN(GPIOAO_6,	"GPIOAO_6"),
	PINCTRL_PIN(GPIOAO_7,	"GPIOAO_7"),
	PINCTRL_PIN(GPIOAO_8,	"GPIOAO_8"),
	PINCTRL_PIN(GPIOAO_9,	"GPIOAO_9"),
	PINCTRL_PIN(GPIOAO_10,	"GPIOAO_10"),
	PINCTRL_PIN(GPIOAO_11,	"GPIOAO_11"),
	PINCTRL_PIN(GPIOAO_12,	"GPIOAO_12"),
	PINCTRL_PIN(GPIOAO_13,	"GPIOAO_13"),

	PINCTRL_PIN(GPIOZ_0,	"GPIOZ_0"),
	PINCTRL_PIN(GPIOZ_1,	"GPIOZ_1"),
	PINCTRL_PIN(GPIOZ_2,	"GPIOZ_2"),
	PINCTRL_PIN(GPIOZ_3,	"GPIOZ_3"),
	PINCTRL_PIN(GPIOZ_4,	"GPIOZ_4"),
	PINCTRL_PIN(GPIOZ_5,	"GPIOZ_5"),
	PINCTRL_PIN(GPIOZ_6,	"GPIOZ_6"),
	PINCTRL_PIN(GPIOZ_7,	"GPIOZ_7"),
	PINCTRL_PIN(GPIOZ_8,	"GPIOZ_8"),
	PINCTRL_PIN(GPIOZ_9,	"GPIOZ_9"),
	PINCTRL_PIN(GPIOZ_10,	"GPIOZ_10"),
	PINCTRL_PIN(GPIOZ_11,	"GPIOZ_11"),
	PINCTRL_PIN(GPIOZ_12,	"GPIOZ_12"),
	PINCTRL_PIN(GPIOZ_13,	"GPIOZ_13"),
	PINCTRL_PIN(GPIOZ_14,	"GPIOZ_14"),
	PINCTRL_PIN(GPIOZ_15,	"GPIOZ_15"),
	PINCTRL_PIN(GPIOZ_16,	"GPIOZ_16"),
	PINCTRL_PIN(GPIOZ_17,	"GPIOZ_17"),
	PINCTRL_PIN(GPIOZ_18,	"GPIOZ_18"),
	PINCTRL_PIN(GPIOZ_19,	"GPIOZ_19"),
	PINCTRL_PIN(GPIOZ_20,	"GPIOZ_20"),

	PINCTRL_PIN(GPIOH_0,	"GPIOH_0"),
	PINCTRL_PIN(GPIOH_1,	"GPIOH_1"),
	PINCTRL_PIN(GPIOH_2,	"GPIOH_2"),
	PINCTRL_PIN(GPIOH_3,	"GPIOH_3"),
	PINCTRL_PIN(GPIOH_4,	"GPIOH_4"),
	PINCTRL_PIN(GPIOH_5,	"GPIOH_5"),
	PINCTRL_PIN(GPIOH_6,	"GPIOH_6"),
	PINCTRL_PIN(GPIOH_7,	"GPIOH_7"),
	PINCTRL_PIN(GPIOH_8,	"GPIOH_8"),
	PINCTRL_PIN(GPIOH_9,	"GPIOH_9"),
	PINCTRL_PIN(GPIOH_10,	"GPIOH_10"),

	PINCTRL_PIN(BOOT_0,	"BOOT_0"),
	PINCTRL_PIN(BOOT_1,	"BOOT_1"),
	PINCTRL_PIN(BOOT_2,	"BOOT_2"),
	PINCTRL_PIN(BOOT_3,	"BOOT_3"),
	PINCTRL_PIN(BOOT_4,	"BOOT_4"),
	PINCTRL_PIN(BOOT_5,	"BOOT_5"),
	PINCTRL_PIN(BOOT_6,	"BOOT_6"),
	PINCTRL_PIN(BOOT_7,	"BOOT_7"),
	PINCTRL_PIN(BOOT_8,	"BOOT_8"),
	PINCTRL_PIN(BOOT_9,	"BOOT_9"),
	PINCTRL_PIN(BOOT_10,	"BOOT_10"),
	PINCTRL_PIN(BOOT_11,	"BOOT_11"),
	PINCTRL_PIN(BOOT_12,	"BOOT_12"),
	PINCTRL_PIN(BOOT_13,	"BOOT_13"),
	PINCTRL_PIN(BOOT_14,	"BOOT_14"),
	PINCTRL_PIN(BOOT_15,	"BOOT_15"),
	PINCTRL_PIN(BOOT_16,	"BOOT_16"),
	PINCTRL_PIN(BOOT_17,	"BOOT_17"),
	PINCTRL_PIN(BOOT_18,	"BOOT_18"),

	PINCTRL_PIN(CARD_0,	"CARD_0"),
	PINCTRL_PIN(CARD_1,	"CARD_1"),
	PINCTRL_PIN(CARD_2,	"CARD_2"),
	PINCTRL_PIN(CARD_3,	"CARD_3"),
	PINCTRL_PIN(CARD_4,	"CARD_4"),
	PINCTRL_PIN(CARD_5,	"CARD_5"),
	PINCTRL_PIN(CARD_6,	"CARD_6"),
	PINCTRL_PIN(CARD_7,	"CARD_7"),
	PINCTRL_PIN(CARD_8,	"CARD_8"),

	PINCTRL_PIN(GPIOW_0,	"GPIOW_0"),
	PINCTRL_PIN(GPIOW_1,	"GPIOW_1"),
	PINCTRL_PIN(GPIOW_2,	"GPIOW_2"),
	PINCTRL_PIN(GPIOW_3,	"GPIOW_3"),
	PINCTRL_PIN(GPIOW_4,	"GPIOW_4"),
	PINCTRL_PIN(GPIOW_5,	"GPIOW_5"),
	PINCTRL_PIN(GPIOW_6,	"GPIOW_6"),
	PINCTRL_PIN(GPIOW_7,	"GPIOW_7"),
	PINCTRL_PIN(GPIOW_8,	"GPIOW_8"),
	PINCTRL_PIN(GPIOW_9,	"GPIOW_9"),
	PINCTRL_PIN(GPIOW_10,	"GPIOW_10"),
	PINCTRL_PIN(GPIOW_11,	"GPIOW_11"),
	PINCTRL_PIN(GPIOW_12,	"GPIOW_12"),
	PINCTRL_PIN(GPIOW_13,	"GPIOW_13"),
	PINCTRL_PIN(GPIOW_14,	"GPIOW_14"),
	PINCTRL_PIN(GPIOW_15,	"GPIOW_15"),
	PINCTRL_PIN(GPIOW_16,	"GPIOW_16"),
	PINCTRL_PIN(GPIOW_17,	"GPIOW_17"),
	PINCTRL_PIN(GPIOW_18,	"GPIOW_18"),
	PINCTRL_PIN(GPIOW_19,	"GPIOW_19"),
	PINCTRL_PIN(GPIOW_20,	"GPIOW_20"),

	PINCTRL_PIN(GPIOY_0,	"GPIOY_0"),
	PINCTRL_PIN(GPIOY_1,	"GPIOY_1"),
	PINCTRL_PIN(GPIOY_2,	"GPIOY_2"),
	PINCTRL_PIN(GPIOY_3,	"GPIOY_3"),
	PINCTRL_PIN(GPIOY_4,	"GPIOY_4"),
	PINCTRL_PIN(GPIOY_5,	"GPIOY_5"),
	PINCTRL_PIN(GPIOY_6,	"GPIOY_6"),
	PINCTRL_PIN(GPIOY_7,	"GPIOY_7"),
	PINCTRL_PIN(GPIOY_8,	"GPIOY_8"),
	PINCTRL_PIN(GPIOY_9,	"GPIOY_9"),
	PINCTRL_PIN(GPIOY_10,	"GPIOY_10"),
	PINCTRL_PIN(GPIOY_11,	"GPIOY_11"),
	PINCTRL_PIN(GPIOY_12,	"GPIOY_12"),
	PINCTRL_PIN(GPIOY_13,	"GPIOY_13"),

	PINCTRL_PIN(GPIOX_0,	"GPIOX_0"),
	PINCTRL_PIN(GPIOX_1,	"GPIOX_1"),
	PINCTRL_PIN(GPIOX_2,	"GPIOX_2"),
	PINCTRL_PIN(GPIOX_3,	"GPIOX_3"),
	PINCTRL_PIN(GPIOX_4,	"GPIOX_4"),
	PINCTRL_PIN(GPIOX_5,	"GPIOX_5"),
	PINCTRL_PIN(GPIOX_6,	"GPIOX_6"),
	PINCTRL_PIN(GPIOX_7,	"GPIOX_7"),
	PINCTRL_PIN(GPIOX_8,	"GPIOX_8"),
	PINCTRL_PIN(GPIOX_9,	"GPIOX_9"),
	PINCTRL_PIN(GPIOX_10,	"GPIOX_10"),
	PINCTRL_PIN(GPIOX_11,	"GPIOX_11"),
	PINCTRL_PIN(GPIOX_12,	"GPIOX_12"),
	PINCTRL_PIN(GPIOX_13,	"GPIOX_13"),
	PINCTRL_PIN(GPIOX_14,	"GPIOX_14"),
	PINCTRL_PIN(GPIOX_15,	"GPIOX_15"),
	PINCTRL_PIN(GPIOX_16,	"GPIOX_16"),
	PINCTRL_PIN(GPIOX_17,	"GPIOX_17"),
	PINCTRL_PIN(GPIOX_18,	"GPIOX_18"),
	PINCTRL_PIN(GPIOX_19,	"GPIOX_19"),
	PINCTRL_PIN(GPIOX_20,	"GPIOX_20"),
	PINCTRL_PIN(GPIOX_21,	"GPIOX_21"),
	PINCTRL_PIN(GPIOX_22,	"GPIOX_22"),
	PINCTRL_PIN(GPIOX_23,	"GPIOX_23"),
	PINCTRL_PIN(GPIOX_24,	"GPIOX_24"),
	PINCTRL_PIN(GPIOX_25,	"GPIOX_25"),
	PINCTRL_PIN(GPIOX_26,	"GPIOX_26"),
	PINCTRL_PIN(GPIOX_27,	"GPIOX_27"),
};

int g9tv_pin_to_pullup(unsigned int pin , unsigned int *reg, unsigned int *bit,
	unsigned int *en)
{
	/*
	AO_RTI_PULL_UP_REG		0xc810002c

	31	R	0		Reserved
	30	R/W	0		TEST_N pull-up/down direction.
	29-16	R/W	0		gpioAO[13:0] pull-up/down direction.
	15	R	0		Reserved
	14	R/W	0		TEST_N pull-up enable.
	13-0	R/W	0		gpioAO[13:0] pull-up enable
	*/
	if (pin <= GPIOAO_13)
	{
		*reg = 5;
		*bit = pin - GPIOAO_0 + 16;
		*en  = pin - GPIOAO_0;
	}
	/*
	PAD_PULL_UP_REG3 0x203d

	31~21	R/W	0		Reserved
	20~0	R/W	0x1FA040	gpioZ[20:0] 1 = pull up.  0 = pull down
	*/
	/*
	PULL_UP_EN_REG3	0x204b

	31~21	R/W	0		Reserved
	20~0	R/W	0x1FE07F	gpioZ[20:0]
	*/
	else if (pin <= GPIOZ_20)
	{
		*reg = 3;
		*bit = pin - GPIOZ_0;
		*en  = *bit;
	}
	/*
	PAD_PULL_UP_REG1 0x203b

	31~27	R/W	0		Unused
	26~16	R/W	0x1FC		gpioH[10:0]
	*/
	/*
	PULL_UP_EN_REG1	0x2049

	31~27	R/W	0		Unused
	26~16	R/W	0x67D		gpioH[10:0]
	*/
	else if(pin <= GPIOH_10)
	{
		*reg = 1;
		*bit = pin - GPIOH_0 + 16;
		*en  = *bit;
	}
	/*
	PAD_PULL_UP_REG2 0x203c

	19	R/W	1		Reserved
	18~0	R/W	0x77FFF 	boot[18:0] 1 = pull up.  0 = pull down
	*/
	/*
	PULL_UP_EN_REG2	0x204a

	19	R/W	1		Reserved
	18~0	R/W	0x7FFFF 	boot[18:0]
	*/
	else if (pin <= BOOT_18)
	{
		*reg = 2;
		*bit = pin - BOOT_0;
		*en  = *bit;
	}
	/*
	PAD_PULL_UP_REG2 0x203c

	31~20	R/W	0		Reserved
	28~20	R/W	0x1FF		card[8:0] 1 = pull up.	0 = pull down
	*/
	/*
	PULL_UP_EN_REG2 0x204a

	31~20	R/W	0		Reserved
	28~20	R/W	0x1FF		card[8:0]
	*/
	else if (pin <= CARD_8)
	{
		*reg = 2;
		*bit = pin - CARD_0 + 20;
		*en  = *bit;
	}
	/*
	PAD_PULL_UP_REG0 0x203a

	31~21	R/W	0		Unused
	20~0	R/W	0x2223F 	gpioW[20:0] 1 = pull up.  0 = pull down
	*/
	/*
	PULL_UP_EN_REG0	0x2048

	31~21	R/W	0		Unused
	20~0	R/W	0x2223F		gpioW[20:0]
	*/
	else if(pin <= GPIOW_20)
	{
		*reg = 0;
		*bit = pin - GPIOW_0;
		*en  = *bit;
	}
	/*
	PAD_PULL_UP_REG1 0x203b

	15~14	R/W	0		Reserved
	13~0	R/W	0x09FB		gpioY[13:0]
	*/
	/*
	PULL_UP_EN_REG1	0x2049

	15~14	R/W	0		Reserved
	13~0	R/W	0x3FFF		gpioY[13:0]
	*/
	else if (pin <= GPIOY_13)
	{
		*reg = 1;
		*bit = pin - GPIOY_0;
		*en  = *bit;
	}
	/*
	PAD_PULL_UP_REG4 0x203e

	31~28	R/W	0		Unused
	27~0	R/W	0x3000000	gpioX[27:0] 1 = pull up.  0 = pull down
	*/
	/*
	PULL_UP_EN_REG4 0x204c

	31~28	R/W	0		Unused
	27~0	R/W	0xF000FFF	gpioX[27:0]
	*/
	else if (pin <= GPIOX_27)
	{
		*reg = 4;
		*bit = pin - GPIOX_0;
		*en  = *bit;
	}
	else
		return -1;
	return 0;
}

int g9tv_pin_map_to_direction(unsigned int pin,unsigned int *reg,unsigned int *bit)
{
	/*
	P_AO_GPIO_O_EN_N	0xc8100024
	13-0			GPIOAO[13:0]
	*/
	if (pin <= GPIOAO_13)
	{
		*reg = 6;
		*bit = pin - GPIOAO_0;
	}
	/*
	PREG_PAD_GPIO3_EN_N	0x2015
	20~0			GPIOZ[20:0]
	*/
	else if (pin <= GPIOZ_20)
	{
		*reg = 3;
		*bit = pin - GPIOZ_0;
	}
	/*
	PREG_PAD_GPIO1_EN_N	0x200f
	26~16			GPIOH[10:0]
	*/
	else if (pin <= GPIOH_10)
	{
		*reg = 1;
		*bit = pin - GPIOH_0 + 16;
	}
	/*
	PREG_PAD_GPIO2_EN_N	0x2012
	18~0			BOOT[18:0]
	*/
	else if (pin <= BOOT_18)
	{
		*reg = 2;
		*bit = pin - BOOT_0;
	}
	/*
	PREG_PAD_GPIO2_EN_N	0x2012
	28~20			CARD[8:0]
	*/
	else if (pin <= CARD_8)
	{
		*reg = 2;
		*bit = pin - CARD_0 + 20;
	}
	/*
	PREG_PAD_GPIO0_EN_N	0x200c
	20~0			GPIOW[20:0]
	*/
	else if (pin <= GPIOW_20)
	{
		*reg = 0;
		*bit = pin - GPIOW_0;
	}
	/*
	PREG_PAD_GPIO1_EN_N	0x200f
	13~0			GPIOY[13:0]
	*/
	else if (pin <= GPIOY_13)
	{
		*reg = 1;
		*bit = pin - GPIOY_0;
	}
	/*
	PREG_PAD_GPIO4_EN_N	0x2018
	27~0			GPIOX[27:0]
	*/
	else if (pin <= GPIOX_27)
	{
		*reg = 4;
		*bit = pin - GPIOX_0;
	}
	else
		return -1;
	return 0;
}

static int g9tv_set_pullup(unsigned int pin, unsigned int config)
{
	unsigned int reg=0, bit=0, bit_en=0, ret;
	u16 pullarg = AML_PINCONF_UNPACK_PULL_ARG(config);
	u16 pullen = AML_PINCONF_UNPACK_PULL_EN(config);
	ret = g9tv_pin_to_pullup(pin, &reg, &bit, &bit_en);

	if (!ret)
	{
		if (pullen) {
			if (pullarg)
				aml_set_reg32_mask(p_pull_up_addr[reg], 1<<bit);
			else
				aml_clr_reg32_mask(p_pull_up_addr[reg], 1<<bit);
			aml_set_reg32_mask(p_pull_upen_addr[reg], 1<<bit_en);
		}
		else
			aml_clr_reg32_mask(p_pull_upen_addr[reg], 1<<bit_en);
	}
	return ret;
}

static struct amlogic_pinctrl_soc_data g9tv_pinctrl = {
	.pins = g9tv_pads,
	.npins = ARRAY_SIZE(g9tv_pads),
	.meson_set_pullup = g9tv_set_pullup,
	.pin_map_to_direction = g9tv_pin_map_to_direction,
};

static struct of_device_id g9tv_pinctrl_of_match[] = {
	{
		.compatible="amlogic,pinmux-g9tv",
	},
	{},
};

static int  g9tv_pmx_probe(struct platform_device *pdev)
{
	return amlogic_pmx_probe(pdev,&g9tv_pinctrl);
}

static int  g9tv_pmx_remove(struct platform_device *pdev)
{
	return amlogic_pmx_remove(pdev);
}

static struct platform_driver g9tv_pmx_driver = {
	.driver = {
		.name = "pinmux-g9tv",
		.owner = THIS_MODULE,
		.of_match_table=of_match_ptr(g9tv_pinctrl_of_match),
	},
	.probe  = g9tv_pmx_probe,
	.remove = g9tv_pmx_remove,
};

static int __init g9tv_pmx_init(void)
{
	return platform_driver_register(&g9tv_pmx_driver);
}
arch_initcall(g9tv_pmx_init);

static void __exit g9tv_pmx_exit(void)
{
	platform_driver_unregister(&g9tv_pmx_driver);
}
module_exit(g9tv_pmx_exit);

MODULE_DESCRIPTION("Amlogic Pin Controller Driver");
MODULE_LICENSE("GPL v2");

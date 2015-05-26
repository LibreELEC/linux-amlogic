/*
 * arch/arm/mach-mesong9tv/iomapping.c
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sizes.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <asm/memory.h>
#include <asm/mach/map.h>
#include <mach/io.h>

static __initdata struct map_desc meson_default_io_desc[]  = {
	{
	.virtual	= IO_PL310_BASE,
	.pfn		= __phys_to_pfn(IO_PL310_PHY_BASE),
	.length		= SZ_4K,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_A9_PERIPH_BASE,
	.pfn		= __phys_to_pfn(IO_A9_PERIPH_PHY_BASE),
	.length		= SZ_16K,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_MMC_BUS_BASE,
	.pfn		= __phys_to_pfn(IO_MMC_PHY_BASE),
	.length		= SZ_32K,
	.type		= MT_DEVICE,
	},{
	.virtual	= IO_BOOTROM_BASE,
	.pfn		= __phys_to_pfn(IO_BOOTROM_PHY_BASE),
	.length		= SZ_64K,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_SRAM_BASE,
	.pfn		= __phys_to_pfn(IO_SRAM_PHY_BASE),
	.length		= SZ_128K,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_USB_A_BASE,
	.pfn		= __phys_to_pfn(IO_USB_A_PHY_BASE),
	.length		= SZ_256K,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_USB_B_BASE,
	.pfn		= __phys_to_pfn(IO_USB_B_PHY_BASE),
	.length		= SZ_256K,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_ETH_BASE,
	.pfn		= __phys_to_pfn(IO_ETH_PHY_BASE),
	.length		= SZ_64K,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_SECBUS_BASE,
	.pfn		= __phys_to_pfn(IO_SECBUS_PHY_BASE),
	.length		= SZ_32K,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_TV_BUS_BASE,
	.pfn		= __phys_to_pfn(IO_TV_BASE),
	.length		= SZ_32K,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_DEMOD_BASE,
	.pfn		= __phys_to_pfn(IO_DEMOD_PHY_BASE),
	.length		= SZ_128K,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_CBUS_BASE,
	.pfn		= __phys_to_pfn(IO_CBUS_PHY_BASE),
	.length		= SZ_1M,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_AXI_BUS_BASE,
	.pfn		= __phys_to_pfn(IO_AXI_BUS_PHY_BASE),
	.length		= SZ_2M,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_APB_BUS_BASE,
	.pfn		= __phys_to_pfn(IO_APB_BUS_PHY_BASE),
	.length		= SZ_2M,
	.type		= MT_DEVICE,
	}, {
	.virtual	= IO_AOBUS_BASE,
	.pfn		= __phys_to_pfn(IO_AOBUS_PHY_BASE),
	.length		= SZ_1M,
	.type		= MT_DEVICE,
	},

#ifdef CONFIG_AMLOGIC_SPI_HW_MASTER
	{
	.virtual	= IO_SPIMEM_BASE,
	.pfn		= __phys_to_pfn(IO_SPIMEM_PHY_BASE),
	.length		= SZ_64M,
	.type		= MT_ROM,
	},
#endif

#ifdef CONFIG_MESON_SUSPEND
	{
	.virtual	= PAGE_ALIGN(__phys_to_virt(0x04f00000)),
	.pfn		= __phys_to_pfn(0x04f00000),
	.length		= SZ_1M,
	.type		= MT_MEMORY_NONCACHED,
	},
#endif
};

unsigned int meson_uart_phy_addr = 0xc8100000;

void __init meson_map_default_io(void)
{
	iotable_init(meson_default_io_desc, ARRAY_SIZE(meson_default_io_desc));
	meson_uart_phy_addr = IO_AOBUS_BASE;
}

/*
 * arch/arm/mach-mesong9bb/include/mach/io.h
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

#ifndef __MACH_MESONG9BB_IO_H
#define __MACH_MESONG9BB_IO_H

///amlogic old style driver porting
#if (defined CONFIG_MESON_LEGACY_REGISTER_API) && CONFIG_MESON_LEGACY_REGISTER_API
#include "avosstyle_io.h"
#else
#warning "You should replace the register operation with \n" 	\
	"writel/readl/setbits_le32/clrbits_le32/clrsetbits_le32.\n" \
	"the register name must be replace with P_REG_NAME . \n"	\
	"REG_NAME is the old stlye reg name . 	"
#endif
//#define IO_SPACE_LIMIT 0xffffffff

//#define __io(a)     __typesafe_io(a)
#define __mem_pci(a)    (a)


/**
 * U boot style operation
 */


#define clrbits_le32 aml_clr_reg32_mask
#define setbits_le32 aml_set_reg32_mask
#define clrsetbits_le32 aml_clrset_reg32_bits

/**
 * PHY IO MEMORY BASE
 */
#define IO_PHY_BASE			0xC0000000  ///value from vlsi team

#define IO_CBUS_PHY_BASE		0xC1100000  ///2M
#define IO_AXI_BUS_PHY_BASE		0xC1300000  ///1M

#define IO_PL310_PHY_BASE		0xC4200000  ///4K
#define IO_A9_PERIPH_PHY_BASE		0xC4300000  ///4K

#define IO_MMC_PHY_BASE			0xC8000000  ///32K
#define IO_TVIN_PHY_BASE		0xC8008000  ///32K
#define IO_DEMOD_COMB_PHY_BASE		0xC8020000  ///128K
#define IO_USB_COMB_PHY_BASE		0xC8022000  ///8K

#define IO_AOBUS_PHY_BASE		0xC8100000  ///1M

#define IO_USB_A_PHY_BASE		0xC9100000  ///256K
#define IO_USB_B_PHY_BASE		0xC9140000  ///256K
#define IO_USB_C_PHY_BASE		0xC9180000  ///256K
#define IO_ETH_PHY_BASE			0xc9410000  ///64K

#define IO_SPIMEM_PHY_BASE		0xCC000000  ///64M

#define IO_APB_BUS_PHY_BASE		0xD0000000  ///256K	///2M
#define IO_HDMITX_PHY_BASE		0xD0042000  ///8K
#define IO_NAND_PHY_BASE		0xD0048000  ///32K
#define IO_DOS_BUS_PHY_BASE		0xD0050000  ///64K
#define IO_MALI_APB_PHY_BASE		0xD00C0000  ///256K
#define IO_VPU_PHY_BASE			0xD0100000  ///256K

#define IO_SRAM_PHY_BASE		0xD9000000  ///128K
#define IO_BOOTROM_PHY_BASE		0xD9040000  ///64K

#define IO_SECBUS_PHY_BASE		0xDA000000
	#define IO_EFUSE_PHY_BASE	(IO_SECBUS_PHY_BASE + 0x0000)  ///8K
	#define IO_SECURE_PHY_BASE	(IO_SECBUS_PHY_BASE + 0x2000)  ///16K
	#define IO_BLKMV_PHY_BASE	(IO_SECBUS_PHY_BASE + 0x6000)  ///8K


#ifdef CONFIG_VMSPLIT_3G
#define IO_REGS_BASE			(0xFE000000)

#define IO_PL310_BASE_4K		(IO_REGS_BASE + 0x000000)
#define IO_A9_PERIPH_BASE_16K		(IO_PL310_BASE_4K + SZ_4K)
#define IO_MMC_BASE_32K			(IO_A9_PERIPH_BASE_16K + SZ_16K)
#define IO_TVIN_BASE_32K		(IO_MMC_BASE_32K + SZ_32K)
#define IO_DEMOD_COMB_BASE_8K		(IO_TVIN_BASE_32K + SZ_32K)
#define IO_USB_COMB_BASE_8K		(IO_DEMOD_COMB_BASE_8K + SZ_8K)

#define IO_USB_A_BASE_256K		(IO_USB_COMB_BASE_8K + SZ_8K)
#define IO_USB_B_BASE_256K		(IO_USB_A_BASE_256K + SZ_256K)
#define IO_USB_C_BASE_256K		(IO_USB_B_BASE_256K + SZ_256K)
#define IO_ETH_BASE_64K			(IO_USB_C_BASE_256K + SZ_256K)

#define IO_SRAM_BASE_128K		(IO_ETH_BASE_64K + SZ_64K)
#define IO_BOOTROM_BASE_64K		(IO_SRAM_BASE_128K + SZ_128K)

#define IO_SECBUS_BASE_32K		(IO_BOOTROM_BASE_64K + SZ_64K)
	#define IO_EFUSE_BASE_8K	(IO_SECBUS_BASE_32K + 0x0000)
	#define IO_SECURE_BASE_16K	(IO_SECBUS_BASE_32K + 0x2000)
	#define IO_BLKMV_BASE_8K	(IO_SECBUS_BASE_32K + 0x6000)

#define IO_CBUS_BASE_2M			(IO_BLKMV_BASE_8K + SZ_8K)
#define IO_AXI_BUS_BASE_1M		(IO_CBUS_BASE_2M + SZ_2M)
#define IO_AOBUS_BASE_1M		(IO_AXI_BUS_BASE_1M + SZ_1M)
#define IO_APB_BUS_BASE_2M		(IO_AOBUS_BASE_1M + SZ_1M)
	#define IO_HDMITX_BASE_8K	(IO_APB_BUS_BASE_2M +  0x42000)
	#define IO_NAND_BASE_32K	(IO_APB_BUS_BASE_2M +  0x48000)
	#define IO_DOS_BUS_BASE_64K	(IO_APB_BUS_BASE_2M +  0x50000)
	#define IO_MALI_APB_BASE_256K	(IO_APB_BUS_BASE_2M +  0xC0000)
	#define IO_VPU_BASE_256K	(IO_APB_BUS_BASE_2M + 0x100000)

#define IO_REGS_END			(IO_REGS_BASE + (15 * SZ_1M) - 1) // Total 15M

#define IO_SPIMEM_BASE_64M		(IO_REGS_BASE - SZ_64M)
#define IO_SPI_END			(IO_SPIMEM_BASE_64M + SZ_64M - 1) // Total 64M


/* original name for compile ok */
#define IO_PL310_BASE			(IO_PL310_BASE_4K)
#define IO_PERIPH_BASE			(IO_A9_PERIPH_BASE_16K)
#define IO_A9_PERIPH_BASE		(IO_A9_PERIPH_BASE_16K)
#define IO_MMC_BASE			(IO_MMC_BASE_32K)
#define IO_TVIN_BASE			(IO_TVIN_BASE_32K)
#define IO_DEMOD_COMB_BASE		(IO_DEMOD_COMB_BASE_8K)
#define IO_USB_COMB_BASE		(IO_USB_COMB_BASE_8K)
#define IO_USB_A_BASE			(IO_USB_A_BASE_256K)
#define IO_USB_B_BASE			(IO_USB_B_BASE_256K)
#define IO_USB_C_BASE			(IO_USB_C_BASE_256K)
#define IO_ETH_BASE			(IO_ETH_BASE_64K)
#define IO_SRAM_BASE			(IO_SRAM_BASE_128K)
#define IO_BOOTROM_BASE			(IO_BOOTROM_BASE_64K)
#define IO_SECBUS_BASE			(IO_SECBUS_BASE_32K)
#define IO_CBUS_BASE			(IO_CBUS_BASE_2M)
#define IO_AXI_BUS_BASE			(IO_AXI_BUS_BASE_1M)
#define IO_AOBUS_BASE			(IO_AOBUS_BASE_1M)
#define IO_APB_BUS_BASE			(IO_APB_BUS_BASE_2M)
#endif // CONFIG_VMSPLIT_3G


#ifdef CONFIG_VMSPLIT_2G
#define IO_PL310_BASE_4K		(IO_PL310_PHY_BASE)
#define IO_A9_PERIPH_BASE_16K		(IO_A9_PERIPH_PHY_BASE)
#define IO_MMC_BASE_32K			(IO_MMC_PHY_BASE)
#define IO_TVIN_BASE_32K		(IO_TVIN_PHY_BASE)
#define IO_DEMOD_COMB_BASE_8K		(IO_DEMOD_COMB_PHY_BASE)
#define IO_USB_COMB_BASE_8K		(IO_USB_COMB_PHY_BASE)

#define IO_USB_A_BASE_256K		(IO_USB_A_PHY_BASE)
#define IO_USB_B_BASE_256K		(IO_USB_B_PHY_BASE)
#define IO_USB_C_BASE_256K		(IO_USB_C_PHY_BASE)
#define IO_ETH_BASE_64K			(IO_ETH_PHY_BASE)

#define IO_SRAM_BASE_128K		(IO_SRAM_PHY_BASE)
#define IO_BOOTROM_BASE_64K		(IO_BOOTROM_PHY_BASE)

#define IO_SECBUS_BASE_32K		(IO_SECBUS_PHY_BASE)
	#define IO_EFUSE_BASE_8K	(IO_EFUSE_PHY_BASE)
	#define IO_SECURE_BASE_16K	(IO_SECURE_PHY_BASE)
	#define IO_BLKMV_BASE_8K	(IO_BLKMV_PHY_BASE)

#define IO_CBUS_BASE_2M			(IO_CBUS_PHY_BASE)
#define IO_AXI_BUS_BASE_1M		(IO_AXI_BUS_PHY_BASE)
#define IO_AOBUS_BASE_1M		(IO_AOBUS_PHY_BASE)
#define IO_APB_BUS_BASE_2M		(IO_APB_BUS_PHY_BASE)
	#define IO_HDMITX_BASE_8K	(IO_APB_BUS_BASE_2M +  0x42000)
	#define IO_NAND_BASE_32K	(IO_APB_BUS_BASE_2M +  0x48000)
	#define IO_DOS_BUS_BASE_64K	(IO_APB_BUS_BASE_2M +  0x50000)
	#define IO_MALI_APB_BASE_256K	(IO_APB_BUS_BASE_2M +  0xC0000)
	#define IO_VPU_BASE_256K	(IO_APB_BUS_BASE_2M + 0x100000)


#define IO_SPIMEM_BASE_64M		(IO_SPIMEM_PHY_BASE)

/* original name for compile ok */
#define IO_PL310_BASE			(IO_PL310_PHY_BASE)
#define IO_PERIPH_BASE			(IO_A9_PERIPH_PHY_BASE)
#define IO_A9_PERIPH_BASE		(IO_A9_PERIPH_PHY_BASE)
#define IO_MMC_BASE			(IO_MMC_PHY_BASE)
#define IO_TVIN_BASE			(IO_TVIN_PHY_BASE)
#define IO_DEMOD_COMB_BASE		(IO_DEMOD_COMB_PHY_BASE)
#define IO_USB_COMB_BASE		(IO_USB_COMB_PHY_BASE)
#define IO_USB_A_BASE			(IO_USB_A_PHY_BASE)
#define IO_USB_B_BASE			(IO_USB_B_PHY_BASE)
#define IO_USB_C_BASE			(IO_USB_C_PHY_BASE)
#define IO_ETH_BASE			(IO_ETH_PHY_BASE)
#define IO_SRAM_BASE			(IO_SRAM_PHY_BASE)
#define IO_BOOTROM_BASE			(IO_BOOTROM_PHY_BASE)
#define IO_SECBUS_BASE			(IO_SECBUS_PHY_BASE)
#define IO_CBUS_BASE			(IO_CBUS_PHY_BASE)
#define IO_AXI_BUS_BASE			(IO_AXI_BUS_PHY_BASE)
#define IO_AOBUS_BASE			(IO_AOBUS_PHY_BASE)
#define IO_APB_BUS_BASE			(IO_APB_BUS_PHY_BASE)
#endif // CONFIG_VMSPLIT_2G


#ifdef CONFIG_VMSPLIT_1G
#error Unsupported Memory Split Type
#endif // CONFIG_VMSPLIT_1G


#define MESON_PERIPHS1_VIRT_BASE	(IO_AOBUS_BASE + 0x4c0)
#define MESON_PERIPHS1_PHYS_BASE	(IO_AOBUS_PHY_BASE + 0x4c0)


#define CBUS_REG_OFFSET(reg)		((reg) << 2)
#define CBUS_REG_ADDR(reg)		(IO_CBUS_BASE_2M + CBUS_REG_OFFSET(reg))

#define USB_REG_OFFSET(reg)		((reg) << 2)
#define USB_REG_ADDR(reg)		(IO_USB_COMB_BASE_8K + USB_REG_OFFSET(reg))

#define VCBUS_REG_ADDR(reg)		(IO_APB_BUS_BASE_2M + 0x100000 + CBUS_REG_OFFSET(reg))
#define DOS_REG_ADDR(reg)		(IO_DOS_BUS_BASE_64K + CBUS_REG_OFFSET(reg))

#define MMC_REG_ADDR(reg)		(IO_MMC_BASE_32K+ (reg))

#define AXI_REG_OFFSET(reg)		((reg) << 2)
#define AXI_REG_ADDR(reg)		(IO_AXI_BUS_BASE_1M + AXI_REG_OFFSET(reg))

#define APB_REG_OFFSET(reg)		(reg&0xfffff)
#define APB_REG_ADDR(reg)		(IO_APB_BUS_BASE_2M + APB_REG_OFFSET(reg))
#define APB_REG_ADDR_VALID(reg)		(((unsigned long)(reg) & 3) == 0)

#define DEMOD_REG_OFFSET(reg)		(reg&0xfffff)
#define DEMOD_REG_ADDR(reg)		(IO_DEMOD_COMB_BASE_8K + DEMOD_REG_OFFSET(reg))

#define HDMI_TX_REG_OFFSET(reg)		(reg&0xfffff)
#define HDMI_TX_REG_ADDR(reg)		(IO_HDMITX_BASE_8K + HDMI_TX_REG_OFFSET(reg))

#define AOBUS_REG_OFFSET(reg)		((reg))
#define AOBUS_REG_ADDR(reg)		(IO_AOBUS_BASE_1M+ AOBUS_REG_OFFSET(reg))

#define SECBUS_REG_OFFSET(reg)		((reg) <<2)
#define SECBUS_REG_ADDR(reg)		(IO_SECBUS_BASE_32K + SECBUS_REG_OFFSET(reg))
#define SECBUS2_REG_ADDR(reg)		(IO_SECBUS_BASE_32K + 0x4000 + SECBUS_REG_OFFSET(reg))
#define SECBUS3_REG_ADDR(reg)		(IO_SECBUS_BASE_32K + 0x6000 + SECBUS_REG_OFFSET(reg))

#define TVBUS_REG_OFFSET(reg)           ((reg))
#define TVBUS_REG_ADDR(reg)             (IO_TVIN_BASE_32K-0x8000+TVBUS_REG_OFFSET(reg))
void meson_map_default_io(void);

#endif

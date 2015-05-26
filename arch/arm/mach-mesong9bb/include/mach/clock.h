/*
 * arch/arm/mach-mesong9bb/include/mach/clock.h
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

#ifndef __MACH_MESONG9BB_CLOCK_H
#define __MACH_MESONG9BB_CLOCK_H

#include <linux/types.h>
#include <linux/list.h>

struct clk_ops {
	//return: 0:success  1:fail
	int (*clk_ratechange_before)(unsigned long newrate,void* privdata);
	//return: 0:success  1:fail
	int (*clk_ratechange_after)(unsigned long newrate,void* privdata,int failed);
	//return: 0:success  1:fail
	int (*clk_enable_before)(void* privdata);
	//return: 0:success  1:fail
	int (*clk_enable_after)(void* privdata,int failed);
	//return: 0:success  1:fail
	int (*clk_disable_before)(void* privdata);
	//return: 0:success  1:fail.
	int (*clk_disable_after)(void* privdata,int failed);
	void* privdata;
	struct clk_ops* next;
};

struct clk {
	#define CLK_RATE_UNKNOWN (-1)
	unsigned long rate;///0xffffffff(-1) means unknown

	unsigned long(*get_rate)(struct clk *);
	int (*set_rate)(struct clk *, unsigned long);
	long (*round_rate)(struct clk *, unsigned long);
	int (*enable)(struct clk *);///disable my self
	int (*disable)(struct clk *);///enable my self
	bool (*status)(struct clk *);
	int (*on_parent_changed)(struct clk *clk, int rate);
	int (*need_parent_changed)(struct clk *clk, int rate);
	int msr;
	unsigned long msr_mul;
	unsigned long msr_div;
	unsigned long min;
	unsigned long max;

	unsigned clk_gate_reg_adr;
	unsigned clk_gate_reg_mask;

	int  open_irq;

	struct list_head child;
	struct list_head sibling;
	struct clk * parent;
	struct clk_ops* clk_ops;
	void * priv;

	unsigned long old_rate;//Just for store old cpu freq for set_sys_pll()
};

int  clk_register(struct clk *clk, const char *parent);
void clk_unregister(struct clk *clk);
int  clk_measure(char index );

//return: 0:disabed. 1:enabled. 2:unknown
int  clk_get_status(struct clk * clk);

//return: 0:success  1: fail
int clk_ops_register(struct clk *clk, struct clk_ops *ops);
//return: 0:success  1: fail
int clk_ops_unregister(struct clk *clk, struct clk_ops *ops);


//M8 all pll controler use bit 29 as reset bit
#define M8_PLL_RESET(pll) aml_set_reg32_mask(pll,(1<<29));

//wait for pll lock
//must wait first (100us+) then polling lock bit to check
#define M8_PLL_WAIT_FOR_LOCK(pll) \
	do { \
		udelay(1000); \
	} while((aml_read_reg32(pll)&0x80000000)==0);

// @todo: G9BB PLL Definition
#define G9BB_PLL_CNTL_CST2 (0x814d3928)
#define G9BB_PLL_CNTL_CST3 (0x6b425012)
#define G9BB_PLL_CNTL_CST4 (0x110)

#define G9BB_PLL_CNTL_CST12 (0x04294000)
#define G9BB_PLL_CNTL_CST13 (0x026b4250)
#define G9BB_PLL_CNTL_CST14 (0x06278410)
#define G9BB_PLL_CNTL_CST15 (0x1e1)
#define G9BB_PLL_CNTL_CST16 (0xacac10ac)
#define G9BB_PLL_CNTL_CST17 (0x0108e000)


//DDR PLL
#define G9BB_DDR_PLL_CNTL_2 (G9BB_PLL_CNTL_CST2)
#define G9BB_DDR_PLL_CNTL_3 (G9BB_PLL_CNTL_CST3)
#define G9BB_DDR_PLL_CNTL_4 (G9BB_PLL_CNTL_CST4)

//SYS PLL
/* ROMBOOT Ref
#define G9BB_SYS_PLL_CNTL_2 (0x69c8c000)
#define G9BB_SYS_PLL_CNTL_3 (0x0a57c221)
#define G9BB_SYS_PLL_CNTL_4 (0x0001d407)
#define G9BB_SYS_PLL_CNTL_5 (0x00000870)
*/
/* V1.0
#define G9BB_SYS_PLL_CNTL_2 (0x59e8ce00)
#define G9BB_SYS_PLL_CNTL_3 (0xca4b8823)
#define G9BB_SYS_PLL_CNTL_4 (0x0286a027)
#define G9BB_SYS_PLL_CNTL_5 (0x00003800)
*/
#define G9BB_SYS_PLL_CNTL_2 (0x59C88000)
#define G9BB_SYS_PLL_CNTL_3 (0xCA45B823)
#define G9BB_SYS_PLL_CNTL_4 (0x0001D407)
#define G9BB_SYS_PLL_CNTL_5 (0x00000870)

//VIID PLL
#define G9BB_VIID_PLL_CNTL_2 (G9BB_PLL_CNTL_CST2)
#define G9BB_VIID_PLL_CNTL_3 (G9BB_PLL_CNTL_CST3)
#define G9BB_VIID_PLL_CNTL_4 (G9BB_PLL_CNTL_CST4)
//Wr(HHI_VIID_PLL_CNTL,  0x20242 );	 //0x1047


//VID PLL
#define G9BB_VID_PLL_CNTL_2 (G9BB_PLL_CNTL_CST2)
#define G9BB_VID_PLL_CNTL_3 (G9BB_PLL_CNTL_CST3)
#define G9BB_VID_PLL_CNTL_4 (G9BB_PLL_CNTL_CST4)
//Wr(HHI_VID_PLL_CNTL,  0xb0442 ); //0x109c

//FIXED PLL/Multi-phase PLL
#define G9BB_MPLL_CNTL     (0xc00009a9)
#define G9BB_MPLL_CNTL_2 (0xadc80000)
#define G9BB_MPLL_CNTL_3 (0x0a57ca21)
#define G9BB_MPLL_CNTL_4 (0x00010006)
#define G9BB_MPLL_CNTL_5 (0xa5500e1a)
#define G9BB_MPLL_CNTL_6 (0xf4454545)
#define G9BB_MPLL_CNTL_7 (0x00000000)
#define G9BB_MPLL_CNTL_8 (0x00000000)
#define G9BB_MPLL_CNTL_9 (0x00000000)

extern unsigned long mali_clock_gating_lock(void);
extern void mali_clock_gating_unlock(unsigned long flags);

#endif //__MACH_MESONG9BB_CLOCK_H

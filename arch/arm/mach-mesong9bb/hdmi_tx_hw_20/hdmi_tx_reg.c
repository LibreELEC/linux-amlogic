/*
 * Amlogic Meson HDMI Transmitter Driver
 * frame buffer driver-----------HDMI_TX
 * Copyright (C) 2010 Amlogic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <mach/am_regs.h>
#include "hdmi_tx_reg.h"

//static DEFINE_SPINLOCK(reg_lock);
static DEFINE_SPINLOCK(reg_lock2);

void hdmitx_wr_reg(unsigned int addr, unsigned int data)
{
    unsigned long offset = (addr >> 24);
    addr = addr & 0xffff;
    aml_write_reg32(P_HDMITX_ADDR_PORT + offset, addr);
    aml_write_reg32(P_HDMITX_ADDR_PORT + offset, addr);
    aml_write_reg32(P_HDMITX_DATA_PORT + offset, data);
    aml_write_reg32(P_HDMITX_DATA_PORT + offset, data);
} /* hdmitx_wr_reg */

void hdmitx_set_reg_bits(unsigned int addr, unsigned int value, unsigned int offset, unsigned int len)
{
    unsigned int data32 = 0;

    data32 = hdmitx_rd_reg(addr);
    data32 &= ~(((1 << len) - 1) << offset);
    data32 |= (value & ((1 << len) - 1)) << offset;
    hdmitx_wr_reg(addr, data32);
}

unsigned int hdmitx_rd_reg (unsigned int addr)
{
    unsigned int offset = (addr >> 24);
    addr = addr & 0xffff;
    aml_write_reg32(P_HDMITX_ADDR_PORT + offset, addr);
    aml_write_reg32(P_HDMITX_ADDR_PORT + offset, addr);

    return aml_read_reg32(P_HDMITX_DATA_PORT + offset);
} /* hdmitx_rd_reg */

void hdmitx_poll_reg(unsigned int addr, unsigned int val, unsigned long timeout)
{
    unsigned long time = 0;

    time = jiffies;
    while ((!(hdmitx_rd_reg(addr) & val)) && time_before(jiffies, time + timeout)) {
        msleep_interruptible(2);
    }
    if (time_after(jiffies, time + timeout))
        printk("poll hdmitx reg:0x%x  val:0x%x T1=%lu t=%lu T2=%lu timeout\n", addr, val, time, timeout, jiffies);
}
void hdmitx_rd_check_reg (unsigned int addr, unsigned int exp_data, unsigned int mask)
{
    unsigned long rd_data;
    rd_data = hdmitx_rd_reg(addr);
    if ((rd_data | mask) != (exp_data | mask)) {
        printk("HDMITX-DWC addr=0x%04x rd_data=0x%02x\n", (unsigned int)addr, (unsigned int)rd_data);
        printk("Error: HDMITX-DWC exp_data=0x%02x mask=0x%02x\n", (unsigned int)exp_data, (unsigned int)mask);
    }
}
#if 0
unsigned int hdmitx_rd_reg(unsigned int addr)
{
    unsigned int data;

    unsigned int flags, fiq_flag;

    spin_lock_irqsave(&reg_lock, flags);
    raw_local_save_flags(fiq_flag);
    local_fiq_disable();

    check_cts_hdmi_sys_clk_status();
    aml_write_reg32(P_HDMITX_DATA_PORT, addr);
    aml_write_reg32(P_HDMITX_DATA_PORT, addr);
    data = aml_read_reg32(P_HDMITX_DATA_PORT);

    raw_local_irq_restore(fiq_flag);
    spin_unlock_irqrestore(&reg_lock, flags);
    return (data);
}

void hdmitx_wr_reg(unsigned int addr, unsigned int data)
{
    unsigned int flags, fiq_flag;
    spin_lock_irqsave(&reg_lock, flags);
    raw_local_save_flags(fiq_flag);
    local_fiq_disable();

    check_cts_hdmi_sys_clk_status();
    aml_write_reg32(P_HDMITX_DATA_PORT, addr);
    aml_write_reg32(P_HDMITX_DATA_PORT, addr);
    aml_write_reg32(P_HDMITX_DATA_PORT, data);
    raw_local_irq_restore(fiq_flag);
    spin_unlock_irqrestore(&reg_lock, flags);
}
#endif
//#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8
#define waiting_aocec_free() \
        do {\
            unsigned long cnt = 0;\
            while (aml_read_reg32(P_AO_CEC_RW_REG) & (1<<23))\
            {\
                if (3500 == cnt++)\
                {\
                    break;\
                }\
            }\
        }while(0)

unsigned long aocec_rd_reg (unsigned long addr)
{
    unsigned long data32;
    unsigned long flags;

    data32  = 0;
    data32 |= 0     << 16;  // [16]     cec_reg_wr
    data32 |= 0     << 8;   // [15:8]   cec_reg_wrdata
    data32 |= addr  << 0;   // [7:0]    cec_reg_addr

    waiting_aocec_free();
    spin_lock_irqsave(&reg_lock2, flags);
    aml_write_reg32(P_AO_CEC_RW_REG, data32);

    waiting_aocec_free();
    data32 = ((aml_read_reg32(P_AO_CEC_RW_REG)) >> 24) & 0xff;
    spin_unlock_irqrestore(&reg_lock2, flags);
    return (data32);
} /* aocec_rd_reg */

void aocec_wr_reg (unsigned long addr, unsigned long data)
{
    unsigned long data32;
    unsigned long flags;
    waiting_aocec_free();
    spin_lock_irqsave(&reg_lock2, flags);
    data32  = 0;
    data32 |= 1     << 16;  // [16]     cec_reg_wr
    data32 |= data  << 8;   // [15:8]   cec_reg_wrdata
    data32 |= addr  << 0;   // [7:0]    cec_reg_addr
    aml_write_reg32(P_AO_CEC_RW_REG, data32);
    spin_unlock_irqrestore(&reg_lock2, flags);
    waiting_aocec_free();
} /* aocec_wr_only_reg */

//#endif

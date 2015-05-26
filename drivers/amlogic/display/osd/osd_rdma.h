#ifndef  _OSD_RDMA_H
#define _OSD_RDMA_H

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <mach/am_regs.h>
#include <mach/power_gate.h>

#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>
#include <linux/amlogic/vout/vout_notify.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/clk.h>
#include <linux/amlogic/logo/logo.h>

typedef  struct {
	u32  addr;
	u32  val;
} rdma_table_item_t;

//static DEFINE_SPINLOCK(rdma_lock);
#define TABLE_SIZE	 PAGE_SIZE
#define MAX_TABLE_ITEM	 (TABLE_SIZE/sizeof(rdma_table_item_t))
#define RDMA_CHANNEL_INDEX	3  //auto  1,2,3   manual is 0
#define START_ADDR		(P_RDMA_AHB_START_ADDR_MAN+(RDMA_CHANNEL_INDEX<<3))
#define END_ADDR		(P_RDMA_AHB_END_ADDR_MAN+(RDMA_CHANNEL_INDEX<<3))

#define OSD_RDMA_FLAG_REG	VIU_OSD2_TCOLOR_AG3
#define P_OSD_RDMA_FLAG_REG	P_VIU_OSD2_TCOLOR_AG3

#define OSD_RDMA_FLAG_DONE	(1<<0) //hw rdma change it to 1 when rdma complete, rdma isr chagne it to 0 to reset it.
#define OSD_RDMA_FLAG_REJECT	(1<<1) //hw rdma own this flag, change it to zero when start rdma,change it to 0 when complete
#define	OSD_RDMA_FLAG_DIRTY	(1<<2) //hw rdma change it to 0 , cpu write change it to 1

#define OSD_RDMA_FLAGS_ALL_ENABLE	(OSD_RDMA_FLAG_DONE|OSD_RDMA_FLAG_REJECT|OSD_RDMA_FLAG_DIRTY)

#define  OSD_RDMA_STATUS		(aml_read_reg32(P_OSD_RDMA_FLAG_REG)&(OSD_RDMA_FLAG_REJECT|OSD_RDMA_FLAG_DONE|OSD_RDMA_FLAG_DIRTY))
#define  OSD_RDMA_STATUS_IS_REJECT 	(aml_read_reg32(P_OSD_RDMA_FLAG_REG)&OSD_RDMA_FLAG_REJECT)
#define  OSD_RDMA_STAUS_IS_DIRTY	(aml_read_reg32(P_OSD_RDMA_FLAG_REG)&OSD_RDMA_FLAG_DIRTY)
#define  OSD_RDMA_STAUS_IS_DONE		(aml_read_reg32(P_OSD_RDMA_FLAG_REG)&OSD_RDMA_FLAG_DONE)

//hw rdma op, set DONE && clear DIRTY && clear REJECT
#define  OSD_RDMA_STATUS_MARK_COMPLETE	((aml_read_reg32(P_OSD_RDMA_FLAG_REG)&~OSD_RDMA_FLAGS_ALL_ENABLE)|(OSD_RDMA_FLAG_DONE))
//hw rdma op,set REJECT && set DIRTY.
#define  OSD_RDMA_STATUS_MARK_TBL_RST	((aml_read_reg32(P_OSD_RDMA_FLAG_REG)&~OSD_RDMA_FLAGS_ALL_ENABLE)|(OSD_RDMA_FLAG_REJECT|OSD_RDMA_FLAG_DIRTY))

//cpu op
#define  OSD_RDMA_STAUS_MARK_DIRTY	(aml_write_reg32(P_OSD_RDMA_FLAG_REG,aml_read_reg32(P_OSD_RDMA_FLAG_REG)|OSD_RDMA_FLAG_DIRTY))
//isr op
#define  OSD_RDMA_STAUS_CLEAR_DONE	(aml_write_reg32(P_OSD_RDMA_FLAG_REG,aml_read_reg32(P_OSD_RDMA_FLAG_REG)&~(OSD_RDMA_FLAG_DONE)))
//cpu reset op.
#define  OSD_RDMA_STATUS_CLEAR_ALL	(aml_write_reg32(P_OSD_RDMA_FLAG_REG,(aml_read_reg32(P_OSD_RDMA_FLAG_REG)&~OSD_RDMA_FLAGS_ALL_ENABLE)))

//time diagram:
/*
** CLEAR: from rdma hw op complete  to the next cpu write .
** REJECT: from rdma hw op start to rdma hw op complete.
** DONE:  from rdma hw op complete to the next osd RDMA ISR
*/

#endif

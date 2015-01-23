#ifndef   _OSD_HW_DEF_H
#define	_OSD_HW_DEF_H
#include <linux/amlogic/osd/osd_hw.h>
#include <linux/amlogic/amports/vframe_provider.h>
#include <linux/list.h>

/************************************************************************
**
**	macro  define  part
**
**************************************************************************/
#define MAX_BUF_NUM						3  /*fence relative*/
#define LEFT								0
#define RIGHT								1
#define OSD_RELATIVE_BITS				0x33370
#define HW_OSD_BLOCK_ENABLE_MASK		0x000F
#define HW_OSD_BLOCK_ENABLE_0			0x0001 /* osd blk0 enable */
#define HW_OSD_BLOCK_ENABLE_1			0x0002 /* osd blk1 enable */
#define HW_OSD_BLOCK_ENABLE_2			0x0004 /* osd blk2 enable */
#define HW_OSD_BLOCK_ENABLE_3			0x0008 /* osd blk3 enable */
#define HW_OSD_BLOCK_LAYOUT_MASK		0xFFFF0000
/*
 * osd block layout horizontal:
 * -------------
 * |     0     |
 * |-----------|
 * |     1     |
 * |-----------|
 * |     2     |
 * |-----------|
 * |     3     |
 * -------------
 */
#define HW_OSD_BLOCK_LAYOUT_HORIZONTAL 0x10000
/*
 * osd block layout vertical:
 * -------------
 * |  |  |  |  |
 * |  |  |  |  |
 * | 0| 1| 2| 3|
 * |  |  |  |  |
 * |  |  |  |  |
 * -------------
 *
 * NOTE:
 *     In this mode, just one of the OSD blocks can be enabled at the same time.
 *     Because the blocks must be sequenced in vertical display order if they
 *     want to be both enabled at the same time.
 */
#define HW_OSD_BLOCK_LAYOUT_VERTICAL 0x20000
/*
 * osd block layout grid:
 * -------------
 * |     |     |
 * |  0  |  1  |
 * |-----|-----|
 * |     |     |
 * |  2  |  3  |
 * -------------
 *
 * NOTE:
 *     In this mode, Block0 and Block1 cannot be enabled at the same time.
 *     Neither can Block2 and Block3.
 */
#define HW_OSD_BLOCK_LAYOUT_GRID 0x30000
/*
 * osd block layout customer, need setting block_windows
 */
#define HW_OSD_BLOCK_LAYOUT_CUSTOMER 0xFFFF0000

/************************************************************************
**
**	func declare  part
**
**************************************************************************/

static  void  osd2_update_color_mode(void);
static  void  osd2_update_enable(void);
static  void  osd2_update_color_key_enable(void);
static  void  osd2_update_color_key(void);
static  void  osd2_update_gbl_alpha(void);
static  void  osd2_update_order(void);
static  void  osd2_update_disp_geometry(void);
static void   osd2_update_coef(void);
static void   osd2_update_disp_freescale_enable(void);
static void   osd2_update_disp_osd_reverse(void);
static void   osd2_update_disp_osd_rotate(void);
static  void  osd2_update_disp_scale_enable(void);
static  void  osd2_update_disp_3d_mode(void);

static  void  osd1_update_color_mode(void);
static  void  osd1_update_enable(void);
static  void  osd1_update_color_key(void);
static  void  osd1_update_color_key_enable(void);
static  void  osd1_update_gbl_alpha(void);
static  void  osd1_update_order(void);
static  void  osd1_update_disp_geometry(void);
static void   osd1_update_coef(void);
static void   osd1_update_disp_freescale_enable(void);
static void   osd1_update_disp_osd_reverse(void);
static void   osd1_update_disp_osd_rotate(void);
static  void  osd1_update_disp_scale_enable(void);
static  void  osd1_update_disp_3d_mode(void);


/************************************************************************
**
**	global varible  define  part
**
**************************************************************************/
LIST_HEAD(update_list);
static DEFINE_SPINLOCK(osd_lock);
hw_para_t osd_hw;
static unsigned long 	lock_flags;
#ifdef FIQ_VSYNC
static unsigned long	fiq_flag;
#endif
static vframe_t vf;
static update_func_t hw_func_array[HW_OSD_COUNT][HW_REG_INDEX_MAX]={
	{
		osd1_update_color_mode,
		osd1_update_enable,
		osd1_update_color_key,
		osd1_update_color_key_enable,
		osd1_update_gbl_alpha,
		osd1_update_order,
		osd1_update_coef,
		osd1_update_disp_geometry,
		osd1_update_disp_scale_enable,
		osd1_update_disp_freescale_enable,
		osd1_update_disp_osd_reverse,
		osd1_update_disp_osd_rotate,
	},
	{
		osd2_update_color_mode,
		osd2_update_enable,
		osd2_update_color_key,
		osd2_update_color_key_enable,
		osd2_update_gbl_alpha,
		osd2_update_order,
		osd2_update_coef,
		osd2_update_disp_geometry,
		osd2_update_disp_scale_enable,
		osd2_update_disp_freescale_enable,
		osd2_update_disp_osd_reverse,
		osd2_update_disp_osd_rotate,
	},
};

#ifdef CONFIG_VSYNC_RDMA
#ifdef FIQ_VSYNC
#define add_to_update_list(osd_idx,cmd_idx) \
	spin_lock_irqsave(&osd_lock, lock_flags); \
	raw_local_save_flags(fiq_flag); \
	local_fiq_disable(); \
	osd_hw.reg[osd_idx][cmd_idx].update_func(); \
	raw_local_irq_restore(fiq_flag); \
	spin_unlock_irqrestore(&osd_lock, lock_flags);
#else
#define add_to_update_list(osd_idx,cmd_idx) \
	spin_lock_irqsave(&osd_lock, lock_flags); \
	osd_hw.reg[osd_idx][cmd_idx].update_func(); \
	spin_unlock_irqrestore(&osd_lock, lock_flags);
#endif
#else
#ifdef FIQ_VSYNC
#define add_to_update_list(osd_idx,cmd_idx) \
	spin_lock_irqsave(&osd_lock, lock_flags); \
	raw_local_save_flags(fiq_flag); \
	local_fiq_disable(); \
	osd_hw.updated[osd_idx]|=(1<<cmd_idx); \
	raw_local_irq_restore(fiq_flag); \
	spin_unlock_irqrestore(&osd_lock, lock_flags);
#else
#define add_to_update_list(osd_idx,cmd_idx) \
	spin_lock_irqsave(&osd_lock, lock_flags); \
	osd_hw.updated[osd_idx]|=(1<<cmd_idx); \
	spin_unlock_irqrestore(&osd_lock, lock_flags);
#endif
#endif

#ifdef CONFIG_VSYNC_RDMA
#define remove_from_update_list(osd_idx,cmd_idx) \
	osd_hw.updated[osd_idx]&=~(1<<cmd_idx);
#else
#define remove_from_update_list(osd_idx,cmd_idx)
#endif

#endif

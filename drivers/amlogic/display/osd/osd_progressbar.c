#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/of_fdt.h>
#include <linux/console.h>
#include <mach/am_regs.h>
#include <linux/amlogic/osd/osd_dev.h>
#include <linux/amlogic/amlog.h>
#include <linux/amlogic/logo/logo_dev.h>
#include <linux/amlogic/logo/logo_dev_osd.h>
#include <linux/amlogic/logo/dev_ge2d.h>
#include <mach/mod_gate.h>

extern myfb_dev_t  *gp_fbdev_list[OSD_COUNT];
extern int osd_blank(int blank_mode, struct fb_info *info);

typedef struct{
	ge2d_context_t  *ge2d_context;
	const vinfo_t *vinfo;
	src_dst_info_t  op_info;
	u32 bar_border;
	u32 bar_width;
	u32 bar_height;
} osd_progress_bar_t;

static osd_progress_bar_t progress_bar;

static int init_fb1_first (const vinfo_t *vinfo)
{
	struct osd_ctl_s  osd_ctl;
	const color_bit_define_t  *color;
	u32 reg = 0, data32 = 0;
	int ret = 0;

	ret = find_reserve_block("mesonfb", 1);
	if (ret < 0) {
		amlog_level(LOG_LEVEL_HIGH,
			"can not find mesonfb1 reserve block\n");
		return -EFAULT;
	}

	osd_ctl.addr = (phys_addr_t)get_reserve_block_addr(ret);

	osd_ctl.index = 1; //fb1
	color=&default_color_format_array[31];

	osd_ctl.xres=vinfo->width;
	osd_ctl.yres=vinfo->height;
	osd_ctl.xres_virtual=osd_ctl.xres;
	osd_ctl.yres_virtual=osd_ctl.yres;
	osd_ctl.disp_start_x=0;
	osd_ctl.disp_end_x=osd_ctl.xres -1;
	osd_ctl.disp_start_y=0;
	osd_ctl.disp_end_y=osd_ctl.yres-1;

	reg = osd_ctl.index==0?P_VIU_OSD1_BLK0_CFG_W0:P_VIU_OSD2_BLK0_CFG_W0;
	data32 = aml_read_reg32(reg)&(~(0xf<<8));
	data32 |=  color->hw_blkmode<< 8; /* osd_blk_mode */
	aml_write_reg32(reg, data32);

	amlog_level(LOG_LEVEL_HIGH, "osd_ctl.addr is 0x%08x, osd_ctl.xres is %d,"
		" osd_ctl.yres is %d\n", osd_ctl.addr,
			osd_ctl.xres, osd_ctl.yres);
	osd_setup(&osd_ctl, \
		0, \
		0, \
		osd_ctl.xres, \
		osd_ctl.yres, \
		osd_ctl.xres_virtual, \
		osd_ctl.yres_virtual, \
		osd_ctl.disp_start_x, \
		osd_ctl.disp_start_y, \
		osd_ctl.disp_end_x, \
		osd_ctl.disp_end_y, \
		osd_ctl.addr, \
		color, \
		osd_ctl.index);

	return SUCCESS;
}

int osd_show_progress_bar(u32 percent) {
	static u32 progress= 0;
	u32 step =1;
	//wait_queue_head_t  wait_head;
	myfb_dev_t *fb_dev;
	ge2d_context_t  *context = progress_bar.ge2d_context;
	src_dst_info_t  *op_info = &progress_bar.op_info;

	if (NULL == context) {
		//osd_init_progress_bar();
		printk("context is NULL\n");
		return -1;
	}
	if (NULL == (fb_dev = gp_fbdev_list[1])) {
		amlog_level(LOG_LEVEL_HIGH, "fb1 should exit!!!");
		return -EFAULT;
	}
	switch_mod_gate_by_name("ge2d", 1);
	//init_waitqueue_head(&wait_head);

	while (progress < percent) {
		/*
		printk("progress is %d, x: [%d], y: [%d], w: [%d], h: [%d]\n",
			progress, op_info->dst_rect.x, op_info->dst_rect.y,
			op_info->dst_rect.w, op_info->dst_rect.h);
		*/

		dev_ge2d_cmd(context,CMD_FILLRECT,op_info);
		//wait_event_interruptible_timeout(wait_head,0,4);
		progress+=step;
		op_info->dst_rect.x+=op_info->dst_rect.w;
		op_info->color-=(0xff*step/100)<<16; //color change smoothly.
	}
	switch_mod_gate_by_name("ge2d", 0);
	if (100 == percent) {
		progress = 0;
		//console_lock();
		//osd_blank(1,fb_dev->fb_info);
		//console_unlock();
	}
	return SUCCESS;
}
EXPORT_SYMBOL(osd_show_progress_bar);

int osd_init_progress_bar(void) {
	src_dst_info_t  *op_info = &progress_bar.op_info;
	const vinfo_t *vinfo = progress_bar.vinfo;
	config_para_t	ge2d_config;
	myfb_dev_t *fb_dev;
	u32 step =1;

	memset(&progress_bar, 0, sizeof(osd_progress_bar_t));

	vinfo = get_current_vinfo();
	/*
	pr_info("vinfo->name is %s, vinfo->mode is %d, vinfo->width: [%d],"
		" vinfo->height: [%d], vinfo->screen_real_width: [%d],"
		" vinfo->screen_real_height: [%d]\n",
		vinfo->name, vinfo->mode, vinfo->width, vinfo->height,
		vinfo->screen_real_width, vinfo->screen_real_height);
	*/
	progress_bar.bar_border =
		(((vinfo->field_height?vinfo->field_height:vinfo->height)*4/720)>>2)<<2;
	progress_bar.bar_width =
		(((vinfo->width*200/1280)>>2)<<2) + progress_bar.bar_border;
	progress_bar.bar_height =
		(((vinfo->field_height?vinfo->field_height:vinfo->height)*32/720)>>2)<<2;

	if (SUCCESS == init_fb1_first(vinfo)) {
		if (NULL == (fb_dev = gp_fbdev_list[1])) {
			amlog_level(LOG_LEVEL_HIGH, "fb1 should exit!!!");
			return -EFAULT;
		}

		switch_mod_gate_by_name("ge2d", 1);
		ge2d_config.src_dst_type = OSD1_OSD1;
		ge2d_config.alu_const_color = 0x000000ff;
		progress_bar.ge2d_context = dev_ge2d_setup(&ge2d_config);
		if (NULL == progress_bar.ge2d_context) {
			pr_info("ge2d_context is NULL!!!!!!\n");
			return -OUTPUT_DEV_SETUP_FAIL;
		}
		amlog_level(LOG_LEVEL_HIGH, "progress bar setup ge2d device OK\n");
		//clear dst rect
		op_info->color = 0x000000bf;
		op_info->dst_rect.x = 0;
		op_info->dst_rect.y = 0;
		op_info->dst_rect.w = vinfo->width;
		op_info->dst_rect.h =
			vinfo->field_height?vinfo->field_height:vinfo->height;
		dev_ge2d_cmd(progress_bar.ge2d_context,CMD_FILLRECT,op_info);
		/*
		pr_info("clear dst:%d-%d-%d-%d\n",
			op_info->dst_rect.x,op_info->dst_rect.y,
			op_info->dst_rect.w,op_info->dst_rect.h);
		*/
		// show fb1
		console_lock();
		osd_blank(0,fb_dev->fb_info);
		console_unlock();
		op_info->color=0x555555ff;
		op_info->dst_rect.x =
			(vinfo->width/2)-progress_bar.bar_width;
		op_info->dst_rect.y =
			((vinfo->field_height?vinfo->field_height:vinfo->height)*9)/10;
		op_info->dst_rect.w = progress_bar.bar_width*2;
		op_info->dst_rect.h = progress_bar.bar_height;
		/*
		pr_info("fill==dst:%d-%d-%d-%d\n",
			op_info->dst_rect.x,op_info->dst_rect.y,
			op_info->dst_rect.w,op_info->dst_rect.h);
		*/
		dev_ge2d_cmd(progress_bar.ge2d_context,CMD_FILLRECT,op_info);
		switch_mod_gate_by_name("ge2d", 0);
	} else {
		amlog_level(LOG_LEVEL_HIGH, "fb1 init failed, exit!!!");
		return -EFAULT;
	}

	// initial op info before draw actrualy
	op_info->dst_rect.x += progress_bar.bar_border;
	op_info->dst_rect.y += progress_bar.bar_border;
	op_info->dst_rect.w =
		(progress_bar.bar_width-progress_bar.bar_border)*2*step/100;
	op_info->dst_rect.h =
		progress_bar.bar_height-progress_bar.bar_border*2;
	op_info->color = 0xffffff;

	return SUCCESS;
}
EXPORT_SYMBOL(osd_init_progress_bar);


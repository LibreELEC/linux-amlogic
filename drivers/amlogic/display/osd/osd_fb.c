/*
 * drivers/amlogic/display/osd/osd_fb.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
*/


/* Linux Headers */
#include <linux/version.h>
#include <linux/compat.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/console.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <meson_ion.h>
/* Amlogic Headers */
#include <linux/amlogic/vout/vout_notify.h>
#include <linux/amlogic/instaboot/instaboot.h>
#include <sw_sync.h>
#include <sync.h>

/* Local Headers */
#include "osd.h"
#include "osd_fb.h"
#include "osd_hw.h"
#include "osd_log.h"
#include "osd_sync.h"


static __u32 var_screeninfo[5];

struct osd_info_s osd_info = {
	.index = 0,
	.osd_reverse = 0,
};

const struct color_bit_define_s default_color_format_array[] = {
	INVALID_BPP_ITEM,
	INVALID_BPP_ITEM,
	{
		COLOR_INDEX_02_PAL4, 0, 0,
		0, 2, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0,
		FB_VISUAL_PSEUDOCOLOR, 2,
	},
	INVALID_BPP_ITEM,
	{
		COLOR_INDEX_04_PAL16, 0, 1,
		0, 4, 0, 0, 4, 0, 0, 4, 0, 0, 0, 0,
		FB_VISUAL_PSEUDOCOLOR, 4,
	},
	INVALID_BPP_ITEM,
	INVALID_BPP_ITEM,
	INVALID_BPP_ITEM,
	{
		COLOR_INDEX_08_PAL256, 0, 2,
		0, 8, 0, 0, 8, 0, 0, 8, 0, 0, 0, 0,
		FB_VISUAL_PSEUDOCOLOR, 8,
	},
	/*16 bit color*/
	{
		COLOR_INDEX_16_655, 0, 4,
		10, 6, 0, 5, 5, 0, 0, 5, 0, 0, 0, 0,
		FB_VISUAL_TRUECOLOR, 16
	},
	{
		COLOR_INDEX_16_844, 1, 4,
		8, 8, 0, 4, 4, 0, 0, 4, 0, 0, 0, 0,
		FB_VISUAL_TRUECOLOR, 16
	},
	{
		COLOR_INDEX_16_6442, 2, 4,
		10, 6, 0, 6, 4, 0, 2, 4, 0, 0, 2, 0,
		FB_VISUAL_TRUECOLOR, 16
	},
	{
		COLOR_INDEX_16_4444_R, 3, 4,
		12, 4, 0, 8, 4, 0, 4, 4, 0, 0, 4, 0,
		FB_VISUAL_TRUECOLOR, 16
	},
	{
		COLOR_INDEX_16_4642_R, 7, 4,
		12, 4, 0, 6, 6, 0, 2, 4, 0, 0, 2, 0,
		FB_VISUAL_TRUECOLOR, 16
	},
	{
		COLOR_INDEX_16_1555_A, 6, 4,
		10, 5, 0, 5, 5, 0, 0, 5, 0, 15, 1, 0,
		FB_VISUAL_TRUECOLOR, 16
	},
	{
		COLOR_INDEX_16_4444_A, 5, 4,
		8, 4, 0, 4, 4, 0, 0, 4, 0, 12, 4, 0,
		FB_VISUAL_TRUECOLOR, 16
	},
	{
		COLOR_INDEX_16_565, 4, 4,
		11, 5, 0, 5, 6, 0, 0, 5, 0, 0, 0, 0,
		FB_VISUAL_TRUECOLOR, 16
	},
	/*24 bit color*/
	INVALID_BPP_ITEM,
	INVALID_BPP_ITEM,
	{
		COLOR_INDEX_24_6666_A, 4, 7,
		12, 6, 0, 6, 6, 0, 0, 6, 0, 18, 6, 0,
		FB_VISUAL_TRUECOLOR, 24
	},
	{
		COLOR_INDEX_24_6666_R, 3, 7,
		18, 6, 0, 12, 6, 0, 6, 6, 0, 0, 6, 0,
		FB_VISUAL_TRUECOLOR, 24
	},
	{
		COLOR_INDEX_24_8565, 2, 7,
		11, 5, 0, 5, 6, 0, 0, 5, 0, 16, 8, 0,
		FB_VISUAL_TRUECOLOR, 24
	},
	{
		COLOR_INDEX_24_5658, 1, 7,
		19, 5, 0, 13, 6, 0, 8, 5, 0, 0, 8, 0,
		FB_VISUAL_TRUECOLOR, 24
	},
	{
		COLOR_INDEX_24_888_B, 5, 7,
		0, 8, 0, 8, 8, 0, 16, 8, 0, 0, 0, 0,
		FB_VISUAL_TRUECOLOR, 24
	},
	{
		COLOR_INDEX_24_RGB, 0, 7,
		16, 8, 0, 8, 8, 0, 0, 8, 0, 0, 0, 0,
		FB_VISUAL_TRUECOLOR, 24
	},
	/*32 bit color RGBX */
	{
		COLOR_INDEX_32_BGRX, 3, 5,
		8, 8, 0, 16, 8, 0, 24, 8, 0, 0, 0, 0,
		FB_VISUAL_TRUECOLOR, 32
	},
	{
		COLOR_INDEX_32_XBGR, 2, 5,
		0, 8, 0, 8, 8, 0, 16, 8, 0, 24, 0, 0,
		FB_VISUAL_TRUECOLOR, 32
	},
	{
		COLOR_INDEX_32_RGBX, 0, 5,
		24, 8, 0, 16, 8, 0, 8, 8, 0, 0, 0, 0,
		FB_VISUAL_TRUECOLOR, 32
	},
	{
		COLOR_INDEX_32_XRGB, 1, 5,
		16, 8, 0, 8, 8, 0, 0, 8, 0, 24, 0, 0,
		FB_VISUAL_TRUECOLOR, 32
	},
	/*32 bit color RGBA */
	{
		COLOR_INDEX_32_BGRA, 3, 5,
		8, 8, 0, 16, 8, 0, 24, 8, 0, 0, 8, 0,
		FB_VISUAL_TRUECOLOR, 32
	},
	{
		COLOR_INDEX_32_ABGR, 2, 5,
		0, 8, 0, 8, 8, 0, 16, 8, 0, 24, 8, 0,
		FB_VISUAL_TRUECOLOR, 32
	},
	{
		COLOR_INDEX_32_RGBA, 0, 5,
		24, 8, 0, 16, 8, 0, 8, 8, 0, 0, 8, 0,
		FB_VISUAL_TRUECOLOR, 32
	},
	{
		COLOR_INDEX_32_ARGB, 1, 5,
		16, 8, 0, 8, 8, 0, 0, 8, 0, 24, 8, 0,
		FB_VISUAL_TRUECOLOR, 32
	},
	/*YUV color*/
	{COLOR_INDEX_YUV_422, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16},
};

static struct fb_var_screeninfo fb_def_var[] = {
	{
		.xres            = 1920,
		.yres            = 1080,
		.xres_virtual    = 1920,
		.yres_virtual    = 2160,
		.xoffset         = 0,
		.yoffset         = 0,
		.bits_per_pixel = 32,
		.grayscale       = 0,
		.red             = {0, 0, 0},
		.green           = {0, 0, 0},
		.blue            = {0, 0, 0},
		.transp          = {0, 0, 0},
		.nonstd          = 0,
		.activate        = FB_ACTIVATE_NOW,
		.height          = -1,
		.width           = -1,
		.accel_flags     = 0,
		.pixclock        = 0,
		.left_margin     = 0,
		.right_margin    = 0,
		.upper_margin    = 0,
		.lower_margin    = 0,
		.hsync_len       = 0,
		.vsync_len       = 0,
		.sync            = 0,
		.vmode           = FB_VMODE_NONINTERLACED,
		.rotate          = 0,
	}
#ifdef CONFIG_FB_OSD2_ENABLE
	,
	{
		.xres            = 32,
		.yres            = 32,
		.xres_virtual    = 32,
		.yres_virtual    = 32,
		.xoffset         = 0,
		.yoffset         = 0,
		.bits_per_pixel = 32,
		.grayscale       = 0,
		/* leave as it is ,set by system. */
		.red             = {0, 0, 0},
		.green           = {0, 0, 0},
		.blue            = {0, 0, 0},
		.transp          = {0, 0, 0},
		.nonstd          = 0,
		.activate        = FB_ACTIVATE_NOW,
		.height          = -1,
		.width           = -1,
		.accel_flags     = 0,
		.pixclock        = 0,
		.left_margin     = 0,
		.right_margin    = 0,
		.upper_margin    = 0,
		.lower_margin    = 0,
		.hsync_len       = 0,
		.vsync_len       = 0,
		.sync            = 0,
		.vmode           = FB_VMODE_NONINTERLACED,
		.rotate          = 0,
	}
#endif
};

static struct fb_fix_screeninfo fb_def_fix = {
	.id         = "OSD FB",
	.xpanstep   = 1,
	.ypanstep   = 1,
	.type       = FB_TYPE_PACKED_PIXELS,
	.visual     = FB_VISUAL_TRUECOLOR,
	.accel      = FB_ACCEL_NONE,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static struct early_suspend early_suspend;
static int early_suspend_flag;
#endif
#ifdef CONFIG_SCREEN_ON_EARLY
static int early_resume_flag;
#endif

static int osd_shutdown_flag;

unsigned int osd_log_level;
int int_viu_vsync = -ENXIO;
#ifdef CONFIG_FB_OSD_VSYNC_RDMA
int int_rdma = INT_RDMA;
#endif
struct osd_fb_dev_s *gp_fbdev_list[OSD_COUNT] = {};
static struct reserved_mem fb_rmem = {.base = 0, .size = 0};
static u32 fb_memsize[2];
static phys_addr_t fb_rmem_paddr[2];
static void __iomem *fb_rmem_vaddr[OSD_COUNT];
static size_t fb_rmem_size[OSD_COUNT];
static phys_addr_t fb_rmem_afbc_paddr[OSD_COUNT][OSD_MAX_BUF_NUM];
static void __iomem *fb_rmem_afbc_vaddr[OSD_COUNT][OSD_MAX_BUF_NUM];
static size_t fb_rmem_afbc_size[OSD_COUNT][OSD_MAX_BUF_NUM];

struct ion_client *fb_ion_client = NULL;
struct ion_handle *fb_ion_handle[OSD_COUNT][OSD_MAX_BUF_NUM] = {
	{NULL, NULL, NULL}, {NULL, NULL, NULL}
};

phys_addr_t get_fb_rmem_paddr(int index)
{
	if (index < 0 || index > 1)
		return 0;
	return fb_rmem_paddr[index];
}

void __iomem *get_fb_rmem_vaddr(int index)
{
	if (index < 0 || index > 1)
		return 0;
	return fb_rmem_vaddr[index];
}

size_t get_fb_rmem_size(int index)
{
	if (index < 0 || index > 1)
		return 0;
	return fb_rmem_size[index];
}


static void osddev_setup(struct osd_fb_dev_s *fbdev)
{
	mutex_lock(&fbdev->lock);
	osd_setup_hw(fbdev->fb_info->node,
		     &fbdev->osd_ctl,
		     fbdev->fb_info->var.xoffset,
		     fbdev->fb_info->var.yoffset,
		     fbdev->fb_info->var.xres,
		     fbdev->fb_info->var.yres,
		     fbdev->fb_info->var.xres_virtual,
		     fbdev->fb_info->var.yres_virtual,
		     fbdev->osd_ctl.disp_start_x,
		     fbdev->osd_ctl.disp_start_y,
		     fbdev->osd_ctl.disp_end_x,
		     fbdev->osd_ctl.disp_end_y,
		     fbdev->fb_mem_paddr, /*phys_addr_t -> u32*/
		     fbdev->fb_mem_afbc_paddr, /* afbc phys_addr_t* */
		     fbdev->color);
	mutex_unlock(&fbdev->lock);

	return;
}

static void osddev_update_disp_axis(struct osd_fb_dev_s *fbdev,
					int mode_change)
{
	osd_update_disp_axis_hw(fbdev->fb_info->node,
				fbdev->osd_ctl.disp_start_x,
				fbdev->osd_ctl.disp_end_x,
				fbdev->osd_ctl.disp_start_y,
				fbdev->osd_ctl.disp_end_y,
				fbdev->fb_info->var.xoffset,
				fbdev->fb_info->var.yoffset,
				mode_change);
}

static int osddev_setcolreg(unsigned regno, u16 red, u16 green, u16 blue,
			    u16 transp, struct osd_fb_dev_s *fbdev)
{
	struct fb_info *info = fbdev->fb_info;
	if ((fbdev->color->color_index == COLOR_INDEX_02_PAL4) ||
	    (fbdev->color->color_index == COLOR_INDEX_04_PAL16) ||
	    (fbdev->color->color_index == COLOR_INDEX_08_PAL256)) {
		mutex_lock(&fbdev->lock);
		osd_setpal_hw(fbdev->fb_info->node, regno, red, green,
				blue, transp);
		mutex_unlock(&fbdev->lock);
	}
	if (info->fix.visual == FB_VISUAL_TRUECOLOR) {
		u32 v, r, g, b, a;
		if (regno >= 16)
			return 1;
		r = red    >> (16 - info->var.red.length);
		g = green  >> (16 - info->var.green.length);
		b = blue   >> (16 - info->var.blue.length);
		a = transp >> (16 - info->var.transp.length);
		v = (r << info->var.red.offset)   |
		    (g << info->var.green.offset) |
		    (b << info->var.blue.offset)  |
		    (a << info->var.transp.offset);
		((u32 *)(info->pseudo_palette))[regno] = v;
	}
	return 0;
}

static const struct color_bit_define_s *
_find_color_format(struct fb_var_screeninfo *var)
{
	u32 upper_margin, lower_margin, i, level;
	const struct color_bit_define_s *found = NULL;
	const struct color_bit_define_s *color = NULL;

	level = (var->bits_per_pixel - 1) / 8;
	switch (level) {
	case 0:
		upper_margin = COLOR_INDEX_08_PAL256;
		lower_margin = COLOR_INDEX_02_PAL4;
		break;
	case 1:
		upper_margin = COLOR_INDEX_16_565;
		lower_margin = COLOR_INDEX_16_655;
		break;
	case 2:
		upper_margin = COLOR_INDEX_24_RGB;
		lower_margin = COLOR_INDEX_24_6666_A;
		break;
	case 3:
		if ((var->nonstd != 0)
		    && (var->transp.length == 0)) {
			/* RGBX Mode */
			upper_margin = COLOR_INDEX_32_XRGB;
			lower_margin = COLOR_INDEX_32_BGRX;
		} else {
			upper_margin = COLOR_INDEX_32_ARGB;
			lower_margin = COLOR_INDEX_32_BGRA;
		}
		break;
	case 4:
		upper_margin = COLOR_INDEX_YUV_422;
		lower_margin = COLOR_INDEX_YUV_422;
		break;
	default:
		osd_log_err("unsupported color format.");
		return NULL;
	}
	/*
	 * if not provide color component length
	 * then we find the first depth match.
	 */

	if ((var->nonstd != 0) && (level == 3)
	    && (var->transp.length == 0)) {
		/* RGBX Mode */
		for (i = upper_margin; i >= lower_margin; i--) {
			color = &default_color_format_array[i];
			if ((color->red_length == var->red.length) &&
			    (color->green_length == var->green.length) &&
			    (color->blue_length == var->blue.length) &&
			    (color->transp_offset == var->transp.offset) &&
			    (color->green_offset == var->green.offset) &&
			    (color->blue_offset == var->blue.offset) &&
			    (color->red_offset == var->red.offset)) {
				found = color;
				break;
			}
			color--;
		}
	} else if ((var->red.length == 0) || (var->green.length == 0)
	    || (var->blue.length == 0) ||
	    var->bits_per_pixel != (var->red.length + var->green.length +
		    var->blue.length + var->transp.length)) {
		osd_log_dbg("not provide color length, use default color\n");
		found = &default_color_format_array[upper_margin];
	} else {
		for (i = upper_margin; i >= lower_margin; i--) {
			color = &default_color_format_array[i];
			if ((color->red_length == var->red.length) &&
			    (color->green_length == var->green.length) &&
			    (color->blue_length == var->blue.length) &&
			    (color->transp_length == var->transp.length) &&
			    (color->transp_offset == var->transp.offset) &&
			    (color->green_offset == var->green.offset) &&
			    (color->blue_offset == var->blue.offset) &&
			    (color->red_offset == var->red.offset)) {
				found = color;
				break;
			}
			color--;
		}
	}
	return found;
}

static void __init _fbdev_set_default(struct osd_fb_dev_s *fbdev, int index)
{
	/* setup default value */
	fbdev->fb_info->var = fb_def_var[index];
	fbdev->fb_info->fix = fb_def_fix;
	fbdev->color = _find_color_format(&fbdev->fb_info->var);
}

static int osd_check_fbsize(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)info->par;

	if (fbdev->fb_index == 0 && osd_get_afbc()) {
		/*fb_afbc_len = fbdev->fb_afbc_len[0] * ((info->node == 0) ?
				OSD_MAX_BUF_NUM : 1);*/
		if (var->xres_virtual * var->yres_virtual *
				var->bits_per_pixel / 8 >
				fbdev->fb_afbc_len[0] * OSD_MAX_BUF_NUM) {
			osd_log_err("afbc no enough memory for %d*%d*%d\n",
					var->xres,
					var->yres,
					var->bits_per_pixel);
			return  -ENOMEM;
		}
	} else {
		if (var->xres_virtual * var->yres_virtual *
				var->bits_per_pixel / 8 > fbdev->fb_len) {
			osd_log_err("no enough memory for %d*%d*%d\n",
					var->xres,
					var->yres,
					var->bits_per_pixel);
			return  -ENOMEM;
		}
	}
	return 0;
}
static int osd_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct fb_fix_screeninfo *fix;
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)info->par;
	const struct color_bit_define_s *color_format_pt;

	fix = &info->fix;
	color_format_pt = _find_color_format(var);
	if (color_format_pt == NULL || color_format_pt->color_index == 0)
		return -EFAULT;

	osd_log_dbg("select color format :index %d, bpp %d\n",
		    color_format_pt->color_index,
		    color_format_pt->bpp);
	fbdev->color = color_format_pt;
	var->red.offset = color_format_pt->red_offset;
	var->red.length = color_format_pt->red_length;
	var->red.msb_right = color_format_pt->red_msb_right;
	var->green.offset  = color_format_pt->green_offset;
	var->green.length  = color_format_pt->green_length;
	var->green.msb_right = color_format_pt->green_msb_right;
	var->blue.offset   = color_format_pt->blue_offset;
	var->blue.length   = color_format_pt->blue_length;
	var->blue.msb_right = color_format_pt->blue_msb_right;
	var->transp.offset = color_format_pt->transp_offset;
	var->transp.length = color_format_pt->transp_length;
	var->transp.msb_right = color_format_pt->transp_msb_right;
	var->bits_per_pixel = color_format_pt->bpp;
	osd_log_dbg("rgba(L/O):%d/%d-%d/%d-%d/%d-%d/%d\n",
		    var->red.length, var->red.offset,
		    var->green.length, var->green.offset,
		    var->blue.length, var->blue.offset,
		    var->transp.length, var->transp.offset);
	fix->visual = color_format_pt->color_type;
	/* adjust memory length. */
	fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
	osd_log_dbg("xvirtual=%d, bpp:%d, line_length=%d\n",
		var->xres_virtual, var->bits_per_pixel, fix->line_length);

	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;
	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;
	var->left_margin = var->right_margin = 0;
	var->upper_margin = var->lower_margin = 0;
	if (var->xres + var->xoffset > var->xres_virtual)
		var->xoffset = var->xres_virtual - var->xres;
	if (var->yres + var->yoffset > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;
	return 0;
}

static int osd_set_par(struct fb_info *info)
{
	const struct vinfo_s *vinfo;
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)info->par;
	struct osd_ctl_s *osd_ctrl = &fbdev->osd_ctl;
	u32 virt_end_x, virt_end_y;

	vinfo = get_current_vinfo();
	if (!vinfo) {
		osd_log_err("current vinfo NULL\n");
		return -1;
	}
	virt_end_x = osd_ctrl->disp_start_x + info->var.xres;
	virt_end_y = osd_ctrl->disp_start_y + info->var.yres;
	if (virt_end_x > vinfo->width)
		osd_ctrl->disp_end_x = vinfo->width - 1;
	else
		osd_ctrl->disp_end_x = virt_end_x - 1;
	if (virt_end_y  > vinfo->height)
		osd_ctrl->disp_end_y = vinfo->height - 1;
	else
		osd_ctrl->disp_end_y = virt_end_y - 1;
	osddev_setup((struct osd_fb_dev_s *)info->par);

	return 0;
}

static int
osd_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue,
	      unsigned transp, struct fb_info *info)
{
	return osddev_setcolreg(regno, red, green, blue,
				transp, (struct osd_fb_dev_s *)info->par);
}

static int
osd_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	int count, index, r;
	u16 *red, *green, *blue, *transp;
	u16 trans = 0xffff;
	red     = cmap->red;
	green   = cmap->green;
	blue    = cmap->blue;
	transp  = cmap->transp;
	index   = cmap->start;
	for (count = 0; count < cmap->len; count++) {
		if (transp)
			trans = *transp++;
		r = osddev_setcolreg(index++, *red++, *green++, *blue++, trans,
				     (struct osd_fb_dev_s *)info->par);
		if (r != 0)
			return r;
	}
	return 0;
}

static int osd_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)info->par;
	void __user *argp = (void __user *)arg;
	u32 src_colorkey;/* 16 bit or 24 bit */
	u32 srckey_enable;
	u32 gbl_alpha;
	u32 osd_order;
	s32 osd_axis[4] = {0};
	s32 osd_dst_axis[4] = {0};
	u32 block_windows[8] = {0};
	u32 block_mode;
	unsigned long ret;
	u32 flush_rate;
	struct fb_sync_request_s sync_request;
	struct fb_sync_request_render_s sync_request_render;
	struct fb_dmabuf_export dmaexp;

	switch (cmd) {
	case  FBIOPUT_OSD_SRCKEY_ENABLE:
		ret = copy_from_user(&srckey_enable, argp, sizeof(u32));
		break;
	case  FBIOPUT_OSD_SRCCOLORKEY:
		ret = copy_from_user(&src_colorkey, argp, sizeof(u32));
		break;
	case FBIOPUT_OSD_SET_GBL_ALPHA:
		ret = copy_from_user(&gbl_alpha, argp, sizeof(u32));
		break;
	case FBIOPUT_OSD_SCALE_AXIS:
		ret = copy_from_user(&osd_axis, argp, 4 * sizeof(s32));
		break;
	case FBIOPUT_OSD_SYNC_ADD:
		ret = copy_from_user(&sync_request, argp,
				sizeof(struct fb_sync_request_s));
		break;
	case FBIOPUT_OSD_SYNC_RENDER_ADD:
		ret = copy_from_user(&sync_request_render, argp,
				sizeof(struct fb_sync_request_render_s));
		break;
	case FBIO_WAITFORVSYNC:
	case FBIOGET_OSD_SCALE_AXIS:
	case FBIOPUT_OSD_ORDER:
	case FBIOGET_OSD_ORDER:
	case FBIOGET_OSD_GET_GBL_ALPHA:
	case FBIOPUT_OSD_2X_SCALE:
	case FBIOPUT_OSD_ENABLE_3D_MODE:
	case FBIOPUT_OSD_FREE_SCALE_ENABLE:
	case FBIOPUT_OSD_FREE_SCALE_MODE:
	case FBIOPUT_OSD_FREE_SCALE_WIDTH:
	case FBIOPUT_OSD_FREE_SCALE_HEIGHT:
	case FBIOGET_OSD_BLOCK_WINDOWS:
	case FBIOGET_OSD_BLOCK_MODE:
	case FBIOGET_OSD_FREE_SCALE_AXIS:
	case FBIOGET_OSD_WINDOW_AXIS:
	case FBIOPUT_OSD_REVERSE:
	case FBIOPUT_OSD_ROTATE_ON:
	case FBIOPUT_OSD_ROTATE_ANGLE:
		break;
	case FBIOGET_DMABUF:
		ret = copy_from_user(&dmaexp, argp,
				sizeof(struct fb_dmabuf_export));
		break;
	case FBIOPUT_OSD_BLOCK_MODE:
		ret = copy_from_user(&block_mode, argp, sizeof(u32));
		break;
	case FBIOPUT_OSD_BLOCK_WINDOWS:
		ret = copy_from_user(&block_windows, argp, 8 * sizeof(u32));
		break;
	case FBIOPUT_OSD_FREE_SCALE_AXIS:
		ret = copy_from_user(&osd_axis, argp, 4 * sizeof(s32));
		break;
	case FBIOPUT_OSD_WINDOW_AXIS:
		ret = copy_from_user(&osd_dst_axis, argp, 4 * sizeof(s32));
		break;
	default:
		osd_log_err("command 0x%x not supported (%s)\n",
				cmd, current->comm);
		return -1;
	}
	mutex_lock(&fbdev->lock);
	switch (cmd) {
	case FBIOPUT_OSD_ORDER:
		osd_set_order_hw(info->node, arg);
		break;
	case FBIOGET_OSD_ORDER:
		osd_get_order_hw(info->node, &osd_order);
		ret = copy_to_user(argp, &osd_order, sizeof(u32));
		break;
	case FBIOPUT_OSD_FREE_SCALE_ENABLE:
		osd_set_free_scale_enable_hw(info->node, arg);
		break;
	case FBIOPUT_OSD_FREE_SCALE_MODE:
		osd_set_free_scale_mode_hw(info->node, arg);
		break;
	case FBIOPUT_OSD_ENABLE_3D_MODE:
		osd_enable_3d_mode_hw(info->node, arg);
		break;
	case FBIOPUT_OSD_2X_SCALE:
		/*
		 * high 16 bits for h_scale_enable;
		 * low 16 bits for v_scale_enable
		 */
		osd_set_2x_scale_hw(info->node, arg & 0xffff0000 ? 1 : 0,
				arg & 0xffff ? 1 : 0);
		break;
	case FBIOGET_OSD_FLUSH_RATE:
		osd_get_flush_rate_hw(&flush_rate);
		if (copy_to_user(argp, &flush_rate, sizeof(u32)))
			return -EFAULT;
		break;
	case FBIOPUT_OSD_REVERSE:
		if (arg >= REVERSE_MAX)
			arg = REVERSE_FALSE;
		osd_set_reverse_hw(info->node, arg);
		break;
	case FBIOPUT_OSD_ROTATE_ON:
		break;
	case FBIOPUT_OSD_ROTATE_ANGLE:
		break;
	case FBIOPUT_OSD_SRCCOLORKEY:
		switch (fbdev->color->color_index) {
		case COLOR_INDEX_16_655:
		case COLOR_INDEX_16_844:
		case COLOR_INDEX_16_565:
		case COLOR_INDEX_24_888_B:
		case COLOR_INDEX_24_RGB:
		case COLOR_INDEX_YUV_422:
			osd_log_dbg("set osd color key 0x%x\n", src_colorkey);
			fbdev->color_key = src_colorkey;
			osd_set_color_key_hw(info->node,
				fbdev->color->color_index, src_colorkey);
			break;
		default:
			break;
		}
		break;
	case FBIOPUT_OSD_SRCKEY_ENABLE:
		switch (fbdev->color->color_index) {
		case COLOR_INDEX_16_655:
		case COLOR_INDEX_16_844:
		case COLOR_INDEX_16_565:
		case COLOR_INDEX_24_888_B:
		case COLOR_INDEX_24_RGB:
		case COLOR_INDEX_YUV_422:
			osd_log_dbg("set osd color key %s\n",
					srckey_enable ? "enable" : "disable");
			if (srckey_enable != 0) {
				fbdev->enable_key_flag |= KEYCOLOR_FLAG_TARGET;
				if (!(fbdev->enable_key_flag &
						KEYCOLOR_FLAG_ONHOLD)) {
					osd_srckey_enable_hw(info->node, 1);
					fbdev->enable_key_flag |=
						KEYCOLOR_FLAG_CURRENT;
				}
			} else {
				osd_srckey_enable_hw(info->node, 0);
				fbdev->enable_key_flag &= ~(KEYCOLOR_FLAG_TARGET
						| KEYCOLOR_FLAG_CURRENT);
			}
			break;
		default:
			break;
		}
		break;
	case FBIOPUT_OSD_SET_GBL_ALPHA:
		osd_set_gbl_alpha_hw(info->node, gbl_alpha);
		break;
	case  FBIOGET_OSD_GET_GBL_ALPHA:
		gbl_alpha = osd_get_gbl_alpha_hw(info->node);
		ret = copy_to_user(argp, &gbl_alpha, sizeof(u32));
		break;
	case FBIOGET_OSD_SCALE_AXIS:
		osd_get_scale_axis_hw(info->node, &osd_axis[0],
			&osd_axis[1], &osd_axis[2], &osd_axis[3]);
		ret = copy_to_user(argp, &osd_axis, 4 * sizeof(s32));
		break;
	case FBIOPUT_OSD_SCALE_AXIS:
		osd_set_scale_axis_hw(info->node, osd_axis[0], osd_axis[1],
				osd_axis[2], osd_axis[3]);
		break;
	case FBIOGET_OSD_BLOCK_WINDOWS:
		osd_get_block_windows_hw(info->node, block_windows);
		ret = copy_to_user(argp, &block_windows, 8 * sizeof(u32));
		break;
	case FBIOPUT_OSD_BLOCK_WINDOWS:
		osd_set_block_windows_hw(info->node, block_windows);
		break;
	case FBIOPUT_OSD_BLOCK_MODE:
		osd_set_block_mode_hw(info->node, block_mode);
		break;
	case FBIOGET_OSD_BLOCK_MODE:
		osd_get_block_mode_hw(info->node, &block_mode);
		ret = copy_to_user(argp, &block_mode, sizeof(u32));
		break;
	case FBIOGET_OSD_FREE_SCALE_AXIS:
		osd_get_free_scale_axis_hw(info->node, &osd_axis[0],
			&osd_axis[1], &osd_axis[2], &osd_axis[3]);
		ret = copy_to_user(argp, &osd_axis, 4 * sizeof(s32));
		break;
	case FBIOGET_OSD_WINDOW_AXIS:
		osd_get_window_axis_hw(info->node, &osd_dst_axis[0],
			&osd_dst_axis[1], &osd_dst_axis[2], &osd_dst_axis[3]);
		ret = copy_to_user(argp, &osd_dst_axis, 4 * sizeof(s32));
		break;
	case FBIOPUT_OSD_FREE_SCALE_AXIS:
		osd_set_free_scale_axis_hw(info->node, osd_axis[0],
			osd_axis[1], osd_axis[2], osd_axis[3]);
		break;
	case FBIOPUT_OSD_WINDOW_AXIS:
		osd_set_window_axis_hw(info->node, osd_dst_axis[0],
			osd_dst_axis[1], osd_dst_axis[2], osd_dst_axis[3]);
		break;
	case FBIOPUT_OSD_SYNC_ADD:
		sync_request.out_fen_fd =
			osd_sync_request(info->node, info->var.yres,
					sync_request.xoffset,
					sync_request.yoffset,
					sync_request.in_fen_fd);
		ret = copy_to_user(argp, &sync_request,
				sizeof(struct fb_sync_request_s));
		if (sync_request.out_fen_fd  < 0) {
			/* fence create fail. */
			ret = -1;
		} else {
			info->var.xoffset = sync_request.xoffset;
			info->var.yoffset = sync_request.yoffset;
		}
		break;
	case FBIOPUT_OSD_SYNC_RENDER_ADD:
		{
			ion_phys_addr_t addr;
			size_t len;
			u32 phys_addr;

			ret = meson_ion_share_fd_to_phys(fb_ion_client,
				sync_request_render.shared_fd, &addr, &len);
			if (ret == 0) {
				if (sync_request_render.type ==
					GE2D_COMPOSE_MODE) {
					phys_addr = addr +
						sync_request_render.yoffset
						* info->fix.line_length;
				} else
					phys_addr = addr;
			} else
				phys_addr = 0;

			sync_request_render.out_fen_fd =
				osd_sync_request_render(info->node,
				info->var.yres,
				&sync_request_render, phys_addr);
			osd_restore_screen_info(info->node,
				&info->screen_base, &info->screen_size);
			ret = copy_to_user(argp,
				&sync_request_render,
				sizeof(struct fb_sync_request_render_s));
			if (sync_request_render.out_fen_fd  < 0) {
				/* fence create fail. */
				ret = -1;
			} else {
				info->var.xoffset = sync_request_render.xoffset;
				info->var.yoffset = sync_request_render.yoffset;
			}
		}
		break;
	case FBIOGET_DMABUF:
		{
			if (info->node == DEV_OSD0 && osd_get_afbc()) {
				dmaexp.fd =
					ion_share_dma_buf_fd(
						fb_ion_client,
						fb_ion_handle[info->node]
						[dmaexp.buffer_idx]);
			} else {
				dmaexp.fd =
						ion_share_dma_buf_fd(
						fb_ion_client,
						fb_ion_handle[info->node][0]);
			}
			dmaexp.flags = O_CLOEXEC;

			ret = copy_to_user(argp, &dmaexp, sizeof(dmaexp))
				? -EFAULT : 0;
			break;
		}
	case FBIO_WAITFORVSYNC:
		osd_wait_vsync_hw();
		ret = copy_to_user(argp, &ret, sizeof(u32));
		break;
	default:
		break;
	}
	mutex_unlock(&fbdev->lock);
	return  ret;
}

#ifdef CONFIG_COMPAT
struct fb_cursor_user32 {
	__u16 set;		/* what to set */
	__u16 enable;		/* cursor on/off */
	__u16 rop;		/* bitop operation */
	compat_caddr_t mask;
	struct fbcurpos hot;	/* cursor hot spot */
	struct fb_image image;	/* Cursor image */
};

static int osd_compat_cursor(struct fb_info *info, unsigned long arg)
{
	unsigned long ret;
	struct fb_cursor_user __user *ucursor;
	struct fb_cursor_user32 __user *ucursor32;
	struct fb_cursor cursor;
	void __user *argp;
	__u32 data;

	ucursor = compat_alloc_user_space(sizeof(*ucursor));
	ucursor32 = compat_ptr(arg);

	if (copy_in_user(&ucursor->set, &ucursor32->set, 3 * sizeof(__u16)))
		return -EFAULT;
	if (get_user(data, &ucursor32->mask) ||
	    put_user(compat_ptr(data), &ucursor->mask))
		return -EFAULT;
	if (copy_in_user(&ucursor->hot, &ucursor32->hot, 2 * sizeof(__u16)))
		return -EFAULT;

	argp = (void __user *)ucursor;
	if (copy_from_user(&cursor, argp, sizeof(cursor)))
		return -EFAULT;

	if (!lock_fb_info(info))
		return -ENODEV;
	if (!info->fbops->fb_cursor)
		return -EINVAL;
	console_lock();
	ret = info->fbops->fb_cursor(info, &cursor);
	console_unlock();
	unlock_fb_info(info);

	if (ret == 0 && copy_to_user(argp, &cursor, sizeof(cursor)))
		return -EFAULT;

	return ret;
}

static int osd_compat_ioctl(struct fb_info *info,
		unsigned int cmd, unsigned long arg)
{
	unsigned long ret;

	arg = (unsigned long)compat_ptr(arg);

	/* handle fbio cursor command for 32-bit app */
	if ((cmd & 0xFFFF) == (FBIO_CURSOR & 0xFFFF))
		ret = osd_compat_cursor(info, arg);
	else
		ret = osd_ioctl(info, cmd, arg);

	return ret;
}
#endif

static int osd_open(struct fb_info *info, int arg)
{
	int j;
	int ret = 0;
	u32 fb_index;
	int logo_index = -1;
	struct osd_fb_dev_s *fbdev;
	struct fb_fix_screeninfo *fix = NULL;
	struct fb_var_screeninfo *var = NULL;
	struct platform_device *pdev = NULL;
	const struct vinfo_s *vinfo;

	fbdev = (struct osd_fb_dev_s *)info->par;
	fbdev->open_count++;
	osd_log_info("osd_open index=%d,open_count=%d\n",
		fbdev->fb_index, fbdev->open_count);
	if (info->screen_base != NULL)
		return 0;
	pdev = fbdev->dev;
	fb_index = fbdev->fb_index;
	fix = &info->fix;
	var = &info->var;

	if (fb_index == DEV_OSD0) {
		vinfo = get_current_vinfo();
		if (!vinfo) {
			osd_log_err("current vinfo NULL\n");
			return -1;
		}
		var->width = vinfo->screen_real_width;
		var->height = vinfo->screen_real_height;
	}

	if (fb_rmem.base == 0) {
		pr_info("use ion buffer for fb memory\n");
		if (!fb_ion_client)
			fb_ion_client = meson_ion_client_create(-1, "meson-fb");
		if (fb_index == DEV_OSD0 && osd_get_afbc()) {
			pr_info("OSD0 as afbcd mode\n");
			for (j = 0; j < OSD_MAX_BUF_NUM; j++) {
				fb_ion_handle[fb_index][j] =
				ion_alloc(fb_ion_client,
					PAGE_ALIGN(fb_memsize[fb_index]/
					OSD_MAX_BUF_NUM),
					0,
					ION_HEAP_CARVEOUT_MASK,
					0);
				ret = ion_phys(fb_ion_client,
					fb_ion_handle[fb_index][j],
					(ion_phys_addr_t *)
					&fb_rmem_afbc_paddr[fb_index][j],
					(size_t *)
					&fb_rmem_afbc_size[fb_index][j]);
				fb_rmem_afbc_vaddr[fb_index][j] =
					ion_map_kernel(fb_ion_client,
					fb_ion_handle[fb_index][j]);
				dev_alert(&pdev->dev,
					"create ion_client %p, handle=%p\n",
					fb_ion_client,
					fb_ion_handle[fb_index][j]);
				dev_alert(&pdev->dev,
					"ion memory(%d): created fb at 0x%p, size %ld MiB\n",
					fb_index,
					(void *)fb_rmem_afbc_paddr
					[fb_index][j],
					(unsigned long)fb_rmem_afbc_size
					[fb_index][j] / SZ_1M);
				fbdev->fb_afbc_len[j] =
					fb_rmem_afbc_size[fb_index][j];
				fbdev->fb_mem_afbc_paddr[j] =
					fb_rmem_afbc_paddr[fb_index][j];
				fbdev->fb_mem_afbc_vaddr[j] =
					fb_rmem_afbc_vaddr[fb_index][j];
				if  (!fbdev->fb_mem_afbc_vaddr[j]) {
					osd_log_err("failed to ioremap afbc frame buffer\n");
					return -1;
				}
				osd_log_info(" %d, phy: 0x%p, vir:0x%p, size=%dK\n\n",
					fb_index,
					(void *)fbdev->fb_mem_afbc_paddr[j],
					fbdev->fb_mem_afbc_vaddr[j],
					fbdev->fb_afbc_len[j] >> 10);
			}
			fb_rmem_paddr[fb_index] =
					fb_rmem_afbc_paddr[fb_index][0];
			fb_rmem_vaddr[fb_index] =
					fb_rmem_afbc_vaddr[fb_index][0];
			fb_rmem_size[fb_index] =
					fb_rmem_afbc_size[fb_index][0];
		} else {
			fb_ion_handle[fb_index][0] =
						ion_alloc(fb_ion_client,
							fb_memsize[fb_index],
							0,
							ION_HEAP_CARVEOUT_MASK,
							0);
			ret = ion_phys(fb_ion_client,
				fb_ion_handle[fb_index][0],
				(ion_phys_addr_t *)&fb_rmem_paddr[fb_index],
				(size_t *)&fb_rmem_size[fb_index]);
			fb_rmem_vaddr[fb_index] =
				ion_map_kernel(fb_ion_client,
						fb_ion_handle[fb_index][0]);
			dev_notice(&pdev->dev,
				"create ion_client %p, handle=%p\n",
				fb_ion_client,
				fb_ion_handle[fb_index][0]);
			dev_notice(&pdev->dev,
				"ion memory(%d): created fb at 0x%p, size %ld MiB\n",
				fb_index,
				(void *)fb_rmem_paddr[fb_index],
				(unsigned long)fb_rmem_size[fb_index] / SZ_1M);
		}
	} else {
		if (!fb_ion_client)
			fb_ion_client = meson_ion_client_create(-1, "meson-fb");
		fb_rmem_size[fb_index] = fb_memsize[fb_index];
		if (fb_index == DEV_OSD0)
			fb_rmem_paddr[fb_index] = fb_rmem.base;
		else if (fb_index == DEV_OSD1) {
			if ((OSD_COUNT == 2) &&
			((fb_memsize[0] + fb_memsize[1]) <= fb_rmem.size)) {
				fb_rmem_paddr[fb_index] =
					fb_rmem.base + fb_memsize[0];
			}
		}
		if ((fb_rmem_paddr[fb_index] > 0) &&
			(fb_rmem_size[fb_index] > 0)) {
			fb_rmem_vaddr[fb_index] =
					ioremap_wc(fb_rmem_paddr[fb_index],
					fb_rmem_size[fb_index]);
			if (!fb_rmem_vaddr[fb_index])
				osd_log_err("fb[%d] ioremap error",
						fb_index);
		}
	}
	fbdev->fb_len = fb_rmem_size[fb_index];
	fbdev->fb_mem_paddr = fb_rmem_paddr[fb_index];
	fbdev->fb_mem_vaddr = fb_rmem_vaddr[fb_index];
	if (!fbdev->fb_mem_vaddr) {
		osd_log_err("failed to ioremap frame buffer\n");
		ret = -ENOMEM;
	}
	osd_log_info("Frame buffer memory assigned at");
	osd_log_info(" %d, phy: 0x%p, vir:0x%p, size=%dK\n\n",
		fb_index, (void *)fbdev->fb_mem_paddr,
		fbdev->fb_mem_vaddr, fbdev->fb_len >> 10);
	if (fb_index == DEV_OSD0 && osd_get_afbc()) {
		for (j = 0; j < OSD_MAX_BUF_NUM; j++) {
			fbdev->fb_afbc_len[j] =
				fb_rmem_afbc_size[fb_index][j];
			fbdev->fb_mem_afbc_paddr[j] =
				fb_rmem_afbc_paddr[fb_index][j];
			fbdev->fb_mem_afbc_vaddr[j] =
				fb_rmem_afbc_vaddr[fb_index][j];
			if (!fbdev->fb_mem_afbc_vaddr[j]) {
				osd_log_err("failed to ioremap afbc frame buffer\n");
				return -ENOMEM;
			}
			osd_log_info(" %d, phy: 0x%p, vir:0x%p, size=%dK\n\n",
					fb_index,
					(void *)
					fbdev->fb_mem_afbc_paddr[j],
					fbdev->fb_mem_afbc_vaddr[j],
					fbdev->fb_afbc_len[j] >> 10);
		}
	}
	fix->smem_start = fbdev->fb_mem_paddr;
	fix->smem_len = fbdev->fb_len;
	info->screen_base = (char __iomem *)fbdev->fb_mem_vaddr;
	info->screen_size = fix->smem_len;
	osd_backup_screen_info(fb_index, info->screen_base, info->screen_size);
	logo_index = osd_get_logo_index();
	if (osd_check_fbsize(var, info))
		return -ENOMEM;
	/* clear osd buffer if not logo layer */
	if ((logo_index < 0) || (logo_index != fb_index)) {
		osd_log_info("---------------clear fb%d memory %p\n",
			fb_index, fbdev->fb_mem_vaddr);
		memset(fbdev->fb_mem_vaddr, 0x0, fbdev->fb_len);
		if (fb_index == DEV_OSD0 && osd_get_afbc()) {
			for (j = 1; j < OSD_MAX_BUF_NUM; j++) {
				osd_log_info(
					"---------------clear fb%d memory %p\n",
					fb_index,
					fbdev->fb_mem_afbc_vaddr[j]);
				memset(fbdev->fb_mem_afbc_vaddr[j],
					0x0,
					fbdev->fb_afbc_len[j]);
			}
		} else {
			/*two case in one
			 * 1. the big buffer ion alloc
			 * 2. reserved memory
			 */

			memset(fb_rmem_vaddr[fb_index],
					0x0,
					fb_rmem_size[fb_index]);
		}
		/* setup osd if not logo layer */
		osddev_setup(fbdev);
	} else {
		/* memset(fb_rmem_vaddr[fb_index],
				0x0,
				fb_rmem_size[fb_index]); */
		osd_log_info("fb%d open, logo index:%d\n",
			fb_index, logo_index);
	}
	return 0;
}


static int osd_release(struct fb_info *info, int arg)
{
	struct osd_fb_dev_s *fbdev;
	int err = 0;

	fbdev = (struct osd_fb_dev_s *)info->par;

	if (!fbdev->open_count) {
		err = -EINVAL;
		osd_log_info("osd already released. index=%d\n",
			fbdev->fb_index);
		goto done;
	}
	osd_log_info("osd_release now.index=%d,open_count=%d\n",
		fbdev->fb_index, fbdev->open_count);

	fbdev->open_count--;
done:
	return err;
}

static ssize_t osd_clear(struct device *device, struct device_attribute *attr,
		const char *buf, size_t count)
{
	u32 res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	osd_log_info("clear: osd %d\n", res);

	memset(fb_rmem_vaddr[res],
			0x0,
			fb_rmem_size[res]);

	return count;
}

int osd_blank(int blank_mode, struct fb_info *info)
{
	osd_enable_hw(info->node, (blank_mode != 0) ? 0 : 1);
	return 0;
}

static int osd_pan_display(struct fb_var_screeninfo *var,
			   struct fb_info *fbi)
{
	osd_pan_display_hw(fbi->node, var->xoffset, var->yoffset);
	/* osd_log_dbg("osd_pan_display:=>osd%d xoff=%d, yoff=%d\n",
			fbi->node, var->xoffset, var->yoffset);
	*/
	return 0;
}

#if defined(CONFIG_FB_OSD2_CURSOR)
static int osd_cursor(struct fb_info *fbi, struct fb_cursor *var)
{
	s16 startx = 0, starty = 0;
	struct osd_fb_dev_s *fb_dev = gp_fbdev_list[1];
	if (fb_dev) {
		startx = fb_dev->osd_ctl.disp_start_x;
		starty = fb_dev->osd_ctl.disp_start_y;
	}
	osd_cursor_hw(fbi->node, (s16)var->hot.x, (s16)var->hot.y, (s16)startx,
		      (s16)starty, fbi->var.xres, fbi->var.yres);
	return 0;
}
#endif

static int osd_sync(struct fb_info *info)
{
	return 0;
}


/* fb_ops structures */
static struct fb_ops osd_ops = {
	.owner          = THIS_MODULE,
	.fb_check_var   = osd_check_var,
	.fb_set_par     = osd_set_par,
	.fb_setcolreg   = osd_setcolreg,
	.fb_setcmap     = osd_setcmap,
	.fb_fillrect    = cfb_fillrect,
	.fb_copyarea    = cfb_copyarea,
	.fb_imageblit   = cfb_imageblit,
#ifdef CONFIG_FB_SOFT_CURSOR
	.fb_cursor      = soft_cursor,
#elif defined(CONFIG_FB_OSD2_CURSOR)
	.fb_cursor      = osd_cursor,
#endif
	.fb_ioctl       = osd_ioctl,
#ifdef CONFIG_COMPAT
	.fb_compat_ioctl = osd_compat_ioctl,
#endif
	.fb_open        = osd_open,
	.fb_blank       = osd_blank,
	.fb_pan_display = osd_pan_display,
	.fb_sync        = osd_sync,
	.fb_release		= osd_release,
};

static void set_default_display_axis(struct fb_var_screeninfo *var,
		struct osd_ctl_s *osd_ctrl, const struct vinfo_s *vinfo)
{
	u32  virt_end_x = osd_ctrl->disp_start_x + var->xres;
	u32  virt_end_y = osd_ctrl->disp_start_y + var->yres;
	if (virt_end_x > vinfo->width) {
		/* screen axis */
		osd_ctrl->disp_end_x = vinfo->width - 1;
	} else {
		/* screen axis */
		osd_ctrl->disp_end_x = virt_end_x - 1;
	}
	if (virt_end_y > vinfo->height)
		osd_ctrl->disp_end_y = vinfo->height - 1;
	else {
		/* screen axis */
		osd_ctrl->disp_end_y = virt_end_y - 1;
	}
	return;
}

int osd_notify_callback(struct notifier_block *block, unsigned long cmd,
			void *para)
{
	const struct vinfo_s *vinfo;
	struct osd_fb_dev_s *fb_dev;
	int  i, blank;
	struct disp_rect_s *disp_rect;

	vinfo = get_current_vinfo();
	if (!vinfo) {
		osd_log_err("current vinfo NULL\n");
		return -1;
	}
	osd_log_info("current vmode=%s, cmd: 0x%lx\n",
		vinfo->name, cmd);
	switch (cmd) {
#if 0
	case VOUT_EVENT_MODE_CHANGE_PRE:
		for (i = 0; i < OSD_COUNT; i++) {
			fb_dev = gp_fbdev_list[i];
			if (osd_hw.enable[i]) {
				fb_dev->dis_osd_mchange = true;
				osd_enable_hw(i, false);
			}
		}
		break;
#endif
	case  VOUT_EVENT_MODE_CHANGE:
		for (i = 0; i < OSD_COUNT; i++) {
			fb_dev = gp_fbdev_list[i];
			if (NULL == fb_dev)
				continue;

			if (i == DEV_OSD0) {
				fb_dev->fb_info->var.width =
					vinfo->screen_real_width;
				fb_dev->fb_info->var.height =
					vinfo->screen_real_height;
			}

			set_default_display_axis(&fb_dev->fb_info->var,
					&fb_dev->osd_ctl, vinfo);
			console_lock();
			osddev_update_disp_axis(fb_dev, 1);
#ifdef CONFIG_FB_OSD2_ENABLE
			osd_set_antiflicker_hw(DEV_OSD1, vinfo->mode,
			       gp_fbdev_list[DEV_OSD1]->fb_info->var.yres);
#endif
			console_unlock();
#if 0
			if (fb_dev->dis_osd_mchange) {
				fb_dev->dis_osd_mchange = false;
				osd_enable_hw(i, true);
			}
#endif
		}
		break;
	case VOUT_EVENT_OSD_BLANK:
		blank = *(int *)para;
		for (i = 0; i < OSD_COUNT; i++) {
			fb_dev = gp_fbdev_list[i];
			if (NULL == fb_dev)
				continue;
			console_lock();
			osd_blank(blank, fb_dev->fb_info);
			console_unlock();
		}
		break;
	case VOUT_EVENT_OSD_DISP_AXIS:
		disp_rect = (struct disp_rect_s *)para;
		for (i = 0; i < OSD_COUNT; i++) {
			if (!disp_rect)
				break;
			fb_dev = gp_fbdev_list[i];
			/*
			 * if osd layer preblend,
			 * it's position is controlled by vpp.
			if (fb_dev->preblend_enable)
				break;
			*/
			fb_dev->osd_ctl.disp_start_x = disp_rect->x;
			fb_dev->osd_ctl.disp_start_y = disp_rect->y;
			osd_log_dbg("set disp axis: x:%d y:%d w:%d h:%d\n",
				    disp_rect->x, disp_rect->y,
				    disp_rect->w, disp_rect->h);
			if (disp_rect->x + disp_rect->w > vinfo->width)
				fb_dev->osd_ctl.disp_end_x = vinfo->width - 1;
			else
				fb_dev->osd_ctl.disp_end_x =
					fb_dev->osd_ctl.disp_start_x +
					disp_rect->w - 1;
			if (disp_rect->y + disp_rect->h > vinfo->height)
				fb_dev->osd_ctl.disp_end_y = vinfo->height - 1;
			else
				fb_dev->osd_ctl.disp_end_y =
					fb_dev->osd_ctl.disp_start_y +
					disp_rect->h - 1;
			disp_rect++;
			osd_log_dbg("new disp axis: x0:%d y0:%d x1:%d y1:%d\n",
				    fb_dev->osd_ctl.disp_start_x,
				    fb_dev->osd_ctl.disp_start_y,
				    fb_dev->osd_ctl.disp_end_x,
				    fb_dev->osd_ctl.disp_end_y);
			console_lock();
			osddev_update_disp_axis(fb_dev, 0);
			console_unlock();
		}
		break;
	}
	return 0;
}

static struct notifier_block osd_notifier_nb = {
	.notifier_call	= osd_notify_callback,
};

static ssize_t store_enable_3d(struct device *device,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	fbdev->enable_3d = res;
	osd_enable_3d_mode_hw(fb_info->node, fbdev->enable_3d);
	return count;
}

static ssize_t show_enable_3d(struct device *device,
			      struct device_attribute *attr,
			      char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	return snprintf(buf, PAGE_SIZE, "3d_enable:[0x%x]\n", fbdev->enable_3d);
}

static ssize_t store_color_key(struct device *device,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 16, &res);
	switch (fbdev->color->color_index) {
	case COLOR_INDEX_16_655:
	case COLOR_INDEX_16_844:
	case COLOR_INDEX_16_565:
	case COLOR_INDEX_24_888_B:
	case COLOR_INDEX_24_RGB:
	case COLOR_INDEX_YUV_422:
		fbdev->color_key = res;
		osd_set_color_key_hw(fb_info->node, fbdev->color->color_index,
				     fbdev->color_key);
		break;
	default:
		break;
	}

	return count;
}

static ssize_t show_color_key(struct device *device,
			      struct device_attribute *attr,
			      char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	return snprintf(buf, PAGE_SIZE, "0x%x\n", fbdev->color_key);
}

static ssize_t store_enable_key(struct device *device,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	mutex_lock(&fbdev->lock);
	if (res != 0) {
		fbdev->enable_key_flag |= KEYCOLOR_FLAG_TARGET;
		if (!(fbdev->enable_key_flag & KEYCOLOR_FLAG_ONHOLD)) {
			osd_srckey_enable_hw(fb_info->node, 1);
			fbdev->enable_key_flag |= KEYCOLOR_FLAG_CURRENT;
		}
	} else {
		fbdev->enable_key_flag &=
			~(KEYCOLOR_FLAG_TARGET | KEYCOLOR_FLAG_CURRENT);
		osd_srckey_enable_hw(fb_info->node, 0);
	}
	mutex_unlock(&fbdev->lock);
	return count;
}

static ssize_t show_enable_key(struct device *device,
			       struct device_attribute *attr,
			       char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	return snprintf(buf, PAGE_SIZE, (fbdev->enable_key_flag
			& KEYCOLOR_FLAG_TARGET) ? "1\n" : "0\n");
}

static ssize_t store_enable_key_onhold(struct device *device,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0 , &res);
	mutex_lock(&fbdev->lock);
	if (res != 0) {
		/* hold all the calls to enable color key */
		fbdev->enable_key_flag |= KEYCOLOR_FLAG_ONHOLD;
		fbdev->enable_key_flag &= ~KEYCOLOR_FLAG_CURRENT;
		osd_srckey_enable_hw(fb_info->node, 0);
	} else {
		fbdev->enable_key_flag &= ~KEYCOLOR_FLAG_ONHOLD;
		/*
		 * if target and current mistach
		 * then recover the pending key settings
		 */
		if (fbdev->enable_key_flag & KEYCOLOR_FLAG_TARGET) {
			if ((fbdev->enable_key_flag & KEYCOLOR_FLAG_CURRENT)
					== 0) {
				fbdev->enable_key_flag |= KEYCOLOR_FLAG_CURRENT;
				osd_srckey_enable_hw(fb_info->node, 1);
			}
		} else {
			if (fbdev->enable_key_flag & KEYCOLOR_FLAG_CURRENT) {
				fbdev->enable_key_flag &=
					~KEYCOLOR_FLAG_CURRENT;
				osd_srckey_enable_hw(fb_info->node, 0);
			}
		}
	}
	mutex_unlock(&fbdev->lock);
	return count;
}

static ssize_t show_enable_key_onhold(struct device *device,
				      struct device_attribute *attr,
				      char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	return snprintf(buf, PAGE_SIZE,
			(fbdev->enable_key_flag & KEYCOLOR_FLAG_ONHOLD) ?
			"1\n" : "0\n");
}

static int parse_para(const char *para, int para_num, int *result)
{
	char *token = NULL;
	char *params, *params_base;
	int *out = result;
	int len = 0, count = 0;
	int res = 0;
	int ret = 0;

	if (!para)
		return 0;

	params = kstrdup(para, GFP_KERNEL);
	params_base = params;
	token = params;
	len = strlen(token);
	do {
		token = strsep(&params, " ");
		while (token && (isspace(*token)
				|| !isgraph(*token)) && len) {
			token++;
			len--;
		}
		if (len == 0)
			break;
		ret = kstrtoint(token, 0, &res);
		if (ret < 0)
			break;
		len = strlen(token);
		*out++ = res;
		count++;
	} while ((token) && (count < para_num) && (len > 0));

	kfree(params_base);
	return count;
}

static ssize_t show_free_scale_axis(struct device *device,
				    struct device_attribute *attr,
				    char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	int x, y, w, h;
	osd_get_free_scale_axis_hw(fb_info->node, &x, &y, &w, &h);
	return snprintf(buf, PAGE_SIZE, "%d %d %d %d\n", x, y, w, h);
}

static ssize_t store_free_scale_axis(struct device *device,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	int parsed[4];
	if (likely(parse_para(buf, 4, parsed) == 4))
		osd_set_free_scale_axis_hw(fb_info->node,
				parsed[0], parsed[1], parsed[2], parsed[3]);
	else
		osd_log_err("set free scale axis error\n");
	return count;
}

static ssize_t show_scale_axis(struct device *device,
			       struct device_attribute *attr,
			       char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	int x0, y0, x1, y1;
	osd_get_scale_axis_hw(fb_info->node, &x0, &y0, &x1, &y1);
	return snprintf(buf, PAGE_SIZE, "%d %d %d %d\n", x0, y0, x1, y1);
}

static ssize_t store_scale_axis(struct device *device,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	int parsed[4];
	if (likely(parse_para(buf, 4, parsed) == 4))
		osd_set_scale_axis_hw(fb_info->node,
				parsed[0], parsed[1], parsed[2], parsed[3]);
	else
		osd_log_err("set scale axis error\n");
	return count;
}


static ssize_t show_scale_width(struct device *device,
				struct device_attribute *attr,
				char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	unsigned int free_scale_width = 0;
	osd_get_free_scale_width_hw(fb_info->node, &free_scale_width);
	return snprintf(buf, PAGE_SIZE, "free_scale_width:%d\n",
			free_scale_width);
}

static ssize_t show_scale_height(struct device *device,
				 struct device_attribute *attr,
				 char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	unsigned int free_scale_height = 0;
	osd_get_free_scale_height_hw(fb_info->node, &free_scale_height);
	return snprintf(buf, PAGE_SIZE, "free_scale_height:%d\n",
			free_scale_height);
}

static ssize_t store_free_scale(struct device *device,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	unsigned int free_scale_enable = 0;
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	free_scale_enable = res;
	osd_set_free_scale_enable_hw(fb_info->node, free_scale_enable);
	return count;
}

static ssize_t show_free_scale(struct device *device,
			       struct device_attribute *attr,
			       char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	unsigned int free_scale_enable = 0;
	osd_get_free_scale_enable_hw(fb_info->node, &free_scale_enable);
	return snprintf(buf, PAGE_SIZE, "free_scale_enable:[0x%x]\n",
			free_scale_enable);
}

static ssize_t store_freescale_mode(struct device *device,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	unsigned int free_scale_mode = 0;
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	free_scale_mode = res;
	osd_set_free_scale_mode_hw(fb_info->node, free_scale_mode);
	return count;
}

static ssize_t show_freescale_mode(struct device *device,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	unsigned int free_scale_mode = 0;
	char *help_info = "free scale mode:\n"
			  "    0: VPU free scaler\n"
			  "    1: OSD free scaler\n"
			  "    2: OSD super scaler\n";
	osd_get_free_scale_mode_hw(fb_info->node, &free_scale_mode);
	return snprintf(buf, PAGE_SIZE, "%scurrent free_scale_mode:%d\n",
			help_info, free_scale_mode);
}

static ssize_t store_scale(struct device *device, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	fbdev->scale = res;
	osd_set_2x_scale_hw(fb_info->node, fbdev->scale & 0xffff0000 ? 1 : 0,
			    fbdev->scale & 0xffff ? 1 : 0);
	return count;
}

static ssize_t show_scale(struct device *device, struct device_attribute *attr,
			  char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	return snprintf(buf, PAGE_SIZE, "scale:[0x%x]\n", fbdev->scale);
}

static ssize_t show_window_axis(struct device *device,
				struct device_attribute *attr,
				char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	int x0, y0, x1, y1;
	osd_get_window_axis_hw(fb_info->node, &x0, &y0, &x1, &y1);
	return snprintf(buf, PAGE_SIZE, "%d %d %d %d\n",
			x0, y0, x1, y1);
}

static ssize_t store_window_axis(struct device *device,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	s32 parsed[4];
	if (likely(parse_para(buf, 4, parsed) == 4))
		osd_set_window_axis_hw(fb_info->node,
				parsed[0], parsed[1], parsed[2], parsed[3]);
	else
		osd_log_err("set window axis error\n");
	return count;
}

static ssize_t show_debug(struct device *device, struct device_attribute *attr,
			  char *buf)
{
	char *help = osd_get_debug_hw();

	return snprintf(buf, strlen(help), "%s", help);
}

static ssize_t store_debug(struct device *device, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int ret = -EINVAL;

	ret = osd_set_debug_hw(buf);
	if (ret == 0)
		ret = count;

	return ret;
}

static ssize_t show_afbcd(struct device *device, struct device_attribute *attr,
			  char *buf)
{
	u32 enable = osd_get_afbc();

	return snprintf(buf, PAGE_SIZE, "%d\n", enable);
}

static ssize_t store_afbcd(struct device *device, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	u32 res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	osd_log_info("afbc: %d\n", res);

	osd_set_afbc(res);

	return count;
}

static ssize_t show_log_level(struct device *device,
			  struct device_attribute *attr,
			  char *buf)
{
	return snprintf(buf, 40, "%d\n", osd_log_level);
}

static ssize_t store_log_level(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	osd_log_info("log_level: %d->%d\n", osd_log_level, res);
	osd_log_level = res;

	return count;
}

static ssize_t store_order(struct device *device, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	fbdev->order = res;
	osd_set_order_hw(fb_info->node, fbdev->order);
	return count;
}

static ssize_t show_order(struct device *device, struct device_attribute *attr,
			  char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	struct osd_fb_dev_s *fbdev = (struct osd_fb_dev_s *)fb_info->par;
	osd_get_order_hw(fb_info->node, &(fbdev->order));
	return snprintf(buf, PAGE_SIZE, "order:[0x%x]\n", fbdev->order);
}

static ssize_t show_block_windows(struct device *device,
				  struct device_attribute *attr,
				  char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	u32 wins[8];
	osd_get_block_windows_hw(fb_info->node, wins);
	return snprintf(buf, PAGE_SIZE,
			"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			wins[0], wins[1], wins[2], wins[3],
			wins[4], wins[5], wins[6], wins[7]);
}

static ssize_t store_block_windows(struct device *device,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	int parsed[8];
	if (likely(parse_para(buf, 8, parsed) == 8))
		osd_set_block_windows_hw(fb_info->node, parsed);
	else
		osd_log_err("set block windows error\n");
	return count;
}

static ssize_t show_block_mode(struct device *device,
			       struct device_attribute *attr,
			       char *buf)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	u32 mode;
	osd_get_block_mode_hw(fb_info->node, &mode);
	return snprintf(buf, PAGE_SIZE, "0x%x\n", mode);
}

static ssize_t store_block_mode(struct device *device,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	u32 mode;
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	mode = res;
	osd_set_block_mode_hw(fb_info->node, mode);
	return count;
}

static ssize_t show_flush_rate(struct device *device,
			       struct device_attribute *attr,
			       char *buf)
{
	u32 flush_rate = 0;
	osd_get_flush_rate_hw(&flush_rate);
	return snprintf(buf, PAGE_SIZE, "flush_rate:[%d]\n", flush_rate);
}

static ssize_t show_osd_reverse(struct device *device,
				struct device_attribute *attr,
				char *buf)
{
	char *str[4] = {"NONE", "ALL", "X_REV", "Y_REV"};
	struct fb_info *fb_info = dev_get_drvdata(device);
	unsigned int osd_reverse = 0;
	osd_get_reverse_hw(fb_info->node, &osd_reverse);
	if (osd_reverse >= REVERSE_MAX)
		osd_reverse = REVERSE_FALSE;
	return snprintf(buf, PAGE_SIZE, "osd_reverse:[%s]\n",
			str[osd_reverse]);
}

static ssize_t store_osd_reverse(struct device *device,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	unsigned int osd_reverse = 0;
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	osd_reverse = res;
	if (osd_reverse >= REVERSE_MAX)
		osd_reverse = REVERSE_FALSE;
	osd_set_reverse_hw(fb_info->node, osd_reverse);
	return count;
}

static ssize_t show_antiflicker(struct device *device,
				struct device_attribute *attr,
				char *buf)
{
	unsigned int osd_antiflicker = 0;
	struct fb_info *fb_info = dev_get_drvdata(device);
	osd_get_antiflicker_hw(fb_info->node, &osd_antiflicker);
	return snprintf(buf, PAGE_SIZE, "osd_antifliter:[%s]\n",
			osd_antiflicker ? "ON" : "OFF");
}

static ssize_t store_antiflicker(struct device *device,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	const struct vinfo_s *vinfo;
	unsigned int osd_antiflicker = 0;
	struct fb_info *fb_info = dev_get_drvdata(device);
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	osd_antiflicker = res;
	vinfo = get_current_vinfo();
	if (!vinfo) {
		osd_log_err("get current vinfo NULL\n");
		return 0;
	}
	if (osd_antiflicker == 2)
		osd_set_antiflicker_hw(fb_info->node,
				osd_antiflicker, fb_info->var.yres);
	else
		osd_set_antiflicker_hw(fb_info->node,
				vinfo->mode, fb_info->var.yres);
	return count;
}

static ssize_t show_update_freescale(struct device *device,
				     struct device_attribute *attr,
				     char *buf)
{
	unsigned int update_state = 0;
	return snprintf(buf, PAGE_SIZE, "update_state:[%s]\n",
			update_state ? "TRUE" : "FALSE");
}

static ssize_t store_update_freescale(struct device *device,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	unsigned int update_state = 0;
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	update_state = res;
	return count;
}

static ssize_t show_ver_angle(struct device *device,
			      struct device_attribute *attr,
			      char *buf)
{
	unsigned int osd_angle = 0;
	struct fb_info *fb_info = dev_get_drvdata(device);
	osd_get_angle_hw(fb_info->node, &osd_angle);
	return snprintf(buf, PAGE_SIZE, "osd_angle:[%d]\n", osd_angle);
}

static ssize_t store_ver_angle(struct device *device,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	unsigned int osd_angle = 0;
	struct fb_info *fb_info = dev_get_drvdata(device);
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	osd_angle = res;
	memset((char *)fb_info->screen_base, 0x80, fb_info->screen_size);
#ifdef CONFIG_FB_OSD2_ENABLE
	osd_set_angle_hw(fb_info->node, osd_angle, fb_def_var[DEV_OSD1].yres,
			 fb_info->var.yres);
#endif
	return count;
}

static ssize_t show_ver_clone(struct device *device,
			      struct device_attribute *attr,
			      char *buf)
{
	unsigned int osd_clone = 0;
	struct fb_info *fb_info = dev_get_drvdata(device);
	osd_get_clone_hw(fb_info->node, &osd_clone);
	return snprintf(buf, PAGE_SIZE, "osd_clone:[%s]\n",
			osd_clone ? "ON" : "OFF");
}

static ssize_t store_ver_clone(struct device *device,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	unsigned int osd_clone = 0;
	struct fb_info *fb_info = dev_get_drvdata(device);
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	osd_clone = res;
	osd_set_clone_hw(fb_info->node, osd_clone);
	return count;
}

static ssize_t store_ver_update_pan(struct device *device,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	osd_set_update_pan_hw(fb_info->node);
	return count;
}

static ssize_t show_reset_status(struct device *device,
	struct device_attribute *attr, char *buf)
{
	u32 status = osd_get_reset_status();

	return snprintf(buf, PAGE_SIZE, "0x%x\n", status);
}

static ssize_t free_scale_switch(struct device *device,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct fb_info *fb_info = dev_get_drvdata(device);
	u32 res = 0;
	int ret = 0;
	unsigned int free_scale_enable = 0;
	ret = kstrtoint(buf, 0, &res);
	free_scale_enable = res;
	osd_switch_free_scale(
		(fb_info->node == DEV_OSD0) ? DEV_OSD1 : DEV_OSD0,
		0, 0, fb_info->node, 1, free_scale_enable);
	osd_log_info("free_scale_switch to fb%d, mode: 0x%x\n",
		fb_info->node, free_scale_enable);
	return count;
}

static ssize_t show_osd_deband(struct device *device,
				struct device_attribute *attr,
				char *buf)
{
	u32 osd_deband_enable;
	osd_get_deband(&osd_deband_enable);
	return snprintf(buf, 40, "%d\n",
		osd_deband_enable);
}

static ssize_t store_osd_deband(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	osd_set_deband(res);

	return count;
}

static ssize_t show_osd_display_debug(struct device *device,
				struct device_attribute *attr,
				char *buf)
{
	u32 osd_display_debug_enable;
	osd_get_display_debug(&osd_display_debug_enable);
	return snprintf(buf, 40, "%d\n",
		osd_display_debug_enable);
}

static ssize_t store_osd_display_debug(struct device *device,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	if (ret < 0)
		return -EINVAL;
	osd_set_display_debug(res);

	return count;
}


static inline  int str2lower(char *str)
{
	while (*str != '\0') {
		*str = tolower(*str);
		str++;
	}
	return 0;
}

static struct para_osd_info_s para_osd_info[OSD_END + 2] = {
	/* head */
	{
		"head", OSD_INVALID_INFO,
		OSD_END + 1, 1,
		0, OSD_END + 1
	},
	/* dev */
	{
		"osd0",	DEV_OSD0,
		OSD_FIRST_GROUP_START - 1, OSD_FIRST_GROUP_START + 1,
		OSD_FIRST_GROUP_START, OSD_SECOND_GROUP_START - 1
	},
	{
		"osd1",	DEV_OSD1,
		OSD_FIRST_GROUP_START, OSD_FIRST_GROUP_START + 2,
		OSD_FIRST_GROUP_START, OSD_SECOND_GROUP_START - 1
	},
	{
		"all", DEV_ALL,
		OSD_FIRST_GROUP_START + 1, OSD_FIRST_GROUP_START + 3,
		OSD_FIRST_GROUP_START, OSD_SECOND_GROUP_START - 1
	},
	/* reverse_mode */
	{
		"false", REVERSE_FALSE,
		OSD_SECOND_GROUP_START - 1, OSD_SECOND_GROUP_START + 1,
		OSD_SECOND_GROUP_START, OSD_END
	},
	{
		"true", REVERSE_TRUE,
		OSD_SECOND_GROUP_START, OSD_SECOND_GROUP_START + 2,
		OSD_SECOND_GROUP_START, OSD_END
	},
	{
		"x_rev", REVERSE_X,
		OSD_SECOND_GROUP_START + 1, OSD_SECOND_GROUP_START + 3,
		OSD_SECOND_GROUP_START, OSD_END
	},
	{
		"y_rev", REVERSE_Y,
		OSD_SECOND_GROUP_START + 2, OSD_SECOND_GROUP_START + 4,
		OSD_SECOND_GROUP_START, OSD_END
	},
	{
		"tail", OSD_INVALID_INFO, OSD_END,
		0, 0,
		OSD_END + 1
	},
};

static inline int install_osd_reverse_info(struct osd_info_s *init_osd_info,
		char *para)
{
	u32 i = 0;
	static u32 tail = OSD_END + 1;
	u32 first = para_osd_info[0].next_idx;
	for (i = first; i < tail; i = para_osd_info[i].next_idx) {
		if (strcmp(para_osd_info[i].name, para) == 0) {
			u32 group_start = para_osd_info[i].cur_group_start;
			u32 group_end = para_osd_info[i].cur_group_end;
			u32	prev = para_osd_info[group_start].prev_idx;
			u32  next = para_osd_info[group_end].next_idx;
			switch (para_osd_info[i].cur_group_start) {
			case OSD_FIRST_GROUP_START:
				init_osd_info->index = para_osd_info[i].info;
				break;
			case OSD_SECOND_GROUP_START:
				init_osd_info->osd_reverse =
					para_osd_info[i].info;
				break;
			}
			para_osd_info[prev].next_idx = next;
			para_osd_info[next].prev_idx = prev;
			return 0;
		}
	}
	return 0;
}

static int __init osd_info_setup(char *str)
{
	char	*ptr = str;
	char	sep[2];
	char	*option;
	int count = 2;
	char find = 0;

	struct osd_info_s *init_osd_info;
	if (NULL == str)
		return -EINVAL;

	init_osd_info = &osd_info;
	memset(init_osd_info, 0, sizeof(struct osd_info_s));
	do {
		if (!isalpha(*ptr) && !isdigit(*ptr)) {
			find = 1;
			break;
		}
	} while (*++ptr != '\0');
	if (!find)
		return -EINVAL;
	sep[0] = *ptr;
	sep[1] = '\0';
	while ((count--) && (option = strsep(&str, sep))) {
		str2lower(option);
		install_osd_reverse_info(init_osd_info, option);
	}
	return 0;
}

__setup("osd_reverse=", osd_info_setup);
static struct device_attribute osd_attrs[] = {
	__ATTR(scale, S_IRUGO | S_IWUSR | S_IWGRP,
			show_scale, store_scale),
	__ATTR(order, S_IRUGO | S_IWUSR | S_IWGRP,
			show_order, store_order),
	__ATTR(enable_3d, S_IRUGO | S_IWUSR,
			show_enable_3d, store_enable_3d),
	__ATTR(free_scale, S_IRUGO | S_IWUSR | S_IWGRP,
			show_free_scale, store_free_scale),
	__ATTR(scale_axis, S_IRUGO | S_IWUSR,
			show_scale_axis, store_scale_axis),
	__ATTR(scale_width, S_IRUGO | S_IRUSR,
			show_scale_width, NULL),
	__ATTR(scale_height, S_IRUGO | S_IRUSR,
			show_scale_height, NULL),
	__ATTR(color_key, S_IRUGO | S_IWUSR,
			show_color_key, store_color_key),
	__ATTR(enable_key, S_IRUGO | S_IWUSR | S_IWGRP,
			show_enable_key, store_enable_key),
	__ATTR(enable_key_onhold, S_IRUGO | S_IWUSR | S_IWGRP,
			show_enable_key_onhold, store_enable_key_onhold),
	__ATTR(block_windows, S_IRUGO | S_IWUSR,
			show_block_windows, store_block_windows),
	__ATTR(block_mode, S_IRUGO | S_IWUSR | S_IWGRP,
			show_block_mode, store_block_mode),
	__ATTR(free_scale_axis, S_IRUGO | S_IWUSR | S_IWGRP,
			show_free_scale_axis, store_free_scale_axis),
	__ATTR(debug, S_IRUGO | S_IWUSR,
			show_debug, store_debug),
	__ATTR(log_level, S_IRUGO | S_IWUSR,
			show_log_level, store_log_level),
	__ATTR(window_axis, S_IRUGO | S_IWUSR | S_IWGRP,
			show_window_axis, store_window_axis),
	__ATTR(freescale_mode, S_IRUGO | S_IWUSR | S_IWGRP,
			show_freescale_mode, store_freescale_mode),
	__ATTR(flush_rate, S_IRUGO | S_IRUSR,
			show_flush_rate, NULL),
	__ATTR(osd_reverse, S_IRUGO | S_IWUSR,
			show_osd_reverse, store_osd_reverse),
	__ATTR(osd_antiflicker, S_IRUGO | S_IWUSR,
			show_antiflicker, store_antiflicker),
	__ATTR(update_freescale, S_IRUGO | S_IWUSR,
			show_update_freescale, store_update_freescale),
	__ATTR(ver_angle, S_IRUGO | S_IWUSR,
			show_ver_angle, store_ver_angle),
	__ATTR(ver_clone, S_IRUGO | S_IWUSR,
			show_ver_clone, store_ver_clone),
	__ATTR(ver_update_pan, S_IWUGO | S_IWUSR,
			NULL, store_ver_update_pan),
	__ATTR(osd_afbcd, S_IRUGO | S_IWUSR | S_IWGRP,
			show_afbcd, store_afbcd),
	__ATTR(osd_clear, S_IWUSR | S_IWGRP,
			NULL, osd_clear),
	__ATTR(reset_status, S_IRUGO | S_IRUSR,
			show_reset_status, NULL),
	__ATTR(free_scale_switch, S_IWUSR | S_IWGRP,
			NULL, free_scale_switch),
	__ATTR(osd_deband, S_IRUGO | S_IWUSR,
			show_osd_deband, store_osd_deband),
	__ATTR(osd_display_debug, S_IRUGO | S_IWUSR,
			show_osd_display_debug, store_osd_display_debug),
};

#ifdef CONFIG_PM
static int osd_suspend(struct platform_device *dev, pm_message_t state)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	if (early_suspend_flag)
		return 0;
#endif
	osd_suspend_hw();
	return 0;
}

static int osd_resume(struct platform_device *dev)
{
#ifdef CONFIG_SCREEN_ON_EARLY
	if (early_resume_flag) {
		early_resume_flag = 0;
		return 0;
	}
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	if (early_suspend_flag)
		return 0;
#endif
	osd_resume_hw();
	return 0;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void osd_early_suspend(struct early_suspend *h)
{
	if (early_suspend_flag)
		return;
	osd_suspend_hw();
	early_suspend_flag = 1;
}

static void osd_late_resume(struct early_suspend *h)
{
	if (!early_suspend_flag)
		return;
	early_suspend_flag = 0;
	osd_resume_hw();
}
#endif

#ifdef CONFIG_SCREEN_ON_EARLY
void osd_resume_early(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	osd_resume_hw();
	early_suspend_flag = 0;
#endif
	early_resume_flag = 1;
	return;
}
EXPORT_SYMBOL(osd_resume_early);
#endif

#ifdef CONFIG_HIBERNATION
static int osd_realdata_save(void)
{
	osd_realdata_save_hw();
	return 0;
}

static void osd_realdata_restore(void)
{
	osd_realdata_restore_hw();
}

static struct instaboot_realdata_ops osd_realdata_ops = {
	.save		= osd_realdata_save,
	.restore	= osd_realdata_restore,
};

static int osd_freeze(struct device *dev)
{
	osd_freeze_hw();
	return 0;
}

static int osd_thaw(struct device *dev)
{
	osd_thaw_hw();
	return 0;
}

static int osd_restore(struct device *dev)
{
	osd_restore_hw();
	return 0;
}
static int osd_pm_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	return osd_suspend(pdev, PMSG_SUSPEND);
}
static int osd_pm_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	return osd_resume(pdev);
}
#endif

static int osd_probe(struct platform_device *pdev)
{
	struct fb_info *fbi = NULL;
	const struct vinfo_s *vinfo;
	struct fb_var_screeninfo *var;
	struct fb_fix_screeninfo *fix;
	int  index, bpp;
	struct osd_fb_dev_s *fbdev = NULL;
	enum vmode_e current_mode = VMODE_MASK;
	enum vmode_e logo_init = 0;
	const void *prop;
	int prop_idx = 0;
	const char *str;
	int i;
	int ret = 0;

	/* get interrupt resource */
	int_viu_vsync = platform_get_irq_byname(pdev, "viu-vsync");
	if (int_viu_vsync  == -ENXIO) {
		osd_log_err("cannot get viu irq resource\n");
		goto failed1;
	} else
		osd_log_info("viu vsync irq: %d\n", int_viu_vsync);
#ifdef CONFIG_FB_OSD_VSYNC_RDMA
	int_rdma = platform_get_irq_byname(pdev, "rdma");
	if (int_viu_vsync  == -ENXIO) {
		osd_log_err("cannot get osd rdma irq resource\n");
		goto failed1;
	}
#endif
	/* get buffer size from dt */
	ret = of_property_read_u32_array(pdev->dev.of_node,
			"mem_size", fb_memsize, 2);
	if (ret) {
		osd_log_err("not found mem_size from dtd\n");
		goto failed1;
	}

	/* init reserved memory */
	ret = of_reserved_mem_device_init(&pdev->dev);
	if ((ret != 0) && ((void *)fb_rmem.base == NULL))
		osd_log_err("failed to init reserved memory\n");

	osd_log_dbg("%d, mem_size: 0x%x, 0x%x\n",
			__LINE__, fb_memsize[0], fb_memsize[1]);

	/* get meson-fb resource from dt */
	prop = of_get_property(pdev->dev.of_node, "scale_mode", NULL);
	if (prop) {
		prop_idx = of_read_ulong(prop, 1);
		osd_set_free_scale_mode_hw(DEV_OSD0, prop_idx);
	}
	prop = of_get_property(pdev->dev.of_node, "4k2k_fb", NULL);
	if (prop) {
		prop_idx = of_read_ulong(prop, 1);
		osd_set_4k2k_fb_mode_hw(prop_idx);
	}
	/* get default display mode from dt */
	ret = of_property_read_string(pdev->dev.of_node,
		"display_mode_default", &str);
	if (ret)
		current_mode = VMODE_MASK;
	else
		current_mode = vmode_name_to_mode(str);
	prop = of_get_property(pdev->dev.of_node, "pxp_mode", NULL);
	if (prop) {
		prop_idx = of_read_ulong(prop, 1);
		osd_set_pxp_mode(prop_idx);
	}

	prop = of_get_property(pdev->dev.of_node, "ddr_urgent", NULL);
	if (prop) {
		prop_idx = of_read_ulong(prop, 1);
		osd_set_urgent(0, (prop_idx != 0) ? 1 : 0);
		osd_set_urgent(1, (prop_idx != 0) ? 1 : 0);
	}

	/* if osd_init_hw is not set by logo, set vmode and init osd hw */
	logo_init = osd_get_init_hw_flag();
	if (logo_init == 0) {
		if (current_mode < VMODE_MASK)
			set_current_vmode(current_mode);
		osd_init_hw(0);
	}

	vinfo = get_current_vinfo();
	osd_log_info("%s vinfo:%p\n", __func__, vinfo);
	for (index = 0; index < OSD_COUNT; index++) {
		/* register frame buffer memory */
		fbi = framebuffer_alloc(sizeof(struct osd_fb_dev_s),
				&pdev->dev);
		if (!fbi) {
			ret = -ENOMEM;
			goto failed1;
		}
		fbdev = (struct osd_fb_dev_s *)fbi->par;
		fbdev->fb_index = index;
		fbdev->fb_info = fbi;
		fbdev->dev = pdev;
		mutex_init(&fbdev->lock);
		var = &fbi->var;
		fix = &fbi->fix;
		gp_fbdev_list[index] = fbdev;
		fbdev->fb_len = 0;
		fbdev->fb_mem_paddr = 0;
		fbdev->fb_mem_vaddr = 0;
		if (vinfo) {
			fb_def_var[index].width = vinfo->screen_real_width;
			fb_def_var[index].height = vinfo->screen_real_height;
		}
		/* setup fb0 display size */
		if (index == DEV_OSD0) {
			ret = of_property_read_u32_array(pdev->dev.of_node,
					"display_size_default",
					&var_screeninfo[0], 5);
			if (ret)
				osd_log_info("not found display_size_default\n");
			else {
				fb_def_var[index].xres = var_screeninfo[0];
				fb_def_var[index].yres = var_screeninfo[1];
				fb_def_var[index].xres_virtual =
					var_screeninfo[2];
				fb_def_var[index].yres_virtual =
					var_screeninfo[3];
				fb_def_var[index].bits_per_pixel =
					var_screeninfo[4];
				osd_log_info("init fbdev bpp is:%d\n",
					fb_def_var[index].bits_per_pixel);
				if (fb_def_var[index].bits_per_pixel > 32)
					fb_def_var[index].bits_per_pixel = 32;
			}
		}

		_fbdev_set_default(fbdev, index);
		if (NULL == fbdev->color) {
			osd_log_err("fbdev->color NULL\n");
			ret = -ENOENT;
			goto failed1;
		}
		bpp = (fbdev->color->color_index > 8 ?
				(fbdev->color->color_index > 16 ?
				(fbdev->color->color_index > 24 ?
				 4 : 3) : 2) : 1);
		fix->line_length = var->xres_virtual * bpp;
		fix->smem_start = fbdev->fb_mem_paddr;
		fix->smem_len = fbdev->fb_len;
		if (fb_alloc_cmap(&fbi->cmap, 16, 0) != 0) {
			osd_log_err("unable to allocate color map memory\n");
			ret = -ENOMEM;
			goto failed2;
		}
		fbi->pseudo_palette = kmalloc(sizeof(u32) * 16, GFP_KERNEL);
		if (!(fbi->pseudo_palette)) {
			osd_log_err("unable to allocate pseudo palette mem\n");
			ret = -ENOMEM;
			goto failed2;
		}
		memset(fbi->pseudo_palette, 0, sizeof(u32) * 16);
		fbi->fbops = &osd_ops;
		fbi->screen_base = (char __iomem *)fbdev->fb_mem_vaddr;
		fbi->screen_size = fix->smem_len;
		if (vinfo)
			set_default_display_axis(&fbdev->fb_info->var,
					&fbdev->osd_ctl, vinfo);
		osd_check_var(var, fbi);
		/* register frame buffer */
		register_framebuffer(fbi);
		/* create device attribute files */
		for (i = 0; i < ARRAY_SIZE(osd_attrs); i++)
			ret = device_create_file(fbi->dev, &osd_attrs[i]);
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING;
	early_suspend.suspend = osd_early_suspend;
	early_suspend.resume = osd_late_resume;
	register_early_suspend(&early_suspend);
#endif

	/* init osd reverse */
	if (osd_info.index == DEV_ALL) {
		osd_set_reverse_hw(0, osd_info.osd_reverse);
		osd_set_reverse_hw(1, osd_info.osd_reverse);
	} else
		osd_set_reverse_hw(osd_info.index, osd_info.osd_reverse);

	/* register vout client */
	vout_register_client(&osd_notifier_nb);

	osd_log_info("osd probe OK\n");
	return 0;
failed2:
	fb_dealloc_cmap(&fbi->cmap);
failed1:
	osd_log_err("osd probe failed.\n");
	return ret;
}

static int osd_remove(struct platform_device *pdev)
{
	struct fb_info *fbi;
	int i;
	osd_log_info("osd_remove.\n");
	if (!pdev)
		return -ENODEV;
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&early_suspend);
#endif
	vout_unregister_client(&osd_notifier_nb);
	for (i = 0; i < OSD_COUNT; i++) {
		int j;
		if (gp_fbdev_list[i]) {
			struct osd_fb_dev_s *fbdev = gp_fbdev_list[i];
			fbi = fbdev->fb_info;
			for (j = 0; j < ARRAY_SIZE(osd_attrs); j++)
				device_remove_file(fbi->dev, &osd_attrs[j]);
			iounmap(fbdev->fb_mem_vaddr);
			if (i == DEV_OSD0 && osd_get_afbc()) {
				for (j = 1; j < OSD_MAX_BUF_NUM; j++)
					iounmap(fbdev->fb_mem_afbc_vaddr[j]);
			}
			kfree(fbi->pseudo_palette);
			fb_dealloc_cmap(&fbi->cmap);
			unregister_framebuffer(fbi);
			framebuffer_release(fbi);
		}
	}
	return 0;
}

static void osd_shutdown(struct platform_device *pdev)
{
	if (!osd_shutdown_flag) {
		osd_shutdown_flag = 1;
		osd_shutdown_hw();
	}
	return;
}

/* Process kernel command line parameters */
static int __init osd_setup_attribute(char *options)
{
	char *this_opt = NULL;
	int r = 0;
	unsigned long res;

	if (!options || !*options)
		goto exit;
	while (!r && (this_opt = strsep(&options, ",")) != NULL) {
		if (!strncmp(this_opt, "xres:", 5)) {
			r = kstrtol(this_opt + 5, 0, &res);
			fb_def_var[0].xres = fb_def_var[0].xres_virtual = res;
		} else if (!strncmp(this_opt, "yres:", 5)) {
			r = kstrtol(this_opt + 5, 0, &res);
			fb_def_var[0].yres = fb_def_var[0].yres_virtual = res;
		} else {
			osd_log_info("invalid option\n");
			r = -1;
		}
	}
exit:
	return r;
}

static int rmem_fb_device_init(struct reserved_mem *rmem, struct device *dev)
{
	return 0;
}

static const struct reserved_mem_ops rmem_fb_ops = {
	.device_init = rmem_fb_device_init,
};

static int __init rmem_fb_setup(struct reserved_mem *rmem)
{
	phys_addr_t align = PAGE_SIZE;
	phys_addr_t mask = align - 1;
	if ((rmem->base & mask) || (rmem->size & mask)) {
		pr_err("Reserved memory: incorrect alignment of region\n");
		return -EINVAL;
	}
	fb_rmem.base = rmem->base;
	fb_rmem.size = rmem->size;
	rmem->ops = &rmem_fb_ops;
	osd_log_dbg("Reserved memory: created fb at 0x%p, size %ld MiB\n",
		     (void *)rmem->base, (unsigned long)rmem->size / SZ_1M);
	return 0;
}
RESERVEDMEM_OF_DECLARE(fb, "amlogic, fb-memory", rmem_fb_setup);

static const struct of_device_id meson_fb_dt_match[] = {
	{.compatible = "amlogic, meson-fb",},
	{},
};

#ifdef CONFIG_HIBERNATION
const struct dev_pm_ops osd_pm = {
	.freeze		= osd_freeze,
	.thaw		= osd_thaw,
	.restore	= osd_restore,
	.suspend	= osd_pm_suspend,
	.resume		= osd_pm_resume,
};
#endif

static struct platform_driver osd_driver = {
	.probe      = osd_probe,
	.remove     = osd_remove,
	.shutdown = osd_shutdown,
#ifdef CONFIG_PM
	.suspend  = osd_suspend,
	.resume    = osd_resume,
#endif
	.driver     = {
		.name   = "meson-fb",
		.of_match_table = meson_fb_dt_match,
#ifdef CONFIG_HIBERNATION
		.pm     = &osd_pm,
#endif
	}
};

static int __init osd_init_module(void)
{
	char *option;
	osd_log_info("%s\n", __func__);
	if (fb_get_options("meson-fb", &option)) {
		osd_log_err("failed to get meson-fb options!\n");
		return -ENODEV;
	}
	osd_setup_attribute(option);
	if (platform_driver_register(&osd_driver)) {
		osd_log_err("failed to register OSD driver!\n");
		return -ENODEV;
	}
#ifdef CONFIG_INSTABOOT
	INIT_LIST_HEAD(&osd_realdata_ops.node);
	register_instaboot_realdata_ops(&osd_realdata_ops);
#endif
	return 0;
}

static void __exit osd_exit_module(void)
{
	osd_log_info("%s\n", __func__);
	platform_driver_unregister(&osd_driver);
}

module_init(osd_init_module);
module_exit(osd_exit_module);

MODULE_AUTHOR("Platform-BJ <platform.bj@amlogic.com>");
MODULE_DESCRIPTION("OSD Module");
MODULE_LICENSE("GPL");

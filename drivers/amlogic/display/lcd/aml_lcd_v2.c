/*
 * Amlogic LCD Driver
 *
 * Author:
 *
 * Copyright (C) 2012 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/amlogic/vout/lcd.h>
#include <linux/amlogic/vout/vinfo.h>
#include <linux/amlogic/vout/vout_notify.h>
#include <linux/amlogic/vout/lcd_aml.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <mach/am_regs.h>
#include <mach/mlvds_regs.h>
#include <mach/clock.h>
#include <asm/fiq.h>
#include <linux/delay.h>
#include <plat/regops.h>
#include <mach/am_regs.h>
#include <mach/hw_enc_clk_config.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/gpio.h>
#include <linux/amlogic/panel.h>
#include <linux/of.h>

static struct class *aml_lcd_clsp;

#define PANEL_NAME	"panel"

#define CLOCK_ENABLE_DELAY			500
#define CLOCK_DISABLE_DELAY			50
#define PWM_ENABLE_DELAY			20
#define PWM_DISABLE_DELAY			20
#define PANEL_POWER_ON_DELAY		50
#define PANEL_POWER_OFF_DELAY		0
#define BACKLIGHT_POWER_ON_DELAY	0
#define BACKLIGHT_POWER_OFF_DELAY	200

#define H_ACTIVE			1920
#define V_ACTIVE			1080
#define H_PERIOD			2200
#define V_PERIOD			1125
#define VIDEO_ON_PIXEL	148
#define VIDEO_ON_LINE	41
#define CLK_UTIL_VID_PLL_DIV_1      0
#define CLK_UTIL_VID_PLL_DIV_2      1
#define CLK_UTIL_VID_PLL_DIV_3      2
#define CLK_UTIL_VID_PLL_DIV_3p5    3
#define CLK_UTIL_VID_PLL_DIV_3p75   4
#define CLK_UTIL_VID_PLL_DIV_4      5
#define CLK_UTIL_VID_PLL_DIV_5      6
#define CLK_UTIL_VID_PLL_DIV_6      7
#define CLK_UTIL_VID_PLL_DIV_6p25   8
#define CLK_UTIL_VID_PLL_DIV_7      9
#define CLK_UTIL_VID_PLL_DIV_7p5    10
#define CLK_UTIL_VID_PLL_DIV_12     11
#define CLK_UTIL_VID_PLL_DIV_14     12
#define CLK_UTIL_VID_PLL_DIV_15     13

/* defines for bits&width */
#define VBO_VIN_YUV_GBR 0
#define VBO_VIN_YVU_GRB 1
#define VBO_VIN_UYV_BGR 2
#define VBO_VIN_UVY_BRG 3
#define VBO_VIN_VYU_RGB 4
#define VBO_VIN_VUY_RBG 5

#define VBO_VIN_30BPP	0
#define VBO_VIN_24BPP   1
#define VBO_VIN_18BPP   2

//VBYONE_CTRL_L
//`define VBO_CTRL_L             8'h60
    #define VBO_ENABLE_BIT  0
    #define VBO_EBABLE_WID  1

    #define VBO_ALN_SHIFT_BIT     1
    #define VBO_ALN_SHIFT_WID     1

    #define VBO_LFSR_BYPASS_BIT   2
    #define VBO_LFSR_BYPASS_WID   1

    #define VBO_VDE_EXTEND_BIT    3
    #define VBO_VDE_EXTEND_WID    1

    #define VBO_HSYNC_SYNC_MODE_BIT   4
    #define VBO_HSYNC_SYNC_MODE_WID   2

    #define VBO_VSYNC_SYNC_MODE_BIT   6
    #define VBO_VSYNC_SYNC_MODE_WID   2

    #define VBO_FSM_CTRL_BIT      8
    #define VBO_FSM_CTRL_WID      3

    #define VBO_CTL_MODE_BIT      11
    #define VBO_CTL_MODE_WID      5

//`define VBO_CTRL_H             8'h61
    #define VBO_CTL_MODE2_BIT     0
    #define VBO_CTL_MODE2_WID     4

    #define VBO_B8B10_CTRL_BIT    4
    #define VBO_B8B10_CTRL_WID    3

    #define VBO_VIN2ENC_TMSYNC_MODE_BIT 8
    #define VBO_VIN2ENC_TMSYNC_MODE_WID 1

    #define VBO_VIN2ENC_HVSYNC_DLY_BIT  9
    #define VBO_VIN2ENC_HVSYNC_DLY_WID  1

    #define VBO_POWER_ON_BIT  12
    #define VBO_POWER_ON_WID  1

    #define VBO_PLL_LOCK_BIT  13
    #define VBO_PLL_LOCK_WID  1

//`define VBO_SOFT_RST           8'h62
   #define  VBO_SOFT_RST_BIT      0
   #define  VBO_SOFT_RST_WID      9

//`define VBO_LANES              8'h63
   #define  VBO_LANE_NUM_BIT      0
   #define  VBO_LANE_NUM_WID      3

   #define  VBO_LANE_REGION_BIT   4
   #define  VBO_LANE_REGION_WID   2

   #define  VBO_SUBLANE_NUM_BIT   8
   #define  VBO_SUBLANE_NUM_WID   3

   #define  VBO_BYTE_MODE_BIT     11
   #define  VBO_BYTE_MODE_WID     2

//`define VBO_VIN_CTRL           8'h64
   #define  VBO_VIN_SYNC_CTRL_BIT 0
   #define  VBO_VIN_SYNC_CTRL_WID 2

   #define  VBO_VIN_HSYNC_POL_BIT 4
   #define  VBO_VIN_HSYNC_POL_WID 1

   #define  VBO_VIN_VSYNC_POL_BIT 5
   #define  VBO_VIN_VSYNC_POL_WID 1

   #define  VBO_VOUT_HSYNC_POL_BIT 6
   #define  VBO_VOUT_HSYNC_POL_WID 1

   #define  VBO_VOUT_VSYNC_POL_BIT 7
   #define  VBO_VOUT_VSYNC_POL_WID 1

   #define  VBO_VIN_PACK_BIT      8
   #define  VBO_VIN_PACK_WID      3

   #define  VBO_VIN_BPP_BIT      11
   #define  VBO_VIN_BPP_WID       2

//`define VBO_ACT_VSIZE          8'h65
//`define VBO_REGION_00          8'h66
//`define VBO_REGION_01          8'h67
//`define VBO_REGION_02          8'h68
//`define VBO_REGION_03          8'h69
//`define VBO_VBK_CTRL_0         8'h6a
//`define VBO_VBK_CTRL_1         8'h6b
//`define VBO_HBK_CTRL           8'h6c
//`define VBO_PXL_CTRL           8'h6d
    #define  VBO_PXL_CTR0_BIT     0
    #define  VBO_PXL_CTR0_WID     4
    #define  VBO_PXL_CTR1_BIT     4
    #define  VBO_PXL_CTR1_WID     4

//`define VBO_LANE_SKEW_L        8'h6e
//`define VBO_LANE_SKEW_H        8'h6f
//`define VBO_GCLK_LANE_CTRL_L   8'h70
//`define VBO_GCLK_LANE_CTRL_H   8'h71
//`define VBO_GCLK_MAIN_CTRL     8'h72
//`define VBO_STATUS_L           8'h73
//`define VBO_STATUS_H           8'h74

//`define LCD_PORT_SWAP          8'h75
typedef struct {
	Lcd_Config_t conf;
	vinfo_t lcd_info;
} lcd_dev_t;
static lcd_dev_t *pDev = NULL;

static vmode_t cur_vmode = VMODE_1080P;

static int pn_swap = 0;
MODULE_PARM_DESC(pn_swap, "\n pn_swap \n");
module_param(pn_swap, int, 0664);

static int dual_port = 1;
MODULE_PARM_DESC(dual_port, "\n dual_port \n");
module_param(dual_port, int, 0664);

static int bit_num_flag = 1;
static int bit_num = 1;
MODULE_PARM_DESC(bit_num, "\n bit_num \n");
module_param(bit_num, int, 0664);

static int lvds_repack_flag = 1;
static int lvds_repack = 1;
MODULE_PARM_DESC(lvds_repack, "\n lvds_repack \n");
module_param(lvds_repack, int, 0664);

static int port_reverse_flag = 1;
static int port_reverse = 1;
MODULE_PARM_DESC(port_reverse, "\n port_reverse \n");
module_param(port_reverse, int, 0664);

int flaga = 0;
MODULE_PARM_DESC(flaga, "\n flaga \n");
module_param(flaga, int, 0664);
EXPORT_SYMBOL(flaga);

int flagb = 0;
MODULE_PARM_DESC(flagb, "\n flagb \n");
module_param(flagb, int, 0664);
EXPORT_SYMBOL(flagb);

int flagc = 0;
MODULE_PARM_DESC(flagc, "\n flagc \n");
module_param(flagc, int, 0664);
EXPORT_SYMBOL(flagc);

int flagd = 0;
MODULE_PARM_DESC(flagd, "\n flagd \n");
module_param(flagd, int, 0664);
EXPORT_SYMBOL(flagd);


void vclk_set_encl_ttl(vmode_t vmode)
{
	int hdmi_clk_out;
	//int hdmi_vx1_clk_od1;
	int vx1_phy_div;
	int encl_div;
	unsigned int xd;
	//no used, od2 must >= od3.
	//hdmi_vx1_clk_od1 = 1; //OD1 always 1

	//pll_video.pl 3500 pll_out
	switch(vmode) {
		case VMODE_720P: //total: 1650*750 pixel clk = 74.25MHz,phy_clk(s)=(pclk*7)= 519.75 = 1039.5/2
			hdmi_clk_out = 1039.5;
			vx1_phy_div  = 2/2;
			encl_div     = vx1_phy_div*7;
			break;

		default:
			pr_err("Error video format!\n");
			return;
	}
	//if(set_hdmi_dpll(hdmi_clk_out,hdmi_vx1_clk_od1)) {
	if(set_hdmi_dpll(hdmi_clk_out,0)) {
		pr_err("Unsupported HDMI_DPLL out frequency!\n");
		return;
	}

	//configure vid_clk_div_top
	if((encl_div%14)==0){//7*even
		clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_14);
		xd = encl_div/14;
	}else if((encl_div%7)==0){ //7*odd
		clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_7);
		xd = encl_div/7;
	}else{ //3.5*odd
		clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_3p5);
		xd = encl_div/3.5;
	}

	//configure crt_video
	set_crt_video_enc(0,0,xd);  //configure crt_video V1: inSel=vid_pll_clk(0),DivN=xd)
	enable_crt_video_encl(1,0); //select and enable the output
}


static void set_pll_ttl(Lcd_Config_t *pConf)
{
	vclk_set_encl_ttl(pDev->lcd_info.mode);
//	set_video_spread_spectrum(pll_sel, ss_level);
}

static void venc_set_ttl(Lcd_Config_t *pConf)
{
	printk("%s\n", __FUNCTION__);
	aml_write_reg32(P_ENCL_VIDEO_EN,           0);
	aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL,
	   (0<<0) |    // viu1 select ENCL
	   (0<<2)      // viu2 select ENCL
	);
	aml_write_reg32(P_ENCL_VIDEO_MODE,        0);
	aml_write_reg32(P_ENCL_VIDEO_MODE_ADV,    0x0418);

	// bypass filter
	aml_write_reg32(P_ENCL_VIDEO_FILT_CTRL,    0x1000);
	aml_write_reg32(P_VENC_DVI_SETTING,        0x11);
	aml_write_reg32(P_VENC_VIDEO_PROG_MODE,    0x100);

	aml_write_reg32(P_ENCL_VIDEO_MAX_PXCNT,    pConf->lcd_basic.h_period - 1);
	aml_write_reg32(P_ENCL_VIDEO_MAX_LNCNT,    pConf->lcd_basic.v_period - 1);

	aml_write_reg32(P_ENCL_VIDEO_HAVON_BEGIN,  pConf->lcd_timing.video_on_pixel);
	aml_write_reg32(P_ENCL_VIDEO_HAVON_END,    pConf->lcd_basic.h_active - 1 + pConf->lcd_timing.video_on_pixel);
	aml_write_reg32(P_ENCL_VIDEO_VAVON_BLINE,  pConf->lcd_timing.video_on_line);
	aml_write_reg32(P_ENCL_VIDEO_VAVON_ELINE,  pConf->lcd_basic.v_active + 3  + pConf->lcd_timing.video_on_line);

	aml_write_reg32(P_ENCL_VIDEO_HSO_BEGIN,    15);
	aml_write_reg32(P_ENCL_VIDEO_HSO_END,      31);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BEGIN,    15);
	aml_write_reg32(P_ENCL_VIDEO_VSO_END,      31);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BLINE,    0);
	aml_write_reg32(P_ENCL_VIDEO_VSO_ELINE,    2);

	// enable ENCL
	aml_write_reg32(P_ENCL_VIDEO_EN,           1);
}

static void set_tcon_ttl(Lcd_Config_t *pConf)
{
	Lcd_Timing_t *tcon_adr = &(pConf->lcd_timing);

	//set_lcd_gamma_table_ttl(pConf->lcd_effect.GammaTableR, LCD_H_SEL_R);
	//set_lcd_gamma_table_ttl(pConf->lcd_effect.GammaTableG, LCD_H_SEL_G);
	//set_lcd_gamma_table_ttl(pConf->lcd_effect.GammaTableB, LCD_H_SEL_B);

	aml_write_reg32(P_L_GAMMA_CNTL_PORT, 0);//pConf->lcd_effect.gamma_cntl_port);
	//aml_write_reg32(P_GAMMA_VCOM_HSWITCH_ADDR, pConf->lcd_effect.gamma_vcom_hswitch_addr);

	aml_write_reg32(P_L_RGB_BASE_ADDR,   0xf0);//pConf->lcd_effect.rgb_base_addr);
	aml_write_reg32(P_L_RGB_COEFF_ADDR,  0x74a);//pConf->lcd_effect.rgb_coeff_addr);
	//aml_write_reg32(P_POL_CNTL_ADDR,   pConf->lcd_timing.pol_cntl_addr);
	if(pConf->lcd_basic.lcd_bits == 8)
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x400);
	else
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x600);

	aml_write_reg32(P_L_STH1_HS_ADDR,    tcon_adr->sth1_hs_addr);
	aml_write_reg32(P_L_STH1_HE_ADDR,    tcon_adr->sth1_he_addr);
	aml_write_reg32(P_L_STH1_VS_ADDR,    tcon_adr->sth1_vs_addr);
	aml_write_reg32(P_L_STH1_VE_ADDR,    tcon_adr->sth1_ve_addr);

	aml_write_reg32(P_L_OEH_HS_ADDR,     tcon_adr->oeh_hs_addr);
	aml_write_reg32(P_L_OEH_HE_ADDR,     tcon_adr->oeh_he_addr);
	aml_write_reg32(P_L_OEH_VS_ADDR,     tcon_adr->oeh_vs_addr);
	aml_write_reg32(P_L_OEH_VE_ADDR,     tcon_adr->oeh_ve_addr);

	aml_write_reg32(P_L_VCOM_HSWITCH_ADDR, tcon_adr->vcom_hswitch_addr);
	aml_write_reg32(P_L_VCOM_VS_ADDR,    tcon_adr->vcom_vs_addr);
	aml_write_reg32(P_L_VCOM_VE_ADDR,    tcon_adr->vcom_ve_addr);

	aml_write_reg32(P_L_CPV1_HS_ADDR,    tcon_adr->cpv1_hs_addr);
	aml_write_reg32(P_L_CPV1_HE_ADDR,    tcon_adr->cpv1_he_addr);
	aml_write_reg32(P_L_CPV1_VS_ADDR,    tcon_adr->cpv1_vs_addr);
	aml_write_reg32(P_L_CPV1_VE_ADDR,    tcon_adr->cpv1_ve_addr);

	aml_write_reg32(P_L_STV1_HS_ADDR,    tcon_adr->stv1_hs_addr);
	aml_write_reg32(P_L_STV1_HE_ADDR,    tcon_adr->stv1_he_addr);
	aml_write_reg32(P_L_STV1_VS_ADDR,    tcon_adr->stv1_vs_addr);
	aml_write_reg32(P_L_STV1_VE_ADDR,    tcon_adr->stv1_ve_addr);

	aml_write_reg32(P_L_OEV1_HS_ADDR,    tcon_adr->oev1_hs_addr);
	aml_write_reg32(P_L_OEV1_HE_ADDR,    tcon_adr->oev1_he_addr);
	aml_write_reg32(P_L_OEV1_VS_ADDR,    tcon_adr->oev1_vs_addr);
	aml_write_reg32(P_L_OEV1_VE_ADDR,    tcon_adr->oev1_ve_addr);

	aml_write_reg32(P_L_INV_CNT_ADDR,    tcon_adr->inv_cnt_addr);
	aml_write_reg32(P_L_TCON_MISC_SEL_ADDR,     tcon_adr->tcon_misc_sel_addr);
	aml_write_reg32(P_L_DUAL_PORT_CNTL_ADDR, tcon_adr->dual_port_cntl_addr);

	aml_write_reg32(P_VPP_MISC, aml_read_reg32(P_VPP_MISC) & ~(VPP_OUT_SATURATE));
}

void set_ttl_pinmux(void)
{
	aml_set_reg32_bits(P_PERIPHS_PIN_MUX_0, 0x3f, 0, 6);
	//aml_set_reg32_bits(P_PERIPHS_PIN_MUX_0, 0x3, 18, 2);
	//aml_set_reg32_bits(P_PERIPHS_PIN_MUX_0, 0x1, 29, 1);
	aml_set_reg32_bits(P_PERIPHS_PIN_MUX_8, 0x7, 22, 3);
	aml_set_reg32_bits(P_PERIPHS_PIN_MUX_8, 0x1, 28, 1);

	//aml_set_reg32_bits(P_PAD_PULL_UP_EN_REG4, 0x1, 26, 1);
	//aml_set_reg32_bits(P_PAD_PULL_UP_REG4, 0x1, 26, 1);
}

void vpp_set_matrix_ycbcr2rgb (int vd1_or_vd2_or_post, int mode)
{
	if (vd1_or_vd2_or_post == 0){ //vd1
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 1, 5, 1);
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 1, 8, 2);
	}else if (vd1_or_vd2_or_post == 1){ //vd2
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 1, 4, 1);
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 2, 8, 2);
	}else{
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 1, 0, 1);
		aml_set_reg32_bits (P_VPP_MATRIX_CTRL, 0, 8, 2);
		if (mode == 0){
			aml_set_reg32_bits(P_VPP_MATRIX_CTRL, 1, 1, 2);
		}else if (mode == 1){
			aml_set_reg32_bits(P_VPP_MATRIX_CTRL, 0, 1, 2);
		}
	}

	if (mode == 0){ //ycbcr not full range, 601 conversion
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET0_1, 0x0064C8FF);
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET2, 0x006400C8);

		//1.164     0       1.596
		//1.164   -0.392    -0.813
		//1.164   2.017     0
		aml_write_reg32(P_VPP_MATRIX_COEF00_01, 0x04000000);
		aml_write_reg32(P_VPP_MATRIX_COEF02_10, 0x059C0400);
		aml_write_reg32(P_VPP_MATRIX_COEF11_12, 0x1EA01D24);
		aml_write_reg32(P_VPP_MATRIX_COEF20_21, 0x04000718);
		aml_write_reg32(P_VPP_MATRIX_COEF22, 0x00000000);
		aml_write_reg32(P_VPP_MATRIX_OFFSET0_1, 0x00000000);
		aml_write_reg32(P_VPP_MATRIX_OFFSET2, 0x00000000);
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET0_1, 0x00000E00);
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET2, 0x00000E00);
	}else if (mode == 1) {//ycbcr full range, 601 conversion
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET0_1, 0x0000600);
		aml_write_reg32(P_VPP_MATRIX_PRE_OFFSET2, 0x0600);

		//1     0       1.402
		//1   -0.34414  -0.71414
		//1   1.772     0
		aml_write_reg32(P_VPP_MATRIX_COEF00_01, (0x400 << 16) |0);
		aml_write_reg32(P_VPP_MATRIX_COEF02_10, (0x59c << 16) |0x400);
		aml_write_reg32(P_VPP_MATRIX_COEF11_12, (0x1ea0 << 16) |0x1d25);
		aml_write_reg32(P_VPP_MATRIX_COEF20_21, (0x400 << 16) |0x717);
		aml_write_reg32(P_VPP_MATRIX_COEF22, 0x0);
		aml_write_reg32(P_VPP_MATRIX_OFFSET0_1, 0x0);
		aml_write_reg32(P_VPP_MATRIX_OFFSET2, 0x0);
	}
}

static void set_tcon_lvds(Lcd_Config_t *pConf)
{
	vpp_set_matrix_ycbcr2rgb(2, 0);
	aml_write_reg32(P_ENCL_VIDEO_RGBIN_CTRL, 3);
	aml_write_reg32(P_L_RGB_BASE_ADDR, 0);
	aml_write_reg32(P_L_RGB_COEFF_ADDR, 0x400);

	if(pConf->lcd_basic.lcd_bits == 8)
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x400);
	else if(pConf->lcd_basic.lcd_bits == 6)
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x600);
	else
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0);

	aml_write_reg32(P_VPP_MISC, aml_read_reg32(P_VPP_MISC) & ~(VPP_OUT_SATURATE));
}



//new lvd_vx1_phy config
void lvds_phy_config(int lvds_vx1_sel)
{
	//debug
	aml_write_reg32(P_VPU_VLOCK_GCLK_EN, 7);
	aml_write_reg32(P_VPU_VLOCK_ADJ_EN_SYNC_CTRL, 0x108010ff);
	aml_write_reg32(P_VPU_VLOCK_CTRL, 0xe0f50f1b);
	//debug

	if(lvds_vx1_sel == 0){ //lvds
		aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL1, 0x6c6cca80);
		aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL2, 0x0000006c);
		aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL3, 0x0fff0800);
		//od   clk 1039.5 / 2 = 519.75 = 74.25*7
		aml_write_reg32(P_HHI_LVDS_TX_PHY_CNTL0, 0x0fff0040);
	}else{

	}

}

void vclk_set_encl_lvds(vmode_t vmode, int lvds_ports)
{
	int hdmi_clk_out;
	//int hdmi_vx1_clk_od1;
	int vx1_phy_div;
	int encl_div;
	unsigned int xd;
	//no used, od2 must >= od3.
	//hdmi_vx1_clk_od1 = 1; //OD1 always 1

	if(lvds_ports<2){
		//pll_video.pl 3500 pll_out
		switch(vmode) {
			case VMODE_1080P: //total: 2200x1125 pixel clk = 148.5MHz,phy_clk(s)=(pclk*7)= 1039.5 = 2079/2
				hdmi_clk_out = 2079;
				vx1_phy_div  = 2/2;
				encl_div     = vx1_phy_div*7;
				break;
			default:
				pr_err("Error video format!\n");
				return;
		}
		//if(set_hdmi_dpll(hdmi_clk_out,hdmi_vx1_clk_od1)) {
		if(set_hdmi_dpll(hdmi_clk_out,0)) {
			pr_err("Unsupported HDMI_DPLL out frequency!\n");
			return;
		}

		if(lvds_ports==1) //dual port
			vx1_phy_div = vx1_phy_div*2;
	}else if(lvds_ports>=2) {
		pr_err("Quad-LVDS is not supported!\n");
		return;
	}

	//configure vid_clk_div_top
	if((encl_div%14)==0){//7*even
		clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_14);
		xd = encl_div/14;
	}else if((encl_div%7)==0){ //7*odd
		clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_7);
		xd = encl_div/7;
	}else{ //3.5*odd
		clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_3p5);
		xd = encl_div/3.5;
	}
	//for lvds phy clock and enable decoupling FIFO
	aml_write_reg32(P_HHI_LVDS_TX_PHY_CNTL1,((3<<6)|((vx1_phy_div-1)<<1)|1)<<24);

	//config lvds phy
	lvds_phy_config(0);
	//configure crt_video
	set_crt_video_enc(0,0,xd);  //configure crt_video V1: inSel=vid_pll_clk(0),DivN=xd)
	enable_crt_video_encl(1,0); //select and enable the output
}

static void set_pll_lvds(Lcd_Config_t *pConf)
{
	pr_info("%s\n", __FUNCTION__);

	vclk_set_encl_lvds(pDev->lcd_info.mode, pConf->lvds_mlvds_config.lvds_config->dual_port);
	aml_write_reg32( P_HHI_VIID_DIVIDER_CNTL, ((aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL) & ~(0x7 << 8)) | (2 << 8) | (0<<10)) );
	aml_write_reg32(P_LVDS_GEN_CNTL, (aml_read_reg32(P_LVDS_GEN_CNTL)| (1 << 3) | (3<< 0)));
}

static void venc_set_lvds(Lcd_Config_t *pConf)
{
	pr_info("%s\n", __FUNCTION__);
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6TV
	aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (0<<0) |    // viu1 select encl
											(0<<2) );    // viu2 select encl
#endif
	aml_write_reg32(P_ENCL_VIDEO_EN, 0);
	//int havon_begin = 80;
	aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (0<<0) |    // viu1 select encl
											(0<<2) );     // viu2 select encl
	aml_write_reg32(P_ENCL_VIDEO_MODE, 0); // Enable Hsync and equalization pulse switch in center; bit[14] cfg_de_v = 1
	aml_write_reg32(P_ENCL_VIDEO_MODE_ADV,     0x0418); // Sampling rate: 1

	// bypass filter
	aml_write_reg32(P_ENCL_VIDEO_FILT_CTRL, 0x1000);

	aml_write_reg32(P_ENCL_VIDEO_MAX_PXCNT, pConf->lcd_basic.h_period - 1);
	if(cur_vmode == VMODE_1080P_50HZ)
		aml_write_reg32(P_ENCL_VIDEO_MAX_LNCNT, 1350 - 1);
	else
		aml_write_reg32(P_ENCL_VIDEO_MAX_LNCNT, pConf->lcd_basic.v_period - 1);

	aml_write_reg32(P_ENCL_VIDEO_HAVON_BEGIN, pConf->lcd_timing.video_on_pixel);
	aml_write_reg32(P_ENCL_VIDEO_HAVON_END, pConf->lcd_basic.h_active - 1 + pConf->lcd_timing.video_on_pixel);
	aml_write_reg32(P_ENCL_VIDEO_VAVON_BLINE,	pConf->lcd_timing.video_on_line);
	aml_write_reg32(P_ENCL_VIDEO_VAVON_ELINE,	pConf->lcd_basic.v_active - 1  + pConf->lcd_timing.video_on_line);

	aml_write_reg32(P_ENCL_VIDEO_HSO_BEGIN,	pConf->lcd_timing.sth1_hs_addr);//10);
	aml_write_reg32(P_ENCL_VIDEO_HSO_END,	pConf->lcd_timing.sth1_he_addr);//20);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BEGIN,	pConf->lcd_timing.stv1_hs_addr);//10);
	aml_write_reg32(P_ENCL_VIDEO_VSO_END,	pConf->lcd_timing.stv1_he_addr);//20);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BLINE,	pConf->lcd_timing.stv1_vs_addr);//2);
	aml_write_reg32(P_ENCL_VIDEO_VSO_ELINE,	pConf->lcd_timing.stv1_ve_addr);//4);
		aml_write_reg32(P_ENCL_VIDEO_RGBIN_CTRL, 0);

		// enable encl
		aml_write_reg32(P_ENCL_VIDEO_EN, 1);
	}


static void venc_set_vx1(Lcd_Config_t *pConf)
{
	pr_info("%s\n", __FUNCTION__);

	aml_write_reg32(P_ENCL_VIDEO_EN, 0);
	//int havon_begin = 80;
	aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (0<<0) |    // viu1 select encl
											(3<<2) );     // viu2 select encl
	aml_write_reg32(P_ENCL_VIDEO_MODE, 40);//0); // Enable Hsync and equalization pulse switch in center; bit[14] cfg_de_v = 1
	aml_write_reg32(P_ENCL_VIDEO_MODE_ADV,     0x18);//0x0418); // Sampling rate: 1

	// bypass filter
	aml_write_reg32(P_ENCL_VIDEO_FILT_CTRL, 0x1000); //??

	aml_write_reg32(P_ENCL_VIDEO_MAX_PXCNT, 3840+560-1);//pConf->lcd_basic.h_period - 1);
	aml_write_reg32(P_ENCL_VIDEO_MAX_LNCNT, 2160+90-1);//pConf->lcd_basic.v_period - 1);

	aml_write_reg32(P_ENCL_VIDEO_HAVON_BEGIN, 560-3);//pConf->lcd_timing.video_on_pixel);
	aml_write_reg32(P_ENCL_VIDEO_HAVON_END, 3839+560-3);//pConf->lcd_basic.h_active - 1 + pConf->lcd_timing.video_on_pixel);
	aml_write_reg32(P_ENCL_VIDEO_VAVON_BLINE,	90);//pConf->lcd_timing.video_on_line);
	aml_write_reg32(P_ENCL_VIDEO_VAVON_ELINE,	2159+90);//pConf->lcd_basic.v_active - 1  + pConf->lcd_timing.video_on_line);

	aml_write_reg32(P_ENCL_VIDEO_HSO_BEGIN,48-1);//	pConf->lcd_timing.sth1_hs_addr);//10);
	aml_write_reg32(P_ENCL_VIDEO_HSO_END,	48-1+32);//pConf->lcd_timing.sth1_he_addr);//20);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BEGIN,	48-1);//pConf->lcd_timing.stv1_hs_addr);//10);
	aml_write_reg32(P_ENCL_VIDEO_VSO_END,	48-1);//pConf->lcd_timing.stv1_he_addr);//20);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BLINE,	3);//pConf->lcd_timing.stv1_vs_addr);//2);
	aml_write_reg32(P_ENCL_VIDEO_VSO_ELINE,	9);//pConf->lcd_timing.stv1_ve_addr);//4);

	aml_write_reg32(P_ENCL_VIDEO_RGBIN_CTRL, 0);

	// enable encl
	aml_write_reg32(P_ENCL_VIDEO_EN, 1);
}

static void set_control_lvds(Lcd_Config_t *pConf)
{
	pr_info("%s\n", __FUNCTION__);

	if(lvds_repack_flag)
		lvds_repack = (pConf->lvds_mlvds_config.lvds_config->lvds_repack) & 0x1;
	pn_swap = (pConf->lvds_mlvds_config.lvds_config->pn_swap) & 0x1;
	dual_port = (pConf->lvds_mlvds_config.lvds_config->dual_port) & 0x1;
	if(port_reverse_flag)
		port_reverse = (pConf->lvds_mlvds_config.lvds_config->port_reverse) & 0x1;

	if(bit_num_flag){
		switch(pConf->lcd_basic.lcd_bits) {
			case 10: bit_num=0; break;
			case 8: bit_num=1; break;
			case 6: bit_num=2; break;
			case 4: bit_num=3; break;
			default: bit_num=1; break;
		}
	}

	aml_write_reg32(P_MLVDS_CONTROL,  (aml_read_reg32(P_MLVDS_CONTROL) & ~(1 << 0)));  //disable mlvds
	aml_write_reg32(P_LVDS_PACK_CNTL_ADDR,
					( lvds_repack<<0 ) | // repack
					( port_reverse?(0<<2):(1<<2)) | // odd_even
					( 0<<3 ) | // reserve
					( 0<<4 ) | // lsb first
					( pn_swap<<5 ) | // pn swap
					( dual_port<<6 ) | // dual port
					( 0<<7 ) | // use tcon control
					( bit_num<<8 ) | // 0:10bits, 1:8bits, 2:6bits, 3:4bits.
					( 0<<10 ) | //r_select  //0:R, 1:G, 2:B, 3:0
					( 1<<12 ) | //g_select  //0:R, 1:G, 2:B, 3:0
					( 2<<14 ));  //b_select  //0:R, 1:G, 2:B, 3:0;
}

static void init_lvds_phy(Lcd_Config_t *pConf)
{
	//debug
	aml_write_reg32(P_VPU_VLOCK_GCLK_EN, 7);
	aml_write_reg32(P_VPU_VLOCK_ADJ_EN_SYNC_CTRL, 0x108010ff);
	aml_write_reg32(P_VPU_VLOCK_CTRL, 0xe0f50f1b);
	//debug
	aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL1, 0x6c6cca80);
	aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL2, 0x0000006c);
	aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL3, 0x0fff0800);
	//od   clk 1039.5 / 2 = 519.75 = 74.25*7
	aml_write_reg32(P_HHI_LVDS_TX_PHY_CNTL0, 0x0fff0040);
}

int config_vbyone(int lane, int byte, int region, int hsize, int vsize)
{
   int sublane_num;
   int region_size[4];
   int tmp;

   if((lane == 0) || (lane == 3) || (lane == 5) || (lane == 6) || (lane == 7) || (lane>8))
      return 1;
   if((region ==0) || (region==3) || (region>4))
      return 1;
   if(lane%region)
      return 1;
   if((byte<3) || (byte>4))
      return 1;

   sublane_num = lane/region;
   aml_set_reg32_bits(P_VBO_LANES,lane-1,  VBO_LANE_NUM_BIT,    VBO_LANE_NUM_WID);
   aml_set_reg32_bits(P_VBO_LANES,region-1,VBO_LANE_REGION_BIT, VBO_LANE_REGION_WID);
   aml_set_reg32_bits(P_VBO_LANES,sublane_num-1,VBO_SUBLANE_NUM_BIT, VBO_SUBLANE_NUM_WID);
   aml_set_reg32_bits(P_VBO_LANES,byte-1,VBO_BYTE_MODE_BIT, VBO_BYTE_MODE_WID);

   if(region>1)
   {
       region_size[3] = (hsize/lane)*sublane_num;
       tmp = (hsize%lane);
       region_size[0] = region_size[3] + (((tmp/sublane_num)>0) ? sublane_num : (tmp%sublane_num));
       region_size[1] = region_size[3] + (((tmp/sublane_num)>1) ? sublane_num : (tmp%sublane_num));
       region_size[2] = region_size[3] + (((tmp/sublane_num)>2) ? sublane_num : (tmp%sublane_num));
       aml_write_reg32(P_VBO_REGION_00,region_size[0]);
       aml_write_reg32(P_VBO_REGION_01,region_size[1]);
       aml_write_reg32(P_VBO_REGION_02,region_size[2]);
       aml_write_reg32(P_VBO_REGION_03,region_size[3]);
   }
   aml_write_reg32(P_VBO_ACT_VSIZE,vsize);
   //aml_set_reg32_bits(P_VBO_CTRL_H,0x80,VBO_CTL_MODE_BIT,VBO_CTL_MODE_WID);  // different from FBC code!!!
   aml_set_reg32_bits(P_VBO_CTRL_H,0x0,VBO_CTL_MODE2_BIT,VBO_CTL_MODE2_WID); // different from simulation code!!!
   aml_set_reg32_bits(P_VBO_CTRL_H,0x1,VBO_VIN2ENC_HVSYNC_DLY_BIT,VBO_VIN2ENC_HVSYNC_DLY_WID);
   //aml_set_reg32_bits(P_VBO_CTRL_L,enable,VBO_ENABLE_BIT,VBO_EBABLE_WID);

   return 0;
}

void set_vbyone_ctlbits(int p3d_en, int p3d_lr, int mode)
{
	if(mode==0)  //insert at the first pixel
		aml_set_reg32_bits(P_VBO_PXL_CTRL,(1<<p3d_en)|(p3d_lr&0x1),VBO_PXL_CTR0_BIT,VBO_PXL_CTR0_WID);
	else
		aml_set_reg32_bits(P_VBO_VBK_CTRL_0,(1<<p3d_en)|(p3d_lr&0x1),0,2);
}

void set_vbyone_sync_pol(int hsync_pol, int vsync_pol)
{
    aml_set_reg32_bits(P_VBO_VIN_CTRL,hsync_pol,VBO_VIN_HSYNC_POL_BIT,VBO_VIN_HSYNC_POL_WID);
    aml_set_reg32_bits(P_VBO_VIN_CTRL,vsync_pol,VBO_VIN_VSYNC_POL_BIT,VBO_VIN_VSYNC_POL_WID);

    aml_set_reg32_bits(P_VBO_VIN_CTRL,hsync_pol,VBO_VOUT_HSYNC_POL_BIT,VBO_VOUT_HSYNC_POL_WID);
    aml_set_reg32_bits(P_VBO_VIN_CTRL,vsync_pol,VBO_VOUT_VSYNC_POL_BIT,VBO_VOUT_VSYNC_POL_WID);
}

static void set_control_vbyone(Lcd_Config_t *pConf)
{
    int lane, byte, region,  hsize, vsize;//color_fmt,
    int vin_color, vin_bpp;

    hsize = 3840;//pConf->lcd_basic.h_active;
	vsize = 2160;//pConf->lcd_basic.v_active;
    lane = 8;//byte_num;
	byte = 4;//byte_num;
	region = 2;//region_num;
	vin_color = 4;
	vin_bpp   = 0;
    //switch (color_fmt) {
    //    case 0:   //SDVT_VBYONE_18BPP_RGB
    //              vin_color = 4;
    //              vin_bpp   = 2;
    //              break;
    //    case 1:   //SDVT_VBYONE_18BPP_YCBCR444
    //              vin_color = 0;
    //              vin_bpp   = 2;
    //              break;
    //    case 2:   //SDVT_VBYONE_24BPP_RGB
    //              vin_color = 4;
    //              vin_bpp   = 1;
    //              break;
    //    case 3:   //SDVT_VBYONE_24BPP_YCBCR444
    //              vin_color = 0;
    //              vin_bpp   = 1;
    //              break;
    //    case 4:   //SDVT_VBYONE_30BPP_RGB
                  //vin_color = 4;
                  //vin_bpp   = 0;
    //              break;
    //    case 5:   //SDVT_VBYONE_30BPP_YCBCR444
    //              vin_color = 0;
    //              vin_bpp   = 0;
    //              break;
    //    default:
    //        printk( "Error VBYONE_COLOR_FORMAT!\n");
    //              return;
    //}
    // clock seting for VX1
    //vclk_set_encl_vx1(vfromat, lane, byte);

    // set encl format
    //set_tv_encl (TV_ENC_LCD3840x2160p_vic03,1,0,0);

    // vpu clock setting
    //aml_set_reg32(P_HHI_VPU_CLK_CNTL,   (0 << 9)    |   // vpu   clk_sel
    //                        (0 << 0) );     // vpu   clk_div
    //aml_set_reg32(P_HHI_VPU_CLK_CNTL, (Rd(HHI_VPU_CLK_CNTL) | (1 << 8)) );

    //PIN_MUX for VX1 need to add this to dtd
    printk("Set VbyOne PIN MUX ......\n");
    aml_set_reg32_bits(P_PERIPHS_PIN_MUX_3,3,8,2);

    // set Vbyone
    printk("VbyOne Configuration ......\n");
    //set_vbyone_vfmt(vin_color,vin_bpp);
    aml_set_reg32_bits(P_VBO_VIN_CTRL, vin_color, VBO_VIN_PACK_BIT,VBO_VIN_PACK_WID);
    aml_set_reg32_bits(P_VBO_VIN_CTRL,vin_bpp,  VBO_VIN_BPP_BIT,VBO_VIN_BPP_WID);
    config_vbyone(lane, byte, region, hsize, vsize);
    set_vbyone_sync_pol(0, 0); //set hsync/vsync polarity to let the polarity is low active inside the VbyOne

    // below line copy from simulation
    aml_set_reg32_bits(P_VBO_VIN_CTRL, 1, 0, 2); //gate the input when vsync asserted
    ///aml_set_reg32(P_VBO_VBK_CTRL_0,0x13);
    //aml_set_reg32(P_VBO_VBK_CTRL_1,0x56);
    //aml_set_reg32(P_VBO_HBK_CTRL,0x3478);
    //aml_set_reg32_bits(P_VBO_PXL_CTRL,0x2,VBO_PXL_CTR0_BIT,VBO_PXL_CTR0_WID);
    //aml_set_reg32_bits(P_VBO_PXL_CTRL,0x3,VBO_PXL_CTR1_BIT,VBO_PXL_CTR1_WID);
    //set_vbyone_ctlbits(1,0,0);
    //set fifo_clk_sel: 3 for 10-bits
    aml_set_reg32_bits(P_HHI_LVDS_TX_PHY_CNTL0,3,6,2);

    //PAD select:
	if((lane == 1) || (lane == 2)) {
		aml_set_reg32_bits(P_LCD_PORT_SWAP,1,9,2);
    } else if(lane == 4) {
    	aml_set_reg32_bits(P_LCD_PORT_SWAP,2,9,2);
    } else {
    	aml_set_reg32_bits(P_LCD_PORT_SWAP,0,9,2);
    }
    //aml_set_reg32_bits(P_LCD_PORT_SWAP, 1, 8, 1);//reverse lane output order

    // Mux pads in combo-phy: 0 for dsi; 1 for lvds or vbyone; 2 for edp
    aml_write_reg32(P_HHI_DSI_LVDS_EDP_CNTL0, 0x1); // Select vbyone in combo-phy
    aml_set_reg32_bits(P_VBO_CTRL_L, 1, VBO_ENABLE_BIT, VBO_EBABLE_WID);

    //force vencl clk enable, otherwise, it might auto turn off by mipi DSI
    //WRITE_VCBUS_REG_BITS(VPU_MISC_CTRL, 1, 0, 1);

    printk("VbyOne is In Normal Status ......\n");
}
static void init_vbyone_phy(Lcd_Config_t *pConf)
{
    printk("%s\n", __FUNCTION__);
	//debug
	aml_write_reg32(P_VPU_VLOCK_GCLK_EN, 7);
	aml_write_reg32(P_VPU_VLOCK_ADJ_EN_SYNC_CTRL, 0x108010ff);
	aml_write_reg32(P_VPU_VLOCK_CTRL, 0xe0f50f1b);
	//debug
	aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL1, 0x6e0ec918);
	aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL2, 0x00000a7c);
	aml_write_reg32(P_HHI_DIF_CSI_PHY_CNTL3, 0x00ff0800);
	//od   clk 2970 / 5 = 594
	aml_write_reg32(P_HHI_LVDS_TX_PHY_CNTL0, 0xfff00c0);
	//clear lvds fifo od (/2)
	aml_write_reg32(P_HHI_LVDS_TX_PHY_CNTL1, 0xc1000000);

}
static void set_tcon_vbyone(Lcd_Config_t *pConf)
{
    //Lcd_Timing_t *tcon_adr = &(pConf->lcd_timing);

    vpp_set_matrix_ycbcr2rgb(2, 0);
    aml_write_reg32(P_ENCL_VIDEO_RGBIN_CTRL, 3);
    aml_write_reg32(P_L_RGB_BASE_ADDR, 0);
    aml_write_reg32(P_L_RGB_COEFF_ADDR, 0x400);
    //aml_write_reg32(P_L_POL_CNTL_ADDR,  3);
    //aml_write_reg32(P_L_DUAL_PORT_CNTL_ADDR, (0x1 << LCD_TTL_SEL));
//	if(pConf->lcd_basic.lcd_bits == 8)
//		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x400);
//	else if(pConf->lcd_basic.lcd_bits == 6)
//		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x600);
//	else
//		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0);
    //PRINT_INFO("final LVDS_FIFO_CLK = %d\n", clk_util_clk_msr(24));
	//PRINT_INFO("final cts_encl_clk = %d\n", clk_util_clk_msr(9));
    aml_write_reg32(P_VPP_MISC, aml_read_reg32(P_VPP_MISC) & ~(VPP_OUT_SATURATE));
}

//clock seting for VX1
static void set_pll_vbyone(Lcd_Config_t *pConf)
{
    int  lane, byte;//vfromat,
	//int hdmi_clk_out;
	int hdmi_vx1_clk_od1;
	int pclk_div;
	int phy_div;
	int xd;
	int minlane;

    lane = 8;//lane_num;
    byte = 4;//byte_num;
	//phy_clk = pixel_clk*10*byte_num/lane_num;
	//                                   lane_num      byte_num      phy_clk
	//TV_ENC_LCD720x480:(pclk=27M) 858x525  8            3            101.25      (pclk*3.75)
	//                                      4            3            202.5       (pclk*7.5)
	//                                      2            3            405         (pclk*15)
	//                                      1            3            810         (pclk*30)
	//                                      8            4            135         (pclk*5)
	//                                      4            4            270         (pclk*10)
	//                                      2            4            540         (pclk*20)
	//                                      1            4            1080        (pclk*40)
	//                                   lane_num      byte_num      phy_clk
	//TV_ENC_LCD1920x1080:(pclk=148.5M)     8            3            556.875     (pclk*3.75)
	//                      2200x1125       4            3            1113.75     (pclk*7.5)
	//                                      2            3            2227.5      (pclk*15)
	//                                      1            3            4455        (pclk*30)
	//                                      8            4            742.5       (pclk*5)
	//                                      4            4            1485        (pclk*10)
	//                                      2            4            2970        (pclk*20)
	//                                      1            4            5940        (pclk*40)
	//                                   lane_num      byte_num      phy_clk
	//TV_ENC_LCD3840x2160p:(pclk=594M)      8            3            2227.5      (pclk*3.75)
	//                      4400x2250       4            3            4455        (pclk*7.5)
	//                                      2            3            8910        (pclk*15)
	//                                      1            3           17820        (pclk*30)
	//                                      8            4            2970        (pclk*5)
	//                                      4            4            5940        (pclk*10)
	//                                      2            4           11880        (pclk*20)
	//                                      1            4           23760        (pclk*40)
    //if (byte_num == 3)
    //	hdmi_clk_out = 2227.5*2;  //OD1 = 1
    //else //4
    //	hdmi_clk_out = 2970*2;    //OD1 = 1
    minlane = 8;
    //hdmi_vx1_clk_od1 = 1;

	if (lane < minlane) {
		printk("VX1 cannot support this configuration!\n");
		return;
	}

	phy_div = lane/minlane; //1,2,4,8
	if (phy_div == 8) {
    	phy_div = phy_div/2;
    	if(hdmi_vx1_clk_od1 != 0) {
    		printk("VX1 cannot support this configuration!\n");
    		return;
    	}
    //	hdmi_vx1_clk_od1=1;
	}
	//need check whether we need to set this dpll !!!!!!!
	//if (set_hdmi_dpll(hdmi_clk_out,hdmi_vx1_clk_od1)){
    //   printk("Unsupported HDMI_DPLL out frequency!\n");
    //    return;
    //}

   pclk_div = (((byte==3) ? 30:40)*100)/minlane;
   printk("vbyone byte:%d, lane:%d, pclk:%d, phy_div:%d \n", byte, lane, pclk_div, phy_div);
   //configure vid_clk_div_top
   if (byte == 3) {
	   if (pclk_div == 375) {
		   clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_3p75);
		   xd = 1;
	   } else if (pclk_div == 750) {
		   clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_7p5);
		   xd = 1;
	   } else {
		   clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_15);
		   xd = pclk_div/100/15;
	   }
	} else if (byte == 4) {
		clocks_set_vid_clk_div(CLK_UTIL_VID_PLL_DIV_5);
		xd = pclk_div/100/5;
	}

	//for lvds phy clock and enable decoupling FIFO
	aml_write_reg32(P_HHI_LVDS_TX_PHY_CNTL1,((3<<6)|((phy_div-1)<<1)|1)<<24);

	//configure crt_video
	//set_crt_video_enc(0, 0, xd);  //configure crt_video V1: inSel=vid_pll_clk(0),DivN=xd)
	//if (vidx == 0)
	{ //V1
		aml_set_reg32_bits(P_HHI_VID_CLK_CNTL, 0, 19, 1); //[19] -disable clk_div0
		udelay(2);  //delay 2uS
		aml_set_reg32_bits(P_HHI_VID_CLK_CNTL, 0,   16, 3); // [18:16] - cntl_clk_in_sel
		aml_set_reg32_bits(P_HHI_VID_CLK_DIV, (xd-1), 0, 8); // [7:0]   - cntl_xd0
		udelay(5);  // delay 5uS
		aml_set_reg32_bits(P_HHI_VID_CLK_CNTL, 1, 19, 1); //[19] -enable clk_div0
	}
    //else { //V2
	//	aml_set_reg32_bits(P_HHI_VIID_CLK_CNTL, 0, 19, 1); //[19] -disable clk_div0
	//	udelay(2);  //delay 2uS
	//	aml_set_reg32_bits(P_HHI_VIID_CLK_CNTL, insel,  16, 3); // [18:16] - cntl_clk_in_sel
	//	aml_set_reg32_bits(P_HHI_VIID_CLK_DIV, (divn-1),0, 8); // [7:0]   - cntl_xd0
	//	udelay(5);  // delay 5uS
	//	aml_set_reg32_bits(P_HHI_VIID_CLK_CNTL, 1, 19, 1); //[19] -enable clk_div0
	//}
	udelay(5);  // delay 5uS
	//enable_crt_video_encl(1, 0); //select and enable the output
	aml_set_reg32_bits(P_HHI_VIID_CLK_DIV, 0,  12, 4); //encl_clk_sel:hi_viid_clk_div[15:12]
	//if(inSel<=4) //V1
	aml_set_reg32_bits(P_HHI_VID_CLK_CNTL,1, 0, 1);
	//else
	//	aml_set_reg32_bits(P_HHI_VIID_CLK_CNTL,1, (inSel-5),1);

	aml_set_reg32_bits(P_HHI_VID_CLK_CNTL2, 1, 3, 1); //gclk_encl_clk:hi_vid_clk_cntl2[3]
}

void set_vx1_pinmux(void)
{
	//VX1_LOCKN && VX1_HTPDN
	aml_set_reg32_bits(P_PERIPHS_PIN_MUX_7, 3, 18, 2);
}

static void _init_display_driver(Lcd_Config_t *pConf)
{
	int lcd_type;

	const char* lcd_type_table[]={
		"NULL",
		"TTL",
		"LVDS",
		"MINILVDS",
		"VBYONE",
		"invalid",
	};

	lcd_type = pDev->conf.lcd_basic.lcd_type;
	printk("\nInit LCD type: %s.\n", lcd_type_table[lcd_type]);

	switch(lcd_type){
		case LCD_DIGITAL_TTL:
			printk("ttl mode is selected!\n");
			set_pll_ttl(pConf);
			venc_set_ttl(pConf);
			set_tcon_ttl(pConf);
			set_ttl_pinmux();
			break;
		case LCD_DIGITAL_LVDS:
			printk("lvds mode is selected!\n");
			set_pll_lvds(pConf);
			venc_set_lvds(pConf);
			set_control_lvds(pConf);
			init_lvds_phy(pConf);
			set_tcon_lvds(pConf);
			break;
		case LCD_DIGITAL_VBYONE:
			printk("vx1 mode is selected!\n");
			set_pll_vbyone(pConf);
			venc_set_vx1(pConf);
			set_control_vbyone(pConf);
			init_vbyone_phy(pConf);
			set_tcon_vbyone(pConf);
			set_vx1_pinmux();
			break;
		default:
			pr_err("Invalid LCD type.\n");
			break;
	}
}

static inline void _disable_display_driver(void)
{
	int vclk_sel;

	vclk_sel = 0;//((pConf->lcd_timing.clk_ctrl) >>4) & 0x1;

	aml_set_reg32_bits(P_HHI_VIID_DIVIDER_CNTL, 0, 11, 1);	//close lvds phy clk gate: 0x104c[11]

	aml_write_reg32(P_ENCL_VIDEO_EN, 0);	//disable ENCL
	aml_write_reg32(P_ENCL_VIDEO_EN, 0);	//disable encl

	if (vclk_sel)
		aml_set_reg32_bits(P_HHI_VIID_CLK_CNTL, 0, 0, 5);		//close vclk2 gate: 0x104b[4:0]
	else
		aml_set_reg32_bits(P_HHI_VID_CLK_CNTL, 0, 0, 5);		//close vclk1 gate: 0x105f[4:0]

	pr_info("disable lcd display driver.\n");
}

static inline void _enable_vsync_interrupt(void)
{
	if ((aml_read_reg32(P_ENCL_VIDEO_EN) & 1) || (aml_read_reg32(P_ENCL_VIDEO_EN) & 1)) {
		aml_write_reg32(P_VENC_INTCTRL, 0x200);
	}else{
		aml_write_reg32(P_VENC_INTCTRL, 0x2);
	}
}
static void _lcd_module_enable(void)
{
	BUG_ON(pDev==NULL);
	_init_display_driver(&pDev->conf);
	_enable_vsync_interrupt();
}

static void change_panel(lcd_dev_t *pDev)
{
	pDev->lcd_info.name = PANEL_NAME;
	pDev->lcd_info.width = pDev->conf.lcd_basic.h_active;
	pDev->lcd_info.height = pDev->conf.lcd_basic.v_active;
	pDev->lcd_info.field_height = pDev->conf.lcd_basic.v_active;
	pDev->lcd_info.aspect_ratio_num = pDev->conf.lcd_basic.screen_ratio_width;
	pDev->lcd_info.aspect_ratio_den = pDev->conf.lcd_basic.screen_ratio_height;
	pDev->lcd_info.screen_real_width= pDev->conf.lcd_basic.screen_actual_width;
	pDev->lcd_info.screen_real_height= pDev->conf.lcd_basic.screen_actual_height;
	pDev->lcd_info.sync_duration_num = pDev->conf.lcd_timing.sync_duration_num;
	pDev->lcd_info.sync_duration_den = pDev->conf.lcd_timing.sync_duration_den;

	if((cur_vmode == VMODE_1080P)||
		(cur_vmode == VMODE_1080P_50HZ))
	{
		pDev->conf.lcd_basic.lcd_type = LCD_DIGITAL_LVDS;
	}else if(cur_vmode == VMODE_4K2K_60HZ)
	{
		pDev->conf.lcd_basic.lcd_type = LCD_DIGITAL_VBYONE;
	}else if(cur_vmode == VMODE_720P)
	{
		pDev->conf.lcd_basic.lcd_type = LCD_DIGITAL_TTL;
	}
}

static void _init_vout(lcd_dev_t *pDev)
{
	change_panel(pDev);
//	vout_register_server(&lcd_vout_server);
}

static void _lcd_init(Lcd_Config_t *pConf)
{
	 _init_vout(pDev);
	 _lcd_module_enable();
}


static struct notifier_block lcd_reboot_nb;

static int lcd_reboot_notifier(struct notifier_block *nb, unsigned long state, void *cmd)
 {
	if (state == SYS_RESTART){
		pr_info("shut down lcd...\n");
	}

	return NOTIFY_DONE;
}


void panel_power_on(void)
{
	pr_info("%s\n", __func__);
}
void panel_power_off(void)
{
	pr_info("%s\n", __func__);
}
static ssize_t power_show(struct class *cls,
			struct class_attribute *attr, char *buf)
{
	int ret = 0;
	bool status = false;
	int value;

	status = true;//gpio_get_status(PAD_GPIOZ_5);
	ret = sprintf(buf, "PAD_GPIOZ_5 gpio_get_status %s\n", (status?"true":"false"));
	ret = sprintf(buf, "\n");

	value = 0;//gpio_in_get(PAD_GPIOZ_5);
	ret = sprintf(buf, "PAD_GPIOZ_5 gpio_in_get %d\n", value);
	ret = sprintf(buf, "\n");

	return ret;
}
static ssize_t power_store(struct class *cls, struct class_attribute *attr,
			 const char *buf, size_t count)
{
	int ret = 0;
	int status = 0;
	status = simple_strtol(buf, NULL, 0);
	pr_info("input status %d\n", status);

	if (status != 0) {
		panel_power_off();
		pr_info("lvds_power_off\n");
	}else {
		panel_power_on();
		pr_info("lvds_power_on\n");
	}

	return ret;
}
static CLASS_ATTR(power, S_IWUSR | S_IRUGO, power_show, power_store);

static inline int _get_lcd_default_config(struct platform_device *pdev)
{
	int ret = 0;
	unsigned int lvds_para[14];
	if (!pdev->dev.of_node){
		pr_err("\n can't get dev node---error----%s----%d",__FUNCTION__,__LINE__);
	}

	ret = of_property_read_u32_array(pdev->dev.of_node,"basic_setting",&lvds_para[0], 10);
	if(ret){
		pr_err("don't find to match basic_setting, use default setting.\n");
	}else{
		pr_info("get basic_setting ok.\n");
		pDev->conf.lcd_basic.h_active = lvds_para[0];
		pDev->conf.lcd_basic.v_active = lvds_para[1];
		pDev->conf.lcd_basic.h_period= lvds_para[2];
		pDev->conf.lcd_basic.v_period= lvds_para[3];
		pDev->conf.lcd_basic.screen_ratio_width = lvds_para[4];
		pDev->conf.lcd_basic.screen_ratio_height = lvds_para[5];
		pDev->conf.lcd_basic.screen_actual_width = lvds_para[6];
		pDev->conf.lcd_basic.screen_actual_height = lvds_para[7];
		pDev->conf.lcd_basic.lcd_type = lvds_para[8];
		pDev->conf.lcd_basic.lcd_bits = lvds_para[9];
	}
	ret = of_property_read_u32_array(pdev->dev.of_node,"lcd_timing",&lvds_para[0], 14);
	if(ret){
		pr_err("don't find to match lcd_timing, use default setting.\n");
	}else{
		pr_info("get lcd_timing ok.\n");
		pDev->conf.lcd_timing.video_on_pixel = lvds_para[0];
		pDev->conf.lcd_timing.video_on_line = lvds_para[1];
		pDev->conf.lcd_timing.sth1_hs_addr = lvds_para[2];
		pDev->conf.lcd_timing.sth1_he_addr = lvds_para[3];
		pDev->conf.lcd_timing.sth1_vs_addr = lvds_para[4];
		pDev->conf.lcd_timing.sth1_ve_addr = lvds_para[5];
		pDev->conf.lcd_timing.stv1_hs_addr = lvds_para[6];
		pDev->conf.lcd_timing.stv1_he_addr = lvds_para[7];
		pDev->conf.lcd_timing.stv1_vs_addr = lvds_para[8];
		pDev->conf.lcd_timing.stv1_ve_addr = lvds_para[9];
		pDev->conf.lcd_timing.oeh_hs_addr = lvds_para[10];
		pDev->conf.lcd_timing.oeh_he_addr = lvds_para[11];
		pDev->conf.lcd_timing.oeh_vs_addr = lvds_para[12];
		pDev->conf.lcd_timing.oeh_ve_addr = lvds_para[13];		
	}
	ret = of_property_read_u32_array(pdev->dev.of_node,"delay_setting",&lvds_para[0], 8);
	if(ret){
		pr_err("don't find to match delay_setting, use default setting.\n");
	}else{
		pDev->conf.lcd_sequence.clock_enable_delay = lvds_para[0];
		pDev->conf.lcd_sequence.clock_disable_delay = lvds_para[1];
		pDev->conf.lcd_sequence.pwm_enable_delay= lvds_para[2];
		pDev->conf.lcd_sequence.pwm_disable_delay= lvds_para[3];
		pDev->conf.lcd_sequence.panel_power_on_delay = lvds_para[4];
		pDev->conf.lcd_sequence.panel_power_off_delay = lvds_para[5];
		pDev->conf.lcd_sequence.backlight_power_on_delay = lvds_para[6];
		pDev->conf.lcd_sequence.backlight_power_off_delay = lvds_para[7];
	}
	return ret;
}

// Define LVDS physical PREM SWING VCM REF
static Lvds_Phy_Control_t lcd_lvds_phy_control = {
	.lvds_prem_ctl  = 0x4,
	.lvds_swing_ctl = 0x2,
	.lvds_vcm_ctl   = 0x4,
	.lvds_ref_ctl   = 0x15,
	.lvds_phy_ctl0  = 0x0002,
	.lvds_fifo_wr_mode = 0x3,
};
//Define LVDS data mapping, pn swap.
static Lvds_Config_t lcd_lvds_config = {
	.lvds_repack    = 1,   //data mapping  //0:JEDIA mode, 1:VESA mode
	.pn_swap    = 0,       //0:normal, 1:swap
	.dual_port  = 1,
	.port_reverse   = 1,
};
static Lcd_Config_t m6tv_lvds_config = {
	// Refer to LCD Spec
	.lcd_basic = {
		.h_active = H_ACTIVE,
		.v_active = V_ACTIVE,
		.h_period = H_PERIOD,
		.v_period = V_PERIOD,
		.screen_ratio_width   = 16,
		.screen_ratio_height  = 9,
		.screen_actual_width  = 127, //this is value for 160 dpi please set real value according to spec.
		.screen_actual_height = 203, //
		.lcd_type = LCD_DIGITAL_LVDS,   //LCD_DIGITAL_TTL  //LCD_DIGITAL_LVDS  //LCD_DIGITAL_MINILVDS
		.lcd_bits = 8,  //8  //6
	},
	.lcd_timing = {
		.pll_ctrl = 0x40050c82,//0x400514d0, //
		.div_ctrl = 0x00010803,
		.clk_ctrl = 0x1111,  //[19:16]ss_ctrl, [12]pll_sel, [8]div_sel, [4]vclk_sel, [3:0]xd
		//.sync_duration_num = 501,
		//.sync_duration_den = 10,

		.video_on_pixel = VIDEO_ON_PIXEL,
		.video_on_line  = VIDEO_ON_LINE,

		.sth1_hs_addr = 44,
		.sth1_he_addr = 2156,
		.sth1_vs_addr = 0,
		.sth1_ve_addr = V_PERIOD - 1,
		.stv1_hs_addr = 2100,
		.stv1_he_addr = 2164,
		.stv1_vs_addr = 3,
		.stv1_ve_addr = 5,
		.oeh_hs_addr = VIDEO_ON_PIXEL+19,
		.oeh_he_addr = VIDEO_ON_PIXEL+19+H_ACTIVE,
		.oeh_vs_addr = VIDEO_ON_LINE,
		.oeh_ve_addr = VIDEO_ON_LINE+V_ACTIVE-1,

		.pol_cntl_addr = (0x0 << LCD_CPH1_POL) |(0x0 << LCD_HS_POL) | (0x1 << LCD_VS_POL),
		.inv_cnt_addr = (0<<LCD_INV_EN) | (0<<LCD_INV_CNT),
		.tcon_misc_sel_addr = (1<<LCD_STV1_SEL) | (1<<LCD_STV2_SEL),
		.dual_port_cntl_addr = (1<<LCD_TTL_SEL) | (1<<LCD_ANALOG_SEL_CPH3) | (1<<LCD_ANALOG_3PHI_CLK_SEL) | (0<<LCD_RGB_SWP) | (0<<LCD_BIT_SWP),
	},
	.lcd_sequence = {
		.clock_enable_delay        = CLOCK_ENABLE_DELAY,
		.clock_disable_delay       = CLOCK_DISABLE_DELAY,
		.pwm_enable_delay          = PWM_ENABLE_DELAY,
		.pwm_disable_delay         = PWM_DISABLE_DELAY,
		.panel_power_on_delay      = PANEL_POWER_ON_DELAY,
		.panel_power_off_delay     = PANEL_POWER_OFF_DELAY,
		.backlight_power_on_delay  = BACKLIGHT_POWER_ON_DELAY,
		.backlight_power_off_delay = BACKLIGHT_POWER_OFF_DELAY,
	},
	.lvds_mlvds_config = {
		.lvds_config = &lcd_lvds_config,
		.lvds_phy_control = &lcd_lvds_phy_control,
	},
};

#ifdef CONFIG_USE_OF
static struct aml_lcd_platform m6tv_lvds_device = {
	.lcd_conf = &m6tv_lvds_config,
};
#define AMLOGIC_LVDS_DRV_DATA ((kernel_ulong_t)&m6tv_lvds_device)
static const struct of_device_id lvds_dt_match[]={
	{
		.compatible = "amlogic,lvds",
		.data = (void *)AMLOGIC_LVDS_DRV_DATA
	},
	{},
};
#else
#define lvds_dt_match NULL
#endif

#ifdef CONFIG_USE_OF
static inline struct aml_lcd_platform *lvds_get_driver_data(struct platform_device *pdev)
{
	const struct of_device_id *match;

	if(pdev->dev.of_node) {
		match = of_match_node(lvds_dt_match, pdev->dev.of_node);
		return (struct aml_lcd_platform *)match->data;
	}else
		pr_err("\n ERROR get data %d",__LINE__);
	return NULL;
}
#endif

static void lcd_output_mode_info(void)
{
	const vinfo_t *info;
	info = get_current_vinfo();
	if(info){
		if(strncmp(info->name, "720p", 4) == 0){
			cur_vmode = VMODE_720P;	
		}else if(strncmp(info->name, "1080p", 5) == 0){
			cur_vmode = VMODE_1080P;
		}else if(strncmp(info->name, "4k2k60hz", 8) == 0){
			cur_vmode = VMODE_4K2K_60HZ;
		}else{
			cur_vmode = VMODE_1080P;
			printk("the output mode is not support,use default mode!\n");
		}
	}
	_lcd_init(&pDev->conf);
}

static int lcd_notify_callback_v(struct notifier_block *block, unsigned long cmd , void *para)
{
    if (cmd != VOUT_EVENT_MODE_CHANGE)
        return 0;

    lcd_output_mode_info();
    return 0;
}

static struct notifier_block lcd_mode_notifier_nb_v = {
    .notifier_call    = lcd_notify_callback_v,
};

static int lcd_probe(struct platform_device *pdev)
{
	struct aml_lcd_platform *pdata;
	int err;

#ifdef CONFIG_USE_OF
	pdata = lvds_get_driver_data(pdev);
#else
	pdata = pdev->dev.platform_data;
#endif

	pDev = (lcd_dev_t *)kmalloc(sizeof(lcd_dev_t), GFP_KERNEL);
	if (!pDev) {
		pr_err("[tcon]: Not enough memory.\n");
		return -ENOMEM;
	}

	pDev->lcd_info.mode = cur_vmode;
	pDev->conf = *(Lcd_Config_t *)(pdata->lcd_conf);        //*(Lcd_Config_t *)(s->start);

#ifdef CONFIG_USE_OF
	_get_lcd_default_config(pdev);
#endif

	printk("LCD probe ok\n");
	vout_register_client(&lcd_mode_notifier_nb_v);
	_lcd_init(&pDev->conf);
	lcd_reboot_nb.notifier_call = lcd_reboot_notifier;
	err = register_reboot_notifier(&lcd_reboot_nb);
	if (err) {
		pr_err("notifier register lcd_reboot_notifier fail!\n");
	}

	return 0;
}

static int lcd_remove(struct platform_device *pdev)
{
	unregister_reboot_notifier(&lcd_reboot_nb);
	kfree(pDev);

	return 0;
}

#ifdef CONFIG_PM
static int lcd_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int lcd_drv_resume(struct platform_device *pdev)
{
	pr_info("%s\n", __func__);
	return 0;
}
#endif /* CONFIG_PM */

static struct platform_driver lcd_driver = {
	.probe = lcd_probe,
	.remove = lcd_remove,
	.driver = {
		.name = "mesonlcd",
		.of_match_table = lvds_dt_match,
	},
#ifdef CONFIG_PM
	.suspend = lcd_drv_suspend,
	.resume = lcd_drv_resume,
#endif
};

static int __init lcd_init(void)
{
	int ret;
	printk("TV LCD driver init\n");

	if (platform_driver_register(&lcd_driver)) {
		pr_err("failed to register tcon driver module\n");
		return -ENODEV;
	}

	aml_lcd_clsp = class_create(THIS_MODULE, "aml_lcd");
	ret = class_create_file(aml_lcd_clsp, &class_attr_power);

	return 0;
}

static void __exit lcd_exit(void)
{
	class_remove_file(aml_lcd_clsp, &class_attr_power);
	class_destroy(aml_lcd_clsp);
	platform_driver_unregister(&lcd_driver);
}

subsys_initcall(lcd_init);
module_exit(lcd_exit);

int __init outputmode_setup(char *s)
{
	if(!strcmp(s,"1080p")){
		cur_vmode = VMODE_1080P;
	}else  if(!strcmp(s,"1080p50hz")){
	        cur_vmode = VMODE_1080P_50HZ;
	}else  if(!strcmp(s,"4k2k60hz")){
		cur_vmode = VMODE_4K2K_60HZ;
	}else  if(!strcmp(s,"720p")){
		cur_vmode = VMODE_720P;
	}else{
		cur_vmode = VMODE_INIT_NULL;
		printk("the output mode is not support!\n");
	}
	printk("the output mode is %d\n",cur_vmode);
	return 0;	
}
__setup("vmode=",outputmode_setup) ;

 int __init lvds_boot_para_setup(char *s)
{
	unsigned char* ptr;
	unsigned char flag_buf[16];
	int i;

	pr_info("LVDS boot args: %s\n", s);

	if(strstr(s, "10bit")){
		bit_num_flag = 0;
		bit_num = 0;
	}
	if(strstr(s, "8bit")){
		bit_num_flag = 0;
		bit_num = 1;
	}
	if(strstr(s, "6bit")){
		bit_num_flag = 0;
		bit_num = 2;
	}
	if(strstr(s, "4bit")){
		bit_num_flag = 0;
		bit_num = 3;
	}
	if(strstr(s, "jeida")){
		lvds_repack_flag = 0;
		lvds_repack = 0;
	}
	if(strstr(s, "port_reverse")){
		port_reverse_flag = 0;
		port_reverse = 0;
	}
	if(strstr(s, "flaga")){
		i=0;
		ptr=strstr(s,"flaga")+5;
		while((*ptr) && ((*ptr)!=',') && (i<10)){
			flag_buf[i]=*ptr;
			ptr++; i++;
		}
		flag_buf[i]=0;
		flaga = simple_strtoul(flag_buf, NULL, 10);
	}
	if(strstr(s, "flagb")){
		i=0;
		ptr=strstr(s,"flagb")+5;
		while((*ptr) && ((*ptr)!=',') && (i<10)){
			flag_buf[i]=*ptr;
			ptr++; i++;
		}
		flag_buf[i]=0;
		flagb = simple_strtoul(flag_buf, NULL, 10);
	}
	if(strstr(s, "flagc")){
		i=0;
		ptr=strstr(s,"flagc")+5;
		while((*ptr) && ((*ptr)!=',') && (i<10)){
			flag_buf[i]=*ptr;
			ptr++; i++;
		}
		flag_buf[i]=0;
		flagc = simple_strtoul(flag_buf, NULL, 10);
	}
	if(strstr(s, "flagd")){
		i=0;
		ptr=strstr(s,"flagd")+5;
		while((*ptr) && ((*ptr)!=',') && (i<10)){
			flag_buf[i]=*ptr;
			ptr++; i++;
		}
		flag_buf[i]=0;
		flagd = simple_strtoul(flag_buf, NULL, 10);
	}
	return 0;
}
__setup("lvds=",lvds_boot_para_setup);

MODULE_DESCRIPTION("Meson LCD Panel Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amlogic, Inc.");


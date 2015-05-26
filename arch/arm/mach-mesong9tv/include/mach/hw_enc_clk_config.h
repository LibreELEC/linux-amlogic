#ifndef __HW_ENC_CLK_CONFIG_H__
#define __HW_ENC_CLK_CONFIG_H__

#include <linux/amlogic/vout/enc_clk_config.h>

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
#define CLK_UTIL_VID_PLL_DIV_2p5    14

typedef struct{
    vmode_t mode;
    unsigned viu_path;
    viu_type_e viu_type;
    unsigned hpll_clk_out;
    unsigned od1;
    unsigned od2;
    unsigned od3;
    unsigned vid_pll_div;
    unsigned vid_clk_div;
    unsigned hdmi_tx_pixel_div;
    unsigned encp_div;
    unsigned enci_div;
    unsigned enct_div;
    unsigned encl_div;
    unsigned vdac0_div;
    unsigned vdac1_div;
}hw_enc_clk_val_t;

extern void clocks_set_vid_clk_div(int div_sel);
extern int set_hdmi_dpll(int freq, int od1);
extern void set_crt_video_enc (int vIdx, int inSel, int DivN);
extern void enable_crt_video_encl(int enable, int inSel);

#endif

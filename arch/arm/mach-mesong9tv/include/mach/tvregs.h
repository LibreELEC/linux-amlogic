/*
 * Amlogic Apollo
 * frame buffer driver
 *
 * Copyright (C) 2009 Amlogic, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author:  Tim Yao <timyao@amlogic.com>
 *
 */

#ifndef TVREGS_H
#define TVREGS_H
#include <mach/register.h>
#include <linux/amlogic/vout/vinfo.h>

	#define VIDEO_CLOCK_HD_25	0x00101529
	#define VIDEO_CLOCK_SD_25	0x00500a6c
	#define VIDEO_CLOCK_HD_24	0x00140863
	#define VIDEO_CLOCK_SD_24	0x0050042d

/*
24M
25M
*/
static const  reg_t tvreg_vclk_sd[]={
//	{HHI_VID_PLL_CNTL,VIDEO_CLOCK_SD_24},//SD.24
//    {HHI_VID_PLL_CNTL,VIDEO_CLOCK_SD_25},//SD,25
};

static const  reg_t tvreg_vclk_hd[]={
//    {HHI_VID_PLL_CNTL,VIDEO_CLOCK_HD_24},//HD,24
//    {HHI_VID_PLL_CNTL,VIDEO_CLOCK_HD_25},//HD,25
};

static const  reg_t tvregs_720p[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_HHI_HDMI_AFC_CNTL,          0x8c0000c3},
    {P_ENCP_VIDEO_FILT_CTRL,       0x0052,},
    {P_VENC_DVI_SETTING,           0x2029,},
    {P_ENCP_VIDEO_MODE,            0x4040,},
    {P_ENCP_VIDEO_MODE_ADV,        0x0019,},
    {P_ENCP_VIDEO_YFP1_HTIME,      648,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      3207,  },
    {P_ENCP_VIDEO_MAX_PXCNT,       3299,  },
    {P_ENCP_VIDEO_HSPULS_BEGIN,    80,    },
    {P_ENCP_VIDEO_HSPULS_END,      240,   },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   80,    },
    {P_ENCP_VIDEO_VSPULS_BEGIN,    688,   },
    {P_ENCP_VIDEO_VSPULS_END,      3248,  },
    {P_ENCP_VIDEO_VSPULS_BLINE,    4,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    8,     },
    {P_ENCP_VIDEO_EQPULS_BLINE,    4,     },
    {P_ENCP_VIDEO_EQPULS_ELINE,    8,     },
    {P_ENCP_VIDEO_HAVON_BEGIN,     648,   },
    {P_ENCP_VIDEO_HAVON_END,       3207,  },
    {P_ENCP_VIDEO_VAVON_BLINE,     29,    },
    {P_ENCP_VIDEO_VAVON_ELINE,     748,   },
    {P_ENCP_VIDEO_HSO_BEGIN,       256    },
    {P_ENCP_VIDEO_HSO_END,         168,   },
    {P_ENCP_VIDEO_VSO_BEGIN,       168,   },
    {P_ENCP_VIDEO_VSO_END,         256,   },
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_VSO_ELINE,       5,     },
    {P_ENCP_VIDEO_MAX_LNCNT,       749,   },
    {P_VENC_VIDEO_PROG_MODE,       0x100, },
    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_SETTING,          0,     },
    {P_VENC_UPSAMPLE_CTRL0,        0x9061,},
    {P_VENC_UPSAMPLE_CTRL1,        0xa061,},
    {P_VENC_UPSAMPLE_CTRL2,        0xb061,},
    {P_VENC_VDAC_DACSEL0,          0x0001,},
    {P_VENC_VDAC_DACSEL1,          0x0001,},
    {P_VENC_VDAC_DACSEL2,          0x0001,},
    {P_VENC_VDAC_DACSEL3,          0x0001,},
    {P_VENC_VDAC_DACSEL4,          0x0001,},
    {P_VENC_VDAC_DACSEL5,          0x0001,},
    {P_VPU_VIU_VENC_MUX_CTRL,      0x000a,},
    {P_VENC_VDAC_FIFO_CTRL,        0x1000,},
    {P_ENCP_DACSEL_0,              0x3102,},
    {P_ENCP_DACSEL_1,              0x0054,},
    {P_ENCP_VIDEO_EN,              1,     },
    {P_ENCI_VIDEO_EN,              0,     },
    {MREG_END_MARKER,            0      }
};

static const  reg_t tvregs_720p_50hz[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_FILT_CTRL,       0x0052,},

    {P_VENC_DVI_SETTING,           0x202d,},
    {P_ENCP_VIDEO_MAX_PXCNT,       3959,  },
    {P_ENCP_VIDEO_MAX_LNCNT,       749,   },

     //analog vidoe position in horizontal
    {P_ENCP_VIDEO_HSPULS_BEGIN,    80,    },
    {P_ENCP_VIDEO_HSPULS_END,      240,   },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   80,    },

    //DE position in horizontal
    {P_ENCP_VIDEO_HAVON_BEGIN,     648,   },
    {P_ENCP_VIDEO_HAVON_END,       3207,  },

    //ditital hsync positon in horizontal
    {P_ENCP_VIDEO_HSO_BEGIN,       128 ,},
    {P_ENCP_VIDEO_HSO_END,         208 , },

    /* vsync horizontal timing */
    {P_ENCP_VIDEO_VSPULS_BEGIN,    688,   },
    {P_ENCP_VIDEO_VSPULS_END,      3248,  },

    /* vertical timing settings */
    {P_ENCP_VIDEO_VSPULS_BLINE,    4,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    8,     },
    {P_ENCP_VIDEO_EQPULS_BLINE,    4,     },
    {P_ENCP_VIDEO_EQPULS_ELINE,    8,     },

    //DE position in vertical
    {P_ENCP_VIDEO_VAVON_BLINE,     29,    },
    {P_ENCP_VIDEO_VAVON_ELINE,     748,   },

    //adjust the vsync start point and end point
    {P_ENCP_VIDEO_VSO_BEGIN,       128,},  //168,   },
    {P_ENCP_VIDEO_VSO_END,         128, },  //256,   },

    //adjust the vsync start line and end line
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_VSO_ELINE,       5,     },

    /* filter & misc settings */
    {P_ENCP_VIDEO_YFP1_HTIME,      648,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      3207,  },


    {P_VENC_VIDEO_PROG_MODE,       0x100, },
    {P_ENCP_VIDEO_MODE,            0x4040,},  //Enable Hsync and equalization pulse switch in center
    {P_ENCP_VIDEO_MODE_ADV,        0x0019,},//bit6:swap PbPr; bit4:YPBPR gain as HDTV type;
                                                 //bit3:Data input from VFIFO;bit[2}0]:repreat pixel a time

     {P_ENCP_VIDEO_SYNC_MODE,       0x407,  },//Video input Synchronization mode ( bit[7:0] -- 4:Slave mode, 7:Master mode)
                                                 //bit[15:6] -- adjust the vsync vertical position
    {P_ENCP_VIDEO_YC_DLY,          0,     },      //Y/Cb/Cr delay
    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_SETTING,          0,     },
    {P_VPU_VIU_VENC_MUX_CTRL,      0x000a,},
    {P_VENC_VDAC_FIFO_CTRL,        0x1000,},
    {P_ENCP_DACSEL_0,              0x3102,},
    {P_ENCP_DACSEL_1,              0x0054,},
    {P_VENC_VDAC_DACSEL0,          0x0001,},
    {P_VENC_VDAC_DACSEL1,          0x0001,},
    {P_VENC_VDAC_DACSEL2,          0x0001,},
    {P_VENC_VDAC_DACSEL3,          0x0001,},
    {P_VENC_VDAC_DACSEL4,          0x0001,},
    {P_VENC_VDAC_DACSEL5,          0x0001,},
    {P_ENCP_VIDEO_EN,              1,     },
    {P_ENCI_VIDEO_EN,              0,     },
    {MREG_END_MARKER,            0      }
};

static const reg_t tvregs_480i[] = {
    {P_VENC_VDAC_SETTING,            0xff,  },

    {P_ENCI_CFILT_CTRL,              0x12,},
    {P_ENCI_CFILT_CTRL2,              0x12,},
    {P_VENC_DVI_SETTING,             0,     },
    {P_ENCI_VIDEO_MODE,              0,     },
    {P_ENCI_VIDEO_MODE_ADV,          0,     },
    {P_ENCI_SYNC_HSO_BEGIN,          5,     },
    {P_ENCI_SYNC_HSO_END,            129,   },
    {P_ENCI_SYNC_VSO_EVNLN,          0x0003 },
    {P_ENCI_SYNC_VSO_ODDLN,          0x0104 },
    {P_ENCI_MACV_MAX_AMP,            0x810b },
    {P_VENC_VIDEO_PROG_MODE,         0xf0   },
    {P_ENCI_VIDEO_MODE,              0x08   },
    {P_ENCI_VIDEO_MODE_ADV,          0x26,  },
    {P_ENCI_VIDEO_SCH,               0x20,  },
    {P_ENCI_SYNC_MODE,               0x07,  },
    {P_ENCI_YC_DELAY,                0x333, },
    {P_ENCI_VFIFO2VD_PIXEL_START,    0xf3,  },
    {P_ENCI_VFIFO2VD_PIXEL_END,      0x0693,},
    {P_ENCI_VFIFO2VD_LINE_TOP_START, 0x12,  },
    {P_ENCI_VFIFO2VD_LINE_TOP_END,   0x102, },
    {P_ENCI_VFIFO2VD_LINE_BOT_START, 0x13,  },
    {P_ENCI_VFIFO2VD_LINE_BOT_END,   0x103, },
    {P_VENC_SYNC_ROUTE,              0,     },
    {P_ENCI_DBG_PX_RST,              0,     },
    {P_VENC_INTCTRL,                 0x2,   },
    {P_ENCI_VFIFO2VD_CTL,            0x4e01,},
    {P_VENC_VDAC_SETTING,          0,     },
    {P_VENC_UPSAMPLE_CTRL0,          0x0061,},
    {P_VENC_UPSAMPLE_CTRL1,          0x4061,},
    {P_VENC_UPSAMPLE_CTRL2,          0x5061,},
    {P_VENC_VDAC_DACSEL0,            0x0000,},
    {P_VENC_VDAC_DACSEL1,            0x0000,},
    {P_VENC_VDAC_DACSEL2,            0x0000,},
    {P_VENC_VDAC_DACSEL3,            0x0000,},
    {P_VENC_VDAC_DACSEL4,            0x0000,},
    {P_VENC_VDAC_DACSEL5,            0x0000,},
    {P_VPU_VIU_VENC_MUX_CTRL,        0x0005,},
    {P_VENC_VDAC_FIFO_CTRL,          0x2000,},
    {P_ENCI_DACSEL_0,                0x0011 },
    {P_ENCI_DACSEL_1,                0x87   },
    {P_ENCP_VIDEO_EN,                0,     },
    {P_ENCI_VIDEO_EN,                1,     },
    {MREG_END_MARKER,              0      }
};

static const reg_t tvregs_480cvbs[] = {
    {P_VENC_VDAC_SETTING,            0xff,  },

    {P_ENCI_CFILT_CTRL,              0x12,},
    {P_ENCI_CFILT_CTRL2,              0x12,},
    {P_VENC_DVI_SETTING,             0,     },
    {P_ENCI_VIDEO_MODE,              0,     },
    {P_ENCI_VIDEO_MODE_ADV,          0,     },
    {P_ENCI_SYNC_HSO_BEGIN,          5,     },
    {P_ENCI_SYNC_HSO_END,            129,   },
    {P_ENCI_SYNC_VSO_EVNLN,          0x0003 },
    {P_ENCI_SYNC_VSO_ODDLN,          0x0104 },
    {P_ENCI_MACV_MAX_AMP,            0x810b },
    {P_VENC_VIDEO_PROG_MODE,         0xf0   },
    {P_ENCI_VIDEO_MODE,              0x08   },
    {P_ENCI_VIDEO_MODE_ADV,          0x26,  },
    {P_ENCI_VIDEO_SCH,               0x20,  },
    {P_ENCI_SYNC_MODE,               0x07,  },
    {P_ENCI_YC_DELAY,                0x333, },
    {P_ENCI_VFIFO2VD_PIXEL_START,    0xe3,  },
    {P_ENCI_VFIFO2VD_PIXEL_END,      0x0683,},
    {P_ENCI_VFIFO2VD_LINE_TOP_START, 0x12,  },
    {P_ENCI_VFIFO2VD_LINE_TOP_END,   0x102, },
    {P_ENCI_VFIFO2VD_LINE_BOT_START, 0x13,  },
    {P_ENCI_VFIFO2VD_LINE_BOT_END,   0x103, },
    {P_VENC_SYNC_ROUTE,              0,     },
    {P_ENCI_DBG_PX_RST,              0,     },
    {P_VENC_INTCTRL,                 0x2,   },
    {P_ENCI_VFIFO2VD_CTL,            0x4e01,},
    {P_VENC_VDAC_SETTING,          0,     },
    {P_VENC_UPSAMPLE_CTRL0,          0x0061,},
    {P_VENC_UPSAMPLE_CTRL1,          0x4061,},
    {P_VENC_UPSAMPLE_CTRL2,          0x5061,},
    {P_VENC_VDAC_DACSEL0,            0x0000,},
    {P_VENC_VDAC_DACSEL1,            0x0000,},
    {P_VENC_VDAC_DACSEL2,            0x0000,},
    {P_VENC_VDAC_DACSEL3,            0x0000,},
    {P_VENC_VDAC_DACSEL4,            0x0000,},
    {P_VENC_VDAC_DACSEL5,            0x0000,},
    {P_VPU_VIU_VENC_MUX_CTRL,        0x0005,},
    {P_VENC_VDAC_FIFO_CTRL,          0x2000,},
    {P_ENCI_DACSEL_0,                0x0011 },
    {P_ENCI_DACSEL_1,                0x11   },
    {P_ENCP_VIDEO_EN,                0,     },
    {P_ENCI_VIDEO_EN,                1,     },
    {P_ENCI_VIDEO_SAT,               0x7        },
    {P_VENC_VDAC_DAC0_FILT_CTRL0,    0x1        },
    {P_VENC_VDAC_DAC0_FILT_CTRL1,    0xfc48     },
    {P_ENCI_MACV_N0,                 0x0        },
    {MREG_END_MARKER,              0      }
};

static const reg_t tvregs_480p[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },

    //{P_HHI_VID_CLK_DIV,            0x01000100,},
    {P_ENCP_VIDEO_FILT_CTRL,       0x2052,},
    {P_VENC_DVI_SETTING,           0x21,  },
    {P_ENCP_VIDEO_MODE,            0x4000,},
    {P_ENCP_VIDEO_MODE_ADV,        9,     },
    {P_ENCP_VIDEO_YFP1_HTIME,      244,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      1630,  },
    {P_ENCP_VIDEO_YC_DLY,          0,     },
    {P_ENCP_VIDEO_MAX_PXCNT,       1715,  },
    {P_ENCP_VIDEO_MAX_LNCNT,       524,   },
    {P_ENCP_VIDEO_HSPULS_BEGIN,    0x22,  },
    {P_ENCP_VIDEO_HSPULS_END,      0xa0,  },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   88,    },
    {P_ENCP_VIDEO_VSPULS_BEGIN,    0,     },
    {P_ENCP_VIDEO_VSPULS_END,      1589   },
    {P_ENCP_VIDEO_VSPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    5,     },
    {P_ENCP_VIDEO_HAVON_BEGIN,     249,   },
    {P_ENCP_VIDEO_HAVON_END,       1689,  },
    {P_ENCP_VIDEO_VAVON_BLINE,     42,    },
    {P_ENCP_VIDEO_VAVON_ELINE,     521,   },
    {P_ENCP_VIDEO_SYNC_MODE,       0x07,  },
    {P_VENC_VIDEO_PROG_MODE,       0x0,   },
    {P_VENC_VIDEO_EXSRC,           0x0,   },
    {P_ENCP_VIDEO_HSO_BEGIN,       0x3,   },
    {P_ENCP_VIDEO_HSO_END,         0x5,   },
    {P_ENCP_VIDEO_VSO_BEGIN,       0x3,   },
    {P_ENCP_VIDEO_VSO_END,         0x5,   },
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },  //added by JZD. Switch Panel to 480p first time, movie video flicks if not set this to 0
    {P_ENCP_VIDEO_SY_VAL,          8,     },
    {P_ENCP_VIDEO_SY2_VAL,         0x1d8, },
    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_SETTING,          0,     },
    {P_VENC_UPSAMPLE_CTRL0,        0x9061,},
    {P_VENC_UPSAMPLE_CTRL1,        0xa061,},
    {P_VENC_UPSAMPLE_CTRL2,        0xb061,},
    {P_VENC_VDAC_DACSEL0,          0xf003,},
    {P_VENC_VDAC_DACSEL1,          0xf003,},
    {P_VENC_VDAC_DACSEL2,          0xf003,},
    {P_VENC_VDAC_DACSEL3,          0xf003,},
    {P_VENC_VDAC_DACSEL4,          0xf003,},
    {P_VENC_VDAC_DACSEL5,          0xf003,},
    {P_VPU_VIU_VENC_MUX_CTRL,      0x000a,},
    {P_VENC_VDAC_FIFO_CTRL,        0x1000,},
    {P_ENCP_DACSEL_0,              0x3102,},
    {P_ENCP_DACSEL_1,              0x0054,},
    {P_ENCI_VIDEO_EN,              0      },
    {P_ENCP_VIDEO_EN,              1      },
    {MREG_END_MARKER,            0      }
};

static const reg_t tvregs_576i[] = {
    {P_VENC_VDAC_SETTING,               0xff,      },

    {P_ENCI_CFILT_CTRL,                 0x12,    },
    {P_ENCI_CFILT_CTRL2,                 0x12,    },
    {P_VENC_DVI_SETTING,                0,         },
    {P_ENCI_VIDEO_MODE,                 0,         },
    {P_ENCI_VIDEO_MODE_ADV,             0,         },
    {P_ENCI_SYNC_HSO_BEGIN,             3,         },
    {P_ENCI_SYNC_HSO_END,               129,       },
    {P_ENCI_SYNC_VSO_EVNLN,             0x0003     },
    {P_ENCI_SYNC_VSO_ODDLN,             0x0104     },
    {P_ENCI_MACV_MAX_AMP,               0x8107     },
    {P_VENC_VIDEO_PROG_MODE,            0xff       },
    {P_ENCI_VIDEO_MODE,                 0x13       },
    {P_ENCI_VIDEO_MODE_ADV,             0x26,      },
    {P_ENCI_VIDEO_SCH,                  0x28,      },
    {P_ENCI_SYNC_MODE,                  0x07,      },
    {P_ENCI_YC_DELAY,                   0x333,     },
    {P_ENCI_VFIFO2VD_PIXEL_START,       0x010b     },
    {P_ENCI_VFIFO2VD_PIXEL_END,         0x06ab     },
    {P_ENCI_VFIFO2VD_LINE_TOP_START,    0x0016     },
    {P_ENCI_VFIFO2VD_LINE_TOP_END,      0x0136     },
    {P_ENCI_VFIFO2VD_LINE_BOT_START,    0x0017     },
    {P_ENCI_VFIFO2VD_LINE_BOT_END,      0x0137     },
    {P_VENC_SYNC_ROUTE,                 0,         },
    {P_ENCI_DBG_PX_RST,                 0,         },
    {P_VENC_INTCTRL,                    0x2,       },
    {P_ENCI_VFIFO2VD_CTL,               0x4e01,    },
    {P_VENC_VDAC_SETTING,          0,     },
    {P_VENC_UPSAMPLE_CTRL0,             0x0061,    },
    {P_VENC_UPSAMPLE_CTRL1,             0x4061,    },
    {P_VENC_UPSAMPLE_CTRL2,             0x5061,    },
    {P_VENC_VDAC_DACSEL0,               0x0000,    },
    {P_VENC_VDAC_DACSEL1,               0x0000,    },
    {P_VENC_VDAC_DACSEL2,               0x0000,    },
    {P_VENC_VDAC_DACSEL3,               0x0000,    },
    {P_VENC_VDAC_DACSEL4,               0x0000,    },
    {P_VENC_VDAC_DACSEL5,               0x0000,    },
    {P_VPU_VIU_VENC_MUX_CTRL,           0x0005,    },
    {P_VENC_VDAC_FIFO_CTRL,             0x2000,    },
    {P_ENCI_DACSEL_0,                   0x0011     },
    {P_ENCI_DACSEL_1,                   0x87       },
    {P_ENCP_VIDEO_EN,                   0,         },
    {P_ENCI_VIDEO_EN,                   1,         },
    {MREG_END_MARKER,                 0          }
};

static const reg_t tvregs_576cvbs[] = {
    {P_VENC_VDAC_SETTING,               0xff,      },

    {P_ENCI_CFILT_CTRL,                 0x12,    },
    {P_ENCI_CFILT_CTRL2,                 0x12,    },
    {P_VENC_DVI_SETTING,                0,         },
    {P_ENCI_VIDEO_MODE,                 0,         },
    {P_ENCI_VIDEO_MODE_ADV,             0,         },
    {P_ENCI_SYNC_HSO_BEGIN,             3,         },
    {P_ENCI_SYNC_HSO_END,               129,       },
    {P_ENCI_SYNC_VSO_EVNLN,             0x0003     },
    {P_ENCI_SYNC_VSO_ODDLN,             0x0104     },
    {P_ENCI_MACV_MAX_AMP,               0x8107     },
    {P_VENC_VIDEO_PROG_MODE,            0xff       },
    {P_ENCI_VIDEO_MODE,                 0x13       },
    {P_ENCI_VIDEO_MODE_ADV,             0x26,      },
    {P_ENCI_VIDEO_SCH,                  0x28,      },
    {P_ENCI_SYNC_MODE,                  0x07,      },
    {P_ENCI_YC_DELAY,                   0x333,     },
    {P_ENCI_VFIFO2VD_PIXEL_START,       0x0fb	   },
    {P_ENCI_VFIFO2VD_PIXEL_END,         0x069b     },
    {P_ENCI_VFIFO2VD_LINE_TOP_START,    0x0016     },
    {P_ENCI_VFIFO2VD_LINE_TOP_END,      0x0136     },
    {P_ENCI_VFIFO2VD_LINE_BOT_START,    0x0017     },
    {P_ENCI_VFIFO2VD_LINE_BOT_END,      0x0137     },
    {P_VENC_SYNC_ROUTE,                 0,         },
    {P_ENCI_DBG_PX_RST,                 0,         },
    {P_VENC_INTCTRL,                    0x2,       },
    {P_ENCI_VFIFO2VD_CTL,               0x4e01,    },
    {P_VENC_VDAC_SETTING,          0,     },
    {P_VENC_UPSAMPLE_CTRL0,             0x0061,    },
    {P_VENC_UPSAMPLE_CTRL1,             0x4061,    },
    {P_VENC_UPSAMPLE_CTRL2,             0x5061,    },
    {P_VENC_VDAC_DACSEL0,               0x0000,    },
    {P_VENC_VDAC_DACSEL1,               0x0000,    },
    {P_VENC_VDAC_DACSEL2,               0x0000,    },
    {P_VENC_VDAC_DACSEL3,               0x0000,    },
    {P_VENC_VDAC_DACSEL4,               0x0000,    },
    {P_VENC_VDAC_DACSEL5,               0x0000,    },
    {P_VPU_VIU_VENC_MUX_CTRL,           0x0005,    },
    {P_VENC_VDAC_FIFO_CTRL,             0x2000,    },
    {P_ENCI_DACSEL_0,                   0x0011     },
    {P_ENCI_DACSEL_1,                   0x11       },
    {P_ENCP_VIDEO_EN,                   0,         },
    {P_ENCI_VIDEO_EN,                   1,         },
    {P_ENCI_VIDEO_SAT,                  0x7        },
    {P_VENC_VDAC_DAC0_FILT_CTRL0,       0x1        },
    {P_VENC_VDAC_DAC0_FILT_CTRL1,       0xfc48     },
    {P_ENCI_MACV_N0,                    0x0        },
    {MREG_END_MARKER,                 0          }
};

static const reg_t tvregs_576p[] = {
    {P_VENC_VDAC_SETTING,          0xff,      },

    {P_HHI_HDMI_AFC_CNTL,          0x8c0000c3,},
    {P_ENCP_VIDEO_FILT_CTRL,       0x52,      },
    {P_VENC_DVI_SETTING,           0x21,      },
    {P_ENCP_VIDEO_MODE,            0x4000,    },
    {P_ENCP_VIDEO_MODE_ADV,        9,         },
    {P_ENCP_VIDEO_YFP1_HTIME,      235,       },
    {P_ENCP_VIDEO_YFP2_HTIME,      1674,      },
    {P_ENCP_VIDEO_YC_DLY,          0xf,       },
    {P_ENCP_VIDEO_MAX_PXCNT,       1727,      },
    {P_ENCP_VIDEO_MAX_LNCNT,       624,       },
    {P_ENCP_VIDEO_HSPULS_BEGIN,    0,         },
    {P_ENCP_VIDEO_HSPULS_END,      0x80,      },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   88,        },
    {P_ENCP_VIDEO_VSPULS_BEGIN,    0,         },
    {P_ENCP_VIDEO_VSPULS_END,      1599       },
    {P_ENCP_VIDEO_VSPULS_BLINE,    0,         },
    {P_ENCP_VIDEO_VSPULS_ELINE,    4,         },
    {P_ENCP_VIDEO_HAVON_BEGIN,     235,       },
    {P_ENCP_VIDEO_HAVON_END,       1674,      },
    {P_ENCP_VIDEO_VAVON_BLINE,     44,        },
    {P_ENCP_VIDEO_VAVON_ELINE,     619,       },
    {P_ENCP_VIDEO_SYNC_MODE,       0x07,      },
    {P_VENC_VIDEO_PROG_MODE,       0x0,       },
    {P_VENC_VIDEO_EXSRC,           0x0,       },
    {P_ENCP_VIDEO_HSO_BEGIN,       0x80,      },
    {P_ENCP_VIDEO_HSO_END,         0x0,       },
    {P_ENCP_VIDEO_VSO_BEGIN,       0x0,       },
    {P_ENCP_VIDEO_VSO_END,         0x5,       },
    {P_ENCP_VIDEO_VSO_BLINE,       0,         },
    {P_ENCP_VIDEO_SY_VAL,          8,         },
    {P_ENCP_VIDEO_SY2_VAL,         0x1d8,     },
    {P_VENC_SYNC_ROUTE,            0,         },
    {P_VENC_INTCTRL,               0x200,     },
    {P_ENCP_VFIFO2VD_CTL,               0,         },
    {P_VENC_VDAC_SETTING,          0,     },
    {P_VENC_UPSAMPLE_CTRL0,        0x9061,    },
    {P_VENC_UPSAMPLE_CTRL1,        0xa061,    },
    {P_VENC_UPSAMPLE_CTRL2,        0xb061,    },
    {P_VENC_VDAC_DACSEL0,          0xf003,    },
    {P_VENC_VDAC_DACSEL1,          0xf003,    },
    {P_VENC_VDAC_DACSEL2,          0xf003,    },
    {P_VENC_VDAC_DACSEL3,          0xf003,    },
    {P_VENC_VDAC_DACSEL4,          0xf003,    },
    {P_VENC_VDAC_DACSEL5,          0xf003,    },
    {P_VPU_VIU_VENC_MUX_CTRL,      0x000a,    },
    {P_VENC_VDAC_FIFO_CTRL,        0x1000,    },
    {P_ENCP_DACSEL_0,              0x3102,    },
    {P_ENCP_DACSEL_1,              0x0054,    },
    {P_ENCI_VIDEO_EN,              0          },
    {P_ENCP_VIDEO_EN,              1          },
    {MREG_END_MARKER,            0          }
};

static const reg_t tvregs_1080i[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_FILT_CTRL,       0x0052,},
    {P_VENC_DVI_SETTING,           0x2029,},
    {P_ENCP_VIDEO_MAX_PXCNT,       4399,  },
    {P_ENCP_VIDEO_MAX_LNCNT,       1124,  },
    {P_ENCP_VIDEO_HSPULS_BEGIN,    88,    },
    {P_ENCP_VIDEO_HSPULS_END,      264,   },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   88,    },
    {P_ENCP_VIDEO_HAVON_BEGIN,     516,   },
    {P_ENCP_VIDEO_HAVON_END,       4355,  },
    {P_ENCP_VIDEO_HSO_BEGIN,       264,   },
    {P_ENCP_VIDEO_HSO_END,         176,   },
    {P_ENCP_VIDEO_EQPULS_BEGIN,    2288,  },
    {P_ENCP_VIDEO_EQPULS_END,      2464,  },
    {P_ENCP_VIDEO_VSPULS_BEGIN,    440,   },
    {P_ENCP_VIDEO_VSPULS_END,      2200,  },
    {P_ENCP_VIDEO_VSPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    4,     },
    {P_ENCP_VIDEO_EQPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_EQPULS_ELINE,    4,     },
    {P_ENCP_VIDEO_VAVON_BLINE,     20,    },
    {P_ENCP_VIDEO_VAVON_ELINE,     559,   },
    {P_ENCP_VIDEO_VSO_BEGIN,       88,    },
    {P_ENCP_VIDEO_VSO_END,         88,    },
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_VSO_ELINE,       5,     },
    {P_ENCP_VIDEO_YFP1_HTIME,      516,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      4355,  },
    {P_VENC_VIDEO_PROG_MODE,       0x100, },
    {P_ENCP_VIDEO_OFLD_VOAV_OFST,  0x11   },
    {P_ENCP_VIDEO_MODE,            0x5ffc,},
    {P_ENCP_VIDEO_MODE_ADV,        0x0019,},
    {P_ENCP_VIDEO_SYNC_MODE,       0x207, },
    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_FIFO_CTRL,        0x1000,},
    {P_VENC_VDAC_SETTING,          0,     },
    {P_ENCP_DACSEL_0,              0x3102,},
    {P_ENCP_DACSEL_1,              0x0054,},
    {P_VENC_VDAC_DACSEL0,          0x0001,},
    {P_VENC_VDAC_DACSEL1,          0x0001,},
    {P_VENC_VDAC_DACSEL2,          0x0001,},
    {P_VENC_VDAC_DACSEL3,          0x0001,},
    {P_VENC_VDAC_DACSEL4,          0x0001,},
    {P_VENC_VDAC_DACSEL5,          0x0001,},
    {P_ENCI_VIDEO_EN,              0,     },
    {P_ENCP_VIDEO_EN,              1,     },
    {MREG_END_MARKER,            0      }
};

static const reg_t tvregs_1080i_50hz[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },
//	{VCLK_HD},

    {P_ENCP_VIDEO_FILT_CTRL,       0x0052,},

    {P_VENC_DVI_SETTING,           0x202d,},
    {P_ENCP_VIDEO_MAX_PXCNT,       5279,  },
    {P_ENCP_VIDEO_MAX_LNCNT,       1124,  },

    //analog vidoe position in horizontal
    {P_ENCP_VIDEO_HSPULS_BEGIN,    88,    },
    {P_ENCP_VIDEO_HSPULS_END,      264,   },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   88,    },

    //DE position in horizontal
    {P_ENCP_VIDEO_HAVON_BEGIN,     526,   },
    {P_ENCP_VIDEO_HAVON_END,       4365,  },

    //ditital hsync positon in horizontal
    {P_ENCP_VIDEO_HSO_BEGIN,       142,   },
    {P_ENCP_VIDEO_HSO_END,         230,   },

    /* vsync horizontal timing */
    {P_ENCP_VIDEO_EQPULS_BEGIN,    2728,  },
    {P_ENCP_VIDEO_EQPULS_END,      2904,  },
    {P_ENCP_VIDEO_VSPULS_BEGIN,    440,   },
    {P_ENCP_VIDEO_VSPULS_END,      2200,  },

    {P_ENCP_VIDEO_VSPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    4,     },
    {P_ENCP_VIDEO_EQPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_EQPULS_ELINE,    4,     },

    //DE position in vertical
    {P_ENCP_VIDEO_VAVON_BLINE,     20,    },
    {P_ENCP_VIDEO_VAVON_ELINE,     559,   },

    //adjust vsync start point and end point
    {P_ENCP_VIDEO_VSO_BEGIN,       142,    },
    {P_ENCP_VIDEO_VSO_END,         142,    },

    //adjust the vsync start line and end line
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_VSO_ELINE,       5,     },

    /* filter & misc settings */
    {P_ENCP_VIDEO_YFP1_HTIME,      526,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      4365,  },

    {P_VENC_VIDEO_PROG_MODE,       0x100, },  // Select clk108 as DAC clock, progressive mode
    {P_ENCP_VIDEO_OFLD_VOAV_OFST,  0x11   },//bit[15:12]: Odd field VSO  offset begin,
                                                        //bit[11:8]: Odd field VSO  offset end,
                                                        //bit[7:4]: Odd field VAVON offset begin,
                                                        //bit[3:0]: Odd field VAVON offset end,
    {P_ENCP_VIDEO_MODE,            0x5ffc,},//Enable Hsync and equalization pulse switch in center
    {P_ENCP_VIDEO_MODE_ADV,        0x0019,}, //bit6:swap PbPr; bit4:YPBPR gain as HDTV type;
                                                 //bit3:Data input from VFIFO;bit[2}0]:repreat pixel a time
    {P_ENCP_VIDEO_SYNC_MODE,       0x7, }, //bit[15:8] -- adjust the vsync vertical position
    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_FIFO_CTRL,        0x1000,},
    {P_VENC_VDAC_SETTING,          0,     },
    {P_ENCP_DACSEL_0,              0x3102,},
    {P_ENCP_DACSEL_1,              0x0054,},
    {P_VENC_VDAC_DACSEL0,          0x0001,},
    {P_VENC_VDAC_DACSEL1,          0x0001,},
    {P_VENC_VDAC_DACSEL2,          0x0001,},
    {P_VENC_VDAC_DACSEL3,          0x0001,},
    {P_VENC_VDAC_DACSEL4,          0x0001,},
    {P_VENC_VDAC_DACSEL5,          0x0001,},
    {P_ENCI_VIDEO_EN,              0,     },
    {P_ENCP_VIDEO_EN,              1,     },
    {MREG_END_MARKER,            0      }
};

static const reg_t tvregs_1080p[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_FILT_CTRL,       0x1052,},
    {P_VENC_DVI_SETTING,           0x0001,},
    {P_ENCP_VIDEO_MODE,            0x4040,},
    {P_ENCP_VIDEO_MODE_ADV,        0x0018,},
    {P_ENCP_VIDEO_YFP1_HTIME,      140,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      2060,  },
    {P_ENCP_VIDEO_MAX_PXCNT,       2199,  },
    {P_ENCP_VIDEO_HSPULS_BEGIN,    2156,  },//1980
    {P_ENCP_VIDEO_HSPULS_END,      44,    },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   44,    },
    {P_ENCP_VIDEO_VSPULS_BEGIN,    140,   },
    {P_ENCP_VIDEO_VSPULS_END,      2059,  },
    {P_ENCP_VIDEO_VSPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    4,     },//35
    {P_ENCP_VIDEO_HAVON_BEGIN,     148,   },
    {P_ENCP_VIDEO_HAVON_END,       2067,  },
    {P_ENCP_VIDEO_VAVON_BLINE,     41,    },
    {P_ENCP_VIDEO_VAVON_ELINE,     1120,  },
    {P_ENCP_VIDEO_HSO_BEGIN,       44,    },
    {P_ENCP_VIDEO_HSO_END,         2156,  },
    {P_ENCP_VIDEO_VSO_BEGIN,       2100,  },
    {P_ENCP_VIDEO_VSO_END,         2164,  },
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_VSO_ELINE,       5,     },
    {P_ENCP_VIDEO_MAX_LNCNT,       1124,  },
    {P_VPU_VIU_VENC_MUX_CTRL,      0x000a,},      //New Add. If not set, when system boots up, switch panel to HDMI 1080P, nothing on TV.
    {P_VENC_VIDEO_PROG_MODE,       0x100, },
    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_SETTING,          0,     },
    {P_VENC_VDAC_DACSEL0,          0x0001,},
    {P_VENC_VDAC_DACSEL1,          0x0001,},
    {P_VENC_VDAC_DACSEL2,          0x0001,},
    {P_VENC_VDAC_DACSEL3,          0x0001,},
    {P_VENC_VDAC_DACSEL4,          0x0001,},
    {P_VENC_VDAC_DACSEL5,          0x0001,},
    {P_VENC_VDAC_FIFO_CTRL,        0x1000,},
    {P_ENCP_DACSEL_0,              0x3102,},
    {P_ENCP_DACSEL_1,              0x0054,},
    {P_ENCI_VIDEO_EN,              0,     },
    {P_ENCP_VIDEO_EN,              1,     },
    {MREG_END_MARKER,            0      }
};

static const reg_t tvregs_1080p_50hz[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_FILT_CTRL,       0x1052,},

    // bit 13    1          (delayed prog_vs)
    // bit 5:4:  2          (pixel[0])
    // bit 3:    1          invert vsync or not
    // bit 2:    1          invert hsync or not
    // bit1:     1          (select viu sync)
    // bit0:     1          (progressive)
    {P_VENC_DVI_SETTING,           0x000d,},
    {P_ENCP_VIDEO_MAX_PXCNT,       2639,  },
    {P_ENCP_VIDEO_MAX_LNCNT,       1124,  },
    /* horizontal timing settings */
    {P_ENCP_VIDEO_HSPULS_BEGIN,    44,  },//1980
    {P_ENCP_VIDEO_HSPULS_END,      132,    },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   44,    },

    //DE position in horizontal
    {P_ENCP_VIDEO_HAVON_BEGIN,     271,   },
    {P_ENCP_VIDEO_HAVON_END,       2190,  },

    //ditital hsync positon in horizontal
    {P_ENCP_VIDEO_HSO_BEGIN,       79 ,    },
    {P_ENCP_VIDEO_HSO_END,         123,  },

    /* vsync horizontal timing */
    {P_ENCP_VIDEO_VSPULS_BEGIN,    220,   },
    {P_ENCP_VIDEO_VSPULS_END,      2140,  },

    /* vertical timing settings */
    {P_ENCP_VIDEO_VSPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    4,     },//35
    {P_ENCP_VIDEO_EQPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_EQPULS_ELINE,    4,     },//35
    {P_ENCP_VIDEO_VAVON_BLINE,     41,    },
    {P_ENCP_VIDEO_VAVON_ELINE,     1120,  },

    //adjust the hsync & vsync start point and end point
    {P_ENCP_VIDEO_VSO_BEGIN,       79,  },
    {P_ENCP_VIDEO_VSO_END,         79,  },

    //adjust the vsync start line and end line
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_VSO_ELINE,       5,     },

    {P_ENCP_VIDEO_YFP1_HTIME,      271,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      2190,  },
    {P_VENC_VIDEO_PROG_MODE,       0x100, },
    {P_ENCP_VIDEO_MODE,            0x4040,},
    {P_ENCP_VIDEO_MODE_ADV,        0x0018,},

    {P_ENCP_VIDEO_SYNC_MODE,       0x7, }, //bit[15:8] -- adjust the vsync vertical position

    {P_ENCP_VIDEO_YC_DLY,          0,     },      //Y/Cb/Cr delay

    {P_ENCP_VIDEO_RGB_CTRL, 2,},       // enable sync on B

    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_FIFO_CTRL,        0x1000,},
    {P_VENC_VDAC_SETTING,          0,     },
    {P_VPU_VIU_VENC_MUX_CTRL,      0x000a,},
    {P_ENCP_DACSEL_0,              0x3102,},
    {P_ENCP_DACSEL_1,              0x0054,},
    {P_VENC_VDAC_DACSEL0,          0x0001,},
    {P_VENC_VDAC_DACSEL1,          0x0001,},
    {P_VENC_VDAC_DACSEL2,          0x0001,},
    {P_VENC_VDAC_DACSEL3,          0x0001,},
    {P_VENC_VDAC_DACSEL4,          0x0001,},
    {P_VENC_VDAC_DACSEL5,          0x0001,},
    {P_ENCI_VIDEO_EN,              0,     },
    {P_ENCP_VIDEO_EN,              1,     },
    {MREG_END_MARKER,            0      }
};

static const reg_t tvregs_1080p_24hz[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_FILT_CTRL,       0x1052,},

    // bit 13    1          (delayed prog_vs)
    // bit 5:4:  2          (pixel[0])
    // bit 3:    1          invert vsync or not
    // bit 2:    1          invert hsync or not
    // bit1:     1          (select viu sync)
    // bit0:     1          (progressive)
    {P_VENC_DVI_SETTING,           0x000d,},
    {P_ENCP_VIDEO_MAX_PXCNT,       2749,  },
    {P_ENCP_VIDEO_MAX_LNCNT,       1124,  },
    /* horizontal timing settings */
    {P_ENCP_VIDEO_HSPULS_BEGIN,    44,  },//1980
    {P_ENCP_VIDEO_HSPULS_END,      132,    },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   44,    },

    //DE position in horizontal
    {P_ENCP_VIDEO_HAVON_BEGIN,     271,   },
    {P_ENCP_VIDEO_HAVON_END,       2190,  },

    //ditital hsync positon in horizontal
    {P_ENCP_VIDEO_HSO_BEGIN,       79 ,    },
    {P_ENCP_VIDEO_HSO_END,         123,  },

    /* vsync horizontal timing */
    {P_ENCP_VIDEO_VSPULS_BEGIN,    220,   },
    {P_ENCP_VIDEO_VSPULS_END,      2140,  },

    /* vertical timing settings */
    {P_ENCP_VIDEO_VSPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    4,     },//35
    {P_ENCP_VIDEO_EQPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_EQPULS_ELINE,    4,     },//35
    {P_ENCP_VIDEO_VAVON_BLINE,     41,    },
    {P_ENCP_VIDEO_VAVON_ELINE,     1120,  },

    //adjust the hsync & vsync start point and end point
    {P_ENCP_VIDEO_VSO_BEGIN,       79,  },
    {P_ENCP_VIDEO_VSO_END,         79,  },

    //adjust the vsync start line and end line
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_VSO_ELINE,       5,     },

    {P_ENCP_VIDEO_YFP1_HTIME,      271,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      2190,  },
    {P_VENC_VIDEO_PROG_MODE,       0x100, },
    {P_ENCP_VIDEO_MODE,            0x4040,},
    {P_ENCP_VIDEO_MODE_ADV,        0x0018,},

    {P_ENCP_VIDEO_SYNC_MODE,       0x7, }, //bit[15:8] -- adjust the vsync vertical position

    {P_ENCP_VIDEO_YC_DLY,          0,     },      //Y/Cb/Cr delay

    {P_ENCP_VIDEO_RGB_CTRL, 2,},       // enable sync on B

    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_FIFO_CTRL,        0x1000,},
    {P_VENC_VDAC_SETTING,          0,     },
    {P_VPU_VIU_VENC_MUX_CTRL,      0x000a,},
    {P_ENCP_DACSEL_0,              0x3102,},
    {P_ENCP_DACSEL_1,              0x0054,},
    {P_VENC_VDAC_DACSEL0,          0x0001,},
    {P_VENC_VDAC_DACSEL1,          0x0001,},
    {P_VENC_VDAC_DACSEL2,          0x0001,},
    {P_VENC_VDAC_DACSEL3,          0x0001,},
    {P_VENC_VDAC_DACSEL4,          0x0001,},
    {P_VENC_VDAC_DACSEL5,          0x0001,},
    {P_ENCI_VIDEO_EN,              0,     },
    {P_ENCP_VIDEO_EN,              1,     },
    {MREG_END_MARKER,            0      }
};

static const reg_t tvregs_4k2k_30hz[] = {
    {P_ENCP_VIDEO_MODE,             0x4040}, // Enable Hsync and equalization pulse switch in center; bit[14] cfg_de_v = 1
    {P_ENCP_VIDEO_MODE_ADV,         0x0008}, // Sampling rate: 1
    {P_ENCP_VIDEO_YFP1_HTIME,       140},
    {P_ENCP_VIDEO_YFP2_HTIME,       140+3840},

    {P_ENCP_VIDEO_MAX_PXCNT,        3840+560-1},
    {P_ENCP_VIDEO_HSPULS_BEGIN,     2156+1920},
    {P_ENCP_VIDEO_HSPULS_END,       44},
    {P_ENCP_VIDEO_HSPULS_SWITCH,    44},
    {P_ENCP_VIDEO_VSPULS_BEGIN,     140},
    {P_ENCP_VIDEO_VSPULS_END,       2059+1920},
    {P_ENCP_VIDEO_VSPULS_BLINE,     0},
    {P_ENCP_VIDEO_VSPULS_ELINE,     4},

    {P_ENCP_VIDEO_HAVON_BEGIN,      148},
    {P_ENCP_VIDEO_HAVON_END,        3987},
    {P_ENCP_VIDEO_VAVON_BLINE,      89},
    {P_ENCP_VIDEO_VAVON_ELINE,      2248},

    {P_ENCP_VIDEO_HSO_BEGIN,	    44},
    {P_ENCP_VIDEO_HSO_END, 		    2156+1920},
    {P_ENCP_VIDEO_VSO_BEGIN,	    2100+1920},
    {P_ENCP_VIDEO_VSO_END, 		    2164+1920},

    {P_ENCP_VIDEO_VSO_BLINE,        51},
    {P_ENCP_VIDEO_VSO_ELINE,        53},
    {P_ENCP_VIDEO_MAX_LNCNT,        2249},

    {P_ENCP_VIDEO_FILT_CTRL,        0x1000}, //bypass filter
    {MREG_END_MARKER,            0      },
};

static const reg_t tvregs_4k2k_25hz[] = {
    {P_ENCP_VIDEO_MODE,             0x4040}, // Enable Hsync and equalization pulse switch in center; bit[14] cfg_de_v = 1
    {P_ENCP_VIDEO_MODE_ADV,         0x0008}, // Sampling rate: 1
    {P_ENCP_VIDEO_YFP1_HTIME,       140},
    {P_ENCP_VIDEO_YFP2_HTIME,       140+3840},

    {P_ENCP_VIDEO_MAX_PXCNT,        3840+1440-1},
    {P_ENCP_VIDEO_HSPULS_BEGIN,     2156+1920},
    {P_ENCP_VIDEO_HSPULS_END,       44},
    {P_ENCP_VIDEO_HSPULS_SWITCH,    44},
    {P_ENCP_VIDEO_VSPULS_BEGIN,     140},
    {P_ENCP_VIDEO_VSPULS_END,       2059+1920},
    {P_ENCP_VIDEO_VSPULS_BLINE,     0},
    {P_ENCP_VIDEO_VSPULS_ELINE,     4},

    {P_ENCP_VIDEO_HAVON_BEGIN,      148},
    {P_ENCP_VIDEO_HAVON_END,        3987},
    {P_ENCP_VIDEO_VAVON_BLINE,      89},
    {P_ENCP_VIDEO_VAVON_ELINE,      2248},

    {P_ENCP_VIDEO_HSO_BEGIN,	    44},
    {P_ENCP_VIDEO_HSO_END, 		    2156+1920},
    {P_ENCP_VIDEO_VSO_BEGIN,	    2100+1920},
    {P_ENCP_VIDEO_VSO_END, 		    2164+1920},

    {P_ENCP_VIDEO_VSO_BLINE,        51},
    {P_ENCP_VIDEO_VSO_ELINE,        53},
    {P_ENCP_VIDEO_MAX_LNCNT,        2249},

    {P_ENCP_VIDEO_FILT_CTRL,        0x1000}, //bypass filter
    {MREG_END_MARKER,            0      },
};

static const reg_t tvregs_4k2k_24hz[] = {
    {P_ENCP_VIDEO_MODE,             0x4040}, // Enable Hsync and equalization pulse switch in center; bit[14] cfg_de_v = 1
    {P_ENCP_VIDEO_MODE_ADV,         0x0008}, // Sampling rate: 1
    {P_ENCP_VIDEO_YFP1_HTIME,       140},
    {P_ENCP_VIDEO_YFP2_HTIME,       140+3840},

    {P_ENCP_VIDEO_MAX_PXCNT,        3840+1660-1},
    {P_ENCP_VIDEO_HSPULS_BEGIN,     2156+1920},
    {P_ENCP_VIDEO_HSPULS_END,       44},
    {P_ENCP_VIDEO_HSPULS_SWITCH,    44},
    {P_ENCP_VIDEO_VSPULS_BEGIN,     140},
    {P_ENCP_VIDEO_VSPULS_END,       2059+1920},
    {P_ENCP_VIDEO_VSPULS_BLINE,     0},
    {P_ENCP_VIDEO_VSPULS_ELINE,     4},

    {P_ENCP_VIDEO_HAVON_BEGIN,      148},
    {P_ENCP_VIDEO_HAVON_END,        3987},
    {P_ENCP_VIDEO_VAVON_BLINE,      89},
    {P_ENCP_VIDEO_VAVON_ELINE,      2248},

    {P_ENCP_VIDEO_HSO_BEGIN,	    44},
    {P_ENCP_VIDEO_HSO_END, 		    2156+1920},
    {P_ENCP_VIDEO_VSO_BEGIN,	    2100+1920},
    {P_ENCP_VIDEO_VSO_END, 		    2164+1920},

    {P_ENCP_VIDEO_VSO_BLINE,        51},
    {P_ENCP_VIDEO_VSO_ELINE,        53},
    {P_ENCP_VIDEO_MAX_LNCNT,        2249},

    {P_ENCP_VIDEO_FILT_CTRL,        0x1000}, //bypass filter

    {MREG_END_MARKER,            0      },
};

static const reg_t tvregs_4k2k_smpte[] = {      //24hz
    {P_ENCP_VIDEO_MODE,             0x4040}, // Enable Hsync and equalization pulse switch in center; bit[14] cfg_de_v = 1
    {P_ENCP_VIDEO_MODE_ADV,         0x0008}, // Sampling rate: 1
    {P_ENCP_VIDEO_YFP1_HTIME,       140},
    {P_ENCP_VIDEO_YFP2_HTIME,       140+3840+256},

    {P_ENCP_VIDEO_MAX_PXCNT,        4096+1404-1},
    {P_ENCP_VIDEO_HSPULS_BEGIN,     2156+1920},
    {P_ENCP_VIDEO_HSPULS_END,       44},
    {P_ENCP_VIDEO_HSPULS_SWITCH,    44},
    {P_ENCP_VIDEO_VSPULS_BEGIN,     140},
    {P_ENCP_VIDEO_VSPULS_END,       2059+1920},
    {P_ENCP_VIDEO_VSPULS_BLINE,     0},
    {P_ENCP_VIDEO_VSPULS_ELINE,     4},

    {P_ENCP_VIDEO_HAVON_BEGIN,      148},
    {P_ENCP_VIDEO_HAVON_END,        3987+256},
    {P_ENCP_VIDEO_VAVON_BLINE,      89},
    {P_ENCP_VIDEO_VAVON_ELINE,      2248},

    {P_ENCP_VIDEO_HSO_BEGIN,	    44},
    {P_ENCP_VIDEO_HSO_END, 		    2156+1920+256},
    {P_ENCP_VIDEO_VSO_BEGIN,	    2100+1920+256},
    {P_ENCP_VIDEO_VSO_END, 		    2164+1920+256},

    {P_ENCP_VIDEO_VSO_BLINE,        51},
    {P_ENCP_VIDEO_VSO_ELINE,        53},
    {P_ENCP_VIDEO_MAX_LNCNT,        2249},

    {P_ENCP_VIDEO_FILT_CTRL,        0x1000}, //bypass filter
    {MREG_END_MARKER,            0      },
};

static const reg_t tvregs_vga_640x480[] = { // 25.17mhz 800 *525
     {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_FILT_CTRL,       0x1052,},
    //{P_HHI_VID_CLK_DIV,            0x01000100,},
    {P_ENCP_VIDEO_FILT_CTRL,       0x2052,},
    {P_VENC_DVI_SETTING,           0x21,  },
    {P_ENCP_VIDEO_MODE,            0,     },
    {P_ENCP_VIDEO_MODE_ADV,        0x009,     },
    {P_ENCP_VIDEO_YFP1_HTIME,      244,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      1630,  },
    {P_ENCP_VIDEO_YC_DLY,          0,     },
    {P_ENCP_VIDEO_MAX_PXCNT,       1599,  },
    {P_ENCP_VIDEO_MAX_LNCNT,       525,   },
    {P_ENCP_VIDEO_HSPULS_BEGIN,    0x60,  },
    {P_ENCP_VIDEO_HSPULS_END,      0xa0,  },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   88,    },
    {P_ENCP_VIDEO_VSPULS_BEGIN,    0,     },
    {P_ENCP_VIDEO_VSPULS_END,      1589   },
    {P_ENCP_VIDEO_VSPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    5,     },
    {P_ENCP_VIDEO_HAVON_BEGIN,     153,   },
    {P_ENCP_VIDEO_HAVON_END,       1433,  },
    {P_ENCP_VIDEO_VAVON_BLINE,     59,    },
    {P_ENCP_VIDEO_VAVON_ELINE,     540,   },
    {P_ENCP_VIDEO_SYNC_MODE,       0x07,  },
    {P_VENC_VIDEO_PROG_MODE,       0x100,   },
    {P_VENC_VIDEO_EXSRC,           0x0,   },
    {P_ENCP_VIDEO_HSO_BEGIN,       0x3,   },
    {P_ENCP_VIDEO_HSO_END,         0x5,   },
    {P_ENCP_VIDEO_VSO_BEGIN,       0x3,   },
    {P_ENCP_VIDEO_VSO_END,         0x5,   },
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_SY_VAL,          8,     },
    {P_ENCP_VIDEO_SY2_VAL,         0x1d8, },
    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_SETTING,          0,     },
    /////////////////////////////
    {P_ENCP_VIDEO_RGB_CTRL,		 0,},
    {P_VENC_UPSAMPLE_CTRL0,        0xc061,},
    {P_VENC_UPSAMPLE_CTRL1,        0xd061,},
    {P_VENC_UPSAMPLE_CTRL2,        0xe061,},
    {P_VENC_VDAC_DACSEL0,          0xf003,},
    {P_VENC_VDAC_DACSEL1,          0xf003,},
    {P_VENC_VDAC_DACSEL2,          0xf003,},
    {P_VENC_VDAC_DACSEL3,          0xf003,},
    {P_VENC_VDAC_DACSEL4,          0xf003,},
    {P_VENC_VDAC_DACSEL5,          0xf003,},
    {P_VPU_VIU_VENC_MUX_CTRL,      0x000a,},
    {P_VENC_VDAC_FIFO_CTRL,        0x1fc0,},
    {P_ENCP_DACSEL_0,              0x0543,},
    {P_ENCP_DACSEL_1,              0x0000,},

    {P_ENCI_VIDEO_EN,              0      },
    {P_ENCP_VIDEO_EN,              1      },
    {MREG_END_MARKER,            0      }
/////////////////////////////////////
};
static const reg_t tvregs_svga_800x600[]={ //39.5mhz 1056 *628
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_FILT_CTRL,       0x0052,},
    {P_VENC_DVI_SETTING,           0x2029,},
    {P_ENCP_VIDEO_MODE,            0x0040,},
    {P_ENCP_VIDEO_MODE_ADV,        0x0019,},
    {P_ENCP_VIDEO_YFP1_HTIME,      500,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      2112,  },
    {P_ENCP_VIDEO_MAX_PXCNT,       2111,  },
    {P_ENCP_VIDEO_MAX_LNCNT,       628,   },//628
    {P_ENCP_VIDEO_HSPULS_BEGIN,    0,    },
    {P_ENCP_VIDEO_HSPULS_END,      230,   },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   80,    },
    {P_ENCP_VIDEO_VSPULS_BEGIN,    0x58,   },
    {P_ENCP_VIDEO_VSPULS_END,      0x80,  },
    {P_ENCP_VIDEO_VSPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    5,     },
    {P_ENCP_VIDEO_EQPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_EQPULS_ELINE,    5,     },
    {P_ENCP_VIDEO_HAVON_BEGIN,     267,   },//59
    {P_ENCP_VIDEO_HAVON_END,       1866,  },//1659
    {P_ENCP_VIDEO_VAVON_BLINE,    59,    },//59
    {P_ENCP_VIDEO_VAVON_ELINE,     658,   },//659
    {P_ENCP_VIDEO_HSO_BEGIN,       0,    },
    {P_ENCP_VIDEO_HSO_END,         260,   },
    {P_ENCP_VIDEO_VSO_BEGIN,       0,   },
    {P_ENCP_VIDEO_VSO_END,         2200,   },
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_VSO_ELINE,       5,     },
    {P_VENC_VIDEO_PROG_MODE,       0x100, },
    {P_VENC_VIDEO_EXSRC,           0x0,   },
    {P_ENCP_VIDEO_HSO_BEGIN,       0x3,   },
    {P_ENCP_VIDEO_HSO_END,         0x5,   },
    {P_ENCP_VIDEO_VSO_BEGIN,       0x3,   },
    {P_ENCP_VIDEO_VSO_END,         0x5,   },
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_SY_VAL,          8,     },
    {P_ENCP_VIDEO_SY2_VAL,         0x1d8, },
    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_SETTING,          0,     },
    {P_ENCP_VIDEO_RGB_CTRL,		 0,},
    {P_VENC_UPSAMPLE_CTRL0,        0xc061,},
    {P_VENC_UPSAMPLE_CTRL1,        0xd061,},
    {P_VENC_UPSAMPLE_CTRL2,        0xe061,},
    {P_VENC_VDAC_DACSEL0,          0xf003,},
    {P_VENC_VDAC_DACSEL1,          0xf003,},
    {P_VENC_VDAC_DACSEL2,          0xf003,},
    {P_VENC_VDAC_DACSEL3,          0xf003,},
    {P_VENC_VDAC_DACSEL4,          0xf003,},
    {P_VENC_VDAC_DACSEL5,          0xf003,},
    {P_VPU_VIU_VENC_MUX_CTRL,      0x000a,},
    {P_VENC_VDAC_FIFO_CTRL,        0x1fc0,},
    {P_ENCP_DACSEL_0,              0x0543,},
    {P_ENCP_DACSEL_1,              0x0000,},
    {P_ENCI_VIDEO_EN,              0      },
    {P_ENCP_VIDEO_EN,              1      },
    {MREG_END_MARKER,            0      }
	//////////////////////////////
 };
static const reg_t tvregs_xga_1024x768[] = {
   /* {P_VENC_VDAC_SETTING,          0xff,  },
    {P_HHI_VID_CLK_CNTL,           0x0,},
    {P_HHI_VID_PLL_CNTL2,          0x814d3928},
    {P_HHI_VID_PLL_CNTL3,          0x6b425012},
    {P_HHI_VID_PLL_CNTL4,          0x110},
    {P_HHI_VID_PLL_CNTL,           0x0001043e,},
    {P_HHI_VID_DIVIDER_CNTL,       0x00010843,},
    {P_HHI_VID_CLK_DIV,            0x100},
    {P_HHI_VID_CLK_CNTL,           0x80000,},
    {P_HHI_VID_CLK_CNTL,           0x88001,},
    {P_HHI_VID_CLK_CNTL,           0x80003,},
    {P_HHI_VIID_CLK_DIV,           0x00000101,},
    {P_ENCP_VIDEO_FILT_CTRL,       0x0052,},
    {P_VENC_DVI_SETTING,           0x2029,},
    {P_ENCP_VIDEO_MODE,            0x0040,},
    {P_ENCP_VIDEO_MODE_ADV,        0x0009,},
    {P_ENCP_VIDEO_YFP1_HTIME,      500,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      2500,  },
    {P_ENCP_VIDEO_MAX_PXCNT,       2531,  },
    {P_ENCP_VIDEO_MAX_LNCNT,       804,   },
    {P_ENCP_VIDEO_HSPULS_BEGIN,    0,    },
    {P_ENCP_VIDEO_HSPULS_END,      230,   },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   80,    },
    {P_ENCP_VIDEO_VSPULS_BEGIN,    0x22,   },
    {P_ENCP_VIDEO_VSPULS_END,      0xa0,  },
    {P_ENCP_VIDEO_VSPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    5,     },
    {P_ENCP_VIDEO_EQPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_EQPULS_ELINE,    5,     },
    {P_ENCP_VIDEO_HAVON_BEGIN,     59,   },
    {P_ENCP_VIDEO_HAVON_END,       2106,  },
    {P_ENCP_VIDEO_VAVON_BLINE,     59,    },
    {P_ENCP_VIDEO_VAVON_ELINE,     827,   },//827
    {P_ENCP_VIDEO_HSO_BEGIN,       0,    },
    {P_ENCP_VIDEO_HSO_END,         260,   },
    {P_ENCP_VIDEO_VSO_BEGIN,       0,   },
    {P_ENCP_VIDEO_VSO_END,         2200,   },
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_VSO_ELINE,       5,     },
    {P_VENC_VIDEO_PROG_MODE,       0x100, },
    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_SETTING,          0,     },
    */
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_FILT_CTRL,       0x0052,},
    {P_VENC_DVI_SETTING,           0x2029,},
    {P_ENCP_VIDEO_MODE,            0x0040,},
    {P_ENCP_VIDEO_MODE_ADV,        0x0009,},
    {P_ENCP_VIDEO_YFP1_HTIME,      500,   },
    {P_ENCP_VIDEO_YFP2_HTIME,      2500,  },
    {P_ENCP_VIDEO_MAX_PXCNT,       2691,  },
    {P_ENCP_VIDEO_MAX_LNCNT,       806,   },
    {P_ENCP_VIDEO_HSPULS_BEGIN,    0,    },
    {P_ENCP_VIDEO_HSPULS_END,      230,   },
    {P_ENCP_VIDEO_HSPULS_SWITCH,   80,    },
    {P_ENCP_VIDEO_VSPULS_BEGIN,    0x22,   },
    {P_ENCP_VIDEO_VSPULS_END,      0xa0,  },
    {P_ENCP_VIDEO_VSPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_VSPULS_ELINE,    5,     },
    {P_ENCP_VIDEO_EQPULS_BLINE,    0,     },
    {P_ENCP_VIDEO_EQPULS_ELINE,    5,     },
    {P_ENCP_VIDEO_HAVON_BEGIN,     315,   },
    {P_ENCP_VIDEO_HAVON_END,       2362,  },
    {P_ENCP_VIDEO_VAVON_BLINE,     59,    },
    {P_ENCP_VIDEO_VAVON_ELINE,     827,   },//827
    {P_ENCP_VIDEO_HSO_BEGIN,       0,    },
    {P_ENCP_VIDEO_HSO_END,         260,   },
    {P_ENCP_VIDEO_VSO_BEGIN,       0,   },
    {P_ENCP_VIDEO_VSO_END,         2200,   },
    {P_ENCP_VIDEO_VSO_BLINE,       0,     },
    {P_ENCP_VIDEO_VSO_ELINE,       5,     },
    {P_VENC_VIDEO_PROG_MODE,       0x100, },
    {P_VENC_SYNC_ROUTE,            0,     },
    {P_VENC_INTCTRL,               0x200, },
    {P_ENCP_VFIFO2VD_CTL,               0,     },
    {P_VENC_VDAC_SETTING,          0,     },
    ////////////////////////
    {P_ENCP_VIDEO_RGB_CTRL,		 0,},
    {P_VENC_UPSAMPLE_CTRL0,        0xc061,},
    {P_VENC_UPSAMPLE_CTRL1,        0xd061,},
    {P_VENC_UPSAMPLE_CTRL2,        0xe061,},
    {P_VENC_VDAC_DACSEL0,          0xf003,},
    {P_VENC_VDAC_DACSEL1,          0xf003,},
    {P_VENC_VDAC_DACSEL2,          0xf003,},
    {P_VENC_VDAC_DACSEL3,          0xf003,},
    {P_VENC_VDAC_DACSEL4,          0xf003,},
    {P_VENC_VDAC_DACSEL5,          0xf003,},
    {P_VPU_VIU_VENC_MUX_CTRL,      0x000a,},
    {P_VENC_VDAC_FIFO_CTRL,        0x1fc0,},
    {P_ENCP_DACSEL_0,              0x0543,},
    {P_ENCP_DACSEL_1,              0x0000,},
    {P_ENCI_VIDEO_EN,              0      },
    {P_ENCP_VIDEO_EN,              1      },
    {MREG_END_MARKER,            0      }
	///////////////////////////////////

};

static const reg_t tvregs_3840x1080p120hz[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_EN,              0,     },
    {P_ENCI_VIDEO_EN,              0,     },

    {P_ENCP_VIDEO_MODE, 0x4040,},
    {P_ENCP_VIDEO_MODE_ADV, 0x18,},
    {P_ENCP_VIDEO_MAX_PXCNT, 0x112F,},
    {P_ENCP_VIDEO_MAX_LNCNT, 0x464,},
    {P_ENCP_VIDEO_HAVON_BEGIN, 0x180,},
    {P_ENCP_VIDEO_HAVON_END, 0x107F,},
    {P_ENCP_VIDEO_VAVON_BLINE, 0x29,},
    {P_ENCP_VIDEO_VAVON_ELINE, 0x460,},
    {P_ENCP_VIDEO_HSO_BEGIN, 0x0,},
    {P_ENCP_VIDEO_HSO_END, 0x58,},
    {P_ENCP_VIDEO_VSO_BEGIN, 0x1E,},
    {P_ENCP_VIDEO_VSO_END, 0x32,},
    {P_ENCP_VIDEO_VSO_BLINE, 0x0,},
    {P_ENCP_VIDEO_VSO_ELINE, 0x5,},

    {P_ENCP_VIDEO_EN,              1,     },
    {P_ENCI_VIDEO_EN,              0,     },
    {MREG_END_MARKER,            0      }
};

static const reg_t tvregs_3840x1080p100hz[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_EN,              0,     },
    {P_ENCI_VIDEO_EN,              0,     },

    {P_ENCP_VIDEO_MODE, 0x4040,},
    {P_ENCP_VIDEO_MODE_ADV, 0x18,},
    {P_ENCP_VIDEO_MAX_PXCNT, 0x149F,},
    {P_ENCP_VIDEO_MAX_LNCNT, 0x464,},
    {P_ENCP_VIDEO_HAVON_BEGIN, 0x180,},
    {P_ENCP_VIDEO_HAVON_END, 0x107F,},
    {P_ENCP_VIDEO_VAVON_BLINE, 0x29,},
    {P_ENCP_VIDEO_VAVON_ELINE, 0x460,},
    {P_ENCP_VIDEO_HSO_BEGIN, 0x0,},
    {P_ENCP_VIDEO_HSO_END, 0x58,},
    {P_ENCP_VIDEO_VSO_BEGIN, 0x1E,},
    {P_ENCP_VIDEO_VSO_END, 0x32,},
    {P_ENCP_VIDEO_VSO_BLINE, 0x0,},
    {P_ENCP_VIDEO_VSO_ELINE, 0x5,},

    {P_ENCP_VIDEO_EN,              1,     },
    {P_ENCI_VIDEO_EN,              0,     },
    {MREG_END_MARKER,            0      }
};

static const reg_t tvregs_3840x540p240hz[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_EN,              0,     },
    {P_ENCI_VIDEO_EN,              0,     },

    {P_ENCP_VIDEO_MODE, 0x4040,},
    {P_ENCP_VIDEO_MODE_ADV, 0x18,},
    {P_ENCP_VIDEO_MAX_PXCNT, 0x112F,},
    {P_ENCP_VIDEO_MAX_LNCNT, 0x231,},
    {P_ENCP_VIDEO_HAVON_BEGIN, 0x180,},
    {P_ENCP_VIDEO_HAVON_END, 0x107F,},
    {P_ENCP_VIDEO_VAVON_BLINE, 0x14,},
    {P_ENCP_VIDEO_VAVON_ELINE, 0x22F,},
    {P_ENCP_VIDEO_HSO_BEGIN, 0x0,},
    {P_ENCP_VIDEO_HSO_END, 0x58,},
    {P_ENCP_VIDEO_VSO_BEGIN, 0x1E,},
    {P_ENCP_VIDEO_VSO_END, 0x32,},
    {P_ENCP_VIDEO_VSO_BLINE, 0x0,},
    {P_ENCP_VIDEO_VSO_ELINE, 0x2,},

    {P_ENCP_VIDEO_EN,              1,     },
    {P_ENCI_VIDEO_EN,              0,     },
    {MREG_END_MARKER,            0      }
};

static const reg_t tvregs_3840x540p200hz[] = {
    {P_VENC_VDAC_SETTING,          0xff,  },

    {P_ENCP_VIDEO_EN,              0,     },
    {P_ENCI_VIDEO_EN,              0,     },

    {P_ENCP_VIDEO_MODE, 0x4040,},
    {P_ENCP_VIDEO_MODE_ADV, 0x18,},
    {P_ENCP_VIDEO_MAX_PXCNT, 0x149F,},
    {P_ENCP_VIDEO_MAX_LNCNT, 0x231,},
    {P_ENCP_VIDEO_HAVON_BEGIN, 0x180,},
    {P_ENCP_VIDEO_HAVON_END, 0x107F,},
    {P_ENCP_VIDEO_VAVON_BLINE, 0x14,},
    {P_ENCP_VIDEO_VAVON_ELINE, 0x22F,},
    {P_ENCP_VIDEO_HSO_BEGIN, 0x0,},
    {P_ENCP_VIDEO_HSO_END, 0x58,},
    {P_ENCP_VIDEO_VSO_BEGIN, 0x1E,},
    {P_ENCP_VIDEO_VSO_END, 0x32,},
    {P_ENCP_VIDEO_VSO_BLINE, 0x0,},
    {P_ENCP_VIDEO_VSO_ELINE, 0x2,},

    {P_ENCP_VIDEO_EN,              1,     },
    {P_ENCI_VIDEO_EN,              0,     },
    {MREG_END_MARKER,            0      }
};

// Using tvmode as index
static struct tvregs_set_t tvregsTab[] = {
    {TVMODE_480I, tvregs_480i,        },
    {TVMODE_480I_RPT, tvregs_480i,        },// For REPEAT MODE use, ENC setting is same
    {TVMODE_480CVBS, tvregs_480cvbs,     },
    {TVMODE_480P, tvregs_480p,        },
    {TVMODE_480P_RPT, tvregs_480p,        },// For REPEAT MODE use, ENC setting is same
    {TVMODE_576I, tvregs_576i,        },
    {TVMODE_576I_RPT, tvregs_576i,        },// For REPEAT MODE use, ENC setting is same
    {TVMODE_576CVBS, tvregs_576cvbs,     },
    {TVMODE_576P, tvregs_576p,        },
    {TVMODE_576P_RPT, tvregs_576p,        },// For REPEAT MODE use, ENC setting is same
    {TVMODE_720P, tvregs_720p,        },
    {TVMODE_1080I, tvregs_1080i,       },//Adjust tvregs_* sequences and match the enum define in tvmode.h
    {TVMODE_1080P, tvregs_1080p,       },
    {TVMODE_720P_50HZ, tvregs_720p_50hz,   },
    {TVMODE_1080I_50HZ, tvregs_1080i_50hz,  },
    {TVMODE_1080P_50HZ, tvregs_1080p_50hz,  },
    {TVMODE_1080P_24HZ, tvregs_1080p_24hz,  },
    {TVMODE_4K2K_30HZ, tvregs_4k2k_30hz,   },
    {TVMODE_4K2K_25HZ, tvregs_4k2k_25hz,   },
    {TVMODE_4K2K_24HZ, tvregs_4k2k_24hz,   },
    {TVMODE_4K2K_SMPTE, tvregs_4k2k_smpte,  },
    {TVMODE_4K2K_FAKE_5G, tvregs_4k2k_30hz,   },      // FAKE 4k2k5g
    {TVMODE_4K2K_60HZ, tvregs_4k2k_30hz,   },      // 4k2k60hz
    {TVMODE_4K2K_60HZ_Y420, tvregs_4k2k_30hz,   },      // 4k2k60hz YCbCr420 mode
    {TVMODE_4K2K_50HZ_Y420, tvregs_4k2k_25hz, },
    {TVMODE_4K1K_120HZ, tvregs_3840x1080p120hz},
    {TVMODE_4K1K_120HZ_Y420, tvregs_3840x1080p120hz},
    {TVMODE_4K1K_100HZ, tvregs_3840x1080p100hz},
    {TVMODE_4K1K_100HZ_Y420, tvregs_3840x1080p100hz},
    {TVMODE_4K05K_240HZ, tvregs_3840x540p240hz},
    {TVMODE_4K05K_240HZ_Y420, tvregs_3840x540p240hz},
    {TVMODE_4K05K_200HZ, tvregs_3840x540p200hz},
    {TVMODE_4K05K_200HZ_Y420, tvregs_3840x540p200hz},
    {TVMODE_VGA, tvregs_vga_640x480, },
    {TVMODE_SVGA, tvregs_svga_800x600,},
    {TVMODE_XGA, tvregs_xga_1024x768,},
};

static const tvinfo_t tvinfoTab[] = {
    {.tvmode = TVMODE_480I, .xres =  720, .yres =  480, .id = "480i"},
    {.tvmode = TVMODE_480I_RPT, .xres =  720, .yres =  480, .id = "480i_rpt"},
    {.tvmode = TVMODE_480CVBS, .xres =  720, .yres =  480, .id = "480cvbs"},
    {.tvmode = TVMODE_480P, .xres =  720, .yres =  480, .id = "480p"},
    {.tvmode = TVMODE_480P_RPT, .xres =  720, .yres =  480, .id = "480p_rpt"},
    {.tvmode = TVMODE_576I, .xres =  720, .yres =  576, .id = "576i"},
    {.tvmode = TVMODE_576I_RPT, .xres =  720, .yres =  576, .id = "576i_rpt"},
    {.tvmode = TVMODE_576CVBS, .xres =  720, .yres =  576, .id = "576cvbs"},
    {.tvmode = TVMODE_576P, .xres =  720, .yres =  576, .id = "576p"},
    {.tvmode = TVMODE_576P_RPT, .xres =  720, .yres =  576, .id = "576p_prt"},
    {.tvmode = TVMODE_720P, .xres = 1280, .yres =  720, .id = "720p"},
    {.tvmode = TVMODE_1080I, .xres = 1920, .yres = 1080, .id = "1080i"},
    {.tvmode = TVMODE_1080P, .xres = 1920, .yres = 1080, .id = "1080p"},
    {.tvmode = TVMODE_720P_50HZ, .xres = 1280, .yres =  720, .id = "720p50hz"},
    {.tvmode = TVMODE_1080I_50HZ, .xres = 1920, .yres = 1080, .id = "1080i50hz"},
    {.tvmode = TVMODE_1080P_50HZ, .xres = 1920, .yres = 1080, .id = "1080p50hz"},
    {.tvmode = TVMODE_1080P_24HZ, .xres = 1920, .yres = 1080, .id = "1080p24hz"},
    {.tvmode = TVMODE_4K2K_30HZ, .xres = 3840, .yres = 2160, .id = "4k2k30hz"},
    {.tvmode = TVMODE_4K2K_25HZ, .xres = 3840, .yres = 2160, .id = "4k2k25hz"},
    {.tvmode = TVMODE_4K2K_24HZ, .xres = 3840, .yres = 2160, .id = "4k2k24hz"},
    {.tvmode = TVMODE_4K2K_SMPTE, .xres = 4096, .yres = 2160, .id = "4k2ksmpte"},
    {.tvmode = TVMODE_4K2K_FAKE_5G, .xres = 4096, .yres = 2160, .id = "4k2k5g"},
    {.tvmode = TVMODE_4K2K_60HZ_Y420, .xres = 3840, .yres = 2160, .id = "4k2k60hz420"},
    {.tvmode = TVMODE_4K2K_50HZ_Y420, .xres = 3840, .yres = 2160, .id = "4k2k50hz420"},
    {.tvmode = TVMODE_4K2K_60HZ, .xres = 3840, .yres = 2160, .id = "4k2k60hz"},
    {.tvmode = TVMODE_4K1K_100HZ, .xres = 3840, .yres = 1080, .id = "4k1k100hz"},
    {.tvmode = TVMODE_4K1K_100HZ_Y420, .xres = 3840, .yres = 1080, .id = "4k1k100hz420"},
    {.tvmode = TVMODE_4K1K_120HZ, .xres = 3840, .yres = 1080, .id = "4k1k120hz"},
    {.tvmode = TVMODE_4K1K_120HZ_Y420, .xres = 3840, .yres = 1080, .id = "4k1k120hz420"},
    {.tvmode = TVMODE_4K05K_200HZ, .xres = 3840, .yres = 540, .id = "4k05k200hz"},
    {.tvmode = TVMODE_4K05K_200HZ_Y420, .xres = 3840, .yres = 540, .id = "4k05k200hz420"},
    {.tvmode = TVMODE_4K05K_240HZ, .xres = 3840, .yres = 540, .id = "4k05k240hz"},
    {.tvmode = TVMODE_4K05K_240HZ_Y420, .xres = 3840, .yres = 540, .id = "4k05k240hz420"},
    {.tvmode = TVMODE_VGA, .xres = 640, .yres = 480, .id = "vga"},
    {.tvmode = TVMODE_SVGA, .xres = 800, .yres = 600, .id = "svga"},
    {.tvmode = TVMODE_XGA, .xres = 1024, .yres = 768, .id = "xga"},
};

static inline void setreg(const reg_t *r)
{
	aml_write_reg32(r->reg, r->val);
	//printk("[0x%x] = 0x%x\n", r->reg, r->val);
}

#endif /* TVREGS_H */


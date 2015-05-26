
#ifndef MACH_LCDOUTC_H
#define MACH_LCDOUTC_H

#define CONFIG_LCD_TYPE_MID_VALID
#define CONFIG_LCD_IF_TTL_VALID
#define CONFIG_LCD_IF_LVDS_VALID
#define CONFIG_LCD_IF_MIPI_VALID
/*
// lcd driver global API, special by CPU
*/
//*************************************************************
// For mipi-dsi external driver use
//*************************************************************
//mipi command(payload)
//format:  data_type, num, data....
//special: data_type=0xff, num<0xff means delay ms, num=0xff means ending.

// ----------------------------------------------------------------------------
//                           Function: dsi_write_cmd
// Supported Data Type: DT_GEN_SHORT_WR_0, DT_GEN_SHORT_WR_1, DT_GEN_SHORT_WR_2,
//                      DT_DCS_SHORT_WR_0, DT_DCS_SHORT_WR_1,
//                      DT_GEN_LONG_WR, DT_DCS_LONG_WR,
//                      DT_SET_MAX_RET_PKT_SIZE
//                      DT_GEN_RD_0, DT_GEN_RD_1, DT_GEN_RD_2,
//                      DT_DCS_RD_0
// Return:              command number
// ----------------------------------------------------------------------------
extern int dsi_write_cmd(unsigned char* payload);

//*************************************************************

#endif

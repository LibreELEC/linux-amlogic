#ifndef  __HDMI_TX_REG_H_
#define  __HDMI_TX_REG_H_

// Use the following functions to access the on-chip HDMITX modules by default
extern void hdmitx_wr_reg(unsigned int addr, unsigned int data);
extern void hdmitx_poll_reg(unsigned int addr, unsigned int val, unsigned long timeout);
extern void hdmitx_set_reg_bits(unsigned int addr, unsigned int value, unsigned int offset, unsigned int len);
extern unsigned int hdmitx_rd_reg(unsigned int addr);
extern void hdmitx_rd_check_reg(unsigned int addr, unsigned int exp_data, unsigned int mask);
extern unsigned long aocec_rd_reg (unsigned long addr);
extern void aocec_wr_reg (unsigned long addr, unsigned long data);

//------------------------------------------------------------------------------
// TOP-level wrapper registers addresses
//------------------------------------------------------------------------------
#define TOP_OFFSET_MASK      (0x00 << 24)
#define DWC_OFFSET_MASK      (0x10 << 24)

// Bit     7 RW Reserved.                                                                                        Default 1.
// Bit     6 RW Reserved.                                                                                        Default 1.
// Bit     5 RW Reserved.                                                                                        Default 1.
// Bit     4 RW sw_reset_phyif: PHY interface.                              1=Apply reset; 0=Release from reset. Default 1.
// Bit     3 RW sw_reset_intr:  interrupt module.                           1=Apply reset; 0=Release from reset. Default 1.
// Bit     2 RW sw_reset_mem:   KSV/REVOC mem.                              1=Apply reset; 0=Release from reset. Default 1.
// Bit     1 RW sw_reset_rnd:   random number interface to HDCP.            1=Apply reset; 0=Release from reset. Default 1.
// Bit     0 RW sw_reset_core:  connects to IP's ~irstz.                    1=Apply reset; 0=Release from reset. Default 1.
#define HDMITX_TOP_SW_RESET                     (TOP_OFFSET_MASK + 0x000)

// Bit    12 RW i2s_ws_inv:     1=Invert i2s_ws;    0=No invert.    Default 0.
// Bit    11 RW i2s_clk_inv:    1=Invert i2s_clk;   0=No invert.    Default 0.
// Bit    10 RW spdif_clk_inv:  1=Invert spdif_clk; 0=No invert.    Default 0.
// Bit     9 RW tmds_clk_inv:   1=Invert tmds_clk;  0=No invert.    Default 0.
// Bit     8 RW pixel_clk_inv:  1=Invert pixel_clk; 0=No invert.    Default 0.
// Bit     4 RW cec_clk_en:     1=enable cec_clk;   0=disable.      Default 0.
// Bit     3 RW i2s_clk_en:     1=enable i2s_clk;   0=disable.      Default 0.
// Bit     2 RW spdif_clk_en:   1=enable spdif_clk; 0=disable.      Default 0.
// Bit     1 RW tmds_clk_en:    1=enable tmds_clk;  0=disable.      Default 0.
// Bit     0 RW pixel_clk_en:   1=enable pixel_clk; 0=disable.      Default 0.
#define HDMITX_TOP_CLK_CNTL                     (TOP_OFFSET_MASK + 0x001)

// Bit 11: 0 RW hpd_valid_width: filter out width <= M*1024.    Default 0.
// Bit 15:12 RW hpd_glitch_width: filter out glitch <= N.       Default 0.
#define HDMITX_TOP_HPD_FILTER                   (TOP_OFFSET_MASK + 0x002)

// intr_maskn: MASK_N, one bit per interrupt source. 1=Enable interrupt source; 0=Disable interrupt source. Default 0.
// Bit     2 RW hpd_fall
// Bit     1 RW hpd_rise
// Bit     0 RW IP interrupt
#define HDMITX_TOP_INTR_MASKN                   (TOP_OFFSET_MASK + 0x003)

// Bit 30: 0 RW intr_stat: For each bit, write 1 to manually set the interrupt bit, read back the interrupt status.
// Bit    31 R  IP interrupt status
// Bit     2 RW hpd_fall
// Bit     1 RW hpd_rise
// Bit     0 RW IP interrupt
#define HDMITX_TOP_INTR_STAT                    (TOP_OFFSET_MASK + 0x004)

// Bit  2: 0  W intr_stat_clr: For each bit, write 1 to clear the interrupt bit.
// Bit     2 RW hpd_fall
// Bit     1 RW hpd_rise
// Bit     0 RW IP interrupt
#define HDMITX_TOP_INTR_STAT_CLR                (TOP_OFFSET_MASK + 0x005)

// Bit 14:12 RW tmds_sel: 3'b000=Output zero; 3'b001=Output normal TMDS data; 3'b010=Output PRBS data; 3'b100=Output shift pattern.         Default 0.
// Bit 11: 9 RW shift_pttn_repeat: 0=New pattern every clk cycle; 1=New pattern every 2 clk cycles; ...; 7=New pattern every 8 clk cycles.  Default 0.
// Bit     8 RW shift_pttn_en: 1= Eanble shift pattern generator; 0=Disable.                                                                Default 0.
// Bit  4: 3 RW prbs_pttn_mode: 0=PRBS11; 1=PRBS15; 2=PRBS7; 3=PRBS31.                                                                      Default 0.
// Bit  2: 1 RW prbs_pttn_width: 0=idle; 1=output 8-bit pattern; 2=Output 1-bit pattern; 3=output 10-bit pattern.                           Default 0.
// Bit     0 RW prbs_pttn_en: 1=Enable PRBS generator; 0=Disable.                                                                           Default 0.
#define HDMITX_TOP_BIST_CNTL                    (TOP_OFFSET_MASK + 0x006)

// Bit 29:20 RW shift_pttn_data[59:50]. Default 0.
// Bit 19:10 RW shift_pttn_data[69:60]. Default 0.
// Bit  9: 0 RW shift_pttn_data[79:70]. Default 0.
#define HDMITX_TOP_SHIFT_PTTN_012               (TOP_OFFSET_MASK + 0x007)

// Bit 29:20 RW shift_pttn_data[29:20]. Default 0.
// Bit 19:10 RW shift_pttn_data[39:30]. Default 0.
// Bit  9: 0 RW shift_pttn_data[49:40]. Default 0.
#define HDMITX_TOP_SHIFT_PTTN_345               (TOP_OFFSET_MASK + 0x008)

// Bit 19:10 RW shift_pttn_data[ 9: 0]. Default 0.
// Bit  9: 0 RW shift_pttn_data[19:10]. Default 0.
#define HDMITX_TOP_SHIFT_PTTN_67                (TOP_OFFSET_MASK + 0x009)

// Bit 25:16 RW tmds_clk_pttn[19:10]. Default 0.
// Bit  9: 0 RW tmds_clk_pttn[ 9: 0]. Default 0.
#define HDMITX_TOP_TMDS_CLK_PTTN_01             (TOP_OFFSET_MASK + 0x00A)

// Bit 25:16 RW tmds_clk_pttn[39:30]. Default 0.
// Bit  9: 0 RW tmds_clk_pttn[29:20]. Default 0.
#define HDMITX_TOP_TMDS_CLK_PTTN_23             (TOP_OFFSET_MASK + 0x00B)

// Bit     1 RW shift_tmds_clk_pttn:1=Enable shifting clk pattern, used when TMDS CLK rate = TMDS character rate /4.    Default 0.
// Bit     0 R  Reserved.                                                                                               Default 0.
#define HDMITX_TOP_TMDS_CLK_PTTN_CNTL           (TOP_OFFSET_MASK + 0x00C)

// Bit     0 RW revocmem_wr_fail: Read back 1 to indicate Host write REVOC MEM failure, write 1 to clear the failure flag.  Default 0.
#define HDMITX_TOP_REVOCMEM_STAT                (TOP_OFFSET_MASK + 0x00D)

// Bit     0 R  filtered HPD status.
#define HDMITX_TOP_STAT0                        (TOP_OFFSET_MASK + 0x00E)

#define HDMITX_TOP_DONT_TOUCH0                  (TOP_OFFSET_MASK + 0x0FE)
#define HDMITX_TOP_DONT_TOUCH1                  (TOP_OFFSET_MASK + 0x0FF)

//------------------------------------------------------------------------------
// DWC_HDMI_TX Controller registers addresses
//------------------------------------------------------------------------------

// Identification Registers
#define HDMITX_DWC_DESIGN_ID                    (DWC_OFFSET_MASK + 0x0000)
#define HDMITX_DWC_REVISION_ID                  (DWC_OFFSET_MASK + 0x0001)
#define HDMITX_DWC_PRODUCT_ID0                  (DWC_OFFSET_MASK + 0x0002)
#define HDMITX_DWC_PRODUCT_ID1                  (DWC_OFFSET_MASK + 0x0003)
#define HDMITX_DWC_CONFIG0_ID                   (DWC_OFFSET_MASK + 0x0004)
#define HDMITX_DWC_CONFIG1_ID                   (DWC_OFFSET_MASK + 0x0005)
#define HDMITX_DWC_CONFIG2_ID                   (DWC_OFFSET_MASK + 0x0006)
#define HDMITX_DWC_CONFIG3_ID                   (DWC_OFFSET_MASK + 0x0007)

// Interrupt Registers
#define HDMITX_DWC_IH_FC_STAT0                  (DWC_OFFSET_MASK + 0x0100)
#define HDMITX_DWC_IH_FC_STAT1                  (DWC_OFFSET_MASK + 0x0101)
#define HDMITX_DWC_IH_FC_STAT2                  (DWC_OFFSET_MASK + 0x0102)
#define HDMITX_DWC_IH_AS_STAT0                  (DWC_OFFSET_MASK + 0x0103)
#define HDMITX_DWC_IH_PHY_STAT0                 (DWC_OFFSET_MASK + 0x0104)
#define HDMITX_DWC_IH_I2CM_STAT0                (DWC_OFFSET_MASK + 0x0105)
#define HDMITX_DWC_IH_CEC_STAT0                 (DWC_OFFSET_MASK + 0x0106)
#define HDMITX_DWC_IH_VP_STAT0                  (DWC_OFFSET_MASK + 0x0107)
#define HDMITX_DWC_IH_I2CMPHY_STAT0             (DWC_OFFSET_MASK + 0x0108)
#define HDMITX_DWC_IH_DECODE                    (DWC_OFFSET_MASK + 0x0170)
#define HDMITX_DWC_IH_MUTE_FC_STAT0             (DWC_OFFSET_MASK + 0x0180)
#define HDMITX_DWC_IH_MUTE_FC_STAT1             (DWC_OFFSET_MASK + 0x0181)
#define HDMITX_DWC_IH_MUTE_FC_STAT2             (DWC_OFFSET_MASK + 0x0182)
#define HDMITX_DWC_IH_MUTE_AS_STAT0             (DWC_OFFSET_MASK + 0x0183)
#define HDMITX_DWC_IH_MUTE_PHY_STAT0            (DWC_OFFSET_MASK + 0x0184)
#define HDMITX_DWC_IH_MUTE_I2CM_STAT0           (DWC_OFFSET_MASK + 0x0185)
#define HDMITX_DWC_IH_MUTE_CEC_STAT0            (DWC_OFFSET_MASK + 0x0186)
#define HDMITX_DWC_IH_MUTE_VP_STAT0             (DWC_OFFSET_MASK + 0x0187)
#define HDMITX_DWC_IH_MUTE_I2CMPHY_STAT0        (DWC_OFFSET_MASK + 0x0188)
#define HDMITX_DWC_IH_MUTE                      (DWC_OFFSET_MASK + 0x01FF)

// Video Sampler Registers
#define HDMITX_DWC_TX_INVID0                    (DWC_OFFSET_MASK + 0x0200)
#define HDMITX_DWC_TX_INSTUFFING                (DWC_OFFSET_MASK + 0x0201)
#define HDMITX_DWC_TX_GYDATA0                   (DWC_OFFSET_MASK + 0x0202)
#define HDMITX_DWC_TX_GYDATA1                   (DWC_OFFSET_MASK + 0x0203)
#define HDMITX_DWC_TX_RCRDATA0                  (DWC_OFFSET_MASK + 0x0204)
#define HDMITX_DWC_TX_RCRDATA1                  (DWC_OFFSET_MASK + 0x0205)
#define HDMITX_DWC_TX_BCBDATA0                  (DWC_OFFSET_MASK + 0x0206)
#define HDMITX_DWC_TX_BCBDATA1                  (DWC_OFFSET_MASK + 0x0207)

// Video Packetizer Registers
#define HDMITX_DWC_VP_STATUS                    (DWC_OFFSET_MASK + 0x0800)
#define HDMITX_DWC_VP_PR_CD                     (DWC_OFFSET_MASK + 0x0801)
#define HDMITX_DWC_VP_STUFF                     (DWC_OFFSET_MASK + 0x0802)
#define HDMITX_DWC_VP_REMAP                     (DWC_OFFSET_MASK + 0x0803)
#define HDMITX_DWC_VP_CONF                      (DWC_OFFSET_MASK + 0x0804)
#define HDMITX_DWC_VP_MASK                      (DWC_OFFSET_MASK + 0x0807)

// Frmae Composer Registers
#define HDMITX_DWC_FC_INVIDCONF                 (DWC_OFFSET_MASK + 0x1000)
#define HDMITX_DWC_FC_INHACTV0                  (DWC_OFFSET_MASK + 0x1001)
#define HDMITX_DWC_FC_INHACTV1                  (DWC_OFFSET_MASK + 0x1002)
#define HDMITX_DWC_FC_INHBLANK0                 (DWC_OFFSET_MASK + 0x1003)
#define HDMITX_DWC_FC_INHBLANK1                 (DWC_OFFSET_MASK + 0x1004)
#define HDMITX_DWC_FC_INVACTV0                  (DWC_OFFSET_MASK + 0x1005)
#define HDMITX_DWC_FC_INVACTV1                  (DWC_OFFSET_MASK + 0x1006)
#define HDMITX_DWC_FC_INVBLANK                  (DWC_OFFSET_MASK + 0x1007)
#define HDMITX_DWC_FC_HSYNCINDELAY0             (DWC_OFFSET_MASK + 0x1008)
#define HDMITX_DWC_FC_HSYNCINDELAY1             (DWC_OFFSET_MASK + 0x1009)
#define HDMITX_DWC_FC_HSYNCINWIDTH0             (DWC_OFFSET_MASK + 0x100A)
#define HDMITX_DWC_FC_HSYNCINWIDTH1             (DWC_OFFSET_MASK + 0x100B)
#define HDMITX_DWC_FC_VSYNCINDELAY              (DWC_OFFSET_MASK + 0x100C)
#define HDMITX_DWC_FC_VSYNCINWIDTH              (DWC_OFFSET_MASK + 0x100D)
#define HDMITX_DWC_FC_INFREQ0                   (DWC_OFFSET_MASK + 0x100E)
#define HDMITX_DWC_FC_INFREQ1                   (DWC_OFFSET_MASK + 0x100F)
#define HDMITX_DWC_FC_INFREQ2                   (DWC_OFFSET_MASK + 0x1010)
#define HDMITX_DWC_FC_CTRLDUR                   (DWC_OFFSET_MASK + 0x1011)
#define HDMITX_DWC_FC_EXCTRLDUR                 (DWC_OFFSET_MASK + 0x1012)
#define HDMITX_DWC_FC_EXCTRLSPAC                (DWC_OFFSET_MASK + 0x1013)
#define HDMITX_DWC_FC_CH0PREAM                  (DWC_OFFSET_MASK + 0x1014)
#define HDMITX_DWC_FC_CH1PREAM                  (DWC_OFFSET_MASK + 0x1015)
#define HDMITX_DWC_FC_CH2PREAM                  (DWC_OFFSET_MASK + 0x1016)
#define HDMITX_DWC_FC_AVICONF3                  (DWC_OFFSET_MASK + 0x1017)
#define HDMITX_DWC_FC_GCP                       (DWC_OFFSET_MASK + 0x1018)
#define HDMITX_DWC_FC_AVICONF0                  (DWC_OFFSET_MASK + 0x1019)
#define HDMITX_DWC_FC_AVICONF1                  (DWC_OFFSET_MASK + 0x101A)
#define HDMITX_DWC_FC_AVICONF2                  (DWC_OFFSET_MASK + 0x101B)
#define HDMITX_DWC_FC_AVIVID                    (DWC_OFFSET_MASK + 0x101C)
#define HDMITX_DWC_FC_AVIETB0                   (DWC_OFFSET_MASK + 0x101D)
#define HDMITX_DWC_FC_AVIETB1                   (DWC_OFFSET_MASK + 0x101E)
#define HDMITX_DWC_FC_AVISBB0                   (DWC_OFFSET_MASK + 0x101F)
#define HDMITX_DWC_FC_AVISBB1                   (DWC_OFFSET_MASK + 0x1020)
#define HDMITX_DWC_FC_AVIELB0                   (DWC_OFFSET_MASK + 0x1021)
#define HDMITX_DWC_FC_AVIELB1                   (DWC_OFFSET_MASK + 0x1022)
#define HDMITX_DWC_FC_AVISRB0                   (DWC_OFFSET_MASK + 0x1023)
#define HDMITX_DWC_FC_AVISRB1                   (DWC_OFFSET_MASK + 0x1024)
#define HDMITX_DWC_FC_AUDICONF0                 (DWC_OFFSET_MASK + 0x1025)
#define HDMITX_DWC_FC_AUDICONF1                 (DWC_OFFSET_MASK + 0x1026)
#define HDMITX_DWC_FC_AUDICONF2                 (DWC_OFFSET_MASK + 0x1027)
#define HDMITX_DWC_FC_AUDICONF3                 (DWC_OFFSET_MASK + 0x1028)
#define HDMITX_DWC_FC_VSDIEEEID0                (DWC_OFFSET_MASK + 0x1029)
#define HDMITX_DWC_FC_VSDSIZE                   (DWC_OFFSET_MASK + 0x102A)
#define HDMITX_DWC_FC_VSDIEEEID1                (DWC_OFFSET_MASK + 0x1030)
#define HDMITX_DWC_FC_VSDIEEEID2                (DWC_OFFSET_MASK + 0x1031)
#define HDMITX_DWC_FC_VSDPAYLOAD0               (DWC_OFFSET_MASK + 0x1032)
#define HDMITX_DWC_FC_VSDPAYLOAD1               (DWC_OFFSET_MASK + 0x1033)
#define HDMITX_DWC_FC_VSDPAYLOAD2               (DWC_OFFSET_MASK + 0x1034)
#define HDMITX_DWC_FC_VSDPAYLOAD3               (DWC_OFFSET_MASK + 0x1035)
#define HDMITX_DWC_FC_VSDPAYLOAD4               (DWC_OFFSET_MASK + 0x1036)
#define HDMITX_DWC_FC_VSDPAYLOAD5               (DWC_OFFSET_MASK + 0x1037)
#define HDMITX_DWC_FC_VSDPAYLOAD6               (DWC_OFFSET_MASK + 0x1038)
#define HDMITX_DWC_FC_VSDPAYLOAD7               (DWC_OFFSET_MASK + 0x1039)
#define HDMITX_DWC_FC_VSDPAYLOAD8               (DWC_OFFSET_MASK + 0x103A)
#define HDMITX_DWC_FC_VSDPAYLOAD9               (DWC_OFFSET_MASK + 0x103B)
#define HDMITX_DWC_FC_VSDPAYLOAD10              (DWC_OFFSET_MASK + 0x103C)
#define HDMITX_DWC_FC_VSDPAYLOAD11              (DWC_OFFSET_MASK + 0x103D)
#define HDMITX_DWC_FC_VSDPAYLOAD12              (DWC_OFFSET_MASK + 0x103E)
#define HDMITX_DWC_FC_VSDPAYLOAD13              (DWC_OFFSET_MASK + 0x103F)
#define HDMITX_DWC_FC_VSDPAYLOAD14              (DWC_OFFSET_MASK + 0x1040)
#define HDMITX_DWC_FC_VSDPAYLOAD15              (DWC_OFFSET_MASK + 0x1041)
#define HDMITX_DWC_FC_VSDPAYLOAD16              (DWC_OFFSET_MASK + 0x1042)
#define HDMITX_DWC_FC_VSDPAYLOAD17              (DWC_OFFSET_MASK + 0x1043)
#define HDMITX_DWC_FC_VSDPAYLOAD18              (DWC_OFFSET_MASK + 0x1044)
#define HDMITX_DWC_FC_VSDPAYLOAD19              (DWC_OFFSET_MASK + 0x1045)
#define HDMITX_DWC_FC_VSDPAYLOAD20              (DWC_OFFSET_MASK + 0x1046)
#define HDMITX_DWC_FC_VSDPAYLOAD21              (DWC_OFFSET_MASK + 0x1047)
#define HDMITX_DWC_FC_VSDPAYLOAD22              (DWC_OFFSET_MASK + 0x1048)
#define HDMITX_DWC_FC_VSDPAYLOAD23              (DWC_OFFSET_MASK + 0x1049)
#define HDMITX_DWC_FC_SPDVENDORNAME0            (DWC_OFFSET_MASK + 0x104A)
#define HDMITX_DWC_FC_SPDVENDORNAME1            (DWC_OFFSET_MASK + 0x104B)
#define HDMITX_DWC_FC_SPDVENDORNAME2            (DWC_OFFSET_MASK + 0x104C)
#define HDMITX_DWC_FC_SPDVENDORNAME3            (DWC_OFFSET_MASK + 0x104D)
#define HDMITX_DWC_FC_SPDVENDORNAME4            (DWC_OFFSET_MASK + 0x104E)
#define HDMITX_DWC_FC_SPDVENDORNAME5            (DWC_OFFSET_MASK + 0x104F)
#define HDMITX_DWC_FC_SPDVENDORNAME6            (DWC_OFFSET_MASK + 0x1050)
#define HDMITX_DWC_FC_SPDVENDORNAME7            (DWC_OFFSET_MASK + 0x1051)
#define HDMITX_DWC_FC_SDPPRODUCTNAME0           (DWC_OFFSET_MASK + 0x1052)
#define HDMITX_DWC_FC_SDPPRODUCTNAME1           (DWC_OFFSET_MASK + 0x1053)
#define HDMITX_DWC_FC_SDPPRODUCTNAME2           (DWC_OFFSET_MASK + 0x1054)
#define HDMITX_DWC_FC_SDPPRODUCTNAME3           (DWC_OFFSET_MASK + 0x1055)
#define HDMITX_DWC_FC_SDPPRODUCTNAME4           (DWC_OFFSET_MASK + 0x1056)
#define HDMITX_DWC_FC_SDPPRODUCTNAME5           (DWC_OFFSET_MASK + 0x1057)
#define HDMITX_DWC_FC_SDPPRODUCTNAME6           (DWC_OFFSET_MASK + 0x1058)
#define HDMITX_DWC_FC_SDPPRODUCTNAME7           (DWC_OFFSET_MASK + 0x1059)
#define HDMITX_DWC_FC_SDPPRODUCTNAME8           (DWC_OFFSET_MASK + 0x105A)
#define HDMITX_DWC_FC_SDPPRODUCTNAME9           (DWC_OFFSET_MASK + 0x105B)
#define HDMITX_DWC_FC_SDPPRODUCTNAME10          (DWC_OFFSET_MASK + 0x105C)
#define HDMITX_DWC_FC_SDPPRODUCTNAME11          (DWC_OFFSET_MASK + 0x105D)
#define HDMITX_DWC_FC_SDPPRODUCTNAME12          (DWC_OFFSET_MASK + 0x105E)
#define HDMITX_DWC_FC_SDPPRODUCTNAME13          (DWC_OFFSET_MASK + 0x105F)
#define HDMITX_DWC_FC_SDPPRODUCTNAME14          (DWC_OFFSET_MASK + 0x1060)
#define HDMITX_DWC_FC_SPDPRODUCTNAME15          (DWC_OFFSET_MASK + 0x1061)
#define HDMITX_DWC_FC_SPDDEVICEINF              (DWC_OFFSET_MASK + 0x1062)
#define HDMITX_DWC_FC_AUDSCONF                  (DWC_OFFSET_MASK + 0x1063)
#define HDMITX_DWC_FC_AUDSSTAT                  (DWC_OFFSET_MASK + 0x1064)
#define HDMITX_DWC_FC_AUDSV                     (DWC_OFFSET_MASK + 0x1065)
#define HDMITX_DWC_FC_AUDSU                     (DWC_OFFSET_MASK + 0x1066)
#define HDMITX_DWC_FC_AUDSCHNLS0                (DWC_OFFSET_MASK + 0x1067)
#define HDMITX_DWC_FC_AUDSCHNLS1                (DWC_OFFSET_MASK + 0x1068)
#define HDMITX_DWC_FC_AUDSCHNLS2                (DWC_OFFSET_MASK + 0x1069)
#define HDMITX_DWC_FC_AUDSCHNLS3                (DWC_OFFSET_MASK + 0x106A)
#define HDMITX_DWC_FC_AUDSCHNLS4                (DWC_OFFSET_MASK + 0x106B)
#define HDMITX_DWC_FC_AUDSCHNLS5                (DWC_OFFSET_MASK + 0x106C)
#define HDMITX_DWC_FC_AUDSCHNLS6                (DWC_OFFSET_MASK + 0x106D)
#define HDMITX_DWC_FC_AUDSCHNLS7                (DWC_OFFSET_MASK + 0x106E)
#define HDMITX_DWC_FC_AUDSCHNLS8                (DWC_OFFSET_MASK + 0x106F)
#define HDMITX_DWC_FC_DATACH0FILL               (DWC_OFFSET_MASK + 0x1070)
#define HDMITX_DWC_FC_DATACH1FILL               (DWC_OFFSET_MASK + 0x1071)
#define HDMITX_DWC_FC_DATACH2FILL               (DWC_OFFSET_MASK + 0x1072)
#define HDMITX_DWC_FC_CTRLQHIGH                 (DWC_OFFSET_MASK + 0x1073)
#define HDMITX_DWC_FC_CTRLQLOW                  (DWC_OFFSET_MASK + 0x1074)
#define HDMITX_DWC_FC_ACP0                      (DWC_OFFSET_MASK + 0x1075)
#define HDMITX_DWC_FC_ACP16                     (DWC_OFFSET_MASK + 0x1082)
#define HDMITX_DWC_FC_ACP15                     (DWC_OFFSET_MASK + 0x1083)
#define HDMITX_DWC_FC_ACP14                     (DWC_OFFSET_MASK + 0x1084)
#define HDMITX_DWC_FC_ACP13                     (DWC_OFFSET_MASK + 0x1085)
#define HDMITX_DWC_FC_ACP12                     (DWC_OFFSET_MASK + 0x1086)
#define HDMITX_DWC_FC_ACP11                     (DWC_OFFSET_MASK + 0x1087)
#define HDMITX_DWC_FC_ACP10                     (DWC_OFFSET_MASK + 0x1088)
#define HDMITX_DWC_FC_ACP9                      (DWC_OFFSET_MASK + 0x1089)
#define HDMITX_DWC_FC_ACP8                      (DWC_OFFSET_MASK + 0x108A)
#define HDMITX_DWC_FC_ACP7                      (DWC_OFFSET_MASK + 0x108B)
#define HDMITX_DWC_FC_ACP6                      (DWC_OFFSET_MASK + 0x108C)
#define HDMITX_DWC_FC_ACP5                      (DWC_OFFSET_MASK + 0x108D)
#define HDMITX_DWC_FC_ACP4                      (DWC_OFFSET_MASK + 0x108E)
#define HDMITX_DWC_FC_ACP3                      (DWC_OFFSET_MASK + 0x108F)
#define HDMITX_DWC_FC_ACP2                      (DWC_OFFSET_MASK + 0x1090)
#define HDMITX_DWC_FC_ACP1                      (DWC_OFFSET_MASK + 0x1091)
#define HDMITX_DWC_FC_ISCR1_0                   (DWC_OFFSET_MASK + 0x1092)
#define HDMITX_DWC_FC_ISCR1_16                  (DWC_OFFSET_MASK + 0x1093)
#define HDMITX_DWC_FC_ISCR1_15                  (DWC_OFFSET_MASK + 0x1094)
#define HDMITX_DWC_FC_ISCR1_14                  (DWC_OFFSET_MASK + 0x1095)
#define HDMITX_DWC_FC_ISCR1_13                  (DWC_OFFSET_MASK + 0x1096)
#define HDMITX_DWC_FC_ISCR1_12                  (DWC_OFFSET_MASK + 0x1097)
#define HDMITX_DWC_FC_ISCR1_11                  (DWC_OFFSET_MASK + 0x1098)
#define HDMITX_DWC_FC_ISCR1_10                  (DWC_OFFSET_MASK + 0x1099)
#define HDMITX_DWC_FC_ISCR1_9                   (DWC_OFFSET_MASK + 0x109A)
#define HDMITX_DWC_FC_ISCR1_8                   (DWC_OFFSET_MASK + 0x109B)
#define HDMITX_DWC_FC_ISCR1_7                   (DWC_OFFSET_MASK + 0x109C)
#define HDMITX_DWC_FC_ISCR1_6                   (DWC_OFFSET_MASK + 0x109D)
#define HDMITX_DWC_FC_ISCR1_5                   (DWC_OFFSET_MASK + 0x109E)
#define HDMITX_DWC_FC_ISCR1_4                   (DWC_OFFSET_MASK + 0x109F)
#define HDMITX_DWC_FC_ISCR1_3                   (DWC_OFFSET_MASK + 0x10A0)
#define HDMITX_DWC_FC_ISCR1_2                   (DWC_OFFSET_MASK + 0x10A1)
#define HDMITX_DWC_FC_ISCR1_1                   (DWC_OFFSET_MASK + 0x10A2)
#define HDMITX_DWC_FC_ISCR0_15                  (DWC_OFFSET_MASK + 0x10A3)
#define HDMITX_DWC_FC_ISCR0_14                  (DWC_OFFSET_MASK + 0x10A4)
#define HDMITX_DWC_FC_ISCR0_13                  (DWC_OFFSET_MASK + 0x10A5)
#define HDMITX_DWC_FC_ISCR0_12                  (DWC_OFFSET_MASK + 0x10A6)
#define HDMITX_DWC_FC_ISCR0_11                  (DWC_OFFSET_MASK + 0x10A7)
#define HDMITX_DWC_FC_ISCR0_10                  (DWC_OFFSET_MASK + 0x10A8)
#define HDMITX_DWC_FC_ISCR0_9                   (DWC_OFFSET_MASK + 0x10A9)
#define HDMITX_DWC_FC_ISCR0_8                   (DWC_OFFSET_MASK + 0x10AA)
#define HDMITX_DWC_FC_ISCR0_7                   (DWC_OFFSET_MASK + 0x10AB)
#define HDMITX_DWC_FC_ISCR0_6                   (DWC_OFFSET_MASK + 0x10AC)
#define HDMITX_DWC_FC_ISCR0_5                   (DWC_OFFSET_MASK + 0x10AD)
#define HDMITX_DWC_FC_ISCR0_4                   (DWC_OFFSET_MASK + 0x10AE)
#define HDMITX_DWC_FC_ISCR0_3                   (DWC_OFFSET_MASK + 0x10AF)
#define HDMITX_DWC_FC_ISCR0_2                   (DWC_OFFSET_MASK + 0x10B0)
#define HDMITX_DWC_FC_ISCR0_1                   (DWC_OFFSET_MASK + 0x10B1)
#define HDMITX_DWC_FC_ISCR0_0                   (DWC_OFFSET_MASK + 0x10B2)
#define HDMITX_DWC_FC_DATAUTO0                  (DWC_OFFSET_MASK + 0x10B3)
#define HDMITX_DWC_FC_DATAUTO1                  (DWC_OFFSET_MASK + 0x10B4)
#define HDMITX_DWC_FC_DATAUTO2                  (DWC_OFFSET_MASK + 0x10B5)
#define HDMITX_DWC_FC_DATMAN                    (DWC_OFFSET_MASK + 0x10B6)
#define HDMITX_DWC_FC_DATAUTO3                  (DWC_OFFSET_MASK + 0x10B7)
#define HDMITX_DWC_FC_RDRB0                     (DWC_OFFSET_MASK + 0x10B8)
#define HDMITX_DWC_FC_RDRB1                     (DWC_OFFSET_MASK + 0x10B9)
#define HDMITX_DWC_FC_RDRB2                     (DWC_OFFSET_MASK + 0x10BA)
#define HDMITX_DWC_FC_RDRB3                     (DWC_OFFSET_MASK + 0x10BB)
#define HDMITX_DWC_FC_RDRB4                     (DWC_OFFSET_MASK + 0x10BC)
#define HDMITX_DWC_FC_RDRB5                     (DWC_OFFSET_MASK + 0x10BD)
#define HDMITX_DWC_FC_RDRB6                     (DWC_OFFSET_MASK + 0x10BE)
#define HDMITX_DWC_FC_RDRB7                     (DWC_OFFSET_MASK + 0x10BF)
#define HDMITX_DWC_FC_MASK0                     (DWC_OFFSET_MASK + 0x10D2)
#define HDMITX_DWC_FC_MASK1                     (DWC_OFFSET_MASK + 0x10D6)
#define HDMITX_DWC_FC_MASK2                     (DWC_OFFSET_MASK + 0x10DA)
#define HDMITX_DWC_FC_PRCONF                    (DWC_OFFSET_MASK + 0x10E0)
#define HDMITX_DWC_FC_SCRAMBLER_CTRL            (DWC_OFFSET_MASK + 0x10E1)

#define HDMITX_DWC_FC_GMD_STAT                  (DWC_OFFSET_MASK + 0x1100)
#define HDMITX_DWC_FC_GMD_EN                    (DWC_OFFSET_MASK + 0x1101)
#define HDMITX_DWC_FC_GMD_UP                    (DWC_OFFSET_MASK + 0x1102)
#define HDMITX_DWC_FC_GMD_CONF                  (DWC_OFFSET_MASK + 0x1103)
#define HDMITX_DWC_FC_GMD_HB                    (DWC_OFFSET_MASK + 0x1104)
#define HDMITX_DWC_FC_GMD_PB0                   (DWC_OFFSET_MASK + 0x1105)
#define HDMITX_DWC_FC_GMD_PB1                   (DWC_OFFSET_MASK + 0x1106)
#define HDMITX_DWC_FC_GMD_PB2                   (DWC_OFFSET_MASK + 0x1107)
#define HDMITX_DWC_FC_GMD_PB3                   (DWC_OFFSET_MASK + 0x1108)
#define HDMITX_DWC_FC_GMD_PB4                   (DWC_OFFSET_MASK + 0x1109)
#define HDMITX_DWC_FC_GMD_PB5                   (DWC_OFFSET_MASK + 0x110A)
#define HDMITX_DWC_FC_GMD_PB6                   (DWC_OFFSET_MASK + 0x110B)
#define HDMITX_DWC_FC_GMD_PB7                   (DWC_OFFSET_MASK + 0x110C)
#define HDMITX_DWC_FC_GMD_PB8                   (DWC_OFFSET_MASK + 0x110D)
#define HDMITX_DWC_FC_GMD_PB9                   (DWC_OFFSET_MASK + 0x110E)
#define HDMITX_DWC_FC_GMD_PB10                  (DWC_OFFSET_MASK + 0x110F)
#define HDMITX_DWC_FC_GMD_PB11                  (DWC_OFFSET_MASK + 0x1110)
#define HDMITX_DWC_FC_GMD_PB12                  (DWC_OFFSET_MASK + 0x1111)
#define HDMITX_DWC_FC_GMD_PB13                  (DWC_OFFSET_MASK + 0x1112)
#define HDMITX_DWC_FC_GMD_PB14                  (DWC_OFFSET_MASK + 0x1113)
#define HDMITX_DWC_FC_GMD_PB15                  (DWC_OFFSET_MASK + 0x1114)
#define HDMITX_DWC_FC_GMD_PB16                  (DWC_OFFSET_MASK + 0x1115)
#define HDMITX_DWC_FC_GMD_PB17                  (DWC_OFFSET_MASK + 0x1116)
#define HDMITX_DWC_FC_GMD_PB18                  (DWC_OFFSET_MASK + 0x1117)
#define HDMITX_DWC_FC_GMD_PB19                  (DWC_OFFSET_MASK + 0x1118)
#define HDMITX_DWC_FC_GMD_PB20                  (DWC_OFFSET_MASK + 0x1119)
#define HDMITX_DWC_FC_GMD_PB21                  (DWC_OFFSET_MASK + 0x111A)
#define HDMITX_DWC_FC_GMD_PB22                  (DWC_OFFSET_MASK + 0x111B)
#define HDMITX_DWC_FC_GMD_PB23                  (DWC_OFFSET_MASK + 0x111C)
#define HDMITX_DWC_FC_GMD_PB24                  (DWC_OFFSET_MASK + 0x111D)
#define HDMITX_DWC_FC_GMD_PB25                  (DWC_OFFSET_MASK + 0x111E)
#define HDMITX_DWC_FC_GMD_PB26                  (DWC_OFFSET_MASK + 0x111F)
#define HDMITX_DWC_FC_GMD_PB27                  (DWC_OFFSET_MASK + 0x1120)

#define HDMITX_DWC_FC_DBGFORCE                  (DWC_OFFSET_MASK + 0x1200)
#define HDMITX_DWC_FC_DBGAUD0CH0                (DWC_OFFSET_MASK + 0x1201)
#define HDMITX_DWC_FC_DBGAUD1CH0                (DWC_OFFSET_MASK + 0x1202)
#define HDMITX_DWC_FC_DBGAUD2CH0                (DWC_OFFSET_MASK + 0x1203)
#define HDMITX_DWC_FC_DBGAUD0CH1                (DWC_OFFSET_MASK + 0x1204)
#define HDMITX_DWC_FC_DBGAUD1CH1                (DWC_OFFSET_MASK + 0x1205)
#define HDMITX_DWC_FC_DBGAUD2CH1                (DWC_OFFSET_MASK + 0x1206)
#define HDMITX_DWC_FC_DBGAUD0CH2                (DWC_OFFSET_MASK + 0x1207)
#define HDMITX_DWC_FC_DBGAUD1CH2                (DWC_OFFSET_MASK + 0x1208)
#define HDMITX_DWC_FC_DBGAUD2CH2                (DWC_OFFSET_MASK + 0x1209)
#define HDMITX_DWC_FC_DBGAUD0CH3                (DWC_OFFSET_MASK + 0x120A)
#define HDMITX_DWC_FC_DBGAUD1CH3                (DWC_OFFSET_MASK + 0x120B)
#define HDMITX_DWC_FC_DBGAUD2CH3                (DWC_OFFSET_MASK + 0x120C)
#define HDMITX_DWC_FC_DBGAUD0CH4                (DWC_OFFSET_MASK + 0x120D)
#define HDMITX_DWC_FC_DBGAUD1CH4                (DWC_OFFSET_MASK + 0x120E)
#define HDMITX_DWC_FC_DBGAUD2CH4                (DWC_OFFSET_MASK + 0x120F)
#define HDMITX_DWC_FC_DBGAUD0CH5                (DWC_OFFSET_MASK + 0x1210)
#define HDMITX_DWC_FC_DBGAUD1CH5                (DWC_OFFSET_MASK + 0x1211)
#define HDMITX_DWC_FC_DBGAUD2CH5                (DWC_OFFSET_MASK + 0x1212)
#define HDMITX_DWC_FC_DBGAUD0CH6                (DWC_OFFSET_MASK + 0x1213)
#define HDMITX_DWC_FC_DBGAUD1CH6                (DWC_OFFSET_MASK + 0x1214)
#define HDMITX_DWC_FC_DBGAUD2CH6                (DWC_OFFSET_MASK + 0x1215)
#define HDMITX_DWC_FC_DBGAUD0CH7                (DWC_OFFSET_MASK + 0x1216)
#define HDMITX_DWC_FC_DBGAUD1CH7                (DWC_OFFSET_MASK + 0x1217)
#define HDMITX_DWC_FC_DBGAUD2CH7                (DWC_OFFSET_MASK + 0x1218)
#define HDMITX_DWC_FC_DBGTMDS0                  (DWC_OFFSET_MASK + 0x1219)
#define HDMITX_DWC_FC_DBGTMDS1                  (DWC_OFFSET_MASK + 0x121A)
#define HDMITX_DWC_FC_DBGTMDS2                  (DWC_OFFSET_MASK + 0x121B)

// HDMI Source PHY Registers
#define HDMITX_DWC_PHY_CONF0                    (DWC_OFFSET_MASK + 0x3000)
#define HDMITX_DWC_PHY_TST0                     (DWC_OFFSET_MASK + 0x3001)
#define HDMITX_DWC_PHY_TST1                     (DWC_OFFSET_MASK + 0x3002)
#define HDMITX_DWC_PHY_TST2                     (DWC_OFFSET_MASK + 0x3003)
#define HDMITX_DWC_PHY_STAT0                    (DWC_OFFSET_MASK + 0x3004)
#define HDMITX_DWC_PHY_INT0                     (DWC_OFFSET_MASK + 0x3005)
#define HDMITX_DWC_PHY_MASK0                    (DWC_OFFSET_MASK + 0x3006)
#define HDMITX_DWC_PHY_POL0                     (DWC_OFFSET_MASK + 0x3007)

// I2C Master PHY Registers
#define HDMITX_DWC_I2CM_PHY_SLAVE               (DWC_OFFSET_MASK + 0x3020)
#define HDMITX_DWC_I2CM_PHY_ADDRESS             (DWC_OFFSET_MASK + 0x3021)
#define HDMITX_DWC_I2CM_PHY_DATAO_1             (DWC_OFFSET_MASK + 0x3022)
#define HDMITX_DWC_I2CM_PHY_DATAO_0             (DWC_OFFSET_MASK + 0x3023)
#define HDMITX_DWC_I2CM_PHY_DATAI_1             (DWC_OFFSET_MASK + 0x3024)
#define HDMITX_DWC_I2CM_PHY_DATAI_0             (DWC_OFFSET_MASK + 0x3025)
#define HDMITX_DWC_I2CM_PHY_OPERATION           (DWC_OFFSET_MASK + 0x3026)
#define HDMITX_DWC_I2CM_PHY_INT                 (DWC_OFFSET_MASK + 0x3027)
#define HDMITX_DWC_I2CM_PHY_CTLINT              (DWC_OFFSET_MASK + 0x3028)
#define HDMITX_DWC_I2CM_PHY_DIV                 (DWC_OFFSET_MASK + 0x3029)
#define HDMITX_DWC_I2CM_PHY_SOFTRSTZ            (DWC_OFFSET_MASK + 0x302A)
#define HDMITX_DWC_I2CM_PHY_SS_SCL_HCNT_1       (DWC_OFFSET_MASK + 0x302B)
#define HDMITX_DWC_I2CM_PHY_SS_SCL_HCNT_0       (DWC_OFFSET_MASK + 0x302C)
#define HDMITX_DWC_I2CM_PHY_SS_SCL_LCNT_1       (DWC_OFFSET_MASK + 0x302D)
#define HDMITX_DWC_I2CM_PHY_SS_SCL_LCNT_0       (DWC_OFFSET_MASK + 0x302E)
#define HDMITX_DWC_I2CM_PHY_FS_SCL_HCNT_1       (DWC_OFFSET_MASK + 0x302F)
#define HDMITX_DWC_I2CM_PHY_FS_SCL_HCNT_0       (DWC_OFFSET_MASK + 0x3030)
#define HDMITX_DWC_I2CM_PHY_FS_SCL_LCNT_1       (DWC_OFFSET_MASK + 0x3031)
#define HDMITX_DWC_I2CM_PHY_FS_SCL_LCNT_0       (DWC_OFFSET_MASK + 0x3032)
#define HDMITX_DWC_I2CM_PHY_SDA_HOLD            (DWC_OFFSET_MASK + 0x3033)

// Audio Sampler Registers
#define HDMITX_DWC_AUD_CONF0                    (DWC_OFFSET_MASK + 0x3100)
#define HDMITX_DWC_AUD_CONF1                    (DWC_OFFSET_MASK + 0x3101)
#define HDMITX_DWC_AUD_INT                      (DWC_OFFSET_MASK + 0x3102)
#define HDMITX_DWC_AUD_CONF2                    (DWC_OFFSET_MASK + 0x3103)
#define HDMITX_DWC_AUD_INT1                     (DWC_OFFSET_MASK + 0x3104)

#define HDMITX_DWC_AUD_N1                       (DWC_OFFSET_MASK + 0x3200)
#define HDMITX_DWC_AUD_N2                       (DWC_OFFSET_MASK + 0x3201)
#define HDMITX_DWC_AUD_N3                       (DWC_OFFSET_MASK + 0x3202)
#define HDMITX_DWC_AUD_CTS1                     (DWC_OFFSET_MASK + 0x3203)
#define HDMITX_DWC_AUD_CTS2                     (DWC_OFFSET_MASK + 0x3204)
#define HDMITX_DWC_AUD_CTS3                     (DWC_OFFSET_MASK + 0x3205)
#define HDMITX_DWC_AUD_INPUTCLKFS               (DWC_OFFSET_MASK + 0x3206)

#define HDMITX_DWC_AUD_SPDIF0                   (DWC_OFFSET_MASK + 0x3300)
#define HDMITX_DWC_AUD_SPDIF1                   (DWC_OFFSET_MASK + 0x3301)
#define HDMITX_DWC_AUD_SPDIFINT                 (DWC_OFFSET_MASK + 0x3302)
#define HDMITX_DWC_AUD_SPDIFINT1                (DWC_OFFSET_MASK + 0x3303)

// Generic Parallel Audio Interface Registers   (DWC_OFFSET_MASK + 0x3500)
// Audio DMA Registers                          (DWC_OFFSET_MASK + 0x3600)

// Main Controller Registers
#define HDMITX_DWC_MC_CLKDIS                    (DWC_OFFSET_MASK + 0x4001)
#define HDMITX_DWC_MC_SWRSTZREQ                 (DWC_OFFSET_MASK + 0x4002)
#define HDMITX_DWC_MC_OPCTRL                    (DWC_OFFSET_MASK + 0x4003)
#define HDMITX_DWC_MC_FLOWCTRL                  (DWC_OFFSET_MASK + 0x4004)
#define HDMITX_DWC_MC_PHYRSTZ                   (DWC_OFFSET_MASK + 0x4005)
#define HDMITX_DWC_MC_LOCKONCLOCK               (DWC_OFFSET_MASK + 0x4006)

// Color Space Converter Registers
#define HDMITX_DWC_CSC_CFG                      (DWC_OFFSET_MASK + 0x4100)
#define HDMITX_DWC_CSC_SCALE                    (DWC_OFFSET_MASK + 0x4101)
#define HDMITX_DWC_CSC_COEF_A1_MSB              (DWC_OFFSET_MASK + 0x4102)
#define HDMITX_DWC_CSC_COEF_A1_LSB              (DWC_OFFSET_MASK + 0x4103)
#define HDMITX_DWC_CSC_COEF_A2_MSB              (DWC_OFFSET_MASK + 0x4104)
#define HDMITX_DWC_CSC_COEF_A2_LSB              (DWC_OFFSET_MASK + 0x4105)
#define HDMITX_DWC_CSC_COEF_A3_MSB              (DWC_OFFSET_MASK + 0x4106)
#define HDMITX_DWC_CSC_COEF_A3_LSB              (DWC_OFFSET_MASK + 0x4107)
#define HDMITX_DWC_CSC_COEF_A4_MSB              (DWC_OFFSET_MASK + 0x4108)
#define HDMITX_DWC_CSC_COEF_A4_LSB              (DWC_OFFSET_MASK + 0x4109)
#define HDMITX_DWC_CSC_COEF_B1_MSB              (DWC_OFFSET_MASK + 0x410A)
#define HDMITX_DWC_CSC_COEF_B1_LSB              (DWC_OFFSET_MASK + 0x410B)
#define HDMITX_DWC_CSC_COEF_B2_MSB              (DWC_OFFSET_MASK + 0x410C)
#define HDMITX_DWC_CSC_COEF_B2_LSB              (DWC_OFFSET_MASK + 0x410D)
#define HDMITX_DWC_CSC_COEF_B3_MSB              (DWC_OFFSET_MASK + 0x410E)
#define HDMITX_DWC_CSC_COEF_B3_LSB              (DWC_OFFSET_MASK + 0x410F)
#define HDMITX_DWC_CSC_COEF_B4_MSB              (DWC_OFFSET_MASK + 0x4110)
#define HDMITX_DWC_CSC_COEF_B4_LSB              (DWC_OFFSET_MASK + 0x4111)
#define HDMITX_DWC_CSC_COEF_C1_MSB              (DWC_OFFSET_MASK + 0x4112)
#define HDMITX_DWC_CSC_COEF_C1_LSB              (DWC_OFFSET_MASK + 0x4113)
#define HDMITX_DWC_CSC_COEF_C2_MSB              (DWC_OFFSET_MASK + 0x4114)
#define HDMITX_DWC_CSC_COEF_C2_LSB              (DWC_OFFSET_MASK + 0x4115)
#define HDMITX_DWC_CSC_COEF_C3_MSB              (DWC_OFFSET_MASK + 0x4116)
#define HDMITX_DWC_CSC_COEF_C3_LSB              (DWC_OFFSET_MASK + 0x4117)
#define HDMITX_DWC_CSC_COEF_C4_MSB              (DWC_OFFSET_MASK + 0x4118)
#define HDMITX_DWC_CSC_COEF_C4_LSB              (DWC_OFFSET_MASK + 0x4119)
#define HDMITX_DWC_CSC_SPARE1                   (DWC_OFFSET_MASK + 0x411A)
#define HDMITX_DWC_CSC_SPARE2                   (DWC_OFFSET_MASK + 0x411B)

// HDCP Encryption Engine Registers
#define HDMITX_DWC_A_HDCPCFG0                   (DWC_OFFSET_MASK + 0x5000)
#define HDMITX_DWC_A_HDCPCFG1                   (DWC_OFFSET_MASK + 0x5001)
#define HDMITX_DWC_A_HDCPOBS0                   (DWC_OFFSET_MASK + 0x5002)
#define HDMITX_DWC_A_HDCPOBS1                   (DWC_OFFSET_MASK + 0x5003)
#define HDMITX_DWC_A_HDCPOBS2                   (DWC_OFFSET_MASK + 0x5004)
#define HDMITX_DWC_A_HDCPOBS3                   (DWC_OFFSET_MASK + 0x5005)
#define HDMITX_DWC_A_APIINTCLR                  (DWC_OFFSET_MASK + 0x5006)
#define HDMITX_DWC_A_APIINTSTAT                 (DWC_OFFSET_MASK + 0x5007)
#define HDMITX_DWC_A_APIINTMSK                  (DWC_OFFSET_MASK + 0x5008)
#define HDMITX_DWC_A_VIDPOLCFG                  (DWC_OFFSET_MASK + 0x5009)
#define HDMITX_DWC_A_OESSWCFG                   (DWC_OFFSET_MASK + 0x500A)
#define HDMITX_DWC_A_COREVERLSB                 (DWC_OFFSET_MASK + 0x5014)
#define HDMITX_DWC_A_COREVERMSB                 (DWC_OFFSET_MASK + 0x5015)
#define HDMITX_DWC_A_KSVMEMCTRL                 (DWC_OFFSET_MASK + 0x5016)

#define HDMITX_DWC_HDCP_BSTATUS_0               (DWC_OFFSET_MASK + 0x5020)
#define HDMITX_DWC_HDCP_BSTATUS_1               (DWC_OFFSET_MASK + 0x5021)
#define HDMITX_DWC_HDCP_M0_0                    (DWC_OFFSET_MASK + 0x5022)
#define HDMITX_DWC_HDCP_M0_1                    (DWC_OFFSET_MASK + 0x5023)
#define HDMITX_DWC_HDCP_M0_2                    (DWC_OFFSET_MASK + 0x5024)
#define HDMITX_DWC_HDCP_M0_3                    (DWC_OFFSET_MASK + 0x5025)
#define HDMITX_DWC_HDCP_M0_4                    (DWC_OFFSET_MASK + 0x5026)
#define HDMITX_DWC_HDCP_M0_5                    (DWC_OFFSET_MASK + 0x5027)
#define HDMITX_DWC_HDCP_M0_6                    (DWC_OFFSET_MASK + 0x5028)
#define HDMITX_DWC_HDCP_M0_7                    (DWC_OFFSET_MASK + 0x5029)
#define HDMITX_DWC_HDCP_KSV                     (DWC_OFFSET_MASK + 0x502A)
#define HDMITX_DWC_HDCP_VH                      (DWC_OFFSET_MASK + 0x52A5)
#define HDMITX_DWC_HDCP_REVOC_SIZE_0            (DWC_OFFSET_MASK + 0x52B9)
#define HDMITX_DWC_HDCP_REVOC_SIZE_1            (DWC_OFFSET_MASK + 0x52BA)
#define HDMITX_DWC_HDCP_REVOC_LIST              (DWC_OFFSET_MASK + 0x52BB)

// HDCP BKSV Registers
#define HDMITX_DWC_HDCPREG_BKSV0                (DWC_OFFSET_MASK + 0x7800)
#define HDMITX_DWC_HDCPREG_BKSV1                (DWC_OFFSET_MASK + 0x7801)
#define HDMITX_DWC_HDCPREG_BKSV2                (DWC_OFFSET_MASK + 0x7802)
#define HDMITX_DWC_HDCPREG_BKSV3                (DWC_OFFSET_MASK + 0x7803)
#define HDMITX_DWC_HDCPREG_BKSV4                (DWC_OFFSET_MASK + 0x7804)

// HDCP AN Registers
#define HDMITX_DWC_HDCPREG_ANCONF               (DWC_OFFSET_MASK + 0x7805)
#define HDMITX_DWC_HDCPREG_AN0                  (DWC_OFFSET_MASK + 0x7806)
#define HDMITX_DWC_HDCPREG_AN1                  (DWC_OFFSET_MASK + 0x7807)
#define HDMITX_DWC_HDCPREG_AN2                  (DWC_OFFSET_MASK + 0x7808)
#define HDMITX_DWC_HDCPREG_AN3                  (DWC_OFFSET_MASK + 0x7809)
#define HDMITX_DWC_HDCPREG_AN4                  (DWC_OFFSET_MASK + 0x780A)
#define HDMITX_DWC_HDCPREG_AN5                  (DWC_OFFSET_MASK + 0x780B)
#define HDMITX_DWC_HDCPREG_AN6                  (DWC_OFFSET_MASK + 0x780C)
#define HDMITX_DWC_HDCPREG_AN7                  (DWC_OFFSET_MASK + 0x780D)
#define HDMITX_DWC_HDCPREG_RMLCTL               (DWC_OFFSET_MASK + 0x780E)

// Encrypted DPK Embedded Storage Registers
#define HDMITX_DWC_HDCPREG_RMLSTS               (DWC_OFFSET_MASK + 0x780F)
#define HDMITX_DWC_HDCPREG_SEED0                (DWC_OFFSET_MASK + 0x7810)
#define HDMITX_DWC_HDCPREG_SEED1                (DWC_OFFSET_MASK + 0x7811)
#define HDMITX_DWC_HDCPREG_DPK0                 (DWC_OFFSET_MASK + 0x7812)
#define HDMITX_DWC_HDCPREG_DPK1                 (DWC_OFFSET_MASK + 0x7813)
#define HDMITX_DWC_HDCPREG_DPK2                 (DWC_OFFSET_MASK + 0x7814)
#define HDMITX_DWC_HDCPREG_DPK3                 (DWC_OFFSET_MASK + 0x7815)
#define HDMITX_DWC_HDCPREG_DPK4                 (DWC_OFFSET_MASK + 0x7816)
#define HDMITX_DWC_HDCPREG_DPK5                 (DWC_OFFSET_MASK + 0x7817)
#define HDMITX_DWC_HDCPREG_DPK6                 (DWC_OFFSET_MASK + 0x7818)

// I2C Master Registers(E-DDC/SCDC)
#define HDMITX_DWC_I2CM_SLAVE                   (DWC_OFFSET_MASK + 0x7E00)
#define HDMITX_DWC_I2CM_ADDRESS                 (DWC_OFFSET_MASK + 0x7E01)
#define HDMITX_DWC_I2CM_DATAO                   (DWC_OFFSET_MASK + 0x7E02)
#define HDMITX_DWC_I2CM_DATAI                   (DWC_OFFSET_MASK + 0x7E03)
#define HDMITX_DWC_I2CM_OPERATION               (DWC_OFFSET_MASK + 0x7E04)
#define HDMITX_DWC_I2CM_INT                     (DWC_OFFSET_MASK + 0x7E05)
#define HDMITX_DWC_I2CM_CTLINT                  (DWC_OFFSET_MASK + 0x7E06)
#define HDMITX_DWC_I2CM_DIV                     (DWC_OFFSET_MASK + 0x7E07)
#define HDMITX_DWC_I2CM_SEGADDR                 (DWC_OFFSET_MASK + 0x7E08)
#define HDMITX_DWC_I2CM_SOFTRSTZ                (DWC_OFFSET_MASK + 0x7E09)
#define HDMITX_DWC_I2CM_SEGPTR                  (DWC_OFFSET_MASK + 0x7E0A)
#define HDMITX_DWC_I2CM_SS_SCL_HCNT_1           (DWC_OFFSET_MASK + 0x7E0B)
#define HDMITX_DWC_I2CM_SS_SCL_HCNT_0           (DWC_OFFSET_MASK + 0x7E0C)
#define HDMITX_DWC_I2CM_SS_SCL_LCNT_1           (DWC_OFFSET_MASK + 0x7E0D)
#define HDMITX_DWC_I2CM_SS_SCL_LCNT_0           (DWC_OFFSET_MASK + 0x7E0E)
#define HDMITX_DWC_I2CM_FS_SCL_HCNT_1           (DWC_OFFSET_MASK + 0x7E0F)
#define HDMITX_DWC_I2CM_FS_SCL_HCNT_0           (DWC_OFFSET_MASK + 0x7E10)
#define HDMITX_DWC_I2CM_FS_SCL_LCNT_1           (DWC_OFFSET_MASK + 0x7E11)
#define HDMITX_DWC_I2CM_FS_SCL_LCNT_0           (DWC_OFFSET_MASK + 0x7E12)
#define HDMITX_DWC_I2CM_SDA_HOLD                (DWC_OFFSET_MASK + 0x7E13)
#define HDMITX_DWC_I2CM_SCDC_UPDATE             (DWC_OFFSET_MASK + 0x7E14)
#define HDMITX_DWC_I2CM_READ_BUFF0              (DWC_OFFSET_MASK + 0x7E20)
#define HDMITX_DWC_I2CM_READ_BUFF1              (DWC_OFFSET_MASK + 0x7E21)
#define HDMITX_DWC_I2CM_READ_BUFF2              (DWC_OFFSET_MASK + 0x7E22)
#define HDMITX_DWC_I2CM_READ_BUFF3              (DWC_OFFSET_MASK + 0x7E23)
#define HDMITX_DWC_I2CM_READ_BUFF4              (DWC_OFFSET_MASK + 0x7E24)
#define HDMITX_DWC_I2CM_READ_BUFF5              (DWC_OFFSET_MASK + 0x7E25)
#define HDMITX_DWC_I2CM_READ_BUFF6              (DWC_OFFSET_MASK + 0x7E26)
#define HDMITX_DWC_I2CM_READ_BUFF7              (DWC_OFFSET_MASK + 0x7E27)
#define HDMITX_DWC_I2CM_SCDC_UPDATE0            (DWC_OFFSET_MASK + 0x7E30)
#define HDMITX_DWC_I2CM_SCDC_UPDATE1            (DWC_OFFSET_MASK + 0x7E31)


//********** CEC related **********//

// CEC 2.0 Engine Registers
#define HDMITX_DWC_CEC_CTRL                     (DWC_OFFSET_MASK + 0x7D00)
#define HDMITX_DWC_CEC_INTR_MASK                (DWC_OFFSET_MASK + 0x7D02)
#define HDMITX_DWC_CEC_LADD_LOW                 (DWC_OFFSET_MASK + 0x7D05)
#define HDMITX_DWC_CEC_LADD_HIGH                (DWC_OFFSET_MASK + 0x7D06)
#define HDMITX_DWC_CEC_TX_CNT                   (DWC_OFFSET_MASK + 0x7D07)
#define HDMITX_DWC_CEC_RX_CNT                   (DWC_OFFSET_MASK + 0x7D08)
#define HDMITX_DWC_CEC_TX_DATA00                (DWC_OFFSET_MASK + 0x7D10)
#define HDMITX_DWC_CEC_TX_DATA01                (DWC_OFFSET_MASK + 0x7D11)
#define HDMITX_DWC_CEC_TX_DATA02                (DWC_OFFSET_MASK + 0x7D12)
#define HDMITX_DWC_CEC_TX_DATA03                (DWC_OFFSET_MASK + 0x7D13)
#define HDMITX_DWC_CEC_TX_DATA04                (DWC_OFFSET_MASK + 0x7D14)
#define HDMITX_DWC_CEC_TX_DATA05                (DWC_OFFSET_MASK + 0x7D15)
#define HDMITX_DWC_CEC_TX_DATA06                (DWC_OFFSET_MASK + 0x7D16)
#define HDMITX_DWC_CEC_TX_DATA07                (DWC_OFFSET_MASK + 0x7D17)
#define HDMITX_DWC_CEC_TX_DATA08                (DWC_OFFSET_MASK + 0x7D18)
#define HDMITX_DWC_CEC_TX_DATA09                (DWC_OFFSET_MASK + 0x7D19)
#define HDMITX_DWC_CEC_TX_DATA10                (DWC_OFFSET_MASK + 0x7D1A)
#define HDMITX_DWC_CEC_TX_DATA11                (DWC_OFFSET_MASK + 0x7D1B)
#define HDMITX_DWC_CEC_TX_DATA12                (DWC_OFFSET_MASK + 0x7D1C)
#define HDMITX_DWC_CEC_TX_DATA13                (DWC_OFFSET_MASK + 0x7D1D)
#define HDMITX_DWC_CEC_TX_DATA14                (DWC_OFFSET_MASK + 0x7D1E)
#define HDMITX_DWC_CEC_TX_DATA15                (DWC_OFFSET_MASK + 0x7D1F)
#define HDMITX_DWC_CEC_RX_DATA00                (DWC_OFFSET_MASK + 0x7D20)
#define HDMITX_DWC_CEC_RX_DATA01                (DWC_OFFSET_MASK + 0x7D21)
#define HDMITX_DWC_CEC_RX_DATA02                (DWC_OFFSET_MASK + 0x7D22)
#define HDMITX_DWC_CEC_RX_DATA03                (DWC_OFFSET_MASK + 0x7D23)
#define HDMITX_DWC_CEC_RX_DATA04                (DWC_OFFSET_MASK + 0x7D24)
#define HDMITX_DWC_CEC_RX_DATA05                (DWC_OFFSET_MASK + 0x7D25)
#define HDMITX_DWC_CEC_RX_DATA06                (DWC_OFFSET_MASK + 0x7D26)
#define HDMITX_DWC_CEC_RX_DATA07                (DWC_OFFSET_MASK + 0x7D27)
#define HDMITX_DWC_CEC_RX_DATA08                (DWC_OFFSET_MASK + 0x7D28)
#define HDMITX_DWC_CEC_RX_DATA09                (DWC_OFFSET_MASK + 0x7D29)
#define HDMITX_DWC_CEC_RX_DATA10                (DWC_OFFSET_MASK + 0x7D2A)
#define HDMITX_DWC_CEC_RX_DATA11                (DWC_OFFSET_MASK + 0x7D2B)
#define HDMITX_DWC_CEC_RX_DATA12                (DWC_OFFSET_MASK + 0x7D2C)
#define HDMITX_DWC_CEC_RX_DATA13                (DWC_OFFSET_MASK + 0x7D2D)
#define HDMITX_DWC_CEC_RX_DATA14                (DWC_OFFSET_MASK + 0x7D2E)
#define HDMITX_DWC_CEC_RX_DATA15                (DWC_OFFSET_MASK + 0x7D2F)
#define HDMITX_DWC_CEC_LOCK_BUF                 (DWC_OFFSET_MASK + 0x7D30)
#define HDMITX_DWC_CEC_WAKEUPCTRL               (DWC_OFFSET_MASK + 0x7D31)


//read/write
#define CEC_TX_MSG_0_HEADER        0x00
#define CEC_TX_MSG_1_OPCODE        0x01
#define CEC_TX_MSG_2_OP1           0x02
#define CEC_TX_MSG_3_OP2           0x03
#define CEC_TX_MSG_4_OP3           0x04
#define CEC_TX_MSG_5_OP4           0x05
#define CEC_TX_MSG_6_OP5           0x06
#define CEC_TX_MSG_7_OP6           0x07
#define CEC_TX_MSG_8_OP7           0x08
#define CEC_TX_MSG_9_OP8           0x09
#define CEC_TX_MSG_A_OP9           0x0A
#define CEC_TX_MSG_B_OP10          0x0B
#define CEC_TX_MSG_C_OP11          0x0C
#define CEC_TX_MSG_D_OP12          0x0D
#define CEC_TX_MSG_E_OP13          0x0E
#define CEC_TX_MSG_F_OP14          0x0F

//read/write
#define CEC_TX_MSG_LENGTH          0x10
#define CEC_TX_MSG_CMD             0x11
#define CEC_TX_WRITE_BUF           0x12
#define CEC_TX_CLEAR_BUF           0x13
#define CEC_RX_MSG_CMD             0x14
#define CEC_RX_CLEAR_BUF           0x15
#define CEC_LOGICAL_ADDR0          0x16
#define CEC_LOGICAL_ADDR1          0x17
#define CEC_LOGICAL_ADDR2          0x18
#define CEC_LOGICAL_ADDR3          0x19
#define CEC_LOGICAL_ADDR4          0x1A
#define CEC_CLOCK_DIV_H            0x1B
#define CEC_CLOCK_DIV_L            0x1C

// The following registers are for fine tuning CEC bit timing parameters.
// They are only valid in AO CEC, NOT valid in HDMITX CEC.
// The AO CEC's timing parameters are already set default to work with 32768Hz clock, so hopefully
// SW never need to program these registers. The timing registers are made programmable just in case.
#define CEC_QUIESCENT_25MS_BIT7_0               0x20
#define CEC_QUIESCENT_25MS_BIT11_8              0x21
#define CEC_STARTBITMINL2H_3MS5_BIT7_0          0x22
#define CEC_STARTBITMINL2H_3MS5_BIT8            0x23
#define CEC_STARTBITMAXL2H_3MS9_BIT7_0          0x24
#define CEC_STARTBITMAXL2H_3MS9_BIT8            0x25
#define CEC_STARTBITMINH_0MS6_BIT7_0            0x26
#define CEC_STARTBITMINH_0MS6_BIT8              0x27
#define CEC_STARTBITMAXH_1MS0_BIT7_0            0x28
#define CEC_STARTBITMAXH_1MS0_BIT8              0x29
#define CEC_STARTBITMINTOTAL_4MS3_BIT7_0        0x2A
#define CEC_STARTBITMINTOTAL_4MS3_BIT9_8        0x2B
#define CEC_STARTBITMAXTOTAL_4MS7_BIT7_0        0x2C
#define CEC_STARTBITMAXTOTAL_4MS7_BIT9_8        0x2D
#define CEC_LOGIC1MINL2H_0MS4_BIT7_0            0x2E
#define CEC_LOGIC1MINL2H_0MS4_BIT8              0x2F
#define CEC_LOGIC1MAXL2H_0MS8_BIT7_0            0x30
#define CEC_LOGIC1MAXL2H_0MS8_BIT8              0x31
#define CEC_LOGIC0MINL2H_1MS3_BIT7_0            0x32
#define CEC_LOGIC0MINL2H_1MS3_BIT8              0x33
#define CEC_LOGIC0MAXL2H_1MS7_BIT7_0            0x34
#define CEC_LOGIC0MAXL2H_1MS7_BIT8              0x35
#define CEC_LOGICMINTOTAL_2MS05_BIT7_0          0x36
#define CEC_LOGICMINTOTAL_2MS05_BIT9_8          0x37
#define CEC_LOGICMAXHIGH_2MS8_BIT7_0            0x38
#define CEC_LOGICMAXHIGH_2MS8_BIT8              0x39
#define CEC_LOGICERRLOW_3MS4_BIT7_0             0x3A
#define CEC_LOGICERRLOW_3MS4_BIT8               0x3B
#define CEC_NOMSMPPOINT_1MS05                   0x3C
#define CEC_DELCNTR_LOGICERR                    0x3E
#define CEC_TXTIME_17MS_BIT7_0                  0x40
#define CEC_TXTIME_17MS_BIT15_8                 0x41
#define CEC_TXTIME_2BIT_BIT7_0                  0x42
#define CEC_TXTIME_2BIT_BIT15_8                 0x43
#define CEC_TXTIME_4BIT_BIT7_0                  0x44
#define CEC_TXTIME_4BIT_BIT15_8                 0x45
#define CEC_STARTBITNOML2H_3MS7_BIT7_0          0x46
#define CEC_STARTBITNOML2H_3MS7_BIT8            0x47
#define CEC_STARTBITNOMH_0MS8_BIT7_0            0x48
#define CEC_STARTBITNOMH_0MS8_BIT8              0x49
#define CEC_LOGIC1NOML2H_0MS6_BIT7_0            0x4A
#define CEC_LOGIC1NOML2H_0MS6_BIT8              0x4B
#define CEC_LOGIC0NOML2H_1MS5_BIT7_0            0x4C
#define CEC_LOGIC0NOML2H_1MS5_BIT8              0x4D
#define CEC_LOGIC1NOMH_1MS8_BIT7_0              0x4E
#define CEC_LOGIC1NOMH_1MS8_BIT8                0x4F
#define CEC_LOGIC0NOMH_0MS9_BIT7_0              0x50
#define CEC_LOGIC0NOMH_0MS9_BIT8                0x51
#define CEC_LOGICERRLOW_3MS6_BIT7_0             0x52
#define CEC_LOGICERRLOW_3MS6_BIT8               0x53
#define CEC_CHKCONTENTION_0MS1                  0x54
#define CEC_PREPARENXTBIT_0MS05_BIT7_0          0x56
#define CEC_PREPARENXTBIT_0MS05_BIT8            0x57
#define CEC_NOMSMPACKPOINT_0MS45                0x58
#define CEC_ACK0NOML2H_1MS5_BIT7_0              0x5A
#define CEC_ACK0NOML2H_1MS5_BIT8                0x5B

#define CEC_BUGFIX_DISABLE_0                    0x60
#define CEC_BUGFIX_DISABLE_1                    0x61

//read only
#define CEC_RX_MSG_0_HEADER        0x80
#define CEC_RX_MSG_1_OPCODE        0x81
#define CEC_RX_MSG_2_OP1           0x82
#define CEC_RX_MSG_3_OP2           0x83
#define CEC_RX_MSG_4_OP3           0x84
#define CEC_RX_MSG_5_OP4           0x85
#define CEC_RX_MSG_6_OP5           0x86
#define CEC_RX_MSG_7_OP6           0x87
#define CEC_RX_MSG_8_OP7           0x88
#define CEC_RX_MSG_9_OP8           0x89
#define CEC_RX_MSG_A_OP9           0x8A
#define CEC_RX_MSG_B_OP10          0x8B
#define CEC_RX_MSG_C_OP11          0x8C
#define CEC_RX_MSG_D_OP12          0x8D
#define CEC_RX_MSG_E_OP13          0x8E
#define CEC_RX_MSG_F_OP14          0x8F

//read only
#define CEC_RX_MSG_LENGTH          0x90
#define CEC_RX_MSG_STATUS          0x91
#define CEC_RX_NUM_MSG             0x92
#define CEC_TX_MSG_STATUS          0x93
#define CEC_TX_NUM_MSG             0x94

// tx_msg_cmd definition
#define TX_NO_OP                0  // No transaction
#define TX_REQ_CURRENT          1  // Transmit earliest message in buffer
#define TX_ABORT                2  // Abort transmitting earliest message
#define TX_REQ_NEXT             3  // Overwrite earliest message in buffer and transmit next message

// tx_msg_status definition
#define TX_IDLE                 0  // No transaction
#define TX_BUSY                 1  // Transmitter is busy
#define TX_DONE                 2  // Message has been successfully transmitted
#define TX_ERROR                3  // Message has been transmitted with error

// rx_msg_cmd
#define RX_NO_OP                0  // No transaction
#define RX_ACK_CURRENT          1  // Read earliest message in buffer
#define RX_DISABLE              2  // Disable receiving latest message
#define RX_ACK_NEXT             3  // Clear earliest message from buffer and read next message

// rx_msg_status
#define RX_IDLE                 0  // No transaction
#define RX_BUSY                 1  // Receiver is busy
#define RX_DONE                 2  // Message has been received successfully
#define RX_ERROR                3  // Message has been received with error
#endif  /* __HDMI_TX_REG_H_ */

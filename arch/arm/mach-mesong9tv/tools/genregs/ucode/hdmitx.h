#ifndef HDMITX_H
#define HDMITX_H

// Device ID to differentiate HDMITX register access to TOP or DWC
#define HDMITX_DEV_ID_TOP       0
#define HDMITX_DEV_ID_DWC       1

#define HDMITX_ADDR_PORT        0xd0042000  // TOP ADDR_PORT: 0xd0042000; DWC ADDR_PORT: 0xd0042010
#define HDMITX_DATA_PORT        0xd0042004  // TOP DATA_PORT: 0xd0042004; DWC DATA_PORT: 0xd0042014
#define HDMITX_CTRL_PORT        0xd0042008  // TOP CTRL_PORT: 0xd0042008; DWC CTRL_PORT: 0xd0042018

#define INT_HDMITX_BASE_OFFSET  0x00000000  // INT TOP ADDR_PORT: 0xd0042000; INT DWC ADDR_PORT: 0xd0042010
#define EXT_HDMITX_BASE_OFFSET  0x00000020  // EXT TOP ADDR_PORT: 0xd0042020; EXT DWC ADDR_PORT: 0xd0042030

// Use the following functions to access the on-chip HDMITX modules by default
extern void             hdmitx_wr_only_TOP (unsigned long base_offset, unsigned long addr, unsigned long data);
extern void             hdmitx_wr_only_DWC (unsigned long base_offset, unsigned long addr, unsigned long data);
extern void             hdmitx_wr_only_reg (unsigned long base_offset, unsigned char dev_id, unsigned long addr, unsigned long data);
extern unsigned long    hdmitx_rd_TOP (unsigned long base_offset, unsigned long addr);
extern unsigned long    hdmitx_rd_DWC (unsigned long base_offset, unsigned long addr);
extern unsigned long    hdmitx_rd_reg (unsigned long base_offset, unsigned char dev_id, unsigned long addr);
extern void             hdmitx_rd_check_TOP (unsigned long base_offset, unsigned long addr, unsigned long exp_data, unsigned long mask);
extern void             hdmitx_rd_check_DWC (unsigned long base_offset, unsigned long addr, unsigned long exp_data, unsigned long mask);
extern void             hdmitx_rd_check_reg (unsigned long base_offset, unsigned char dev_id, unsigned long addr, unsigned long exp_data, unsigned long mask);
extern void             hdmitx_wr_TOP (unsigned long base_offset, unsigned long addr, unsigned long data);
extern void             hdmitx_wr_DWC (unsigned long base_offset, unsigned long addr, unsigned long data);
extern void             hdmitx_wr_reg (unsigned long base_offset, unsigned char dev_id, unsigned long addr, unsigned long data);
extern void             hdmitx_poll_TOP (unsigned long base_offset, unsigned long addr, unsigned long exp_data, unsigned long mask);
extern void             hdmitx_poll_DWC (unsigned long base_offset, unsigned long addr, unsigned long exp_data, unsigned long mask, unsigned long max_try);
extern void             hdmitx_poll_reg (unsigned long base_offset, unsigned char dev_id, unsigned long addr, unsigned long exp_data, unsigned long mask);

// Use the following functions to access the off-chip HDMI modules by default, input address the same as on-chip HDMI's,
// the regisers accessed by these functions only exist in a simulation environment.



extern void set_hdmi_audio_source (unsigned int src);

extern void config_hdmi20_tx (  unsigned long   base_offset,            // Int/Ext TX address offset. =0 for internal TX; =0x20 for external TX (sim model).
                                unsigned char   pixel_repeat,
                                unsigned char   color_depth,            // Pixel bit width: 4=24-bit; 5=30-bit; 6=36-bit; 7=48-bit.
                                unsigned char   input_color_format,     // Pixel format: 0=RGB444; 1=YCbCr422; 2=YCbCr444; 3=YCbCr420.
                                unsigned char   input_color_range,      // Pixel range: 0=limited; 1=full.
                                unsigned char   output_color_format,    // Pixel format: 0=RGB444; 1=YCbCr422; 2=YCbCr444; 3=YCbCr420.
                                unsigned char   output_color_range,     // Pixel range: 0=limited; 1=full.
                                unsigned char   vic,                    // Video format identification code
                                unsigned char   interlace,              // 0=Progressive; 1=Interlaced.
                                unsigned char   vs_in_pol,              // VSync input polarity: 0=active low; 1=active high.
                                unsigned char   hs_in_pol,              // HSync input polarity: 0=active low; 1=active high.
                                unsigned long   active_pixels,          // Number of active pixels per line
                                unsigned long   blank_pixels,           // Number of blank pixels per line
                                unsigned long   active_lines,           // Number of active lines per field
                                unsigned long   blank_lines,            // Number of blank lines per field
                                unsigned long   front_porch,            // Number of pixels from DE end to HS start
                                unsigned long   hsync_pixels,           // Number of pixels of HS pulse width
                                unsigned long   eof_lines,              // HS count between last active line and start of VS.
                                unsigned long   vsync_lines,            // HS count of VS width
                                unsigned char   scrambler_en,
                                unsigned char   tmds_clk_div4,          // 0:TMDS_CLK_rate=TMDS_Character_rate; 1:TMDS_CLK_rate=TMDS_Character_rate/4, for TMDS_Character_rate>340Mcsc.
                                unsigned long   aud_n,                  // ACR N
                                unsigned char   i2s_spdif,              // 0=SPDIF; 1=I2S. Note: Must select I2S if CHIP_HAVE_HDMI_RX is defined.
                                unsigned char   i2s_8chan,              // 0=I2S 2-channel; 1=I2S 4 x 2-channel.
                                unsigned char   audio_packet_type,      // 2=audio sample packet; 7=one bit audio; 8=DST audio packet; 9=HBR audio packet.
                                unsigned char   hdcp_on,
                                unsigned char   hdcp_an_sw,             // 0=AN is from HW; 1=AN is from SW register.
                                unsigned long   hdcp_an_sw_val_hi,      // SW defined AN[63:32]
                                unsigned long   hdcp_an_sw_val_lo);     // SW defined AN[31:0]

extern void hdmi20_tx_read_edid (   unsigned long   base_offset,    // Int/Ext TX address offset. =0 for internal TX; =0x20 for external TX (sim model).
                                    unsigned char*  rx_edid);

extern unsigned long    hdmitx_scdc_rd          (unsigned long base_offset, unsigned long addr);
extern void             hdmitx_scdc_rd_check    (unsigned long base_offset, unsigned long addr, unsigned long exp_data, unsigned long mask);
extern void             hdmitx_scdc_wr_only     (unsigned long base_offset, unsigned long addr, unsigned long data);
extern void             hdmitx_scdc_wr          (unsigned long base_offset, unsigned long addr, unsigned long data);

// Internal functions:
void hdmitx_csc_config (unsigned long base_offset,          // Int/Ext TX address offset. =0 for internal TX; =0x20 for external TX (sim model).
                        unsigned char input_color_format,
                        unsigned char output_color_format,
                        unsigned char color_depth);
void configure_hdcp_dpk (   unsigned long base_offset,      // Int/Ext TX address offset. =0 for internal TX; =0x20 for external TX (sim model).
                            unsigned long sw_enc_key);

#endif  /* HDMITX_H */

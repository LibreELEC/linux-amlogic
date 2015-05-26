/* linux/drivers/usb/phy/phy-aml-usb.h
 *
 */

#include <linux/usb/phy.h>

#define PHY_REGISTER_SIZE	0x20
/* Register definitions */
typedef struct u2p_aml_regs {
	volatile uint32_t u2p_r0; 
	volatile uint32_t u2p_r1;      
	volatile uint32_t u2p_r2; 
} u2p_aml_regs_t;

typedef union u2p_r0 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned bypass_sel:1;   // 0
		unsigned bypass_dm_en:1; // 1 
		unsigned bypass_dp_en:1; // 2
		unsigned txbitstuffenh:1;// 3
  		unsigned txbitstuffen:1; // 4
		unsigned dmpulldown:1;   // 5
		unsigned dppulldown:1;   // 6
		unsigned vbusvldextsel:1;// 7
		unsigned vbusvldext:1;   // 8
		unsigned adp_prb_en:1;   // 9
		unsigned adp_dischrg:1;  // 10
		unsigned adp_chrg:1;     // 11
		unsigned drvvbus:1;      // 12
		unsigned idpullup:1;     // 13
		unsigned loopbackenb:1;  // 14
		unsigned otgdisable:1;   // 15
		unsigned commononn:1;    // 16
 		unsigned fsel:3;         // 17
		unsigned refclksel:2;    // 20
		unsigned por:1;          // 22
		unsigned vatestenb:2;    // 23
		unsigned set_iddq:1;     // 25
		unsigned ate_reset:1;    // 26
		unsigned fsv_minus:1;    // 27
		unsigned fsv_plus:1;     // 28
		unsigned bypass_dm_data:1; // 29 
		unsigned bypass_dp_data:1; // 30
        	unsigned not_used:1;
	} b;
} u2p_r0_t;

typedef union u2p_r1 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned burn_in_test:1; // 0
		unsigned aca_enable:1;   // 1
		unsigned dcd_enable:1;   // 2
		unsigned vdatsrcenb:1;   // 3
		unsigned vdatdetenb:1;   // 4
		unsigned chrgsel:1;      // 5 
		unsigned tx_preemp_pulse_tune:1; // 6
		unsigned tx_preemp_amp_tune:2;   // 7 
		unsigned tx_res_tune:2;  // 9
		unsigned tx_rise_tune:2; // 11
		unsigned tx_vref_tune:4; // 13
		unsigned tx_fsls_tune:4; // 17
		unsigned tx_hsxv_tune:2; // 21
		unsigned otg_tune:3;     // 23
		unsigned sqrx_tune:3;    // 26
		unsigned comp_dis_tune:3;// 29 
	} b;
} u2p_r1_t;

typedef union u2p_r2 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned data_in:4;      // 0
		unsigned data_in_en:4;   // 4
		unsigned addr:4;         // 8
		unsigned data_out_sel:1; // 12
		unsigned clk:1;          // 13 
		unsigned data_out:4;     // 14
		unsigned aca_pin_range_c:1; // 18
		unsigned aca_pin_range_b:1; // 19
		unsigned aca_pin_range_a:1; // 20
		unsigned aca_pin_gnd:1;     // 21
		unsigned aca_pin_float:1;   // 22 
		unsigned chg_det:1;         // 23
		unsigned device_sess_vld:1; // 24
		unsigned adp_probe:1;    // 25
		unsigned adp_sense:1;    // 26
		unsigned sessend:1;      // 27
		unsigned vbusvalid:1;    // 28
		unsigned bvalid:1;       // 29
		unsigned avalid:1;       // 30
		unsigned iddig:1;        // 31
	} b;
} u2p_r2_t;

typedef struct usb_aml_regs {
	volatile uint32_t usb_r0; 
	volatile uint32_t usb_r1;      
	volatile uint32_t usb_r2; 
	volatile uint32_t usb_r3; 
	volatile uint32_t usb_r4; 
	volatile uint32_t usb_r5; 
	volatile uint32_t usb_r6; 
} usb_aml_regs_t;

typedef union usb_r0 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned p30_fsel:6; // 0
		unsigned p30_phy_reset:1; // 6
		unsigned p30_test_powerdown_hsp:1; // 7
		unsigned p30_test_powerdown_ssp:1; // 8
		unsigned p30_acjt_level:5;         // 9
		unsigned p30_tx_vboost_lvl:3;      // 14
  		unsigned p30_lane0_tx2rx_loopbk:1; // 17
		unsigned p30_lane0_ext_pclk_req:1; // 18
		unsigned p30_pcs_rx_los_mask_val:10; // 19
		unsigned u2d_ss_scaledown_mode:2;  // 29
		unsigned u2d_act:1; // 31
	} b;
} usb_r0_t;

typedef union usb_r1 {
	/** raw register data */
	uint32_t d32;
 	/** register bits */
	struct {
		unsigned u3h_bigendian_gs:1; // 0
		unsigned u3h_pme_en:1; // 1
		unsigned u3h_hub_port_overcurrent:5; // 2
 		unsigned u3h_hub_port_perm_attach:5; // 7
		unsigned u3h_host_u2_port_disable:4; // 12
 		unsigned u3h_host_u3_port_disable:1; // 16
		unsigned u3h_host_port_power_control_present:1; // 17
		unsigned u3h_host_msi_enable:1; // 18
  		unsigned u3h_fladj_30mhz_reg:6; // 19
		unsigned p30_pcs_tx_swing_full:7; // 25
	} b;
} usb_r1_t;

typedef union usb_r2 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned p30_cr_data_in:16;     // 0
		unsigned p30_cr_read:1;         // 16
		unsigned p30_cr_write:1;        // 17
		unsigned p30_cr_cap_addr:1;     // 18
		unsigned p30_cr_cap_data:1;     // 19 
		unsigned p30_pcs_tx_deemph_3p5db:6; // 20
		unsigned p30_pcs_tx_deemph_6db:6; // 26
	} b;
} usb_r2_t;

typedef union usb_r3 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned p30_ssc_en:1; // 0
		unsigned p30_ssc_range:3; // 1
		unsigned p30_ssc_ref_clk_sel:9; // 4
		unsigned p30_ref_ssp_en:1; // 13
		unsigned reserved14:2; // 14
		unsigned p30_los_bias:3; // 16
		unsigned p30_los_level:5; // 19
		unsigned p30_mpll_multiplier:7; // 24
		unsigned reserved31:1; // 31        
	} b;
} usb_r3_t;

typedef union usb_r4 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned p21_PORTRESET0:1; // 0
		unsigned p21_SLEEPM0:1; // 1
		unsigned mem_pd:2;
		unsigned reserved4:28; // 31        
	} b;
} usb_r4_t;

typedef union usb_r5 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned reserved0:32; // 32        
	} b;
} usb_r5_t;

typedef union usb_r6 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned p30_cr_data_out:16; // 0
		unsigned p30_cr_ack:1;       // 16
		unsigned not_used:15;        // 17
	} b;
} usb_r6_t;

struct amlogic_usb {
	struct usb_phy		phy;
	struct device		*dev;
	void __iomem	*regs;
	int portnum;
};

#define	phy_to_amlusb(x)	container_of((x), struct amlogic_usb, phy)

extern int amlogic_usbphy_reset(void);
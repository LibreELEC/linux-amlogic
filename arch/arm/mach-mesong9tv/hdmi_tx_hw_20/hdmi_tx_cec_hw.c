#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <asm/delay.h>
#include <mach/am_regs.h>
#include <mach/power_gate.h>
#include <linux/amlogic/tvin/tvin.h>

#include <mach/gpio.h>
#include <linux/amlogic/hdmi_tx/hdmi_info_global.h>
#include <linux/amlogic/hdmi_tx/hdmi_tx_module.h>
#include <linux/amlogic/hdmi_tx/hdmi_tx_cec.h>
#include "hdmi_tx_reg.h"
//#include "mach/hdmi_parameter.h" 

static DEFINE_MUTEX(cec_mutex);

unsigned int cec_irq_enable_flag = 1;
extern bool cec_msg_dbg_en;

static unsigned char msg_log_buf[128] = { 0 };
static int pos;
//There are two cec modules:AO CEC & HDMI CEC2.0

#ifdef AO_CEC
//*****************************************************AOCEC*************************************************
static void ao_cec_disable_irq(void)
{
    // disable all AO_CEC interrupt sources
    aml_set_reg32_bits(P_AO_CEC_INTR_MASKN, 0x0, 0, 3);
    cec_irq_enable_flag = 0;
    hdmi_print(INF, CEC "disable:int mask:0x%x\n", aml_read_reg32(P_AO_CEC_INTR_MASKN));
}

static void ao_cec_enable_irq(void)
{
    aml_set_reg32_bits(P_AO_CEC_INTR_MASKN, 0x6, 0, 3);
    cec_irq_enable_flag = 1;
    hdmi_print(INF, CEC "enable:int mask:0x%x\n", aml_read_reg32(P_AO_CEC_INTR_MASKN));
}

static void ao_cec_clk_set(void)
{
    //AO CEC clock = 24M/732=32786.9Hz
    //unsigned long data32;
    
    //data32  = 0;
	//data32 |= 1         << (10+16);  // [26]  clk_en
	//data32 |= (732 - 1) << (0+16);   // [25:16] clk_div
    //*P_AO_CRT_CLK_CNTL1 =  data32;
    aml_set_reg32_bits(P_AO_CRT_CLK_CNTL1, 0, 16, 10);
    aml_set_reg32_bits(P_AO_CRT_CLK_CNTL1, 1, 26, 1);
    //aocec_wr_reg(CEC_CLOCK_DIV_L,                   (240-1)&0xff);
    //aocec_wr_reg(CEC_CLOCK_DIV_H,                   ((240-1)>>8)&0xff);
}

static void ao_cec_sw_reset(void)
{
    // Assert SW reset AO_CEC
    //data32  = 0;
    //data32 |= 0 << 1;   // [2:1]    cntl_clk: 0=Disable clk (Power-off mode); 1=Enable gated clock (Normal mode); 2=Enable free-run clk (Debug mode).
    //data32 |= 1 << 0;   // [0]      sw_reset: 1=Reset
    aml_write_reg32(P_AO_CEC_GEN_CNTL, 0x1);
    // Enable gated clock (Normal mode).
    aml_set_reg32_bits(P_AO_CEC_GEN_CNTL, 1, 1, 1);
    // Release SW reset
    aml_set_reg32_bits(P_AO_CEC_GEN_CNTL, 0, 0, 1);
}

static void ao_cec_logic_addr_set(enum _cec_log_dev_addr_e logic_addr)
{
    //tmp debug:set addr 4 for G9TV CEC. To do.
    aocec_wr_reg(CEC_LOGICAL_ADDR0, (0x1 << 4) | 0x4);
}
/*
static void ao_cec_arbit_bit_time_read(void){//11bit:bit[10:0]
    //3 bit
    hdmi_print(INF, CEC "read 3 bit:0x%x%x \n", aocec_rd_reg(CEC_TXTIME_4BIT_BIT15_8),aocec_rd_reg(CEC_TXTIME_4BIT_BIT7_0));
    //5 bit
    hdmi_print(INF, CEC "read 5 bit:0x%x%x \n", aocec_rd_reg(CEC_TXTIME_2BIT_BIT15_8), aocec_rd_reg(CEC_TXTIME_2BIT_BIT7_0));
    //7 bit
    hdmi_print(INF, CEC "read 7 bit:0x%x%x \n", aocec_rd_reg(CEC_TXTIME_17MS_BIT15_8), aocec_rd_reg(CEC_TXTIME_17MS_BIT7_0));
}

static void ao_cec_arbit_bit_time_set(unsigned bit_set, unsigned time_set, unsigned flag){//11bit:bit[10:0]
    if(flag)
        hdmi_print(INF, CEC "bit_set:0x%x;time_set:0x%x \n", bit_set, time_set);
    switch(bit_set){
    case 3:
        //3 bit
        if(flag)
            hdmi_print(INF, CEC "read 3 bit:0x%x%x \n", aocec_rd_reg(CEC_TXTIME_4BIT_BIT15_8),aocec_rd_reg(CEC_TXTIME_4BIT_BIT7_0));
        aocec_wr_reg(CEC_TXTIME_4BIT_BIT7_0, time_set & 0xff);
        aocec_wr_reg(CEC_TXTIME_4BIT_BIT15_8, (time_set >> 8) & 0x7);
        if(flag)
            hdmi_print(INF, CEC "write 3 bit:0x%x%x \n", aocec_rd_reg(CEC_TXTIME_4BIT_BIT15_8),aocec_rd_reg(CEC_TXTIME_4BIT_BIT7_0));
        break;
        //5 bit
    case 5:
        if(flag)
            hdmi_print(INF, CEC "read 5 bit:0x%x%x \n", aocec_rd_reg(CEC_TXTIME_2BIT_BIT15_8), aocec_rd_reg(CEC_TXTIME_2BIT_BIT7_0));
        aocec_wr_reg(CEC_TXTIME_2BIT_BIT7_0, time_set & 0xff);
        aocec_wr_reg(CEC_TXTIME_2BIT_BIT15_8, (time_set >> 8) & 0x7);
        if(flag)
            hdmi_print(INF, CEC "write 5 bit:0x%x%x \n", aocec_rd_reg(CEC_TXTIME_2BIT_BIT15_8), aocec_rd_reg(CEC_TXTIME_2BIT_BIT7_0));
        break;
        //7 bit
	case 7:
        if(flag)
            hdmi_print(INF, CEC "read 7 bit:0x%x%x \n", aocec_rd_reg(CEC_TXTIME_17MS_BIT15_8), aocec_rd_reg(CEC_TXTIME_17MS_BIT7_0));
        aocec_wr_reg(CEC_TXTIME_17MS_BIT7_0, time_set & 0xff);
        aocec_wr_reg(CEC_TXTIME_17MS_BIT15_8, (time_set >> 8) & 0x7);
        if(flag)
            hdmi_print(INF, CEC "write 7 bit:0x%x%x \n", aocec_rd_reg(CEC_TXTIME_17MS_BIT15_8), aocec_rd_reg(CEC_TXTIME_17MS_BIT7_0));
        break;
    default:
        break;
    }
}
*/

static void ao_cec_timing_set(void)
{
    //Cec arbitration 3/5/7 bit time set.
    //ao_cec_arbit_bit_time_set(3, 0x118, 0);
    //ao_cec_arbit_bit_time_set(5, 0x000, 0);
    //ao_cec_arbit_bit_time_set(7, 0x2aa, 0);
    
    // Program AO CEC's timing parameters to work with 24MHz cec_clk 
    aocec_wr_reg(CEC_CLOCK_DIV_L,                   (240-1)&0xff);
    aocec_wr_reg(CEC_CLOCK_DIV_H,                   ((240-1)>>8)&0xff);
    aocec_wr_reg(CEC_QUIESCENT_25MS_BIT7_0,         0xC4);  // addr=0x20
    aocec_wr_reg(CEC_QUIESCENT_25MS_BIT11_8,        0x09);  // addr=0x21
    aocec_wr_reg(CEC_STARTBITMINL2H_3MS5_BIT7_0,    0x5E);  // addr=0x22
    aocec_wr_reg(CEC_STARTBITMINL2H_3MS5_BIT8,      0x01);  // addr=0x23
    aocec_wr_reg(CEC_STARTBITMAXL2H_3MS9_BIT7_0,    0x86);  // addr=0x24
    aocec_wr_reg(CEC_STARTBITMAXL2H_3MS9_BIT8,      0x01);  // addr=0x25
    aocec_wr_reg(CEC_STARTBITMINH_0MS6_BIT7_0,      0x3C);  // addr=0x26
    aocec_wr_reg(CEC_STARTBITMINH_0MS6_BIT8,        0x00);  // addr=0x27
    aocec_wr_reg(CEC_STARTBITMAXH_1MS0_BIT7_0,      0x64);  // addr=0x28
    aocec_wr_reg(CEC_STARTBITMAXH_1MS0_BIT8,        0x00);  // addr=0x29
    aocec_wr_reg(CEC_STARTBITMINTOTAL_4MS3_BIT7_0,  0xAE);  // addr=0x2A
    aocec_wr_reg(CEC_STARTBITMINTOTAL_4MS3_BIT9_8,  0x01);  // addr=0x2B
    aocec_wr_reg(CEC_STARTBITMAXTOTAL_4MS7_BIT7_0,  0xD6);  // addr=0x2C
    aocec_wr_reg(CEC_STARTBITMAXTOTAL_4MS7_BIT9_8,  0x01);  // addr=0x2D
    aocec_wr_reg(CEC_LOGIC1MINL2H_0MS4_BIT7_0,      0x28);  // addr=0x2E
    aocec_wr_reg(CEC_LOGIC1MINL2H_0MS4_BIT8,        0x00);  // addr=0x2F
    aocec_wr_reg(CEC_LOGIC1MAXL2H_0MS8_BIT7_0,      0x50);  // addr=0x30
    aocec_wr_reg(CEC_LOGIC1MAXL2H_0MS8_BIT8,        0x00);  // addr=0x31
    aocec_wr_reg(CEC_LOGIC0MINL2H_1MS3_BIT7_0,      0x82);  // addr=0x32
    aocec_wr_reg(CEC_LOGIC0MINL2H_1MS3_BIT8,        0x00);  // addr=0x33
    aocec_wr_reg(CEC_LOGIC0MAXL2H_1MS7_BIT7_0,      0xAA);  // addr=0x34
    aocec_wr_reg(CEC_LOGIC0MAXL2H_1MS7_BIT8,        0x00);  // addr=0x35
    aocec_wr_reg(CEC_LOGICMINTOTAL_2MS05_BIT7_0,    0xCD);  // addr=0x36
    aocec_wr_reg(CEC_LOGICMINTOTAL_2MS05_BIT9_8,    0x00);  // addr=0x37
    aocec_wr_reg(CEC_LOGICMAXHIGH_2MS8_BIT7_0,      0x18);  // addr=0x38
    aocec_wr_reg(CEC_LOGICMAXHIGH_2MS8_BIT8,        0x01);  // addr=0x39
    aocec_wr_reg(CEC_LOGICERRLOW_3MS4_BIT7_0,       0x54);  // addr=0x3A
    aocec_wr_reg(CEC_LOGICERRLOW_3MS4_BIT8,         0x01);  // addr=0x3B
    aocec_wr_reg(CEC_NOMSMPPOINT_1MS05,             0x69);  // addr=0x3C
    aocec_wr_reg(CEC_DELCNTR_LOGICERR,              0x35);  // addr=0x3E
    aocec_wr_reg(CEC_TXTIME_17MS_BIT7_0,            0xA4);  // addr=0x40
    aocec_wr_reg(CEC_TXTIME_17MS_BIT15_8,           0x06);  // addr=0x41
    aocec_wr_reg(CEC_TXTIME_2BIT_BIT7_0,            0xF0);  // addr=0x42
    aocec_wr_reg(CEC_TXTIME_2BIT_BIT15_8,           0x01);  // addr=0x43
    aocec_wr_reg(CEC_TXTIME_4BIT_BIT7_0,            0xD0);  // addr=0x44
    aocec_wr_reg(CEC_TXTIME_4BIT_BIT15_8,           0x03);  // addr=0x45
    aocec_wr_reg(CEC_STARTBITNOML2H_3MS7_BIT7_0,    0x72);  // addr=0x46
    aocec_wr_reg(CEC_STARTBITNOML2H_3MS7_BIT8,      0x01);  // addr=0x47
    aocec_wr_reg(CEC_STARTBITNOMH_0MS8_BIT7_0,      0x50);  // addr=0x48
    aocec_wr_reg(CEC_STARTBITNOMH_0MS8_BIT8,        0x00);  // addr=0x49
    aocec_wr_reg(CEC_LOGIC1NOML2H_0MS6_BIT7_0,      0x3C);  // addr=0x4A
    aocec_wr_reg(CEC_LOGIC1NOML2H_0MS6_BIT8,        0x00);  // addr=0x4B
    aocec_wr_reg(CEC_LOGIC0NOML2H_1MS5_BIT7_0,      0x96);  // addr=0x4C
    aocec_wr_reg(CEC_LOGIC0NOML2H_1MS5_BIT8,        0x00);  // addr=0x4D
    aocec_wr_reg(CEC_LOGIC1NOMH_1MS8_BIT7_0,        0xB4);  // addr=0x4E
    aocec_wr_reg(CEC_LOGIC1NOMH_1MS8_BIT8,          0x00);  // addr=0x4F
    aocec_wr_reg(CEC_LOGIC0NOMH_0MS9_BIT7_0,        0x5A);  // addr=0x50
    aocec_wr_reg(CEC_LOGIC0NOMH_0MS9_BIT8,          0x00);  // addr=0x51
    aocec_wr_reg(CEC_LOGICERRLOW_3MS6_BIT7_0,       0x68);  // addr=0x52
    aocec_wr_reg(CEC_LOGICERRLOW_3MS6_BIT8,         0x01);  // addr=0x53
    aocec_wr_reg(CEC_CHKCONTENTION_0MS1,            0x0A);  // addr=0x54
    aocec_wr_reg(CEC_PREPARENXTBIT_0MS05_BIT7_0,    0x05);  // addr=0x56
    aocec_wr_reg(CEC_PREPARENXTBIT_0MS05_BIT8,      0x00);  // addr=0x57
    aocec_wr_reg(CEC_NOMSMPACKPOINT_0MS45,          0x2D);  // addr=0x58
    aocec_wr_reg(CEC_ACK0NOML2H_1MS5_BIT7_0,        0x96);  // addr=0x5A
    aocec_wr_reg(CEC_ACK0NOML2H_1MS5_BIT8,          0x00);  // addr=0x5B
}

static void ao_cec_set(void)
{
    //to do
}

static void ao_cec_hw_reset(void)
{ 
    //init ao cec clk
    //ao_cec_clk_set();
    
    //ao cec software reset
    ao_cec_sw_reset();
    
    ao_cec_timing_set();
        
    // Enable all AO_CEC interrupt sources
    if(!cec_irq_enable_flag)
        cec_enable_irq();

    cec_logic_addr_set(CEC_PLAYBACK_DEVICE_1_ADDR);
}

static int ao_cec_ll_rx( unsigned char *msg, unsigned char *len)
{
    unsigned char i;
    unsigned char data;

    unsigned char n;
    unsigned char *msg_start = msg;
	unsigned int num;
    int rx_msg_length;
    int rx_status;
	
	rx_status = aocec_rd_reg(CEC_RX_MSG_STATUS);
	num = aocec_rd_reg(CEC_RX_NUM_MSG);
	
	printk("rx irq:rx_status:0x%x:: num :0x%x\n", rx_status, num);
	//aml_set_reg32_bits(P_AO_CEC_INTR_CLR, 1, 2, 1);
    if(RX_DONE != rx_status){
		printk("rx irq:!!!RX_DONE\n");
        aocec_wr_reg(CEC_RX_MSG_CMD,  RX_ACK_CURRENT);
        aocec_wr_reg(CEC_RX_MSG_CMD,  RX_NO_OP);
        return -1;
    }
    if(1 != num){
		printk("rx irq:!!!num\n");
        //aocec_wr_reg(CEC_RX_MSG_CMD,  RX_ACK_CURRENT);
        //aocec_wr_reg(CEC_RX_MSG_CMD,  RX_NO_OP);
		aocec_wr_reg(CEC_RX_CLEAR_BUF, 1);
		aml_set_reg32_bits(P_AO_CEC_INTR_CLR, 1, 2, 1);
        return -1;
    }
    rx_msg_length = aocec_rd_reg(CEC_RX_MSG_LENGTH) + 1;

    aocec_wr_reg(CEC_RX_MSG_CMD,  RX_ACK_CURRENT);

    for (i = 0; i < rx_msg_length && i < MAX_MSG; i++) {
        data = aocec_rd_reg(CEC_RX_MSG_0_HEADER +i);
        *msg = data;
        msg++;
    }
    *len = rx_msg_length;
    rx_status = aocec_rd_reg(CEC_RX_MSG_STATUS);

    aocec_wr_reg(CEC_RX_MSG_CMD, RX_NO_OP);
    //aocec_wr_reg(CEC_RX_CLEAR_BUF, 1);
    aml_set_reg32_bits(P_AO_CEC_INTR_CLR, 1, 2, 1);
	cec_hw_reset();

    if(cec_msg_dbg_en  == 1){
        pos = 0;
        pos += sprintf(msg_log_buf + pos, "CEC: rx msg len: %d   dat: ", rx_msg_length);
        for(n = 0; n < rx_msg_length; n++) {
            pos += sprintf(msg_log_buf + pos, "%02x ", msg_start[n]);
        }
        pos += sprintf(msg_log_buf + pos, "\n");
        msg_log_buf[pos] = '\0';
        hdmi_print(INF, CEC "%s", msg_log_buf);
    }
    return rx_status;
}

static int ao_cec_rx_irq_handle(unsigned char *msg, unsigned char *len)
{
    //to do
    return ao_cec_ll_rx(msg, len);
}

static unsigned int ao_cec_intr_stat(void)
{
    return aml_read_reg32(P_AO_CEC_INTR_STAT);
}

// return value: 1: successful      0: error
static int ao_cec_ll_tx(const unsigned char *msg, unsigned char len)
{
    int i;
    unsigned int ret = 0xf;
    unsigned int n;
    unsigned int cnt = 30;

    while(aocec_rd_reg(CEC_TX_MSG_STATUS)){
        msleep(5);
        if(TX_ERROR == aocec_rd_reg(CEC_TX_MSG_STATUS)){
            //aocec_wr_reg(CEC_TX_MSG_CMD, TX_ABORT);
            aocec_wr_reg(CEC_TX_MSG_CMD, TX_NO_OP);
            //cec_hw_reset();
            break;
        }
        if(!(cnt--)){
            hdmi_print(INF, CEC "tx busy time out.\n");
            aocec_wr_reg(CEC_TX_MSG_CMD, TX_ABORT);
            aocec_wr_reg(CEC_TX_MSG_CMD, TX_NO_OP);
            break;
        }
    }

    for (i = 0; i < len; i++)
    {
        aocec_wr_reg(CEC_TX_MSG_0_HEADER + i, msg[i]);
    }
    aocec_wr_reg(CEC_TX_MSG_LENGTH, len-1);
    aocec_wr_reg(CEC_TX_MSG_CMD, RX_ACK_CURRENT);

    if(cec_msg_dbg_en  == 1) {
        pos = 0;
        pos += sprintf(msg_log_buf + pos, "CEC: tx msg len: %d   dat: ", len);
        for(n = 0; n < len; n++) {
            pos += sprintf(msg_log_buf + pos, "%02x ", msg[n]);
        }
        pos += sprintf(msg_log_buf + pos, "\n");

        msg_log_buf[pos] = '\0';
        printk("%s", msg_log_buf);
    }
    return ret;
}

static int ao_cec_ll_tx_polling(const unsigned char *msg, unsigned char len)
{
    int i;
    unsigned int ret = 0xf;
    unsigned int n;
	unsigned int j = 30;

    while( aocec_rd_reg(CEC_TX_MSG_STATUS)){
        if(TX_ERROR == aocec_rd_reg(CEC_TX_MSG_STATUS)){
            //aocec_wr_reg(CEC_TX_MSG_CMD, TX_ABORT);
            aocec_wr_reg(CEC_TX_MSG_CMD, TX_NO_OP);
            //cec_hw_reset();
            break;
        }
        if(!(j--)){
            hdmi_print(INF, CEC "tx busy time out.\n");
            aocec_wr_reg(CEC_TX_MSG_CMD, TX_ABORT);
            aocec_wr_reg(CEC_TX_MSG_CMD, TX_NO_OP);
            break;
        }
        msleep(5);
    }

    aml_set_reg32_bits(P_AO_CEC_INTR_MASKN, 0x0, 1, 1);
    for (i = 0; i < len; i++)
    {
        aocec_wr_reg(CEC_TX_MSG_0_HEADER + i, msg[i]);
    }
    aocec_wr_reg(CEC_TX_MSG_LENGTH, len-1);
    aocec_wr_reg(CEC_TX_MSG_CMD, RX_ACK_CURRENT);

    j = 30;
    while((TX_DONE != aocec_rd_reg(CEC_TX_MSG_STATUS)) && (j--)){
        if(TX_ERROR == aocec_rd_reg(CEC_TX_MSG_STATUS))
            break;
		msleep(5);
	}

    ret = aocec_rd_reg(CEC_TX_MSG_STATUS);

    if(ret == TX_DONE)
        ret = 1;
    else
        ret = 0;
    aocec_wr_reg(CEC_TX_MSG_CMD, TX_NO_OP);
    aml_set_reg32_bits(P_AO_CEC_INTR_MASKN, 1, 1, 1);
    
    msleep(100);
    if(cec_msg_dbg_en  == 1) {
        pos = 0;
        pos += sprintf(msg_log_buf + pos, "CEC: tx msg len: %d   dat: ", len);
        for(n = 0; n < len; n++) {
            pos += sprintf(msg_log_buf + pos, "%02x ", msg[n]);
        }
        pos += sprintf(msg_log_buf + pos, "\nCEC: tx state: %d\n", ret);
        msg_log_buf[pos] = '\0';
        printk("%s", msg_log_buf);
    }
    return ret;
}

static void ao_cec_tx_irq_handle(void)
{
    unsigned tx_status = aocec_rd_reg(CEC_TX_MSG_STATUS);
    printk("tx_status:0x%x\n", tx_status);
    switch(tx_status){
    case TX_DONE:
      aocec_wr_reg(CEC_TX_MSG_CMD, TX_NO_OP);
      break;
    case TX_BUSY:
        aocec_wr_reg(CEC_TX_MSG_CMD, TX_ABORT);
        aocec_wr_reg(CEC_TX_MSG_CMD, TX_NO_OP);
        break;
    case TX_ERROR:
        cec_hw_reset();
        //aocec_wr_reg(CEC_TX_MSG_CMD, TX_ABORT);
        //aocec_wr_reg(CEC_TX_MSG_CMD, TX_NO_OP);
        break;
    default:
        break;
    }
    aml_set_reg32_bits(P_AO_CEC_INTR_CLR, 1, 1, 1);
}

static void ao_cec_polling_online_dev(int log_addr, int *bool)
{
    unsigned long r;
    unsigned char msg[1];

    cec_global_info.my_node_index = log_addr;
    msg[0] = (log_addr<<4) | log_addr;

    aocec_wr_reg(CEC_LOGICAL_ADDR0, (0x1 << 4) | 0x4);
    hdmi_print(INF, CEC "CEC_LOGICAL_ADDR0:0x%lx\n",aocec_rd_reg(CEC_LOGICAL_ADDR0));
    r = cec_ll_tx_polling(msg, 1);
    cec_hw_reset();

    if (r == 0) {
        *bool = 0;
    }else{
        memset(&(cec_global_info.cec_node_info[log_addr]), 0, sizeof(cec_node_info_t));
        cec_global_info.cec_node_info[log_addr].dev_type = cec_log_addr_to_dev_type(log_addr);
    	  *bool = 1;
    }
    if(*bool == 0) {
        aocec_wr_reg(CEC_LOGICAL_ADDR0, (0x1 << 4) | log_addr);
    }

}

#else
//****************************************************HDMI CEC2.0**************************************************
static void hdmi_cec_disable_irq(void)
{
    //to do
    // Configure HDMI CEC2.0 interrupts
    //data32  = 0;
    //data32 |= (0    << 6);  // [  6] wakeup
    //data32 |= (0    << 5);  // [  5] error_follower
    //data32 |= (0    << 4);  // [  4] error_initiator
    //data32 |= (0    << 3);  // [  3] arb_lost
    //data32 |= (0    << 2);  // [  2] nack
    //data32 |= (0    << 1);  // [  1] eom
    //data32 |= (0    << 0);  // [  0] done
    //hdmitx_wr_reg(HDMITX_DWC_CEC_INTR_MASK, 0);
    cec_irq_enable_flag = 0;
    //hdmi_print(INF, CEC "disable:int mask:0x%x\n", hdmitx_rd_reg(HDMITX_DWC_CEC_INTR_MASK));
}

static void hdmi_cec_enable_irq(void)
{
    cec_irq_enable_flag = 1;
    //hdmi_print(INF, CEC "enable:int mask:0x%x\n", hdmitx_rd_reg(HDMITX_DWC_CEC_INTR_MASK));
}

static void hdmi_cec_clk_set(void)
{
    //HDMI IP CEC clock = 24M/732=32786.9Hz
    //unsigned long data32;
    //data32  = 0;
    //data32 |= 0         << 16;  // [17:16] clk_sel: 0=oscin; 1=slow_oscin; 2=fclk_div3; 3=fclk_div5.
    //data32 |= 1         << 15;  // [   15] clk_en
    //data32 |= (732-1)   << 0;   // [13: 0] clk_div 
    aml_set_reg32_bits(P_HHI_32K_CLK_CNTL, 1, 15, 1);
    aml_set_reg32_bits(P_HHI_32K_CLK_CNTL, 0, 16, 2);
    aml_set_reg32_bits(P_HHI_32K_CLK_CNTL, (732 - 1), 0, 14);
}

static void hdmi_cec_sw_reset(void)
{
    //to do
}

static void hdmi_cec_logic_addr_set(enum _cec_log_dev_addr_e logic_addr)
{
    //tmp debug:set addr 4 for G9TV CEC. To do.
    hdmitx_wr_reg(HDMITX_DWC_CEC_LADD_LOW, (1<<4));
    hdmitx_wr_reg(HDMITX_DWC_CEC_LADD_HIGH, 0);
}

static void hdmi_cec_timing_set(void)
{
    //to do
}
/*
static void hdmi_cec_ctrl(void)
{
    //to do;
    //HDMITX_DWC_CEC_CTRL
    hdmitx_wr_reg(HDMITX_DWC_CEC_CTRL, hdmitx_rd_reg(HDMITX_DWC_CEC_CTRL) | 0x1);
}


static void hdmi_cec_mask(void)
{
    //to do;
    //HDMITX_DWC_CEC_INTR_MASK
}

static int hdmi_cec_lock(cec_rw_e flag, int value)
{
    int ret;
    switch(flag)
    {
    case CEC_READ:
        ret = hdmitx_rd_reg(HDMITX_DWC_CEC_LOCK_BUF);
        break;
    case CEC_WRITE:
        hdmitx_wr_reg(HDMITX_DWC_CEC_LOCK_BUF, value);
        ret = hdmitx_rd_reg(HDMITX_DWC_CEC_LOCK_BUF);
        break;
    default:
        ret = hdmitx_rd_reg(HDMITX_DWC_CEC_LOCK_BUF);
        break;
    }
    return ret;
}
static void _hdmi_clean_buf_(unsigned int offset)
{
    int i;
    if(!offset)
        return;
    for(i = 0; i < MAX_MSG; i++)
    {
        hdmitx_wr_reg(offset +i, 0);
    }
}

static void hdmi_clean_buf(cec_rw_e flag)
{
    unsigned int offset;
    switch(flag)
    {
    case CEC_READ:
        offset = HDMITX_DWC_CEC_RX_DATA00;
        break;
    case CEC_WRITE:
        offset = HDMITX_DWC_CEC_TX_DATA00; 
        break;
    default:
        break;
    }
   _hdmi_clean_buf_(offset);
}

*/
static void hdmi_cec_set(void)
{
    unsigned long data32;
    // Configure CEC interrupts
    data32  = 0;
    data32 |= (0    << 6);  // [  6] wakeup
    data32 |= (0    << 5);  // [  5] error_follower
    data32 |= (0    << 4);  // [  4] error_initiator
    data32 |= (0    << 3);  // [  3] arb_lost
    data32 |= (0    << 2);  // [  2] nack
    data32 |= (0    << 1);  // [  1] eom
    data32 |= (0    << 0);  // [  0] done
    hdmitx_wr_reg(HDMITX_DWC_CEC_INTR_MASK,  data32);
    
    data32  = 0;
    data32 |= (0    << 6);  // [  6] wakeup
    data32 |= (0    << 5);  // [  5] error_follower
    data32 |= (0    << 4);  // [  4] error_initiator
    data32 |= (0    << 3);  // [  3] arb_lost
    data32 |= (0    << 2);  // [  2] nack
    data32 |= (0    << 1);  // [  1] eom
    data32 |= (0    << 0);  // [  0] done
    hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_CEC_STAT0,  data32);

    data32  = 0;
    data32 |= (0    << 1);  // [  1] mute_wakeup_interrupt
    data32 |= (0    << 0);  // [  0] mute_all_interrupt
    hdmitx_wr_reg(HDMITX_DWC_IH_MUTE,    data32);

    data32  = 0;
    data32 |= (1    << 2);  // [  2] hpd_fall_intr
    data32 |= (1    << 1);  // [  1] hpd_rise_intr
    data32 |= (1    << 0);  // [  0] core_intr
    hdmitx_wr_reg(HDMITX_TOP_INTR_MASKN, data32);

    // Mute all interrupts except CEC related
    //hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_FC_STAT0,      0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_FC_STAT1,      0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_FC_STAT2,      0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_AS_STAT0,      0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_PHY_STAT0,     0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_I2CM_STAT0,    0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_VP_STAT0,      0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_I2CMPHY_STAT0, 0xff);

    // Clear all interrupts
    //hdmitx_wr_reg(HDMITX_DWC_IH_FC_STAT0,       0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_FC_STAT1,       0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_FC_STAT2,       0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_AS_STAT0,       0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_PHY_STAT0,      0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_I2CM_STAT0,     0xff);
    hdmitx_wr_reg(HDMITX_DWC_IH_CEC_STAT0,      0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_VP_STAT0,       0xff);
    //hdmitx_wr_reg(HDMITX_DWC_IH_I2CMPHY_STAT0,  0xff);
    //hdmitx_wr_reg(HDMITX_DWC_A_APIINTCLR,       0xff);
    // [2]      hpd_fall
    // [1]      hpd_rise
    // [0]      core_intr_rise
    hdmitx_wr_reg(HDMITX_TOP_INTR_STAT_CLR, 0x00000007);
}

static void hdmi_cec_hw_reset(void)
{ 
    
    // Enable HDMI IP CEC2.0 interrupt sources
    if(!cec_irq_enable_flag)
        cec_enable_irq();

    //cec_logic_addr_set(CEC_PLAYBACK_DEVICE_1_ADDR);
}

static int hdmi_cec_ll_rx( unsigned char *msg, unsigned char *len)
{
    unsigned char i;
    unsigned char data;
    unsigned char n;
    unsigned char *msg_start = msg;
    int rx_status = 1;
    int rx_msg_length;

    // Check received message
    
    hdmitx_wr_reg(HDMITX_DWC_CEC_LOCK_BUF, 1);
    rx_msg_length = aocec_rd_reg(HDMITX_DWC_CEC_RX_CNT);
    
    for (i = 0; i < rx_msg_length && i < MAX_MSG; i++) {
        data = hdmitx_rd_reg(HDMITX_DWC_CEC_RX_DATA00+i);
        *msg = data;
        msg++;
    }
    
    hdmitx_wr_reg(HDMITX_DWC_CEC_LOCK_BUF, 0);

    //if(cec_lock(CEC_READ, 0))
    //    cec_lock(CEC_WRITE, 0);
    //
    //clean_buf(CEC_READ);
    
    if(cec_msg_dbg_en  == 1)
        pos = 0;
        pos += sprintf(msg_log_buf + pos, "CEC: rx msg len: %d   dat: ", rx_msg_length);
        for(n = 0; n < rx_msg_length; n++) {
            pos += sprintf(msg_log_buf + pos, "%02x ", msg_start[n]);
        }
        pos += sprintf(msg_log_buf + pos, "\n");
        msg_log_buf[pos] = '\0';
        hdmi_print(INF, CEC "%s", msg_log_buf);

    return rx_status;
}

static int hdmi_cec_rx_irq_handle(unsigned char *msg, unsigned char *len)
{
    unsigned long data32;
    int ret;
    int int_stat_main;
    int int_stat;
    
    ret = 0;
    data32  = hdmitx_rd_reg(HDMITX_TOP_INTR_STAT);
    hdmitx_wr_reg(HDMITX_TOP_INTR_STAT_CLR, data32);    // clear interrupts in HDMITX_TOP module
    
    if (data32 & (1 << 0))
    {  // core_intr
        int_stat_main = hdmitx_rd_reg(HDMITX_DWC_IH_DECODE);
        if (int_stat_main)
        {
            int_stat = hdmitx_rd_reg(HDMITX_DWC_IH_CEC_STAT0);
            hdmitx_wr_reg(HDMITX_DWC_IH_CEC_STAT0, int_stat);  // Clear ih_cec_stat0 in HDMITX_DWC
            if (int_stat == 0x01) {
                //HDMITX cec_done Interrupt Process_Irq
                ret = cec_ll_rx(msg, len);
            } else {
                hdmi_print(INF, CEC "Error: HDMITX CEC interrupt error, expecting DONE\n");
            }
        } else {
            hdmi_print(INF, CEC "Error: Unintended interrupt seen at HDMITX DWC\n");
        }
    } else {
        hdmi_print(INF, CEC "[Error: Unintended interrupt seen at HDMITX TOP\n");
    }
    
    return ret;
}

static unsigned int hdmi_cec_intr_stat(void)
{
    return hdmitx_rd_reg(HDMITX_DWC_IH_CEC_STAT0);
}

// return value: 1: successful      0: error
static int hdmi_cec_ll_tx(const unsigned char *msg, unsigned char len)
{
    int i;
    unsigned int ret = 1;
    unsigned int n;

    hdmitx_wr_reg(HDMITX_DWC_CEC_TX_CNT, len);
    for (i = 0; i < len; i++) {
        hdmitx_wr_reg(HDMITX_DWC_CEC_TX_DATA00+i, msg[i]);
    }
    hdmitx_wr_reg(HDMITX_DWC_CEC_CTRL,   hdmitx_rd_reg(HDMITX_DWC_CEC_CTRL) | (1<<0));

    if(cec_msg_dbg_en  == 1) {
        pos = 0;
        pos += sprintf(msg_log_buf + pos, "CEC: tx msg len: %d   dat: ", len);
        for(n = 0; n < len; n++) {
            pos += sprintf(msg_log_buf + pos, "%02x ", msg[n]);
        }
        pos += sprintf(msg_log_buf + pos, "\n");

        msg_log_buf[pos] = '\0';
        printk("%s", msg_log_buf);
    }
    return ret;
}

static int hdmi_cec_ll_tx_polling(const unsigned char *msg, unsigned char len)
{
    //to do
    return 1;
}

static int hdmi_cec_tx_irq_handle(void)
{
    unsigned long data32;
    int ret;
    int int_stat_main;
    int int_stat;
    
    ret = 0;
    data32  = hdmitx_rd_reg(HDMITX_TOP_INTR_STAT);
    hdmitx_wr_reg(HDMITX_TOP_INTR_STAT_CLR, data32);    // clear interrupts in HDMITX_TOP module
    
    if (data32 & (1 << 0))
    {  // core_intr
        int_stat_main = hdmitx_rd_reg(HDMITX_DWC_IH_DECODE);
        if (int_stat_main)
        {
            int_stat = hdmitx_rd_reg(HDMITX_DWC_IH_CEC_STAT0);
            hdmitx_wr_reg(HDMITX_DWC_IH_CEC_STAT0, int_stat);  // Clear ih_cec_stat0 in HDMITX_DWC
            if (int_stat == 0x02) {
                //HDMITX cec_eom Interrupt Process_Irq
                hdmi_print(INF, CEC "tx successful\n");
            } else {
                hdmi_print(INF, CEC "Error: HDMITX CEC interrupt error, expecting EOM\n\n");
            }
        } else {
            hdmi_print(INF, CEC "Error: Unintended interrupt seen at HDMITX DWC\n");
        }
    } else {
        hdmi_print(INF, CEC "[Error: Unintended interrupt seen at HDMITX TOP\n");
    }
    
    return ret;
}

static void hdmi_cec_polling_online_dev(int log_addr, int *bool)
{
    //to do
}

#endif

//******************************************************************************************************
void cec_disable_irq(void)
{
#ifdef AO_CEC
    ao_cec_disable_irq();
#else
    hdmi_cec_disable_irq();
#endif
}

void cec_enable_irq(void)
{
#ifdef AO_CEC
    ao_cec_enable_irq();
#else
    hdmi_cec_enable_irq();
#endif
}

// 0xc8100014              
void cec_pinmux_set(cec_pinmux_set_e cnt, int vaule)
{
    //To do. gpioao_8/9
    switch(cnt)
    {
    case JTAG_TMS:
        break;
    case HDMI_CEC_AO:
        //pm_gpioAO_8_cec bit[17]
        aml_set_reg32_bits(P_AO_RTI_PIN_MUX_REG, 1, 17, 1);
        //pm_gpioAO_9_cec bit[27]
        //aml_set_reg32_bits(P_AO_RTI_PIN_MUX_REG, 1, 27, 1);
        
        //disable HDMITX_CEC on gpioao_8 pin
        // out: pm_hdmitx_cec_gpioAO_8          = pin_mux_reg[14];
        // in:  pm_hdmitx_cec_gpioAO_8          = pin_mux_reg11[26];
        aml_set_reg32_bits(P_AO_RTI_PIN_MUX_REG, 0, 14, 1);
        aml_set_reg32_bits(P_PERIPHS_PIN_MUX_11, 0, 26, 1); 
        //disable JTAG on gpioao_8 pin for aocec
        // Secureregister[1:0] = 0;
        aml_set_reg32_bits(P_AO_SECURE_REG1, 0, 0, 2); 
        //disable HDMIRX_CEC on gpioao_8 pin
        // out: pm_hdmitx_cec_gpioAO_8          = pin_mux_reg[16];
        // in:  pm_hdmitx_cec_gpioAO_8          = pin_mux_reg11[28];
        aml_set_reg32_bits(P_AO_RTI_PIN_MUX_REG, 0, 16, 1);
        aml_set_reg32_bits(P_AO_RTI_PIN_MUX_REG, 0, 28, 1); 
        break;
    case HDMITX_CEC:
        //gpioao_8
        // out: pm_hdmitx_cec_gpioAO_8          = pin_mux_reg[14];
        // in:  pm_hdmitx_cec_gpioAO_8          = pin_mux_reg11[26];
        aml_set_reg32_bits(P_AO_RTI_PIN_MUX_REG, 1, 14, 1);
        aml_set_reg32_bits(P_PERIPHS_PIN_MUX_11, 1, 26, 1);        
        
        //gpioao_9
        // out: pm_hdmitx_cec_gpioAO_9          = pin_mux_reg[13];
        // in:  pm_hdmitx_cec_gpioAO_9          = pin_mux_reg11[25];
        //aml_set_reg32_bits(P_AO_RTI_PIN_MUX_REG, 1, 13, 1);
        //aml_set_reg32_bits(P_PERIPHS_PIN_MUX_11, 1, 25, 1);
        break;
    case HDMIRX_CEC:
        break;
    default:
        //GPIOAO_9
        break;
    }
}

void cec_clk_set(void)
{
#ifdef AO_CEC
    ao_cec_clk_set();
#else
    hdmi_cec_clk_set();
#endif
}

void cec_sw_reset(void)
{
#ifdef AO_CEC
    ao_cec_sw_reset();
#else
    hdmi_cec_sw_reset();
#endif
}

void cec_logic_addr_set(enum _cec_log_dev_addr_e logic_addr)
{
#ifdef AO_CEC
    ao_cec_logic_addr_set(logic_addr);
#else
    hdmi_cec_logic_addr_set(logic_addr);
#endif
}

void cec_timing_set(void)
{
#ifdef AO_CEC
    ao_cec_timing_set();
#else
    hdmi_cec_timing_set();
#endif
}

void cec_phyaddr_set(int phyaddr)
{
    //to do
}

void cec_set(void)
{
#ifdef AO_CEC
    ao_cec_set();
#else
    hdmi_cec_set();
#endif
}

void cec_hw_reset(void)
{ 
#ifdef AO_CEC
    ao_cec_hw_reset();
#else
    hdmi_cec_hw_reset();
#endif
    hdmi_print(INF, CEC "hw reset!\n");
}

void cec_hw_init(void)
{
    cec_clk_set();
    
    cec_pinmux_set(HDMI_CEC_AO, 0);
    
    //init cec clk
    cec_clk_set();
    
    //cec software reset
    cec_sw_reset();
    
    cec_timing_set();
    
    cec_set();

    // Enable CEC interrupt sources
    cec_enable_irq();

    cec_logic_addr_set(CEC_PLAYBACK_DEVICE_1_ADDR);
    
    hdmi_print(INF, CEC "cec hw init!\n");
}

int cec_ll_rx( unsigned char *msg, unsigned char *len)
{
#ifdef AO_CEC
    return ao_cec_ll_rx(msg, len);
#else
    return hdmi_cec_ll_rx(msg, len);
#endif
}

// Return value: 0: fail    1: success
int cec_ll_tx(const unsigned char *msg, unsigned char len)
{
    int ret = 0;
    if(!cec_irq_enable_flag)
        return 2;
        
    mutex_lock(&cec_mutex);
#ifdef AO_CEC
    ao_cec_ll_tx(msg, len);
#else
    hdmi_cec_ll_tx(msg, len);
#endif
    mutex_unlock(&cec_mutex);
    
    return ret;
}

unsigned int cec_intr_stat(void)
{
    //to do
#ifdef AO_CEC
    return ao_cec_intr_stat();
#else
    return hdmi_cec_intr_stat();
#endif
}

int cec_rx_irq_handle(unsigned char *msg, unsigned char *len)
{
    //to do
#ifdef AO_CEC
    return ao_cec_rx_irq_handle(msg, len);
#else
    return hdmi_cec_rx_irq_handle(msg, len);
#endif
}

int cec_ll_tx_polling(const unsigned char *msg, unsigned char len)
{
#ifdef AO_CEC
    return ao_cec_ll_tx_polling(msg, len);
#else
    return hdmi_cec_ll_tx_polling(msg, len);
#endif
}

void cec_tx_irq_handle(void)
{
#ifdef AO_CEC
    ao_cec_tx_irq_handle();
#else
    hdmi_cec_tx_irq_handle();
#endif
}

void cec_polling_online_dev(int log_addr, int *bool)
{
#ifdef AO_CEC
    ao_cec_polling_online_dev(log_addr, bool);
#else
    hdmi_cec_polling_online_dev(log_addr, bool);
#endif
    hdmi_print(INF, CEC "CEC: poll online logic device: 0x%x BOOL: %d\n", log_addr, *bool);
}


// DELETE LATER, TEST ONLY
void cec_test_(unsigned int cmd)
{
    
}

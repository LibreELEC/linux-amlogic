/*
 * AMLOGIC Smart card driver.
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
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#ifdef CONFIG_ARCH_ARC700
#include <asm/arch/am_regs.h>
#else
#include "linux/clk.h"
#include <mach/am_regs.h>
#endif
#include <linux/poll.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
#include <mach/mod_gate.h>
#include <mach/power_gate.h>
#endif

#include <linux/version.h>
#if LINUX_VERSION_CODE>= KERNEL_VERSION(3,10,0)
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/amlogic/amsmc.h>
#else
#include <linux/amsmc.h>
#endif

#include "smc_reg.h"

#define DRIVER_NAME "amsmc"
#define MODULE_NAME "amsmc"
#define DEVICE_NAME "amsmc"
#define CLASS_NAME  "amsmc-class"

#define FILE_DEBUG

#ifdef FILE_DEBUG
#include <linux/fs.h>
#define DEBUG_FILE_NAME     "/storage/external_storage/debug.smc"
static struct file* debug_filp = NULL;
static loff_t debug_file_pos = 0;

void debug_write(const char __user *buf, size_t count)
{
    mm_segment_t old_fs;

    if (!debug_filp)
        return;

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    if (count != vfs_write(debug_filp, buf, count, &debug_file_pos)) {
        printk("Failed to write debug file\n");
    }
    set_fs(old_fs);

    return;
}
void open_debug(void) {
    debug_filp = filp_open(DEBUG_FILE_NAME, O_WRONLY, 0);
    if (IS_ERR(debug_filp)) {
        printk("smartcard: open debug file failed\n");
        debug_filp = NULL;
    }else {
		printk("smc: debug file[%s] open.\n", DEBUG_FILE_NAME);
    }
}
void close_debug(void) {
    if (debug_filp) {
        filp_close(debug_filp, current->files);
        debug_filp = NULL;
        debug_file_pos = 0;
    }
	printk("smc: debug file close.\n");
}
#endif

#ifdef FILE_DEBUG
char dbuf[512*2];
int cnt;
#define pr_dbg(fmt, args...) do { if(smc_debug>0) {cnt = sprintf(dbuf, "Smartcard: " fmt, ## args);debug_write(dbuf, cnt);}} while(0)
#define Fpr(a...)            do { if(smc_debug>1) {cnt = sprintf(dbuf, a);debug_write(dbuf, cnt);}} while(0)
#define Ipr                  Fpr
#else
#if 1
#define pr_dbg(fmt, args...) do { if(smc_debug>0) printk("Smartcard: " fmt, ## args);} while(0)
#define Fpr(a...)            do { if(smc_debug>1) printk(a);} while(0)
#define Ipr                  Fpr
#else
#define pr_dbg(fmt, args...)
#define Fpr(a...)
#endif
#endif

#define pr_error(fmt, args...) printk(KERN_ERR "Smartcard: " fmt, ## args)


MODULE_PARM_DESC(smc0_irq, "\n\t\t Irq number of smartcard0");
static int smc0_irq = -1;
module_param(smc0_irq, int, S_IRUGO);

MODULE_PARM_DESC(smc0_reset, "\n\t\t Reset GPIO pin of smartcard0");
static int smc0_reset = -1;
module_param(smc0_reset, int, S_IRUGO);

MODULE_PARM_DESC(smc_debug, "\n\t\t more debug info");
static int smc_debug = 0;
module_param(smc_debug, int, 0644);

MODULE_PARM_DESC(atr_delay, "\n\t\t atr delay");
static int atr_delay = 0;
module_param(atr_delay, int, 0644);

MODULE_PARM_DESC(atr_holdoff, "\n\t\t atr_holdoff");
static int atr_holdoff = 1;
module_param(atr_holdoff, int, 0644);

MODULE_PARM_DESC(cwt_det_en, "\n\t\t cwt_det_en");
static int cwt_det_en = 1;
module_param(cwt_det_en, int, 0644);

MODULE_PARM_DESC(clock_source, "\n\t\t clock_source");
static int clock_source = 0;
module_param(clock_source, int, 0644);

#define NO_HOT_RESET
//#define DISABLE_RECV_INT
#define ATR_FROM_INT
#define SW_INVERT
#define SMC_FIQ

#ifdef CONFIG_AM_SMARTCARD_GPIO_FOR_DET
#define DET_FROM_PIO
#endif
#ifdef CONFIG_AM_SMARTCARD_GPIO_FOR_RST
#define RST_FROM_PIO
#endif

#ifndef  CONFIG_MESON_ARM_GIC_FIQ
#undef SMC_FIQ
#endif

#ifdef SMC_FIQ
#include <plat/fiq_bridge.h>
#ifndef ATR_FROM_INT
#define ATR_FROM_INT
#endif
#endif

#ifdef SMC_FIQ
#define RECV_BUF_SIZE     1024
#define SEND_BUF_SIZE     1024
#else
#define RECV_BUF_SIZE     512
#define SEND_BUF_SIZE     512
#endif

#define RESET_ENABLE      (smc->reset_level)  //reset
#define RESET_DISABLE     (!smc->reset_level) //dis-reset

typedef enum sc_type {
	SC_DIRECT,
	SC_INVERSE,
} sc_type_t;

typedef struct {
	int                id;
	struct device     *dev;
	struct platform_device *pdev;
	int                init;
	int                used;
	int                cardin;
	int                active;
	struct mutex       lock;
	spinlock_t         slock;
	wait_queue_head_t  rd_wq;
	wait_queue_head_t  wr_wq;
	int                recv_start;
	int                recv_count;
	int                send_start;
	int                send_count;
	char               recv_buf[RECV_BUF_SIZE];
	char               send_buf[SEND_BUF_SIZE];
	struct am_smc_param param;
	struct am_smc_atr   atr;

	u32                enable_pin;
#define SMC_ENABLE_PIN_NAME "smc:ENABLE"
	int 			   enable_level;

	u32 			   enable_5v3v_pin;
#define SMC_ENABLE_5V3V_PIN_NAME "smc:5V3V"
	int 			   enable_5v3v_level;

	int                (*reset)(void *, int);
	u32                irq_num;
	int                reset_level;

	u32              pin_clk_pinmux_reg;
	u32              pin_clk_pinmux_bit;
	u32              pin_clk_pin;
#define SMC_CLK_PIN_NAME "smc:PINCLK"
	u32              pin_clk_oen_reg;
	u32              pin_clk_out_reg;
	u32              pin_clk_bit;

#ifdef SW_INVERT
	int                atr_mode;
	sc_type_t          sc_type;
#endif

#ifdef SMC_FIQ
	int 			   recv_end;
	int 			   send_end;
	bridge_item_t      smc_fiq_bridge;
#endif

#ifdef DET_FROM_PIO
	u32                detect_pin;
#define SMC_DETECT_PIN_NAME "smc:DETECT"
#endif
#ifdef RST_FROM_PIO
		u32 		   reset_pin;
#define SMC_RESET_PIN_NAME "smc:RESET"
#endif

	int                detect_invert;

	struct pinctrl	   *pinctrl;
} smc_dev_t;

#define SMC_DEV_NAME     "smc"
#define SMC_CLASS_NAME   "smc-class"
#define SMC_DEV_COUNT    1

#define SMC_READ_REG(a)           READ_MPEG_REG(SMARTCARD_##a)
#define SMC_WRITE_REG(a,b)        WRITE_MPEG_REG(SMARTCARD_##a,b)

static struct mutex smc_lock;
static int          smc_major;
static smc_dev_t    smc_dev[SMC_DEV_COUNT];
static int ENA_GPIO_PULL = 1;

#ifdef SW_INVERT
static const unsigned char inv_table[256] = {
	0xFF, 0x7F, 0xBF, 0x3F, 0xDF, 0x5F, 0x9F, 0x1F, 0xEF, 0x6F, 0xAF, 0x2F, 0xCF, 0x4F, 0x8F, 0x0F,
	0xF7, 0x77, 0xB7, 0x37, 0xD7, 0x57, 0x97, 0x17, 0xE7, 0x67, 0xA7, 0x27, 0xC7, 0x47, 0x87, 0x07,
	0xFB, 0x7B, 0xBB, 0x3B, 0xDB, 0x5B, 0x9B, 0x1B, 0xEB, 0x6B, 0xAB, 0x2B, 0xCB, 0x4B, 0x8B, 0x0B,
	0xF3, 0x73, 0xB3, 0x33, 0xD3, 0x53, 0x93, 0x13, 0xE3, 0x63, 0xA3, 0x23, 0xC3, 0x43, 0x83, 0x03,
	0xFD, 0x7D, 0xBD, 0x3D, 0xDD, 0x5D, 0x9D, 0x1D, 0xED, 0x6D, 0xAD, 0x2D, 0xCD, 0x4D, 0x8D, 0x0D,
	0xF5, 0x75, 0xB5, 0x35, 0xD5, 0x55, 0x95, 0x15, 0xE5, 0x65, 0xA5, 0x25, 0xC5, 0x45, 0x85, 0x05,
	0xF9, 0x79, 0xB9, 0x39, 0xD9, 0x59, 0x99, 0x19, 0xE9, 0x69, 0xA9, 0x29, 0xC9, 0x49, 0x89, 0x09,
	0xF1, 0x71, 0xB1, 0x31, 0xD1, 0x51, 0x91, 0x11, 0xE1, 0x61, 0xA1, 0x21, 0xC1, 0x41, 0x81, 0x01,
	0xFE, 0x7E, 0xBE, 0x3E, 0xDE, 0x5E, 0x9E, 0x1E, 0xEE, 0x6E, 0xAE, 0x2E, 0xCE, 0x4E, 0x8E, 0x0E,
	0xF6, 0x76, 0xB6, 0x36, 0xD6, 0x56, 0x96, 0x16, 0xE6, 0x66, 0xA6, 0x26, 0xC6, 0x46, 0x86, 0x06,
	0xFA, 0x7A, 0xBA, 0x3A, 0xDA, 0x5A, 0x9A, 0x1A, 0xEA, 0x6A, 0xAA, 0x2A, 0xCA, 0x4A, 0x8A, 0x0A,
	0xF2, 0x72, 0xB2, 0x32, 0xD2, 0x52, 0x92, 0x12, 0xE2, 0x62, 0xA2, 0x22, 0xC2, 0x42, 0x82, 0x02,
	0xFC, 0x7C, 0xBC, 0x3C, 0xDC, 0x5C, 0x9C, 0x1C, 0xEC, 0x6C, 0xAC, 0x2C, 0xCC, 0x4C, 0x8C, 0x0C,
	0xF4, 0x74, 0xB4, 0x34, 0xD4, 0x54, 0x94, 0x14, 0xE4, 0x64, 0xA4, 0x24, 0xC4, 0x44, 0x84, 0x04,
	0xF8, 0x78, 0xB8, 0x38, 0xD8, 0x58, 0x98, 0x18, 0xE8, 0x68, 0xA8, 0x28, 0xC8, 0x48, 0x88, 0x08,
	0xF0, 0x70, 0xB0, 0x30, 0xD0, 0x50, 0x90, 0x10, 0xE0, 0x60, 0xA0, 0x20, 0xC0, 0x40, 0x80, 0x00
};
#endif /*SW_INVERT*/

#define dump(b, l) do { \
	int i; \
	pr_dbg("dump: "); \
	for(i=0; i<(l); i++) \
		pr_dbg("%02x ", *(((unsigned char*)(b))+i)); \
	pr_dbg("\n"); \
	}while(0)

static int _gpio_out(unsigned int gpio, int val, const char *owner);

static ssize_t show_gpio_pull(struct class *class, struct class_attribute *attr,	char *buf)
{
	if(ENA_GPIO_PULL > 0)
		return sprintf(buf, "%s\n","enable GPIO pull low");
	else
		return sprintf(buf, "%s\n","disable GPIO pull low");
}

static ssize_t set_gpio_pull(struct class *class, struct class_attribute *attr,	const char *buf, size_t count)
{
	unsigned int dbg;
	ssize_t r;

	r = sscanf(buf, "%d", &dbg);
	if (r != 1)
		return -EINVAL;

	ENA_GPIO_PULL = dbg;
	printk("Smartcard the ENA_GPIO_PULL is:%d.\n", ENA_GPIO_PULL);
	return count;
}

static ssize_t show_5v3v(struct class *class, struct class_attribute *attr,	char *buf)
{
	smc_dev_t *smc = NULL;
	int enable_5v3v = 0;

	mutex_lock(&smc_lock);
	smc = &smc_dev[0];
	enable_5v3v = smc->enable_5v3v_level;
	mutex_unlock(&smc_lock);

	return sprintf(buf, "5v3v_pin level=%d\n", enable_5v3v);
}

static ssize_t store_5v3v(struct class *class, struct class_attribute *attr,	const char *buf, size_t count)
{
	unsigned int enable_5v3v=0;
	ssize_t r;
	smc_dev_t *smc = NULL;

	r = sscanf(buf, "%d", &enable_5v3v);
	if (r != 1)
		return -EINVAL;

	mutex_lock(&smc_lock);
	smc = &smc_dev[0];
	smc->enable_5v3v_level = enable_5v3v;

	if (smc->enable_5v3v_pin != -1) {
		_gpio_out(smc->enable_5v3v_pin, smc->enable_5v3v_level, SMC_ENABLE_5V3V_PIN_NAME);
		pr_error("enable_pin: -->(%d)\n", (smc->enable_5v3v_level)? 1 : 0);
	}
	mutex_unlock(&smc_lock);

	return count;
}

static struct class_attribute smc_class_attrs[] = {
	__ATTR(smc_gpio_pull,  S_IRUGO | S_IWUSR, show_gpio_pull,    set_gpio_pull),
	__ATTR(ctrl_5v3v,      S_IRUGO | S_IWUSR, show_5v3v,         store_5v3v),
    __ATTR_NULL
};

static struct class smc_class = {
    .name = SMC_CLASS_NAME,
    .class_attrs = smc_class_attrs,
};

static unsigned long get_clk(char *name)
{
	struct clk *clk=NULL;
	clk = clk_get_sys(name, NULL);
	if(clk)
		return clk_get_rate(clk);
	return 0;
}

static unsigned long get_module_clk(int sel)
{
#ifdef CONFIG_ARCH_ARC700
	extern unsigned long get_mpeg_clk(void);
	return get_mpeg_clk();
#else

	unsigned long clk=0;

#ifdef CONFIG_ARCH_MESON6/*M6*/
	/*sel = [0:clk81, 1:ddr-pll, 2:fclk-div5, 3:XTAL]*/
	switch(sel)
	{
		case 0: clk = get_clk("clk81"); break;
		case 1: clk = get_clk("pll_ddr"); break;
		case 2: clk = get_clk("fixed")/5; break;
		case 3: clk = get_clk("xtal"); break;
	}
#else
	/*sel = [0:fclk-div2/fclk-div4(M8 and further), 1:fclk-div3, 2:fclk-div5, 3:XTAL]*/
	switch(sel)
	{
#if defined(MESON_CPU_TYPE_MESON8) && (MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8)
		case 0: clk = get_clk("pll_fixed")/4;break;
#else/*M6TV/TVD/TVLITE*/
		case 0: clk = 1000000000; break;
		//case 0: clk = get_clk("fixed")/2; break;
#endif
		case 1: clk = get_clk("pll_fixed")/3; break;
		case 2: clk = get_clk("pll_fixed")/5; break;
		case 3: clk = get_clk("xtal"); break;
	}
#endif /*M6*/

	if(!clk)
		pr_error("fail: unknown clk source");

	return clk;

#endif
}

static int _gpio_request(unsigned int gpio, const char *owner)
{
#if LINUX_VERSION_CODE>= KERNEL_VERSION(3,10,0)
	amlogic_gpio_request(gpio, owner);
#else
	gpio_request(gpio, owner);
#endif
	return 0;
}

static int _gpio_out(unsigned int gpio, int val, const char *owner)
{
	if(val<0) {
		pr_error("gpio out val=-1.\n");
		return -1;
	}

#if LINUX_VERSION_CODE>= KERNEL_VERSION(3,10,0)
	amlogic_gpio_direction_output(gpio, val, owner);
#else
	gpio_out(gpio, val);
#endif

	return 0;
}

#ifdef DET_FROM_PIO
static int _gpio_in(unsigned int gpio, const char *owner)
{
	int ret=0;

#if LINUX_VERSION_CODE>= KERNEL_VERSION(3,10,0)
	ret = amlogic_get_value(gpio, owner);
#else
	ret = gpio_in_get(gpio);
#endif

	return ret;
}
#endif
static int _gpio_free(unsigned int gpio, const char *owner)
{

#if LINUX_VERSION_CODE>= KERNEL_VERSION(3,10,0)
	amlogic_gpio_free(gpio, owner);
#else
	gpio_free(gpio);
#endif

	return 0;
}

static int inline smc_write_end(smc_dev_t *smc)
{
	int ret;
	unsigned long flags;
	
	spin_lock_irqsave(&smc->slock, flags);
	ret = (!smc->cardin || !smc->send_count);
	spin_unlock_irqrestore(&smc->slock, flags);
	
	return ret;
}


static int inline smc_can_read(smc_dev_t *smc)
{
	int ret;
	unsigned long flags;
	
	spin_lock_irqsave(&smc->slock, flags);
	ret = (!smc->cardin || smc->recv_count);
	spin_unlock_irqrestore(&smc->slock, flags);
	
	return ret;
}

static int inline smc_can_write(smc_dev_t *smc)
{
	int ret;
	unsigned long flags;
	
	spin_lock_irqsave(&smc->slock, flags);
	ret = (!smc->cardin || (smc->send_count!=SEND_BUF_SIZE));
	spin_unlock_irqrestore(&smc->slock, flags);
	return ret;
}

static int smc_hw_set_param(smc_dev_t *smc)
{
	unsigned long v=0;
	SMCCARD_HW_Reg0_t *reg0;
	SMCCARD_HW_Reg6_t *reg6;
	SMCCARD_HW_Reg2_t *reg2;
	SMCCARD_HW_Reg5_t *reg5;
/*
	SMC_ANSWER_TO_RST_t *reg1;
	SMC_INTERRUPT_Reg_t *reg_int;
*/
	unsigned sys_clk_rate = get_module_clk(clock_source);
	unsigned long freq_cpu = sys_clk_rate/1000;

	v = SMC_READ_REG(REG0);
	reg0 = (SMCCARD_HW_Reg0_t*)&v;
	reg0->etu_divider = ETU_DIVIDER_CLOCK_HZ*smc->param.f/(smc->param.d*smc->param.freq)-1;
	SMC_WRITE_REG(REG0, v);

	v = SMC_READ_REG(REG2);
	reg2 = (SMCCARD_HW_Reg2_t*)&v;
	reg2->recv_invert = smc->param.recv_invert;
	reg2->recv_parity = smc->param.recv_parity;
	reg2->recv_lsb_msb = smc->param.recv_lsb_msb;
	reg2->xmit_invert = smc->param.xmit_invert;
	reg2->xmit_parity = smc->param.xmit_parity;
	reg2->xmit_lsb_msb = smc->param.xmit_lsb_msb;
	reg2->xmit_retries = smc->param.xmit_retries;
	reg2->xmit_repeat_dis = smc->param.xmit_repeat_dis;
	reg2->recv_no_parity = smc->param.recv_no_parity;
	reg2->clk_tcnt = freq_cpu/smc->param.freq - 1;
	reg2->det_filter_sel = DET_FILTER_SEL_DEFAULT;
	reg2->io_filter_sel = IO_FILTER_SEL_DEFAULT; 
	reg2->clk_sel = clock_source;
	//reg2->pulse_irq = 0;
	SMC_WRITE_REG(REG2, v);

	v = SMC_READ_REG(REG5);
	reg5 = (SMCCARD_HW_Reg5_t*)&v;
	//reg5->cwt_detect_en = 1;
	reg5->bwt_base_time_gnt = BWT_BASE_DEFAULT;
	SMC_WRITE_REG(REG5, v);

	v = SMC_READ_REG(REG6);
	reg6 = (SMCCARD_HW_Reg6_t*)&v;
	reg6->cwi_value = smc->param.cwi;
	reg6->bwi = smc->param.bwi;
	reg6->bgt = smc->param.bgt-2;
	reg6->N_parameter = smc->param.n;
	SMC_WRITE_REG(REG6, v);

	return 0;
}

/*
static int smc_hw_get_param(smc_dev_t *smc)
{
	unsigned long v=0;
	SMCCARD_HW_Reg0_t *reg0;
	SMC_ANSWER_TO_RST_t *reg1;
	SMCCARD_HW_Reg2_t *reg2;
	SMC_INTERRUPT_Reg_t *reg_int;
	SMCCARD_HW_Reg5_t *reg5;
	SMCCARD_HW_Reg6_t *reg6;

	v = SMC_READ_REG(REG0);
	printk("REG0:0X%08X\n", v);
	v = SMC_READ_REG(REG2);
	printk("REG2:0X%08X\n", v);
	v = SMC_READ_REG(REG6);
	printk("REG6:0X%08X\n", v);

	return 0;
}
*/

static int smc_hw_setup(smc_dev_t *smc)
{
	unsigned long v=0;
	SMCCARD_HW_Reg0_t *reg0;
	SMC_ANSWER_TO_RST_t *reg1;
	SMCCARD_HW_Reg2_t *reg2;
	SMC_INTERRUPT_Reg_t *reg_int;
	SMCCARD_HW_Reg5_t *reg5;
	SMCCARD_HW_Reg6_t *reg6;

	unsigned sys_clk_rate = get_module_clk(clock_source);

	unsigned long freq_cpu = sys_clk_rate/1000;

	pr_error("SMC CLK SOURCE - %luKHz\n", freq_cpu);

#ifdef RST_FROM_PIO
	_gpio_out(smc->reset_pin, RESET_ENABLE, SMC_RESET_PIN_NAME);
#endif
	
	v = SMC_READ_REG(REG0);
	reg0 = (SMCCARD_HW_Reg0_t*)&v;
	reg0->enable = 1;
	reg0->clk_en = 0;
	reg0->clk_oen = 0;
	reg0->card_detect = 0;
	reg0->start_atr = 0;
	reg0->start_atr_en = 0;
	reg0->rst_level = RESET_ENABLE;
	reg0->io_level = 0;
	reg0->recv_fifo_threshold = FIFO_THRESHOLD_DEFAULT;
	reg0->etu_divider = ETU_DIVIDER_CLOCK_HZ*smc->param.f/(smc->param.d*smc->param.freq)-1;
	reg0->first_etu_offset = 5;
	SMC_WRITE_REG(REG0, v);
	pr_error("REG0: 0x%08lx\n", v);
	pr_error("f       :%d\n", smc->param.f);	
	pr_error("d       :%d\n", smc->param.d);
	pr_error("freq    :%d\n", smc->param.freq);

	v = SMC_READ_REG(REG1);
	reg1 = (SMC_ANSWER_TO_RST_t*)&v;
	reg1->atr_final_tcnt = ATR_FINAL_TCNT_DEFAULT;
	reg1->atr_holdoff_tcnt = ATR_HOLDOFF_TCNT_DEFAULT;
	reg1->atr_clk_mux = ATR_CLK_MUX_DEFAULT;
	reg1->atr_holdoff_en = atr_holdoff;//ATR_HOLDOFF_EN;
	reg1->etu_clk_sel = ETU_CLK_SEL;
	SMC_WRITE_REG(REG1, v);
	pr_error("REG1: 0x%08lx\n", v);
	
	v = SMC_READ_REG(REG2);
	reg2 = (SMCCARD_HW_Reg2_t*)&v;
	reg2->recv_invert = smc->param.recv_invert;
	reg2->recv_parity = smc->param.recv_parity;
	reg2->recv_lsb_msb = smc->param.recv_lsb_msb;
	reg2->xmit_invert = smc->param.xmit_invert;
	reg2->xmit_lsb_msb = smc->param.xmit_lsb_msb;	
	reg2->xmit_retries = smc->param.xmit_retries;
	reg2->xmit_repeat_dis = smc->param.xmit_repeat_dis;
	reg2->recv_no_parity = smc->param.recv_no_parity;
	reg2->clk_tcnt = freq_cpu/smc->param.freq - 1;
	reg2->det_filter_sel = DET_FILTER_SEL_DEFAULT;
	reg2->io_filter_sel = IO_FILTER_SEL_DEFAULT; 
	reg2->clk_sel = clock_source;
	//reg2->pulse_irq = 0;
	SMC_WRITE_REG(REG2, v);
	pr_error("REG2: 0x%08lx\n", v);
	pr_error("recv_inv:%d\n", smc->param.recv_invert);
	pr_error("recv_lsb:%d\n", smc->param.recv_lsb_msb);
	pr_error("recv_par:%d\n", smc->param.recv_parity);
	pr_error("recv_npa:%d\n", smc->param.recv_no_parity);
	pr_error("xmit_inv:%d\n", smc->param.xmit_invert);
	pr_error("xmit_lsb:%d\n", smc->param.xmit_lsb_msb);
	pr_error("xmit_par:%d\n", smc->param.xmit_parity);
	pr_error("xmit_rep:%d\n", smc->param.xmit_repeat_dis);
	pr_error("xmit_try:%d\n", smc->param.xmit_retries);
	
	v = SMC_READ_REG(INTR);	
	reg_int = (SMC_INTERRUPT_Reg_t*)&v;
	reg_int->recv_fifo_bytes_threshold_int_mask = 0;
	reg_int->send_fifo_last_byte_int_mask = 1;
	reg_int->cwt_expeired_int_mask = 1;
	reg_int->bwt_expeired_int_mask = 1;
	reg_int->write_full_fifo_int_mask = 1;
	reg_int->send_and_recv_confilt_int_mask = 1;
	reg_int->recv_error_int_mask = 1;
	reg_int->send_error_int_mask = 1;
	reg_int->rst_expired_int_mask = 1;
	reg_int->card_detect_int_mask = 0;
	SMC_WRITE_REG(INTR,v|0x03FF);
	pr_error("INTR: 0x%08lx\n", v);

	v = SMC_READ_REG(REG5);
	reg5 = (SMCCARD_HW_Reg5_t*)&v;
	reg5->cwt_detect_en = cwt_det_en;
	reg5->bwt_base_time_gnt = BWT_BASE_DEFAULT;
	SMC_WRITE_REG(REG5, v);
	pr_error("REG5: 0x%08lx\n", v);

						
	v = SMC_READ_REG(REG6);
	reg6 = (SMCCARD_HW_Reg6_t*)&v;
	reg6->N_parameter = smc->param.n;
	reg6->cwi_value = smc->param.cwi;
	reg6->bgt = smc->param.bgt-2;
	reg6->bwi = smc->param.bwi;
	SMC_WRITE_REG(REG6, v);
	pr_error("REG6: 0x%08lx\n", v);
	pr_error("N       :%d\n", smc->param.n);
	pr_error("cwi     :%d\n", smc->param.cwi);
	pr_error("bgt     :%d\n", smc->param.bgt);
	pr_error("bwi     :%d\n", smc->param.bwi);
	return 0;
}

static void enable_smc_clk(smc_dev_t *smc){
	unsigned int _value;

	if((smc->pin_clk_pinmux_reg == -1)
		|| (smc->pin_clk_pinmux_bit == -1))
		return;

	_value = READ_CBUS_REG(smc->pin_clk_pinmux_reg);
	_value |= smc->pin_clk_pinmux_bit;
	WRITE_CBUS_REG(smc->pin_clk_pinmux_reg, _value);
	_value = READ_CBUS_REG(smc->pin_clk_pinmux_reg);

	pr_dbg("enable smc_clk: mux[%x] \n", _value);
}

static void disable_smc_clk(smc_dev_t *smc){
	unsigned int _value;

	if((smc->pin_clk_pinmux_reg == -1)
		|| (smc->pin_clk_pinmux_bit == -1))
		return;

	_value = READ_CBUS_REG(smc->pin_clk_pinmux_reg);
	_value &= ~smc->pin_clk_pinmux_bit;
	WRITE_CBUS_REG(smc->pin_clk_pinmux_reg, _value);
	_value = READ_CBUS_REG(smc->pin_clk_pinmux_reg);

	pr_dbg("disable smc_clk: mux[%x] \n", _value);

	if((smc->pin_clk_oen_reg != -1)
		&&(smc->pin_clk_out_reg != -1)
		&&(smc->pin_clk_bit != -1)) {

		//force the clk pin to low.
		_value = READ_CBUS_REG(smc->pin_clk_oen_reg);
		_value &= ~smc->pin_clk_bit;
		WRITE_CBUS_REG(smc->pin_clk_oen_reg, _value);
		_value = READ_CBUS_REG(smc->pin_clk_out_reg);
		_value &= ~smc->pin_clk_bit;
		WRITE_CBUS_REG(smc->pin_clk_out_reg, _value);

		pr_dbg("disable smc_clk: pin[%x](reg) \n", _value);
	} else if(smc->pin_clk_pin != -1) {

	   udelay(20);
	   _gpio_out(smc->pin_clk_pin, 0, SMC_CLK_PIN_NAME);
	   udelay(1000);

		pr_dbg("disable smc_clk: pin[%x](pin) \n", smc->pin_clk_pin);
	} else {

		pr_error("no reg/bit or pin defined for clk-pin contrl.\n");
	}

	return;
}

static int smc_hw_active(smc_dev_t *smc)
{
    if(ENA_GPIO_PULL > 0){
        enable_smc_clk(smc);
        udelay(200);
    }
	if(!smc->active) {
		if(smc->reset) {
			smc->reset(NULL, 0);
		} else {
			if(smc->enable_pin != -1) {
				_gpio_out(smc->enable_pin, smc->enable_level, SMC_ENABLE_PIN_NAME);
			}
		}
	
		udelay(200);
		smc_hw_setup(smc);

		smc->active = 1;
	}
	
	return 0;
}

static int smc_hw_deactive(smc_dev_t *smc)
{
	if(smc->active) {
		unsigned long sc_reg0 = SMC_READ_REG(REG0);
		SMCCARD_HW_Reg0_t *sc_reg0_reg = (void *)&sc_reg0;
		sc_reg0_reg->rst_level = RESET_ENABLE;
		sc_reg0_reg->enable= 1;
		sc_reg0_reg->start_atr = 0;	
		sc_reg0_reg->start_atr_en = 0;	
		sc_reg0_reg->clk_en=0;
		SMC_WRITE_REG(REG0,sc_reg0);
#ifdef RST_FROM_PIO
		_gpio_out(smc->reset_pin, RESET_ENABLE, SMC_RESET_PIN_NAME);
#endif

		udelay(200);

		if(smc->reset) {
			smc->reset(NULL, 1);
		} else {
			if(smc->enable_pin != -1) {
				_gpio_out(smc->enable_pin, !smc->enable_level, SMC_ENABLE_PIN_NAME);
			}
		}
        if(ENA_GPIO_PULL > 0){
             disable_smc_clk(smc);
        }
		smc->active = 0;
	}
	
	return 0;
}

#ifndef ATR_FROM_INT
static int smc_hw_get(smc_dev_t *smc, int cnt, int timeout)
{
	unsigned long sc_status;
	int times = timeout*100;
	SMC_STATUS_Reg_t *sc_status_reg = (SMC_STATUS_Reg_t*)&sc_status;
	
	while((times > 0) && (cnt > 0)) {
		sc_status = SMC_READ_REG(STATUS);

		//pr_dbg("read atr status %08x\n", sc_status);
						
		if(sc_status_reg->rst_expired_status)
		{
			//pr_error("atr timeout\n");
		}
		
		if(sc_status_reg->cwt_expeired_status)
		{
			//pr_error("cwt timeout when get atr, but maybe it is natural!\n");
		}

		if(sc_status_reg->recv_fifo_empty_status)
		{
			udelay(10);
			times--;
		}
		else
		{
			while(sc_status_reg->recv_fifo_bytes_number > 0)
			{
				u8 byte = (SMC_READ_REG(FIFO))&0xff;

#ifdef SW_INVERT
				if(smc->sc_type == SC_INVERSE)
					byte = inv_table[byte];
#endif

				smc->atr.atr[smc->atr.atr_len++] = byte;
				sc_status_reg->recv_fifo_bytes_number--;
				cnt--;
				if(cnt==0){
					pr_dbg("read atr bytes ok\n");
					return 0;
				}
			}
		}
	}
	
	pr_error("read atr failed\n");
	return -1;
}
#endif /*ifndef ATR_FROM_INT*/

#define INV(a) ((smc->sc_type==SC_INVERSE)? inv_table[(int)(a)] : (a))

static int smc_get(smc_dev_t *smc, int size, int timeout)
{
	unsigned long flags;
	int pos = 0, ret=0;
	int times = timeout/10;

	if(!times)
		times = 1;

	while((times>0) && (size>0)) {

		spin_lock_irqsave(&smc->slock, flags);

		if(!smc->cardin) {
			ret = -ENODEV;
		} else if (!smc->recv_count) {
			ret = -EAGAIN;
		} else {
			ret = smc->recv_count;
			if(ret>size) ret = size;
			
			pos = smc->recv_start;
			smc->recv_start += ret;
			smc->recv_count -= ret;
			smc->recv_start %= RECV_BUF_SIZE;
		}

		spin_unlock_irqrestore(&smc->slock, flags);

		if(ret>0) {
			int cnt = RECV_BUF_SIZE-pos;
			int i;
			unsigned char byte = 0xff;
			pr_dbg("read %d bytes\n", ret);

			if(cnt>=ret) {
				for(i=0; i<ret; i++){
					byte = smc->recv_buf[pos+i];
					smc->atr.atr[smc->atr.atr_len+i] = INV(byte);
				}
				smc->atr.atr_len+=ret;
				dump(smc->recv_buf+pos, ret);
			} else {
				int cnt1 = ret-cnt;
				
				for(i=0; i<cnt; i++){
					byte = smc->recv_buf[pos+i];
					smc->atr.atr[smc->atr.atr_len+i] = INV(byte);
				}
				smc->atr.atr_len+=cnt;
				dump(smc->recv_buf+pos, cnt);

				for(i=0; i<cnt1; i++){
					byte = smc->recv_buf[i];
					smc->atr.atr[smc->atr.atr_len+i] = INV(byte);
				}
				smc->atr.atr_len+=cnt1;
				dump(smc->recv_buf, cnt1);
			}
			size-=ret;
		} else {
			msleep(1);
			times--;
		}
	}

	if(size>0)
		ret = -ETIMEDOUT;

	return ret;
}

#ifdef SMC_FIQ
static int smc_fiq_get(smc_dev_t *smc, int size, int timeout)
{
	int pos = 0, ret=0;
	int times = timeout/10;
	int start, end;

	if(!times)
		times = 1;
	
	while((times>0) && (size>0)) {

		start = smc->recv_start;
		end = smc->recv_end;/*momentary value*/
		
		if(!smc->cardin) {
			ret = -ENODEV;
		} else if (start == end) {
			ret = -EAGAIN;
		} else {
			int i;
			/*ATR only, no loop*/
			ret = end - start;
			if(ret>size) ret=size;
			memcpy(&smc->atr.atr[smc->atr.atr_len], &smc->recv_buf[start], ret);
			for(i=smc->atr.atr_len;i<smc->atr.atr_len+ret;i++)
				smc->atr.atr[i] = INV((int)smc->atr.atr[i]);
			smc->atr.atr_len += ret;

			smc->recv_start += ret;
			size -= ret;
		}

		if(ret<0) {
			msleep(10);
			times--;
		}
	}

	if(size>0)
		ret = -ETIMEDOUT;
	
	return ret;
}
#endif

//static int smc_hw_read_atr(smc_dev_t *smc) __attribute__((unused));

static int smc_hw_read_atr(smc_dev_t *smc)
{
	char *ptr = smc->atr.atr;
	int his_len, t, tnext = 0, only_t0 = 1, loop_cnt=0;
	int i;

	pr_dbg("read atr\n");

#ifdef ATR_FROM_INT
#ifdef SMC_FIQ
#define smc_hw_get smc_fiq_get
#else
#define smc_hw_get smc_get
#endif
#endif
	
	smc->atr.atr_len = 0;
	if(smc_hw_get(smc, 2, 2000)<0){
		goto end;
	}

#ifdef SW_INVERT
	if(0x03 == ptr[0]){
		smc->sc_type = SC_INVERSE;
		ptr[0] = inv_table[(int)ptr[0]];
		if(smc->atr.atr_len>1)
			ptr[1] = inv_table[(int)ptr[1]];
	}else if(0x3F == ptr[0]){
		smc->sc_type = SC_INVERSE;
		if(smc->atr.atr_len>1)
			ptr[1] = inv_table[(int)ptr[1]];
	}else if(0x3B == ptr[0]){
		smc->sc_type = SC_DIRECT;
	}else if(0x23 == ptr[0]){
		smc->sc_type = SC_DIRECT;
		ptr[0] = inv_table[(int)ptr[0]];
		if(smc->atr.atr_len>1)
			ptr[1] = inv_table[(int)ptr[1]];
	}
#endif /*SW_INVERT*/

	ptr++;
	his_len = ptr[0]&0x0F;

	do {
		tnext = 0;
		loop_cnt++;
		if(ptr[0]&0x10) {
			if(smc_hw_get(smc, 1, 1000)<0)
				goto end;
		}
		if(ptr[0]&0x20) {
			if(smc_hw_get(smc, 1, 1000)<0)
				goto end;
		}
		if(ptr[0]&0x40) {
			if(smc_hw_get(smc, 1, 1000)<0)
				goto end;
		}
		if(ptr[0]&0x80) {
			if(smc_hw_get(smc, 1, 1000)<0)
				goto end;

			ptr = &smc->atr.atr[smc->atr.atr_len-1];
			t = ptr[0]&0x0F;
			if(t) {
				only_t0 = 0;
			}
			if(ptr[0]&0xF0) {
				tnext = 1;
			}
		}
	} while(tnext && loop_cnt<4);
	
	if(!only_t0) his_len++;
	smc_hw_get(smc, his_len, 2000);

	printk("get atr len:%d data: ", smc->atr.atr_len);
	for(i=0; i<smc->atr.atr_len; i++){
		printk("%02x ", smc->atr.atr[i]);
	}
	printk("\n");

	return 0;

end:
	pr_error("read atr failed\n");
	return -EIO;
#ifdef ATR_FROM_INT
#undef smc_hw_get
#endif	
}

static int smc_hw_reset(smc_dev_t *smc)
{
	unsigned long flags;
	int ret;
	unsigned long sc_reg0 = SMC_READ_REG(REG0);
	SMCCARD_HW_Reg0_t *sc_reg0_reg = (void *)&sc_reg0;
	unsigned long sc_int;
	SMC_INTERRUPT_Reg_t *sc_int_reg = (void *)&sc_int;
	
	spin_lock_irqsave(&smc->slock, flags);
	if(smc->cardin) {
		ret = 0;
	} else {
		ret = -ENODEV;
	}
	spin_unlock_irqrestore(&smc->slock, flags);
	
	if(ret>=0) {
		/*Reset*/
#ifdef NO_HOT_RESET
		smc->active = 0;
#endif
		if(smc->active) {
			pr_dbg("hot reset\n");

			sc_reg0_reg->rst_level = RESET_ENABLE;
			sc_reg0_reg->clk_en = 1;
			sc_reg0_reg->etu_divider = ETU_DIVIDER_CLOCK_HZ*smc->param.f/(smc->param.d*smc->param.freq)-1;
			SMC_WRITE_REG(REG0, sc_reg0);
#ifdef RST_FROM_PIO
			_gpio_out(smc->reset_pin, RESET_ENABLE, SMC_RESET_PIN_NAME);
#endif
			
			udelay(800/smc->param.freq); // >= 400/f ;

			/* disable receive interrupt*/
			sc_int = SMC_READ_REG(INTR);
			sc_int_reg->recv_fifo_bytes_threshold_int_mask = 0;
			SMC_WRITE_REG(INTR, sc_int|0x3FF);
		
			sc_reg0_reg->rst_level = RESET_DISABLE;
			sc_reg0_reg->start_atr = 1;
			SMC_WRITE_REG(REG0, sc_reg0);
#ifdef RST_FROM_PIO
			_gpio_out(smc->reset_pin, RESET_DISABLE, SMC_RESET_PIN_NAME);
#endif
		} else {
			pr_dbg("cold reset\n");
			
			smc_hw_deactive(smc);
			
			udelay(200);

			smc_hw_active(smc);

			sc_reg0_reg->clk_en =1 ;
			sc_reg0_reg->enable = 0;
			sc_reg0_reg->rst_level = RESET_ENABLE;
			SMC_WRITE_REG(REG0, sc_reg0);
#ifdef RST_FROM_PIO
			_gpio_out(smc->reset_pin, RESET_ENABLE, SMC_RESET_PIN_NAME);
#endif
			udelay(2000); // >= 400/f ;

			/* disable receive interrupt*/
			sc_int = SMC_READ_REG(INTR);
			sc_int_reg->recv_fifo_bytes_threshold_int_mask = 0;
			SMC_WRITE_REG(INTR, sc_int|0x3FF);

			sc_reg0_reg->rst_level = RESET_DISABLE;
			sc_reg0_reg->start_atr_en = 1;
			sc_reg0_reg->start_atr = 1;	
			sc_reg0_reg->enable = 1;
			SMC_WRITE_REG(REG0, sc_reg0);
#ifdef RST_FROM_PIO
			_gpio_out(smc->reset_pin, RESET_DISABLE, SMC_RESET_PIN_NAME);
#endif
		}

		/*reset recv&send buf*/
		smc->send_start = 0;
		smc->send_count = 0;
		smc->recv_start = 0;
		smc->recv_count = 0;
		
		/*Read ATR*/
		smc->atr.atr_len = 0;
		smc->recv_count = 0;
		smc->send_count = 0;

#ifdef SMC_FIQ
		smc->recv_end = 0;
		smc->send_end = 0;
#endif

#ifdef SW_INVERT
		smc->atr_mode = 1;
#endif

#ifdef ATR_FROM_INT
		printk("ATR from INT\n");
		/* enable receive interrupt*/
		sc_int = SMC_READ_REG(INTR);
		sc_int_reg->recv_fifo_bytes_threshold_int_mask = 1;
		SMC_WRITE_REG(INTR, sc_int|0x3FF);
#endif

		//msleep(atr_delay);
		ret = smc_hw_read_atr(smc);

#ifdef SW_INVERT
		smc->atr_mode = 0;
#endif

#ifdef ATR_FROM_INT
		/* disable receive interrupt*/
		sc_int = SMC_READ_REG(INTR);
		sc_int_reg->recv_fifo_bytes_threshold_int_mask = 0;
		SMC_WRITE_REG(INTR, sc_int|0x3FF);
#endif
		
		/*Disable ATR*/
		sc_reg0 = SMC_READ_REG(REG0);
		sc_reg0_reg->start_atr_en = 0;
		sc_reg0_reg->start_atr = 0;
		SMC_WRITE_REG(REG0,sc_reg0);
		
#ifndef DISABLE_RECV_INT
		sc_int_reg->recv_fifo_bytes_threshold_int_mask = 1;
#endif
		SMC_WRITE_REG(INTR, sc_int|0x3FF);
	}
	
	return ret;
}

static int smc_hw_get_status(smc_dev_t *smc, int *sret)
{
	unsigned long flags;
#ifndef DET_FROM_PIO
	unsigned int  reg_val;
	SMCCARD_HW_Reg0_t *reg = (SMCCARD_HW_Reg0_t*)&reg_val;
#endif

	spin_lock_irqsave(&smc->slock, flags);

#ifdef DET_FROM_PIO
	smc->cardin = _gpio_in(smc->detect_pin, SMC_DETECT_PIN_NAME);
	pr_dbg("get_status: card detect: %d\n", smc->cardin);
#else
	reg_val = SMC_READ_REG(REG0);
	smc->cardin = reg->card_detect;
	//pr_dbg("get_status: smc reg0 %08x, card detect: %d\n", reg_val, smc->cardin);
#endif
	if (smc->detect_invert)
		smc->cardin = !smc->cardin;

	*sret = smc->cardin;

	spin_unlock_irqrestore(&smc->slock, flags);

	return 0;
}

#ifdef SMC_FIQ

static inline void _atomic_wrap_inc(int *p, int wrap)
{
    int i = *p;
    i++;
    if(i>=wrap)
		i = 0;
	*p = i;
}

static inline void _atomic_wrap_add(int *p, int add, int wrap)
{
    int i = *p;
    i+=add;
    if(i>=wrap)
		i %= wrap;
	*p = i;
}

static int smc_hw_start_send(smc_dev_t *smc)
{
	unsigned long flags;
	unsigned int sc_status;
	SMC_STATUS_Reg_t *sc_status_reg = (SMC_STATUS_Reg_t*)&sc_status;
	u8 byte;
	int cnt = 0;

#if 1
	/*trigger only*/
	sc_status = SMC_READ_REG(STATUS);
	if (smc->send_end!=smc->send_start && !sc_status_reg->send_fifo_full_status) {
		pr_dbg("s i f [%d:%d]\n", smc->send_start, smc->send_end);
		byte = smc->send_buf[smc->send_end];
		_atomic_wrap_inc(&smc->send_end, SEND_BUF_SIZE);
#ifdef SW_INVERT
		if(smc->sc_type == SC_INVERSE)
			byte = inv_table[byte];
#endif
		SMC_WRITE_REG(FIFO, byte);
		pr_dbg("send 1st byte to hw\n");
	}
#else
	while(1) {
		sc_status = SMC_READ_REG(STATUS);
		if (smc->send_end==smc->send_start || sc_status_reg->send_fifo_full_status) {
			break;
		}

		pr_dbg("s i f [%d:%d]\n", smc->send_start, smc->send_end);
		byte = smc->send_buf[smc->send_end];
		_atomic_wrap_inc(&smc->send_end, SEND_BUF_SIZE);
#ifdef SW_INVERT
		if(smc->sc_type == SC_INVERSE)
			byte = inv_table[byte];
#endif
		SMC_WRITE_REG(FIFO, byte);
		cnt++;
	}
	pr_dbg("send %d bytes to hw\n", cnt);
#endif

	return 0;
}

static irqreturn_t smc_bridge_isr(int irq, void *dev_id)
{
	smc_dev_t *smc = (smc_dev_t*)dev_id;

#ifdef DET_FROM_PIO
	smc->cardin = _gpio_in(smc->detect_pin, SMC_DETECT_PIN_NAME);
	if (smc->detect_invert)
		smc->cardin = !smc->cardin;
#endif

    if(smc->recv_start != smc->recv_end)
		wake_up_interruptible(&smc->rd_wq);
    if(smc->send_start == smc->send_end)
		wake_up_interruptible(&smc->wr_wq);

    return IRQ_HANDLED;
}

static inline int smc_can_recv_max(smc_dev_t *smc)
{
	int start=smc->recv_start;
	int end=smc->recv_end;

	if(end >= start)
		return RECV_BUF_SIZE - end + start;
	else
		return start - end;
}

static void smc_irq_handler(void)
{
	smc_dev_t *smc = &smc_dev[0];
	unsigned int sc_status;
	unsigned int sc_reg0;
	unsigned int sc_int;
	SMC_STATUS_Reg_t *sc_status_reg = (SMC_STATUS_Reg_t*)&sc_status;
	SMC_INTERRUPT_Reg_t *sc_int_reg = (SMC_INTERRUPT_Reg_t*)&sc_int;
	SMCCARD_HW_Reg0_t *sc_reg0_reg = (void *)&sc_reg0;

	sc_int = SMC_READ_REG(INTR);
	//Fpr("smc intr:0x%x\n", sc_int);

	if(sc_int_reg->recv_fifo_bytes_threshold_int) {

		int num;

		sc_status = SMC_READ_REG(STATUS);
		num = sc_status_reg->recv_fifo_bytes_number;

		if(num > smc_can_recv_max(smc)) {
			pr_error("receive buffer overflow\n");
		} else {
			u8 byte;

			while(num){
				byte = SMC_READ_REG(FIFO);
#ifdef SW_INVERT
				if(!smc->atr_mode && smc->sc_type == SC_INVERSE)
					byte = inv_table[byte];
#endif
				smc->recv_buf[smc->recv_end] = byte;
				_atomic_wrap_inc(&smc->recv_end, RECV_BUF_SIZE);
				num--;
				Fpr("fiq R[%x]     \n", smc->recv_buf[smc->recv_end]);
			}
			Fpr("fiq R%d     \n", sc_status_reg->recv_fifo_bytes_number);

			fiq_bridge_pulse_trigger(&smc->smc_fiq_bridge);
		}

		sc_int_reg->recv_fifo_bytes_threshold_int = 0;
		
	}
	
	if(sc_int_reg->send_fifo_last_byte_int) {
		int start = smc->send_start;
		int cnt = 0;
		u8 byte;

		while(1) {
			sc_status = SMC_READ_REG(STATUS);
			if (smc->send_end == start || sc_status_reg->send_fifo_full_status) {
				break;
			}
			
			byte = smc->send_buf[smc->send_end];
			_atomic_wrap_inc(&smc->send_end, SEND_BUF_SIZE);
#ifdef SW_INVERT
			if(smc->sc_type == SC_INVERSE)
				byte = inv_table[byte];
#endif
			SMC_WRITE_REG(FIFO, byte);
			cnt++;
		}
		
		Fpr("fiq W%d     \n", cnt);

		if(smc->send_end==start) {
			sc_int_reg->send_fifo_last_byte_int_mask = 0;
			sc_int_reg->recv_fifo_bytes_threshold_int_mask = 1;
			fiq_bridge_pulse_trigger(&smc->smc_fiq_bridge);
		}

		sc_int_reg->send_fifo_last_byte_int = 0;

	}

#ifndef DET_FROM_PIO
	sc_reg0 = SMC_READ_REG(REG0);
	smc->cardin = sc_reg0_reg->card_detect;
	if (smc->detect_invert)
		smc->cardin = !smc->cardin;
#endif

	SMC_WRITE_REG(INTR, sc_int|0x3FF);

	return IRQ_HANDLED;
}

#else

static int smc_hw_start_send(smc_dev_t *smc)
{
	unsigned long flags;
	unsigned int sc_status;
	SMC_STATUS_Reg_t *sc_status_reg = (SMC_STATUS_Reg_t*)&sc_status;
	u8 byte;
	int cnt = 0;

	while(1) {			
		spin_lock_irqsave(&smc->slock, flags);
		
		sc_status = SMC_READ_REG(STATUS);
		if (!smc->send_count || sc_status_reg->send_fifo_full_status) {
			spin_unlock_irqrestore(&smc->slock, flags);
			break;
		}

		pr_dbg("s i f [%d], [%d]\n", smc->send_count, cnt);
		byte = smc->send_buf[smc->send_start++];
#ifdef SW_INVERT
		if(smc->sc_type == SC_INVERSE)
			byte = inv_table[byte];
#endif
		SMC_WRITE_REG(FIFO, byte);
		smc->send_start %= SEND_BUF_SIZE;
		smc->send_count--;
		cnt++;

		spin_unlock_irqrestore(&smc->slock, flags);
	}

	pr_dbg("send %d bytes to hw\n", cnt);
	
	return 0;
}

static irqreturn_t smc_irq_handler(int irq, void *data)
{
	smc_dev_t *smc = (smc_dev_t*)data;
	unsigned int sc_status;
	unsigned int sc_int;
	SMC_STATUS_Reg_t *sc_status_reg = (SMC_STATUS_Reg_t*)&sc_status;
	SMC_INTERRUPT_Reg_t *sc_int_reg = (SMC_INTERRUPT_Reg_t*)&sc_int;
#ifndef DET_FROM_PIO
	unsigned int sc_reg0;
	SMCCARD_HW_Reg0_t *sc_reg0_reg = (void *)&sc_reg0;
#endif
	unsigned long flags;

	spin_lock_irqsave(&smc->slock, flags);		

	sc_int = SMC_READ_REG(INTR);
	Ipr("smc intr:0x%x\n", sc_int);

	if(sc_int_reg->recv_fifo_bytes_threshold_int) {
		
		if(smc->recv_count==RECV_BUF_SIZE) {
			pr_error("receive buffer overflow\n");
		} else {
			int pos = smc->recv_start+smc->recv_count;
			int i;
			u8 byte;
#if 0
			pos %= RECV_BUF_SIZE;
			smc->recv_buf[pos] = SMC_READ_REG(FIFO);
			smc->recv_count++;
			
			pr_dbg("irq: recv 1 byte [0x%x]\n", smc->recv_buf[pos]);
#else
			sc_status = SMC_READ_REG(STATUS);
			for(i=0; i< sc_status_reg->recv_fifo_bytes_number; i++) {
				pos %= RECV_BUF_SIZE;
				byte = SMC_READ_REG(FIFO);
#ifdef SW_INVERT
				if(!smc->atr_mode && smc->sc_type == SC_INVERSE)
					byte = inv_table[byte];
#endif
				Ipr("%02x ", byte);
				smc->recv_buf[pos++] = byte;
				smc->recv_count++;
			}
			Ipr("irq: recv %d byte from hw\n", sc_status_reg->recv_fifo_bytes_number);
#endif
		}

		sc_int_reg->recv_fifo_bytes_threshold_int = 0;
		
		wake_up_interruptible(&smc->rd_wq);
	}
	
	if(sc_int_reg->send_fifo_last_byte_int) {
		int cnt = 0;
		u8 byte;

		while(1) {
			sc_status = SMC_READ_REG(STATUS);
			if (!smc->send_count || sc_status_reg->send_fifo_full_status) {
				break;
			}
			
			byte = smc->send_buf[smc->send_start++];
#ifdef SW_INVERT
			if(smc->sc_type == SC_INVERSE)
				byte = inv_table[byte];
#endif
			SMC_WRITE_REG(FIFO, byte);
			smc->send_start %= SEND_BUF_SIZE;
			smc->send_count--;
			cnt++;
		}
		
		Ipr("irq: send %d bytes to hw\n", cnt);

		if(!smc->send_count) {
			sc_int_reg->send_fifo_last_byte_int_mask = 0;
			sc_int_reg->recv_fifo_bytes_threshold_int_mask = 1;
		}

		sc_int_reg->send_fifo_last_byte_int = 0;

		wake_up_interruptible(&smc->wr_wq);
	}

#ifndef DET_FROM_PIO
	sc_reg0 = SMC_READ_REG(REG0);
	smc->cardin = sc_reg0_reg->card_detect;
#else
	smc->cardin = _gpio_in(smc->detect_pin, SMC_DETECT_PIN_NAME);
#endif
	if (smc->detect_invert)
		smc->cardin = !smc->cardin;

	SMC_WRITE_REG(INTR, sc_int|0x3FF);

	spin_unlock_irqrestore(&smc->slock, flags);

	return IRQ_HANDLED;
}
#endif

static void smc_dev_deinit(smc_dev_t *smc)
{
	if(smc->irq_num!=-1) {
		free_irq(smc->irq_num, &smc);
	}

	if(smc->enable_pin != -1) {
		_gpio_free(smc->enable_pin,SMC_ENABLE_PIN_NAME);
	}
	if(smc->pin_clk_pin != -1) {
		_gpio_free(smc->pin_clk_pin,SMC_CLK_PIN_NAME);
	}
#ifdef DET_FROM_PIO
	if(smc->detect_pin != -1) {
		_gpio_free(smc->detect_pin,SMC_DETECT_PIN_NAME);
	}
#endif
#ifdef RST_FROM_PIO
	if(smc->reset_pin != -1) {
		_gpio_free(smc->reset_pin,SMC_RESET_PIN_NAME);
	}
#endif
#ifdef CONFIG_OF
	if(smc->pinctrl)
		devm_pinctrl_put(smc->pinctrl);
#endif
	if(smc->dev) {
		device_destroy(&smc_class, MKDEV(smc_major, smc->id));
	}
	
	mutex_destroy(&smc->lock);

	smc->init = 0;

#if defined(MESON_CPU_TYPE_MESON8) && (MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8)
	CLK_GATE_OFF(SMART_CARD_MPEG_DOMAIN);
#endif

}

static int smc_dev_init(smc_dev_t *smc, int id)
{
#ifndef CONFIG_OF
	struct resource *res;
#else
	const char *str;
	u32 value;
	int ret;
#endif
	char buf[32];

#if defined(MESON_CPU_TYPE_MESON8) && (MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8)
	CLK_GATE_ON(SMART_CARD_MPEG_DOMAIN);
#endif

	smc->id = id;

#ifdef CONFIG_OF
	smc->pinctrl = devm_pinctrl_get_select_default(&smc->pdev->dev);
#endif

	smc->enable_pin = -1;
	if(smc->enable_pin==-1) {
		snprintf(buf, sizeof(buf), "smc%d_enable_pin", id);
#ifdef CONFIG_OF
		ret = of_property_read_string(smc->pdev->dev.of_node, buf, &str);
		if(!ret){
			smc->enable_pin = amlogic_gpio_name_map_num(str);
			ret = _gpio_request(smc->enable_pin, SMC_ENABLE_PIN_NAME);
			pr_error("%s: %s [%d]\n", buf, str, ret);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->enable_pin = res->start;
			gpio_request(smc->enable_pin, SMC_ENABLE_PIN_NAME);
		}
#endif /*CONFIG_OF*/
	}

	smc->enable_level = 1;
	if(1) {
		snprintf(buf, sizeof(buf), "smc%d_enable_level", id);
#ifdef CONFIG_OF
		ret = of_property_read_u32(smc->pdev->dev.of_node, buf, &value);
		if(!ret){
			smc->enable_level = value;
			pr_error("%s: %d\n", buf, smc->enable_level);
			if(smc->enable_pin!=-1) {
				_gpio_out(smc->enable_pin, !smc->enable_level, SMC_ENABLE_PIN_NAME);
				pr_error("enable_pin: -->(%d)\n", (!smc->enable_level)?1:0);
			}
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->enable_level = res->start;
		}
#endif /*CONFIG_OF*/
	}

	smc->reset_level = 1;
	if(1) {
		snprintf(buf, sizeof(buf), "smc%d_reset_level", id);
#ifdef CONFIG_OF
		ret = of_property_read_u32(smc->pdev->dev.of_node, buf, &value);
		if(!ret){
			smc->reset_level = value;
			pr_error("%s: %d\n", buf, smc->reset_level);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->reset_level = res->start;
		}
#endif /*CONFIG_OF*/
	}
	
	smc->irq_num = smc0_irq;
	if(smc->irq_num==-1) {
		snprintf(buf, sizeof(buf), "smc%d_irq", id);
#ifdef CONFIG_OF
		ret = of_property_read_u32(smc->pdev->dev.of_node, buf, &value);
		if(!ret){
			smc->irq_num = value;
			pr_error("%s: %d\n", buf, smc->irq_num);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
			return -1;
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_IRQ, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			return -1;
		}
		smc->irq_num = res->start;
#endif /*CONFIG_OF*/
	}
	
	smc->pin_clk_pinmux_reg = -1;
	if(smc->pin_clk_pinmux_reg==-1) {
		snprintf(buf, sizeof(buf), "smc%d_clk_pinmux_reg", id);
#ifdef CONFIG_OF
		ret = of_property_read_u32(smc->pdev->dev.of_node, buf, &value);
		if(!ret){
			smc->pin_clk_pinmux_reg = value;
			pr_error("%s: 0x%x\n", buf, smc->pin_clk_pinmux_reg);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->pin_clk_pinmux_reg = res->start;
		}
#endif /*CONFIG_OF*/
	}

	smc->pin_clk_pinmux_bit = -1;
	if(smc->pin_clk_pinmux_bit==-1) {
		snprintf(buf, sizeof(buf), "smc%d_clk_pinmux_bit", id);
#ifdef CONFIG_OF
		ret = of_property_read_u32(smc->pdev->dev.of_node, buf, &value);
		if(!ret){
			smc->pin_clk_pinmux_bit = value;
			pr_error("%s: 0x%x\n", buf, smc->pin_clk_pinmux_bit);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->pin_clk_pinmux_bit = res->start;
		}
#endif /*CONFIG_OF*/
	}

	smc->pin_clk_oen_reg = -1;
	if(smc->pin_clk_oen_reg==-1) {
		snprintf(buf, sizeof(buf), "smc%d_clk_oen_reg", id);
#ifdef CONFIG_OF
		ret = of_property_read_u32(smc->pdev->dev.of_node, buf, &value);
		if(!ret){
			smc->pin_clk_oen_reg = value;
			pr_error("%s: 0x%x\n", buf, smc->pin_clk_oen_reg);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->pin_clk_oen_reg = res->start;
		}
#endif /*CONFIG_OF*/
	}

	smc->pin_clk_out_reg = -1;
	if(smc->pin_clk_out_reg==-1) {
		snprintf(buf, sizeof(buf), "smc%d_clk_out_reg", id);
#ifdef CONFIG_OF
		ret = of_property_read_u32(smc->pdev->dev.of_node, buf, &value);
		if(!ret){
			smc->pin_clk_out_reg = value;
			pr_error("%s: 0x%x\n", buf, smc->pin_clk_out_reg);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->pin_clk_out_reg = res->start;
		}
#endif /*CONFIG_OF*/
	}

	smc->pin_clk_bit = -1;
	if(smc->pin_clk_bit==-1) {
		snprintf(buf, sizeof(buf), "smc%d_clk_bit", id);
#ifdef CONFIG_OF
		ret = of_property_read_u32(smc->pdev->dev.of_node, buf, &value);
		if(!ret){
			smc->pin_clk_bit = value;
			pr_error("%s: 0x%x\n", buf, smc->pin_clk_bit);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->pin_clk_bit = res->start;
		}
#endif /*CONFIG_OF*/
	}

	smc->pin_clk_pin = -1;
	if(smc->pin_clk_pin==-1) {
		snprintf(buf, sizeof(buf), "smc%d_clk_pin", id);
#ifdef CONFIG_OF
		ret = of_property_read_string(smc->pdev->dev.of_node, buf, &str);
		if(!ret){
			smc->pin_clk_pin = amlogic_gpio_name_map_num(str);
			ret = _gpio_request(smc->pin_clk_pin, SMC_CLK_PIN_NAME);
			pr_error("%s: %s [%d]\n", buf, str, ret);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->pin_clk_pin = res->start;
			gpio_request(smc->pin_clk_pin, SMC_CLK_PIN_NAME);
		}
#endif /*CONFIG_OF*/
	}

#ifdef DET_FROM_PIO
	smc->detect_pin = -1;
	if(smc->detect_pin==-1) {
		snprintf(buf, sizeof(buf), "smc%d_det_pin", id);
#ifdef CONFIG_OF
		ret = of_property_read_string(smc->pdev->dev.of_node, buf, &str);
		if(!ret){
			smc->detect_pin = amlogic_gpio_name_map_num(str);
			ret = _gpio_request(smc->detect_pin, SMC_DETECT_PIN_NAME);
			amlogic_gpio_direction_input(smc->detect_pin, SMC_DETECT_PIN_NAME);
			pr_error("%s: %s [%d]\n", buf, str, ret);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->detect_pin = res->start;
			gpio_request(smc->detect_pin, SMC_DETECT_PIN_NAME);
		}
#endif /*CONFIG_OF*/
	}
#endif

#ifdef RST_FROM_PIO
	smc->reset_pin = -1;
	if(smc->reset_pin==-1) {
		snprintf(buf, sizeof(buf), "smc%d_reset_pin", id);
#ifdef CONFIG_OF
		ret = of_property_read_string(smc->pdev->dev.of_node, buf, &str);
		if(!ret){
			smc->reset_pin = amlogic_gpio_name_map_num(str);
			ret = _gpio_request(smc->reset_pin, SMC_RESET_PIN_NAME);
			pr_error("%s: %s [%d]\n", buf, str, ret);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->reset_pin = res->start;
			gpio_request(smc->reset_pin, SMC_RESET_PIN_NAME);
		}
#endif /*CONFIG_OF*/
	}
#endif

	if (1) {
		snprintf(buf, sizeof(buf), "smc%d_clock_source", id);
#ifdef CONFIG_OF
		ret = of_property_read_u32(smc->pdev->dev.of_node, buf, &value);
		if (!ret) {
			clock_source = value;
			pr_error("%s: %d\n", buf, clock_source);
		} else {
			pr_error("cannot find resource \"%s\"\n", buf);
			pr_error("using clock source default: %d\n", clock_source);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			pr_error("using clock source default: %d\n", clock_source);
		} else {
			clock_source = res->start;
		}
#endif /*CONFIG_OF*/
	}

	smc->detect_invert = 0;
	if (1) {
		snprintf(buf, sizeof(buf), "smc%d_det_invert", id);
#ifdef CONFIG_OF
		ret = of_property_read_u32(smc->pdev->dev.of_node, buf, &value);
		if (!ret) {
			smc->detect_invert = value;
			pr_error("%s: %d\n", buf, smc->detect_invert);
		}else{
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->detect_invert = res->start;
		}
#endif /*CONFIG_OF*/
	}

	smc->enable_5v3v_pin = -1;
	if (smc->enable_5v3v_pin == -1) {
		snprintf(buf, sizeof(buf), "smc%d_5v3v_pin", id);
#ifdef CONFIG_OF
		ret = of_property_read_string(smc->pdev->dev.of_node, buf, &str);
		if (!ret) {
			smc->enable_5v3v_pin = amlogic_gpio_name_map_num(str);
			ret = _gpio_request(smc->enable_5v3v_pin, SMC_ENABLE_5V3V_PIN_NAME);
			pr_error("%s: %s [%d]\n", buf, str, ret);
		} else {
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->enable_5v3v_pin = res->start;
			gpio_request(smc->enable_5v3v_pin, SMC_ENABLE_5V3V_PIN_NAME);
		}
#endif /*CONFIG_OF*/
	}

	smc->enable_5v3v_level = 0;
	if (1) {
		snprintf(buf, sizeof(buf), "smc%d_5v3v_level", id);
#ifdef CONFIG_OF
		ret = of_property_read_u32(smc->pdev->dev.of_node, buf, &value);
		if (!ret) {
			smc->enable_5v3v_level = value;
			pr_error("%s: %d\n", buf, smc->enable_5v3v_level);
			if (smc->enable_5v3v_pin != -1) {
				_gpio_out(smc->enable_5v3v_pin, smc->enable_5v3v_level, SMC_ENABLE_5V3V_PIN_NAME);
				pr_error("enable_pin: -->(%d)\n", (smc->enable_5v3v_level)? 1 : 0);
			}
		} else {
			pr_error("cannot find resource \"%s\"\n", buf);
		}
#else /*CONFIG_OF*/
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->enable_5v3v_level = res->start;
		}
#endif /*CONFIG_OF*/
	}

	init_waitqueue_head(&smc->rd_wq);
	init_waitqueue_head(&smc->wr_wq);

	spin_lock_init(&smc->slock);
	mutex_init(&smc->lock);

#ifdef SMC_FIQ
	{
		int r = -1;
		smc->smc_fiq_bridge.handle = smc_bridge_isr;
		smc->smc_fiq_bridge.key=(u32)smc;
		smc->smc_fiq_bridge.name="smc_bridge_isr";
		r = register_fiq_bridge_handle(&smc->smc_fiq_bridge);
		if(r) {
			pr_error( "smc fiq bridge register error.\n");
			return -1;
		}
	}
	request_fiq(smc->irq_num, &smc_irq_handler);
#else
	smc->irq_num=request_irq(smc->irq_num,(irq_handler_t)smc_irq_handler,IRQF_SHARED,"smc",smc);
	if(smc->irq_num<0) {
		pr_error("request irq error!\n");
		smc_dev_deinit(smc);
		return -1;
	}
#endif
	
	snprintf(buf, sizeof(buf), "smc%d", smc->id);
	smc->dev=device_create(&smc_class, NULL, MKDEV(smc_major, smc->id), smc, buf);
	if(!smc->dev) {
		pr_error("create device error!\n");
		smc_dev_deinit(smc);
		return -1;
	}

	smc->param.f = F_DEFAULT;
	smc->param.d = D_DEFAULT;
	smc->param.n = N_DEFAULT;
	smc->param.bwi = BWI_DEFAULT;
	smc->param.cwi = CWI_DEFAULT;
	smc->param.bgt = BGT_DEFAULT;
	smc->param.freq = FREQ_DEFAULT;
	smc->param.recv_invert = 0;
	smc->param.recv_parity = 0;
	smc->param.recv_lsb_msb = 0;
	smc->param.recv_no_parity = 1;
	smc->param.xmit_invert = 0;
	smc->param.xmit_lsb_msb = 0;
	smc->param.xmit_retries = 1;
	smc->param.xmit_repeat_dis = 1;
	smc->param.xmit_parity = 0;
	smc->init = 1;

	smc_hw_setup(smc);
	
	return 0;
}

static int smc_open(struct inode *inode, struct file *filp)
{
	int id = iminor(inode);
	smc_dev_t *smc = &smc_dev[id];
	
	mutex_lock(&smc->lock);

#ifdef FILE_DEBUG
	open_debug();
#endif
	
	if(smc->used) {
		mutex_unlock(&smc->lock);
		pr_error("smartcard %d already openned!", id);
		return -EBUSY;
	}
	
	smc->used = 1;

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
	switch_mod_gate_by_name("smart_card", 1);
#endif

	mutex_unlock(&smc->lock);
	
	filp->private_data = smc;

	return 0;
}

static int smc_close(struct inode *inode, struct file *filp)
{
	smc_dev_t *smc = (smc_dev_t*)filp->private_data;
	
	mutex_lock(&smc->lock);
	smc_hw_deactive(smc);
	smc->used = 0;

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
	switch_mod_gate_by_name("smart_card", 0);
#endif

#ifdef FILE_DEBUG
	close_debug();
#endif

	mutex_unlock(&smc->lock);
	
	return 0;
}

#ifdef SMC_FIQ
static int smc_read(struct file *filp,char __user *buff, size_t size, loff_t *ppos)
{
	smc_dev_t *smc = (smc_dev_t*)filp->private_data;
	unsigned long flags;
	int ret;
	unsigned long sc_int;
	SMC_INTERRUPT_Reg_t *sc_int_reg = (void *)&sc_int;
	int start=0, end;

	ret = mutex_lock_interruptible(&smc->lock);
	if(ret) return ret;

	spin_lock_irqsave(&smc->slock, flags);
	if(ret==0) {
		
		start = smc->recv_start;
		end = smc->recv_end;

		if(!smc->cardin) {
			ret = -ENODEV;
		} else if (start==end) {
			ret = -EAGAIN;
		} else {
			ret = (end>start)? (end-start):(RECV_BUF_SIZE-start+end);
			if(ret>size)
				ret = size;
		}
	}
	
	if(ret>0) {
		int cnt = RECV_BUF_SIZE-start;
		pr_dbg("read %d bytes\n", ret);
		if(cnt>=ret) {
			copy_to_user(buff, smc->recv_buf+start, ret);
		} else {
			int cnt1 = ret-cnt;
			copy_to_user(buff, smc->recv_buf+start, cnt);
			copy_to_user(buff+cnt, smc->recv_buf, cnt1);
		}
		_atomic_wrap_add(&smc->recv_start, ret, RECV_BUF_SIZE);
	}
	spin_unlock_irqrestore(&smc->slock, flags);
	
	mutex_unlock(&smc->lock);
	
	return ret;
}

static int smc_write(struct file *filp, const char __user *buff, size_t size, loff_t *offp)
{
	smc_dev_t *smc = (smc_dev_t*)filp->private_data;
	unsigned long flags;
	int pos = 0, ret;
	unsigned long sc_int;
	SMC_INTERRUPT_Reg_t *sc_int_reg = (void *)&sc_int;
	int start=0, end;

	ret = mutex_lock_interruptible(&smc->lock);
	if(ret) return ret;
	
	spin_lock_irqsave(&smc->slock, flags);

	if(ret==0) {

		start = smc->send_start;
		end = smc->send_end;

		if(!smc->cardin) {
			ret = -ENODEV;
		} else if(start!=end){
			ret = -EAGAIN;
		} else {
			ret = size;
			if(ret>=SEND_BUF_SIZE)
				ret=SEND_BUF_SIZE-1;
		}
	}
	
	if(ret>0) {
		int cnt = SEND_BUF_SIZE-start;
		
		if(cnt>=ret) {
			copy_from_user(smc->send_buf+start, buff, ret);
		} else {
			int cnt1 = ret-cnt;
			copy_from_user(smc->send_buf+start, buff, cnt);
			copy_from_user(smc->send_buf, buff+cnt, cnt1);
		}
		_atomic_wrap_add(&smc->send_start, ret, SEND_BUF_SIZE);
	}

	spin_unlock_irqrestore(&smc->slock, flags);
	
	if(ret>0) {
		sc_int = SMC_READ_REG(INTR);
#ifdef DISABLE_RECV_INT
		sc_int_reg->recv_fifo_bytes_threshold_int_mask = 0;
#endif
		sc_int_reg->send_fifo_last_byte_int_mask = 1;
		SMC_WRITE_REG(INTR, sc_int|0x3FF);
		
		pr_dbg("write %d bytes\n", ret);

		smc_hw_start_send(smc);
	}
	
	mutex_unlock(&smc->lock);
	
	return ret;
}

static unsigned int smc_poll(struct file *filp, struct poll_table_struct *wait)
{
	smc_dev_t *smc = (smc_dev_t*)filp->private_data;
	unsigned int ret = 0;
	unsigned long flags;
	
	poll_wait(filp, &smc->rd_wq, wait);
	poll_wait(filp, &smc->wr_wq, wait);
	
	spin_lock_irqsave(&smc->slock, flags);

	if(smc->recv_start!=smc->recv_end) ret |= POLLIN|POLLRDNORM;
	if(smc->send_start==smc->send_end) ret |= POLLOUT|POLLWRNORM;
	if(!smc->cardin) ret |= POLLERR;
	
	spin_unlock_irqrestore(&smc->slock, flags);
	
	return ret;
}

#else

static int smc_read(struct file *filp,char __user *buff, size_t size, loff_t *ppos)
{
	smc_dev_t *smc = (smc_dev_t*)filp->private_data;
	unsigned long flags;
	int pos = 0, ret;
#ifdef DISABLE_RECV_INT
	unsigned long sc_int;
	SMC_INTERRUPT_Reg_t *sc_int_reg = (void *)&sc_int;
#endif

	ret = mutex_lock_interruptible(&smc->lock);
	if(ret) return ret;

#ifdef DISABLE_RECV_INT
	pr_dbg("wait write end\n");
	if(!(filp->f_flags&O_NONBLOCK)) {
		ret = wait_event_interruptible(smc->wr_wq, smc_write_end(smc));
	}
	
	if(ret==0) {
		pr_dbg("wait read buffer\n");
		
		sc_int = SMC_READ_REG(INTR);
		sc_int_reg->recv_fifo_bytes_threshold_int_mask = 1;
		sc_int_reg->send_fifo_last_byte_int_mask = 0;
		SMC_WRITE_REG(INTR, sc_int|0x3FF);
		
		if(!(filp->f_flags&O_NONBLOCK)) {
			ret = wait_event_interruptible(smc->rd_wq, smc_can_read(smc));
		}
	}
#endif
	
	if(ret==0) {
		
		spin_lock_irqsave(&smc->slock, flags);
		
		if(!smc->cardin) {
			ret = -ENODEV;
		} else if (!smc->recv_count) {
			ret = -EAGAIN;
		} else {
			ret = smc->recv_count;
			if(ret>size) ret = size;
			
			pos = smc->recv_start;
			smc->recv_start += ret;
			smc->recv_count -= ret;
			smc->recv_start %= RECV_BUF_SIZE;
		}
		spin_unlock_irqrestore(&smc->slock, flags);
	}
	
	if(ret>0) {
		int cnt = RECV_BUF_SIZE-pos;
		long cr;

		pr_dbg("read %d bytes\n", ret);

		if(cnt>=ret) {
			cr = copy_to_user(buff, smc->recv_buf+pos, ret);
		} else {
			int cnt1 = ret-cnt;
			cr = copy_to_user(buff, smc->recv_buf+pos, cnt);
			cr = copy_to_user(buff+cnt, smc->recv_buf, cnt1);
		}
	}
	
	mutex_unlock(&smc->lock);
	
	return ret;
}

static int smc_write(struct file *filp, const char __user *buff, size_t size, loff_t *offp)
{
	smc_dev_t *smc = (smc_dev_t*)filp->private_data;
	unsigned long flags;
	int pos = 0, ret;
	unsigned long sc_int;
	SMC_INTERRUPT_Reg_t *sc_int_reg = (void *)&sc_int;

	ret = mutex_lock_interruptible(&smc->lock);
	if(ret) return ret;
	
	//pr_dbg("wait write buffer\n");
	
	if(!(filp->f_flags&O_NONBLOCK)) {
		ret = wait_event_interruptible(smc->wr_wq, smc_can_write(smc));
	}
	
	if(ret==0) {
		spin_lock_irqsave(&smc->slock, flags);
		
		if(!smc->cardin) {
			ret = -ENODEV;
		} else if (smc->send_count==SEND_BUF_SIZE) {
			ret = -EAGAIN;
		} else {
			ret = SEND_BUF_SIZE-smc->send_count;
			if(ret>size) ret = size;
			
			pos = smc->send_start+smc->send_count;
			pos %= SEND_BUF_SIZE;
			smc->send_count += ret;
		}
		
		spin_unlock_irqrestore(&smc->slock, flags);
	}
	
	if(ret>0) {
		int cnt = SEND_BUF_SIZE-pos;
		long cr;

		if(cnt>=ret) {
			cr = copy_from_user(smc->send_buf+pos, buff, ret);
		} else {
			int cnt1 = ret-cnt;
			cr = copy_from_user(smc->send_buf+pos, buff, cnt);
			cr = copy_from_user(smc->send_buf, buff+cnt, cnt1);
		}
		
		sc_int = SMC_READ_REG(INTR);
#ifdef DISABLE_RECV_INT
		sc_int_reg->recv_fifo_bytes_threshold_int_mask = 0;
#endif
		sc_int_reg->send_fifo_last_byte_int_mask = 1;
		SMC_WRITE_REG(INTR, sc_int|0x3FF);
		
		pr_dbg("write %d bytes\n", ret);

		smc_hw_start_send(smc);
	}

	
	mutex_unlock(&smc->lock);
	
	return ret;
}

static unsigned int smc_poll(struct file *filp, struct poll_table_struct *wait)
{
	smc_dev_t *smc = (smc_dev_t*)filp->private_data;
	unsigned int ret = 0;
	unsigned long flags;
	
	poll_wait(filp, &smc->rd_wq, wait);
	poll_wait(filp, &smc->wr_wq, wait);
	
	spin_lock_irqsave(&smc->slock, flags);

	if(smc->recv_count) ret |= POLLIN|POLLRDNORM;
	if(smc->send_count!=SEND_BUF_SIZE) ret |= POLLOUT|POLLWRNORM;
	if(!smc->cardin) ret |= POLLERR;
	
	spin_unlock_irqrestore(&smc->slock, flags);
	
	return ret;
}

#endif

static long smc_ioctl(struct file *file, unsigned int cmd, ulong arg)
{
	smc_dev_t *smc = (smc_dev_t*)file->private_data;
	int ret = 0;
	long cr;

	switch(cmd) {
		case AMSMC_IOC_RESET:
		{
			ret = mutex_lock_interruptible(&smc->lock);
			if(ret) return ret;
			
			ret = smc_hw_reset(smc);
			if(ret>=0) {
				cr = copy_to_user((void*)arg, &smc->atr, sizeof(struct am_smc_atr));
			}
			
			mutex_unlock(&smc->lock);
		}
		break;
		case AMSMC_IOC_GET_STATUS:
		{
			int status;
			ret = smc_hw_get_status(smc, &status);
			if(ret>=0) {
				cr = copy_to_user((void*)arg, &status, sizeof(int));
			}
		}
		break;
		case AMSMC_IOC_ACTIVE:
		{
			ret = mutex_lock_interruptible(&smc->lock);
			if(ret) return ret;
			
			ret = smc_hw_active(smc);
			
			mutex_unlock(&smc->lock);
		}
		break;
		case AMSMC_IOC_DEACTIVE:
		{
			ret = mutex_lock_interruptible(&smc->lock);
			if(ret) return ret;
			
			ret = smc_hw_deactive(smc);
			
			mutex_unlock(&smc->lock);
		}
		break;
		case AMSMC_IOC_GET_PARAM:
		{
			ret = mutex_lock_interruptible(&smc->lock);
			if(ret) return ret;
			
			cr = copy_to_user((void*)arg, &smc->param, sizeof(struct am_smc_param));
			
			mutex_unlock(&smc->lock);
		}
		break;
		case AMSMC_IOC_SET_PARAM:
		{
			ret = mutex_lock_interruptible(&smc->lock);
			if(ret) return ret;
			
			cr = copy_from_user(&smc->param, (void*)arg, sizeof(struct am_smc_param));
			ret = smc_hw_set_param(smc);
			
			mutex_unlock(&smc->lock);
		}
		break;
		default:
			ret = -EINVAL;
		break;
	}
	
	return ret;
}


static struct file_operations smc_fops =
{
	.owner = THIS_MODULE,
	.open  = smc_open,
	.write = smc_write,
	.read  = smc_read,
	.release = smc_close,
	.unlocked_ioctl = smc_ioctl,
	.poll  = smc_poll
};

static int smc_probe(struct platform_device *pdev)
{
	smc_dev_t *smc = NULL;
	int i, ret;
	
	mutex_lock(&smc_lock);
	
	for (i=0; i<SMC_DEV_COUNT; i++) {
		if (!smc_dev[i].init) {
			smc = &smc_dev[i];
			break;
		}
	}
	
	if(smc) {
		smc->init = 1;
		smc->pdev = pdev;
		dev_set_drvdata(&pdev->dev, smc);
	
		if ((ret=smc_dev_init(smc, i))<0) {
			smc = NULL;
		}
	}
	
	mutex_unlock(&smc_lock);
	
	return smc ? 0 : -1;
}

static int smc_remove(struct platform_device *pdev)
{
	smc_dev_t *smc = (smc_dev_t*)dev_get_drvdata(&pdev->dev);
	
	mutex_lock(&smc_lock);
	
	smc_dev_deinit(smc);
	
	mutex_unlock(&smc_lock);
	
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id smc_dt_match[]={
	{	.compatible = "amlogic,smartcard",
	},
	{},
};
#else
#define smc_dt_match NULL
#endif

static struct platform_driver smc_driver = {
	.probe		= smc_probe,
	.remove		= smc_remove,	
	.driver		= {
		.name	= "amlogic-smc",
		.owner	= THIS_MODULE,
		.of_match_table = smc_dt_match,
	},
};

static int __init smc_mod_init(void)
{
	int ret = -1;
	
	mutex_init(&smc_lock);
	
	smc_major = register_chrdev(0, SMC_DEV_NAME, &smc_fops);
	if(smc_major<=0) {
		mutex_destroy(&smc_lock);
		pr_error("register chrdev error\n");
		goto error_register_chrdev;
	}
	
	if(class_register(&smc_class)<0) {
		pr_error("register class error\n");
		goto error_class_register;
	}
	
	if(platform_driver_register(&smc_driver)<0) {
		pr_error("register platform driver error\n");
		goto error_platform_drv_register;
	}
	
	return 0;
error_platform_drv_register:
	class_unregister(&smc_class);
error_class_register:
	unregister_chrdev(smc_major, SMC_DEV_NAME);
error_register_chrdev:
	mutex_destroy(&smc_lock);
	return ret;
}

static void __exit smc_mod_exit(void)
{
	platform_driver_unregister(&smc_driver);
	class_unregister(&smc_class);
	unregister_chrdev(smc_major, SMC_DEV_NAME);
	mutex_destroy(&smc_lock);
}

module_init(smc_mod_init);

module_exit(smc_mod_exit);

MODULE_AUTHOR("AMLOGIC");

MODULE_DESCRIPTION("AMLOGIC smart card driver");

MODULE_LICENSE("GPL");

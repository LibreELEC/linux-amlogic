#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/reboot.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/blktrans.h>
#include <plat/regops.h>

#include <mach/nand.h>
#include <mach/clock.h>
#include "version.h"
#include <linux/of.h>
#include <mach/pinmux.h>
#include <linux/pinctrl/consumer.h>
extern int nand_get_device( struct mtd_info *mtd,  int new_state);
extern void nand_release_device(struct mtd_info *mtd);


static char *aml_nand_plane_string[]={
	"NAND_SINGLE_PLANE_MODE",
	"NAND_TWO_PLANE_MODE",
};

static char *aml_nand_internal_string[]={
	"NAND_NONE_INTERLEAVING_MODE",
	"NAND_INTERLEAVING_MODE",
};

#define ECC_INFORMATION(name_a, bch_a, size_a, parity_a, user_a) {                \
        .name=name_a, .bch_mode=bch_a, .bch_unit_size=size_a, .bch_bytes=parity_a, .user_byte_mode=user_a    \
    }
static struct aml_nand_bch_desc m3_bch_list[] = {
	[0]=ECC_INFORMATION("NAND_RAW_MODE", NAND_ECC_SOFT_MODE, 0, 0, 0),
	[1]=ECC_INFORMATION("NAND_SHORT_MODE" ,NAND_ECC_SHORT_MODE, NAND_ECC_UNIT_SHORT, NAND_BCH60_1K_ECC_SIZE, 2),
	[2]=ECC_INFORMATION("NAND_BCH8_MODE", NAND_ECC_BCH8_MODE, NAND_ECC_UNIT_SIZE, NAND_BCH8_ECC_SIZE, 2),
	[3]=ECC_INFORMATION("NAND_BCH24_1K_MODE" ,NAND_ECC_BCH24_1K_MODE, NAND_ECC_UNIT_1KSIZE, NAND_BCH24_1K_ECC_SIZE, 2),
	[4]=ECC_INFORMATION("NAND_BCH30_1K_MODE" ,NAND_ECC_BCH30_1K_MODE, NAND_ECC_UNIT_1KSIZE, NAND_BCH30_1K_ECC_SIZE, 2),
	[5]=ECC_INFORMATION("NAND_BCH40_1K_MODE" ,NAND_ECC_BCH40_1K_MODE, NAND_ECC_UNIT_1KSIZE, NAND_BCH40_1K_ECC_SIZE, 2),
	[6]=ECC_INFORMATION("NAND_BCH60_1K_MODE" ,NAND_ECC_BCH60_1K_MODE, NAND_ECC_UNIT_1KSIZE, NAND_BCH60_1K_ECC_SIZE, 2),
};

#ifdef MX_REVD
unsigned char pagelist_hynix256[128] = {
	0x00, 0x01, 0x02, 0x03, 0x06, 0x07, 0x0A, 0x0B,
	0x0E, 0x0F, 0x12, 0x13, 0x16, 0x17, 0x1A, 0x1B,
	0x1E, 0x1F, 0x22, 0x23, 0x26, 0x27, 0x2A, 0x2B,
	0x2E, 0x2F, 0x32, 0x33, 0x36, 0x37, 0x3A, 0x3B,

	0x3E, 0x3F, 0x42, 0x43, 0x46, 0x47, 0x4A, 0x4B,
	0x4E, 0x4F, 0x52, 0x53, 0x56, 0x57, 0x5A, 0x5B,
	0x5E, 0x5F, 0x62, 0x63, 0x66, 0x67, 0x6A, 0x6B,
	0x6E, 0x6F, 0x72, 0x73, 0x76, 0x77, 0x7A, 0x7B,

	0x7E, 0x7F, 0x82, 0x83, 0x86, 0x87, 0x8A, 0x8B,
	0x8E, 0x8F, 0x92, 0x93, 0x96, 0x97, 0x9A, 0x9B,
	0x9E, 0x9F, 0xA2, 0xA3, 0xA6, 0xA7, 0xAA, 0xAB,
	0xAE, 0xAF, 0xB2, 0xB3, 0xB6, 0xB7, 0xBA, 0xBB,

	0xBE, 0xBF, 0xC2, 0xC3, 0xC6, 0xC7, 0xCA, 0xCB,
	0xCE, 0xCF, 0xD2, 0xD3, 0xD6, 0xD7, 0xDA, 0xDB,
	0xDE, 0xDF, 0xE2, 0xE3, 0xE6, 0xE7, 0xEA, 0xEB,
	0xEE, 0xEF, 0xF2, 0xF3, 0xF6, 0xF7, 0xFA, 0xFB,
};
unsigned char pagelist_1ynm_hynix256[128] = {
	0x00, 0x01, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d,
	0x0f, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1b, 0x1d,
	0x1f, 0x21, 0x23, 0x25, 0x27, 0x29, 0x2b, 0x2d,
	0x2f, 0x31, 0x33, 0x35, 0x37, 0x39, 0x3b, 0x3d,
	0x3f, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4b, 0x4d,
	0x4f, 0x51, 0x53, 0x55, 0x57, 0x59, 0x5b, 0x5d,
	0x5f, 0x61, 0x63, 0x65, 0x67, 0x69, 0x6b, 0x6d,
	0x6f, 0x71, 0x73, 0x75, 0x77, 0x79, 0x7b, 0x7d,
	0x7f, 0x81, 0x83, 0x85, 0x87, 0x89, 0x8b, 0x8d,
	0x8f, 0x91, 0x93, 0x95, 0x97, 0x99, 0x9b, 0x9d,
	0x9f, 0xa1, 0xA3, 0xA5, 0xA7, 0xA9, 0xAb, 0xAd,
	0xAf, 0xb1, 0xB3, 0xB5, 0xB7, 0xB9, 0xBb, 0xBd,
	0xBf, 0xc1, 0xC3, 0xC5, 0xC7, 0xC9, 0xCb, 0xCd,
	0xCf, 0xd1, 0xD3, 0xD5, 0xD7, 0xD9, 0xDb, 0xDd,
	0xDf, 0xe1, 0xE3, 0xE5, 0xE7, 0xE9, 0xEb, 0xEd,
	0xEf, 0xf1, 0xF3, 0xF5, 0xF7, 0xF9, 0xFb, 0xFd,
};
#endif
static unsigned char mx_revd_flag = 0;
static unsigned mx_nand_check_chiprevd(void)
{
    printk("checking ChiprevD :%d\n", mx_revd_flag);
    return mx_revd_flag;
}
static int __init check_chiprevd(char *str)
{
    printk("cheked chiprev : %s\n", str);
    mx_revd_flag = 0;
    if(str[0] == 'D'){
        mx_revd_flag = 1;
    }
    printk("checking ChiprevD :%d\n", mx_revd_flag);
    return 0;
}
early_param("chiprev", check_chiprevd);
static struct aml_nand_device *to_nand_dev(struct platform_device *pdev)
{
	return pdev->dev.platform_data;
}
#if 0
static pinmux_item_t nand_ce0_pins[] = {
    {
        .reg = PINMUX_REG(2),
        .setmask = 1<<25,
    },
    PINMUX_END_ITEM
};

static pinmux_item_t nand_ce1_pins[] = {
	{
        .reg = PINMUX_REG(2),
        .setmask = 1<<24,
    },
    PINMUX_END_ITEM
};

 static pinmux_item_t nand_ce2_pins[] = {
	{
        .reg = PINMUX_REG(2),
        .setmask = 1<<23,
    },
    PINMUX_END_ITEM
};

 static pinmux_item_t nand_ce3_pins[] = {
	{
        .reg = PINMUX_REG(2),
        .setmask = 1<<22,
    },
    PINMUX_END_ITEM
};

static pinmux_set_t nand_ce0 = {
    .chip_select = NULL,
    .pinmux = &nand_ce0_pins[0]
};
static pinmux_set_t nand_ce1 = {
    .chip_select = NULL,
    .pinmux = &nand_ce1_pins[0]
};
static pinmux_set_t nand_ce2 = {
    .chip_select = NULL,
    .pinmux = &nand_ce2_pins[0]
};
static pinmux_set_t nand_ce3 = {
    .chip_select = NULL,
    .pinmux = &nand_ce3_pins[0]
};
#endif
static pinmux_item_t nand_rb0_pins[] = {
    {
        .reg = PINMUX_REG(2),
        .setmask = 1<<17,
    },
    PINMUX_END_ITEM
};

static pinmux_item_t nand_rb1_pins[] ={
	{
        .reg = PINMUX_REG(2),
        .setmask = 1<<16,
    },
    PINMUX_END_ITEM
};

static pinmux_set_t nand_rb0 = {
    .chip_select = NULL,
    .pinmux = &nand_rb0_pins[0]
};

static pinmux_set_t nand_rb1 = {
    .chip_select = NULL,
    .pinmux = &nand_rb1_pins[0]
};



static struct pinctrl *p_pictl=NULL;
struct device *nand_dev;
void aml_pinmux_set(const char *name)
{
	struct pinctrl_state *s;
	int ret;

	if(!p_pictl){
		p_pictl=devm_pinctrl_get(nand_dev);
		if(IS_ERR(p_pictl))
			printk("set pinmux error!\n");
	}
	s = pinctrl_lookup_state(p_pictl,name);
			if (IS_ERR(s)) {
				pinctrl_put(p_pictl);
				p_pictl=NULL;
			}
			ret = pinctrl_select_state(p_pictl, s);
			if (ret < 0) {
				pinctrl_put(p_pictl);
				p_pictl=NULL;
			}
}


static void m3_nand_select_chip(struct aml_nand_chip *aml_chip, int chipnr)
{
//	int i;
	//struct device *nand_dev = aml_chip->device ;
	switch (chipnr) {
		case 0:
		case 1:
		case 2:
		case 3:

			aml_chip->chip_selected = aml_chip->chip_enable[chipnr];
			aml_chip->rb_received = aml_chip->rb_enable[chipnr];
#if 0
			for (i=0; i<aml_chip->chip_num; i++) {

				if (aml_chip->valid_chip[i]) {
					if (!((aml_chip->chip_enable[i] >> 10) & 1))
						aml_pinmux_set(nand_dev,&nand_ce0);


					if (!((aml_chip->chip_enable[i] >> 10) & 2))
						aml_pinmux_set(nand_dev,&nand_ce1);
						#if 0
					if (!((aml_chip->chip_enable[i] >> 10) & 4))
						//pinmux_set(&nand_ce2);
						;
					if (!((aml_chip->chip_enable[i] >> 10) & 8))
						//pinmux_set(&nand_ce3);
						;
					#endif
					if (((aml_chip->ops_mode & AML_CHIP_NONE_RB) == 0) && (aml_chip->rb_enable[i])){
						if (!((aml_chip->rb_enable[i] >> 10) & 1))
							aml_pinmux_set(nand_dev,&nand_rb0);

						if (!((aml_chip->rb_enable[i] >> 10) & 2))
							aml_pinmux_set(nand_dev,&nand_rb1);

					}
				}
			}
#endif
			NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);

			break;

		default:
			BUG();
			aml_chip->chip_selected = CE_NOT_SEL;
			break;
	}
	return;
}

static void m3_nand_hw_init(struct aml_nand_chip *aml_chip)
{
	struct clk *sys_clk;
	int sys_clk_rate, sys_time, start_cycle, end_cycle, bus_cycle, bus_timing, Tcycle, T_REA = DEFAULT_T_REA, T_RHOH = DEFAULT_T_RHOH;

	sys_clk = clk_get_sys(NAND_SYS_CLK_NAME, NULL);
	sys_clk_rate = clk_get_rate(sys_clk);
	sys_time = (10000 / (sys_clk_rate / 1000000));

	start_cycle = (((NAND_CYCLE_DELAY + T_REA * 10) * 10) / sys_time);
	start_cycle = (start_cycle + 9) / 10;

	for (bus_cycle = 4; bus_cycle <= MAX_CYCLE_NUM; bus_cycle++) {
		Tcycle = bus_cycle * sys_time;
		end_cycle = (((NAND_CYCLE_DELAY + Tcycle / 2 + T_RHOH * 10) * 10) / sys_time);
		end_cycle = end_cycle / 10;
		if ((((start_cycle >= 3) && (start_cycle <= ( bus_cycle + 1)))
			|| ((end_cycle >= 3) && (end_cycle <= (bus_cycle + 1))))
			&& (start_cycle <= end_cycle)) {
			break;
		}
	}
	if (bus_cycle > MAX_CYCLE_NUM)
		return;

	bus_timing = (start_cycle + end_cycle) / 2;
	NFC_SET_CFG(0);
	NFC_SET_TIMING_ASYC(bus_timing, (bus_cycle - 1));
	NFC_SEND_CMD(1<<31);
	printk("init bus_cycle=%d, bus_timing=%d, start_cycle=%d, end_cycle=%d,system=%d.%dns\n",
		bus_cycle, bus_timing, start_cycle, end_cycle, sys_time/10, sys_time%10);
	return;
}

static void m3_nand_adjust_timing(struct aml_nand_chip *aml_chip)
{
	struct clk *sys_clk;
	int sys_clk_rate, sys_time, start_cycle, end_cycle, bus_cycle, bus_timing, Tcycle;

	if (!aml_chip->T_REA)
		aml_chip->T_REA = 20;
	if (!aml_chip->T_RHOH)
		aml_chip->T_RHOH = 15;

	sys_clk = clk_get_sys(NAND_SYS_CLK_NAME, NULL);
	sys_clk_rate = clk_get_rate(sys_clk);
	sys_time = (10000 / (sys_clk_rate / 1000000));
	start_cycle = (((NAND_CYCLE_DELAY + aml_chip->T_REA * 10) * 10) / sys_time);
	start_cycle = (start_cycle + 9) / 10;

	for (bus_cycle = 4; bus_cycle <= MAX_CYCLE_NUM; bus_cycle++) {
		Tcycle = bus_cycle * sys_time;
		end_cycle = (((NAND_CYCLE_DELAY + Tcycle / 2 + aml_chip->T_RHOH * 10) * 10) / sys_time);
		end_cycle = end_cycle / 10;
		if ((((start_cycle >= 3) && (start_cycle <= ( bus_cycle + 1)))
			|| ((end_cycle >= 3) && (end_cycle <= (bus_cycle + 1))))
			&& (start_cycle <= end_cycle)) {
			break;
		}
	}
	if (bus_cycle > MAX_CYCLE_NUM)
		return;

	bus_timing = (start_cycle + end_cycle) / 2;
	NFC_SET_CFG(0);
	NFC_SET_TIMING_ASYC(bus_timing, (bus_cycle - 1));
	NFC_SEND_CMD(1<<31);
	printk("bus_cycle=%d, bus_timing=%d, start_cycle=%d, end_cycle=%d,system=%d.%dns\n",
		bus_cycle, bus_timing, start_cycle, end_cycle, sys_time/10, sys_time%10);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
#if 0
static void m3_nand_early_suspend(struct early_suspend *nand_early_suspend)
{
	printk("m3_nand_early suspend entered\n");
	return;
}

static void m3_nand_late_resume(struct early_suspend *nand_early_suspend)
{
	printk("m3_nand_late resume entered\n");
	return;
}
#endif
#endif

static int m3_nand_suspend(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct aml_nand_platform *plat = aml_chip->platform;
	struct nand_chip *chip = &aml_chip->chip;
	spinlock_t *lock = &chip->controller->lock;

	if (!strncmp((char*)plat->name, NAND_BOOT_NAME, strlen((const char*)NAND_BOOT_NAME)))
		return 0;

	spin_lock(lock);
	if (!chip->controller->active)
		chip->controller->active = chip;
	chip->state = FL_PM_SUSPENDED;
	spin_unlock(lock);

	printk("m3 nand suspend entered\n");
	return 0;
}

static void m3_nand_resume(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct aml_nand_platform *plat = aml_chip->platform;
	struct nand_chip *chip = &aml_chip->chip;
	u8 onfi_features[4];

	if (!strncmp((char*)plat->name, NAND_BOOT_NAME, strlen((const char*)NAND_BOOT_NAME)))
		return;

	chip->select_chip(mtd, 0);
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	if (aml_chip->onfi_mode) {
		aml_nand_set_onfi_features(aml_chip, (uint8_t *)(&aml_chip->onfi_mode), ONFI_TIMING_ADDR);
		aml_nand_get_onfi_features(aml_chip, onfi_features, ONFI_TIMING_ADDR);
		if (onfi_features[0] != aml_chip->onfi_mode) {
			aml_chip->T_REA = DEFAULT_T_REA;
			aml_chip->T_RHOH = DEFAULT_T_RHOH;
			printk("onfi timing mode set failed: %x\n", onfi_features[0]);
		}
	}
	if (chip->state == FL_PM_SUSPENDED)
		nand_release_device(mtd);

//	chip->select_chip(mtd, -1);
//	spin_lock(&chip->controller->lock);
//	chip->controller->active = NULL;
//	chip->state = FL_READY;
//	wake_up(&chip->controller->wq);
//	spin_unlock(&chip->controller->lock);

	printk("m3 nand resume entered\n");
	return;
}

static int m3_nand_options_confirm(struct aml_nand_chip *aml_chip)
{
	struct mtd_info *mtd = &aml_chip->mtd;
	struct nand_chip *chip = &aml_chip->chip;
	struct aml_nand_platform *plat = aml_chip->platform;
	struct aml_nand_bch_desc *ecc_supports = aml_chip->bch_desc;
	unsigned max_bch_mode = aml_chip->max_bch_mode;
	unsigned options_selected = 0, options_support = 0, ecc_bytes, options_define, valid_chip_num = 0;
	int error = 0, i, j;

	options_selected = (plat->platform_nand_data.chip.options & NAND_ECC_OPTIONS_MASK);
	options_define = (aml_chip->options & NAND_ECC_OPTIONS_MASK);

	for (i=0; i<max_bch_mode; i++) {
		if (ecc_supports[i].bch_mode == options_selected) {
			break;
		}
	}
	j = i;

    for(i=max_bch_mode-1; i>0; i--)
    {
        ecc_bytes = aml_chip->oob_size / (aml_chip->page_size / ecc_supports[i].bch_unit_size);
        if(ecc_bytes >= ecc_supports[i].bch_bytes + ecc_supports[i].user_byte_mode)
        {
            options_support = ecc_supports[i].bch_mode;
            break;
        }
    }

	if (options_define != options_support) {
		options_define = options_support;
		//printk("define oob size: %d could support bch mode: %s\n", aml_chip->oob_size, ecc_supports[options_support].name);
	}

	if (options_selected > options_define) {
		printk("oob size is not enough for selected bch mode: %s force bch to mode: %s\n", ecc_supports[j].name, ecc_supports[i].name);
		options_selected = options_define;
	}
	aml_chip->oob_fill_cnt = aml_chip->oob_size -(ecc_supports[i].bch_bytes + ecc_supports[i].user_byte_mode)*(aml_chip->page_size / ecc_supports[i].bch_unit_size);
	printk("aml_chip->oob_fill_cnt =%d,aml_chip->oob_size =%d,bch_bytes =%d\n",aml_chip->oob_fill_cnt,aml_chip->oob_size,ecc_supports[i].bch_bytes);

	switch (options_selected) {

		case NAND_ECC_BCH8_MODE:
			chip->ecc.size = NAND_ECC_UNIT_SIZE;
			chip->ecc.bytes = NAND_BCH8_ECC_SIZE;
			aml_chip->bch_mode = NAND_ECC_BCH8;
			aml_chip->user_byte_mode = 2;
			chip->ecc.steps = mtd->writesize / chip->ecc.size;
			break;



		case NAND_ECC_BCH24_1K_MODE:
			chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
			chip->ecc.bytes = NAND_BCH24_1K_ECC_SIZE;
			aml_chip->bch_mode = NAND_ECC_BCH24_1K;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH30_1K_MODE:
			chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
			chip->ecc.bytes = NAND_BCH30_1K_ECC_SIZE;
			aml_chip->bch_mode = NAND_ECC_BCH30_1K;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH40_1K_MODE:
			chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
			chip->ecc.bytes = NAND_BCH40_1K_ECC_SIZE;
			aml_chip->bch_mode = NAND_ECC_BCH40_1K;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH60_1K_MODE:
			chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
			chip->ecc.bytes = NAND_BCH60_1K_ECC_SIZE;
			aml_chip->bch_mode = NAND_ECC_BCH60_1K;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_SHORT_MODE:
			chip->ecc.size = NAND_ECC_UNIT_SHORT;
			chip->ecc.bytes = NAND_BCH60_1K_ECC_SIZE;
			aml_chip->bch_mode = NAND_ECC_BCH_SHORT;
			aml_chip->user_byte_mode = 2;
			chip->ecc.steps = mtd->writesize / 512;
			break;

		default :
			if ((plat->platform_nand_data.chip.options & NAND_ECC_OPTIONS_MASK) != NAND_ECC_SOFT_MODE) {
				printk("soft ecc or none ecc just support in linux self nand base please selected it at platform options\n");
				error = -ENXIO;
			}
			break;
	}

	options_selected = (plat->platform_nand_data.chip.options & NAND_INTERLEAVING_OPTIONS_MASK);
	options_define = (aml_chip->options & NAND_INTERLEAVING_OPTIONS_MASK);
	if (options_selected > options_define) {
		printk("internal mode error for selected internal mode: %s force internal mode to : %s\n", aml_nand_internal_string[options_selected >> 16], aml_nand_internal_string[options_define >> 16]);
		options_selected = options_define;
	}

	switch (options_selected) {

		case NAND_INTERLEAVING_MODE:
			aml_chip->ops_mode |= AML_INTERLEAVING_MODE;
			mtd->erasesize *= aml_chip->internal_chipnr;
			mtd->writesize *= aml_chip->internal_chipnr;
			mtd->oobsize *= aml_chip->internal_chipnr;
			break;

		default:
			break;
	}
	options_selected = (plat->platform_nand_data.chip.options & NAND_PLANE_OPTIONS_MASK);
	options_define = (aml_chip->options & NAND_PLANE_OPTIONS_MASK);
	if (options_selected > options_define) {
		printk("multi plane error for selected plane mode: %s force plane to : %s\n", aml_nand_plane_string[options_selected >> 4], aml_nand_plane_string[options_define >> 4]);
		options_selected = options_define;
	}

	for (i=0; i<aml_chip->chip_num; i++) {
		if (aml_chip->valid_chip[i]) {
		    valid_chip_num++;
		}
	}

	if (aml_chip->ops_mode & AML_INTERLEAVING_MODE)
		valid_chip_num *= aml_chip->internal_chipnr;

	if(valid_chip_num > 2){
		aml_chip->plane_num = 1;
		printk("detect valid_chip_num:%d over 2, and aml_chip->internal_chipnr:%d, disable NAND_TWO_PLANE_MODE here\n", valid_chip_num, aml_chip->internal_chipnr);
	}
	else{
		switch (options_selected) {

			case NAND_TWO_PLANE_MODE:
				aml_chip->plane_num = 2;
				mtd->erasesize *= 2;
				mtd->writesize *= 2;
				mtd->oobsize *= 2;
				break;

			default:
				aml_chip->plane_num = 1;
				break;
		}
	}

	return error;
}


static int aml_platform_dma_waiting(struct aml_nand_chip *aml_chip)
{
	unsigned time_out_cnt = 0;

	NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);
	NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);
	do {
		if (NFC_CMDFIFO_SIZE() <= 0)
			break;
	}while (time_out_cnt++ <= AML_DMA_BUSY_TIMEOUT);

	if (time_out_cnt < AML_DMA_BUSY_TIMEOUT)
		return 0;

	return -EBUSY;
}

static int m3_nand_dma_write(struct aml_nand_chip *aml_chip, unsigned char *buf, int len, unsigned bch_mode)
{
	int ret = 0;
	unsigned dma_unit_size = 0, count = 0;
	struct nand_chip *chip = &aml_chip->chip;
    struct mtd_info *mtd = &aml_chip->mtd;

	memcpy(aml_chip->aml_nand_data_buf, buf, len);
    smp_wmb();
	wmb();

	if (bch_mode == NAND_ECC_NONE)
		count = 1;
	else if (bch_mode == NAND_ECC_BCH_SHORT) {
		dma_unit_size = (chip->ecc.size >> 3);
		count = len/chip->ecc.size;
	}
	else
		count = len/chip->ecc.size;
#ifdef CONFIG_CLK81_DFS
    down(&aml_chip->nand_sem);
#endif
	NFC_SEND_CMD_ADL(aml_chip->data_dma_addr);
	NFC_SEND_CMD_ADH(aml_chip->data_dma_addr);
	NFC_SEND_CMD_AIL(aml_chip->nand_info_dma_addr);
	NFC_SEND_CMD_AIH((aml_chip->nand_info_dma_addr));

	if(aml_chip->ran_mode){
	      if(aml_chip->plane_num == 2)
	        NFC_SEND_CMD_SEED((aml_chip->page_addr/(mtd->writesize >> chip->page_shift)) * (mtd->writesize >> chip->page_shift));
	    else
		NFC_SEND_CMD_SEED(aml_chip->page_addr);
	}
	if(!bch_mode)
		NFC_SEND_CMD_M2N_RAW(0, len);
	else
		NFC_SEND_CMD_M2N(aml_chip->ran_mode, ((bch_mode == NAND_ECC_BCH_SHORT)?NAND_ECC_BCH60_1K:bch_mode), ((bch_mode == NAND_ECC_BCH_SHORT)?1:0), dma_unit_size, count);

	ret = aml_platform_dma_waiting(aml_chip);

	if(aml_chip->oob_fill_cnt >0) {
		NFC_SEND_CMD_M2N_RAW(aml_chip->ran_mode, aml_chip->oob_fill_cnt);
		ret = aml_platform_dma_waiting(aml_chip);
	}
#ifdef CONFIG_CLK81_DFS
    up(&aml_chip->nand_sem);
#endif
	return ret;
}

static int m3_nand_dma_read(struct aml_nand_chip *aml_chip, unsigned char *buf, int len, unsigned bch_mode)
{
	volatile unsigned int * info_buf=NULL;
	//volatile int cmp=0;

	struct nand_chip *chip = &aml_chip->chip;
	unsigned dma_unit_size = 0, count = 0, info_times_int_len;
	int ret = 0;
	struct mtd_info *mtd = &aml_chip->mtd;

	info_times_int_len = PER_INFO_BYTE/sizeof(unsigned int);
	if (bch_mode == NAND_ECC_NONE)
		count = 1;
	else if (bch_mode == NAND_ECC_BCH_SHORT) {
		dma_unit_size = (chip->ecc.size >> 3);
		count = len/chip->ecc.size;
	}
	else
		count = len/chip->ecc.size;

	info_buf = (volatile unsigned *)&(aml_chip->user_info_buf[(count-1)*info_times_int_len]);
	memset((unsigned char *)aml_chip->user_info_buf, 0, count*PER_INFO_BYTE);
	smp_wmb();
	wmb();
#ifdef CONFIG_CLK81_DFS
    down(&aml_chip->nand_sem);
#endif
	NFC_SEND_CMD_ADL(aml_chip->data_dma_addr);
	NFC_SEND_CMD_ADH(aml_chip->data_dma_addr);
	NFC_SEND_CMD_AIL(aml_chip->nand_info_dma_addr);
	NFC_SEND_CMD_AIH(aml_chip->nand_info_dma_addr);
	if(aml_chip->ran_mode){
			if(aml_chip->plane_num == 2)
	        NFC_SEND_CMD_SEED((aml_chip->page_addr/(mtd->writesize >> chip->page_shift)) * (mtd->writesize >> chip->page_shift));
	    else
		NFC_SEND_CMD_SEED(aml_chip->page_addr);
	}
	if(bch_mode == NAND_ECC_NONE)
		NFC_SEND_CMD_N2M_RAW(aml_chip->ran_mode,len);
	else
		NFC_SEND_CMD_N2M(aml_chip->ran_mode, ((bch_mode == NAND_ECC_BCH_SHORT)?NAND_ECC_BCH60_1K:bch_mode), ((bch_mode == NAND_ECC_BCH_SHORT)?1:0), dma_unit_size, count);
	ret = aml_platform_dma_waiting(aml_chip);
#ifdef CONFIG_CLK81_DFS
    up(&aml_chip->nand_sem);
#endif
	if (ret)
		return ret;
	/*do{
		info_buf = (volatile unsigned *)&(aml_chip->user_info_buf[(count-1)*info_times_int_len]);
		cmp = *info_buf;
	}while((cmp)==0);*/
	do{
	    smp_rmb();
	}while(NAND_INFO_DONE(aml_read_reg32((unsigned) info_buf)) == 0);

	smp_rmb();
	if (buf != aml_chip->aml_nand_data_buf)
		memcpy(buf, aml_chip->aml_nand_data_buf, len);
	smp_wmb();
	wmb();

	return 0;
}

static int m3_nand_hwecc_correct(struct aml_nand_chip *aml_chip, unsigned char *buf, unsigned size, unsigned char *oob_buf)
{
	struct nand_chip *chip = &aml_chip->chip;
	unsigned ecc_step_num;
	unsigned info_times_int_len = PER_INFO_BYTE/sizeof(unsigned int);

	if (size % chip->ecc.size) {
		printk ("error parameter size for ecc correct %x\n", size);
		return -EINVAL;
	}

	aml_chip->ecc_cnt_cur = 0;
	 for (ecc_step_num = 0; ecc_step_num < (size / chip->ecc.size); ecc_step_num++) {
		//check if there have uncorrectable sector
		if(NAND_ECC_CNT(aml_read_reg32((unsigned )(&aml_chip->user_info_buf[ecc_step_num*info_times_int_len]))) == 0x3f)
		{
		aml_chip->zero_cnt = NAND_ZERO_CNT(*(unsigned *)(&aml_chip->user_info_buf[ecc_step_num*info_times_int_len]));
			//printk ("nand communication have uncorrectable ecc error %d\n", ecc_step_num);
			return -EIO;
		}
		else {
			//mtd->ecc_stats.corrected += NAND_ECC_CNT(*(unsigned *)(&aml_chip->user_info_buf[ecc_step_num*info_times_int_len]));
			aml_chip->ecc_cnt_cur = NAND_ECC_CNT(*(unsigned *)(&aml_chip->user_info_buf[ecc_step_num*info_times_int_len]));
		}
	}

	return 0;
}

 void m3_nand_boot_erase_cmd(struct mtd_info *mtd, int page)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = mtd->priv;
	loff_t ofs;
	int i, page_addr;

	if (page >= M3_BOOT_PAGES_PER_COPY)
		return;

	if (aml_chip->valid_chip[0]) {

		for (i=0; i<M3_BOOT_COPY_NUM; i++) {
			page_addr = page + i*M3_BOOT_PAGES_PER_COPY;
			ofs = (page_addr << chip->page_shift);

			if (chip->block_bad(mtd, ofs, 0))
				continue;

			aml_chip->aml_nand_select_chip(aml_chip, 0);
			aml_chip->aml_nand_command(aml_chip, NAND_CMD_ERASE1, -1, page_addr, i);
			aml_chip->aml_nand_command(aml_chip, NAND_CMD_ERASE2, -1, -1, i);
			chip->waitfunc(mtd, chip);
		}
	}

	return ;
}

 int m3_nand_boot_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf, int oob_required,int page)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t *oob_buf = chip->oob_poi;
	unsigned nand_page_size = chip->ecc.steps * chip->ecc.size;
	unsigned pages_per_blk_shift = (chip->phys_erase_shift - chip->page_shift);
	int user_byte_num = (chip->ecc.steps * aml_chip->user_byte_mode);
	int bch_mode = aml_chip->bch_mode, ran_mode, read_page;
	int error = 0, i = 0, stat = 0;
	int en_slc = 0;
#ifdef MX_REVD
	en_slc = ((aml_chip->new_nand_info.type < 10)&&(aml_chip->new_nand_info.type))? 1:0;
 #endif

    if(en_slc){
	if (page >= (M3_BOOT_PAGES_PER_COPY/2 - 1))
	{
		memset(buf, 0, (1 << chip->page_shift));
		//printk("line:%d nand boot read out of uboot failed, page:%d\n", __LINE__, page);
		goto exit;
	}
    }
    else{

		if (page >= (M3_BOOT_PAGES_PER_COPY - 1))
	{
		memset(buf, 0, (1 << chip->page_shift));
		//printk("line:%d nand boot read out of uboot failed, page:%d\n", __LINE__, page);
		goto exit;
	}
    }


	read_page = page;
	read_page++;

#ifdef MX_REVD
			if(en_slc){
				read_page = page%M3_BOOT_PAGES_PER_COPY;
			if(aml_chip->new_nand_info.type == HYNIX_1YNM_8GB)
				read_page = pagelist_1ynm_hynix256[read_page+1] + (page/M3_BOOT_PAGES_PER_COPY)*M3_BOOT_PAGES_PER_COPY;
			else
				read_page = pagelist_hynix256[read_page+1] + (page/M3_BOOT_PAGES_PER_COPY)*M3_BOOT_PAGES_PER_COPY;
			}
#endif


        chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, read_page);
	/*
	if (((page % M3_BOOT_PAGES_PER_COPY) == 0) && (aml_chip->bch_mode != NAND_ECC_BCH_SHORT)) {
		nand_page_size = (mtd->writesize / 512) * NAND_ECC_UNIT_SHORT;
		bch_mode = NAND_ECC_BCH_SHORT;
		chip->ecc.size = NAND_ECC_UNIT_SHORT;
	}
	else
		bch_mode = aml_chip->bch_mode;*/
	//printk("%s:%d,bch_mode =%d,page =%d\n",__func__,__LINE__,bch_mode,page);
	memset(buf, 0xff, (1 << chip->page_shift));
	if (aml_chip->valid_chip[i]) {

		if (!aml_chip->aml_nand_wait_devready(aml_chip, i)) {
                printk("read couldn`t found selected chip0 ready, page: %d \n", page);
			error = -EBUSY;
			goto exit;
		}

		if (aml_chip->ops_mode & AML_CHIP_NONE_RB)
			chip->cmd_ctrl(mtd, NAND_CMD_READ0 & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

		error = aml_chip->aml_nand_dma_read(aml_chip, buf, nand_page_size, bch_mode);
		if (error)
			goto exit;

		aml_chip->aml_nand_get_user_byte(aml_chip, oob_buf, user_byte_num);
		stat = aml_chip->aml_nand_hwecc_correct(aml_chip, buf, nand_page_size, oob_buf);
		if (stat < 0) {
			mtd->ecc_stats.failed++;
			printk("aml nand boot read data ecc failed at blk %d page:%d chip %d\n", (page >> pages_per_blk_shift), page, i);
		}
		else
			mtd->ecc_stats.corrected += stat;
	}
	else {
		error = -ENODEV;
		goto exit;
	}

exit:

	return error;
}

 void m3_nand_boot_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf,int oob_required)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t *oob_buf = chip->oob_poi;
	unsigned nand_page_size = chip->ecc.steps * chip->ecc.size;
	int user_byte_num = (chip->ecc.steps * aml_chip->user_byte_mode);
	int error = 0, i = 0, bch_mode, ecc_size;

	ecc_size = chip->ecc.size;
	if (((aml_chip->page_addr % M3_BOOT_PAGES_PER_COPY) == 0) && (aml_chip->bch_mode != NAND_ECC_BCH_SHORT)) {
		nand_page_size = (mtd->writesize / 512) * NAND_ECC_UNIT_SHORT;
		bch_mode = NAND_ECC_BCH_SHORT;
		chip->ecc.size = NAND_ECC_UNIT_SHORT;
	}
	else
		bch_mode = aml_chip->bch_mode;

	for (i=0; i<mtd->oobavail; i+=2) {
		oob_buf[i] = 0x55;
		oob_buf[i+1] = 0xaa;
	}
	i = 0;
	if (aml_chip->valid_chip[i]) {

		aml_chip->aml_nand_select_chip(aml_chip, i);

		aml_chip->aml_nand_set_user_byte(aml_chip, oob_buf, user_byte_num);
		error = aml_chip->aml_nand_dma_write(aml_chip, (unsigned char *)buf, nand_page_size, bch_mode);
		if (error)
			goto exit;
		aml_chip->aml_nand_command(aml_chip, NAND_CMD_PAGEPROG, -1, -1, i);
	}
	else {
		error = -ENODEV;
		goto exit;
	}

exit:
	if (((aml_chip->page_addr % M3_BOOT_PAGES_PER_COPY) == 0) && (aml_chip->bch_mode != NAND_ECC_BCH_SHORT))
		chip->ecc.size = ecc_size;

	return;
}

 int m3_nand_boot_write_page(struct mtd_info *mtd, struct nand_chip *chip,uint32_t offset, int data_len, const uint8_t *buf,int oob_required, int page, int cached, int raw)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	int status, i, write_page, configure_data, pages_per_blk, write_page_tmp, ran_mode;
	int new_nand_type = 0;
	int en_slc = 0;
#ifdef MX_REVD
	int magic = NAND_PAGELIST_MAGIC;
	int page_list[6] = {0x01, 0x02, 0x03, 0x06, 0x07, 0x0A};
	unsigned priv_slc_page,next_msb_page;
	unsigned char *fill_buf =NULL;
#endif

#ifdef MX_REVD
	if(aml_chip->new_nand_info.type == HYNIX_1YNM_8GB) {
		page_list[1] = 0X03;page_list[2] = 0X05;page_list[3] = 0X07;
		page_list[4] = 0X09;page_list[5] = 0X0B;
	}
	new_nand_type = aml_chip->new_nand_info.type;
	en_slc = ((aml_chip->new_nand_info.type < 10)&&(aml_chip->new_nand_info.type))? 1:0;
	if(new_nand_type == HYNIX_1YNM_8GB) {
		fill_buf = kzalloc(mtd->writesize, GFP_KERNEL);
		if (fill_buf == NULL) {
			printk("malloc fill buf fail \n");
			return -ENOMEM;
		}
		memset(fill_buf, 0xff, mtd->writesize);
	}
#endif

	if(en_slc){
		if (page >= (M3_BOOT_PAGES_PER_COPY/2 - 1))
			return 0;
#ifdef MX_REVD
		if(aml_chip->new_nand_info.slc_program_info.enter_enslc_mode)
		   aml_chip->new_nand_info.slc_program_info.enter_enslc_mode(mtd);
#endif
	}
	else{
		if (page >= (M3_BOOT_PAGES_PER_COPY - 1))
			return 0;
	}

	pages_per_blk = (1 << (chip->phys_erase_shift - chip->page_shift));
	for (i=0; i<M3_BOOT_COPY_NUM; i++) {

		write_page = page + i*M3_BOOT_PAGES_PER_COPY;
		if ((write_page % M3_BOOT_PAGES_PER_COPY) == 0) {
			if (aml_chip->bch_mode == NAND_ECC_BCH_SHORT)
				configure_data = NFC_CMD_N2M(aml_chip->ran_mode, NAND_ECC_BCH60_1K, 1, (chip->ecc.size >> 3), chip->ecc.steps);
			else
				configure_data = NFC_CMD_N2M(aml_chip->ran_mode, aml_chip->bch_mode, 0, (chip->ecc.size >> 3), chip->ecc.steps);

			memset(chip->buffers->databuf, 0xbb, mtd->writesize);
			memcpy(chip->buffers->databuf, (unsigned char *)(&configure_data), sizeof(int));
			memcpy(chip->buffers->databuf + sizeof(int), (unsigned char *)(&pages_per_blk), sizeof(int));
			//add for new nand
			memcpy(chip->buffers->databuf + sizeof(int) + sizeof(int), (unsigned char *)(&new_nand_type), sizeof(int));

#ifdef MX_REVD
			//if(en_slc && (mtd->writesize<16384)){
			if(en_slc ){
				memcpy(chip->buffers->databuf + 5*sizeof(int), (unsigned char *)(&magic), sizeof(int));
				memcpy(chip->buffers->databuf + 6*sizeof(int), (unsigned char *)(&page_list[0]), 6*sizeof(int));
			}
#endif

			chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, write_page);
//#ifndef MX_REVD
			if((mx_nand_check_chiprevd() != 1) && (en_slc == 0)){
			ran_mode = aml_chip->ran_mode;
			aml_chip->ran_mode = 0;
			}
//#endif
			chip->ecc.write_page(mtd, chip, chip->buffers->databuf,oob_required);
//#ifndef MX_REVD
			if((mx_nand_check_chiprevd() != 1) && (en_slc == 0))
			aml_chip->ran_mode = ran_mode;
//#endif
			status = chip->waitfunc(mtd, chip);

			if ((status & NAND_STATUS_FAIL) && (chip->errstat))
				status = chip->errstat(mtd, chip, FL_WRITING, status, write_page);

		/*	if (status & NAND_STATUS_FAIL)
				return -EIO;*/
		}
	//	printk("%s, write_page:0x%x\n", __func__, write_page);
#ifdef MX_REVD
		if(en_slc){
			if(aml_chip->new_nand_info.type == HYNIX_1YNM_8GB)
			write_page = pagelist_1ynm_hynix256[page+1] + i*M3_BOOT_PAGES_PER_COPY;
			else
			write_page = pagelist_hynix256[page+1] + i*M3_BOOT_PAGES_PER_COPY;
		}
		else{
			write_page++;
		}
#else
		write_page++;
#endif
		if(new_nand_type == HYNIX_1YNM_8GB) {
			if((page+1)>1)
				priv_slc_page = pagelist_1ynm_hynix256[page] + i*M3_BOOT_PAGES_PER_COPY;
			else
				priv_slc_page = page + i*M3_BOOT_PAGES_PER_COPY;
			while((priv_slc_page+1)<write_page) {
				chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, (++priv_slc_page));
				chip->ecc.write_page_raw(mtd, chip, fill_buf,oob_required);
				chip->waitfunc(mtd, chip);
				printk("%s, fill page:0x%x\n", __func__, priv_slc_page);
			}
		}
		chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, write_page);

		if (unlikely(raw))
			chip->ecc.write_page_raw(mtd, chip, buf,oob_required);
		else
			chip->ecc.write_page(mtd, chip, buf,oob_required);

		if (!cached || !(chip->options & NAND_CACHEPRG)) {

			status = chip->waitfunc(mtd, chip);

			if ((status & NAND_STATUS_FAIL) && (chip->errstat))
				status = chip->errstat(mtd, chip, FL_WRITING, status, write_page);

			if (status & NAND_STATUS_FAIL){
#ifdef MX_REVD
		if(en_slc&&aml_chip->new_nand_info.slc_program_info.exit_enslc_mode)
					aml_chip->new_nand_info.slc_program_info.exit_enslc_mode(mtd);
 #endif
		//		return -EIO;
			}
		} else {

			status = chip->waitfunc(mtd, chip);
		}
	}
#ifdef MX_REVD
	if(en_slc&&aml_chip->new_nand_info.slc_program_info.exit_enslc_mode)
		aml_chip->new_nand_info.slc_program_info.exit_enslc_mode(mtd);
#endif
	return 0;
}
#ifdef CONFIG_CLK81_DFS

static int clk_disable_before(void* privdata)
{
	//printk("[%s] :%s\n",__func__,privdata);
	return 0;
}
static int clk_disable_after(void* privdata,int failed)
{
	//printk("[%s] :%s   failed:%d\n",__func__,privdata,failed);
	return 0;
}

static int clk_enable_before(void* privdata)
{
	//printk("%s] :%s\n",__func__,privdata);
	return 0;
}
static int clk_enable_after(void* privdata,int failed)
{
	//printk("[%s] :%s   failed:%d\n",__func__,privdata,failed);
	return 0;
}
static int clk_ratechange_before(unsigned long newrate,void* privdata)
{
	struct aml_nand_chip *aml_chip = privdata;

	down(&aml_chip->nand_sem);


	//printk("[%s] :%s\n",__func__,privdata);

	return 0;
}
static int clk_ratechange_after(unsigned long newrate,void* privdata,int failed)
{
	//printk("[%s] :%s   failed:%d\n",__func__,privdata,failed);
	struct aml_nand_chip *aml_chip = privdata;

	if(failed == 0){
		aml_chip->aml_nand_adjust_timing(aml_chip);
	}

	up(&aml_chip->nand_sem);

	return 0;
}

#if 0
static int nand_pre_change_fun(struct clk81_client* client)
{
    struct aml_nand_chip *aml_chip = client->param;
    return 0;
}


static int nand_check_client_ready(struct clk81_client* client)
{
    struct aml_nand_chip *aml_chip = client->param;

    if(aml_chip->lock_state){
        printk(KERN_DEBUG "nand already lock");
        return 0;
    }
    else{
        if(down_trylock(&aml_chip->nand_sem)){
            printk(KERN_DEBUG "nand lock not ready");
            return -1;
        }
        aml_chip->lock_state = 1;
        printk(KERN_DEBUG "nand lock ready");
        return 0;
    }
}

static int nand_post_change_fun(struct clk81_client* client)
{
    struct aml_nand_chip *aml_chip = client->param;

    aml_chip->aml_nand_adjust_timing(aml_chip);

    up(&aml_chip->nand_sem);
    aml_chip->lock_state = 0;
    printk(KERN_DEBUG "nand unlock");
    return 0;
}
#endif
#endif
struct nand_hw_control controller;
//extern struct device *nand_dev;
static int aml_nand_probe(struct aml_nand_platform *plat, struct device *dev)
{
	struct aml_nand_chip *aml_chip = NULL;
	struct nand_chip *chip = NULL;
	struct mtd_info *mtd = NULL;
	int err = 0;

	aml_chip = kzalloc(sizeof(*aml_chip), GFP_KERNEL);
	if (aml_chip == NULL) {
		printk("no memory for flash info\n");
		err = -ENOMEM;
		goto exit_error;
	}
#ifdef CONFIG_CLK81_DFS
	static struct clk_ops nand_clk81_ops={
		.clk_disable_before = clk_disable_before,
		.clk_disable_after = clk_disable_after,
		.clk_enable_before = clk_enable_before,
		.clk_enable_after = clk_enable_after,
		.clk_ratechange_before = clk_ratechange_before,
		.clk_ratechange_after = clk_ratechange_after,
		//.privdata = (void *)aml_chip,
	};

	nand_clk81_ops.privdata = (void *)aml_chip;

	init_MUTEX(&aml_chip->nand_sem);
	//regist_clk81_client(plat->name, nand_pre_change_fun, nand_post_change_fun, nand_check_client_ready, aml_chip);
	clk_ops_register(clk_get_sys(NAND_SYS_CLK_NAME, NULL), &nand_clk81_ops);
#endif

	/* initialize mtd info data struct */
	dev->coherent_dma_mask = DMA_BIT_MASK(32);
	aml_chip->device = dev;
	aml_chip->platform = plat;
	aml_chip->bch_desc = m3_bch_list;
	aml_chip->max_bch_mode = sizeof(m3_bch_list) / sizeof(m3_bch_list[0]);
	plat->aml_chip = aml_chip;
	chip = &aml_chip->chip;
	chip->priv = aml_chip;//&aml_chip->mtd;
	aml_chip->ran_mode = plat->ran_mode;
	aml_chip->rbpin_detect = plat->rbpin_detect;

	chip->controller=&controller;
	printk("chip->controller=%p\n",chip->controller);
	mtd = &aml_chip->mtd;
	mtd->priv = chip;
    mtd->dev.parent= dev->parent;
	mtd->owner = THIS_MODULE;

	aml_chip->aml_nand_hw_init = m3_nand_hw_init;
	aml_chip->aml_nand_adjust_timing = m3_nand_adjust_timing;
	aml_chip->aml_nand_select_chip = m3_nand_select_chip;
	aml_chip->aml_nand_options_confirm = m3_nand_options_confirm;
	aml_chip->aml_nand_dma_read = m3_nand_dma_read;
	aml_chip->aml_nand_dma_write = m3_nand_dma_write;
	aml_chip->aml_nand_hwecc_correct = m3_nand_hwecc_correct;
//    aml_chip->nand_early_suspend.suspend = m3_nand_early_suspend;
//    aml_chip->nand_early_suspend.resume = m3_nand_late_resume;
	nand_dev = aml_chip->device;
	err = aml_nand_init(aml_chip);
	if (err)
		goto exit_error;
	#if 0
	if (!strncmp((char*)plat->name, NAND_BOOT_NAME, strlen((const char*)NAND_BOOT_NAME))) {
		chip->erase_cmd = m3_nand_boot_erase_cmd;
		chip->ecc.read_page = m3_nand_boot_read_page_hwecc;
		chip->ecc.write_page = m3_nand_boot_write_page_hwecc;
		chip->write_page = m3_nand_boot_write_page;
		if (chip->ecc.layout)
			chip->ecc.layout->oobfree[0].length = ((mtd->writesize / 512) * aml_chip->user_byte_mode);
		chip->ecc.layout->oobavail = 0;
		for (i = 0; chip->ecc.layout->oobfree[i].length && i < ARRAY_SIZE(chip->ecc.layout->oobfree); i++)
			chip->ecc.layout->oobavail += chip->ecc.layout->oobfree[i].length;
		mtd->oobavail = chip->ecc.layout->oobavail;
		mtd->ecclayout = chip->ecc.layout;
	}
	#endif
	mtd->_suspend = m3_nand_suspend;
	mtd->_resume = m3_nand_resume;
	//chip->select_chip(mtd, -1);
	return 0;

exit_error:
	if (aml_chip)
		kfree(aml_chip);
	mtd->name = NULL;
	return err;
}

#define m3_nand_notifier_to_blk(l)	container_of(l, struct aml_nand_device, nb)
static int m3_nand_reboot_notifier(struct notifier_block *nb, unsigned long priority, void * arg)
{
	int error = 0;
	struct aml_nand_device *aml_nand_dev = m3_nand_notifier_to_blk(nb);
	struct aml_nand_platform *plat = NULL;
	struct aml_nand_chip *aml_chip = NULL;
	struct mtd_info *mtd = NULL;
	int i;
	printk("%s %d \n", __func__, __LINE__);


	for (i=1; i<aml_nand_dev->dev_num; i++) {
		plat = &aml_nand_dev->aml_nand_platform[i];
		aml_chip = plat->aml_chip;
		if (aml_chip) {
			mtd = &aml_chip->mtd;


		}
	}

	return error;
}

#ifdef CONFIG_OF
static const struct of_device_id amlogic_nand_dt_match[];
static inline struct aml_nand_device   *aml_get_driver_data(
			struct platform_device *pdev)
{
	if (pdev->dev.of_node) {
		const struct of_device_id *match;
		match = of_match_node(amlogic_nand_dt_match, pdev->dev.of_node);
		return (struct aml_nand_device *)match->data;
	}
}
#endif

static int m3_get_nand_platform(struct aml_nand_device *aml_nand_dev,struct platform_device *pdev);
static int m3_nand_probe(struct platform_device *pdev)
{
#ifdef CONFIG_OF
	struct aml_nand_device *aml_nand_dev;
#else
	struct aml_nand_device *aml_nand_dev = to_nand_dev(pdev);
#endif
	struct aml_nand_platform *plat = NULL;
	int err = 0, i;

	dev_dbg(&pdev->dev, "(%p)\n", pdev);
#ifdef CONFIG_OF
	aml_nand_dev = aml_get_driver_data(pdev);
	pdev->dev.platform_data=aml_nand_dev;
	printk("===========================================");
	printk("%s:%d,nand device tree ok,dev-name:%s",__func__,__LINE__,dev_name(&pdev->dev));
#endif

	if (!aml_nand_dev) {
		dev_err(&pdev->dev, "no platform specific information\n");
		err = -ENOMEM;
		goto exit_error;
	}
	platform_set_drvdata(pdev, aml_nand_dev);
	printk("%d\n",aml_nand_dev->dev_num);
	spin_lock_init(&controller.lock);
	init_waitqueue_head(&controller.wq);
	aml_nand_dev->nb.notifier_call = m3_nand_reboot_notifier;
	register_reboot_notifier(&aml_nand_dev->nb);
	atomic_notifier_chain_register(&panic_notifier_list, &aml_nand_dev->nb);
#ifdef CONFIG_OF
	err = m3_get_nand_platform(aml_nand_dev,pdev);
#else
	for (i=0; i<aml_nand_dev->dev_num; i++) {
		plat = &aml_nand_dev->aml_nand_platform[i];
		if (!plat) {
			printk("error for not platform data\n");
			continue;
		}
		err = aml_nand_probe(plat, &pdev->dev);
		if (err) {
			printk("%s dev probe failed %d\n", plat->name, err);
			continue;
		}
	}
#endif
exit_error:
	return err;
}

static int m3_nand_remove(struct platform_device *pdev)
{
	struct aml_nand_device *aml_nand_dev = to_nand_dev(pdev);
	struct aml_nand_platform *plat = NULL;
	struct aml_nand_chip *aml_chip = NULL;
	struct mtd_info *mtd = NULL;
	int i;

	platform_set_drvdata(pdev, NULL);
	for (i=0; i<aml_nand_dev->dev_num; i++) {
		plat = &aml_nand_dev->aml_nand_platform[i];
		aml_chip = plat->aml_chip;
		if (aml_chip) {
			mtd = &aml_chip->mtd;

			if (mtd) {
				nand_release(mtd);
				kfree(mtd);
			}

			kfree(aml_chip);
		}
	}
#ifdef CONFIG_OF
	if(pdev->dev.of_node){
		if(aml_nand_dev->aml_nand_platform){
			for(i=0;i<aml_nand_dev->dev_num;i++){
				plat = &aml_nand_dev->aml_nand_platform[i];
			}
			kfree(aml_nand_dev->aml_nand_platform);
		}
		of_node_put(pdev->dev.of_node);
	}
#endif
	return 0;
}

static void m3_nand_shutdown(struct platform_device *pdev)
{
	struct aml_nand_device *aml_nand_dev = to_nand_dev(pdev);
	struct aml_nand_platform *plat = NULL;
	struct aml_nand_chip *aml_chip = NULL;
	struct mtd_info *mtd = NULL;
	struct nand_chip *chip = NULL;
	int i;

	for (i=1; i<aml_nand_dev->dev_num; i++) {
		plat = &aml_nand_dev->aml_nand_platform[i];
		aml_chip = plat->aml_chip;
		if (aml_chip) {
			mtd = &aml_chip->mtd;
			if(mtd){
				chip = mtd->priv;
				if(chip){
					nand_get_device(mtd, FL_SHUTDOWN);
					chip->options |= NAND_ROM;
					printk("%s %d chip->options:%x\n", __func__, __LINE__, chip->options);
					nand_release_device(mtd);
				}
			}
		}
	}

	return;
}

static int amlogic_parse_nand_partion(struct device_node *np,struct aml_nand_platform *plat)
{
	struct mtd_partition *mtd_ptn=NULL;
	struct mtd_partition *p=NULL;
	struct device_node *child;
	unsigned int part_num=0;
//	int i=0;
	child = of_get_next_child(np, NULL);
	if (!child) {
		printk("no partition \n");
		goto  err;
	}
	part_num=of_get_child_count(np) ;
	if(part_num>0){
		mtd_ptn=kzalloc(sizeof(struct mtd_partition)*part_num,GFP_KERNEL);
		p=mtd_ptn;
		memset(p,0,sizeof(struct mtd_partition)*part_num);
		if(!mtd_ptn){
			printk("malloc mtd_partition error\n");
			goto err;
		}
		for_each_child_of_node(np, child) {
			p->name=child->name;
			if (of_property_read_u64(child, "offset", &p->offset))
				goto err;
			if (of_property_read_u64(child, "size", &p->size))
				goto err;
			p++;
		}
		plat->platform_nand_data.chip.partitions=mtd_ptn;
		plat->platform_nand_data.chip.nr_partitions=part_num;
		return 0;
	}
err:
	if(mtd_ptn)
		kzfree(mtd_ptn);
	return -1;
}
int aml_class_register(struct class *class)
{
		return class_register(class);
}
int aml_register_mtd_blktrans(struct mtd_blktrans_ops *tr)
{
		return register_mtd_blktrans(tr);
}
int aml_deregister_mtd_blktrans(struct mtd_blktrans_ops *tr)
{
	return  deregister_mtd_blktrans(tr);
}
int aml_add_mtd_blktrans_dev(struct mtd_blktrans_dev *new)
{
	return  add_mtd_blktrans_dev(new);
}
int aml_del_mtd_blktrans_dev(struct mtd_blktrans_dev *old)
{
	return  del_mtd_blktrans_dev(old);
}
EXPORT_SYMBOL(aml_class_register);
EXPORT_SYMBOL(aml_register_mtd_blktrans);
EXPORT_SYMBOL(aml_deregister_mtd_blktrans);
EXPORT_SYMBOL(aml_add_mtd_blktrans_dev);
EXPORT_SYMBOL(aml_del_mtd_blktrans_dev);
ssize_t show_nand_version_info(struct class *class,
			struct class_attribute *attr,	char *buf)
{
//    struct aml_nand_chip *aml_chip = container_of(class, struct aml_nand_chip, cls);

    printk(KERN_INFO "kernel Version %s,uboot version %s\n", DRV_VERSION,DRV_UBOOT_VERSION);

	return 0;
}
#ifdef CONFIG_OF
static struct mtd_partition normal_partition_info[] = {
    {
        .name = "logo",
        .offset = 32*SZ_1M+40*SZ_1M,
        .size = 8*SZ_1M,
    },
    {
        .name = "aml_logo",
        .offset = 48*SZ_1M+40*SZ_1M,
        .size = 8*SZ_1M,
    },
    {
        .name = "recovery",
        .offset = 64*SZ_1M+40*SZ_1M,
        .size = 8*SZ_1M,
    },
    {
        .name = "boot",
        .offset = 96*SZ_1M+40*SZ_1M,
        .size = 8*SZ_1M,
    },
    {
        .name = "system",
        .offset = 128*SZ_1M+40*SZ_1M,
        .size = 1024*SZ_1M,
    },
    {
        .name = "cache",
        .offset = 1152*SZ_1M+40*SZ_1M,
        .size = 512*SZ_1M,
    },
    {
        .name = "backup",
        .offset = 1152*SZ_1M+512*SZ_1M+40*SZ_1M,
        .size = 256*SZ_1M,
    },
#if 1
   {
        .name = "userdata",
        .offset = MTDPART_OFS_APPEND,
        .size = MTDPART_SIZ_FULL,
    },
#else
    {
        .name = "userdata",
        .offset = 1408*SZ_1M+40*SZ_1M,
        .size = 1024*SZ_1M,
    },
    {
        .name = "NFTL_Part",
        .offset = MTDPART_OFS_APPEND,
        .size = MTDPART_SIZ_FULL,
    },
#endif
};
static struct aml_nand_platform aml_nand_mid_platform[] = {
#ifndef CONFIG_AMLOGIC_SPI_NOR
    {
		#if 1
        .name = NAND_BOOT_NAME,
        .chip_enable_pad = AML_NAND_CE0,
        .ready_busy_pad = AML_NAND_CE0,
        .platform_nand_data = {
            .chip =  {
                .nr_chips = 1,
                .options = (NAND_TIMING_MODE5 | NAND_ECC_BCH60_1K_MODE),
            },
        },
        .rbpin_detect=1,
        .T_REA = 20,
        .T_RHOH = 15,
        #endif
    },
#endif
    {
	#if 0
        .name = NAND_NORMAL_NAME,
        .chip_enable_pad = ((AML_NAND_CE0) | (AML_NAND_CE1 << 4)  | (AML_NAND_CE2 << 8) | (AML_NAND_CE3 << 12)),
        .ready_busy_pad = (AML_NAND_CE0) | (AML_NAND_CE1 << 4),// | (AML_NAND_CE1 << 8) | (AML_NAND_CE1 << 12)*/),
        .platform_nand_data = {
            .chip =  {
                .nr_chips = 1,
                .nr_partitions = ARRAY_SIZE(normal_partition_info),
                .partitions = normal_partition_info,
                .options = (NAND_TIMING_MODE5 | NAND_ECC_BCH60_1K_MODE | NAND_TWO_PLANE_MODE),
            },
        },
        .rbpin_detect=1,
        .T_REA = 20,
        .T_RHOH = 15,
    #endif
    }
};
static struct aml_nand_device aml_nand_mid_device = {
    .aml_nand_platform = aml_nand_mid_platform,
    .dev_num = ARRAY_SIZE(aml_nand_mid_platform),
};
#define AMLOGIC_NAND_DRV_DATA ((kernel_ulong_t)&aml_nand_mid_device)
#endif
struct nand_pad_ce{
	char *ce_name;
	unsigned int ce_value;
};
struct nand_pad_rb{
	char *rb_name;
	unsigned int rb_value;
};
struct nand_timming_mode{
	char *timming_name;
	unsigned int timimg_value;
};
struct nand_bch_mode{
	char *bch_name;
	unsigned int bch_value;
};
struct nand_plane_mode{
	char *plane_name;
	char plane_value;
};
static struct nand_pad_ce nand_pad_ce_cfg[]={
	{	.ce_name = "ce0",
		.ce_value = AML_NAND_CE0,
	},
	{	.ce_name = "ce1",
		.ce_value = AML_NAND_CE1 << 4,
	},
	{	.ce_name = "ce2",
		.ce_value = AML_NAND_CE2 << 8,
	},
	{	.ce_name = "ce3",
		.ce_value = AML_NAND_CE3 << 12,
	},
};
static struct nand_pad_rb nand_pad_rb_cfg[]={
	{	.rb_name = "rb0",
		.rb_value = AML_NAND_CE0,
	},
	{	.rb_name = "rb1",
		.rb_value = AML_NAND_CE1 << 4,
	},
};
static struct nand_timming_mode nand_timming_mode_cfg[]={
	{	.timming_name = "mode5",
		.timimg_value = NAND_TIMING_MODE5,
	},
	{	.timming_name = "mode4",
		.timimg_value = NAND_TIMING_MODE4,
	},
	{	.timming_name = "mode3",
		.timimg_value = NAND_TIMING_MODE3,
	},
	{	.timming_name = "mode2",
		.timimg_value = NAND_TIMING_MODE2,
	},
	{	.timming_name = "mode1",
		.timimg_value = NAND_TIMING_MODE1,
	},
	{	.timming_name = "mode0",
		.timimg_value = NAND_TIMING_MODE0,
	},
};
static struct nand_bch_mode nand_bch_mode_cfg[]={
	{	.bch_name = "bch60_1k",
		.bch_value = NAND_ECC_BCH60_1K_MODE,
	},
	{	.bch_name = "bch40_1k",
		.bch_value = NAND_ECC_BCH40_1K_MODE,
	},
	{	.bch_name = "bch30_1k",
		.bch_value = NAND_ECC_BCH30_1K_MODE,
	},
	{	.bch_name = "bch24_1k",
		.bch_value = NAND_ECC_BCH24_1K_MODE,
	},
	{	.bch_name = "bch16_1k",
		.bch_value = NAND_ECC_BCH16_1K_MODE,
	},
	{	.bch_name = "bch8_1k",
		.bch_value = NAND_ECC_BCH8_1K_MODE,
	},
	{	.bch_name = "bch16_512",
		.bch_value = NAND_ECC_BCH16_MODE,
	},
	{	.bch_name = "bch12_512",
		.bch_value = NAND_ECC_BCH12_MODE,
	},
	{	.bch_name = "bch8_512",
		.bch_value = NAND_ECC_BCH8_MODE,
	},
	{	.bch_name = "bch9_512",
		.bch_value = NAND_ECC_BCH9_MODE,
	},
	{	.bch_name = "bch_short",
		.bch_value = NAND_ECC_SHORT_MODE,
	},
	{	.bch_name = "bch_soft",
		.bch_value = NAND_ECC_SOFT_MODE,
	},
};
static struct nand_plane_mode nand_plane_mode_cfg[]={
	{
		.plane_name = "twoplane",
		.plane_value = NAND_TWO_PLANE_MODE,
	},
};
static int m3_get_nand_platform(struct aml_nand_device *aml_nand_dev,struct platform_device *pdev)
{
	int ret;
	const char *propname;
	struct property *prop;
	const __be32 *list;
	int size,config,index;
	int selector,match_mode;
	const char *select;
	phandle phandle;
	int val=0;
	struct device_node *np_config;
	struct device_node *np_part;
	struct device_node *np = pdev->dev.of_node;
	struct aml_nand_platform *plat=NULL;
	struct aml_nand_platform *plat_array=NULL;
	if(pdev->dev.of_node){
		of_node_get(np);
		ret = of_property_read_u32(np,"plat-num",&aml_nand_dev->dev_num);
		if(ret){
			printk("%s:%d,please config plat-num item\n",__func__,__LINE__);
			return -1;
		}
		if(aml_nand_dev->dev_num > 0){
			plat_array = kzalloc(sizeof(*plat_array)*aml_nand_dev->dev_num, GFP_KERNEL);
			if(!plat_array){
				return -1;
			}
			aml_nand_dev->aml_nand_platform = plat_array;
		}

		for(index=0; index < aml_nand_dev->dev_num;index++){
			plat = &aml_nand_dev->aml_nand_platform[index];
			if(plat == NULL){
				printk("%s:%d, kzalloc mem fail\n",__func__,__LINE__);
				continue;
			}
			propname = kasprintf(GFP_KERNEL, "plat-part-%d", index);
			prop = of_find_property(np, propname, &size);
			kfree(propname);
			if (!prop)
				break;
			list = prop->value;
			size /= sizeof(*list);
			ret = of_property_read_string_index(np, "plat-names",index, (const char*)&plat->name);
			if (ret < 0) {
				plat->name = prop->name + 8;
			}
			for(config=0;config<size;config++){
				phandle = be32_to_cpup(list++);
				np_config = of_find_node_by_phandle(phandle);
				if( !np_config){
					printk("%s:%d,can't find device node\n",__func__,__LINE__);
					goto err;
				}
				plat->chip_enable_pad = 0;
				for(selector=0; ;selector++){
					ret = of_property_read_string_index(np_config, "enable_pad", selector, &select);
					if(ret < 0){
						printk("%s:%d,find enable pad finished\n",__func__,__LINE__);
						break;
					}
					for(match_mode=0;match_mode<sizeof(nand_pad_ce_cfg)/sizeof(nand_pad_ce_cfg[0]);match_mode++){
						if(strcmp(select,nand_pad_ce_cfg[match_mode].ce_name) == 0){
							plat->chip_enable_pad |= nand_pad_ce_cfg[match_mode].ce_value;
						}
					}
				}
				plat->ready_busy_pad = 0;
				for(selector=0; ;selector++){
					ret = of_property_read_string_index(np_config, "busy_pad", selector, &select);
					if(ret < 0){
						printk("%s:%d,find busy_pad finished\n",__func__,__LINE__);
						break;
					}
					for(match_mode=0;match_mode<sizeof(nand_pad_rb_cfg)/sizeof(nand_pad_rb_cfg[0]);match_mode++){
						if(strcmp(select,nand_pad_rb_cfg[match_mode].rb_name) == 0){
							plat->ready_busy_pad |= nand_pad_rb_cfg[match_mode].rb_value;
						}
					}
				}
				for(selector=0; ;selector++){
					ret = of_property_read_string_index(np_config, "timming_mode", selector, &select);
					if(ret < 0){
						printk("%s:%d,find timming_mode finished\n",__func__,__LINE__);
						break;
					}
					for(match_mode=0;match_mode<sizeof(nand_timming_mode_cfg)/sizeof(nand_timming_mode_cfg[0]);match_mode++){
						if(strcmp(select,nand_timming_mode_cfg[match_mode].timming_name) == 0){
							plat->platform_nand_data.chip.options |= nand_timming_mode_cfg[match_mode].timimg_value;
						}
					}
				}
				for(selector=0; ;selector++){
					ret = of_property_read_string_index(np_config, "bch_mode", selector, &select);
					if(ret < 0){
						printk("%s:%d,find bch_mode finished\n",__func__,__LINE__);
						break;
					}
					for(match_mode=0;match_mode<sizeof(nand_bch_mode_cfg)/sizeof(nand_bch_mode_cfg[0]);match_mode++){
						if(strcmp(select,nand_bch_mode_cfg[match_mode].bch_name) == 0){
							plat->platform_nand_data.chip.options |= nand_bch_mode_cfg[match_mode].bch_value;
						}
					}
				}
				for(selector=0; ;selector++){
					ret = of_property_read_string_index(np_config, "plane_mode", selector, &select);
					if(ret < 0){
						printk("%s:%d,find plane_modefinished\n",__func__,__LINE__);
						break;
					}
					for(match_mode=0;match_mode<sizeof(nand_plane_mode_cfg)/sizeof(nand_plane_mode_cfg[0]);match_mode++){
						if(strcmp(select,nand_plane_mode_cfg[match_mode].plane_name) == 0){
							plat->platform_nand_data.chip.options |= nand_plane_mode_cfg[match_mode].plane_value;
						}
					}
				}
				ret = of_property_read_u32(np_config,"chip_num",&plat->platform_nand_data.chip.nr_chips);
				if(ret){
					printk("%s:%d,can't find chip_num\n",__func__,__LINE__);
					plat->platform_nand_data.chip.nr_chips=1;
				}
				ret = of_property_read_u32(np_config,"rb_detect",&plat->rbpin_detect);
				if(ret){
					printk("%s:%d,can't rb_detect \n",__func__,__LINE__);
					plat->rbpin_detect=1;
				}
				ret = of_property_read_u32(np_config,"part_num",&plat->platform_nand_data.chip.nr_partitions);
				if(ret==0){
					if(plat->platform_nand_data.chip.nr_partitions > ARRAY_SIZE(normal_partition_info)){
						plat->platform_nand_data.chip.nr_partitions = ARRAY_SIZE(normal_partition_info);
					}
					if(plat->platform_nand_data.chip.nr_partitions > 0){
						plat->platform_nand_data.chip.partitions = normal_partition_info;
					}
				}
				#if 1
				ret = of_property_read_u32(np_config,"partition",&val);
				if(ret==0){
					phandle=val;
					np_part = of_find_node_by_phandle(phandle);
					if( !np_config){
						printk("%s:%d,can't find device node\n",__func__,__LINE__);
						goto err;
					}
					ret=amlogic_parse_nand_partion(np_part,plat);
					if(ret){
						plat->platform_nand_data.chip.partitions = normal_partition_info;
						plat->platform_nand_data.chip.nr_partitions = ARRAY_SIZE(normal_partition_info);
					}
				}
				#endif
				ret = of_property_read_u32(np_config,"t_rea",&plat->T_REA);
				if(ret){
					plat->T_REA=20;
					printk("%s:%d,not config t_rea,use default value, T_REA=%d\n",__func__,__LINE__,plat->T_REA);
				}
				ret = of_property_read_u32(np_config,"t_rhoh",&plat->T_RHOH);
				if(ret){
					plat->T_RHOH=15;
					printk("%s:%d,not config t_rea,use default value,T_RHOH=%d \n",__func__,__LINE__,plat->T_RHOH);
				}
				of_node_put(np_config);
			}
			printk("%s:%d,parse dts\n",__func__,__LINE__);

			ret = aml_nand_probe(plat, &pdev->dev);
		}
	};
	return 0;
err:
	return -1;
}
#ifdef CONFIG_OF
static const struct of_device_id amlogic_nand_dt_match[]={
	{	.compatible = "amlogic,aml_nand",
		.data		= (void *)AMLOGIC_NAND_DRV_DATA,
	},
	{},
};
#else
#define amlogic_nand_dt_match NULL
#endif

/* driver device registration */
static struct platform_driver m3_nand_driver = {
	.probe		= m3_nand_probe,
	.remove		= m3_nand_remove,
	.shutdown	= m3_nand_shutdown,
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table=amlogic_nand_dt_match,
	},
};

static int __init m3_nand_init(void)
{
	printk(KERN_INFO "%s, Version %s (c) 2010 Amlogic Inc.\n", DRV_DESC, DRV_VERSION);
	printk(KERN_INFO "####Version of Uboot must be newer than %s!!!!! \n",  DRV_UBOOT_VERSION);
	return platform_driver_register(&m3_nand_driver);
}

static void __exit m3_nand_exit(void)
{
	platform_driver_unregister(&m3_nand_driver);
}

module_init(m3_nand_init);
module_exit(m3_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_ALIAS("platform:" DRV_NAME);


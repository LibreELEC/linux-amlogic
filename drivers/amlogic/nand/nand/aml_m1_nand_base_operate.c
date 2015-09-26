/*
 * AMLOGIC NAND  driver.
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
 * Changelog:
 *
 * TODO:
 *
 */

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
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <mach/nand.h>
#include <mach/pinmux.h>
#include <linux/dma-mapping.h>
#define MAX_INFO_LEN 4*8*2
#define CM0	((1<<29) | (1<<27) | (1<<25) | (1<<23))
#define SM0	(1<<30) | (1<<28) | (1<<26) | (1<<24)
#define CM7	((1<<29) | (1<<27) | (1<<28) | (1<<26)|(1<<24))
struct aml_m1_nand_info
{
	struct nand_hw_control			controller;
	struct mtd_info					mtd;
	struct nand_chip				chip;
	struct aml_m1_nand_platform*	platform;
	struct device*					device;
//	unsigned int 					nand_config;
	unsigned int 					bch_mode;
	unsigned int 					encode_size;
	unsigned int 					ce_sel;
//	unsigned int					info_buf[32];
	uint8_t *						aml_nand_dma_buf;
	uint8_t * 						info_buf;
	dma_addr_t 						aml_nand_info_dma_addr;
};

static unsigned int def_sparebytes = 0x6d616d61;
static struct nand_ecclayout m1_ecclayout;

//FIXME , nand.h
extern  int nand_get_device(struct nand_chip *chip, struct mtd_info *mtd,int new_state);
extern  void nand_release_device(struct mtd_info *mtd);

static struct aml_m1_nand_info *mtd_to_nand_info(struct mtd_info *mtd)
{
	return container_of(mtd, struct aml_m1_nand_info, mtd);
}

/*static struct aml_m1_nand_info *to_nand_info(struct platform_device *pdev){

	return platform_get_drvdata(pdev);
}
*/
static struct aml_m1_nand_platform *to_nand_plat(struct platform_device *pdev)
{
	return pdev->dev.platform_data;
}
/*get_device also slect chip add pin mux here*/


static void aml_m1_nand_select_chip(struct mtd_info *mtd, int chipnr)
{
	struct nand_chip *chip = mtd->priv;
	struct aml_m1_nand_info * info=mtd_to_nand_info(mtd);

	clear_mio_mux(1,CM0);
	set_mio_mux(1,SM0);
	set_mio_mux(6,0x7ff);
	clear_mio_mux(7,CM7);

	switch (chipnr) {
		case -1:
			chip->cmd_ctrl(mtd, NAND_CMD_NONE, 0 | NAND_CTRL_CHANGE);
			set_mio_mux(1,CM0);
			clear_mio_mux(1,SM0);
			clear_mio_mux(6,0x7ff);
			set_mio_mux(7,CM7);
			break;
		case 0:
			info->ce_sel=CE0;
			break;
		case 1:
			info->ce_sel=CE1;
			break;
		case 2:
			info->ce_sel=CE2;
			break;
		case 3:
			info->ce_sel=CE3;
			break;
		default:
			BUG();
	}

	NFC_SEND_CMD(info->ce_sel|IDLE | 1);

	return ;
}

static void aml_m1_nand_hwcontrol(struct mtd_info *mtd, int cmd,  unsigned int ctrl)
{
	struct aml_m1_nand_info * info=mtd_to_nand_info(mtd);
	volatile int val=0;
	unsigned int  ce=info->ce_sel;

	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE){
		val=NFC_CMD_CLE(ce,cmd);
		NFC_SEND_CMD(val);
	}
	else
	{
		val=NFC_CMD_ALE(ce,cmd);
		NFC_SEND_CMD(val);
	}
	return ;
}

static int aml_m1_nand_devready(struct mtd_info *mtd)
{
	struct aml_m1_nand_info * info=mtd_to_nand_info(mtd);
	volatile  int ce;
	if(info->ce_sel==CE0)
		ce=1;
	else
		if(info->ce_sel==CE1)
			ce=2;
		else
			if(info->ce_sel==CE2)
				ce=4;
			else
				if(info->ce_sel==CE3)
					ce=8;
				else
					BUG();

	return (NFC_INFO_GET()>>26)&ce;
}

static void aml_m1_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	return;
}

static int aml_m1_nand_calculate_ecc(struct mtd_info *mtd,const u_char *dat, u_char *ecc_code)
{
	return 0;
}

static int aml_m1_nand_correct_data(struct mtd_info *mtd, u_char *dat,u_char *read_ecc, u_char *calc_ecc)
{
    return 0;
}
static uint8_t aml_m1_nand_read_byte(struct mtd_info *mtd)
{
	uint8_t val;
	struct aml_m1_nand_info * info=mtd_to_nand_info(mtd);
	unsigned int  ce=info->ce_sel;

/*	memset(&info->info_buf[1],0,sizeof(unsigned int));
	NFC_SET_DADDR(&info->info_buf[0]);
    NFC_SET_IADDR(&info->info_buf[1]);
    NFC_SEND_CMD_N2M(1,0);
    while(NAND_INFO_DONE(info->info_buf[1])==0);
    val=info->info_buf[0]&0xff;
*/

	NFC_SEND_CMD(ce|DRD | 0);
	NFC_SEND_CMD(ce|IDLE | 5);

	while(NFC_CMDFIFO_SIZE()>0);
	val=NFC_GET_BUF()&0xff;

	return val;
}
/*
static void prepare_info_buf_before_read(struct mtd_info *mtd,int len)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	memset(aml_info->info_buf,0,32*sizeof(int));
}*/
static int  transfer_info_buf_after_read(struct mtd_info *mtd,int len)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	struct nand_chip *chip = mtd->priv;
	volatile uint16_t *p=(uint16_t *)chip->oob_poi;
	volatile uint8_t  *ptr=(uint8_t *)chip->oob_poi;
	volatile int * pinfobuf=(uint32_t *)aml_info->info_buf;
	int i;
	int res=0;
	for(i=0;i<len;i++)
	{
		if(NAND_ECC_FAIL(pinfobuf[i]))
		{
			mtd->ecc_stats.failed++;
			res=1;										//FIXME -1 for err
		}
		else
			mtd->ecc_stats.corrected+=NAND_ECC_CNT(pinfobuf[i]);

		if(aml_info->bch_mode!=NAND_ECC_BCH9)
			p[i]=pinfobuf[i]&0xffff;
		//	p[i]=aml_info->info_buf[i]&0xffff;
		else
			ptr[i]=pinfobuf[i]&0xff;
		//	ptr[i]=aml_info->info_buf[i]&0xff;
	}
	return res;
}
/* for write oob use
 * */
static void prepare_info_buf_before_write(struct mtd_info *mtd,int len, int ecc)
{
	int i;
	struct aml_m1_nand_info *aml_info=mtd_to_nand_info(mtd);
	struct nand_chip *chip  = mtd->priv;
	volatile uint16_t *p	=(uint16_t *)chip->oob_poi;
	volatile uint8_t  *ptr	=(uint8_t *)chip->oob_poi;
	volatile uint32_t *pinfobuf=(uint32_t *)aml_info->info_buf;

	if(ecc!=AML_NAND_ECC_NONE)
	{
		for(i=0;i<len;i++)
		{
			if(aml_info->bch_mode!=NAND_ECC_BCH9)
				pinfobuf[i]=p[i];
			else
				pinfobuf[i]=ptr[i];
		}
	}
	else
	{
		for(i=0;i<len;i++)
		{
			pinfobuf[i]=def_sparebytes;
		}
	}
}
static int  aml_m1_nand_dma_read(struct mtd_info *mtd, uint8_t *buf, int len,int ecc)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	volatile int res=0;
	dma_addr_t data_dma_addr=0;
	volatile 	unsigned int * pbuf=(unsigned int *)((unsigned char *)aml_info->info_buf+((len>>9)-1)*4);
	memset(aml_info->info_buf,0,32*sizeof(int));

	if(buf)
	{
		data_dma_addr=dma_map_single(aml_info->device,(void *)buf,len,DMA_FROM_DEVICE);
		//	NFC_SEND_CMD_ADL(aml_info->aml_nand_dma_addr)    ;
		//	NFC_SEND_CMD_ADH(aml_info->aml_nand_dma_addr)     ;
		NFC_SEND_CMD_ADL(data_dma_addr);
		NFC_SEND_CMD_ADH(data_dma_addr);
	}
	NFC_SEND_CMD_AIL(aml_info->aml_nand_info_dma_addr);
	NFC_SEND_CMD_AIH((aml_info->aml_nand_info_dma_addr));
	NFC_SEND_CMD_N2M(len,ecc);
	while(NFC_CMDFIFO_SIZE()>0);
	while(NAND_INFO_DONE(*pbuf)==0);

	if(buf)
		dma_unmap_single(aml_info->device,data_dma_addr,len,DMA_FROM_DEVICE);

	if(ecc!=AML_NAND_ECC_NONE)
		res=transfer_info_buf_after_read(mtd,(len+511)>>9);

	return res;
}
static void aml_m1_nand_dma_write(struct mtd_info *mtd, const uint8_t *buf, int len,int ecc)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	dma_addr_t data_dma_addr;
	prepare_info_buf_before_write(mtd,(len+511)>>9,ecc);

//	NFC_SET_DADDR(buf);
//	NFC_SET_IADDR(aml_info->info_buf);
	data_dma_addr=dma_map_single(aml_info->device,(void *)buf,len,DMA_TO_DEVICE);
	NFC_SEND_CMD_ADL(data_dma_addr);
	NFC_SEND_CMD_ADH(data_dma_addr);
	NFC_SEND_CMD_AIL(aml_info->aml_nand_info_dma_addr);
	NFC_SEND_CMD_AIH(aml_info->aml_nand_info_dma_addr);
	NFC_SEND_CMD_M2N(len,ecc);
	NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
	NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
	while(NFC_CMDFIFO_SIZE()>0);
	dma_unmap_single(aml_info->device,data_dma_addr,len,DMA_TO_DEVICE);
}

static void aml_m1_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	aml_m1_nand_dma_read(mtd,buf,len,AML_NAND_ECC_NONE);
}

static void aml_m1_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	aml_m1_nand_dma_write(mtd,buf,len,AML_NAND_ECC_NONE);
}

static int aml_m1_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip,uint8_t *buf,int page)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	return	aml_m1_nand_dma_read(mtd,buf,mtd->writesize,aml_info->bch_mode);
}

static void aml_m1_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	aml_m1_nand_dma_write(mtd,buf,mtd->writesize,aml_info->bch_mode);
}

static int aml_m1_nand_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip,uint8_t *buf,int page)
{
	BUG();
	return 0;
}

static int aml_m1_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip,int page, int sndcmd)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	int res=0;
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
	NFC_SET_SPARE_ONLY();
	res=aml_m1_nand_dma_read(mtd,aml_info->aml_nand_dma_buf,mtd->writesize,aml_info->bch_mode);	//confilct chip->buffers?
	NFC_CLEAR_SPARE_ONLY();

	//BUG();
	return res;
}

static int aml_m1_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip,int page)
{
	BUG();
	return -EIO;
}
static int aml_m1_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	volatile int ret=0,chipnr;
	struct nand_chip * chip=mtd->priv;
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	volatile	int page = (int)(ofs >> chip->page_shift)&chip->pagemask;
	volatile	unsigned int size=((mtd->writesize+mtd->oobsize)/(aml_info->encode_size))<<9;				//FIXME for boot!=normal

	if(getchip)
	{
		nand_get_device(chip,mtd,FL_READING);
		chipnr = (int)(ofs>>chip->chip_shift);
		chip->select_chip(mtd, chipnr);
	}

	NFC_SET_SPARE_ONLY();

	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);

//	ret=aml_m1_nand_dma_read(mtd,aml_info->aml_nand_dma_buf,size,aml_info->bch_mode);	//confilct chip->buffers?
	ret=aml_m1_nand_dma_read(mtd,NULL,size,aml_info->bch_mode);

	NFC_CLEAR_SPARE_ONLY();


	if(getchip)
		nand_release_device(mtd);

	return ret;
}
static int aml_m1_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	printk(" MARK the BAD BLOCK , NOT IMPLENMENT\n");
	BUG();
	return 0;
}

static int aml_m1_nand_hw_init(struct aml_m1_nand_info *info)
{
	struct aml_m1_nand_platform *plat = info->platform;
	volatile	unsigned mode,cycles,adjust=0;
	volatile unsigned tmp=0xffff;
	mode=plat->timing_mode;

	if(mode==5){
		cycles=	3;
	}else if(mode==0)
		cycles=19;
	else if(mode==4){
		cycles=4;
	}else if(mode==3){
		cycles=5;
	}else if(mode==2){
		cycles=6;
	}else if(mode==1){
		cycles=9;
	}
	else{
		mode =0;
		cycles=19;
	}

	clear_mio_mux(1,( (1<<29) | (1<<27) | (1<<25) | (1<<23)));
	set_mio_mux(1,(1<<30) | (1<<28) | (1<<26) | (1<<24));
	set_mio_mux(6,0x7ff);
	clear_mio_mux(7,((1<<29) | (1<<27) | (1<<28) | (1<<26))|(1<<24));

	NFC_SET_CFG(0);
	NFC_SET_TIMING(0,19,0)  ;
	NFC_SEND_CMD(1<<31);
	while (NFC_CMDFIFO_SIZE() > 0);


	if(plat->onfi_mode==1)
	{
	/*	NFC_SEND_CMD_CLE(CE0,0xff);
		NFC_SEND_CMD_IDLE(CE0,10);
		while (!(NFC_INFO_GET()&(1<<26)));

		printk("NAND SET ONFI MODE %d \n",mode);
		NFC_SEND_CMD_CLE(CE0,0xef);
		NFC_SEND_CMD_ALE(CE0,0x1);
		NFC_SEND_CMD_IDLE(CE0,3);
		NFC_SEND_CMD_DWR(mode);
		NFC_SEND_CMD_DWR(0);
		NFC_SEND_CMD_DWR(0);
		NFC_SEND_CMD_DWR(0);
		NFC_SEND_CMD_IDLE(CE0,5);
		while (NFC_CMDFIFO_SIZE() > 0);
		while (!(NFC_INFO_GET()&(1<<26)));

		NFC_SEND_CMD_CLE(CE0,0xee);
		NFC_SEND_CMD_ALE(CE0,0x1);
		NFC_SEND_CMD_IDLE(CE0,3);
	//	while (NFC_CMDFIFO_SIZE() > 0);
		while (!(NFC_INFO_GET()&(1<<26)));
		//NFC_SEND_CMD_IDLE(CE0,3);
		NFC_SEND_CMD(CE0|DRD | 3);
	//	NFC_SEND_CMD(CE0|IDLE | 5);
		while(NFC_CMDFIFO_SIZE()>0);
		tmp=NFC_GET_BUF();
		printk("NAND GET ONFI MODE  %d \n", tmp);

		*/
	}

	NFC_SET_TIMING(mode,cycles,adjust);
	printk("NAND CONFIG IS 0x%4x \n",NFC_GET_CFG());
	NFC_SEND_CMD(1<<31);
	while (NFC_CMDFIFO_SIZE() > 0);

	return 0;
}

static int aml_m1_nand_add_partition(struct aml_m1_nand_info *info)
{
	struct mtd_info *mtd = &info->mtd;
#ifdef CONFIG_MTD_PARTITIONS
	struct mtd_partition *parts = info->platform->partitions;
	int nr = info->platform->nr_partitions;
	return add_mtd_partitions(mtd, parts, nr);
#else
	return add_mtd_device(mtd);
#endif
}

static int aml_m1_nand_probe(struct platform_device *pdev)
{
	struct aml_m1_nand_platform *plat = to_nand_plat(pdev);
	struct aml_m1_nand_info *info = NULL;
	struct nand_chip *chip = NULL;
	struct mtd_info *mtd = NULL;
	int err = 0;
	dev_dbg(&pdev->dev, "(%p)\n", pdev);

//	printk("NAND probe \n");

	if (!plat)
	{
		dev_err(&pdev->dev, "no platform specific information\n");
		goto exit_error;
	}

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL)
	{
		dev_err(&pdev->dev, "no memory for flash info\n");
		err = -ENOMEM;
		goto exit_error;
	}

	platform_set_drvdata(pdev, info);

	spin_lock_init(&info->controller.lock);
	init_waitqueue_head(&info->controller.wq);

	pdev->dev.coherent_dma_mask =DMA_BIT_MASK(32);

	info->device     = &pdev->dev;
	info->platform   = plat;
	info->bch_mode	 = plat->bch_mode;
	info->encode_size= plat->encode_size;

	chip 		  	 = &info->chip;
	chip->buffers    = kmalloc(sizeof(struct nand_buffers), GFP_KERNEL);
	if (info->chip.buffers == NULL)
	{
		dev_err(&pdev->dev, "no memory for flash info\n");
		err = -ENOMEM;
		goto exit_error;
	}
	chip->options |= NAND_OWN_BUFFERS;
	chip->options |=  NAND_SKIP_BBTSCAN;
	chip->options &= ~NAND_BUSWIDTH_16	;

	chip->read_byte    	=aml_m1_nand_read_byte;
	chip->cmd_ctrl     	=aml_m1_nand_hwcontrol;
	chip->dev_ready    	=aml_m1_nand_devready;
	chip->block_bad 	=aml_m1_nand_block_bad;
	chip->block_markbad =aml_m1_nand_block_markbad;
	chip->priv	  		=&info->mtd;
	chip->controller   	=&info->controller;
	chip->chip_delay   	= 20;					//us unit
	chip->select_chip=aml_m1_nand_select_chip;
	mtd 		= &info->mtd;
	mtd->priv	= chip;
	mtd->owner	= THIS_MODULE;


//	nand_get_chip();			//pinmux chipnum

	err = aml_m1_nand_hw_init(info);
	if (err != 0)
		goto exit_error;

	if(plat->page_size!=512)
	{
		chip->ecc.steps		= 1;
		chip->ecc.bytes		= plat->spare_size;	 												//for simple  step 1
		chip->ecc.size 		= plat->page_size;
		chip->phys_erase_shift=ffs(plat->erase_size)-1;
		chip->read_buf      = aml_m1_nand_read_buf;
		chip->write_buf     = aml_m1_nand_write_buf;
		chip->ecc.read_page = aml_m1_nand_read_page;
		chip->ecc.write_page= aml_m1_nand_write_page;
		chip->ecc.read_oob  = aml_m1_nand_read_oob;
		chip->ecc.write_oob = aml_m1_nand_write_oob;
		chip->ecc.calculate = aml_m1_nand_calculate_ecc;
		chip->ecc.correct   = aml_m1_nand_correct_data;
		chip->ecc.mode 		= NAND_ECC_HW;
		chip->ecc.hwctl	    = aml_m1_nand_enable_hwecc;
		chip->ecc.read_page_raw=aml_m1_nand_read_page_raw;
		chip->ecc.layout	= &m1_ecclayout;

		m1_ecclayout.oobavail =(plat->page_size/512)*((plat->bch_mode!=NAND_ECC_BCH9)?2:1);
		m1_ecclayout.oobfree[0].offset=0;
		m1_ecclayout.oobfree[0].length=m1_ecclayout.oobavail;									//FIXME base 2960

		info->aml_nand_dma_buf = kzalloc(plat->page_size+plat->spare_size, GFP_KERNEL);
		if (info->aml_nand_dma_buf == NULL)
		{
			dev_err(&pdev->dev, "no memory for flash info\n");
			err = -ENOMEM;
			goto exit_error;
		}
		info->info_buf=	dma_alloc_coherent(info->device,32*sizeof(int),&(info->aml_nand_info_dma_addr),GFP_KERNEL);
		if (info->info_buf== NULL)
		{
			dev_err(&pdev->dev, "no memory for flash info\n");
			err = -ENOMEM;
			goto exit_error;
		}
		//((unsigned)aml_nand_dma_buf_temp+ARC_DCACHE_LINE_LEN-1)&DCACHE_LINE_MASK;
	}
	else
	{
		BUG();
	}

	if (nand_scan(mtd, plat->chip_num))			//FIXME chip_num!=ce_num
	{
		err = -ENXIO;
		goto exit_error;
	}

	if(aml_m1_nand_add_partition(info)!=0)
	{
		err = -ENXIO;
		goto exit_error;
	}

	dev_dbg(&pdev->dev, "initialised ok\n");
	printk("NAND initialised ok\n");

	//nand_release_chip();

	return 0;

exit_error:

	printk("NAND initialised err\n");

	kfree(info);
	if (err == 0)
		err = -EINVAL;
	return err;
}

#define DRV_NAME	"aml_m1_nand"
#define DRV_VERSION	"0.1"
#define DRV_AUTHOR	"pfs"
#define DRV_DESC	"Amlogic M1 on-chip NAND FLash Controller Driver"

static struct platform_driver aml_m1_nand_driver =
{
	.probe		= aml_m1_nand_probe,
	.driver		=
	{
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};



static int __init aml_m1_nand_init(void)
{
//	printk(KERN_INFO "%s, Version %s (c) 2010 Amlogic Inc.\n",DRV_DESC, DRV_VERSION);
//	asm("wfi");
	return platform_driver_register(&aml_m1_nand_driver);
}

static void __exit aml_m1_nand_exit(void)
{
	platform_driver_unregister(&aml_m1_nand_driver);
}

module_init(aml_m1_nand_init);
module_exit(aml_m1_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);

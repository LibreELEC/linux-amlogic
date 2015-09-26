/* linux/drivers/mtd/nand/am8218net_nand.c
 *
 *
 * Copyright (c) 2008 Amlogic, Inc.
 *
 * Derived from drivers/mtd/nand/s3c2410.c
 * Copyright (c) 2007 Ben Dooks <ben@simtec.co.uk>
 *
 * Derived from drivers/mtd/nand/cafe.c
 * Copyright © 2006 Red Hat, Inc.
 * Copyright © 2006 David Woodhouse <dwmw2@infradead.org>
 *
 * Changelog:
 *
 * TODO:
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

#include <asm/cache.h>
#include <asm/cacheflush.h>
#include <asm/nand.h>
#include <asm/io.h>


static unsigned  int hardware_ecc = 1;
static unsigned  int nand_config=0;
static unsigned char nf_bch_inv = 0;
static unsigned char nf_bch_mode = 0;				//NIKE  only BCH9
static unsigned long USER_BYTES_3_0 = 0x123456ff;
//For MLC 4K page need more user byte

#define BADBLOCK_FLAG_BYTE    6  //NOT USE BBT

/*
 * Data structures for nand flash controller driver
 */
static uint8_t scan_userbyte_pattern[] = {0xff, 0x56,0x34,0x12};
static unsigned char flash_id[8];
static unsigned char PageShiftTable[4] = {10, 11, 12, 13};

static struct nand_bbt_descr slcpage_memorybased = {
	.options = 0,
	.offs = 0,
	.len = 1,
	.pattern = scan_userbyte_pattern
};


struct am8218_nand_info {
	/* mtd info */
	struct nand_hw_control	controller;
	struct mtd_info			mtd;
	struct nand_chip		chip;

	/* platform info */
	struct am8218_nand_platform	*platform;

	/* device info */
	struct device			*device;
	unsigned char *aml_nand_dma_buf;
};




/*
 * Conversion functions
 */
static struct am8218_nand_info *mtd_to_nand_info(struct mtd_info *mtd){

	return container_of(mtd, struct am8218_nand_info, mtd);
}

static struct am8218_nand_info *to_nand_info(struct platform_device *pdev){

	return platform_get_drvdata(pdev);
}

static struct am8218_nand_platform *to_nand_plat(struct platform_device *pdev){

	return pdev->dev.platform_data;
}


/*
 * struct nand_chip interface function pointers
 */

/*
 * am8218_nand_hwcontrol
 * Issue command and address cycles to the chip
 */
static void am8218_nand_hwcontrol(struct mtd_info *mtd, int cmd,  unsigned int ctrl){

	if (cmd == NAND_CMD_NONE)
		return;


	if (ctrl & NAND_CLE)
		NAND_SEND_COMMAND(cmd);
	else
		NAND_SEND_ADDR(cmd);

	//tWB  note MTD also do some like  this
//	while (!NAND_CHECK_BUSY())
//		cpu_relax();

}

/*
 * am8218_nand_devready
 * returns 0 if the nand is busy, 1 if it is ready
 * note nand_erase use but
 * should set nandchip->delay for other func such as
 * nand_command_lp will latch twb
 *
 */
static int am8218_nand_devready(struct mtd_info *mtd){

	return NAND_CHECK_BUSY();
			//?0:1;
}

/*
 * ECC functions
 *
 * note  callback func must provide for safe
 * *
 */

static void am8218_nand_enable_hwecc(struct mtd_info *mtd, int mode){

	return;
}


static int am8218_nand_calculate_ecc(struct mtd_info *mtd,const u_char *dat, u_char *ecc_code){

	return 0;

}



static int am8218_nand_correct_data(struct mtd_info *mtd, u_char *dat,u_char *read_ecc, u_char *calc_ecc){

//	BUG();

    return 0;


}


/*
 * PIO mode for buffer writing and reading
 */

static uint8_t am8218_nand_read_byte(struct mtd_info *mtd){

	uint8_t val;
	struct nand_chip * this=mtd->priv;
	val=IO_READ8(this->IO_ADDR_R);

	return val;
}

static void am8218_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len){

	uint32_t * ptr=(uint32_t *)buf;

	for(;len>0;len-=4)
		*ptr++=NAND_GET_DATA();

}


static void am8218_nand_read_buf16(struct mtd_info *mtd, uint8_t *buf, int len){

	uint32_t * ptr=(uint32_t *)buf;

	for(;len>0;len-=4)
		*ptr++=NAND_GET_DATA();
}

static void am8218_nand_write_buf(struct mtd_info *mtd,	const uint8_t *buf, int len){

	uint32_t * ptr=(uint32_t *)buf;

	for(;len>0;len-=4)
		NAND_SEND_DATA(*ptr++);
}


static void am8218_nand_write_buf16(struct mtd_info *mtd,const uint8_t *buf, int len) {

	uint32_t * ptr=(uint32_t *)buf;

	for(;len>0;len-=4)
		NAND_SEND_DATA(*ptr++);
}

static int nand_id_read(unsigned char * data)
{
    unsigned *p;
    unsigned i;
    p = (unsigned*)data;
    NAND_SEND_COMMAND(NAND_CMD_READID);
    NAND_SEND_ADDR(0x00);

    for (i = 0; i < 8 / sizeof(unsigned); i++)
    {
        *p++ = NAND_GET_DATA();
    }

    return 0;
}

static  int  Nand_Dma_Waiting(void){

	uint32_t i=0;

	while(READ_PERIPHS_REG(PNAND_DMA_CNTL) & (1 << 31) && i<0x1000000)
		i++;

	if(i<0x1000000)
	return 0;
	NAND_SET_DMA_CNTL(NAND_DMA_ABORT);
	return 1;

}

#ifdef CONFIG_AMLOGIC_BOARD_NIKE
static void am8218_nand_reset(void){

		NAND_RESET_FOR_WRITE();

		NAND_SET_MANUAL_CONFIG(nand_config);

}
#endif

int nf_id_valid (char *flash_id)
{
    char    manufacturers_id;
    manufacturers_id = flash_id[0];

    switch (manufacturers_id)
    {
    case NAND_MFR_SAMSUNG:
    case NAND_MFR_HYNIX:
    case NAND_MFR_TOSHIBA:
    case NAND_MFR_FUJITSU:
    case NAND_MFR_NATIONAL:
    case NAND_MFR_RENESAS:
    case NAND_MFR_STMICRO:
    case NAND_MFR_MICRON:
    case NAND_MFR_AMD:
    case NAND_MFR_INTEL:
        return 0;
    default:
        return -1;
    }
}

static int Nand_Ecc_Check(void * dest,uint32_t page_size){

	uint32_t j;
    uint32_t page;
    uint32_t err_count;
    uint32_t bch15  = nf_bch_mode;
	uint32_t sectors = 0;
	uint32_t byte;
	uint32_t bit;
	uint32_t error_cnt_reg;
	uint32_t allff;

	if(!hardware_ecc)
		return 0;

	sectors	= page_size/512;
	if(!((sectors==1)||(sectors==4)||(sectors==8)))
		return 1;

	//flush_and_inv_dcache_all();																//FIXME
	//inv_dcache_all();
	inv_dcache_range((unsigned long)dest,((unsigned long)dest)+page_size-1);


#if (defined CONFIG_AMLOGIC_BOARD_APOLLO) || (defined CONFIG_AMLOGIC_BOARD_APOLLO_H)
	 allff=READ_PERIPHS_REG(PNAND_CONFIG_REG);

	 if(allff&((1<<29)|(1<<30)) == ((1<<29)|(1<<30))){
				return 0;
	}
#endif

	 error_cnt_reg = READ_PERIPHS_REG(PNAND_ERR_CNT0);

	 for( page = 0; page < sectors; page++ ) {
	    // If page transferred and has correctable errors
	    if( !(READ_PERIPHS_REG(PNAND_ERR_UNC) & (1 << page)) ) {

	        // Fix errors for 512 bytes (page)
	        err_count = (error_cnt_reg >> ((page)*4)) & 0xf;
	        //sdram_base  = *(volatile unsigned char*)dest ;
	        // Point to the error list in the FIFO/memory

			if(err_count){
		if( bch15 ) { WRITE_PERIPHS_REG(PNAND_ERR_LOC, page*15); }
			else        { WRITE_PERIPHS_REG(PNAND_ERR_LOC, page*9);  }
			}


	        for(j = 0; j < err_count; j++ ) {

		 byte    = READ_PERIPHS_REG(PNAND_ERR_LOC); // Read byte/bit location
			 bit     = byte & 0x7;

				//ASSERT((err_count!=0)&is_printf,err_count,nf_address,page,(byte >> 4),bit)
	            // If the error is in the payload section
	            if( (byte >> 4) <= 511 ) {
	                (*(volatile unsigned char *)((volatile unsigned char *)dest + page*512 + (byte >> 4))) ^= (1 << bit);
	            }

				printk(" correct error at byte %x bit %x ",byte>>4,bit);
	        }
	    }
	    else {

			//MAYBE FOR BBT
			printk(" NAND  Uncorrect error ,May be all oxff \n");
			return 	-1;

	    }
	}
	return 0;

}



static int am8218_nand_dma_rw(uint8_t * buf, int len, int isread, int eccornot){

	int32_t ret=0;
	if(isread){
			inv_dcache_range((unsigned long )buf,((unsigned long )buf)+len-1);
	}
	else{

			flush_dcache_range((unsigned long )buf, ((unsigned long )buf)+len-1);

#ifdef CONFIG_AMLOGIC_BOARD_NIKE

			am8218_nand_reset();
#endif

			NAND_SET_USERBYTES_0_3(USER_BYTES_3_0);
	}

	invalidate_ahb_cache();
	NAND_SET_DMA_START_ADDR(buf);
	NAND_SET_DMA_XFER_COUNT(len);
	NAND_SET_DMA_CNTL((isread?NAND_DMA_DIRECTOR_READ:0)|
					 NAND_DMA_TYPE_NORMAL |
					 (eccornot?NAND_DMA_ECC_ENABLE:0)|
					 (nf_bch_inv<<25)|
					 (nf_bch_mode<<26)|
					 NAND_DMA_BURST_PARAM(10));

	ret=Nand_Dma_Waiting();

	if(isread&&eccornot)
				ret=Nand_Ecc_Check(buf,len);


	return ret;

}

/*
 * DMA functions for buffer writing and reading
 */
 // one case not page size( PIO) , other is page size(use dma)
//with ECC OR NOT
static void am8218_nand_dma_read_buf(struct mtd_info *mtd,uint8_t *buf, int len){

	struct am8218_nand_info *info = mtd_to_nand_info(mtd);
	struct am8218_nand_platform *plat = info->platform;
	uint32_t page_size = plat->page_size;						//or mtd->writesize
	uint32_t eccornot;

	dev_dbg(info->device, "mtd->%p, buf->%p, int %d\n", mtd, buf, len);

	if ((len < page_size)) {
		am8218_nand_read_buf(mtd, buf, len);
	}
	else {
		//page size or page+oob
		if(len==(mtd->writesize+mtd->oobsize))
			eccornot=0;
		else if (page_size == 512)
			eccornot=0;
		else
			eccornot=1;

		am8218_nand_dma_rw(info->aml_nand_dma_buf,len,1,eccornot);
		if (info->chip.state != FL_READING)
			printk("nand dev state error at nand_dma_read_buf %d\n", info->chip.state);
		memcpy(buf, info->aml_nand_dma_buf, len);
	}
}

// one case not page size( PIO) , other is page size(use dma)
static void am8218_nand_dma_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len){

	struct am8218_nand_info *info = mtd_to_nand_info(mtd);
	struct am8218_nand_platform *plat = info->platform;
	unsigned short page_size = plat->page_size;						//or mtd->writesize
	uint32_t eccornot;

	dev_dbg(info->device, "mtd->%p, buf->%p, int %d\n", mtd, buf, len);

	memcpy(info->aml_nand_dma_buf, buf, len);
	if (page_size == 512)
		am8218_nand_dma_rw((uint8_t * )info->aml_nand_dma_buf,len,0,0);
	else {
		if (len < page_size)
			am8218_nand_write_buf(mtd,info->aml_nand_dma_buf,len);						//with ECC OR NOT FIXME
		else{
			if(len!=page_size)
					BUG();

			eccornot=1;
			am8218_nand_dma_rw((uint8_t * )info->aml_nand_dma_buf,len,0,eccornot);
		}
	}
}


//read+ecc correct
static int am8218_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip,uint8_t *buf){

		am8218_nand_dma_read_buf(mtd,buf,chip->ecc.size);		//mtd->writesize

		return 0;

}



static void am8218_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf){

		am8218_nand_dma_write_buf(mtd,buf,chip->ecc.size);	//mtd->writesize



}


//FOr NIKE use SLC NAND, bootldr no ecc , kernel+fs use bch 9
//FIXME  this func is used  to check a block is bad or not( by check the user buye , but the tranfer is disable bch) , so
// the user byte maybe wrong ; if use __MLC__ NAND , this may happen , need more test
static int am8218_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
			     int page, int sndcmd)
{

	uint32_t pagenum=0 ;
	uint32_t sectors=mtd->writesize/512;
	uint8_t * ptr=NULL;
	uint32_t checkbad;
	//uint8_t tmp;
	uint8_t *oobrbuf;
	unsigned nand_page_size = mtd->writesize;

	if(0) {
		memset(chip->oob_poi,0xff,mtd->oobsize);
	}else{

		chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);

		am8218_nand_dma_rw(chip->buffers->databuf,mtd->writesize+mtd->oobsize,1,0);	//read with out ecc

		//oobrbuf=chip->buffers->oobrbuf;
		if (nand_page_size >= 512) {
			oobrbuf=&chip->buffers->databuf[nand_page_size];
			for(pagenum=0;pagenum<sectors-1;pagenum++){

					ptr=chip->buffers->databuf+(pagenum+1)*528;
					ptr=ptr-16;
					memcpy(oobrbuf+pagenum*16,ptr,16);
			}

			if(sectors==1)
				checkbad=oobrbuf[0];
			else
				checkbad=oobrbuf[0]|(oobrbuf[16]<<8)| (oobrbuf[32]<<16)|(oobrbuf[48]<<24);

			if(checkbad==USER_BYTES_3_0)
					return 0;

			if(checkbad==0xffffffff)
					return 0;						//this for am_block_isbad

			if(checkbad!=USER_BYTES_3_0)
					return 1;	//-1;
		}

		//tmp=chip->buffers->oobrbuf[1]
		//chip->buffers->oobrbuf[1]
	}

	return 0;

}

#define STATUS_FAILED			0x01
#define STATUS_FAILED2			0x02
//May be write new dat to user byte pos
static int am8218_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip,
			      int page)
{
	uint8_t status[4];
	uint8_t *oobrbuf;

	unsigned nand_page_size = mtd->writesize;
	oobrbuf=chip->oob_poi;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, nand_page_size, page);
	am8218_nand_write_buf(mtd, oobrbuf, mtd->oobsize);
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
	am8218_nand_read_buf(mtd, status, 4);
	if(status[0] & STATUS_FAILED)
		return 1;

	/*printk(" WRITE OOB  , NOT IMPLENMENT\n");
	BUG();*/
	return 0;
}

static int am8218_nand_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
                  uint8_t *buf)
{
    am8218_nand_read_oob(mtd,chip,-1,0);

    memcpy(buf,chip->buffers->databuf,mtd->writesize);

    //  am8218_nand_dma_read_buf(mtd,buf,(mtd->writesize + mtd->oobsize));
    return 0;
}

//got yfs mtd->block_isbad -> nand_block_isbad ->here
//0 for ok 1 for bad
extern int nand_dev_num;
static int am8218_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	int32_t ret;
	struct nand_chip * chip=mtd->priv;
	uint32_t page=ofs>>chip->page_shift;
	struct am8218_nand_info *info = mtd_to_nand_info(mtd);
	struct am8218_nand_platform *plat = info->platform;
	unsigned short page_size = plat->page_size;

	if(getchip)
		nand_get_chip();

	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);

	if (page_size == 512) {
		ret = am8218_nand_dma_rw(chip->buffers->databuf, mtd->writesize + mtd->oobsize, 1, 0);
		if(ret)
			return 1;
		if( chip->buffers->databuf[mtd->writesize + BADBLOCK_FLAG_BYTE-1] != 0xff ) {
			printk(" NAND detect Bad block at %x \n",(uint32_t)ofs);
			return 1;
		}
		memset(chip->buffers->databuf,0,mtd->writesize + mtd->oobsize);
		chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page+1);
		ret = am8218_nand_dma_rw(chip->buffers->databuf, mtd->writesize + mtd->oobsize, 1, 0); //second page
		if(ret)
			return 1;
		if( chip->buffers->databuf[mtd->writesize + BADBLOCK_FLAG_BYTE-1] != 0xff ) {
			printk(" NAND detect Bad block at %x \n",(uint32_t)ofs);
			return 1;
		}
    }
	else {
		ret=am8218_nand_dma_rw(chip->buffers->databuf,mtd->writesize ,1,1);
		if(ret<0){
			ret=am8218_nand_read_oob(mtd, chip,page, 1);
			if(ret!=0)
				printk(" NAND detect Bad block at %x \n",(uint32_t)ofs);
		}
	}

	if(getchip) {
	 	nand_release_chip();
	}

	return ret;
 }


// may be call nand_default_block_markbad--->nand_do_write_oob--->chip->ecc.writeoob
static int am8218_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	printk(" MARK the BAD BLOCK , NOT IMPLENMENT\n");
	BUG();
	return 0;
}

static int am8218_nand_hw_init(struct am8218_nand_info *info)
{
	struct am8218_nand_platform *plat = info->platform;

	dev_info(info->device,"page_size=%d, data_width=%d, dma_dly=%d, twb_dly=%d\n",
		plat->page_size,plat->data_width,plat->dma_dly, plat->twb_dly);

	nand_config|=(0<<1)|(1<<6)| (1 << 0); 	//|(1 << 7);

#if  defined(ONFIG_AMLOGIC_BOARD_APOLLO) ||\
	defined(ONFIG_AMLOGIC_BOARD_APOLLO_H)
	nand_config|=(1<<7); 					// NAND connected to DCU1
#endif

#if defined(CONFIG_AMLOGIC_BOARD_APOLLO) ||\
    defined(CONFIG_AMLOGIC_BOARD_APOLLO_H)
	nand_config|=1<<11;
	nand_config|=2<<12;
#endif
	NAND_SET_MANUAL_CONFIG(nand_config);

	if((plat->page_size==4096)||(plat->page_size==2048))
		nand_config|=	(1<<3)|(1<< 2) ; // Large page (2k), 3 ALE
	else
		nand_config|= (0<<3)|(0<< 2);	  //small 512 ,2 ALE

	if(plat->data_width==16)
		nand_config|=  (1 << 1);           // 16-bit strang
	else
		nand_config|=  (0<<1);

	dev_info(info->device, "Nand Config is 0x%04x\n", nand_config);

	NAND_SET_MANUAL_CONFIG(nand_config);

	return 0;
}


static int am8218_nand_add_partition(struct am8218_nand_info *info){
	struct mtd_info *mtd = &info->mtd;

#ifdef CONFIG_MTD_PARTITIONS
	struct mtd_partition *parts = info->platform->partitions;
	int nr = info->platform->nr_partitions;

	return add_mtd_partitions(mtd, parts, nr);
#else
	return add_mtd_device(mtd);
#endif
}

static int am8218_nand_remove(struct platform_device *pdev){

	struct am8218_nand_info *info = to_nand_info(pdev);
	struct mtd_info *mtd = NULL;

	platform_set_drvdata(pdev, NULL);

	mtd = &info->mtd;
	if (mtd) {
		nand_release(mtd);
		kfree(mtd);
	}

	kfree(info);
	return 0;
}


static int am8218_nand_probe(struct platform_device *pdev){

	struct am8218_nand_platform *plat = to_nand_plat(pdev);
	struct am8218_nand_info *info = NULL;
	struct nand_chip *chip = NULL;
	struct mtd_info *mtd = NULL;
	unsigned char *aml_nand_dma_buf_temp = NULL;
	int err = 0;
	unsigned page_shift, maf_idx;

	dev_dbg(&pdev->dev, "(%p)\n", pdev);

	if (!plat) {
		dev_err(&pdev->dev, "no platform specific information\n");
		goto exit_error;
	}

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL) {
		dev_err(&pdev->dev, "no memory for flash info\n");
		err = -ENOMEM;
		goto exit_error;
	}

	platform_set_drvdata(pdev, info);

	spin_lock_init(&info->controller.lock);
	init_waitqueue_head(&info->controller.wq);

	info->device     = &pdev->dev;
	info->platform   = plat;

	chip = &info->chip;

	if (plat->data_width==16)
		chip->options |= NAND_BUSWIDTH_16;

	chip->options |=  NAND_SKIP_BBTSCAN;

	chip->buffers = kmalloc(sizeof(struct nand_buffers), GFP_KERNEL);
	if (info->chip.buffers == NULL) {
		dev_err(&pdev->dev, "no memory for flash info\n");
		err = -ENOMEM;
		goto exit_error;
	}
	chip->options |= NAND_OWN_BUFFERS;

	chip->read_buf = (plat->data_width==16) ?
		am8218_nand_read_buf16 : am8218_nand_read_buf;
	chip->write_buf = (plat->data_width==16) ?
		am8218_nand_write_buf16 : am8218_nand_write_buf;

	chip->read_byte    = am8218_nand_read_byte;						//NOTE MUST PROVIDE , the defualt cause kernel OOP
	chip->cmd_ctrl     = am8218_nand_hwcontrol;
	chip->dev_ready    = am8218_nand_devready;
	chip->block_bad 	=am8218_nand_block_bad;
	chip->block_markbad =am8218_nand_block_markbad;
	chip->priv	   = &info->mtd;
	chip->controller   = &info->controller;

#if defined(CONFIG_AMLOGIC_BOARD_APOLLO_H)
	chip->IO_ADDR_R    = (void __iomem *) (PNAND_MANUAL_DATA_REG+AHBOFFSET);
	chip->IO_ADDR_W    = (void __iomem *) (PNAND_MANUAL_DATA_REG+AHBOFFSET);
#else//for nike,aopollo
	chip->IO_ADDR_R    = (void __iomem *) (PNAND_MANUAL_DATA_REG+PERIPHSBASE);
	chip->IO_ADDR_W    = (void __iomem *) (PNAND_MANUAL_DATA_REG+PERIPHSBASE);
#endif
	chip->chip_delay   = 400;										//FIXME

	/* initialise mtd info data struct */
	mtd 		= &info->mtd;
	mtd->priv	= chip;
	mtd->owner	= THIS_MODULE;


	nand_get_chip();

	err = am8218_nand_hw_init(info);
	if (err != 0)
		goto exit_error;

    nand_id_read(flash_id);
    if(nf_id_valid(flash_id) < 0){
        printk("\nInvalid ID, No NAND device found or not support\n");
        return -1;
    }
    else{
        for (maf_idx = 0; nand_flash_ids[maf_idx].id != 0x0; maf_idx++) {
                if (nand_flash_ids[maf_idx].id == flash_id[0])
                    break;
        }
    }

	if((flash_id[1] == 0x35) || (flash_id[1] == 0x45) || (flash_id[1] == 0x55) || (flash_id[1] == 0x75)
		|| (flash_id[1] == 0x76) || (flash_id[1] == 0x36) || (flash_id[1] == 0x46) || (flash_id[1] == 0x56)) {
		page_shift = 9;
		plat->page_size = (1 << page_shift);
	}
	else {
	page_shift =  PageShiftTable[flash_id[3] & 0x3];
	plat->page_size = (1 << page_shift);
    }

	if (hardware_ecc) {
		if(plat->page_size==4096){
				chip->ecc.steps=1;
				chip->ecc.bytes=128;
				chip->ecc.size =4096;
				chip->badblock_pattern=&slcpage_memorybased;
		}
		else if (plat->page_size ==2048) {
			chip->ecc.steps=1;									//for one read per page
			chip->ecc.bytes= 64;
			chip->ecc.size =2048;
			chip->badblock_pattern=&slcpage_memorybased;			//not consider slc small page case, put this here , for mtd mem bbt
		}
		else if (plat->page_size == 512) {
			chip->ecc.steps=1;
			chip->ecc.bytes = 16;
			chip->ecc.size = 512;
			nand_config &= ~(1<<6);
		}
		NAND_SET_MANUAL_CONFIG(nand_config);

		aml_nand_dma_buf_temp = kzalloc(chip->ecc.size+chip->ecc.bytes+64, GFP_KERNEL);
		if (aml_nand_dma_buf_temp == NULL) {
			dev_err(&pdev->dev, "no memory for flash info\n");
			err = -ENOMEM;
			goto exit_error;
		}
		info->aml_nand_dma_buf = ((unsigned)aml_nand_dma_buf_temp+ARC_DCACHE_LINE_LEN-1)&DCACHE_LINE_MASK;

		chip->read_buf      = am8218_nand_dma_read_buf;
		chip->write_buf     = am8218_nand_dma_write_buf;
	chip->ecc.read_page = am8218_nand_read_page;
	 	chip->ecc.write_page = am8218_nand_write_page;
	chip->ecc.read_oob  = am8218_nand_read_oob;
	chip->ecc.write_oob = am8218_nand_write_oob;
	chip->ecc.calculate = am8218_nand_calculate_ecc;
	chip->ecc.correct   = am8218_nand_correct_data;
		if (plat->page_size == 512) {
		chip->ecc.mode = NAND_ECC_SOFT;
	}
		else {
		chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.hwctl	    = am8218_nand_enable_hwecc;
		}
		//chip->ecc.read_page_raw=am8218_nand_read_page_raw;
	} else {
		chip->ecc.mode	    = NAND_ECC_SOFT;
	}

	if (nand_scan(mtd, 1)) {
		err = -ENXIO;
		goto exit_error;
	}

	if(am8218_nand_add_partition(info)!=0){
		err = -ENXIO;
		goto exit_error;
	}

	dev_dbg(&pdev->dev, "initialised ok\n");


	//nand_release_chip();

	return 0;

exit_error:
//changed by zhouzhi 09-8-11
//because we have not add the mtd device at the error occur;
//this operate should make the system broke!!
//error occur at  no nand device;

//	am8218_nand_remove(pdev);
	kfree(info);
	if (err == 0)
		err = -EINVAL;
	return err;
}


#define DRV_NAME	"am8218-nand"
#define DRV_VERSION	"0.1"
#define DRV_AUTHOR	"pfs"
#define DRV_DESC	"Amlogic 8218 on-chip NAND FLash Controller Driver"


/* driver device registration */
static struct platform_driver am8218_nand_driver = {
	.probe		= am8218_nand_probe,
	.remove		= am8218_nand_remove,
//	.suspend	= am8218_nand_suspend,
//	.resume		= am8218_nand_resume,
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};



static int __init am8218_nand_init(void)
{
	printk(KERN_INFO "%s, Version %s (c) 2008 Amlogic Inc.\n",
		DRV_DESC, DRV_VERSION);

	return platform_driver_register(&am8218_nand_driver);
}

static void __exit am8218_nand_exit(void)
{
	platform_driver_unregister(&am8218_nand_driver);
}

module_init(am8218_nand_init);
module_exit(am8218_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_ALIAS("platform:" DRV_NAME);

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
#include <mach/irqs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#define MAX_INFO_LEN 128
#define CM0	((1<<29) | (1<<27) | (1<<25) | (1<<23))
#define SM0	(1<<30) | (1<<28) | (1<<26) | (1<<24)
#define CM7	((1<<29) | (1<<27) | (1<<28) | (1<<26)|(1<<24))
struct aml_m1_nand_info
{
	struct nand_hw_control			controller;
	struct mtd_info					mtd;
	struct nand_chip				chip;
	//
	//struct mtd_info				specmtd;
	//struct nand_chip				specchip;
	//
	struct aml_m1_nand_platform*	platform;
	struct device*					device;
	unsigned int 					bch_mode;
	unsigned int 					encode_size;
	unsigned int 					ppb;
	unsigned int 					ce_sel;
	unsigned int 					chip_num;
	unsigned int 					planemode;
	unsigned int					interlmode;
	unsigned int					curpage;
	unsigned int 					cur_rd_len;
//	unsigned int 					cmdque[64];
//	unsigned int 					cmdlen;
	uint8_t *						aml_nand_dma_buf;
	unsigned char * 				info_buf;
	dma_addr_t                      aml_nand_dma_buf_dma_addr;
	dma_addr_t 						aml_nand_info_dma_addr;
};

static unsigned nand_timing = 0;
static unsigned int def_sparebytes = 0x6d616d61;
static struct nand_ecclayout m1_ecclayout;
static struct completion m1_nand_rb_complete;

//FIXME , nand.h no use  extern
extern  int nand_get_device(struct nand_chip *chip, struct mtd_info *mtd,int new_state);
extern  void nand_release_device(struct mtd_info *mtd);

static void aml_m1_nand_check_clock()
{
    unsigned timing;
    if (READ_CBUS_REG(HHI_MPEG_CLK_CNTL)&(1<<8)){
        // normal
        timing = NFC_GET_CFG()&0x3ff;
        if ((nand_timing&0x3ff) != timing){
            printk("CLK81 = %x\n", READ_CBUS_REG(HHI_MPEG_CLK_CNTL));
            printk("NAND OLD CONFIG IS 0x%x: %d %d %d\n", timing, (timing>>5)&7, timing&0x1f, (timing>>10)&0x0f);
            NFC_SET_CFG(nand_timing);
            timing = NFC_GET_CFG()&0x3ff;
            printk("NAND NEW CONFIG IS 0x%x: %d %d %d\n", timing, (timing>>5)&7, timing&0x1f, (timing>>10)&0x0f);
        }
    }
    else{
        // xtal
        timing = NFC_GET_CFG()&0x3ff;
        if ((nand_timing&0x3ff) == timing){
            printk("CLK81 = %x\n", READ_CBUS_REG(HHI_MPEG_CLK_CNTL));
            printk("NAND OLD CONFIG IS 0x%x: %d %d %d\n", timing, (timing>>5)&7, timing&0x1f, (timing>>10)&0x0f);
            NFC_SET_TIMING(0, 5, -6); // mode, cycle, adjust
            timing = NFC_GET_CFG()&0x3ff;
            printk("NAND NEW CONFIG IS 0x%x: %d %d %d\n", timing, (timing>>5)&7, timing&0x1f, (timing>>10)&0x0f);
        }
    }
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

static void aml_m1_nand_select_chip(struct mtd_info *mtd, int chipnr)
{
	//struct nand_chip *chip = mtd->priv;
	struct aml_m1_nand_info * info=mtd_to_nand_info(mtd);

	clear_mio_mux(1,CM0);
	set_mio_mux(1,SM0);
	//set_mio_mux(6,0x7fff);
	set_mio_mux(6,0x7ff);
	clear_mio_mux(7,CM7);

	switch (chipnr) {
		case -1:
		//	chip->cmd_ctrl(mtd, NAND_CMD_NONE, 0 | NAND_CTRL_CHANGE);
			set_mio_mux(1,CM0);
			clear_mio_mux(1,SM0);
			//clear_mio_mux(6,0x7fff);
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

//	info->ce_sel=CE1;
	return ;
}

static void aml_m1_nand_hwcontrol(struct mtd_info *mtd, int cmd,  unsigned int ctrl)
{
	struct aml_m1_nand_info * info=mtd_to_nand_info(mtd);
	volatile int val=0;
	unsigned int  ce=info->ce_sel;

	if(cmd==NAND_CMD_NONE)
		return;

    aml_m1_nand_check_clock();

	if(ctrl&NAND_CLE)
	{
		val=NFC_CMD_CLE(ce,cmd);
		NFC_SEND_CMD(val);
		//if(cmd==NAND_CMD_READSTART)
		{

			/*	if(info->ce_sel==CE0)
				bit=1;
				else
				if(info->ce_sel==CE1)
				bit=2;
				*/
			//	while(NFC_CMDFIFO_SIZE()>0);
			//	init_completion(&m1_nand_rb_complete);
			//	SET_CBUS_REG_MASK(A9_0_IRQ_IN1_INTR_MASK,1<<2);
			//	SET_CBUS_REG_MASK(A9_0_IRQ_IN1_INTR_STAT,1<<2);
			//	NFC_SEND_CMD_RB_INT(info->ce_sel,12,bit);				//2^30
		}
	}
	else if(ctrl & NAND_ALE)
	{
		val=NFC_CMD_ALE(ce,cmd);
		NFC_SEND_CMD(val);
	}
	else
	{
		BUG();
	}
	return;
}
static irqreturn_t aml_m1_nand_rbirq(int irq, void *dev_id)
{
	struct aml_m1_nand_info * info=dev_id;
	unsigned  int bit,val=0;

	if(info->ce_sel==CE0)
		bit=1;
	else
		if(info->ce_sel==CE1)
			bit=2;
		else
			if(info->ce_sel==CE2)
				bit=4;
			else
				if(info->ce_sel==CE3)
					bit=8;
				else{
					BUG();
				}

	val=NFC_INFO_GET();

	if(!((NFC_INFO_GET()>>26)&(bit)))
	{
	//	printk("NAND RB NOT TIME OUT ENTER IRQ");
		BUG();
	}

	/*if((val&(1<<25))&&(!((val>>26)&(bit))))
	{
		printk("NAND RB TIME OUT BUT BUSY ENTER IRQ");
		BUG();
	}

	if(!((NFC_INFO_GET())&(1<<25)))
	{
		printk("NAND RB NOT TIME OUT ENTER IRQ");
	//	BUG();
	}
	*/
    printk("NAND RB NOT TIME OUT ENTER IRQ");
	SET_CBUS_REG_MASK(A9_0_IRQ_IN1_INTR_STAT,1<<2);
//	complete(&m1_nand_rb_complete);
//	printk(" get  rb busy ok \n");

	return IRQ_HANDLED;
}
static int aml_m1_nand_devready(struct mtd_info *mtd)
{
	struct aml_m1_nand_info * info=mtd_to_nand_info(mtd);
	unsigned int bit=0;


	if(info->interlmode!=1)
	{
		if(info->ce_sel==CE0)
			bit=1;
		else
			if(info->ce_sel==CE1)
				bit=2;
			else
				if(info->ce_sel==CE2)
					bit=4;
				else
					if(info->ce_sel==CE3)
						bit=8;
					else
						BUG();

		//	if((NFC_INFO_GET()>>26)&bit)
		//		return 1;

		return (NFC_INFO_GET()>>26)&bit;
	}
	else
	{
		//	return (NFC_INFO_GET()>>26)&1;
		//	if(chip->state == FL_WRITING)		return 1;
		return (((NFC_INFO_GET()>>26)&0x1)&&((NFC_INFO_GET()>>26)&0x2));
	}

}

static int aml_m1_nand_waitfunc(struct mtd_info *mtd, struct nand_chip * chip)
{
	int rc0=0,rc1=0;
	struct aml_m1_nand_info * info=mtd_to_nand_info(mtd);

	if (chip->state == FL_WRITING || chip->state == FL_ERASING)
	{
		while(!(chip->dev_ready(mtd)));
		info->ce_sel=CE0;
		chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
		rc0= (int)chip->read_byte(mtd);
		if(info->interlmode!=0){
			info->ce_sel=CE1;
			chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
			rc1= (int)chip->read_byte(mtd);
		}
		if((rc0&NAND_STATUS_FAIL)||(rc1&NAND_STATUS_FAIL))
			return NAND_STATUS_FAIL;
		else
			return 0;

	}
	return 0;
}

static void aml_m1_erase_cmd(struct mtd_info *mtd, int page)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	struct nand_chip *chip = &(aml_info->chip);
	unsigned int tmp_pag=0;

	aml_m1_nand_check_clock();

	if((aml_info->interlmode==0)&&(aml_info->planemode==0))
	{
		chip->cmdfunc(mtd, NAND_CMD_ERASE1, -1, page);
		chip->cmdfunc(mtd, NAND_CMD_ERASE2, -1, -1);
	}else if((aml_info->interlmode!=0)&&(aml_info->planemode==0))
	{
	BUG();
	}else if((aml_info->interlmode==0)&&(aml_info->planemode!=0))
	{
		volatile unsigned char status;
		NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
		NFC_SEND_CMD_RB(aml_info->ce_sel,31);
		tmp_pag = (aml_info->curpage/aml_info->ppb) *(aml_info-> ppb) + aml_info->curpage;
		chip->cmd_ctrl(mtd, 0x60, NAND_CLE );
		chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag >> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, 0x60, NAND_CLE );
		chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, 0xD0, NAND_CLE );
		do{
			status = NFC_GET_RB_STATUS(aml_info->ce_sel);
		}while((status!=1)&&(status!=2));

		NFC_SEND_CMD_RB(CE0,0);
		NFC_SEND_CMD_IDLE(CE0,0);
		chip->cmd_ctrl(mtd, 0x70, NAND_CLE );

		do{
			status = NFC_GET_RB_STATUS(aml_info->ce_sel);
		}while((status!=1)&&(status!=2));

    NFC_SEND_CMD_IDLE(aml_info->ce_sel,3);
    NFC_SEND_CMD_RB(aml_info->ce_sel,0);
	  NFC_SEND_CMD_N2M (4,0);

	  NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
	  NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
		NFC_SEND_CMD_RB(aml_info->ce_sel,0);
	  while(NFC_CMDFIFO_SIZE()>0);
	  status = NFC_GET_BUF();
	  if(status&0x1==1)
		 printk("erase error status=%d \n",status);

	}
	else if((aml_info->interlmode!=0)&&(aml_info->planemode!=0))
	{
	    int val;
		aml_info->ce_sel=CE0;
		tmp_pag = (aml_info->curpage/aml_info->ppb) *(aml_info-> ppb) + aml_info->curpage;
		chip->cmd_ctrl(mtd, 0x60, NAND_CLE );
		chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag >> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, 0xd1, NAND_CLE );
		aml_info->ce_sel=CE1;
		chip->cmd_ctrl(mtd, 0x60, NAND_CLE );
		chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag >> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, 0xd1, NAND_CLE );

//		while(!(aml_m1_nand_devready(mtd)));
        aml_info->ce_sel=CE0;
        NFC_SEND_CMD_RB(CE0,31);
		chip->cmd_ctrl(mtd, 0x60, NAND_CLE );
		chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, 0xD0, NAND_CLE );
		aml_info->ce_sel=CE1;
		NFC_SEND_CMD_RB(CE1,31);
		chip->cmd_ctrl(mtd, 0x60, NAND_CLE );
		chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, 0xD0, NAND_CLE );
		NFC_SEND_CMD_RB(CE1|CE0,31);
		while(NFC_CMDFIFO_SIZE()>0);
        wmb();
		NFC_SET_DADDR(aml_info->aml_nand_dma_buf_dma_addr);
		NFC_SET_IADDR(aml_info->aml_nand_info_dma_addr);
		memset(aml_info->info_buf,0,8);
        wmb();
		volatile unsigned * i=(unsigned*)&aml_info->info_buf[4];
		aml_info->ce_sel=CE0;
		chip->cmd_ctrl(mtd, 0x70, NAND_CLE );
        NFC_SEND_CMD_IDLE(CE0,15);
	    NFC_SEND_CMD_N2M (512,0);
	    NFC_SEND_CMD_IDLE(CE0,0);
	    NFC_SEND_CMD_IDLE(CE0,0);
	    while(NFC_CMDFIFO_SIZE()>0);

	    aml_info->ce_sel=CE1;
		chip->cmd_ctrl(mtd, 0x70, NAND_CLE );
        NFC_SEND_CMD_IDLE(CE1,15);
	    NFC_SEND_CMD_N2M (512,0);
	    NFC_SEND_CMD_IDLE(CE1,0);
	    NFC_SEND_CMD_IDLE(CE1,0);
        rmb();
	    while(NFC_CMDFIFO_SIZE()>0);
	    while(*i==0);
        rmb();
	    i=(unsigned*)aml_info->aml_nand_dma_buf;
	}
}
static void aml_m1_nand_cmdfunc(struct mtd_info *mtd, unsigned command,
				int column, int page_addr)
{
	struct aml_m1_nand_info * info=mtd_to_nand_info(mtd);
	struct nand_chip *chip = &(info->chip);

	info->curpage=page_addr;

    aml_m1_nand_check_clock();

	switch (command)
	{
		case NAND_CMD_READID:
		    NFC_SEND_CMD(info->ce_sel|IDLE | 1);
			chip->cmd_ctrl(mtd, command & 0xff, NAND_CLE);
			chip->cmd_ctrl(mtd,0, NAND_ALE);
			NFC_SEND_CMD_IDLE(info->ce_sel,31);
	        NFC_SEND_CMD_RB(info->ce_sel,31);
			break;
		case NAND_CMD_STATUS:
			NFC_SEND_CMD(info->ce_sel|IDLE | 1);
			chip->cmd_ctrl(mtd, command & 0xff, NAND_CLE);
			NFC_SEND_CMD(info->ce_sel|IDLE | 1);
			NFC_SEND_CMD(info->ce_sel|IDLE | 1);
			NFC_SEND_CMD(info->ce_sel|IDLE | 1);
			break;
		case NAND_CMD_RESET:
			chip->cmd_ctrl(mtd, command & 0xff, NAND_CLE);
			udelay(chip->chip_delay);
			chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
					NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
			NFC_SEND_CMD_IDLE(info->ce_sel,31);
	        NFC_SEND_CMD_RB(info->ce_sel,31);
//			while (!(chip->read_byte(mtd) & NAND_STATUS_READY)) ;
			break;
		case NAND_CMD_READ0:
			if((info->interlmode==0)&&(info->planemode==0))
			{
				chip->cmd_ctrl(mtd, NAND_CMD_READ0, NAND_CLE );
				chip->cmd_ctrl(mtd, column, NAND_ALE);
				chip->cmd_ctrl(mtd, column >> 8, NAND_ALE);
				chip->cmd_ctrl(mtd, page_addr, NAND_ALE);
				chip->cmd_ctrl(mtd, page_addr >> 8, NAND_ALE);
				chip->cmd_ctrl(mtd, page_addr >> 16,NAND_ALE);
				chip->cmd_ctrl(mtd, NAND_CMD_READSTART,NAND_CLE);
			}
			break;
		case NAND_CMD_SEQIN:
			if((info->interlmode==0)&&(info->planemode==0))
			{
				chip->cmd_ctrl(mtd, command & 0xff, NAND_CLE );
				chip->cmd_ctrl(mtd, column, NAND_ALE);
				chip->cmd_ctrl(mtd, column >> 8, NAND_ALE);
				chip->cmd_ctrl(mtd, page_addr, NAND_ALE);
				chip->cmd_ctrl(mtd, page_addr >> 8, NAND_ALE);
				chip->cmd_ctrl(mtd, page_addr >> 16,NAND_ALE);
			}
			break;
		case NAND_CMD_PAGEPROG:
			if((info->interlmode==0)&&(info->planemode==0))
			{
				chip->cmd_ctrl(mtd, command & 0xff, NAND_CLE );
			}
			break;
		case NAND_CMD_ERASE1:
			chip->cmd_ctrl(mtd, command & 0xff, NAND_CLE );
			chip->cmd_ctrl(mtd, page_addr, NAND_ALE);
			chip->cmd_ctrl(mtd, page_addr >> 8, NAND_ALE);
			chip->cmd_ctrl(mtd, page_addr >> 16,NAND_ALE);
			break;
		case NAND_CMD_ERASE2:
			chip->cmd_ctrl(mtd, command & 0xff, NAND_CLE );
			break;
		default:
			printk(KERN_ERR "non-supported command is 0x%x.\n",command);
			BUG();
			break;
	}

}
static uint8_t aml_m1_nand_read_byte(struct mtd_info *mtd)
{
	uint8_t val;
	struct aml_m1_nand_info * info=mtd_to_nand_info(mtd);
	unsigned int  ce=info->ce_sel;

	aml_m1_nand_check_clock();
	NFC_SEND_CMD(ce|DRD | 0);
	NFC_SEND_CMD(ce|IDLE | 5);
	NFC_SEND_CMD(ce|IDLE | 5);
	while(NFC_CMDFIFO_SIZE()>0);
	val=NFC_GET_BUF()&0xff;
	return val;
}
static unsigned int skip_oob=0;

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
			//	mtd->ecc_stats.failed++;
			if(aml_info->ce_sel==CE0)
				res=-1;										//FIXME -1 for err
			else if(aml_info->ce_sel==CE1)
				res=-2;
			else BUG();

			//			printk("ecc_info %d in %d  is %x  res %x \n",i,len,pinfobuf[i],-res);
		}
		else{

			//	mtd->ecc_stats.corrected+=NAND_ECC_CNT(pinfobuf[i]);
		}

		if(!skip_oob){

			if(aml_info->bch_mode!=NAND_ECC_BCH9)
				p[i]=pinfobuf[i]&0xffff;
			else
				ptr[i]=pinfobuf[i]&0xff;
		}
	}
	return res;
}

static void prepare_info_buf_before_write(struct mtd_info *mtd,int len, int ecc)
{
	int i;
	struct aml_m1_nand_info *aml_info=mtd_to_nand_info(mtd);
	struct nand_chip *chip  = mtd->priv;
	volatile uint16_t *p	=(uint16_t *)chip->oob_poi;
	volatile uint8_t  *ptr	=(uint8_t *)chip->oob_poi;
	volatile uint32_t *pinfobuf=(uint32_t *)aml_info->info_buf;

	if(ecc!=NAND_ECC_NONE)
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

//static int  aml_m1_nand_dma_read(struct mtd_info *mtd, dma_addr_t data_dma_addr, int len,int ecc)
static int  aml_m1_nand_dma_read(struct mtd_info *mtd, uint8_t *buf, int len,int ecc)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	volatile int res=0;
	dma_addr_t data_dma_addr=(dma_addr_t)buf;
	volatile 	unsigned int * pbuf=(unsigned int *)((unsigned char *)aml_info->info_buf+((len>>9)-1)*4);

    aml_m1_nand_check_clock();

	memset(aml_info->info_buf,0,MAX_INFO_LEN*sizeof(int));
    wmb();
	NFC_SEND_CMD(aml_info->ce_sel|IDLE | 1);
	//	while(NFC_CMDFIFO_SIZE()>0);
	//	    data_dma_addr=dma_map_single(aml_info->device,(void *)buf,len,DMA_FROM_DEVICE);
	NFC_SEND_CMD_ADL(data_dma_addr);
	NFC_SEND_CMD_ADH(data_dma_addr);
	NFC_SEND_CMD_AIL(aml_info->aml_nand_info_dma_addr);
	NFC_SEND_CMD_AIH((aml_info->aml_nand_info_dma_addr));
	NFC_SEND_CMD_N2M(len,ecc);
	while(NFC_CMDFIFO_SIZE()>0);
	while(NAND_INFO_DONE(*pbuf)==0);
	//	    dma_unmap_single(aml_info->device,data_dma_addr,len,DMA_FROM_DEVICE);
    rmb();

	if((ecc!=NAND_ECC_NONE))
		res=transfer_info_buf_after_read(mtd,(len+511)>>9);			//FIXME 2*page

	return res;
}
static void aml_m1_nand_dma_write(struct mtd_info *mtd, const uint8_t *buf, int len,int ecc)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	dma_addr_t data_dma_addr;

    aml_m1_nand_check_clock();

	prepare_info_buf_before_write(mtd,(len+511)>>9,ecc);

	data_dma_addr=dma_map_single(aml_info->device,(void *)buf,len,DMA_TO_DEVICE);
    wmb();
	NFC_SEND_CMD_ADL(data_dma_addr);
	NFC_SEND_CMD_ADH(data_dma_addr);
	NFC_SEND_CMD_AIL(aml_info->aml_nand_info_dma_addr);
	NFC_SEND_CMD_AIH(aml_info->aml_nand_info_dma_addr);
	NFC_SEND_CMD_M2N(len,ecc);
	NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
	NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
	while(NFC_CMDFIFO_SIZE()>0);
    rmb();
	dma_unmap_single(aml_info->device,data_dma_addr,len,DMA_TO_DEVICE);
}

static void aml_m1_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
//    printk("read buf\n");
    aml_m1_nand_check_clock();

	aml_m1_nand_dma_read(mtd,buf,len,NAND_ECC_NONE);
}

static void aml_m1_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
//	aml_m1_nand_dma_write(mtd,buf,len,NAND_ECC_NONE);
}
	//upper need  negative for err
static int aml_m1_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip,uint8_t *buf_i,int page)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	int rc0=0,rc1=0;
	unsigned int tmp_pag;
	unsigned * temp=(unsigned*)aml_info->aml_nand_dma_buf;
	//	dma_addr_t data_dma_addr=0;
	//	unsigned int len=mtd->writesize;
	//	data_dma_addr=dma_map_single(aml_info->device,(void *)buf,len,DMA_FROM_DEVICE);
	//
	uint8_t * buf=(uint8_t*)aml_info->aml_nand_dma_buf_dma_addr;
	volatile unsigned  status;

    aml_m1_nand_check_clock();

	if((aml_info->interlmode!=0)&&(aml_info->planemode==0))
	{
		BUG();
	}
	else if((aml_info->planemode!=0)&&((aml_info->interlmode==0)))
	{
		if(!(strcmp(mtd->name,"Hynix")) || !(strcmp(mtd->name,"Samsung")))
		{

			tmp_pag = (aml_info->curpage/aml_info->ppb) *(aml_info->ppb) + aml_info->curpage;
		  NFC_SEND_CMD_RB(aml_info->ce_sel,0);

			chip->cmd_ctrl(mtd, 0x60,NAND_CLE );
			chip->cmd_ctrl(mtd, tmp_pag,NAND_ALE);
			chip->cmd_ctrl(mtd, (tmp_pag)>>8, NAND_ALE);
			chip->cmd_ctrl(mtd,	(tmp_pag)>>16,NAND_ALE);
			chip->cmd_ctrl(mtd, 0x60, NAND_CLE );
			chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
			chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
			chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
			chip->cmd_ctrl(mtd, NAND_CMD_READSTART,NAND_CLE);

			NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
			NFC_SEND_CMD_RB(aml_info->ce_sel,31);
			do{
				status = NFC_GET_RB_STATUS(aml_info->ce_sel);
		    }while((status!=1)&&(status!=2));
            rmb();

			chip->cmd_ctrl(mtd, 0x00, NAND_CLE );
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
			chip->cmd_ctrl(mtd, (tmp_pag) >> 8, NAND_ALE);
			chip->cmd_ctrl(mtd,	( tmp_pag)>> 16,NAND_ALE);
			chip->cmd_ctrl(mtd, 0x05, NAND_CLE );
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, 0xe0,NAND_CLE);
			while(NFC_CMDFIFO_SIZE()>0);
            rmb();
			rc0=aml_m1_nand_dma_read(mtd,buf,mtd->writesize/2,aml_info->bch_mode);
			if(rc0!=0)
			{
//				printk(" ce0 read rc0 error !\n");
//				printk(" temp[0]=0x%x !\n",temp[0]);
//				printk(" temp[1]=0x%x !\n",temp[1]);
//				printk("aml_info->ce_sel=%d aml_info->curpage=0x%x\n",aml_info->ce_sel,aml_info->curpage);
				return rc0;
			}
			chip->cmd_ctrl(mtd, 0x00, NAND_CLE );
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
			chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
			chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
			chip->cmd_ctrl(mtd, 0x05, NAND_CLE );
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, 0xe0,NAND_CLE);
			while(NFC_CMDFIFO_SIZE()>0);
            rmb();
			rc1=aml_m1_nand_dma_read(mtd,buf+mtd->writesize/2,mtd->writesize/2,aml_info->bch_mode);
			if(rc1!=0)
			{
//				printk("aml_info->curpage is = 0x%x \n",aml_info->curpage);
//				printk(" ce0 read rc1 error !\n");
				return rc1;
			}
		}
		else if(!(strcmp(mtd->name,"Micron")))
		{
			tmp_pag = (aml_info->curpage/aml_info->ppb) *(aml_info-> ppb) + aml_info->curpage;
			chip->cmd_ctrl(mtd, NAND_CMD_READ0, NAND_CLE );
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
			chip->cmd_ctrl(mtd, (tmp_pag) >> 8, NAND_ALE);
			chip->cmd_ctrl(mtd,	( tmp_pag)>> 16,NAND_ALE);
			chip->cmd_ctrl(mtd, NAND_CMD_READ0, NAND_CLE );
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
			chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
			chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
			chip->cmd_ctrl(mtd, NAND_CMD_READSTART,NAND_CLE);
			while(NFC_CMDFIFO_SIZE()>0);

            rmb();
			NFC_SEND_CMD_RB(aml_info->ce_sel,0);
			rc0=aml_m1_nand_dma_read(mtd,buf,mtd->writesize/2,aml_info->bch_mode);

			skip_oob=1;

			chip->cmd_ctrl(mtd, 0x06, NAND_CLE );
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
			chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
			chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
			chip->cmd_ctrl(mtd, 0xe0, NAND_CLE );
            rmb();
			rc1=aml_m1_nand_dma_read(mtd,buf+mtd->writesize/2,mtd->writesize/2,aml_info->bch_mode);
			if(rc1!=0){

				//			printk("%d:%d %d %d %d %x\n",__LINE__,rc0,rc1,buf[0],((unsigned)buf)&7,temp[0]);
				//			printk("%x %x %x %x\n",temp[256-1],temp[512-1],temp[768-1],temp[1023]);
				//memset(buf+mtd->writesize-20,0xBB,20);
				return rc1;
			}
			if(rc0!=0){
				//			printk("%d:%d %d %d %d %x\n",__LINE__,rc0,rc1,buf[0],((unsigned)buf)&7,temp[0]);
				//			printk("%x %x %x %x\n",temp[256-1],temp[512-1],temp[768-1],temp[1023]);
				//memset(buf+mtd->writesize-20,0xBB,20);
				return rc0;
			}
			skip_oob=0;
		}
	}
	else if((aml_info->planemode!=0)&&((aml_info->interlmode!=0)))
	{
		tmp_pag = (aml_info->curpage/aml_info->ppb) *(aml_info-> ppb) + aml_info->curpage;
		aml_info->ce_sel=CE0;
		chip->cmd_ctrl(mtd, NAND_CMD_READ0, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag)>> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, NAND_CMD_READ0, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, NAND_CMD_READSTART,NAND_CLE);
		while(NFC_CMDFIFO_SIZE()>0);
        rmb();

		if(aml_info->cur_rd_len==(512))
		{
			NFC_SEND_CMD_RB(aml_info->ce_sel,0);
			rc0=aml_m1_nand_dma_read(mtd,buf,512,aml_info->bch_mode);
			return rc0;
		}

		aml_info->ce_sel=CE1;
		chip->cmd_ctrl(mtd, NAND_CMD_READ0, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag)>> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, NAND_CMD_READ0, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, NAND_CMD_READSTART,NAND_CLE);
		while(NFC_CMDFIFO_SIZE()>0);
        rmb();
		aml_info->ce_sel=CE0;
		NFC_SEND_CMD_RB(aml_info->ce_sel,0);
		rc0=aml_m1_nand_dma_read(mtd,buf,mtd->writesize/4,aml_info->bch_mode);

		skip_oob=1;

		chip->cmd_ctrl(mtd, 0x06, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, 0xe0, NAND_CLE );
        rmb();
		rc1=aml_m1_nand_dma_read(mtd,buf+mtd->writesize/4,mtd->writesize/4,aml_info->bch_mode);
		if(rc1!=0){

			//			printk("%d:%d %d %d %d %x\n",__LINE__,rc0,rc1,buf[0],((unsigned)buf)&7,temp[0]);
			//			printk("%x %x %x %x\n",temp[256-1],temp[512-1],temp[768-1],temp[1023]);
			//memset(buf+mtd->writesize-20,0xBB,20);
			return rc1;
		}
		if(rc0!=0){
			//			printk("%d:%d %d %d %d %x\n",__LINE__,rc0,rc1,buf[0],((unsigned)buf)&7,temp[0]);
			//			printk("%x %x %x %x\n",temp[256-1],temp[512-1],temp[768-1],temp[1023]);
			//memset(buf+mtd->writesize-20,0xBB,20);
			return rc0;
		}
		aml_info->ce_sel=CE1;
		NFC_SEND_CMD_RB(aml_info->ce_sel,0);
		rc0=aml_m1_nand_dma_read(mtd,buf+mtd->writesize/2,mtd->writesize/4,aml_info->bch_mode);
		chip->cmd_ctrl(mtd, 0x06, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
		chip->cmd_ctrl(mtd, 0xe0, NAND_CLE );
        rmb();
		rc1=aml_m1_nand_dma_read(mtd,buf+(3*mtd->writesize)/4,mtd->writesize/4,aml_info->bch_mode);
		if(rc1!=0){
			//			printk("%d:%d %d %d %d %x\n",__LINE__,rc0,rc1,buf[0],((unsigned)buf)&7,temp[0]);
			//			printk("%x %x %x %x %d\n",temp[2048+256-1],temp[2048+512-1],temp[2048+768-1],temp[2048+1023],mtd->writesize);
			//memset(buf+mtd->writesize-20,0xBB,20);
			return rc1;
		}
		if(rc0!=0){
			    //printk("%d:%d %d %d %d %x\n",__LINE__,rc0,rc1,buf[0],((unsigned)buf)&7,temp[0]);
			//	printk("%x %x %x %x %d\n",temp[2048+256-1],temp[2048+512-1],temp[2048+768-1],temp[2048+1023],mtd->writesize);
			//memset(buf+mtd->writesize-20,0xBB,20);
			return rc0;
		}

		//		return 0;

		skip_oob=0;
	}
	else
	{
		rc0=aml_m1_nand_dma_read(mtd,buf,mtd->writesize,aml_info->bch_mode);
		if(rc0!=0){
			//printk("%d error\n",__LINE__);
			//memset(buf+mtd->writesize-20,0xBB,20);
			return rc0;
		}
		//		return rc0;
	}
	if(buf_i&&buf_i!=aml_info->aml_nand_dma_buf)
	{
		//printk("%s  wer\n",__func__);
			    //unsigned *temp1=(unsigned*)buf;
		        //printk("%d:%x %x \n",__LINE__,temp[0],temp1[0]);
        rmb();
		memcpy(buf_i,aml_info->aml_nand_dma_buf,mtd->writesize);
	}else{
		//        printk("%d:%x %x\n",__LINE__,buf_i,aml_info->aml_nand_dma_buf);
	}
	return 0;

}

/*static void aml_m1_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	unsigned int tmp_pag=0;
	if((aml_info->interlmode==0)&&(aml_info->planemode==0))
		aml_m1_nand_dma_write(mtd,buf,mtd->writesize,aml_info->bch_mode);
	else if((aml_info->interlmode!=0)&&(aml_info->planemode==0))
	{
		BUG();
	}
	else if((aml_info->planemode!=0)&&((aml_info->interlmode==0)))
	{
		BUG();
	}
	else if((aml_info->planemode!=0)&&((aml_info->interlmode!=0)))
	{
		aml_info->ce_sel=CE0;
		tmp_pag = (aml_info->curpage/aml_info->ppb) *(aml_info-> ppb) + aml_info->curpage;
		chip->cmd_ctrl(mtd, 0x80, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag)>> 16,NAND_ALE);
		aml_m1_nand_dma_write(mtd,buf,mtd->writesize/4,aml_info->bch_mode);
		chip->cmd_ctrl(mtd, 0x11, NAND_CLE );
		while(!(aml_m1_nand_devready(mtd)));
		chip->cmd_ctrl(mtd, 0x80, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
		aml_m1_nand_dma_write(mtd,buf+mtd->writesize/4,mtd->writesize/4,aml_info->bch_mode);
		chip->cmd_ctrl(mtd, 0x10, NAND_CLE );
		while(NFC_CMDFIFO_SIZE()>0);
		aml_info->ce_sel=CE1;
		chip->cmd_ctrl(mtd, 0x80, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag)>> 16,NAND_ALE);
		aml_m1_nand_dma_write(mtd,buf+mtd->writesize/2,mtd->writesize/4,aml_info->bch_mode);
		chip->cmd_ctrl(mtd, 0x11, NAND_CLE );
		while(!(aml_m1_nand_devready(mtd)));
		chip->cmd_ctrl(mtd, 0x80, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);
		aml_m1_nand_dma_write(mtd,buf+(3*mtd->writesize)/4,mtd->writesize/4,aml_info->bch_mode);
		chip->cmd_ctrl(mtd, 0x10, NAND_CLE );
	}
}*/

static void aml_m1_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf_i)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	unsigned int tmp_pag=0;
	uint8_t * buf=(uint8_t*)aml_info->aml_nand_dma_buf_dma_addr;

    aml_m1_nand_check_clock();

	if((aml_info->interlmode==0)&&(aml_info->planemode==0))
		aml_m1_nand_dma_write(mtd,buf,mtd->writesize,aml_info->bch_mode);
	else if((aml_info->interlmode!=0)&&(aml_info->planemode==0))
	{
			BUG();
	}
	else if((aml_info->planemode!=0)&&((aml_info->interlmode==0)))
	{
			volatile unsigned char status;
			unsigned len=mtd->writesize/2;

	    NFC_SET_DADDR(aml_info->aml_nand_dma_buf_dma_addr);
	    NFC_SET_IADDR(aml_info->aml_nand_info_dma_addr);
			memcpy(aml_info->aml_nand_dma_buf,buf_i,len);
			prepare_info_buf_before_write(mtd,(len+511)>>9,aml_info->bch_mode);
            wmb();
			NFC_SEND_CMD_RB(aml_info->ce_sel,0);
			tmp_pag = (aml_info->curpage/aml_info->ppb) *(aml_info-> ppb) + aml_info->curpage;

			chip->cmd_ctrl(mtd, 0x80, NAND_CLE );
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
			chip->cmd_ctrl(mtd, (tmp_pag) >> 8, NAND_ALE);
			chip->cmd_ctrl(mtd,	(tmp_pag)>> 16,NAND_ALE);
			NFC_SEND_CMD_M2N(len,aml_info->bch_mode);
			chip->cmd_ctrl(mtd, 0x11, NAND_CLE );
			while(NFC_CMDFIFO_SIZE()>0);
			NFC_SEND_CMD_IDLE(aml_info->ce_sel,10);
			NFC_SEND_CMD_RB(aml_info->ce_sel,31);
			memcpy(&(aml_info->aml_nand_dma_buf[len]),&buf_i[len],len);
		memcpy(&(aml_info->info_buf[((len)>>7)]),aml_info->info_buf,(len)>>7);
        wmb();
//	  	NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
//			NFC_SEND_CMD_RB(aml_info->ce_sel,31);
			do{
				status = NFC_GET_RB_STATUS(aml_info->ce_sel);
			}while((status!=1)&&(status!=2));
            rmb();
			if(!(strcmp(mtd->name,"Hynix")))
			{
				chip->cmd_ctrl(mtd, 0x81, NAND_CLE );
			}
			else if(!(strcmp(mtd->name,"Micron")))
			{
				chip->cmd_ctrl(mtd, 0x80, NAND_CLE );
			}
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, 0, NAND_ALE);
			chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
			chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
			chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);

			NFC_SEND_CMD_M2N(len,aml_info->bch_mode);
			chip->cmd_ctrl(mtd, 0x10, NAND_CLE );
			while(NFC_CMDFIFO_SIZE()>0);
			NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
			NFC_SEND_CMD_RB(aml_info->ce_sel,31);
			do{
				status = NFC_GET_RB_STATUS(aml_info->ce_sel);
			}while((status!=1)&&(status!=2));
            rmb();

			chip->cmd_ctrl(mtd, 0x70, NAND_CLE );
			NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
			do{
				status = NFC_GET_RB_STATUS(aml_info->ce_sel);
			}while((status!=1)&&(status!=2));
            rmb();

	    NFC_SEND_CMD_IDLE(aml_info->ce_sel,3);
	    NFC_SEND_CMD_RB(aml_info->ce_sel,0);
		  NFC_SEND_CMD_N2M (4,0);
		  NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
		  NFC_SEND_CMD_IDLE(aml_info->ce_sel,0);
		  NFC_SEND_CMD_RB(aml_info->ce_sel,0);
		  while(NFC_CMDFIFO_SIZE()>0);
		  status = NFC_GET_BUF();
		  if(status&0x1==1)
			printk("write error status=%d \n",status);


	}
	else if((aml_info->planemode!=0)&&((aml_info->interlmode!=0)))
	{

		unsigned len=mtd->writesize/4;

	    NFC_SET_DADDR(aml_info->aml_nand_dma_buf_dma_addr);
	    NFC_SET_IADDR(aml_info->aml_nand_info_dma_addr);

		memcpy(aml_info->aml_nand_dma_buf,buf_i,len);
	//	memcpy(aml_info->aml_nand_dma_buf,buf_i,mtd->writesize);			//bug
		prepare_info_buf_before_write(mtd,(len+511)>>9,aml_info->bch_mode);
        wmb();

	    aml_info->ce_sel=CE0;
		tmp_pag = (aml_info->curpage/aml_info->ppb) *(aml_info-> ppb) + aml_info->curpage;
		chip->cmd_ctrl(mtd, 0x80, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	(tmp_pag)>> 16,NAND_ALE);
		NFC_SEND_CMD_M2N(len,aml_info->bch_mode);
		chip->cmd_ctrl(mtd, 0x11, NAND_CLE );

		aml_info->ce_sel=CE0;
			memcpy(&(aml_info->aml_nand_dma_buf[len]),&buf_i[len],len);
		memcpy(&(aml_info->info_buf[((len)>>7)]),aml_info->info_buf,(len)>>7);
        wmb();
		NFC_SEND_CMD_IDLE(CE0,10);
		NFC_SEND_CMD_RB(CE0,31);
		chip->cmd_ctrl(mtd, 0x80, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);


		NFC_SEND_CMD_M2N(len,aml_info->bch_mode);
		chip->cmd_ctrl(mtd, 0x10, NAND_CLE );


		aml_info->ce_sel=CE1;
			memcpy(&(aml_info->aml_nand_dma_buf[2*len]),&buf_i[2*len],len);
		memcpy(&(aml_info->info_buf[2*((len)>>7)]),aml_info->info_buf,(len)>>7);
        wmb();
		chip->cmd_ctrl(mtd, 0x80, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag)>> 16,NAND_ALE);


		NFC_SEND_CMD_M2N(len,aml_info->bch_mode);
		chip->cmd_ctrl(mtd, 0x11, NAND_CLE );


		aml_info->ce_sel=CE1;
		memcpy(&(aml_info->aml_nand_dma_buf[3*len]),&buf_i[3*len],len);
		memcpy(&(aml_info->info_buf[3*((len)>>7)]),aml_info->info_buf,(len)>>7);
        wmb();
		NFC_SEND_CMD_IDLE(CE1,10);
		NFC_SEND_CMD_RB(CE1,31);
		chip->cmd_ctrl(mtd, 0x80, NAND_CLE );
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, 0, NAND_ALE);
		chip->cmd_ctrl(mtd, tmp_pag+aml_info->ppb, NAND_ALE);
		chip->cmd_ctrl(mtd, (tmp_pag+aml_info->ppb) >> 8, NAND_ALE);
		chip->cmd_ctrl(mtd,	( tmp_pag+aml_info->ppb)>> 16,NAND_ALE);

		NFC_SEND_CMD_M2N(len,aml_info->bch_mode);
		chip->cmd_ctrl(mtd, 0x10, NAND_CLE );

		NFC_SEND_CMD_IDLE(CE0|CE1,10);
		NFC_SEND_CMD_RB(CE0|CE1,31);
		NFC_SEND_CMD_IDLE(CE0|CE1,0);
		NFC_SEND_CMD_IDLE(CE0|CE1,0);
		while(NFC_CMDFIFO_SIZE()>0);
	}
}

static int aml_m1_nand_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip,uint8_t *buf,int page)
{
	BUG();
	return 0;
}

static int aml_m1_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip,int page)
{
	BUG();
	return -EIO;
}
static int aml_m1_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	printk(" MARK the BAD BLOCK , NOT IMPLENMENT\n");
//	BUG();
	return 0;
}
static int aml_m1_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip,int page, int sndcmd)
{
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	int res=0;

    aml_m1_nand_check_clock();

	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);

	//NFC_SET_SPARE_ONLY();

	aml_info->cur_rd_len=512;

	//res=aml_m1_nand_dma_read(mtd,aml_info->aml_nand_dma_buf,mtd->writesize,aml_info->bch_mode);
	res=chip->ecc.read_page(mtd,chip,aml_info->aml_nand_dma_buf,mtd->writesize);
    rmb();

	//NFC_CLEAR_SPARE_ONLY();

	aml_info->cur_rd_len=0;

	if(res!=0)
	{
		return 1;			//nousefor upp
	}
	return res;
}
static int aml_m1_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	volatile int ret=0,chipnr;
	struct nand_chip * chip=mtd->priv;
	struct aml_m1_nand_info * aml_info=mtd_to_nand_info(mtd);
	volatile	int lpage = (int)(ofs >> chip->page_shift);
	volatile int	page  =	lpage&chip->pagemask;
//	volatile	unsigned int size=((mtd->writesize+mtd->oobsize)/(aml_info->encode_size))<<9;				//FIXME for boot!=normal
	unsigned int size=mtd->writesize;

    aml_m1_nand_check_clock();

	if(getchip)					//eraseing upper
	{
		nand_get_device(chip,mtd,FL_READING);
		chipnr = (int)(ofs>>chip->chip_shift);			//swi to 1 , but still on 0
		chip->select_chip(mtd, chipnr);
	}

	NFC_SET_SPARE_ONLY();
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
    rmb();
	ret=chip->ecc.read_page(mtd,chip,aml_info->aml_nand_dma_buf,size);
    rmb();
	NFC_CLEAR_SPARE_ONLY();

	if(getchip)
		nand_release_device(mtd);
	if(ret!=0)
	{
		printk("Get bad if at off %#llx page %d  chipnr %d res %x \n", (unsigned long long )ofs,aml_info->curpage,chipnr,-ret);
		return 1;
	}


	return ret;
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
	//ok

	clear_mio_mux(1,( (1<<29) | (1<<27) | (1<<25) | (1<<23)));
	set_mio_mux(1,(1<<30) | (1<<28) | (1<<26) | (1<<24));
	//set_mio_mux(6,0x7fff);
	set_mio_mux(6,0x7ff);
	clear_mio_mux(7,((1<<29) | (1<<27) | (1<<28) | (1<<26))|(1<<24));
	//return 0;
	//ok

	NFC_SET_CFG(0);
	NFC_SET_TIMING(0,19,0)  ;
	NFC_SEND_CMD(1<<31);

	while (NFC_CMDFIFO_SIZE() > 0);

	printk("\n%s over\n",__func__);

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
	nand_timing = NFC_GET_CFG();
	printk("NAND CONFIG IS 0x%04x \n",NFC_GET_CFG(), (NFC_GET_CFG()>>5)&7, NFC_GET_CFG()&0x1f, (NFC_GET_CFG()>>10)&0x0f);

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

static int aml_m1_nand_probe_1(struct platform_device *pdev)
{
	printk("\n%s\n",__func__);
	return 0;
}
static int aml_m1_nand_probe(struct platform_device *pdev)
{
	struct aml_m1_nand_platform *plat = to_nand_plat(pdev);
	struct aml_m1_nand_info *info = NULL;
	struct nand_chip *chip = NULL;
	struct mtd_info *mtd = NULL;
	int err = 0;
//	dev_dbg(&pdev->dev, "(%p)\n", pdev);

	printk("NAND probe pdev %d \n", (unsigned )pdev);

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

/*
	SET_CBUS_REG_MASK(A9_0_IRQ_IN1_INTR_MASK,1<<2);
	SET_CBUS_REG_MASK(A9_0_IRQ_IN1_INTR_STAT,1<<2);
	err = request_irq(INT_NAND, &aml_m1_nand_rbirq, IRQF_SHARED, "aml_nand_rb", info);
*/
	platform_set_drvdata(pdev, info);

	spin_lock_init(&info->controller.lock);
	init_waitqueue_head(&info->controller.wq);

	pdev->dev.coherent_dma_mask =DMA_BIT_MASK(64);
//	pdev->dev.irq=INT_NAND;

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
	chip->options |=  NAND_SKIP_BBTSCAN;		//FIXME  add part stiill but we skip bbt bld
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

	printk("\call naml_m1_nand_hw_init before %s\n",__func__);
//	return 0;

	err = aml_m1_nand_hw_init(info);
	if (err != 0)
		goto exit_error;

	//printk("\n%s\n",__func__);
	//return 0;

	printk("\naml_m1_nand_hw_init end\n");
	//goto exit_error;

	if(plat->page_size!=512)
	{
		chip->ecc.steps		= 1;
//		chip->ecc.bytes		= plat->spare_size;	 												//for simple  step 1
//		chip->ecc.size 		= plat->page_size;
//		chip->phys_erase_shift=ffs(plat->erase_size)-1;
//		info->ppb=plat->erase_size/plat->page_size;
		if(plat->planemode==1)
		{
			info->planemode=1;
			chip->planemode=1;
			chip->erase_cmd = aml_m1_erase_cmd;
			chip->cmdfunc		=aml_m1_nand_cmdfunc;
		//	chip->waitfunc=aml_m1_nand_waitfunc;
		}
		if(plat->interlmode==1)
		{
			info->interlmode=1;
			chip->interlmode=1;
			chip->erase_cmd = aml_m1_erase_cmd;
			chip->cmdfunc		=aml_m1_nand_cmdfunc;
		//	chip->waitfunc=aml_m1_nand_waitfunc	;
		}
		//	chip->cmdfunc		=aml_m1_nand_cmdfunc;
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

//		m1_ecclayout.oobavail =(plat->page_size/512)*((plat->bch_mode!=NAND_ECC_BCH9)?2:1);			//fill oob twice
//		m1_ecclayout.oobfree[0].offset=0;
//		m1_ecclayout.oobfree[0].length=m1_ecclayout.oobavail;									//FIXME base 2960

//		info->aml_nand_dma_buf = dma_alloc_coherent(info->device,4*(plat->page_size+plat->spare_size),
//		                       &(info->aml_nand_dma_buf_dma_addr), GFP_KERNEL);
//       // printk("%x %x\n",info->aml_nand_dma_buf,info->aml_nand_dma_buf_dma_addr);
//		if (info->aml_nand_dma_buf == NULL)
//		{
//			dev_err(&pdev->dev, "no memory for flash info\n");
//			err = -ENOMEM;
//			goto exit_error;
//		}
//
//		info->info_buf=	dma_alloc_coherent(info->device,MAX_INFO_LEN*sizeof(int),&(info->aml_nand_info_dma_addr),GFP_KERNEL);
//		if (info->info_buf== NULL)
//		{
//			dev_err(&pdev->dev, "no memory for flash info\n");
//			err = -ENOMEM;
//			goto exit_error;
//		}
//		printk("\nplat->page_size :%d\n",plat->page_size);
//	}
//   	else
//   	{
//		BUG();
//	}
	if (nand_scan(mtd, plat->chip_num))			//FIXME chip_num!=ce_num
	{
		err = -ENXIO;
		goto exit_error;
	}
	info->ppb=mtd->erasesize/mtd->writesize;
	printk("info->ppb = %d mtd->erasesize = %d mtd->writesize = %d \n",info->ppb,mtd->erasesize,mtd->writesize);
	m1_ecclayout.oobavail =(mtd->writesize/2/512)*((plat->bch_mode!=NAND_ECC_BCH9)?2:1);			//fill oob twice
		m1_ecclayout.oobfree[0].offset=0;
		m1_ecclayout.oobfree[0].length=m1_ecclayout.oobavail;									//FIXME base 2960


		info->aml_nand_dma_buf = dma_alloc_coherent(info->device,2*(mtd->writesize+mtd->oobsize),
		                       &(info->aml_nand_dma_buf_dma_addr), GFP_KERNEL); //2 mean two plane mode
       // printk("%x %x\n",info->aml_nand_dma_buf,info->aml_nand_dma_buf_dma_addr);
		if (info->aml_nand_dma_buf == NULL)
		{
			dev_err(&pdev->dev, "no memory for flash info\n");
			err = -ENOMEM;
			goto exit_error;
		}

		info->info_buf=	dma_alloc_coherent(info->device,MAX_INFO_LEN*sizeof(int),&(info->aml_nand_info_dma_addr),GFP_KERNEL);
		if (info->info_buf== NULL)
		{
			dev_err(&pdev->dev, "no memory for flash info\n");
			err = -ENOMEM;
			goto exit_error;
		}
		printk("\nplat->page_size :%d\n",plat->page_size);
	}
	else
	{
		BUG();
	}


	printk("\n nand_scan over\n");

	if(aml_m1_nand_add_partition(info)!=0)
	{
		err = -ENXIO;
		goto exit_error;
	}

	dev_dbg(&pdev->dev, "initialised ok\n");

	printk("NAND initialised OK \n ");
//	SET_CBUS_REG_MASK(A9_0_IRQ_IN1_INTR_MASK,1<<2);
//	SET_CBUS_REG_MASK(A9_0_IRQ_IN1_INTR_STAT,1<<2);
//	err = request_irq(INT_NAND, &aml_m1_nand_rbirq, IRQF_SHARED, "aml_nand_rb", info);
//	if(err)
//	{
//		printk("NAND IRQ REQUEST ERR\n");
//		goto exit_error;
//	}

	return 0;

exit_error:

	kfree(info);
	printk("\n%s over exit_error\n",__func__);
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
	printk(KERN_INFO "%s, Version %s (c) 2010 Amlogic Inc.\n",DRV_DESC, DRV_VERSION);
//	asm("wfi");
//	print();
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

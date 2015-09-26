
/*
 * Aml nftl hw interface
 *
 * (C) 2012 8
 */


#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>

#include "aml_nftl_block.h"
extern void *aml_nftl_malloc(uint32 size);
extern void aml_nftl_free(const void *ptr);
//extern int aml_nftl_dbg(const char * fmt,args...);

int nand_read_page(struct aml_nftl_part_t *part,_physic_op_par *p);
int nand_write_page(struct aml_nftl_part_t *part,_physic_op_par *p);
int  nand_erase_superblk(struct aml_nftl_part_t *part,_physic_op_par *p);
int nand_mark_bad_blk(struct aml_nftl_part_t *part,_physic_op_par *p);
int nand_is_blk_good(struct aml_nftl_part_t *part,_physic_op_par *p);

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int  nand_erase_superblk(struct aml_nftl_part_t *part,_physic_op_par *p)
{
    struct aml_nftl_blk_t *aml_nftl_blk = (struct aml_nftl_blk_t *)aml_nftl_get_part_priv(part);
	struct mtd_info *mtd = aml_nftl_blk->mbd.mtd;
	struct erase_info aml_nftl_erase_info;

	memset(&aml_nftl_erase_info, 0, sizeof(struct erase_info));
	aml_nftl_erase_info.mtd = mtd;
	aml_nftl_erase_info.addr = mtd->erasesize;
	aml_nftl_erase_info.addr *= p->phy_page.blkNO_in_chip;
	aml_nftl_erase_info.len = mtd->erasesize;

	return mtd->_erase(mtd, &aml_nftl_erase_info);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_read_page(struct aml_nftl_part_t *part,_physic_op_par *p)
{
    struct aml_nftl_blk_t *aml_nftl_blk = (struct aml_nftl_blk_t *)aml_nftl_get_part_priv(part);
	struct mtd_info *mtd = aml_nftl_blk->mbd.mtd;
	struct mtd_oob_ops aml_oob_ops;
	loff_t from;
	size_t len, retlen =0;
	int ret;

	from = mtd->erasesize;
	from *= p->phy_page.blkNO_in_chip;
	from += p->phy_page.Page_NO * mtd->writesize;

	len = mtd->writesize;
	aml_oob_ops.mode = MTD_OPS_AUTO_OOB;
	aml_oob_ops.len = mtd->writesize;
	if(mtd->writesize <4096)
	aml_oob_ops.ooblen = 8;
	else
	aml_oob_ops.ooblen = BYTES_OF_USER_PER_PAGE;
	aml_oob_ops.ooboffs = mtd->ecclayout->oobfree[0].offset;
	aml_oob_ops.datbuf = p->main_data_addr;
	aml_oob_ops.oobbuf = p->spare_data_addr;

	aml_nftl_add_part_total_read(part);

	if (p->spare_data_addr)
		ret = mtd->_read_oob(mtd, from, &aml_oob_ops);
	else
		ret = mtd->_read(mtd, from, len, &retlen, p->main_data_addr);

	if (ret == -EUCLEAN)
	{
		//if (mtd->ecc_stats.corrected >= 10)
			//do read err
		//ret = 0;
		PRINT("read reclaim\n");
	}

	if ((ret!=0) &&(ret != -EUCLEAN))
		PRINT("aml_ops_read_page failed: %llx %d %d\n", from, p->phy_page.blkNO_in_chip, p->phy_page.Page_NO);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_write_page(struct aml_nftl_part_t *part,_physic_op_par *p)
{
    struct aml_nftl_blk_t *aml_nftl_blk = (struct aml_nftl_blk_t *)aml_nftl_get_part_priv(part);
	struct mtd_info *mtd = aml_nftl_blk->mbd.mtd;
	struct mtd_oob_ops aml_oob_ops;
	loff_t from;
	size_t len, retlen =0;
	int ret;

	from = mtd->erasesize;
	from *= p->phy_page.blkNO_in_chip;
	from += p->phy_page.Page_NO * mtd->writesize;

	len = mtd->writesize;
	aml_oob_ops.mode = MTD_OPS_AUTO_OOB;
	aml_oob_ops.len = mtd->writesize;
	if(mtd->writesize <4096)
	aml_oob_ops.ooblen = 8;
	else
	aml_oob_ops.ooblen = BYTES_OF_USER_PER_PAGE;
	aml_oob_ops.ooboffs = mtd->ecclayout->oobfree[0].offset;
	aml_oob_ops.datbuf = p->main_data_addr;
	aml_oob_ops.oobbuf = p->spare_data_addr;

	aml_nftl_add_part_total_write(part);

	if (p->spare_data_addr)
		ret = mtd->_write_oob(mtd, from, &aml_oob_ops);
	else
		ret = mtd->_write(mtd, from, len, &retlen, p->main_data_addr);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_is_blk_good(struct aml_nftl_part_t *part,_physic_op_par *p)
{
	int ret;
    struct aml_nftl_blk_t *aml_nftl_blk = (struct aml_nftl_blk_t *)aml_nftl_get_part_priv(part);
	struct mtd_info *mtd = aml_nftl_blk->mbd.mtd;
	loff_t from;

	from = mtd->erasesize;
	from *= p->phy_page.blkNO_in_chip;
	ret = mtd->_block_isbad(mtd, from);
	if(ret == 0)
	{
		return RET_YES;
	}
	else
	{
        return ret;
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_mark_bad_blk(struct aml_nftl_part_t *part,_physic_op_par *p)
{
	uchar ret;
    struct aml_nftl_blk_t *aml_nftl_blk = (struct aml_nftl_blk_t *)aml_nftl_get_part_priv(part);
	struct mtd_info *mtd = aml_nftl_blk->mbd.mtd;
	loff_t from;

	from = mtd->erasesize;
	from *= p->phy_page.blkNO_in_chip;
	ret = mtd->_block_markbad(mtd, from);
	if(ret == 0)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

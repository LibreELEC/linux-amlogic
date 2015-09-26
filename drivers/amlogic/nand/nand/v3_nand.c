
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

#include <mach/nand_m3.h>

#define a3_nand_debug(a...) printk(a)
static struct aml_nand_device *to_nand_dev(struct platform_device *pdev)
{
	return pdev->dev.platform_data;
}

#if 0		//Fix compile warning Lee.Rong 2011.04.18
static int a3_nand_boot_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf, int page)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t *oob_buf = chip->oob_poi;
	unsigned nand_page_size = A3_BOOT_WRITE_SIZE;
	unsigned pages_per_blk_shift = (chip->phys_erase_shift - chip->page_shift);
	int user_byte_num = (((nand_page_size + chip->ecc.size - 1) / chip->ecc.size) * aml_chip->user_byte_mode);
	int error = 0, i = 0, stat = 0;

	memset(buf, 0xff, (1 << chip->page_shift));
	WARN_ON(!aml_chip->valid_chip[0]);
	if (aml_chip->valid_chip[i]) {

		if (!aml_chip->aml_nand_wait_devready(aml_chip, i)) {
			printk ("read couldn`t found selected chip: %d ready\n", i);
			error = -EBUSY;
			goto exit;
		}

		error = aml_chip->aml_nand_dma_read(aml_chip, buf, nand_page_size, aml_chip->bch_mode);
		if (error)
			goto exit;

		aml_chip->aml_nand_get_user_byte(aml_chip, oob_buf, user_byte_num);
		stat = aml_chip->aml_nand_hwecc_correct(aml_chip, buf, nand_page_size, oob_buf);
		if (stat < 0) {
			mtd->ecc_stats.failed++;
			printk("aml nand read data ecc failed at blk %d chip %d\n", (page >> pages_per_blk_shift), i);
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

static void a3_nand_boot_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t *oob_buf = chip->oob_poi;
	unsigned nand_page_size = A3_BOOT_WRITE_SIZE;
	int user_byte_num = (((nand_page_size + chip->ecc.size - 1) / chip->ecc.size) * aml_chip->user_byte_mode);
	int error = 0, i = 0;

	WARN_ON(!aml_chip->valid_chip[0]);
	if (aml_chip->valid_chip[i]) {

		aml_chip->aml_nand_select_chip(aml_chip, i);

		aml_chip->aml_nand_set_user_byte(aml_chip, oob_buf, user_byte_num);
		error = aml_chip->aml_nand_dma_write(aml_chip, (unsigned char *)buf, nand_page_size, aml_chip->bch_mode);
		if (error)
			goto exit;
		aml_chip->aml_nand_command(aml_chip, NAND_CMD_PAGEPROG, -1, -1, i);
	}
	else {
		error = -ENODEV;
		goto exit;
	}

exit:
	return;
}

static int a3_nand_boot_write_page(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf, int page, int cached, int raw)
{
	int status, i, write_page;

	for (i=0; i<A3_BOOT_COPY_NUM; i++) {

		write_page = page + i*A3_BOOT_PAGES_PER_COPY;
		chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, write_page);

		if (unlikely(raw))
			chip->ecc.write_page_raw(mtd, chip, buf);
		else
			chip->ecc.write_page(mtd, chip, buf);

		if (!cached || !(chip->options & NAND_CACHEPRG)) {

			status = chip->waitfunc(mtd, chip);

			if ((status & NAND_STATUS_FAIL) && (chip->errstat))
				status = chip->errstat(mtd, chip, FL_WRITING, status, write_page);

			if (status & NAND_STATUS_FAIL)
				return -EIO;
		} else {

			status = chip->waitfunc(mtd, chip);
		}
	}

	return 0;
}

#endif


#define ECC_INFORMATION(name_a,bch_a,size_a,parity_a,user_a) {                \
        .name=name_a,.bch=bch_a,.size=size_a,.parity=parity_a,.user=user_a    \
    }
static struct ecc_desc_s ecc_list[]={
	[0]=ECC_INFORMATION("NAND_RAW_MODE",NAND_ECC_SOFT_MODE,0,0,0),
	[1]=ECC_INFORMATION("NAND_BCH8_512_MODE",NAND_ECC_BCH8_512_MODE,NAND_ECC_UNIT_SIZE,NAND_BCH8_512_ECC_SIZE,2),
	[2]=ECC_INFORMATION("NAND_BCH8_1K_MODE" ,NAND_ECC_BCH8_1K_MODE,NAND_ECC_UNIT_1KSIZE,NAND_BCH8_1K_ECC_SIZE,2),
	[3]=ECC_INFORMATION("NAND_BCH16_MODE" ,NAND_ECC_BCH16_MODE,NAND_ECC_UNIT_1KSIZE,NAND_BCH16_ECC_SIZE,2),
	[4]=ECC_INFORMATION("NAND_BCH24_MODE" ,NAND_ECC_BCH24_MODE,NAND_ECC_UNIT_1KSIZE,NAND_BCH24_ECC_SIZE,2),
	[5]=ECC_INFORMATION("NAND_BCH30_MODE" ,NAND_ECC_BCH30_MODE,NAND_ECC_UNIT_1KSIZE,NAND_BCH30_ECC_SIZE,2),
	[6]=ECC_INFORMATION("NAND_BCH40_MODE" ,NAND_ECC_BCH40_MODE,NAND_ECC_UNIT_1KSIZE,NAND_BCH40_ECC_SIZE,2),
	[7]=ECC_INFORMATION("NAND_BCH60_MODE" ,NAND_ECC_BCH60_MODE,NAND_ECC_UNIT_1KSIZE,NAND_BCH60_ECC_SIZE,2),
	[8]=ECC_INFORMATION("NAND_SHORT_MODE" ,NAND_ECC_SHORT_MODE,NAND_ECC_UNIT_SHORT,NAND_BCH60_ECC_SIZE,2),
};
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

	//platform_set_drvdata(pdev, aml_chip);

	/* initialize mtd info data struct */
	dev->coherent_dma_mask = DMA_BIT_MASK(32);

	aml_chip->device = dev;
	aml_chip->platform = plat;
	plat->aml_chip = aml_chip;
	chip = &aml_chip->chip;
	chip->priv = &aml_chip->mtd;
	mtd = &aml_chip->mtd;
	mtd->priv = chip;
	mtd->owner = THIS_MODULE;
    aml_chip->max_ecc=sizeof(ecc_list)/sizeof(ecc_list[0]);
    aml_chip->ecc=ecc_list;
    #if 0
    a3_nand_debug("A3 NAND Chip ECC ability\n");
    for(err=1;err<aml_chip->max_ecc;err++)
    {
        a3_nand_debug("name=%s,"
    }
    #endif
	err = aml_nand_init(aml_chip);
	if (err)
		goto exit_error;

	if (!strncmp((char*)plat->name, NAND_BOOT_NAME, strlen((const char*)NAND_BOOT_NAME))) {
		chip->ecc.read_page = NULL; //a3_nand_boot_read_page_hwecc;
		chip->ecc.write_page =NULL; //  a3_nand_boot_write_page_hwecc;
		chip->write_page =NULL  ;	//a3_nand_boot_write_page;
	}

	return 0;

exit_error:
	if (aml_chip)
		kfree(aml_chip);

	return err;
}
static int a3_nand_probe(struct platform_device *pdev)
{
	struct aml_nand_device *aml_nand_dev = to_nand_dev(pdev);
	struct aml_nand_platform *plat = NULL;
	int err = 0, i;

	printk(&pdev->dev, "(%p)\n", pdev);

	if (!aml_nand_dev) {
		printk(&pdev->dev, "no platform specific information\n");
		err = -ENOMEM;
		goto exit_error;
	}
	platform_set_drvdata(pdev, aml_nand_dev);

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

exit_error:
	return err;
}

static int a3_nand_remove(struct platform_device *pdev)
{
	struct aml_nand_device *aml_nand_dev = to_nand_dev(pdev);
	struct aml_nand_platform *plat = NULL;
	struct aml_nand_chip *aml_chip = NULL;
	struct mtd_info *mtd = NULL;
	int i;

	platform_set_drvdata(pdev, NULL);
	for (i=0; i<aml_nand_dev->dev_num; i++) {
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

	return 0;
}

#define DRV_NAME	"aml_m3_nand"
#define DRV_VERSION	"1.0"
#define DRV_AUTHOR	"xiaojun_yoyo"
#define DRV_DESC	"Amlogic nand flash host controll driver for m3"

/* driver device registration */
static struct platform_driver a3_nand_driver = {
	.probe		= a3_nand_probe,
	.remove		= a3_nand_remove,
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init a3_nand_init(void)
{
	printk(KERN_INFO "%s, Version %s (c) 2010 Amlogic Inc.\n", DRV_DESC, DRV_VERSION);

	return platform_driver_register(&a3_nand_driver);
}

static void __exit a3_nand_exit(void)
{
	platform_driver_unregister(&a3_nand_driver);
}

module_init(a3_nand_init);
module_exit(a3_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_ALIAS("platform:" DRV_NAME);

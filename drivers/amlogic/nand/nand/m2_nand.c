
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

static struct aml_nand_device *to_nand_dev(struct platform_device *pdev)
{
	return pdev->dev.platform_data;
}

static void aml_m2_select_chip(struct aml_nand_chip *aml_chip, int chipnr)
{
	switch (chipnr) {
		case 0:
		case 1:
		case 2:
		case 3:
			NFC_SEND_CMD_IDLE(aml_chip->chip_enable[chipnr], 0);
			aml_chip->chip_selected = aml_chip->chip_enable[chipnr];
			aml_chip->rb_received = aml_chip->rb_enable[chipnr];

			if (!((aml_chip->chip_selected >> 10) & 1))
				SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_5, (1 << 4));
			if (!((aml_chip->chip_selected >> 10) & 2))
				SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_5, (1 << 3));
			if (!((aml_chip->chip_selected >> 10) & 4))
				SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_5, (1 << 14));
			if (!((aml_chip->chip_selected >> 10) & 8))
				SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_5, (1 << 13));

			if (!((aml_chip->rb_received >> 10) & 1))
				SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_5, (1 << 2));
			if (!((aml_chip->rb_received >> 10) & 2))
				SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_5, (1 << 1));
			if (!((aml_chip->rb_received >> 10) & 4))
				SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_5, (1 << 12));
			if (!((aml_chip->rb_received >> 10) & 8))
				SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_5, (1 << 11));

			break;

		default:
			BUG();
			aml_chip->chip_selected = CE_NOT_SEL;
			break;
	}
	return;
}

static int m1_nand_boot_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf, int page)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t *oob_buf = chip->oob_poi;
	unsigned nand_page_size = M1_BOOT_WRITE_SIZE;
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

static void m1_nand_boot_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t *oob_buf = chip->oob_poi;
	unsigned nand_page_size = M1_BOOT_WRITE_SIZE;
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

static int m1_nand_boot_write_page(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf, int page, int cached, int raw)
{
	int status, i, write_page;

	for (i=0; i<M1_BOOT_COPY_NUM; i++) {

		write_page = page + i*M1_BOOT_PAGES_PER_COPY;
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

	aml_chip->aml_nand_select_chip = aml_m2_select_chip;
	aml_chip->device = dev;
	aml_chip->platform = plat;
	plat->aml_chip = aml_chip;
	chip = &aml_chip->chip;
	chip->priv = &aml_chip->mtd;
	mtd = &aml_chip->mtd;
	mtd->priv = chip;
	mtd->owner = THIS_MODULE;

	err = aml_nand_init(aml_chip);
	if (err)
		goto exit_error;

	if (!strncmp((char*)plat->name, NAND_BOOT_NAME, strlen((const char*)NAND_BOOT_NAME))) {
		chip->ecc.read_page = m1_nand_boot_read_page_hwecc;
		chip->ecc.write_page = m1_nand_boot_write_page_hwecc;
		chip->write_page = m1_nand_boot_write_page;
	}

	return 0;

exit_error:
	if (aml_chip)
		kfree(aml_chip);

	return err;
}
static int m1_nand_probe(struct platform_device *pdev)
{
	struct aml_nand_device *aml_nand_dev = to_nand_dev(pdev);
	struct aml_nand_platform *plat = NULL;
	int err = 0, i;

	dev_dbg(&pdev->dev, "(%p)\n", pdev);

	if (!aml_nand_dev) {
		dev_err(&pdev->dev, "no platform specific information\n");
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

static int m1_nand_remove(struct platform_device *pdev)
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

#define DRV_NAME	"aml_m2_nand"
#define DRV_VERSION	"1.0"
#define DRV_AUTHOR	"xiaojun_yoyo"
#define DRV_DESC	"Amlogic nand flash host controll driver for m1"

/* driver device registration */
static struct platform_driver m1_nand_driver = {
	.probe		= m1_nand_probe,
	.remove		= m1_nand_remove,
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init m1_nand_init(void)
{
	printk(KERN_INFO "%s, Version %s (c) 2010 Amlogic Inc.\n", DRV_DESC, DRV_VERSION);

	return platform_driver_register(&m1_nand_driver);
}

static void __exit m1_nand_exit(void)
{
	platform_driver_unregister(&m1_nand_driver);
}

module_init(m1_nand_init);
module_exit(m1_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_ALIAS("platform:" DRV_NAME);

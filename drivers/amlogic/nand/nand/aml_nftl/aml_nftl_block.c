/*
 * Aml nftl block device access
 *
 * (C) 2010 10
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mii.h>
#include <linux/skbuff.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/device.h>
#include <linux/pagemap.h>

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>

#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/freezer.h>
#include <linux/spinlock.h>
#include <linux/hdreg.h>
#include <linux/kthread.h>
#include <asm/uaccess.h>

#include <linux/hdreg.h>
#include <linux/blkdev.h>
#include <linux/reboot.h>

#include <linux/kmod.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/scatterlist.h>

#include "aml_nftl.h"

static struct mutex aml_nftl_lock;

#define nftl_notifier_to_blk(l)	container_of(l, struct aml_nftl_blk_t, nb)
#define cache_list_to_node(l)	container_of(l, struct write_cache_node, list)
#define free_list_to_node(l)	container_of(l, struct free_sects_list, list)

static int aml_nftl_write_cache_data(struct aml_nftl_blk_t *aml_nftl_blk, uint8_t cache_flag)
{
	struct aml_nftl_info_t *aml_nftl_info;
	int err = 0, i, j, limit_cache_cnt = NFTL_CACHE_FORCE_WRITE_LEN / 4;
	uint32_t blks_per_sect, current_cache_cnt;
	struct list_head *l, *n;

	current_cache_cnt = aml_nftl_blk->cache_buf_cnt;
	aml_nftl_info = aml_nftl_blk->aml_nftl_info;
	blks_per_sect = aml_nftl_info->writesize / 512;
	list_for_each_safe(l, n, &aml_nftl_blk->cache_list) {
		struct write_cache_node *cache_node = cache_list_to_node(l);

		for (i=0; i<blks_per_sect; i++) {
			if (!cache_node->cache_fill_status[i])
				break;
		}

		if (i < blks_per_sect) {
			err = aml_nftl_info->read_sect(aml_nftl_info, cache_node->vt_sect_addr, aml_nftl_blk->cache_buf);
			if (err)
				return err;

			aml_nftl_blk->cache_sect_addr = -1;
			//aml_nftl_dbg("nftl write cache data blk: %d page: %d \n", (cache_node->vt_sect_addr / aml_nftl_info->pages_per_blk), (cache_node->vt_sect_addr % aml_nftl_info->pages_per_blk));
			for (j=0; j<blks_per_sect; j++) {
				if (!cache_node->cache_fill_status[j])
					memcpy(cache_node->buf + j*512, aml_nftl_blk->cache_buf + j*512, 512);
			}
		}
		if (aml_nftl_blk->cache_sect_addr == cache_node->vt_sect_addr)
			memcpy(aml_nftl_blk->cache_buf, cache_node->buf, 512 * blks_per_sect);

		err = aml_nftl_info->write_sect(aml_nftl_info, cache_node->vt_sect_addr, cache_node->buf);
		if (err)
			return err;

		if (cache_node->bounce_buf_num < 0)
			aml_nftl_free(cache_node->buf);
		else
			aml_nftl_blk->bounce_buf_free[cache_node->bounce_buf_num] = NFTL_BOUNCE_FREE;
		list_del(&cache_node->list);
		aml_nftl_free(cache_node);
		aml_nftl_blk->cache_buf_cnt--;
		if ((cache_flag != CACHE_CLEAR_ALL) && (aml_nftl_blk->cache_buf_cnt <= (current_cache_cnt - limit_cache_cnt)))
			return 0;
	}

	return 0;
}

static int aml_nftl_search_cache_list(struct aml_nftl_blk_t *aml_nftl_blk, uint32_t sect_addr,
										uint32_t blk_pos, uint32_t blk_num, unsigned char *buf)
{
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_blk->aml_nftl_info;
	struct write_cache_node *cache_node;
	int err = 0, i, j;
	uint32_t blks_per_sect;
	struct list_head *l, *n;

	blks_per_sect = aml_nftl_info->writesize / 512;
	list_for_each_safe(l, n, &aml_nftl_blk->cache_list) {
		cache_node = cache_list_to_node(l);

		if (cache_node->vt_sect_addr == sect_addr) {
			for (i=blk_pos; i<(blk_pos+blk_num); i++) {
				if (!cache_node->cache_fill_status[i])
					break;
			}
			if (i < (blk_pos + blk_num)) {
				err = aml_nftl_info->read_sect(aml_nftl_info, sect_addr, aml_nftl_blk->cache_buf);
				if (err)
					return err;
				aml_nftl_blk->cache_sect_addr = sect_addr;

				for (j=0; j<blks_per_sect; j++) {
					if (cache_node->cache_fill_status[j])
						memcpy(aml_nftl_blk->cache_buf + j*512, cache_node->buf + j*512, 512);
				}
				memcpy(buf, aml_nftl_blk->cache_buf + blk_pos * 512, blk_num * 512);
			}
			else {
				memcpy(buf, cache_node->buf + blk_pos * 512, blk_num * 512);
			}

			return 0;
		}
	}

	return 1;
}

static int aml_nftl_add_cache_list(struct aml_nftl_blk_t *aml_nftl_blk, uint32_t sect_addr,
										uint32_t blk_pos, uint32_t blk_num, unsigned char *buf)
{
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_blk->aml_nftl_info;
	struct write_cache_node *cache_node;
	int err = 0, i, j, need_write = 0;
	uint32_t blks_per_sect;
	struct list_head *l, *n;

	blks_per_sect = aml_nftl_info->writesize / 512;
	list_for_each_safe(l, n, &aml_nftl_blk->cache_list) {
		cache_node = cache_list_to_node(l);

		if (cache_node->vt_sect_addr == sect_addr) {

			for (j=0; j<blks_per_sect; j++) {
				if (!cache_node->cache_fill_status[j])
					break;
			}
			if (j >= blks_per_sect)
				need_write = 1;

			for (i=blk_pos; i<(blk_pos+blk_num); i++)
				cache_node->cache_fill_status[i] = 1;
			memcpy(cache_node->buf + blk_pos * 512, buf, blk_num * 512);

			for (j=0; j<blks_per_sect; j++) {
				if (!cache_node->cache_fill_status[j])
					break;
			}

			if ((j >= blks_per_sect) && (need_write == 1)) {

				if (aml_nftl_blk->cache_sect_addr == cache_node->vt_sect_addr)
					memcpy(aml_nftl_blk->cache_buf, cache_node->buf, 512 * blks_per_sect);

				err = aml_nftl_info->write_sect(aml_nftl_info, sect_addr, cache_node->buf);
				if (err)
					return err;

				if (cache_node->bounce_buf_num < 0)
					aml_nftl_free(cache_node->buf);
				else
					aml_nftl_blk->bounce_buf_free[cache_node->bounce_buf_num] = NFTL_BOUNCE_FREE;
				list_del(&cache_node->list);
				aml_nftl_free(cache_node);
				aml_nftl_blk->cache_buf_cnt--;
			}

			return 0;
		}
	}

	if ((blk_pos == 0) && (blk_num == blks_per_sect))
		return CACHE_LIST_NOT_FOUND;

	if (aml_nftl_blk->cache_buf_cnt >= NFTL_CACHE_FORCE_WRITE_LEN) {
		err = aml_nftl_blk->write_cache_data(aml_nftl_blk, 0);
		if (err) {
			aml_nftl_dbg("nftl cache data full write faile %d err: %d\n", aml_nftl_blk->cache_buf_cnt, err);
			return err;
		}
	}

	cache_node = aml_nftl_malloc(sizeof(struct write_cache_node));
	if (!cache_node)
		return -ENOMEM;
	memset(cache_node->cache_fill_status, 0, MAX_BLKS_PER_SECTOR);

	for (i=0; i<NFTL_CACHE_FORCE_WRITE_LEN; i++) {
		if ((aml_nftl_blk->bounce_buf_free[i] == NFTL_BOUNCE_FREE) && (aml_nftl_blk->bounce_buf != NULL)) {
			cache_node->buf = (aml_nftl_blk->bounce_buf + i*aml_nftl_info->writesize);
			cache_node->bounce_buf_num = i;
			aml_nftl_blk->bounce_buf_free[i] = NFTL_BOUNCE_USED;
			break;
		}
	}

	if (!cache_node->buf) {
		aml_nftl_dbg("cache buf need malloc %d\n", aml_nftl_blk->cache_buf_cnt);
		cache_node->buf = aml_nftl_malloc(aml_nftl_info->writesize);
		if (!cache_node->buf)
			return -ENOMEM;
		cache_node->bounce_buf_num = -1;
	}

	cache_node->vt_sect_addr = sect_addr;
	for (i=blk_pos; i<(blk_pos+blk_num); i++)
		cache_node->cache_fill_status[i] = 1;
	memcpy(cache_node->buf + blk_pos * 512, buf, blk_num * 512);
	list_add_tail(&cache_node->list, &aml_nftl_blk->cache_list);
	aml_nftl_blk->cache_buf_cnt++;
	if (aml_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_IDLE) {
		aml_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_READY;
		wake_up_process(aml_nftl_blk->nftl_thread);
	}

#ifdef NFTL_DONT_CACHE_DATA
	if (aml_nftl_blk->cache_buf_cnt >= 1) {
		err = aml_nftl_blk->write_cache_data(aml_nftl_blk, 0);
		if (err) {
			aml_nftl_dbg("nftl cache data full write faile %d err: %d\n", aml_nftl_blk->cache_buf_cnt, err);
			return err;
		}
	}
#endif

	return 0;
}

static int aml_nftl_read_data(struct aml_nftl_blk_t *aml_nftl_blk, unsigned long block, unsigned nblk, unsigned char *buf)
{
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_blk->aml_nftl_info;
	unsigned blks_per_sect, read_sect_addr, read_misalign_num, buf_offset = 0;
	int i, error = 0, status = 0, total_blk_count = 0, read_sect_num;

	blks_per_sect = aml_nftl_info->writesize / 512;
	read_sect_addr = block / blks_per_sect;
	total_blk_count = nblk;
	read_misalign_num = ((blks_per_sect - block % blks_per_sect) > nblk ?  nblk : (blks_per_sect - block % blks_per_sect));

	if (block % blks_per_sect) {
		status = aml_nftl_blk->search_cache_list(aml_nftl_blk, read_sect_addr, block % blks_per_sect, read_misalign_num, buf);
		if (status) {
			if (read_sect_addr != aml_nftl_blk->cache_sect_addr) {
				error = aml_nftl_info->read_sect(aml_nftl_info, read_sect_addr, aml_nftl_blk->cache_buf);
				if (error)
					return error;
				aml_nftl_blk->cache_sect_addr = read_sect_addr;
			}
			memcpy(buf + buf_offset, aml_nftl_blk->cache_buf + (block % blks_per_sect) * 512, read_misalign_num * 512);
		}
		read_sect_addr++;
		total_blk_count -= read_misalign_num;
		buf_offset += read_misalign_num * 512;
	}

	if (total_blk_count >= blks_per_sect) {
		read_sect_num = total_blk_count / blks_per_sect;
		for (i=0; i<read_sect_num; i++) {
			status = aml_nftl_blk->search_cache_list(aml_nftl_blk, read_sect_addr, 0, blks_per_sect, buf + buf_offset);
			if (status) {
				error = aml_nftl_info->read_sect(aml_nftl_info, read_sect_addr, buf + buf_offset);
				if (error)
					return error;
			}
			read_sect_addr += 1;
			total_blk_count -= blks_per_sect;
			buf_offset += aml_nftl_info->writesize;
		}
	}

	if (total_blk_count > 0) {
		status = aml_nftl_blk->search_cache_list(aml_nftl_blk, read_sect_addr, 0, total_blk_count, buf + buf_offset);
		if (status) {
			if (read_sect_addr != aml_nftl_blk->cache_sect_addr) {
				error = aml_nftl_info->read_sect(aml_nftl_info, read_sect_addr, aml_nftl_blk->cache_buf);
				if (error)
					return error;
				aml_nftl_blk->cache_sect_addr = read_sect_addr;
			}
			memcpy(buf + buf_offset, aml_nftl_blk->cache_buf, total_blk_count * 512);
		}
	}

	return 0;
}

static int aml_nftl_write_data(struct aml_nftl_blk_t *aml_nftl_blk, unsigned long block, unsigned nblk, unsigned char *buf)
{
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_blk->aml_nftl_info;
	unsigned blks_per_sect, write_sect_addr, write_misalign_num, buf_offset = 0;
	int error = 0, status = 0, total_blk_count = 0, write_sect_num, i;

	ktime_get_ts(&aml_nftl_blk->ts_write_start);
	if (aml_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_READY_DONE)
		aml_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_DONE;
	blks_per_sect = aml_nftl_info->writesize / 512;
	write_sect_addr = block / blks_per_sect;
	total_blk_count = nblk;
	write_misalign_num = ((blks_per_sect - block % blks_per_sect) > nblk ? nblk : (blks_per_sect - block % blks_per_sect));
	//aml_nftl_dbg("nftl write data blk: %d page: %d block: %ld count %d\n", (write_sect_addr / aml_nftl_info->pages_per_blk), (write_sect_addr % aml_nftl_info->pages_per_blk), block, nblk);

	if (block % blks_per_sect) {
		status = aml_nftl_blk->add_cache_list(aml_nftl_blk, write_sect_addr, block % blks_per_sect, write_misalign_num, buf);
		if (status)
			return status;

		write_sect_addr++;
		total_blk_count -= write_misalign_num;
		buf_offset += write_misalign_num * 512;
	}

	if (total_blk_count >= blks_per_sect) {
		write_sect_num = total_blk_count / blks_per_sect;
		for (i=0; i<write_sect_num; i++) {
			status = aml_nftl_blk->add_cache_list(aml_nftl_blk, write_sect_addr, 0, blks_per_sect, buf + buf_offset);
			if ((status) && (status != CACHE_LIST_NOT_FOUND))
				return status;

			if (status == CACHE_LIST_NOT_FOUND) {
				if (aml_nftl_blk->cache_sect_addr == write_sect_addr)
					memcpy(aml_nftl_blk->cache_buf, buf + buf_offset, 512 * blks_per_sect);

				error = aml_nftl_info->write_sect(aml_nftl_info, write_sect_addr, buf + buf_offset);
				if (error)
					return error;
			}
			write_sect_addr += 1;
			total_blk_count -= blks_per_sect;
			buf_offset += aml_nftl_info->writesize;
		}
	}

	if (total_blk_count > 0) {
		status = aml_nftl_blk->add_cache_list(aml_nftl_blk, write_sect_addr, 0, total_blk_count, buf + buf_offset);
		if (status)
			return status;
	}

	return 0;
}

static int aml_nftl_flush(struct mtd_blktrans_dev *dev)
{
	int error = 0;
	struct mtd_info *mtd = dev->mtd;
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)dev;

	mutex_lock(&aml_nftl_lock);
	//aml_nftl_dbg("nftl flush all cache data: %d\n", aml_nftl_blk->cache_buf_cnt);
	if (aml_nftl_blk->cache_buf_cnt > 0)
		error = aml_nftl_blk->write_cache_data(aml_nftl_blk, CACHE_CLEAR_ALL);

	if (mtd->sync)
		mtd->sync(mtd);
	mutex_unlock(&aml_nftl_lock);

	return error;
}

static unsigned int aml_nftl_map_sg(struct aml_nftl_blk_t *aml_nftl_blk)
{
	unsigned int sg_len;
	size_t buflen;
	struct scatterlist *sg;
	int i;

	if (!aml_nftl_blk->bounce_buf)
		return blk_rq_map_sg(aml_nftl_blk->queue, aml_nftl_blk->req, aml_nftl_blk->sg);

	sg_len = blk_rq_map_sg(aml_nftl_blk->queue, aml_nftl_blk->req, aml_nftl_blk->bounce_sg);

	aml_nftl_blk->bounce_sg_len = sg_len;

	buflen = 0;
	for_each_sg(aml_nftl_blk->bounce_sg, sg, sg_len, i)
		buflen += sg->length;

	sg_init_one(aml_nftl_blk->sg, aml_nftl_blk->bounce_buf, buflen);

	return sg_len;
}

static int aml_nftl_calculate_sg(struct aml_nftl_blk_t *aml_nftl_blk, size_t buflen, unsigned **buf_addr, unsigned *offset_addr)
{
	struct scatterlist *sgl;
	unsigned int offset = 0, segments = 0, buf_start = 0;
	struct sg_mapping_iter miter;
	unsigned long flags;
	unsigned int nents;
	unsigned int sg_flags = SG_MITER_ATOMIC;

	nents = aml_nftl_blk->bounce_sg_len;
	sgl = aml_nftl_blk->bounce_sg;

	if (rq_data_dir(aml_nftl_blk->req) == WRITE)
		sg_flags |= SG_MITER_FROM_SG;
	else
		sg_flags |= SG_MITER_TO_SG;

	sg_miter_start(&miter, sgl, nents, sg_flags);

	local_irq_save(flags);

	while (offset < buflen) {
		unsigned int len;
		if(!sg_miter_next(&miter))
			break;

		if (!buf_start) {
			segments = 0;
			*(buf_addr + segments) = (unsigned *)miter.addr;
			*(offset_addr + segments) = offset;
			buf_start = 1;
		}
		else {
			if ((unsigned char *)(*(buf_addr + segments)) + (offset - *(offset_addr + segments)) != miter.addr) {
				segments++;
				*(buf_addr + segments) = (unsigned *)miter.addr;
				*(offset_addr + segments) = offset;
			}
		}

		len = min(miter.length, buflen - offset);
		offset += len;
	}
	*(offset_addr + segments + 1) = offset;

	sg_miter_stop(&miter);

	local_irq_restore(flags);

	return segments;
}

/*
 * Alloc bounce buf for read/write numbers of pages in one request
 */
int aml_nftl_init_bounce_buf(struct mtd_blktrans_dev *dev, struct request_queue *rq)
{
	int ret=0, i;
	unsigned int bouncesz;
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)dev;

	if(aml_nftl_blk->queue && aml_nftl_blk->sg && aml_nftl_blk->bounce_sg)
	{
	    aml_nftl_dbg("_nftl_init_bounce_buf already init\n");
	    return 0;
        }
	aml_nftl_blk->queue = rq;
	bouncesz = (aml_nftl_blk->aml_nftl_info->writesize * NFTL_CACHE_FORCE_WRITE_LEN);
	if(bouncesz < AML_NFTL_BOUNCE_SIZE)
		bouncesz = AML_NFTL_BOUNCE_SIZE;

	if (bouncesz >= PAGE_CACHE_SIZE) {
		aml_nftl_blk->bounce_buf = aml_nftl_malloc(bouncesz);
		if (!aml_nftl_blk->bounce_buf) {
			aml_nftl_dbg("not enough memory for bounce buf\n");
			return -1;
		}
	}

	for (i=0; i<NFTL_CACHE_FORCE_WRITE_LEN; i++)
		aml_nftl_blk->bounce_buf_free[i] = NFTL_BOUNCE_FREE;

    spin_lock_irq(rq->queue_lock);
	queue_flag_test_and_set(QUEUE_FLAG_NONROT, rq);
	blk_queue_bounce_limit(aml_nftl_blk->queue, BLK_BOUNCE_HIGH);
	blk_queue_max_hw_sectors(aml_nftl_blk->queue, bouncesz / 512);
	blk_queue_physical_block_size(aml_nftl_blk->queue, bouncesz);
	blk_queue_max_segments(aml_nftl_blk->queue, bouncesz / PAGE_CACHE_SIZE);
	blk_queue_max_segment_size(aml_nftl_blk->queue, bouncesz);
    spin_unlock_irq(rq->queue_lock);

	aml_nftl_blk->req = NULL;

	aml_nftl_blk->sg = aml_nftl_malloc(sizeof(struct scatterlist));
	if (!aml_nftl_blk->sg) {
		ret = -ENOMEM;
		blk_cleanup_queue(aml_nftl_blk->queue);
		return ret;
	}
	sg_init_table(aml_nftl_blk->sg, 1);

	aml_nftl_blk->bounce_sg = aml_nftl_malloc(sizeof(struct scatterlist) * bouncesz / PAGE_CACHE_SIZE);
	if (!aml_nftl_blk->bounce_sg) {
		ret = -ENOMEM;
		kfree(aml_nftl_blk->sg);
		aml_nftl_blk->sg = NULL;
		blk_cleanup_queue(aml_nftl_blk->queue);
		return ret;
	}
	sg_init_table(aml_nftl_blk->bounce_sg, bouncesz / PAGE_CACHE_SIZE);

	return 0;
}
#if 0
static void aml_nftl_search_free_list(struct aml_nftl_blk_t *aml_nftl_blk, uint32_t sect_addr,
										uint32_t blk_pos, uint32_t blk_num)
{
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_blk->aml_nftl_info;
	struct free_sects_list *free_list;
	int i;
	uint32_t blks_per_sect;
	struct list_head *l, *n;

	blks_per_sect = aml_nftl_info->writesize / 512;
	list_for_each_safe(l, n, &aml_nftl_blk->free_list) {
		free_list = free_list_to_node(l);

		if (free_list->vt_sect_addr == sect_addr) {
			for (i=blk_pos; i<(blk_pos+blk_num); i++) {
				if (free_list->free_blk_status[i])
					free_list->free_blk_status[i] = 0;
			}

			for (i=0; i<blks_per_sect; i++) {
				if (free_list->free_blk_status[i])
					break;;
			}
			if (i >= blks_per_sect) {
				list_del(&free_list->list);
				aml_nftl_free(free_list);
				return;
			}
			return;
		}
	}

	return;
}

static int aml_nftl_add_free_list(struct aml_nftl_blk_t *aml_nftl_blk, uint32_t sect_addr,
										uint32_t blk_pos, uint32_t blk_num)
{
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_blk->aml_nftl_info;
	struct free_sects_list *free_list;
	int i;
	uint32_t blks_per_sect;
	struct list_head *l, *n;

	blks_per_sect = aml_nftl_info->writesize / 512;
	list_for_each_safe(l, n, &aml_nftl_blk->free_list) {
		free_list = free_list_to_node(l);

		if (free_list->vt_sect_addr == sect_addr) {
			for (i=blk_pos; i<(blk_pos+blk_num); i++)
				free_list->free_blk_status[i] = 1;

			for (i=0; i<blks_per_sect; i++) {
				if (free_list->free_blk_status[i] == 0)
					break;;
			}
			if (i >= blks_per_sect) {
				list_del(&free_list->list);
				aml_nftl_free(free_list);
				return 0;
			}

			return 1;
		}
	}

	if ((blk_pos == 0) && (blk_num == blks_per_sect))
		return 0;

	free_list = aml_nftl_malloc(sizeof(struct free_sects_list));
	if (!free_list)
		return -ENOMEM;

	free_list->vt_sect_addr = sect_addr;
	for (i=blk_pos; i<(blk_pos+blk_num); i++)
		free_list->free_blk_status[i] = 1;
	list_add_tail(&free_list->list, &aml_nftl_blk->free_list);

	return 1;
}
#endif
#if 0
static void aml_nftl_update_blktrans_sysinfo(struct mtd_blktrans_dev *dev, unsigned int cmd, unsigned long arg)
{
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)dev;
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_blk->aml_nftl_info;
	struct fat_sectors *aml_nftl_sects = (struct fat_sectors *)arg;
	unsigned sect_addr, blks_per_sect, nblk, blk_pos, blk_num;

	mutex_lock(&aml_nftl_lock);

	nblk = aml_nftl_sects->sectors;
	blks_per_sect = aml_nftl_info->writesize / 512;
	sect_addr = (uint32_t)aml_nftl_sects->start / blks_per_sect;
	blk_pos = (uint32_t)aml_nftl_sects->start % blks_per_sect;
	while (nblk > 0){
		blk_pos = blk_pos % blks_per_sect;
		blk_num = ((blks_per_sect - blk_pos) > nblk ? nblk : (blks_per_sect - blk_pos ));
		if (cmd == BLKGETSECTS) {
			aml_nftl_search_free_list(aml_nftl_blk, sect_addr, blk_pos, blk_num);
		}
		else if (cmd == BLKFREESECTS) {
			if (!aml_nftl_add_free_list(aml_nftl_blk, sect_addr, blk_pos, blk_num))
				aml_nftl_info->delete_sect(aml_nftl_info, sect_addr);
		}
		else {
			break;
		}
		nblk -= blk_num;
		blk_pos += blk_num;
		sect_addr++;
	};

	mutex_unlock(&aml_nftl_lock);

	return;
}
#endif
static int do_nftltrans_request(struct mtd_blktrans_ops *tr,
			       struct mtd_blktrans_dev *dev,
			       struct request *req)
{
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)dev;
	int ret = 0, segments, i;
	unsigned long block, nblk, blk_addr, blk_cnt;
        if(!aml_nftl_blk->queue || !aml_nftl_blk->sg || !aml_nftl_blk->bounce_sg)
        {
            if (aml_nftl_init_bounce_buf(&aml_nftl_blk->mbd, aml_nftl_blk->mbd.rq))
            {
                aml_nftl_dbg("_nftl_init_bounce_buf  failed\n");
            }
        }

	unsigned short max_segm = queue_max_segments(aml_nftl_blk->queue);
	unsigned *buf_addr[max_segm+1];
	unsigned offset_addr[max_segm+1];
	size_t buflen;
	char *buf;

	memset((unsigned char *)buf_addr, 0, (max_segm+1)*4);
	memset((unsigned char *)offset_addr, 0, (max_segm+1)*4);
	aml_nftl_blk->req = req;
	block = blk_rq_pos(req) << 9 >> tr->blkshift;
	nblk = blk_rq_sectors(req);
	buflen = (nblk << tr->blkshift);

	if (!blk_fs_request(req))
		return -EIO;

	if (blk_rq_pos(req) + blk_rq_cur_sectors(req) >
	    get_capacity(req->rq_disk))
		return -EIO;

	if (blk_discard_rq(req))
		return tr->discard(dev, block, nblk);

	aml_nftl_blk->bounce_sg_len = aml_nftl_map_sg(aml_nftl_blk);
	segments = aml_nftl_calculate_sg(aml_nftl_blk, buflen, buf_addr, offset_addr);
	if (offset_addr[segments+1] != (nblk << tr->blkshift))
		return -EIO;

	mutex_lock(&aml_nftl_lock);
	switch(rq_data_dir(req)) {
		case READ:
			for(i=0; i<(segments+1); i++) {
				blk_addr = (block + (offset_addr[i] >> tr->blkshift));
				blk_cnt = ((offset_addr[i+1] - offset_addr[i]) >> tr->blkshift);
				buf = (char *)buf_addr[i];
				if (aml_nftl_blk->read_data(aml_nftl_blk, blk_addr, blk_cnt, buf)) {
					ret = -EIO;
					break;
				}
			}
			bio_flush_dcache_pages(aml_nftl_blk->req->bio);
			break;

		case WRITE:
			bio_flush_dcache_pages(aml_nftl_blk->req->bio);
			for(i=0; i<(segments+1); i++) {
				blk_addr = (block + (offset_addr[i] >> tr->blkshift));
				blk_cnt = ((offset_addr[i+1] - offset_addr[i]) >> tr->blkshift);
				buf = (char *)buf_addr[i];
				if (aml_nftl_blk->write_data(aml_nftl_blk, blk_addr, blk_cnt, buf)) {
					ret = -EIO;
					break;
				}
			}
			break;

		default:
			aml_nftl_dbg(KERN_NOTICE "Unknown request %u\n", rq_data_dir(req));
			break;
	}

	mutex_unlock(&aml_nftl_lock);

	return ret;
}

static int aml_nftl_writesect(struct mtd_blktrans_dev *dev, unsigned long block, char *buf)
{
	return 0;
}

static int aml_nftl_thread(void *arg)
{
	struct aml_nftl_blk_t *aml_nftl_blk = arg;
	unsigned long period = NFTL_MAX_SCHEDULE_TIMEOUT / 10;
	struct timespec ts_nftl_current;

	while (!kthread_should_stop()) {
		struct aml_nftl_info_t *aml_nftl_info;
		struct aml_nftl_wl_t *aml_nftl_wl;

		mutex_lock(&aml_nftl_lock);
		aml_nftl_info = aml_nftl_blk->aml_nftl_info;
		aml_nftl_wl = aml_nftl_info->aml_nftl_wl;

		if ((aml_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_READY)
			|| (aml_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_READY_DONE)
			|| (aml_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_DONE)) {

			aml_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_READY_DONE;
			mutex_unlock(&aml_nftl_lock);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(period);

			mutex_lock(&aml_nftl_lock);
			if ((aml_nftl_blk->cache_buf_status == NFTL_CACHE_STATUS_READY_DONE)
				&& (aml_nftl_blk->cache_buf_cnt > 0)) {

				ktime_get_ts(&ts_nftl_current);
				if ((ts_nftl_current.tv_sec - aml_nftl_blk->ts_write_start.tv_sec) >= NFTL_FLUSH_DATA_TIME) {
					aml_nftl_dbg("nftl writed cache data: %d %d %d\n", aml_nftl_blk->cache_buf_cnt, aml_nftl_wl->used_root.count, aml_nftl_wl->free_root.count);
					if (aml_nftl_blk->cache_buf_cnt >= (NFTL_CACHE_FORCE_WRITE_LEN / 2))
						aml_nftl_blk->write_cache_data(aml_nftl_blk, 0);
					else
						aml_nftl_blk->write_cache_data(aml_nftl_blk, CACHE_CLEAR_ALL);
				}

				if (aml_nftl_blk->cache_buf_cnt > 0) {
					mutex_unlock(&aml_nftl_lock);
					continue;
				}
				else {
					if ((aml_nftl_wl->free_root.count < (aml_nftl_info->fillfactor / 2)) && (!aml_nftl_wl->erased_root.count))
						aml_nftl_wl->gc_need_flag = 1;
					aml_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_IDLE;
				}
			}
			else {
				aml_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_READY_DONE;
				mutex_unlock(&aml_nftl_lock);
				continue;
			}
		}

		if (!aml_nftl_info->isinitialised) {

			mutex_unlock(&aml_nftl_lock);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(10);

			mutex_lock(&aml_nftl_lock);
			aml_nftl_info->creat_structure(aml_nftl_info);
			mutex_unlock(&aml_nftl_lock);
			continue;
		}

		if (!aml_nftl_wl->gc_need_flag) {
			mutex_unlock(&aml_nftl_lock);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
			continue;
		}

		aml_nftl_wl->garbage_collect(aml_nftl_wl, DO_COPY_PAGE);
		if (aml_nftl_wl->free_root.count >= (aml_nftl_info->fillfactor / 2)) {
			aml_nftl_dbg("nftl initilize garbage completely: %d \n", aml_nftl_wl->free_root.count);
			aml_nftl_wl->gc_need_flag = 0;
		}
		mutex_unlock(&aml_nftl_lock);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(period);
	}

	printk("%s:exit\n",__func__);
	aml_nftl_blk->nftl_thread=NULL;
	return 0;
}

static int aml_nftl_reboot_notifier(struct notifier_block *nb, unsigned long priority, void * arg)
 {
    int error = 0;
    struct aml_nftl_blk_t *aml_nftl_blk = nftl_notifier_to_blk(nb);

    aml_nftl_dbg("%s, %d nftl flush all cache data: %d\n", __func__, __LINE__,
            aml_nftl_blk->cache_buf_cnt);

    mutex_lock(&aml_nftl_lock);
    //aml_nftl_dbg("nftl reboot flush cache data: %d\n", aml_nftl_blk->cache_buf_cnt);
    if (aml_nftl_blk->cache_buf_cnt > 0)
        error = aml_nftl_blk->write_cache_data(aml_nftl_blk, CACHE_CLEAR_ALL);
    mutex_unlock(&aml_nftl_lock);

    if(aml_nftl_blk->nftl_thread!=NULL){
        kthread_stop(aml_nftl_blk->nftl_thread);
        aml_nftl_blk->nftl_thread=NULL;
    }
    aml_nftl_dbg("%s, %d nftl flush all cache data: %d\n", __func__, __LINE__,
            aml_nftl_blk->cache_buf_cnt);

    return error;

}

static void aml_nftl_add_mtd(struct mtd_blktrans_ops *tr, struct mtd_info *mtd)
{
	struct aml_nftl_blk_t *aml_nftl_blk;

	if (mtd->type != MTD_NANDFLASH)
		return;

	if (memcmp(mtd->name, "NFTL_Part", 9))
		return;

	aml_nftl_blk = aml_nftl_malloc(sizeof(struct aml_nftl_blk_t));
	if (!aml_nftl_blk)
		return;

	aml_nftl_blk->cache_sect_addr = -1;
	aml_nftl_blk->cache_buf_status = NFTL_CACHE_STATUS_IDLE;
	aml_nftl_blk->cache_buf_cnt = 0;
	aml_nftl_blk->mbd.mtd = mtd;
	aml_nftl_blk->mbd.devnum = mtd->index;
	aml_nftl_blk->mbd.tr = tr;
	aml_nftl_blk->nb.notifier_call = aml_nftl_reboot_notifier;

	register_reboot_notifier(&aml_nftl_blk->nb);
	INIT_LIST_HEAD(&aml_nftl_blk->cache_list);
	INIT_LIST_HEAD(&aml_nftl_blk->free_list);
	aml_nftl_blk->read_data = aml_nftl_read_data;
	aml_nftl_blk->write_data = aml_nftl_write_data;
	aml_nftl_blk->write_cache_data = aml_nftl_write_cache_data;
	aml_nftl_blk->search_cache_list = aml_nftl_search_cache_list;
	aml_nftl_blk->add_cache_list = aml_nftl_add_cache_list;
	aml_nftl_blk->cache_buf = aml_nftl_malloc(mtd->writesize);
	if (!aml_nftl_blk->cache_buf)
		return;

	if (aml_nftl_initialize(aml_nftl_blk))
		return;

	aml_nftl_blk->nftl_thread = kthread_run(aml_nftl_thread, aml_nftl_blk, "%sd", "aml_nftl");
	if (IS_ERR(aml_nftl_blk->nftl_thread))
		return;

	if (!(mtd->flags & MTD_WRITEABLE))
		aml_nftl_blk->mbd.readonly = 0;

	if (add_mtd_blktrans_dev(&aml_nftl_blk->mbd))
		aml_nftl_dbg("nftl add blk disk dev failed\n");

    if (aml_nftl_init_bounce_buf(&aml_nftl_blk->mbd, aml_nftl_blk->mbd.rq)){
		aml_nftl_dbg("aml_nftl_init_bounce_buf  failed\n");
		return;
    }

    aml_nftl_dbg("aml_nftl_add_mtd ok\n");
    return;
}

static int aml_nftl_open(struct mtd_blktrans_dev *mbd)
{
	return 0;
}

static int aml_nftl_release(struct mtd_blktrans_dev *mbd)
{
	int error = 0;
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)mbd;

	mutex_lock(&aml_nftl_lock);
	aml_nftl_dbg("nftl release flush cache data: %d\n", aml_nftl_blk->cache_buf_cnt);
	if (aml_nftl_blk->cache_buf_cnt > 0)
		error = aml_nftl_blk->write_cache_data(aml_nftl_blk, CACHE_CLEAR_ALL);
	mutex_unlock(&aml_nftl_lock);

	return error;
}


static void aml_nftl_blk_release(struct aml_nftl_blk_t *aml_nftl_blk)
{
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_blk->aml_nftl_info;
	aml_nftl_info_release(aml_nftl_info);
	aml_nftl_free(aml_nftl_info);
	if (aml_nftl_blk->cache_buf)
		aml_nftl_free(aml_nftl_blk->cache_buf);
	if (aml_nftl_blk->bounce_buf)
		aml_nftl_free(aml_nftl_blk->bounce_buf);
}

static void aml_nftl_remove_dev(struct mtd_blktrans_dev *dev)
{
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)dev;

	unregister_reboot_notifier(&aml_nftl_blk->nb);
	del_mtd_blktrans_dev(dev);
	aml_nftl_free(dev);
	aml_nftl_blk_release(aml_nftl_blk);
	aml_nftl_free(aml_nftl_blk);
}

static struct mtd_blktrans_ops aml_nftl_tr = {
	.name		= "avnftl",
	.major		= AML_NFTL_MAJOR,
	.part_bits	= 2,
	.blksize 	= 512,
	.open		= aml_nftl_open,
	.release	= aml_nftl_release,
#if 0 //removed for cost too much time while count free sectors at the time of mounting FS.
	.update_blktrans_sysinfo = aml_nftl_update_blktrans_sysinfo,
#endif
	.do_blktrans_request = do_nftltrans_request,
	.writesect	= aml_nftl_writesect,
	.flush		= aml_nftl_flush,
	.add_mtd	= aml_nftl_add_mtd,
	.remove_dev	= aml_nftl_remove_dev,
	.owner		= THIS_MODULE,
};

static int __init init_aml_nftl(void)
{
	mutex_init(&aml_nftl_lock);
	return register_mtd_blktrans(&aml_nftl_tr);
}

static void __exit cleanup_aml_nftl(void)
{
	deregister_mtd_blktrans(&aml_nftl_tr);
}

#if defined(CONFIG_DEFERRED_MODULE_INIT) && defined(CONFIG_AML_NFTL)
deferred_module_init(init_aml_nftl);
#else
module_init(init_aml_nftl);
#endif
module_exit(cleanup_aml_nftl);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("AML xiaojun_yoyo and rongrong_zhou");
MODULE_DESCRIPTION("aml nftl block interface");

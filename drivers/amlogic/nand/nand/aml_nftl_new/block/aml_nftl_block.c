/*
 * Aml nftl block device access
 *
 * (C) 2012 8
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
#include <linux/mtd/blktrans.h>
#include <linux/scatterlist.h>
#include <linux/blk_types.h>

#include "aml_nftl_block.h"

extern int aml_nftl_initialize(struct aml_nftl_blk_t *aml_nftl_blk,int no);
extern uint32 nand_flush_write_cache(struct aml_nftl_blk_t* aml_nftl_blk);
extern void aml_nftl_part_release(struct aml_nftl_part_t* part);
extern uint32 do_prio_gc(struct aml_nftl_part_t* part);
extern uint32 garbage_collect(struct aml_nftl_part_t* part);
extern uint32 do_static_wear_leveling(struct aml_nftl_part_t* part);
extern void *aml_nftl_malloc(uint32 size);
extern void aml_nftl_free(const void *ptr);
//extern int aml_nftl_dbg(const char * fmt,args...);
uint32 _nand_read(struct aml_nftl_blk_t *aml_nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf);
uint32 _nand_write(struct aml_nftl_blk_t *aml_nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf);
extern uint32 __nand_read(struct aml_nftl_part_t* part,uint32 start_sector,uint32 len,unsigned char *buf);
extern uint32 __nand_write(struct aml_nftl_part_t* part,uint32 start_sector,uint32 len,unsigned char *buf);
extern int aml_nftl_set_status(struct aml_nftl_part_t *part,unsigned char status);
static int nftl_num;

#if 0
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int aml_nftl_flush(struct mtd_blktrans_dev *dev)
{
	int error = 0;
	struct mtd_info *mtd = dev->mtd;
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)dev;


	mutex_lock(aml_nftl_blk->aml_nftl_lock);

	//aml_nftl_dbg("aml_nftl_flush: %d:%s\n", aml_nftl_part->cache.cache_write_nums,dev->mtd->name);
	error = aml_nftl_blk->flush_write_cache(aml_nftl_blk);

	if (mtd->_sync)
		mtd->_sync(mtd);

	mutex_unlock(aml_nftl_blk->aml_nftl_lock);

	return error;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int aml_nftl_calculate_sg(struct aml_nftl_blk_t *aml_nftl_blk, size_t buflen, unsigned **buf_addr, unsigned *offset_addr, struct request *req)
{
	struct scatterlist *sgl;
	unsigned int offset = 0, segments = 0, buf_start = 0;
	struct sg_mapping_iter miter;
	unsigned long flags;
	unsigned int nents;
	unsigned int sg_flags = SG_MITER_ATOMIC;

	nents = aml_nftl_blk->bounce_sg_len;
	sgl = aml_nftl_blk->bounce_sg;

	if (rq_data_dir(req) == WRITE)
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
#endif
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :Alloc bounce buf for read/write numbers of pages in one request
*****************************************************************************/
int aml_nftl_init_bounce_buf(struct mtd_blktrans_dev *dev, struct request_queue *rq)
{
	int ret=0;
	unsigned int bouncesz, buf_cnt = 0;
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)dev;

	if(aml_nftl_blk->queue && aml_nftl_blk->bounce_sg)
	{
	    aml_nftl_dbg("_nftl_init_bounce_buf already init %lx\n",PAGE_CACHE_SIZE);
	    return 0;
	}
	aml_nftl_blk->queue = rq;

	//bouncesz = (aml_nftl_blk->aml_nftl_part->nand_chip->bytes_per_page * NFTL_CACHE_FORCE_WRITE_LEN);
    if(NFTL_DONT_CACHE_DATA){
        aml_nftl_dbg("%s, no not use cache\n",__func__);
        buf_cnt = 1;
    }
    else{
        aml_nftl_dbg("%s, use cache here\n",__func__);
        buf_cnt = NFTL_CACHE_FORCE_WRITE_LEN;
    }

	bouncesz = (aml_nftl_blk->mbd.mtd->writesize * buf_cnt);
	if(bouncesz < AML_NFTL_BOUNCE_SIZE)
		bouncesz = AML_NFTL_BOUNCE_SIZE;

    spin_lock_irq(rq->queue_lock);
	queue_flag_test_and_set(QUEUE_FLAG_NONROT, rq);
	blk_queue_bounce_limit(aml_nftl_blk->queue, BLK_BOUNCE_HIGH);
	blk_queue_max_hw_sectors(aml_nftl_blk->queue, bouncesz / BYTES_PER_SECTOR);
	blk_queue_physical_block_size(aml_nftl_blk->queue, bouncesz);
	blk_queue_max_segments(aml_nftl_blk->queue, bouncesz / PAGE_CACHE_SIZE);
	blk_queue_max_segment_size(aml_nftl_blk->queue, bouncesz);
    spin_unlock_irq(rq->queue_lock);

	aml_nftl_blk->req = NULL;

	aml_nftl_blk->bounce_sg = aml_nftl_malloc(sizeof(struct scatterlist) * (bouncesz/PAGE_CACHE_SIZE));
	if (!aml_nftl_blk->bounce_sg) {
		ret = -ENOMEM;
		blk_cleanup_queue(aml_nftl_blk->queue);
		return ret;
	}

	sg_init_table(aml_nftl_blk->bounce_sg, bouncesz / PAGE_CACHE_SIZE);

	return 0;
}

uint32 write_sync_flag(struct aml_nftl_blk_t *aml_nftl_blk)
{
#if NFTL_CACHE_FLUSH_SYNC
//for USB burning tool using cache/preloader.. as media partition,
//so just disable sync flag for usb burning case.
	struct mtd_info *mtd = aml_nftl_blk->mbd.mtd;

	if(memcmp(mtd->name, "NFTL_Part", 9)==0)
		return 0;
	else
		return (aml_nftl_blk->req->cmd_flags & REQ_SYNC);
#else
	return 0;

#endif
}
#if 0
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int do_nftltrans_request(struct mtd_blktrans_ops *tr,struct mtd_blktrans_dev *dev,struct request *req)
{
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)dev;
	int ret = 0, segments, i;
	unsigned long block, nblk, blk_addr, blk_cnt;
	struct aml_nftl_part_t* aml_nftl_part = aml_nftl_blk->aml_nftl_part;

	if(!aml_nftl_blk->queue || !aml_nftl_blk->bounce_sg)
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

	//when notifer coming,nftl didnot respond to request.
	if(aml_nftl_blk->reboot_flag)
		return 0;

	memset((unsigned char *)buf_addr, 0, (max_segm+1)*4);
	memset((unsigned char *)offset_addr, 0, (max_segm+1)*4);
	block = blk_rq_pos(req) << SHIFT_PER_SECTOR >> tr->blkshift;
	nblk = blk_rq_sectors(req);
	buflen = (nblk << tr->blkshift);

	if (!blk_fs_request(req)){
	    aml_nftl_dbg("blk_fs_request == 0\n");
		return -EIO;
	}

	if (blk_rq_pos(req) + blk_rq_cur_sectors(req) > get_capacity(req->rq_disk)){
	    aml_nftl_dbg("over capacity\n");
		return -EIO;
	}

	if (blk_discard_rq(req))
		return tr->discard(dev, block, nblk);

	aml_nftl_blk->bounce_sg_len = blk_rq_map_sg(aml_nftl_blk->queue, req, aml_nftl_blk->bounce_sg);
	segments = aml_nftl_calculate_sg(aml_nftl_blk, buflen, buf_addr, offset_addr, req);
	if (offset_addr[segments+1] != (nblk << tr->blkshift))
		return -EIO;

//	aml_nftl_dbg("nftl segments: %d\n", segments+1);

	mutex_lock(aml_nftl_blk->aml_nftl_lock);
	aml_nftl_blk->req = req;
	switch(rq_data_dir(req)) {
		case READ:
			for(i=0; i<(segments+1); i++) {
				blk_addr = (block + (offset_addr[i] >> tr->blkshift));
				blk_cnt = ((offset_addr[i+1] - offset_addr[i]) >> tr->blkshift);
				buf = (char *)buf_addr[i];
//				aml_nftl_dbg("read blk_addr: %d blk_cnt: %d buf: %x\n", blk_addr,blk_cnt,buf);
				if (aml_nftl_blk->read_data(aml_nftl_blk, blk_addr, blk_cnt, buf)) {
					ret = -EIO;
					break;
				}
			}
			bio_flush_dcache_pages(aml_nftl_blk->req->bio);
			break;

		case WRITE:
			bio_flush_dcache_pages(aml_nftl_blk->req->bio);
			//aml_nftl_dbg("write blk_addr: %d blk_cnt: %d flags: 0x%x\n", block, nblk, req->cmd_flags);
			for(i=0; i<(segments+1); i++) {
				blk_addr = (block + (offset_addr[i] >> tr->blkshift));
				blk_cnt = ((offset_addr[i+1] - offset_addr[i]) >> tr->blkshift);
				buf = (char *)buf_addr[i];
//				aml_nftl_dbg("write blk_addr: %d blk_cnt: %d buf: %x\n", blk_addr,blk_cnt,buf);
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

	mutex_unlock(aml_nftl_blk->aml_nftl_lock);

	return ret;
}
#endif

uint32 _nand_write(struct aml_nftl_blk_t *aml_nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf)
{
    uint32 ret;
	mutex_lock(aml_nftl_blk->aml_nftl_lock);
    ret = __nand_write(aml_nftl_blk->aml_nftl_part,start_sector,len,buf);
    aml_nftl_blk->time = jiffies;
	mutex_unlock(aml_nftl_blk->aml_nftl_lock);
    return ret;
}
uint32 _nand_read(struct aml_nftl_blk_t *aml_nftl_blk,uint32 start_sector,uint32 len,unsigned char *buf)
{
	uint32 ret;
	mutex_lock(aml_nftl_blk->aml_nftl_lock);
	 ret = __nand_read(aml_nftl_blk->aml_nftl_part,start_sector,len,buf);
	mutex_unlock(aml_nftl_blk->aml_nftl_lock);
	 return ret;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int aml_nftl_writesect(struct mtd_blktrans_dev *dev, unsigned long block, char *buf)
{
	return _nand_write((void *)dev, block, 1,buf);
}

static int aml_nftl_readsect(struct mtd_blktrans_dev *dev, unsigned long block, char *buf)
{
	return _nand_read((void *)dev, block, 1,buf);
}
#if 0
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int flush_time_out(struct timespec* time_old,struct timespec* time_new,unsigned long time_out_s)
{
    if(time_new->tv_sec <= time_old->tv_sec)
    {
        return 0;
    }

    if(time_new->tv_sec - time_old->tv_sec >= time_out_s)
    {
        return 1;
    }

    return 0;
}
#endif
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int aml_nftl_thread(void *arg)
{
	struct aml_nftl_blk_t *aml_nftl_blk = arg;
    unsigned long time;

	while (!kthread_should_stop()) {
		mutex_lock(aml_nftl_blk->aml_nftl_lock);

		if(aml_nftl_get_part_write_cache_nums(aml_nftl_blk->aml_nftl_part) > 0){
            time = jiffies;
//			ktime_get_ts(&ts_nftl_current);
//			if (flush_time_out(&aml_nftl_blk->ts_write_start,&ts_nftl_current,NFTL_FLUSH_DATA_TIME) != 0){
			if (time_after(time,aml_nftl_blk->time+HZ)){
				//aml_nftl_dbg("aml_nftl_thread flush data: %d:%s\n", aml_nftl_part->cache.cache_write_nums,aml_nftl_blk->mbd.mtd->name);
//				aml_nftl_dbg("@%d",aml_nftl_part->cache.cache_write_nums);
				aml_nftl_blk->flush_write_cache(aml_nftl_blk);
			}
		}

#if  SUPPORT_WEAR_LEVELING
		if(do_static_wear_leveling(aml_nftl_blk->aml_nftl_part) != 0){
			PRINT("aml_nftl_thread do_static_wear_leveling error!\n");
		}
#endif

		if(garbage_collect(aml_nftl_blk->aml_nftl_part) != 0){
			PRINT("aml_nftl_thread garbage_collect error!\n");
		}

		if(do_prio_gc(aml_nftl_blk->aml_nftl_part) != 0){
			PRINT("aml_nftl_thread do_prio_gc error!\n");
		}

		mutex_unlock(aml_nftl_blk->aml_nftl_lock);

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(NFTL_SCHEDULE_TIMEOUT);
	}

	aml_nftl_blk->nftl_thread=NULL;
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int aml_nftl_reboot_notifier(struct notifier_block *nb, unsigned long priority, void * arg)
{
	int error = 0;
	struct aml_nftl_blk_t *aml_nftl_blk = nftl_notifier_to_blk(nb);


    if(aml_nftl_blk->nftl_thread!=NULL){
        kthread_stop(aml_nftl_blk->nftl_thread); //add stop thread to ensure nftl quit safely
        aml_nftl_blk->nftl_thread=NULL;
    }

	mutex_lock(aml_nftl_blk->aml_nftl_lock);

	if(aml_nftl_blk->reboot_flag == 0){
	    error = aml_nftl_blk->flush_write_cache(aml_nftl_blk);
	    error |= aml_nftl_blk->shutdown_op(aml_nftl_blk);
        aml_nftl_blk->reboot_flag = 1;
        aml_nftl_dbg("aml_nftl_reboot_notifier :%s %d\n",aml_nftl_blk->mbd.mtd->name,error);
        }

	mutex_unlock(aml_nftl_blk->aml_nftl_lock);

	return error;
}
static void aml_nftl_wipe_part(struct mtd_blktrans_dev *mbd)
{
	int error = 0;
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)mbd;
	struct aml_nftl_part_t* aml_nftl_part = aml_nftl_blk->aml_nftl_part;
	error = aml_nftl_reinit_part(aml_nftl_blk);
	if(error){
		PRINT("aml_nftl_reinit_part: failed\n");
	}
	return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void aml_nftl_add_mtd(struct mtd_blktrans_ops *tr, struct mtd_info *mtd)
{
	struct aml_nftl_blk_t *aml_nftl_blk;
	unsigned long part_size ;

	if (mtd->type != MTD_NANDFLASH)
		return;
/*
    if(mtd->size < 0x8000000)   // >128M
    {
        return;
    }*/
    if(mtd->writesize < 4096)
	 part_size = 0x1400000;	//20M
   else
	 part_size = 0x8000000;     //128	M

    if(mtd->size < part_size)
        return;

    PRINT("mtd->name: %s,mtd->size = 0x%llx\n",mtd->name,mtd->size);

	aml_nftl_blk = aml_nftl_malloc(sizeof(struct aml_nftl_blk_t));
	if (!aml_nftl_blk)
		return;

	aml_nftl_blk->aml_nftl_lock = aml_nftl_malloc(sizeof(struct mutex));
	if (!aml_nftl_blk->aml_nftl_lock)
		return;

    mutex_init(aml_nftl_blk->aml_nftl_lock);

	aml_nftl_blk->mbd.mtd = mtd;
	aml_nftl_blk->mbd.devnum = mtd->index;
	aml_nftl_blk->mbd.tr = tr;
	aml_nftl_blk->nb.notifier_call = aml_nftl_reboot_notifier;
    aml_nftl_blk->reboot_flag = 0;
    aml_nftl_blk->init_flag = 0;

	register_reboot_notifier(&aml_nftl_blk->nb);

	if (aml_nftl_initialize(aml_nftl_blk,nftl_num)){
	    aml_nftl_dbg("aml_nftl_initialize failed\n");
		return;
	}
    aml_nftl_blk->init_flag = 1;
	aml_nftl_blk->nftl_thread = kthread_run(aml_nftl_thread, aml_nftl_blk, "%sd", "aml_nftl");
	if (IS_ERR(aml_nftl_blk->nftl_thread))
		return;

	if (!(mtd->flags & MTD_WRITEABLE))
		aml_nftl_blk->mbd.readonly = 0;
#if 0
    if((memcmp(mtd->name, "system", 6)==0)){
        aml_nftl_blk->mbd.tr->name = "system" ;
	}else if ((memcmp(mtd->name, "cache", 5)==0)){
		aml_nftl_blk->mbd.tr->name = "cache" ;
	}else if((memcmp(mtd->name, "userdata", 8)==0)){
		aml_nftl_blk->mbd.tr->name = "data" ;
	}else if ((memcmp(mtd->name, "NFTL_Part", 9)==0)){
		aml_nftl_blk->mbd.tr->name = "media" ;
	}else if ((memcmp(mtd->name, "preloaded", 9)==0)){
		aml_nftl_blk->mbd.tr->name = "preloaded" ;
	}
#else
    if((memcmp(mtd->name, "userdata", 8)==0)){
		aml_nftl_blk->mbd.tr->name = "data" ;
	}else if ((memcmp(mtd->name, "NFTL_Part", 9)==0)){
		aml_nftl_blk->mbd.tr->name = "media" ;
	}
	else{
        aml_nftl_blk->mbd.tr->name = aml_nftl_malloc(strlen(mtd->name)+2);
        memset(aml_nftl_blk->mbd.tr->name, 0, strlen(mtd->name)+2);
        memcpy(aml_nftl_blk->mbd.tr->name,mtd->name,strlen(mtd->name)+1);
	}
#endif

	PRINT("aml_nftl_blk->mbd.tr.name =%s\n",aml_nftl_blk->mbd.tr->name );

	if (aml_add_mtd_blktrans_dev(&aml_nftl_blk->mbd)){
		aml_nftl_dbg("nftl add blk disk dev failed\n");
		return;
	}

	nftl_num++;
/*
    if (aml_nftl_init_bounce_buf(&aml_nftl_blk->mbd, aml_nftl_blk->mbd.rq)){
		aml_nftl_dbg("aml_nftl_init_bounce_buf  failed\n");
		return;
    }
*/
    aml_nftl_dbg("aml_nftl_add_mtd ok\n");
    return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int aml_nftl_getgeo(struct mtd_blktrans_dev *dev,  struct hd_geometry *geo)
{

	u_long sect;
	sect = 8;
	geo->heads = 1;
	geo->sectors = 8;
	geo->cylinders = sect >> 3;
	return 0;
}
#if 0
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int aml_nftl_release(struct mtd_blktrans_dev *mbd)
{
	int error = 0;
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)mbd;


	mutex_lock(aml_nftl_blk->aml_nftl_lock);

//	aml_nftl_dbg("aml_nftl_release flush cache data:%s\n",mbd->mtd->name);
	error = aml_nftl_blk->flush_write_cache(aml_nftl_blk);

	mutex_unlock(aml_nftl_blk->aml_nftl_lock);

	return error;
}
#endif
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void aml_nftl_blk_release(struct aml_nftl_blk_t *aml_nftl_blk)
{

	aml_nftl_part_release(aml_nftl_blk->aml_nftl_part);

	if (aml_nftl_blk->bounce_sg)
		aml_nftl_free(aml_nftl_blk->bounce_sg);

}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void aml_nftl_remove_dev(struct mtd_blktrans_dev *dev)
{
	struct aml_nftl_blk_t *aml_nftl_blk = (void *)dev;

	unregister_reboot_notifier(&aml_nftl_blk->nb);
	aml_del_mtd_blktrans_dev(dev);
	aml_nftl_free(dev);
	aml_nftl_blk_release(aml_nftl_blk);
	aml_nftl_free(aml_nftl_blk);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
#if 0
static struct mtd_blktrans_ops aml_nftl_tr = {
	.name		= "avnftl",
	.major		= AML_NFTL_MAJOR,
	.part_bits	= 2,
	.blksize 	= BYTES_PER_SECTOR,
	.open		= aml_nftl_open,
	.release	= aml_nftl_release,
	//.do_blktrans_request = do_nftltrans_request,
	.writesect	= aml_nftl_writesect,
	.flush		= aml_nftl_flush,
	.add_mtd	= aml_nftl_add_mtd,
	.remove_dev	= aml_nftl_remove_dev,
	.owner		= THIS_MODULE,
};
#else
static struct mtd_blktrans_ops aml_nftl_tr = {
	.name		= "avnftl",
	.major		= AML_NFTL_MAJOR,
	.part_bits	= 2,
	.blksize 	= BYTES_PER_SECTOR,
	.getgeo		= aml_nftl_getgeo,
	.readsect	= aml_nftl_readsect,
	.writesect	= aml_nftl_writesect,
	.add_mtd	= aml_nftl_add_mtd,
	.remove_dev	= aml_nftl_remove_dev,
	.wipe_part  = aml_nftl_wipe_part,
	.owner		= THIS_MODULE,
};

#endif
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int __init init_aml_nftl(void)
{
    int ret;

    PRINT("init_aml_nftl start\n");
    nftl_num = 0;
    //mutex_init(&aml_nftl_lock);
	ret = aml_register_mtd_blktrans(&aml_nftl_tr);
	PRINT("init_aml_nftl end\n");

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void __exit cleanup_aml_nftl(void)
{
	aml_deregister_mtd_blktrans(&aml_nftl_tr);
}




module_init(init_aml_nftl);
module_exit(cleanup_aml_nftl);

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("AML xiaojun_yoyo and rongrong_zhou");
MODULE_DESCRIPTION("aml nftl block interface");


#ifndef __AML_NFTL_BLOCK_H
#define __AML_NFTL_BLOCK_H

#include "aml_nftl_cfg.h"
#include <linux/device.h>
#include <linux/mtd/blktrans.h>


#define AML_NFTL_MAGIC			     "aml_nftl"
#define AML_USERDATA_MAGIC			 "aml_userdata"

#define AML_NFTL_MAJOR			 250
#define TIMESTAMP_LENGTH         15
#define MAX_TIMESTAMP_NUM        ((1<<(TIMESTAMP_LENGTH-1))-1)
#define AML_NFTL_BOUNCE_SIZE	 		0x40000

#define NFTL_SCHEDULE_TIMEOUT		             (HZ/2)
//#define NFTL_FLUSH_DATA_TIME			         1
#define NFTL_CACHE_FORCE_WRITE_LEN               16

#define RET_YES                           1
#define RET_NO                            0



#define  PRINT aml_nftl_dbg

#define nftl_notifier_to_blk(l)	container_of(l, struct aml_nftl_blk_t, nb)


#pragma pack(1)


extern int aml_class_register(struct class *class);
extern int aml_register_mtd_blktrans(struct mtd_blktrans_ops *tr);
extern int aml_deregister_mtd_blktrans(struct mtd_blktrans_ops *tr);
extern int aml_add_mtd_blktrans_dev(struct mtd_blktrans_dev *new);
extern int aml_del_mtd_blktrans_dev(struct mtd_blktrans_dev *old);

struct aml_nftl_blk_t;

struct aml_nftl_blk_t{
	struct mtd_blktrans_dev mbd;
	struct request*         req;
	struct request_queue*   queue;
//	struct scatterlist	    *sg;
//	char			        *bounce_buf;
//	int8_t			        bounce_buf_free[NFTL_CACHE_FORCE_WRITE_LEN];
	struct scatterlist*     bounce_sg;
	unsigned int		    bounce_sg_len;
	unsigned long		    time;
	unsigned long		    time_flush;
	unsigned int		    reboot_flag;
	struct task_struct*		nftl_thread;
	struct aml_nftl_part_t* aml_nftl_part;
	struct mutex*           aml_nftl_lock;
	struct aml_nftl_blk_t*  nftl_blk_next;

	struct notifier_block   nb;
//	struct timespec      	ts_write_start;
	spinlock_t 				thread_lock;

	struct class            debug;
    int                     init_flag;
	struct _nftl_cfg        nftl_cfg;

	uint32 (*read_data)(struct aml_nftl_blk_t *aml_nftl_blk, unsigned long block, unsigned nblk, unsigned char *buf);
	uint32 (*write_data)(struct aml_nftl_blk_t *aml_nftl_blk, unsigned long block, unsigned nblk, unsigned char *buf);
	uint32 (*flush_write_cache)(struct aml_nftl_blk_t *aml_nftl_blk);
	uint32 (*shutdown_op)(struct aml_nftl_blk_t *aml_nftl_blk);

};
extern int aml_nftl_reinit_part(struct aml_nftl_blk_t *aml_nftl_blk);

#pragma pack()

#endif

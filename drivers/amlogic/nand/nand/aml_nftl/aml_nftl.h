#ifndef __AML_NFTL_H
#define __AML_NFTL_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/rbtree.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/mtd/blktrans.h>
#include <linux/time.h>

#pragma pack(1)

typedef int16_t         addr_sect_t;		//-1 , free , 0~page , valid data
typedef int16_t         addr_blk_t;
typedef int16_t         erase_count_t;
typedef int32_t         addr_page_t;
typedef int16_t      	addr_vtblk_t;
typedef  int32_t        addr_logic_t;
typedef  int32_t        addr_physical_t;
typedef int16_t     	addr_linearblk_t;

#define aml_nftl_malloc(n)		kzalloc(n, GFP_KERNEL)
#define aml_nftl_free			kfree
#define aml_nftl_dbg			printk

/**
*/
//addr_sect_t nftl_get_sector(addr_sect_t addr,sect_map_t logic_map);

//#define NFTL_DONT_CACHE_DATA

#define AML_NFTL_MAGIC			 "aml_nftl"
#define AML_NFTL_MAJOR			 250
#define TIMESTAMP_LENGTH         15
#define MAX_TIMESTAMP_NUM        ((1<<(TIMESTAMP_LENGTH-1))-1)
#define MAX_PAGES_IN_BLOCK       256
#define MAX_BLKS_PER_SECTOR		 128
#define MAX_BLK_NUM_PER_NODE	 4
#define BASIC_BLK_NUM_PER_NODE	 2
#define DEFAULT_SPLIT_UNIT		 8
#define NFTL_FAT_TABLE_NUM		 8

#define DEFAULT_SPLIT_UNIT_GARBAGE		8
#define NFTL_BOUNCE_FREE		 		0
#define NFTL_BOUNCE_USED		 		1
#define NFTL_MAX_SCHEDULE_TIMEOUT		1000
#define NFTL_FLUSH_DATA_TIME			8
#define NFTL_CACHE_STATUS_IDLE			0
#define NFTL_CACHE_STATUS_READY			1
#define NFTL_CACHE_STATUS_READY_DONE	2
#define NFTL_CACHE_STATUS_DONE			3
#define NFTL_CACHE_FORCE_WRITE_LEN		16
#define CACHE_CLEAR_ALL					1
#define AML_NFTL_BOUNCE_SIZE	 		0x40000
#define AML_LIMIT_FACTOR		 4
#define DO_COPY_PAGE			 1
#define DO_COPY_PAGE_TOTAL		 2
#define DO_COPY_PAGE_AVERAGELY	 3
#define AVERAGELY_COPY_NUM		 2
#define READ_OPERATION			 0
#define WRITE_OPERATION			 1
#define CACHE_LIST_NOT_FOUND	 1

#define WL_DELTA		397

#define BLOCK_INIT_VALUE		0xffff

#define MAKE_EC_BLK(ec, blk) ((ec<<16)|blk)

#define SECTOR_GAP		0x1000
#define SECTOR_GAP_MASK	(SECTOR_GAP-1)
#define ROOT_SECT		(0*SECTOR_GAP)
#define LEAF_SECT		(1*SECTOR_GAP)
#define SROOT_SECT		(2*SECTOR_GAP)
#define SLEAF_SECT		(3*SECTOR_GAP)
#define SEND_SECT		(4*SECTOR_GAP)

#define NODE_ROOT		0x1
#define NODE_LEAF		0x2
#define DEFAULT_IDLE_FREE_BLK	12
#define STATUS_BAD_BLOCK	0
#define STATUS_GOOD_BLOCK	1

typedef enum {
	AML_NFTL_SUCCESS				=0,
	AML_NFTL_FAILURE				=1,
	AML_NFTL_INVALID_PARTITION		=2,
	AML_NFTL_INVALID_ADDRESS		=3,
	AML_NFTL_DELETED_SECTOR			=4,
	AML_NFTL_FLUSH_ERROR			=5,
	AML_NFTL_UNFORMATTED			=6,
	AML_NFTL_UNWRITTEN_SECTOR		=7,
	AML_NFTL_PAGENOTFOUND			=0x08,
	AML_NFTL_NO_FREE_BLOCKS			=0x10,
	AML_NFTL_STRUCTURE_FULL			=0x11,
	AML_NFTL_NO_INVALID_BLOCKS		=0x12,
	AML_NFTL_SECTORDELETED			=0x50,
	AML_NFTL_ABT_FAILURE 			=0x51,
	AML_NFTL_MOUNTED_PARTITION		=0,
	AML_NFTL_UNMOUNTED_PARTITION	=1,
	AML_NFTL_SPAREAREA_ERROR      	=0x13,
	AML_NFTL_STATIC_WL_FINISH      	=0x14,
	AML_NFTL_BLKNOTFOUND      		=0x15
}t_AML_NFTL_error;

//typedef struct vtblk_node_s 	vtblk_node_t;
//typedef struct phyblk_node_s 	phyblk_node_t;

///** storage structure ** /
//typedef struct aml_nftl_oob_info_s nftl_oobinfo_t;
struct nftl_oobinfo_t{
	addr_sect_t    sect;
    erase_count_t  ec;    //00, Bad
    addr_vtblk_t   vtblk; //-1 , free,-2~-10, Snapshot
    unsigned        timestamp: 15;
    unsigned       status_page: 1;
};

struct phyblk_node_t{
    erase_count_t  ec;
    int16_t       valid_sects;
    addr_vtblk_t   vtblk;
    addr_sect_t    last_write;//offset_validation
    int16_t		  status_page;
    int16_t       timestamp;
    addr_sect_t    phy_page_map[MAX_PAGES_IN_BLOCK];
    uint8_t		   phy_page_delete[MAX_PAGES_IN_BLOCK>>3];
};

struct vtblk_node_t {
	addr_blk_t	phy_blk_addr;
	struct vtblk_node_t *next;
};

/*struct vtblk_special_node_t {
	struct vtblk_node_t *vtblk_node;
	addr_blk_t	ext_phy_blk_addr;
};*/

struct write_cache_node {

	struct list_head list;
	unsigned char *buf;
	int8_t bounce_buf_num;
	uint32_t vt_sect_addr;
	unsigned char cache_fill_status[MAX_BLKS_PER_SECTOR];		//blk unit fill status every vitual sector
};

struct free_sects_list {

	struct list_head list;
	uint32_t vt_sect_addr;
	unsigned char free_blk_status[MAX_BLKS_PER_SECTOR];
};

struct wl_rb_t {
	struct rb_node	rb_node;
	uint16_t			blk;		/*linear block*/
	uint16_t			ec;		/*erase count*/
};

struct wl_list_t {
	struct list_head 	list;
	addr_blk_t 	vt_blk;		/*logical block*/
	addr_blk_t 	phy_blk;
};

struct gc_blk_list {
	struct list_head list;
	addr_blk_t	gc_blk_addr;
};

/**
* tree root & node count
*/
struct wl_tree_t {
	struct rb_root  root;	/*tree root*/
	uint16_t		count;	/*number of nodes in this tree*/
};

struct aml_nftl_blk_t;
struct aml_nftl_ops_t;
struct aml_nftl_info_t;
struct aml_nftl_wl_t;

struct aml_nftl_ops_t{
    int		(*read_page)(struct aml_nftl_info_t * aml_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr, unsigned char *data_buf, unsigned char *nftl_oob_buf, int oob_len);
    int		(*write_page)(struct aml_nftl_info_t * aml_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr, unsigned char *data_buf, unsigned char *nftl_oob_buf, int oob_len);
    int     (*read_page_oob)(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr, unsigned char * nftl_oob_buf, int oob_len);
    int    (* blk_isbad)(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr);
    int    (* blk_mark_bad)(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr);
    int    (* erase_block)(struct aml_nftl_info_t * aml_nftl_info, addr_blk_t blk_addr);
};


/**
 * weal leveling structure
 */
struct aml_nftl_wl_t {
	/*tree for erased free blocks, dirty free blocks, used blocks*/
	struct wl_tree_t		erased_root;
	struct wl_tree_t		free_root;
	struct wl_tree_t		used_root;
	/*list for dynamic wl(leaf blocks), read ecc error blocks*/
	struct wl_list_t		readerr_head;
	struct list_head 		gc_blk_list;

	/*static wl threshold*/
	uint32_t		wl_delta;
	uint32_t		cur_delta;
	uint8_t			gc_need_flag;
	addr_blk_t		gc_start_block;
	addr_blk_t	  	wait_gc_block;
	addr_sect_t		page_copy_per_gc;

	/*
	 * pages per physical block & total blocks, get from hardware layer,
	 * init in function nftl_wl_init
	 */
	uint16_t		pages_per_blk;
	struct aml_nftl_info_t *aml_nftl_info;

	void (*add_free)(struct aml_nftl_wl_t *wl, addr_blk_t blk);
	void (*add_erased)(struct aml_nftl_wl_t *wl, addr_blk_t blk);
	void (*add_used)(struct aml_nftl_wl_t *wl, addr_blk_t blk);
	void (*add_gc)(struct aml_nftl_wl_t *wl, addr_blk_t blk);
	int32_t (*get_best_free)(struct aml_nftl_wl_t *wl, addr_blk_t *blk);

	/*nftl node info adapter function*/
	int (*garbage_one)(struct aml_nftl_wl_t *aml_nftl_wl, addr_blk_t vt_blk, uint8_t gc_flag);
	//int (*gc_structure_full)(struct aml_nftl_wl_t *aml_nftl_wl, addr_page_t logic_page_addr, unsigned char *buf);
	int (*garbage_collect)(struct aml_nftl_wl_t *aml_nftl_wl, uint8_t gc_flag);
};

struct aml_nftl_blk_t{
	struct mtd_blktrans_dev mbd;

	struct request *req;
	struct request_queue *queue;
	struct scatterlist	*sg;
	struct notifier_block nb;
	struct timespec ts_write_start;

	char			*bounce_buf;
	int8_t			bounce_buf_free[NFTL_CACHE_FORCE_WRITE_LEN];
	struct scatterlist	*bounce_sg;
	unsigned int		bounce_sg_len;

    struct list_head cache_list;
    struct list_head free_list;
    struct mutex cache_mutex;

	int cache_sect_addr;
	unsigned char *cache_buf;
	uint8_t cache_buf_status;
	int8_t cache_buf_cnt;
	struct task_struct *nftl_thread;
	spinlock_t thread_lock;
	struct aml_nftl_info_t *aml_nftl_info;

	int (*read_data)(struct aml_nftl_blk_t *aml_nftl_blk, unsigned long block, unsigned nblk, unsigned char *buf);
	int (*write_data)(struct aml_nftl_blk_t *aml_nftl_blk, unsigned long block, unsigned nblk, unsigned char *buf);
	int (*write_cache_data)(struct aml_nftl_blk_t *aml_nftl_blk, uint8_t cache_flag);
	int (*search_cache_list)(struct aml_nftl_blk_t *aml_nftl_blk, uint32_t sect_addr, uint32_t blk_pos, uint32_t blk_num, unsigned char *buf);
	int (*add_cache_list)(struct aml_nftl_blk_t *aml_nftl_blk, uint32_t sect_addr, uint32_t blk_pos, uint32_t blk_num, unsigned char *buf);
};

struct aml_nftl_info_t{
	struct mtd_info *mtd;

	uint32_t	  writesize;
	uint32_t	  oobsize;
	uint32_t	  pages_per_blk;

    uint32_t	  startblock;
    uint32_t	  endBlock;
    uint32_t      accessibleblocks;
    uint8_t       isinitialised;
    uint16_t	  cur_split_blk;
    uint32_t      fillfactor;
    addr_blk_t	  current_write_block;
    addr_sect_t	  continue_writed_sects;

    unsigned char *copy_page_buf;
    void   		**vtpmt;
    struct phyblk_node_t  *phypmt;

    struct aml_nftl_ops_t  *aml_nftl_ops;
    struct aml_nftl_wl_t	*aml_nftl_wl;
    struct class      cls;

    int		(*read_page)(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr, unsigned char *data_buf, unsigned char *nftl_oob_buf, int oob_len);
    int		(*write_page)(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr, unsigned char *data_buf, unsigned char *nftl_oob_buf, int oob_len);
    int    (* copy_page)(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t dest_blk_addr, addr_page_t dest_page, addr_blk_t src_blk_addr, addr_page_t src_page);
    int     (*get_page_info)(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr, unsigned char * nftl_oob_buf, int oob_len);
    int    (* blk_isbad)(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr);
    int    (* blk_mark_bad)(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr);
    int    (* get_phy_sect_map)(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr);
    int    (* erase_block)(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr);

    int (* delete_sector)(struct aml_nftl_info_t *aml_nftl_info,addr_page_t page,uint32_t len);
	int (*read_sect)(struct aml_nftl_info_t *aml_nftl_info, addr_page_t sect_addr, unsigned char *buf);
	int (*write_sect)(struct aml_nftl_info_t *aml_nftl_info, addr_page_t sect_addr, unsigned char *buf);
	void (*delete_sect)(struct aml_nftl_info_t *aml_nftl_info, addr_page_t sect_addr);
	void (*creat_structure)(struct aml_nftl_info_t *aml_nftl_info);
};

#pragma pack()

static inline unsigned int aml_nftl_get_node_length(struct aml_nftl_info_t *aml_nftl_info, struct vtblk_node_t  *vt_blk_node)
{
	unsigned int node_length = 0;

	while (vt_blk_node != NULL) {
		node_length++;
		vt_blk_node = vt_blk_node->next;
	}
	return node_length;
}

extern void aml_nftl_ops_init(struct aml_nftl_info_t *aml_nftl_info);
extern int aml_nftl_initialize(struct aml_nftl_blk_t *aml_nftl_blk);
extern void aml_nftl_info_release(struct aml_nftl_info_t *aml_nftl_info);
extern int aml_nftl_wl_init(struct aml_nftl_info_t *aml_nftl_info);
extern int aml_nftl_check_node(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr);
extern int aml_nftl_add_node(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t logic_blk_addr, addr_blk_t phy_blk_addr);
extern int aml_nftl_badblock_handle(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t phy_blk_addr, addr_blk_t logic_blk_addr);
#endif




/*
 * Aml nftl core
 *
 * (C) 2010 10
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>
#include <linux/mutex.h>

#include "aml_nftl.h"

static int get_vt_node_info(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr)
{
	addr_blk_t vt_blk_num, phy_blk_num;
	int k, node_length, node_len_actual;
	struct vtblk_node_t  *vt_blk_node;
	struct phyblk_node_t *phy_blk_node;

	vt_blk_num = blk_addr;
	vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
	if (vt_blk_node == NULL){
		aml_nftl_dbg("%s, %d, blk_addr:%d, have no vt node !!!!\n", __func__, __LINE__, blk_addr);
		return -ENOMEM;
	}

	node_len_actual = aml_nftl_get_node_length(aml_nftl_info, vt_blk_node);
	int16_t valid_page[node_len_actual+2];
	aml_nftl_dbg("###########%s, %d, blk_addr:%d, node_len_actual:%d\n", __func__, __LINE__, blk_addr, node_len_actual);
	memset((unsigned char *)valid_page, 0x0, sizeof(int16_t)*(node_len_actual+2));

	for (k=0; k<aml_nftl_info->pages_per_blk; k++) {

		node_length = 0;
		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
		while (vt_blk_node != NULL) {

			phy_blk_num = vt_blk_node->phy_blk_addr;
			phy_blk_node = &aml_nftl_info->phypmt[phy_blk_num];
			if (!(phy_blk_node->phy_page_delete[k>>3] & (1 << (k % 8))))
				break;
			aml_nftl_info->get_phy_sect_map(aml_nftl_info, phy_blk_num);
			if ((phy_blk_node->phy_page_map[k] >= 0) && (phy_blk_node->phy_page_delete[k>>3] & (1 << (k % 8)))) {
				valid_page[node_length]++;
				aml_nftl_dbg("node_length:%d page:%d, at:phy_blk_num:%d\n", node_length, k, phy_blk_num);
				break;
			}
			node_length++;
			vt_blk_node = vt_blk_node->next;
		}
	}

	for(k=0; k<node_len_actual; k++){
		aml_nftl_dbg("valid_page[%d]: %d\n", k, valid_page[k]);
	}

	return 0;
}
int aml_nftl_badblock_handle(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t phy_blk_addr, addr_blk_t logic_blk_addr)
{
    struct aml_nftl_wl_t *aml_nftl_wl = aml_nftl_info->aml_nftl_wl;
    struct phyblk_node_t *phy_blk_node, *phy_blk_node_new;
    addr_blk_t phy_blk_addr_new, src_page, dest_page;
    int status, i, retry_cnt_map = 0, retry_cnt_copy = 0;

RETRY:
    if ((aml_nftl_wl->free_root.count < (aml_nftl_info->fillfactor / 2)) && (!aml_nftl_wl->erased_root.count) && (aml_nftl_wl->wait_gc_block < 0))
    {
	aml_nftl_wl->garbage_collect(aml_nftl_wl, 0);
	}

    status = aml_nftl_wl->get_best_free(aml_nftl_wl, &phy_blk_addr_new);
    if (status) {
        status = aml_nftl_wl->garbage_collect(aml_nftl_wl, DO_COPY_PAGE);
        if (status == 0) {
            aml_nftl_dbg("%s line:%d nftl couldn`t found free block: %d %d\n", __func__, __LINE__, aml_nftl_wl->free_root.count, aml_nftl_wl->wait_gc_block);
            return -ENOENT;
        }
        status = aml_nftl_wl->get_best_free(aml_nftl_wl, &phy_blk_addr_new);
        if (status)
        {
            aml_nftl_dbg("%s line:%d nftl couldn`t get best free block: %d %d\n", __func__, __LINE__, aml_nftl_wl->free_root.count, aml_nftl_wl->wait_gc_block);
            return -ENOENT;
        }
    }

    aml_nftl_add_node(aml_nftl_info, logic_blk_addr, phy_blk_addr_new);
    aml_nftl_wl->add_used(aml_nftl_wl, phy_blk_addr_new);
    phy_blk_node_new = &aml_nftl_info->phypmt[phy_blk_addr_new];
    status = aml_nftl_info->get_phy_sect_map(aml_nftl_info, phy_blk_addr_new);
    if(status)
    {
		aml_nftl_dbg("%s line:%d nftl couldn`t get block sectmap: block: %d, retry_cnt_map:%d\n", __func__, __LINE__, phy_blk_addr_new, retry_cnt_map);
		aml_nftl_info->blk_mark_bad(aml_nftl_info, phy_blk_addr_new);
		if(retry_cnt_map++ < 3)
			goto RETRY;
		else
			return -ENOENT;
    }

	phy_blk_node = &aml_nftl_info->phypmt[phy_blk_addr];
    for(i=0; i<aml_nftl_wl->pages_per_blk; i++)
    {
        src_page = phy_blk_node->phy_page_map[i];
        if (src_page < 0)
            continue;

        dest_page = phy_blk_node_new->last_write + 1;
        if(aml_nftl_info->copy_page(aml_nftl_info, phy_blk_addr_new, dest_page, phy_blk_addr, src_page) == 2)
        {
			aml_nftl_dbg("%s line:%d nftl copy block write failed block: %d, retry_cnt_copy:%d\n", __func__, __LINE__, phy_blk_addr_new, retry_cnt_copy);
			aml_nftl_info->blk_mark_bad(aml_nftl_info, phy_blk_addr_new);
			if(retry_cnt_copy++ < 3)
				goto RETRY;
			else
	return -ENOENT;
        }
    }
    aml_nftl_info->blk_mark_bad(aml_nftl_info, phy_blk_addr);

	return phy_blk_addr_new;
}

int aml_nftl_check_node(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr)
{
	struct aml_nftl_wl_t *aml_nftl_wl;
	struct phyblk_node_t *phy_blk_node;
	struct vtblk_node_t  *vt_blk_node, *vt_blk_node_free;
	addr_blk_t phy_blk_num, vt_blk_num;
	//int16_t valid_page[MAX_BLK_NUM_PER_NODE+2];
	int16_t	*valid_page;
	int k, node_length, node_len_actual, node_len_max, status = 0;

	aml_nftl_wl = aml_nftl_info->aml_nftl_wl;
	vt_blk_num = blk_addr;
	vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
	if (vt_blk_node == NULL)
		return -ENOMEM;

	if (vt_blk_num < NFTL_FAT_TABLE_NUM)
		node_len_max = (MAX_BLK_NUM_PER_NODE + 2);
	else
		node_len_max = (MAX_BLK_NUM_PER_NODE + 1);

	node_len_actual = aml_nftl_get_node_length(aml_nftl_info, vt_blk_node);
	node_len_actual = (node_len_actual > (aml_nftl_info->accessibleblocks-1)) ? (aml_nftl_info->accessibleblocks-1) : node_len_actual;

	if(node_len_actual >node_len_max) {
		valid_page = aml_nftl_malloc(sizeof(int16_t) * (node_len_actual + 2));

		if(valid_page == NULL){
			aml_nftl_dbg("have no mem for valid_page, at blk_addr:%d,  node_len_actual:%d!!!!\n", blk_addr, node_len_actual);
			return -ENOMEM;
		}
		memset((unsigned char *)valid_page, 0x0, sizeof(int16_t)*(node_len_actual+2));
	}
	else {
		valid_page = aml_nftl_malloc(sizeof(int16_t) * (node_len_max + 2));

		if(valid_page == NULL){
			aml_nftl_dbg("have no mem for valid_page, at blk_addr:%d,  node_len_actual:%d!!!!\n", blk_addr, node_len_max);
			return -ENOMEM;
		}
		memset((unsigned char *)valid_page, 0x0, sizeof(int16_t)*(node_len_max+2));
	}


#if 0  //removed unused printk
	if (node_len_actual > node_len_max) {
		aml_nftl_dbg("%s Line:%d blk_addr:%d  have node length over MAX, and node_len_actual:%d\n", __func__, __LINE__, blk_addr, node_len_actual);
#if 0
		node_length = 0;
		while (vt_blk_node != NULL) {
			node_length++;
			if ((vt_blk_node != NULL) && (node_length > node_len_max)) {
				vt_blk_node_free = vt_blk_node->next;
				if (vt_blk_node_free != NULL) {
					aml_nftl_wl->add_free(aml_nftl_wl, vt_blk_node_free->phy_blk_addr);
					vt_blk_node->next = vt_blk_node_free->next;
					aml_nftl_free(vt_blk_node_free);
					continue;
				}
			}
			vt_blk_node = vt_blk_node->next;
		}
#endif
	}
#endif


	for (k=0; k<aml_nftl_info->pages_per_blk; k++) {

		node_length = 0;
		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
		while (vt_blk_node != NULL) {

			phy_blk_num = vt_blk_node->phy_blk_addr;
			phy_blk_node = &aml_nftl_info->phypmt[phy_blk_num];
			if (!(phy_blk_node->phy_page_delete[k>>3] & (1 << (k % 8))))
				break;
			aml_nftl_info->get_phy_sect_map(aml_nftl_info, phy_blk_num);
			if ((phy_blk_node->phy_page_map[k] >= 0) && (phy_blk_node->phy_page_delete[k>>3] & (1 << (k % 8)))) {
				valid_page[node_length]++;
				break;
			}
			node_length++;
			vt_blk_node = vt_blk_node->next;
		}
	}

	//aml_nftl_dbg("NFTL check node logic blk: %d  %d %d %d %d %d\n", vt_blk_num, valid_page[0], valid_page[1], valid_page[2], valid_page[3], valid_page[4]);
	node_length = 0;
	vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
	while (vt_blk_node->next != NULL) {
		node_length++;
		if (valid_page[node_length] == 0) {
			vt_blk_node_free = vt_blk_node->next;
			aml_nftl_wl->add_free(aml_nftl_wl, vt_blk_node_free->phy_blk_addr);
			vt_blk_node->next = vt_blk_node_free->next;
			aml_nftl_free(vt_blk_node_free);
			continue;
		}

		if (vt_blk_node->next != NULL)
			vt_blk_node = vt_blk_node->next;
	}

	vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
	phy_blk_num = vt_blk_node->phy_blk_addr;
	phy_blk_node = &aml_nftl_info->phypmt[phy_blk_num];
	node_length = aml_nftl_get_node_length(aml_nftl_info, vt_blk_node);
	if ((node_length == node_len_max) && (valid_page[node_len_max-1] == (aml_nftl_info->pages_per_blk - (phy_blk_node->last_write + 1)))) {
		status = AML_NFTL_STRUCTURE_FULL;
	}

	aml_nftl_free(valid_page);

	return status;
}

static void aml_nftl_update_sectmap(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t phy_blk_addr, addr_page_t logic_page_addr, addr_page_t phy_page_addr)
{
	struct phyblk_node_t *phy_blk_node;
	phy_blk_node = &aml_nftl_info->phypmt[phy_blk_addr];

	phy_blk_node->valid_sects++;
	phy_blk_node->phy_page_map[logic_page_addr] = phy_page_addr;
	phy_blk_node->phy_page_delete[logic_page_addr>>3] |= (1 << (logic_page_addr % 8));
	phy_blk_node->last_write = phy_page_addr;
}

static int aml_nftl_write_page(struct aml_nftl_info_t * aml_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr,
								unsigned char *data_buf, unsigned char *nftl_oob_buf, int oob_len)
{
	int status;
	struct aml_nftl_ops_t *aml_nftl_ops = aml_nftl_info->aml_nftl_ops;
	struct nftl_oobinfo_t *nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;

	status = aml_nftl_ops->write_page(aml_nftl_info, blk_addr, page_addr, data_buf, nftl_oob_buf, oob_len);
	if (status)
		return status;

	aml_nftl_update_sectmap(aml_nftl_info, blk_addr, nftl_oob_info->sect, page_addr);
	return 0;
}

static int aml_nftl_read_page(struct aml_nftl_info_t * aml_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr,
								unsigned char *data_buf, unsigned char *nftl_oob_buf, int oob_len)
{
	struct aml_nftl_ops_t *aml_nftl_ops = aml_nftl_info->aml_nftl_ops;

	return aml_nftl_ops->read_page(aml_nftl_info, blk_addr, page_addr, data_buf, nftl_oob_buf, oob_len);
}

static int aml_nftl_copy_page(struct aml_nftl_info_t * aml_nftl_info, addr_blk_t dest_blk_addr, addr_page_t dest_page,
				addr_blk_t src_blk_addr, addr_page_t src_page)
{
	int status = 0, oob_len;
	unsigned char *nftl_data_buf;
	unsigned char nftl_oob_buf[aml_nftl_info->oobsize];
	struct nftl_oobinfo_t *nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;
	struct phyblk_node_t *phy_blk_node = &aml_nftl_info->phypmt[dest_blk_addr];

	oob_len = min(aml_nftl_info->oobsize, (sizeof(struct nftl_oobinfo_t) + strlen(AML_NFTL_MAGIC)));
	nftl_data_buf = aml_nftl_info->copy_page_buf;
	status = aml_nftl_info->read_page(aml_nftl_info, src_blk_addr, src_page, nftl_data_buf, nftl_oob_buf, oob_len);
	if (status) {
		aml_nftl_dbg("copy page read failed: %d %d status: %d\n", src_blk_addr, src_page, status);
		status = 1;
		goto exit;
	}

	if (aml_nftl_info->oobsize >= (sizeof(struct nftl_oobinfo_t) + strlen(AML_NFTL_MAGIC))) {
		if (memcmp((nftl_oob_buf + sizeof(struct nftl_oobinfo_t)), AML_NFTL_MAGIC, strlen(AML_NFTL_MAGIC))) {
			aml_nftl_dbg("nftl read invalid magic vtblk: %d %d \n", nftl_oob_info->vtblk, src_blk_addr);
		}
	}
	nftl_oob_info->ec = phy_blk_node->ec;
	nftl_oob_info->timestamp = phy_blk_node->timestamp;
	nftl_oob_info->status_page = 1;
	status = aml_nftl_info->write_page(aml_nftl_info, dest_blk_addr, dest_page, nftl_data_buf, nftl_oob_buf, oob_len);
	if (status) {
		aml_nftl_dbg("copy page write failed: %d status: %d\n", dest_blk_addr, status);
		status = 2;
		goto exit;
	}

exit:
	return status;
}

static int aml_nftl_get_page_info(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr, addr_page_t page_addr, unsigned char *nftl_oob_buf, int oob_len)
{
	struct aml_nftl_ops_t *aml_nftl_ops = aml_nftl_info->aml_nftl_ops;

	return aml_nftl_ops->read_page_oob(aml_nftl_info, blk_addr, page_addr, nftl_oob_buf, oob_len);
}

static int aml_nftl_blk_isbad(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr)
{
	struct aml_nftl_ops_t *aml_nftl_ops = aml_nftl_info->aml_nftl_ops;

	return aml_nftl_ops->blk_isbad(aml_nftl_info, blk_addr);
}

static int aml_nftl_blk_mark_bad(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t blk_addr)
{
	struct aml_nftl_ops_t *aml_nftl_ops = aml_nftl_info->aml_nftl_ops;
	struct phyblk_node_t *phy_blk_node = &aml_nftl_info->phypmt[blk_addr];
	struct vtblk_node_t  *vt_blk_node, *vt_blk_node_prev;

	if (phy_blk_node->vtblk >= 0) {
		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + phy_blk_node->vtblk));
		do {
			vt_blk_node_prev = vt_blk_node;
			vt_blk_node = vt_blk_node->next;

			printk("aml_nftl_blk_mark_bad :vt_blk_node:0x%lx, blk_addr:%d\n", (long unsigned int)vt_blk_node, blk_addr);
			//mdelay(10);

			if ((vt_blk_node != NULL) && (vt_blk_node->phy_blk_addr == blk_addr)) {
				vt_blk_node_prev->next = vt_blk_node->next;
				aml_nftl_free(vt_blk_node);
	            vt_blk_node = NULL; //NULL for free
			}
			else if ((vt_blk_node_prev != NULL) && (vt_blk_node_prev->phy_blk_addr == blk_addr)) {
				if (*(aml_nftl_info->vtpmt + phy_blk_node->vtblk) == vt_blk_node_prev)
					*(aml_nftl_info->vtpmt + phy_blk_node->vtblk) = vt_blk_node;

				aml_nftl_free(vt_blk_node_prev);
			}

		} while (vt_blk_node != NULL);
	}
	memset((unsigned char *)phy_blk_node, 0xff, sizeof(struct phyblk_node_t));
	phy_blk_node->status_page = STATUS_BAD_BLOCK;

	return aml_nftl_ops->blk_mark_bad(aml_nftl_info, blk_addr);
}

static int aml_nftl_creat_sectmap(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t phy_blk_addr)
{
	int i, status;
	uint32_t page_per_blk;
	int16_t valid_sects = 0;
	struct phyblk_node_t *phy_blk_node;
	struct nftl_oobinfo_t *nftl_oob_info;
	//unsigned char nftl_oob_buf[sizeof(struct nftl_oobinfo_t)];
	unsigned char *nftl_oob_buf;
	nftl_oob_buf = aml_nftl_malloc(sizeof(struct nftl_oobinfo_t));

	if(nftl_oob_buf== NULL){
		printk("%s,%d malloc failed\n",__func__,__LINE__);
		return -ENOMEM;;
	}
	phy_blk_node = &aml_nftl_info->phypmt[phy_blk_addr];
	nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;

	page_per_blk = aml_nftl_info->pages_per_blk;
	for (i=0; i<page_per_blk; i++) {

		if (i == 0) {
		status = aml_nftl_info->get_page_info(aml_nftl_info, phy_blk_addr, i, nftl_oob_buf, sizeof(struct nftl_oobinfo_t));
		if (status) {
			aml_nftl_dbg("nftl creat sect map faile at: %d\n", phy_blk_addr);
				aml_nftl_free(nftl_oob_buf);
			return AML_NFTL_FAILURE;
		}
			phy_blk_node->ec = nftl_oob_info->ec;
			phy_blk_node->vtblk = nftl_oob_info->vtblk;
			phy_blk_node->timestamp = nftl_oob_info->timestamp;
		}
		else{
            status = aml_nftl_info->get_page_info(aml_nftl_info, phy_blk_addr, i, nftl_oob_buf, sizeof(addr_sect_t));
		if (status) {
			aml_nftl_dbg("nftl creat sect map faile at: %d\n", phy_blk_addr);
				aml_nftl_free(nftl_oob_buf);
			return AML_NFTL_FAILURE;
		}
		}

		if (nftl_oob_info->sect >= 0) {
			phy_blk_node->phy_page_map[nftl_oob_info->sect] = i;
			phy_blk_node->last_write = i;
			valid_sects++;
		}
		else
			break;
	}
	phy_blk_node->valid_sects = valid_sects;
	aml_nftl_free(nftl_oob_buf);
	return 0;
}

static int aml_nftl_get_phy_sect_map(struct aml_nftl_info_t * aml_nftl_info, addr_blk_t blk_addr)
{
	int status;
	struct phyblk_node_t *phy_blk_node;
	phy_blk_node = &aml_nftl_info->phypmt[blk_addr];

	if (phy_blk_node->valid_sects < 0) {
		status = aml_nftl_creat_sectmap(aml_nftl_info, blk_addr);
		if (status)
			return AML_NFTL_FAILURE;
	}

	return 0;
}

static void aml_nftl_erase_sect_map(struct aml_nftl_info_t * aml_nftl_info, addr_blk_t blk_addr)
{
	struct phyblk_node_t *phy_blk_node;
	struct vtblk_node_t  *vt_blk_node, *vt_blk_node_prev;
	phy_blk_node = &aml_nftl_info->phypmt[blk_addr];

	if (phy_blk_node->vtblk >= 0) {
		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + phy_blk_node->vtblk));
		do {
			vt_blk_node_prev = vt_blk_node;
			vt_blk_node = vt_blk_node->next;

			if ((vt_blk_node != NULL) && (vt_blk_node->phy_blk_addr == blk_addr)) {
				aml_nftl_dbg("free blk had mapped vt blk: %d phy blk: %d\n", phy_blk_node->vtblk, blk_addr);
				vt_blk_node_prev->next = vt_blk_node->next;
				aml_nftl_free(vt_blk_node);
				vt_blk_node = NULL; //NULL for free
			}
			else if ((vt_blk_node_prev != NULL) && (vt_blk_node_prev->phy_blk_addr == blk_addr)) {
				aml_nftl_dbg("free blk had mapped vt blk: %d phy blk: %d\n", phy_blk_node->vtblk, blk_addr);
				if (*(aml_nftl_info->vtpmt + phy_blk_node->vtblk) == vt_blk_node_prev)
					*(aml_nftl_info->vtpmt + phy_blk_node->vtblk) = vt_blk_node;

				aml_nftl_free(vt_blk_node_prev);
			}

		} while (vt_blk_node != NULL);
	}

	phy_blk_node->ec++;
	phy_blk_node->valid_sects = 0;
	phy_blk_node->vtblk = BLOCK_INIT_VALUE;
	phy_blk_node->last_write = BLOCK_INIT_VALUE;
	phy_blk_node->timestamp = MAX_TIMESTAMP_NUM;
	memset(phy_blk_node->phy_page_map, 0xff, (sizeof(addr_sect_t)*MAX_PAGES_IN_BLOCK));
	memset(phy_blk_node->phy_page_delete, 0xff, (MAX_PAGES_IN_BLOCK>>3));

	return;
}

static int aml_nftl_erase_block(struct aml_nftl_info_t * aml_nftl_info, addr_blk_t blk_addr)
{
	struct aml_nftl_ops_t *aml_nftl_ops = aml_nftl_info->aml_nftl_ops;
	int status;

	status = aml_nftl_ops->erase_block(aml_nftl_info, blk_addr);
	if (status)
		return AML_NFTL_FAILURE;

	aml_nftl_erase_sect_map(aml_nftl_info, blk_addr);
	return 0;
}

int aml_nftl_add_node(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t logic_blk_addr, addr_blk_t phy_blk_addr)
{
	struct phyblk_node_t *phy_blk_node_curt, *phy_blk_node_add;
	struct vtblk_node_t  *vt_blk_node_curt, *vt_blk_node_add;
	struct aml_nftl_wl_t *aml_nftl_wl;
	addr_blk_t phy_blk_cur;
	int status = 0;

	aml_nftl_wl = aml_nftl_info->aml_nftl_wl;
	vt_blk_node_add = (struct vtblk_node_t *)aml_nftl_malloc(sizeof(struct vtblk_node_t));
	if (vt_blk_node_add == NULL)
		return AML_NFTL_FAILURE;

	vt_blk_node_add->phy_blk_addr = phy_blk_addr;
	vt_blk_node_add->next = NULL;
	phy_blk_node_add = &aml_nftl_info->phypmt[phy_blk_addr];
	phy_blk_node_add->vtblk = logic_blk_addr;
	vt_blk_node_curt = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + logic_blk_addr));

	if (vt_blk_node_curt == NULL) {
		phy_blk_node_add->timestamp = 0;
	}
	else {
		phy_blk_cur = vt_blk_node_curt->phy_blk_addr;
		phy_blk_node_curt = &aml_nftl_info->phypmt[phy_blk_cur];
		if (phy_blk_node_curt->timestamp >= MAX_TIMESTAMP_NUM)
			phy_blk_node_add->timestamp = 0;
		else
			phy_blk_node_add->timestamp = phy_blk_node_curt->timestamp + 1;
		vt_blk_node_add->next = vt_blk_node_curt;

	}
	*(aml_nftl_info->vtpmt + logic_blk_addr) = vt_blk_node_add;

	return status;
}

static int aml_nftl_calculate_last_write(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t phy_blk_addr)
{
	int status;
	struct phyblk_node_t *phy_blk_node;
	phy_blk_node = &aml_nftl_info->phypmt[phy_blk_addr];

	if (phy_blk_node->valid_sects < 0) {
		status = aml_nftl_creat_sectmap(aml_nftl_info, phy_blk_addr);
		if (status)
			return AML_NFTL_FAILURE;
	}

	return 0;
}

static int aml_nftl_get_valid_pos(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t logic_blk_addr, addr_blk_t *phy_blk_addr,
									 addr_page_t logic_page_addr, addr_page_t *phy_page_addr, uint8_t flag )
{
	struct phyblk_node_t *phy_blk_node;
	struct vtblk_node_t  *vt_blk_node;
	int status;
	uint32_t page_per_blk;

	page_per_blk = aml_nftl_info->pages_per_blk;
	*phy_blk_addr = BLOCK_INIT_VALUE;
	vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + logic_blk_addr));
	if (vt_blk_node == NULL)
		return AML_NFTL_BLKNOTFOUND;

	*phy_blk_addr = vt_blk_node->phy_blk_addr;
	phy_blk_node = &aml_nftl_info->phypmt[*phy_blk_addr];
	status = aml_nftl_get_phy_sect_map(aml_nftl_info, *phy_blk_addr);
	if (status)
		return AML_NFTL_FAILURE;

	if (flag == WRITE_OPERATION) {
		if (phy_blk_node->last_write < 0)
			aml_nftl_calculate_last_write(aml_nftl_info, *phy_blk_addr);

		*phy_page_addr = phy_blk_node->last_write + 1;
		if (*phy_page_addr >= page_per_blk)
			return AML_NFTL_PAGENOTFOUND;

		return 0;
	}
	else if (flag == READ_OPERATION) {
		do {

			*phy_blk_addr = vt_blk_node->phy_blk_addr;
			phy_blk_node = &aml_nftl_info->phypmt[*phy_blk_addr];

			status = aml_nftl_get_phy_sect_map(aml_nftl_info, *phy_blk_addr);
			if (status)
				return AML_NFTL_FAILURE;

			if (phy_blk_node->phy_page_map[logic_page_addr] >= 0) {
				*phy_page_addr = phy_blk_node->phy_page_map[logic_page_addr];
				return 0;
			}

			vt_blk_node = vt_blk_node->next;
		} while (vt_blk_node != NULL);

		return AML_NFTL_PAGENOTFOUND;
	}

	return AML_NFTL_FAILURE;
}

static int aml_nftl_read_sect(struct aml_nftl_info_t *aml_nftl_info, addr_page_t sect_addr, unsigned char *buf)
{
	uint32_t page_per_blk;
	addr_page_t logic_page_addr, phy_page_addr;
	addr_blk_t logic_blk_addr, phy_blk_addr;
	int status = 0;

	page_per_blk = aml_nftl_info->pages_per_blk;
	logic_page_addr = sect_addr % page_per_blk;
	logic_blk_addr = sect_addr / page_per_blk;

	status = aml_nftl_get_valid_pos(aml_nftl_info, logic_blk_addr, &phy_blk_addr, logic_page_addr, &phy_page_addr, READ_OPERATION);
	if ((status == AML_NFTL_PAGENOTFOUND) || (status == AML_NFTL_BLKNOTFOUND)) {
		memset(buf, 0xff, aml_nftl_info->writesize);
		return 0;
	}

	if (status == AML_NFTL_FAILURE)
	{
		aml_nftl_badblock_handle(aml_nftl_info, phy_blk_addr, logic_blk_addr);
        return AML_NFTL_FAILURE;
	}

	status = aml_nftl_info->read_page(aml_nftl_info, phy_blk_addr, phy_page_addr, buf, NULL, 0);
	if (status)
	{
		aml_nftl_badblock_handle(aml_nftl_info, phy_blk_addr, logic_blk_addr);
		return status;
	}

	return 0;
}

static void aml_nftl_delete_sect(struct aml_nftl_info_t *aml_nftl_info, addr_page_t sect_addr)
{
	struct vtblk_node_t  *vt_blk_node;
	struct phyblk_node_t *phy_blk_node;
	uint32_t page_per_blk;
	addr_page_t logic_page_addr;
	addr_blk_t logic_blk_addr, phy_blk_addr;

	page_per_blk = aml_nftl_info->pages_per_blk;
	logic_page_addr = sect_addr % page_per_blk;
	logic_blk_addr = sect_addr / page_per_blk;
	vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + logic_blk_addr));
	if (vt_blk_node == NULL)
		return;

	while (vt_blk_node != NULL) {

		phy_blk_addr = vt_blk_node->phy_blk_addr;
		phy_blk_node = &aml_nftl_info->phypmt[phy_blk_addr];
		phy_blk_node->phy_page_delete[logic_page_addr>>3] &= (~(1 << (logic_page_addr % 8)));
		vt_blk_node = vt_blk_node->next;
	}
	return;
}

static int aml_nftl_write_sect(struct aml_nftl_info_t *aml_nftl_info, addr_page_t sect_addr, unsigned char *buf)
{
	struct vtblk_node_t  *vt_blk_node;
	struct phyblk_node_t *phy_blk_node;
	int status = 0, special_gc = 0, oob_len, retry_cnt = 0;
	struct aml_nftl_wl_t *aml_nftl_wl = aml_nftl_info->aml_nftl_wl;
	uint32_t page_per_blk;
	addr_page_t logic_page_addr, phy_page_addr;
	addr_blk_t logic_blk_addr, phy_blk_addr;
	unsigned char nftl_oob_buf[aml_nftl_info->oobsize];
	struct nftl_oobinfo_t *nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;

	page_per_blk = aml_nftl_info->pages_per_blk;

	logic_page_addr = sect_addr % page_per_blk;
	logic_blk_addr = sect_addr / page_per_blk;
	aml_nftl_info->current_write_block = logic_blk_addr;
	if (aml_nftl_wl->wait_gc_block >= 0) {
		aml_nftl_info->continue_writed_sects++;
		if (aml_nftl_wl->wait_gc_block == logic_blk_addr) {
			aml_nftl_wl->wait_gc_block = BLOCK_INIT_VALUE;
			aml_nftl_wl->garbage_collect(aml_nftl_wl, 0);
			aml_nftl_info->continue_writed_sects = 0;
			aml_nftl_wl->add_gc(aml_nftl_wl, logic_blk_addr);
		}
	}

	status = aml_nftl_get_valid_pos(aml_nftl_info, logic_blk_addr, &phy_blk_addr, logic_page_addr, &phy_page_addr, WRITE_OPERATION);
	if (status == AML_NFTL_FAILURE)
	{
	    aml_nftl_info->blk_mark_bad(aml_nftl_info, phy_blk_addr);
	    aml_nftl_dbg("%s, line %d, nftl write page faile blk: %d page: %d status: %d\n", __func__, __LINE__, phy_blk_addr, phy_page_addr, status);
	    status = AML_NFTL_BLKNOTFOUND;
            //return AML_NFTL_FAILURE;
	}

	if ((status == AML_NFTL_PAGENOTFOUND) || (status == AML_NFTL_BLKNOTFOUND)) {

		if ((aml_nftl_wl->free_root.count < (aml_nftl_info->fillfactor / 2)) && (!aml_nftl_wl->erased_root.count) && (aml_nftl_wl->wait_gc_block < 0))
			aml_nftl_wl->garbage_collect(aml_nftl_wl, 0);

		status = aml_nftl_wl->get_best_free(aml_nftl_wl, &phy_blk_addr);
		if (status) {
			status = aml_nftl_wl->garbage_collect(aml_nftl_wl, DO_COPY_PAGE);
			if (status == 0) {
				aml_nftl_dbg("nftl couldn`t found free block: %d %d\n", aml_nftl_wl->free_root.count, aml_nftl_wl->wait_gc_block);
				return -ENOENT;
			}
			status = aml_nftl_wl->get_best_free(aml_nftl_wl, &phy_blk_addr);
			if (status)
				return status;
		}

		aml_nftl_add_node(aml_nftl_info, logic_blk_addr, phy_blk_addr);
		aml_nftl_wl->add_used(aml_nftl_wl, phy_blk_addr);
		phy_page_addr = 0;
	}

WRITE_RETRY:
	phy_blk_node = &aml_nftl_info->phypmt[phy_blk_addr];
	nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;
	nftl_oob_info->ec = phy_blk_node->ec;
	nftl_oob_info->vtblk = logic_blk_addr;
	nftl_oob_info->timestamp = phy_blk_node->timestamp;
	nftl_oob_info->status_page = 1;
	nftl_oob_info->sect = logic_page_addr;
	oob_len = min(aml_nftl_info->oobsize, (sizeof(struct nftl_oobinfo_t) + strlen(AML_NFTL_MAGIC)));
	if (aml_nftl_info->oobsize >= (sizeof(struct nftl_oobinfo_t) + strlen(AML_NFTL_MAGIC)))
		memcpy((nftl_oob_buf + sizeof(struct nftl_oobinfo_t)), AML_NFTL_MAGIC, strlen(AML_NFTL_MAGIC));

	status = aml_nftl_info->write_page(aml_nftl_info, phy_blk_addr, phy_page_addr, buf, nftl_oob_buf, oob_len);
	if (status && (retry_cnt++ < 3))
	{  //do not markbad when write failed, just get another block and write again till ok
	    aml_nftl_dbg("%s, line %d, nftl write page faile blk: %d page: %d status: %d, retry_cnt:%d\n", __func__, __LINE__, phy_blk_addr, phy_page_addr, status, retry_cnt);
#if 1
		phy_blk_addr = aml_nftl_badblock_handle(aml_nftl_info, phy_blk_addr, logic_blk_addr);
		if(phy_blk_addr >= 0) //sucess
			goto WRITE_RETRY;
		else
		{
			aml_nftl_dbg("%s, line %d, aml_nftl_badblock_handle failed blk: %d retry_cnt:%d\n", __func__, __LINE__, phy_blk_addr, retry_cnt);
			return -ENOENT;
		}
#else
		aml_nftl_info->blk_mark_bad(aml_nftl_info, phy_blk_addr);
		aml_nftl_dbg("nftl write page faile blk: %d page: %d status: %d\n", phy_blk_addr, phy_page_addr, status);
		return status;
#endif
	}
	else if(status)
	{
		aml_nftl_info->blk_mark_bad(aml_nftl_info, phy_blk_addr);
		aml_nftl_dbg("%s, line %d, nftl write page faile blk: %d page: %d status: %d, retry_cnt:%d\n", __func__, __LINE__, phy_blk_addr, phy_page_addr, status, retry_cnt);
		return status;
	}

	vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + logic_blk_addr));
	if ((aml_nftl_get_node_length(aml_nftl_info, vt_blk_node) > MAX_BLK_NUM_PER_NODE) || (phy_page_addr == (page_per_blk - 1))) {
		status = aml_nftl_check_node(aml_nftl_info, logic_blk_addr);
		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + logic_blk_addr));
		if (status == AML_NFTL_STRUCTURE_FULL) {
			//aml_nftl_dbg("aml nftl structure full at logic : %d phy blk: %d %d \n", logic_blk_addr, phy_blk_addr, phy_blk_node->last_write);
			status = aml_nftl_wl->garbage_one(aml_nftl_wl, logic_blk_addr, 0);
			if (status < 0)
				return status;
			special_gc = 1;
		}
		else if (aml_nftl_get_node_length(aml_nftl_info, vt_blk_node) >= BASIC_BLK_NUM_PER_NODE) {
			aml_nftl_wl->add_gc(aml_nftl_wl, logic_blk_addr);
		}
	}

	if ((aml_nftl_wl->wait_gc_block >= 0) && (!special_gc)) {
		if (((aml_nftl_info->continue_writed_sects % 8) == 0) || (aml_nftl_wl->free_root.count < (aml_nftl_info->fillfactor / 4))) {
			aml_nftl_wl->garbage_collect(aml_nftl_wl, 0);
			if (aml_nftl_wl->wait_gc_block == BLOCK_INIT_VALUE)
				aml_nftl_info->continue_writed_sects = 0;
		}
	}

	return 0;
}

static void aml_nftl_add_block(struct aml_nftl_info_t *aml_nftl_info, addr_blk_t phy_blk, struct nftl_oobinfo_t *nftl_oob_info)
{
	struct phyblk_node_t *phy_blk_node_curt, *phy_blk_node_add;
	struct vtblk_node_t  *vt_blk_node_curt, *vt_blk_node_prev, *vt_blk_node_add;

	vt_blk_node_add = (struct vtblk_node_t *)aml_nftl_malloc(sizeof(struct vtblk_node_t));
	if (vt_blk_node_add == NULL)
		return;
	vt_blk_node_add->phy_blk_addr = phy_blk;
	vt_blk_node_add->next = NULL;
	phy_blk_node_add = &aml_nftl_info->phypmt[phy_blk];
	phy_blk_node_add->ec = nftl_oob_info->ec;
	phy_blk_node_add->vtblk = nftl_oob_info->vtblk;
	phy_blk_node_add->timestamp = nftl_oob_info->timestamp;
	vt_blk_node_curt = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + nftl_oob_info->vtblk));

	if (vt_blk_node_curt == NULL) {
		vt_blk_node_curt = vt_blk_node_add;
		*(aml_nftl_info->vtpmt + nftl_oob_info->vtblk) = vt_blk_node_curt;
		return;
	}

	vt_blk_node_prev = vt_blk_node_curt;
	while(vt_blk_node_curt != NULL) {

		phy_blk_node_curt = &aml_nftl_info->phypmt[vt_blk_node_curt->phy_blk_addr];
		if (((phy_blk_node_add->timestamp > phy_blk_node_curt->timestamp)
			 && ((phy_blk_node_add->timestamp - phy_blk_node_curt->timestamp) < (MAX_TIMESTAMP_NUM - aml_nftl_info->accessibleblocks)))
			|| ((phy_blk_node_add->timestamp < phy_blk_node_curt->timestamp)
			 && ((phy_blk_node_curt->timestamp - phy_blk_node_add->timestamp) >= (MAX_TIMESTAMP_NUM - aml_nftl_info->accessibleblocks)))) {

			vt_blk_node_add->next = vt_blk_node_curt;
			if (*(aml_nftl_info->vtpmt + nftl_oob_info->vtblk) == vt_blk_node_curt)
				*(aml_nftl_info->vtpmt + nftl_oob_info->vtblk) = vt_blk_node_add;
			else
				vt_blk_node_prev->next = vt_blk_node_add;
			break;
		}
		else if (phy_blk_node_add->timestamp == phy_blk_node_curt->timestamp) {
			aml_nftl_dbg("NFTL timestamp err logic blk: %d phy blk: %d %d\n", nftl_oob_info->vtblk, vt_blk_node_curt->phy_blk_addr, vt_blk_node_add->phy_blk_addr);
			if (phy_blk_node_add->ec < phy_blk_node_curt->ec) {

				vt_blk_node_prev->next = vt_blk_node_add;
				vt_blk_node_add->next = vt_blk_node_curt->next;
				vt_blk_node_add = vt_blk_node_curt;
			}
			vt_blk_node_curt = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + nftl_oob_info->vtblk));
			vt_blk_node_add->next = vt_blk_node_curt;
			*(aml_nftl_info->vtpmt + nftl_oob_info->vtblk) = vt_blk_node_add;
			break;
		}
		else {
			if (vt_blk_node_curt->next != NULL) {
				vt_blk_node_prev = vt_blk_node_curt;
				vt_blk_node_curt = vt_blk_node_curt->next;
			}
			else {
				vt_blk_node_curt->next = vt_blk_node_add;
				vt_blk_node_curt = vt_blk_node_curt->next;
				break;
			}
		}
	}

	return;
}

static void aml_nftl_check_conflict_node(struct aml_nftl_info_t *aml_nftl_info)
{
	struct vtblk_node_t  *vt_blk_node;
	struct aml_nftl_wl_t *aml_nftl_wl;
	addr_blk_t vt_blk_num;
	int node_length = 0, status = 0;
	struct timespec ts_check_start, ts_check_current;

	ktime_get_ts(&ts_check_start);
	aml_nftl_wl = aml_nftl_info->aml_nftl_wl;
	for (vt_blk_num=0; vt_blk_num<aml_nftl_info->accessibleblocks; vt_blk_num++) {

		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
		if (vt_blk_node == NULL)
			continue;

		node_length = aml_nftl_get_node_length(aml_nftl_info, vt_blk_node);
		if (node_length < MAX_BLK_NUM_PER_NODE/* BASIC_BLK_NUM_PER_NODE */)
			continue;

        //aml_nftl_dbg("need check conflict node vt blk: %d and node_length:%d\n", vt_blk_num, node_length);

		status = aml_nftl_check_node(aml_nftl_info, vt_blk_num);
		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
		if (aml_nftl_get_node_length(aml_nftl_info, vt_blk_node) >= BASIC_BLK_NUM_PER_NODE)
			aml_nftl_wl->add_gc(aml_nftl_wl, vt_blk_num);
		if (status == AML_NFTL_STRUCTURE_FULL) {
			aml_nftl_dbg("found conflict node vt blk: %d \n", vt_blk_num);
			aml_nftl_wl->garbage_one(aml_nftl_wl, vt_blk_num, 0);
		}
		if (vt_blk_num >= (aml_nftl_info->accessibleblocks / 2)) {
			ktime_get_ts(&ts_check_current);
			if ((ts_check_current.tv_sec - ts_check_start.tv_sec) >= 5) {
				aml_nftl_dbg("check conflict node timeout: %d \n", vt_blk_num);
				break;
			}
		}
	}
//	if (vt_blk_num >= aml_nftl_info->accessibleblocks)
//		aml_nftl_info->isinitialised = 1;
//	aml_nftl_info->cur_split_blk = vt_blk_num;

	return;
}

static void aml_nftl_creat_structure(struct aml_nftl_info_t *aml_nftl_info)
{
	struct vtblk_node_t  *vt_blk_node;
	struct aml_nftl_wl_t *aml_nftl_wl;
	int status = 0;
	addr_blk_t vt_blk_num;

	aml_nftl_wl = aml_nftl_info->aml_nftl_wl;
	for (vt_blk_num=aml_nftl_info->cur_split_blk; vt_blk_num<aml_nftl_info->accessibleblocks; vt_blk_num++) {

		if ((vt_blk_num - aml_nftl_info->cur_split_blk) >= DEFAULT_SPLIT_UNIT)
			break;

		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
		if (vt_blk_node == NULL)
			continue;

		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
		if (aml_nftl_get_node_length(aml_nftl_info, vt_blk_node) < BASIC_BLK_NUM_PER_NODE)
			continue;

		status = aml_nftl_check_node(aml_nftl_info, vt_blk_num);
		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
		if (aml_nftl_get_node_length(aml_nftl_info, vt_blk_node) >= BASIC_BLK_NUM_PER_NODE)
			aml_nftl_wl->add_gc(aml_nftl_wl, vt_blk_num);
		if (status == AML_NFTL_STRUCTURE_FULL) {
			aml_nftl_dbg("found conflict node vt blk: %d \n", vt_blk_num);
			aml_nftl_wl->garbage_one(aml_nftl_wl, vt_blk_num, 0);
		}
	}

	aml_nftl_info->cur_split_blk = vt_blk_num;
	if (aml_nftl_info->cur_split_blk >= aml_nftl_info->accessibleblocks) {
		if ((aml_nftl_wl->free_root.count <= DEFAULT_IDLE_FREE_BLK) && (!aml_nftl_wl->erased_root.count))
			aml_nftl_wl->gc_need_flag = 1;

		aml_nftl_info->isinitialised = 1;
		aml_nftl_wl->gc_start_block = aml_nftl_info->accessibleblocks - 1;
		aml_nftl_dbg("nftl creat stucture completely free blk: %d erased blk: %d\n", aml_nftl_wl->free_root.count, aml_nftl_wl->erased_root.count);
	}

	return;
}

/* Our partition node structure */
struct mtd_part {
	struct mtd_info mtd;
	struct mtd_info *master;
	uint64_t offset;
	struct list_head list;
};

static ssize_t show_address_map_table(struct class *class,
			struct class_attribute *attr,	const char *buf, size_t count)
{
    struct aml_nftl_info_t *aml_nftl_info = container_of(class, struct aml_nftl_info_t, cls);
	unsigned int address;
    unsigned blks_per_sect, sect_addr;
    addr_page_t logic_page_addr, phy_page_addr;
	addr_blk_t logic_blk_addr, phy_blk_addr;
    uint32_t page_per_blk;
    int ret;
    struct mtd_info *mtd = aml_nftl_info->mtd;
    struct mtd_part *part = (struct mtd_part *)mtd;
    loff_t phy_address;

	ret =  sscanf(buf, "%x", &address);
    blks_per_sect = aml_nftl_info->writesize / 512;
	sect_addr = address / blks_per_sect;

    page_per_blk = aml_nftl_info->pages_per_blk;
    logic_page_addr = sect_addr % page_per_blk;
	logic_blk_addr = sect_addr / page_per_blk;

    ret = aml_nftl_get_valid_pos(aml_nftl_info, logic_blk_addr, &phy_blk_addr, logic_page_addr, &phy_page_addr, READ_OPERATION);
	if (ret == AML_NFTL_FAILURE){
        printk("AML_NFTL_FAILURE!!\n");
        return AML_NFTL_FAILURE;
	}

    if ((ret == AML_NFTL_PAGENOTFOUND) || (ret == AML_NFTL_BLKNOTFOUND)) {
		printk("the phy address not found\n");
		return 1;
	}
    phy_address = part->offset + (phy_blk_addr * mtd->erasesize) + (phy_page_addr * mtd->writesize);
    printk("address %x map phy address:blk %x page %x (address %llx)\n", address, phy_blk_addr, phy_page_addr, phy_address);

	return count;
}
static ssize_t show_logic_block_table(struct class *class,
            struct class_attribute *attr,   char *buf)
{
    ssize_t count=0;
    static addr_blk_t logic_blk_addr=0;
    struct aml_nftl_info_t *aml_nftl_info = container_of(class, struct aml_nftl_info_t, cls);
    logic_blk_addr=logic_blk_addr<aml_nftl_info->accessibleblocks?logic_blk_addr:0;
    count += snprintf(&buf[count], PAGE_SIZE - count,
            "Logic Block Address(%4dKBytes) :Physical Block Address ,",
            aml_nftl_info->pages_per_blk * aml_nftl_info->writesize / 1024);
    count += snprintf(&buf[count], PAGE_SIZE - count,
                "start=%d,end=%d\n",
                aml_nftl_info->startblock,aml_nftl_info->startblock);
    for(;logic_blk_addr<aml_nftl_info->accessibleblocks;logic_blk_addr++)
    {
        struct vtblk_node_t * vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + logic_blk_addr));
        if(vt_blk_node==NULL)
            continue;
        count+=snprintf(&buf[count],PAGE_SIZE-count,"%d:",logic_blk_addr);
        if(count>=PAGE_SIZE)
            return count;
        while (vt_blk_node != NULL) {
            count+=snprintf(&buf[count],PAGE_SIZE-count, "%d,", vt_blk_node->phy_blk_addr);
            vt_blk_node = vt_blk_node->next;
            if(count>=PAGE_SIZE)
                        return count;
        }
        count+=snprintf(&buf[count],PAGE_SIZE-count,"\n");
        if(count>=PAGE_SIZE)
                    return count;

    }


    return count;
}

static ssize_t show_node_info(struct class *class,
			struct class_attribute *attr,	const char *buf, size_t count)
{
	struct aml_nftl_info_t *aml_nftl_info = container_of(class, struct aml_nftl_info_t, cls);
	unsigned blk_addr;
	int ret;

	ret =  sscanf(buf, "%x", &blk_addr);

	get_vt_node_info(aml_nftl_info, blk_addr);

	return count;
}


static struct class_attribute nftl_class_attrs[] = {
    __ATTR(map_table,  S_IRUGO | S_IWUSR, show_logic_block_table,    show_address_map_table),
//    __ATTR(logic_block_table,  S_IRUGO , show_logic_block_table,    NULL),
	__ATTR(check_node,  S_IRUGO | S_IWUSR, NULL,    show_node_info),
    __ATTR_NULL
};

int aml_nftl_initialize(struct aml_nftl_blk_t *aml_nftl_blk)
{
	struct mtd_info *mtd = aml_nftl_blk->mbd.mtd;
	struct aml_nftl_info_t *aml_nftl_info;
	struct nftl_oobinfo_t *nftl_oob_info;
	struct aml_nftl_wl_t *aml_nftl_wl;
	struct phyblk_node_t *phy_blk_node;
	int error = 0, phy_blk_num, oob_len;
	uint32_t phy_page_addr, size_in_blk;
	uint32_t phys_erase_shift;
	unsigned char nftl_oob_buf[mtd->oobavail];

	if (mtd->oobavail < sizeof(struct nftl_oobinfo_t))
		return -EPERM;
	aml_nftl_info = aml_nftl_malloc(sizeof(struct aml_nftl_info_t));
	if (!aml_nftl_info)
		return -ENOMEM;

	aml_nftl_blk->aml_nftl_info = aml_nftl_info;
	aml_nftl_info->mtd = mtd;
	aml_nftl_info->writesize = mtd->writesize;
	aml_nftl_info->oobsize = mtd->oobavail;
	phys_erase_shift = ffs(mtd->erasesize) - 1;
	size_in_blk =  (mtd->size >> phys_erase_shift);
	if (size_in_blk <= AML_LIMIT_FACTOR)
		return -EPERM;

	aml_nftl_info->pages_per_blk = mtd->erasesize / mtd->writesize;
	aml_nftl_info->fillfactor = (size_in_blk / 32);
	if (aml_nftl_info->fillfactor < AML_LIMIT_FACTOR)
		aml_nftl_info->fillfactor = AML_LIMIT_FACTOR;
	aml_nftl_info->fillfactor += NFTL_FAT_TABLE_NUM * 2* MAX_BLK_NUM_PER_NODE;
	aml_nftl_info->accessibleblocks = size_in_blk - aml_nftl_info->fillfactor;
	aml_nftl_info->current_write_block = BLOCK_INIT_VALUE;

	aml_nftl_info->copy_page_buf = aml_nftl_malloc(aml_nftl_info->writesize);
	if (!aml_nftl_info->copy_page_buf)
		return -ENOMEM;
	aml_nftl_info->phypmt = (struct phyblk_node_t *)aml_nftl_malloc((sizeof(struct phyblk_node_t) * size_in_blk));
	if (!aml_nftl_info->phypmt)
		return -ENOMEM;
	aml_nftl_info->vtpmt = (void **)aml_nftl_malloc((sizeof(void*) * aml_nftl_info->accessibleblocks));
	if (!aml_nftl_info->vtpmt)
		return -ENOMEM;
	memset((unsigned char *)aml_nftl_info->phypmt, 0xff, sizeof(struct phyblk_node_t)*size_in_blk);
    memset((unsigned char *)aml_nftl_info->vtpmt, 0, ((sizeof(void*) * aml_nftl_info->accessibleblocks)));

	aml_nftl_ops_init(aml_nftl_info);

	aml_nftl_info->read_page = aml_nftl_read_page;
	aml_nftl_info->write_page = aml_nftl_write_page;
	aml_nftl_info->copy_page = aml_nftl_copy_page;
	aml_nftl_info->get_page_info = aml_nftl_get_page_info;
	aml_nftl_info->blk_mark_bad = aml_nftl_blk_mark_bad;
	aml_nftl_info->blk_isbad = aml_nftl_blk_isbad;
	aml_nftl_info->get_phy_sect_map = aml_nftl_get_phy_sect_map;
	aml_nftl_info->erase_block = aml_nftl_erase_block;

	aml_nftl_info->read_sect = aml_nftl_read_sect;
	aml_nftl_info->write_sect = aml_nftl_write_sect;
	aml_nftl_info->delete_sect = aml_nftl_delete_sect;
	aml_nftl_info->creat_structure = aml_nftl_creat_structure;

	error = aml_nftl_wl_init(aml_nftl_info);
	if (error)
		return error;

	aml_nftl_wl = aml_nftl_info->aml_nftl_wl;
	nftl_oob_info = (struct nftl_oobinfo_t *)nftl_oob_buf;
	oob_len = min(aml_nftl_info->oobsize, (sizeof(struct nftl_oobinfo_t) + strlen(AML_NFTL_MAGIC)));
	for (phy_blk_num=0; phy_blk_num<size_in_blk; phy_blk_num++) {

		phy_page_addr = 0;
		phy_blk_node = &aml_nftl_info->phypmt[phy_blk_num];

		error = aml_nftl_info->blk_isbad(aml_nftl_info, phy_blk_num);
		if (error) {
			aml_nftl_info->accessibleblocks--;
			aml_nftl_dbg("nftl detect bad blk at : %d \n", phy_blk_num);
			continue;
		}

		error = aml_nftl_info->get_page_info(aml_nftl_info, phy_blk_num, phy_page_addr, nftl_oob_buf, oob_len);
		if (error) {
			aml_nftl_info->accessibleblocks--;
			phy_blk_node->status_page = STATUS_BAD_BLOCK;
			aml_nftl_dbg("get status error at blk: %d \n", phy_blk_num);
			continue;
		}

		if (nftl_oob_info->status_page == 0) {
			aml_nftl_info->accessibleblocks--;
			phy_blk_node->status_page = STATUS_BAD_BLOCK;
			aml_nftl_dbg("get status faile at blk: %d \n", phy_blk_num);
			aml_nftl_info->blk_mark_bad(aml_nftl_info, phy_blk_num);
			continue;
		}

		if (nftl_oob_info->vtblk == -1) {
			phy_blk_node->valid_sects = 0;
			phy_blk_node->ec = 0;
			aml_nftl_wl->add_erased(aml_nftl_wl, phy_blk_num);
		}
		else if ((nftl_oob_info->vtblk < 0) || (nftl_oob_info->vtblk >= (size_in_blk - aml_nftl_info->fillfactor))) {
			aml_nftl_dbg("nftl invalid vtblk: %d \n", nftl_oob_info->vtblk);
			error = aml_nftl_info->erase_block(aml_nftl_info, phy_blk_num);
			if (error) {
				aml_nftl_info->accessibleblocks--;
				phy_blk_node->status_page = STATUS_BAD_BLOCK;
				aml_nftl_info->blk_mark_bad(aml_nftl_info, phy_blk_num);
			}
			else {
				phy_blk_node->valid_sects = 0;
				aml_nftl_wl->add_erased(aml_nftl_wl, phy_blk_num);
			}
		}
		else {
			if (aml_nftl_info->oobsize >= (sizeof(struct nftl_oobinfo_t) + strlen(AML_NFTL_MAGIC))) {
				if (memcmp((nftl_oob_buf + sizeof(struct nftl_oobinfo_t)), AML_NFTL_MAGIC, strlen(AML_NFTL_MAGIC))) {
					aml_nftl_dbg("nftl invalid magic vtblk: %d \n", nftl_oob_info->vtblk);
					error = aml_nftl_info->erase_block(aml_nftl_info, phy_blk_num);
					if (error) {
						aml_nftl_info->accessibleblocks--;
						phy_blk_node->status_page = STATUS_BAD_BLOCK;
						aml_nftl_info->blk_mark_bad(aml_nftl_info, phy_blk_num);
					}
					else {
						phy_blk_node->valid_sects = 0;
						phy_blk_node->ec = 0;
						aml_nftl_wl->add_erased(aml_nftl_wl, phy_blk_num);
					}
					continue;
				}
			}
			aml_nftl_add_block(aml_nftl_info, phy_blk_num, nftl_oob_info);
			aml_nftl_wl->add_used(aml_nftl_wl, phy_blk_num);
		}
	}

	aml_nftl_info->isinitialised = 0;
	aml_nftl_info->cur_split_blk = 0;
	aml_nftl_wl->gc_start_block = aml_nftl_info->accessibleblocks - 1;

	aml_nftl_check_conflict_node(aml_nftl_info);

	aml_nftl_blk->mbd.size = (aml_nftl_info->accessibleblocks * (mtd->erasesize  >> 9));
	aml_nftl_dbg("nftl initilize completely dev size: 0x%lx %d\n", aml_nftl_blk->mbd.size * 512, aml_nftl_wl->free_root.count);

    /*setup class*/
	aml_nftl_info->cls.name = kzalloc(strlen((const char*)AML_NFTL_MAGIC)+1, GFP_KERNEL);

    strcpy((char *)aml_nftl_info->cls.name, (char*)AML_NFTL_MAGIC);
    aml_nftl_info->cls.class_attrs = nftl_class_attrs;
	error = class_register(&aml_nftl_info->cls);
	if(error)
		printk(" class register nand_class fail!\n");

	return 0;
}

void aml_nftl_info_release(struct aml_nftl_info_t *aml_nftl_info)
{
	if (aml_nftl_info->vtpmt)
		aml_nftl_free(aml_nftl_info->vtpmt);
	if (aml_nftl_info->phypmt)
		aml_nftl_free(aml_nftl_info->phypmt);
	if (aml_nftl_info->aml_nftl_wl)
		aml_nftl_free(aml_nftl_info->aml_nftl_wl);
}


/**
 *wl.c - jnftl wear leveling & garbage collection
 */

#include <linux/rbtree.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/freezer.h>

#include "aml_nftl.h"

#define list_to_node(l)	container_of(l, struct wl_list_t, list)
#define list_to_gc_node(l)	container_of(l, struct gc_blk_list, list)
/**
 * construct_lnode - construct list node
 * @ vblk : logical block addr
 *
 *     1. check valid block addr
 *     2. malloc node
 *     3. evaluate logical block
 *
 * If the logical block is not a valid value, NULL will be returned,
 * else return list node
 */
static inline struct wl_list_t *construct_lnode(struct aml_nftl_wl_t* wl, addr_blk_t vt_blk, addr_blk_t phy_blk)
{
	struct wl_list_t *lnode;

	if(vt_blk == BLOCK_INIT_VALUE)
		return NULL;

	lnode = aml_nftl_malloc(sizeof(struct wl_list_t));
	if(lnode) {
		lnode->vt_blk = vt_blk;
		lnode->phy_blk = phy_blk;

	}
	return lnode;
}

/**
 * construct_tnode - construct tree node
 * @ blk : linear block addr
 *
 *     1. check valid block addr
 *     2. malloc node
 *     3. evaluate linear block & ec
 *
 * If the linear block is not a valid value, NULL will be returned,
 * else return tree node
 */
static inline struct wl_rb_t* construct_tnode(struct aml_nftl_wl_t* wl, addr_blk_t blk)
{
	struct aml_nftl_info_t *aml_nftl_info = wl->aml_nftl_info;
	struct wl_rb_t* tnode;
	struct phyblk_node_t *phy_blk_node;

	if(blk == BLOCK_INIT_VALUE)
		return NULL;

	phy_blk_node = &aml_nftl_info->phypmt[blk];
	tnode = aml_nftl_malloc(sizeof(struct wl_rb_t));
	if(tnode){
		tnode->blk = blk;
		tnode->ec = phy_blk_node->ec;
	}
	return tnode;
}

/**
 * del_from_tree - delete tree node from redblack tree
 * @ tree : free / erased / used tree
 * @ tnode : tree node
 *
 * Erase tree node & tree count-- & free
 */
static void del_from_tree(struct aml_nftl_wl_t* wl, struct wl_tree_t* tree, struct wl_rb_t* tnode)
{
	rb_erase(&tnode->rb_node, &tree->root);
	tree->count--;
	aml_nftl_free(tnode);
	return;
}

/**
 * _del_free - delete tree node from free tree
 */
static inline void _del_free(struct aml_nftl_wl_t* wl, struct wl_rb_t* tnode)
{
	del_from_tree(wl, &wl->free_root, tnode);
	return;
}

/**
 * _del_free - delete tree node from used tree
 */
static inline void _del_used(struct aml_nftl_wl_t* wl, struct wl_rb_t* tnode)
{
	del_from_tree(wl, &wl->used_root, tnode);
}

/**
 * _del_free - delete tree node from erased tree
 */
static inline void _del_erased(struct aml_nftl_wl_t* wl, struct wl_rb_t* tnode)
{
	del_from_tree(wl, &wl->erased_root, tnode);
}


/**
 * add_tree - add tree node into redblack tree
 * @ tree : free / erased / used tree
 * @ tnode : tree node
 *
 * To avoid the same ec in the rb tree, as to get confused in rb_left & rb_right,
 * we make a new value: ec<<16+blk to make tree, without modify rb structure
 *     1. make ec blk
 *     2. find a suitable place in tree
 *     3. linke node & insert it at the place we just found
 *     4. add tree node number
 */
static void add_tree(struct aml_nftl_wl_t* wl, struct wl_tree_t* tree, struct wl_rb_t* tnode)
{
	struct rb_node **p = &tree->root.rb_node;
	struct rb_node *parent = NULL;
	struct wl_rb_t *cur;
	uint32_t  node_ec_blk = MAKE_EC_BLK(tnode->ec, tnode->blk);
	uint32_t  cur_ec_blk;

	while (*p) {
		parent = *p;
		cur = rb_entry(parent, struct wl_rb_t, rb_node);
		cur_ec_blk = MAKE_EC_BLK(cur->ec, cur->blk);

		if (node_ec_blk < cur_ec_blk)
			p = &parent->rb_left;
		else
			p = &parent->rb_right;
	}
	rb_link_node(&tnode->rb_node, parent, p);
	rb_insert_color(&tnode->rb_node, &tree->root);

	tree->count++;
	return;
}

/**
 * search_tree - search ec_blk from the rb tree, get the tree node
 * @ tree : tree searched in
 * @ blk : linear block addr
 * @ ec : erase count
 *
 * To avoid the same ec in the rb tree, as to get confused in rb_left & rb_right,
 * we make a new value: ec<<16+blk to make tree, without modify rb structure
 *     1. make ec blk
 *     2. search the same ec_blk in the tree
 *     3. return tree node or NULL
 */
static struct wl_rb_t* search_tree(struct aml_nftl_wl_t* wl, struct wl_tree_t* tree, addr_blk_t blk,
						erase_count_t ec)
{
	struct rb_node **p = &tree->root.rb_node;
	struct rb_node *parent = NULL;
	struct wl_rb_t* cur;
	uint32_t  node_ec_blk = MAKE_EC_BLK(ec, blk);
	uint32_t  cur_ec_blk;

	while(*p){
		parent = *p;
		cur = rb_entry(parent, struct wl_rb_t, rb_node);
		cur_ec_blk = MAKE_EC_BLK(cur->ec, cur->blk);

		if(node_ec_blk < cur_ec_blk)
			p = &parent->rb_left;
		else if(node_ec_blk > cur_ec_blk)
			p = &parent->rb_right;
		else
			return cur;
	}

	return NULL;
}

static void add_used(struct aml_nftl_wl_t* wl, addr_blk_t blk)
{
	struct wl_rb_t* tnode;

	tnode = construct_tnode(wl, blk);
	if(tnode)
		add_tree(wl, &wl->used_root, tnode);

	return;
}

/**
 * add_erased - add erased free block info erase rb_tree
 * @ blk : linear block addr
 */
static void add_erased(struct aml_nftl_wl_t* wl, addr_blk_t blk)
{
	struct aml_nftl_info_t *aml_nftl_info = wl->aml_nftl_info;
	struct phyblk_node_t *phy_blk_node;
	struct wl_rb_t* tnode;

	phy_blk_node = &aml_nftl_info->phypmt[blk];
	tnode = search_tree(wl, &wl->used_root, blk, phy_blk_node->ec);
	if (tnode)
		_del_used(wl, tnode);

	tnode = construct_tnode(wl, blk);
	if(tnode)
		add_tree(wl, &wl->erased_root, tnode);
}


/**
 * add_free - add free block into free rb tree
 * @ blk : linear block addr
 *
 *     1. construct tree node
 *     2. add node into free rb tree
 *     3. update current delta(current free block ec - coldest used block)
 */
static void add_free(struct aml_nftl_wl_t * wl, addr_blk_t blk)
{
	struct aml_nftl_info_t *aml_nftl_info = wl->aml_nftl_info;
	struct phyblk_node_t *phy_blk_node;
	struct wl_rb_t* tnode_cold;
	struct wl_rb_t* tnode;

	if (blk < 0)
		return;

	phy_blk_node = &aml_nftl_info->phypmt[blk];
	tnode = search_tree(wl, &wl->used_root, blk, phy_blk_node->ec);
	if (tnode)
		_del_used(wl, tnode);

	tnode = construct_tnode(wl, blk);
	if(tnode) {
		add_tree(wl, &wl->free_root, tnode);

		/*update current delta(current free block, coldest block)*/
		tnode_cold = rb_entry(rb_first(&wl->used_root.root), struct wl_rb_t, rb_node);
		if ((tnode->ec > tnode_cold->ec) && (tnode_cold->ec > 0) && (tnode->ec > 0))
			wl->cur_delta = tnode->ec - tnode_cold->ec;
	}
}

/**
 * staticwl_linear_blk - static wear leveling this linear block
 * @ blk : root linear block
 *
 * Static wl block should be the logical block only with root block
 *
 *     1. get root sector map
 *     2. get best free block
 *     3. calculate src, dest page addr base
 *     4. do copy_page from 0 to block end
 *     5. add this block to free tree
 */
static int32_t staticwl_linear_blk(struct aml_nftl_wl_t* aml_nftl_wl, addr_blk_t blk)
{
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_wl->aml_nftl_info;
	struct phyblk_node_t *phy_blk_node_src, *phy_blk_node_dest;
	struct vtblk_node_t  *vt_blk_node;
	uint16_t i, retry_cnt = 0;
	addr_blk_t dest_blk, src_blk;
	addr_page_t dest_page;
	addr_page_t src_page;

WRITE_RETRY:
	if(aml_nftl_wl->get_best_free(aml_nftl_wl, &dest_blk))
	{
		aml_nftl_dbg("%s line:%d nftl couldn`t get best free block: %d %d\n", __func__, __LINE__, aml_nftl_wl->free_root.count, aml_nftl_wl->wait_gc_block);
		return -ENOENT;
	}

	aml_nftl_wl->add_used(aml_nftl_wl, dest_blk);
	vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + blk));
	src_blk = vt_blk_node->phy_blk_addr;
	phy_blk_node_src = &aml_nftl_info->phypmt[src_blk];

	phy_blk_node_dest = &aml_nftl_info->phypmt[dest_blk];
	phy_blk_node_dest->vtblk = phy_blk_node_src->vtblk;
	if (phy_blk_node_src->timestamp >= MAX_TIMESTAMP_NUM)
		phy_blk_node_dest->timestamp = 0;
	else
		phy_blk_node_dest->timestamp = (phy_blk_node_src->timestamp + 1);

	dest_page = 0;
	for(i=0; i<aml_nftl_wl->pages_per_blk; i++){

		src_page = phy_blk_node_src->phy_page_map[i];
		if (src_page < 0)
			continue;

		dest_page = phy_blk_node_dest->last_write + 1;
		if((aml_nftl_info->copy_page(aml_nftl_info, dest_blk, dest_page, src_blk, src_page) == 2) && (retry_cnt++ <3))
		{
		       //aml_nftl_wl->add_free(aml_nftl_wl, dest_blk);
		       aml_nftl_info->blk_mark_bad(aml_nftl_info, dest_blk);
		       aml_nftl_dbg("%s: nftl write page faile blk: %d page: %d, retry_cnt:%d\n", __func__, dest_blk, dest_page, retry_cnt);
                       goto WRITE_RETRY;
		}
	}

	aml_nftl_wl->add_free(aml_nftl_wl, src_blk);
	vt_blk_node->phy_blk_addr = dest_blk;
	return 0;
}

static void add_gc(struct aml_nftl_wl_t* aml_nftl_wl, addr_blk_t gc_blk_addr)
{
	struct gc_blk_list *gc_add_list, *gc_cur_list, *gc_prev_list = NULL;
	struct list_head *l, *n;

	if (gc_blk_addr < NFTL_FAT_TABLE_NUM)
		return;

	if (!list_empty(&aml_nftl_wl->gc_blk_list)) {
		list_for_each_safe(l, n, &aml_nftl_wl->gc_blk_list) {
			gc_cur_list = list_to_gc_node(l);
			if (gc_cur_list->gc_blk_addr == gc_blk_addr)
				return;
			else if (gc_blk_addr < gc_cur_list->gc_blk_addr) {
				gc_cur_list = gc_prev_list;
				break;
			}
			else
				gc_prev_list = gc_cur_list;
		}
	}
	else {
		gc_cur_list = NULL;
	}

	gc_add_list = aml_nftl_malloc(sizeof(struct gc_blk_list));
	if (!gc_add_list)
		return;
	gc_add_list->gc_blk_addr = gc_blk_addr;

	if (gc_cur_list != NULL)
		list_add(&gc_add_list->list, &gc_cur_list->list);
	else
		list_add(&gc_add_list->list, &aml_nftl_wl->gc_blk_list);
	return;
}

static int gc_get_dirty_block(struct aml_nftl_wl_t* aml_nftl_wl, uint8_t gc_flag)
{
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_wl->aml_nftl_info;
	struct gc_blk_list *gc_cur_list;
	struct list_head *l, *n;
	struct vtblk_node_t  *vt_blk_node;
	struct phyblk_node_t *phy_blk_node;
	addr_blk_t dest_blk;
	int16_t vt_blk_num, start_free_num;
	int node_length, valid_page, k;

	start_free_num = aml_nftl_wl->free_root.count;
	if (!list_empty(&aml_nftl_wl->gc_blk_list)) {
		list_for_each_safe(l, n, &aml_nftl_wl->gc_blk_list) {
			gc_cur_list = list_to_gc_node(l);
			vt_blk_num = gc_cur_list->gc_blk_addr;
			if (vt_blk_num == aml_nftl_info->current_write_block)
				continue;
			if ((vt_blk_num >= aml_nftl_info->current_write_block - MAX_BLK_NUM_PER_NODE)
				&& (vt_blk_num <= aml_nftl_info->current_write_block + MAX_BLK_NUM_PER_NODE)
				&& (gc_flag != DO_COPY_PAGE))
				continue;

			vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
			node_length = aml_nftl_get_node_length(aml_nftl_info, vt_blk_node);
			if (node_length < BASIC_BLK_NUM_PER_NODE) {
				list_del(&gc_cur_list->list);
				aml_nftl_free(gc_cur_list);
				continue;
			}

			aml_nftl_check_node(aml_nftl_info, vt_blk_num);
			vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
			node_length = aml_nftl_get_node_length(aml_nftl_info, vt_blk_node);
			if (node_length < BASIC_BLK_NUM_PER_NODE) {
				list_del(&gc_cur_list->list);
				aml_nftl_free(gc_cur_list);
				continue;
			}

			if (aml_nftl_wl->free_root.count <= AML_LIMIT_FACTOR) {
				vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
				dest_blk = vt_blk_node->phy_blk_addr;
				phy_blk_node = &aml_nftl_info->phypmt[dest_blk];
				aml_nftl_info->get_phy_sect_map(aml_nftl_info, dest_blk);
				valid_page = 0;
				for (k=0; k<aml_nftl_wl->pages_per_blk; k++) {
					if (phy_blk_node->phy_page_map[k] >= 0)
						valid_page++;
				}
				if ((valid_page < (phy_blk_node->last_write + 1)) && (gc_flag != DO_COPY_PAGE))
					continue;
			}

			aml_nftl_wl->wait_gc_block = vt_blk_num;
			list_del(&gc_cur_list->list);
			aml_nftl_free(gc_cur_list);
			break;
		}
	}
	else {
		for (vt_blk_num=aml_nftl_info->accessibleblocks - 1; vt_blk_num>=0; vt_blk_num--) {

			vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk_num));
			if (vt_blk_node == NULL)
				continue;

			node_length = aml_nftl_get_node_length(aml_nftl_info, vt_blk_node);
			if (node_length == 1) {
				if(aml_nftl_wl->cur_delta >= aml_nftl_wl->wl_delta)
					staticwl_linear_blk(aml_nftl_wl, vt_blk_num);

				continue;
			}

			aml_nftl_wl->add_gc(aml_nftl_wl, vt_blk_num);
		}
	}

	return (aml_nftl_wl->free_root.count - start_free_num);
}

/**
 * gc_copy_special - copy root, leaf, sroot, sleaf to free block
 *
 * Copy all block to new free block, regardless valid sectors in each blocks
 *
 *     1. malloc special root, leaf sector map for temp use
 *     2. get best free linear block
 *     3. get root, leaf, sroot, sleaf linear block relative to special node
 *     4. get sector map for root, leaf, sroot, sleaf
 *     5. calculate page addr base
 *     6. copy_page depend on sector map, do not encode/decode ga_pmap
 *     7. add root, leaf, sroot, sleaf linear block into free tree
 *         add_free will not process invalid block(if no sleaf in this vblk)
 *     8. free temp sroot, sleaf sector map
 */
static int32_t gc_copy_one(struct aml_nftl_wl_t* aml_nftl_wl, addr_blk_t vt_blk, uint8_t gc_flag)
{
	int gc_free = 0, node_length, node_length_cnt, k, writed_pages = 0, retry_cnt = 0;
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_wl->aml_nftl_info;
	struct phyblk_node_t *phy_blk_src_node, *phy_blk_node_dest;
	struct vtblk_node_t  *vt_blk_node, *vt_blk_node_free;
	addr_blk_t dest_blk, src_blk;
	addr_page_t dest_page, src_page;

	vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk));
	if (vt_blk_node == NULL)
		return -ENOMEM;
WRITE_RETRY:
	dest_blk = vt_blk_node->phy_blk_addr;
	phy_blk_node_dest = &aml_nftl_info->phypmt[dest_blk];
	node_length = aml_nftl_get_node_length(aml_nftl_info, vt_blk_node);

	for (k=0; k<aml_nftl_wl->pages_per_blk; k++) {

		if (phy_blk_node_dest->phy_page_map[k] >= 0)
			continue;

		node_length_cnt = 0;
		src_blk = -1;
		src_page = -1;
		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk));
		while (vt_blk_node != NULL) {

			node_length_cnt++;
			src_blk = vt_blk_node->phy_blk_addr;
			phy_blk_src_node = &aml_nftl_info->phypmt[src_blk];
			if ((phy_blk_src_node->phy_page_map[k] >= 0) && (phy_blk_src_node->phy_page_delete[k>>3] & (1 << (k % 8)))) {
				src_page = phy_blk_src_node->phy_page_map[k];
				break;
			}
			vt_blk_node = vt_blk_node->next;
		}
		if ((src_page < 0) || (src_blk < 0) || ((node_length_cnt < node_length) && (gc_flag == 0)))
			continue;

		dest_page = phy_blk_node_dest->last_write + 1;
		if((aml_nftl_info->copy_page(aml_nftl_info, dest_blk, dest_page, src_blk, src_page) == 2) && (retry_cnt++ < 3))  //write error
		{
			aml_nftl_dbg("%s, line %d, aml_nftl_badblock_handle failed blk: %d retry_cnt:%d\n", __func__, __LINE__, dest_blk, retry_cnt);
#if 1
			if(aml_nftl_badblock_handle(aml_nftl_info, dest_blk, vt_blk) >= 0) //sucess
				goto WRITE_RETRY;
			else
			{
				aml_nftl_dbg("%s, line %d, aml_nftl_badblock_handle failed blk: %d retry_cnt:%d\n", __func__, __LINE__, dest_blk, retry_cnt);
				return -ENOENT;
			}
#else
                        aml_nftl_dbg("%s line:%d, nftl write page faile blk: %d page: %d , retry_cnt:%d\n", __func__,  __LINE__, dest_blk, dest_page, retry_cnt);
                        //aml_nftl_wl->add_free(aml_nftl_wl, dest_blk);
                        aml_nftl_info->blk_mark_bad(aml_nftl_info, dest_blk);
                        if(aml_nftl_wl->get_best_free(aml_nftl_wl, &dest_blk))
                        {
                            aml_nftl_dbg("%s line:%d, nftl write page faile blk: %d page: %d , retry_cnt:%d\n", __func__,  __LINE__, dest_blk, dest_page, retry_cnt);
		            return -ENOENT;
		        }
		         goto WRITE_RETRY;
#endif
		}
		writed_pages++;
		if ((gc_flag == DO_COPY_PAGE_AVERAGELY) && (writed_pages >= aml_nftl_wl->page_copy_per_gc) && (k < (aml_nftl_wl->pages_per_blk - aml_nftl_wl->page_copy_per_gc)))
			goto exit;
	}

	node_length_cnt = 0;
	vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + vt_blk));
	while (vt_blk_node->next != NULL) {
		node_length_cnt++;
		vt_blk_node_free = vt_blk_node->next;
		if (((node_length_cnt == (node_length - 1)) && (gc_flag == 0))
			|| ((node_length_cnt > 0) && (gc_flag != 0))) {
			aml_nftl_wl->add_free(aml_nftl_wl, vt_blk_node_free->phy_blk_addr);
			vt_blk_node->next = vt_blk_node_free->next;
			aml_nftl_free(vt_blk_node_free);
			gc_free++;
			continue;
		}
		if (vt_blk_node->next != NULL)
			vt_blk_node = vt_blk_node->next;
	}

exit:
	return gc_free;
}

static int aml_nftl_garbage_collect(struct aml_nftl_wl_t *aml_nftl_wl, uint8_t gc_flag)
{
	struct vtblk_node_t  *vt_blk_node;
	struct phyblk_node_t *phy_blk_node;
	addr_blk_t dest_blk, vt_blk_num;
	int gc_num = 0, copy_page_bounce_num, copy_page_total_num, valid_page, k;
	struct aml_nftl_info_t *aml_nftl_info = aml_nftl_wl->aml_nftl_info;

	if ((aml_nftl_info->fillfactor / 4) >= AML_LIMIT_FACTOR)
		copy_page_total_num = aml_nftl_info->fillfactor / 4;
	else
		copy_page_total_num = AML_LIMIT_FACTOR;
	copy_page_bounce_num = copy_page_total_num * 2;

	if ((aml_nftl_wl->wait_gc_block < 0) && (aml_nftl_wl->free_root.count < copy_page_bounce_num)) {
		gc_get_dirty_block(aml_nftl_wl, 0);
		if (aml_nftl_wl->wait_gc_block < 0)
			gc_get_dirty_block(aml_nftl_wl, DO_COPY_PAGE);
		if ((gc_flag == 0) && (aml_nftl_wl->free_root.count >= copy_page_total_num))
			return 0;
	}
	if (aml_nftl_wl->wait_gc_block >= 0) {
		if ((aml_nftl_wl->free_root.count < copy_page_total_num) || (gc_flag == DO_COPY_PAGE))
			aml_nftl_wl->page_copy_per_gc = aml_nftl_wl->pages_per_blk;
		else if (aml_nftl_wl->free_root.count >= (copy_page_total_num + AML_LIMIT_FACTOR))
			aml_nftl_wl->page_copy_per_gc = 4;
		else
			aml_nftl_wl->page_copy_per_gc = 8;

		vt_blk_node = (struct vtblk_node_t *)(*(aml_nftl_info->vtpmt + aml_nftl_wl->wait_gc_block));
		dest_blk = vt_blk_node->phy_blk_addr;
		phy_blk_node = &aml_nftl_info->phypmt[dest_blk];
		aml_nftl_info->get_phy_sect_map(aml_nftl_info, dest_blk);
		valid_page = 0;
		for (k=0; k<aml_nftl_wl->pages_per_blk; k++) {
			if (phy_blk_node->phy_page_map[k] >= 0)
				valid_page++;
		}

		if (valid_page < (phy_blk_node->last_write + 1)) {
			if(aml_nftl_wl->get_best_free(aml_nftl_wl, &dest_blk)) {
				aml_nftl_dbg("nftl garbage couldn`t found free block: %d %d\n", aml_nftl_wl->wait_gc_block, aml_nftl_wl->free_root.count);
				vt_blk_num = aml_nftl_wl->wait_gc_block;
				aml_nftl_wl->wait_gc_block = BLOCK_INIT_VALUE;
				gc_get_dirty_block(aml_nftl_wl, 0);
				if (aml_nftl_wl->wait_gc_block < 0) {
					aml_nftl_dbg("nftl garbage couldn`t found copy block: %d %d\n", aml_nftl_wl->wait_gc_block, aml_nftl_wl->free_root.count);
					return 0;
				}
				gc_num = aml_nftl_wl->garbage_one(aml_nftl_wl, aml_nftl_wl->wait_gc_block, DO_COPY_PAGE_AVERAGELY);
				if (gc_num > 0)
					aml_nftl_wl->wait_gc_block = BLOCK_INIT_VALUE;
				aml_nftl_wl->wait_gc_block = vt_blk_num;
				if (aml_nftl_wl->get_best_free(aml_nftl_wl, &dest_blk))
				return 0;
			}

			aml_nftl_add_node(aml_nftl_info, aml_nftl_wl->wait_gc_block, dest_blk);
			aml_nftl_wl->add_used(aml_nftl_wl, dest_blk);
		}

		gc_num = aml_nftl_wl->garbage_one(aml_nftl_wl, aml_nftl_wl->wait_gc_block, DO_COPY_PAGE_AVERAGELY);
		if (gc_num > 0)
			aml_nftl_wl->wait_gc_block = BLOCK_INIT_VALUE;

		return gc_num;
	}

		return 0;
}

/**
 * get_best_free - get best free block
 * @block : linear block
 *
 *     1. get free block from erased free tree & remove node from tree
 *     2. get free block from dirty free tree & remove node from tree
 *     3.      update ec
 *     4.      erase dirty free block
 */
static int32_t get_best_free(struct aml_nftl_wl_t *wl, addr_blk_t* blk)
{
	int error = 0;
	struct rb_node* p;
	struct wl_rb_t* tnode;
	struct aml_nftl_info_t *aml_nftl_info = wl->aml_nftl_info;

get_free:
	if(wl->erased_root.count) {
		p = rb_first(&wl->erased_root.root);
		tnode = rb_entry(p, struct wl_rb_t, rb_node);
		*blk = tnode->blk;
		_del_erased(wl, tnode);
	}
	else if(wl->free_root.count) {
		p = rb_first(&wl->free_root.root);
		tnode = rb_entry(p, struct wl_rb_t, rb_node);
		*blk = tnode->blk;
		_del_free(wl, tnode);

		error = aml_nftl_info->erase_block(aml_nftl_info, *blk);
		if (error) {
			aml_nftl_info->blk_mark_bad(aml_nftl_info, *blk);
			goto get_free;
		}
	}
	else
		return -ENOENT;

	return 0;
}

int aml_nftl_wl_init(struct aml_nftl_info_t *aml_nftl_info)
{
	struct aml_nftl_wl_t *aml_nftl_wl = aml_nftl_malloc(sizeof(struct aml_nftl_wl_t));
	if(!aml_nftl_wl)
		return -1;

	aml_nftl_wl->aml_nftl_info = aml_nftl_info;
	aml_nftl_info->aml_nftl_wl = aml_nftl_wl;

	aml_nftl_wl->wl_delta = WL_DELTA;
	aml_nftl_wl->pages_per_blk = aml_nftl_info->pages_per_blk;
	aml_nftl_wl->gc_start_block = aml_nftl_info->accessibleblocks - 1;

	aml_nftl_wl->erased_root.root = RB_ROOT;
	aml_nftl_wl->free_root.root = RB_ROOT;
	aml_nftl_wl->used_root.root = RB_ROOT;

	INIT_LIST_HEAD(&aml_nftl_wl->gc_blk_list);
	INIT_LIST_HEAD(&aml_nftl_wl->readerr_head.list);
	aml_nftl_wl->readerr_head.vt_blk = BLOCK_INIT_VALUE;
	aml_nftl_wl->wait_gc_block = BLOCK_INIT_VALUE;

	/*init function pointer*/
	aml_nftl_wl->add_free = add_free;
	aml_nftl_wl->add_erased = add_erased;
	aml_nftl_wl->add_used = add_used;
	aml_nftl_wl->add_gc = add_gc;
	aml_nftl_wl->get_best_free = get_best_free;

	aml_nftl_wl->garbage_one = gc_copy_one;
	aml_nftl_wl->garbage_collect = aml_nftl_garbage_collect;

	return 0;
}

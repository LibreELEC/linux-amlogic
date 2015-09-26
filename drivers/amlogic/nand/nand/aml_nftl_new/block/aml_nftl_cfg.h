
#include "aml_nftl_type.h"

#define  PRINT aml_nftl_dbg

#define NFTL_DONT_CACHE_DATA                      0
#define SUPPORT_GC_READ_RECLAIM                   0
#define SUPPORT_WEAR_LEVELING                     0
#define NFTL_ERASE                                0

#define SUPPORT_FILL_BLOCK             0

#define PART_RESERVED_BLOCK_RATIO                 8
#define MIN_FREE_BLOCK_NUM                        6
#define GC_THRESHOLD_FREE_BLOCK_NUM               4

#define MIN_FREE_BLOCK                            2

#define GC_THRESHOLD_RATIO_NUMERATOR              2
#define GC_THRESHOLD_RATIO_DENOMINATOR            3

#define MAX_CACHE_WRITE_NUM  				      4

//for req sync flag, writting into flash immediately
#define NFTL_CACHE_FLUSH_SYNC                      0



extern uint32 aml_nftl_get_part_cap(void * _part);
extern void aml_nftl_set_part_test(void * _part,uint32 test);
extern void* aml_nftl_get_part_priv(void * _part);
extern void aml_nftl_add_part_total_write(void * _part);
extern void aml_nftl_add_part_total_read(void * _part);
extern uint16 aml_nftl_get_part_write_cache_nums(void * _part);


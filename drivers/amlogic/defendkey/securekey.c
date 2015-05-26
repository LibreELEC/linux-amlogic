#include <linux/types.h>
#include <linux/slab.h>

#ifdef CONFIG_MESON_TRUSTZONE
#include <mach/meson-secure.h>
#include <plat/io.h>
#endif

#include "../efuse/efuse_regs.h"

#include <polarssl/rsa.h>

extern ssize_t aml__efuse_read( char *buf, size_t count, loff_t *ppos );
extern ssize_t aml__efuse_write(const char *buf, size_t count, loff_t *ppos );
extern efuse_socchip_type_e efuse_get_socchip_type(void);

static efuse_socchip_type_e secure_chip_type(void)
{
	return efuse_get_socchip_type();
}

int aml_is_secure_set(void)
{
	int ret = -1;
	efuse_socchip_type_e type;
	type = secure_chip_type();
	switch(type)
	{
		case EFUSE_SOC_CHIP_M8BABY:
		case EFUSE_SOC_CHIP_M8:{
			unsigned int info1=0,info2=-1;
			loff_t pos=0;
			aml__efuse_read((char*)&info1,4,&pos);
			pos = 0;
			aml__efuse_read((char*)&info2,4,&pos);
			if(info1 == info2){
				ret = 0;
				if(info1 & (1<<7)){
					ret = 1;
					if(info1 & (1<<31)){
						ret++;
					}
				}
			}
		}
		break;
		default:
		break;
	}
	return ret;
}


void *boot_malloc (int size)
{
	return kzalloc(size, GFP_KERNEL);
}
void boot_free (void *ptr)
{
	kfree (ptr);
}

#define EFUSE_RSA_PUK_N            72
#define EFUSE_RSA_PUK_E            68

static int boot_rsa_read_puk ( rsa_context *ctx, int is2k )
{
	int size = is2k==1 ? 256 : 128;
	unsigned char buf[POLARSSL_MPI_MAX_SIZE];
	loff_t pos=EFUSE_RSA_PUK_N;
	//public key N
	aml__efuse_read((char*)&buf[0],size,&pos);
	mpi_read_binary( &ctx->N, buf, size );
	
	// public key E
	pos = EFUSE_RSA_PUK_E;
	aml__efuse_read((char*)&buf[0],4,&pos);
	mpi_read_binary( &ctx->E, buf, 4 );
	return is2k;
}

static int aml_kernelkey_verify(unsigned char *pSRC,int nLen)
{
#ifdef AMLOGIC_CHKBLK_ID
#undef AMLOGIC_CHKBLK_ID
#endif
#ifdef AMLOGIC_CHKBLK_VER
#undef AMLOGIC_CHKBLK_VER
#endif
#define AMLOGIC_CHKBLK_ID  (0x434C4D41) //414D4C43 AMLC
#define AMLOGIC_CHKBLK_VER (1)

	typedef struct {
		unsigned int	nSizeH; 	   ////4@0
		unsigned int	nLength1;      ////4@4
		unsigned int	nLength2;      ////4@8
		unsigned int	nLength3;      ////4@12
		unsigned int	nLength4;      ////4@16
		unsigned char	szkey1[116];   ////116@20
		unsigned char	szkey2[108];   ////108@136
		unsigned int	nSizeT;        ////4@244
		unsigned int	nVer;          ////4@248
		unsigned int	unAMLID;       ////4@252
	}st_aml_chk_blk; //256

	typedef struct{
		int ver; int len;
		unsigned char szBuf1[12];
		unsigned char szBuf2[188];
	} st_crypto_blk1;


	int i;
	int nRet = -1;
	st_crypto_blk1 cb1_ctx;
	st_aml_chk_blk chk_blk;
	unsigned char szkey[32+16];
	unsigned char *pBuf = (unsigned char *)&chk_blk;
	unsigned int nState  = 0;
	loff_t pos=0;
#ifdef CONFIG_MESON_TRUSTZONE
	struct efuse_hal_api_arg arg;
	unsigned int retcnt;
#endif

	if(nLen & 0xF){
		printk("source data is not 16 byte aligned\n");
		goto exit;
	}

	rsa_init(&cb1_ctx,0,0);
	aml__efuse_read((char*)&nState,4,&pos);
	if(!(nState & (1<<7))){
		printk("the board is not encrypt\n");
		nRet = 0;
		goto exit;
	}

	cb1_ctx.len = (nState & (1<<23)) ? 256 : 128;

	memcpy((unsigned char*)&chk_blk,(unsigned char*)(pSRC+nLen-sizeof(chk_blk)),sizeof(chk_blk));

#ifndef CONFIG_MESON_TRUSTZONE
	boot_rsa_read_puk(&cb1_ctx,(nState & (1<<23)) ? 1 : 0);

	for(i = 0;i< sizeof(chk_blk);i+=cb1_ctx.len){
		if(rsa_public(&cb1_ctx, pBuf+i, pBuf+i )){
			printk("rsa decrypt fail, %s:%d\n",__func__,__LINE__);
			goto exit;
		}
	}
#else
	arg.cmd=EFUSE_HAL_API_VERIFY_IMG;
	arg.size = sizeof(chk_blk);
	arg.buffer_phy=virt_to_phys(&chk_blk);
	arg.retcnt_phy=virt_to_phys(&retcnt);
	nRet = meson_trustzone_efuse(&arg);
	if (nRet) {
		printk("meson trustzone verify failed.\n");
		goto exit;
	}
#endif

	if(AMLOGIC_CHKBLK_ID != chk_blk.unAMLID ||
		AMLOGIC_CHKBLK_VER < chk_blk.nVer)
		goto exit;

	if(sizeof(st_aml_chk_blk) != chk_blk.nSizeH ||
		sizeof(st_aml_chk_blk) != chk_blk.nSizeT ||
		chk_blk.nSizeT != chk_blk.nSizeH)
		goto exit;
	sha2( pSRC,chk_blk.nLength3, szkey, 0 );
	
	nRet = memcmp(szkey,chk_blk.szkey1,32);
	if(nRet){
		nRet = -2;  //be encrypted, key is not same board
	}
	else{
		nRet = 1;  //be encrypted, key is same as board
	}

exit:
	return nRet;

#undef AMLOGIC_CHKBLK_ID
#undef AMLOGIC_CHKBLK_VER
}

int aml_img_key_check(unsigned char *pSRC,int nLen)
{
	int ret=-10;
	efuse_socchip_type_e type;
	type = secure_chip_type();
	switch(type){
		case EFUSE_SOC_CHIP_M8BABY:
		case EFUSE_SOC_CHIP_M8:
		ret = aml_kernelkey_verify(pSRC,nLen);
		break;
		default:
		printk("chip not support,%s:%d\n",__func__,__LINE__);
		break;
	}

	return ret;
}


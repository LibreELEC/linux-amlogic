/*
 * AMLOGIC DVB driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 */

#define ENABLE_DEMUX_DRIVER

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/fcntl.h>
#include <asm/irq.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/amlogic/amports/amstream.h>
#ifdef ARC_700
#include <asm/arch/am_regs.h>
#else
#include "c_stb_define.h"
#include "c_stb_regs_define.h"
#endif

/*#include <mach/mod_gate.h>*/
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/amlogic/amdsc.h>
#include <linux/string.h>
#include <linux/pinctrl/consumer.h>

#include <linux/reset.h>

#include "linux/amlogic/cpu_version.h"

#include "aml_dvb.h"
#include "aml_dvb_reg.h"

#define pr_dbg(fmt, args...)\
	do {\
		if (debug_dvb)\
			printk("DVB: %s: " fmt, __func__, ## args);\
	} while (0)
#define pr_error(fmt, args...) printk("DVB: %s: " fmt, __func__, ## args)
#define pr_inf(fmt, args...)   printk("DVB: %s: " fmt, __func__, ## args)

MODULE_PARM_DESC(debug_dvb, "\n\t\t Enable dvb debug information");
static int debug_dvb;
module_param(debug_dvb, int, 0644);

#define CARD_NAME "amlogic-dvb"

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

static struct aml_dvb aml_dvb_device;
static struct reset_control *aml_dvb_demux_reset_ctl;
static struct reset_control *aml_dvb_afifo_reset_ctl;
static struct reset_control *aml_dvb_ahbarb0_reset_ctl;
static struct reset_control *aml_dvb_uparsertop_reset_ctl;

static int aml_tsdemux_reset(void);
static int aml_tsdemux_set_reset_flag(void);
static int aml_tsdemux_request_irq(irq_handler_t handler, void *data);
static int aml_tsdemux_free_irq(void);
static int aml_tsdemux_set_vid(int vpid);
static int aml_tsdemux_set_aid(int apid);
static int aml_tsdemux_set_sid(int spid);
static int aml_tsdemux_set_pcrid(int pcrpid);
static int aml_tsdemux_set_skipbyte(int skipbyte);
static int aml_tsdemux_set_demux(int id);

static struct tsdemux_ops aml_tsdemux_ops = {
.reset          = aml_tsdemux_reset,
.set_reset_flag = aml_tsdemux_set_reset_flag,
.request_irq    = aml_tsdemux_request_irq,
.free_irq       = aml_tsdemux_free_irq,
.set_vid        = aml_tsdemux_set_vid,
.set_aid        = aml_tsdemux_set_aid,
.set_sid        = aml_tsdemux_set_sid,
.set_pcrid      = aml_tsdemux_set_pcrid,
.set_skipbyte   = aml_tsdemux_set_skipbyte,
.set_demux      = aml_tsdemux_set_demux
};
static struct class   aml_stb_class;

static int control_ts_on_csi_port(int tsin, int enable)
{
//#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8
   if (is_meson_m8_cpu() || is_meson_m8b_cpu() || is_meson_m8m2_cpu()) {
	unsigned int temp_data;
	if(tsin==2 && enable) {
		//TS2 is on CSI port.
		//power on mipi csi phy
		pr_error("power on mipi csi phy for TSIN2\n");
		WRITE_CBUS_REG(HHI_CSI_PHY_CNTL0,0xfdc1ff81);
		WRITE_CBUS_REG(HHI_CSI_PHY_CNTL1,0x3fffff);
		temp_data = READ_CBUS_REG(HHI_CSI_PHY_CNTL2);
		temp_data &= 0x7ff00000;
		temp_data |= 0x80000fc0;
		WRITE_CBUS_REG(HHI_CSI_PHY_CNTL2,temp_data);
	}
   }
//#endif
	return 0;
}

static void aml_dvb_dmx_release(struct aml_dvb *advb, struct aml_dmx *dmx)
{
	int i;

	dvb_net_release(&dmx->dvb_net);
	aml_dmx_hw_deinit(dmx);
	dmx->demux.dmx.close(&dmx->demux.dmx);
	dmx->demux.dmx.remove_frontend(&dmx->demux.dmx, &dmx->mem_fe);

	for (i=0; i<DMX_DEV_COUNT; i++) {
		dmx->demux.dmx.remove_frontend(&dmx->demux.dmx, &dmx->hw_fe[i]);
	}

	dvb_dmxdev_release(&dmx->dmxdev);
	dvb_dmx_release(&dmx->demux);
}

static int aml_dvb_dmx_init(struct aml_dvb *advb, struct aml_dmx *dmx, int id)
{
	int i, ret;
#ifndef CONFIG_OF
	struct resource *res;
	char buf[32];
#endif
	switch(id){
		case 0:
			dmx->dmx_irq = INT_DEMUX;
			break;
		case 1:
			dmx->dmx_irq = INT_DEMUX_1;
			break;
		case 2:
			dmx->dmx_irq = INT_DEMUX_2;
			break;
	}

#ifndef CONFIG_OF
	snprintf(buf, sizeof(buf), "demux%d_irq", id);
	res = platform_get_resource_byname(advb->pdev, IORESOURCE_IRQ, buf);
	if (res) {
		dmx->dmx_irq = res->start;
	}
#endif
	pr_dbg("demux%d_irq: %d\n", id, dmx->dmx_irq);

	dmx->source  = 0;
	dmx->dump_ts_select = 0;
	dmx->dvr_irq = -1;

	dmx->demux.dmx.capabilities 	= (DMX_TS_FILTERING | DMX_SECTION_FILTERING | DMX_MEMORY_BASED_FILTERING);
	dmx->demux.filternum 		= dmx->demux.feednum = FILTER_COUNT;
	dmx->demux.priv 		= advb;
	dmx->demux.start_feed 		= aml_dmx_hw_start_feed;
	dmx->demux.stop_feed 		= aml_dmx_hw_stop_feed;
	dmx->demux.write_to_decoder 	= NULL;

	if ((ret = dvb_dmx_init(&dmx->demux)) < 0) {
		pr_error("dvb_dmx failed: error %d\n",ret);
		goto error_dmx_init;
	}

	dmx->dmxdev.filternum = dmx->demux.feednum;
	dmx->dmxdev.demux = &dmx->demux.dmx;
	dmx->dmxdev.capabilities = 0;
	if ((ret = dvb_dmxdev_init(&dmx->dmxdev, &advb->dvb_adapter)) < 0) {
		pr_error("dvb_dmxdev_init failed: error %d\n",ret);
		goto error_dmxdev_init;
	}

	for (i=0; i<DMX_DEV_COUNT; i++) {
		int source = i+DMX_FRONTEND_0;
		dmx->hw_fe[i].source = source;
	pr_dbg("demux%d: source:%d\n", i, source);

		if ((ret = dmx->demux.dmx.add_frontend(&dmx->demux.dmx, &dmx->hw_fe[i])) < 0) {
			pr_error("adding hw_frontend to dmx failed: error %d",ret);
			dmx->hw_fe[i].source = 0;
			goto error_add_hw_fe;
		}
	}

	dmx->mem_fe.source = DMX_MEMORY_FE;
	if ((ret = dmx->demux.dmx.add_frontend(&dmx->demux.dmx, &dmx->mem_fe)) < 0) {
		pr_error("adding mem_frontend to dmx failed: error %d",ret);
		goto error_add_mem_fe;
	}

	if ((ret = dmx->demux.dmx.connect_frontend(&dmx->demux.dmx, &dmx->hw_fe[1])) < 0) {
		pr_error("connect frontend failed: error %d",ret);
		goto error_connect_fe;
	}

	dmx->id = id;
	dmx->aud_chan = -1;
	dmx->vid_chan = -1;
	dmx->sub_chan = -1;
	dmx->pcr_chan = -1;
	dmx->smallsec.bufsize   = SS_BUFSIZE_DEF;
	dmx->smallsec.enable    = 0;
	dmx->smallsec.dmx       = dmx;
	dmx->timeout.dmx        =  dmx;
	dmx->timeout.enable     = 1;
	dmx->timeout.timeout    = DTO_TIMEOUT_DEF;
	dmx->timeout.ch_disable = DTO_CHDIS_VAS;
	dmx->timeout.match      = 1;
	dmx->timeout.trigger    = 0;

	if ((ret = aml_dmx_hw_init(dmx)) <0) {
		pr_error("demux hw init error %d", ret);
		dmx->id = -1;
		goto error_dmx_hw_init;
	}
pr_dbg("demux%d \n", id);

	dvb_net_init(&advb->dvb_adapter, &dmx->dvb_net, &dmx->demux.dmx);

	return 0;
error_dmx_hw_init:
error_connect_fe:
	dmx->demux.dmx.remove_frontend(&dmx->demux.dmx, &dmx->mem_fe);
error_add_mem_fe:
error_add_hw_fe:
	for (i=0; i<DMX_DEV_COUNT; i++) {
		if (dmx->hw_fe[i].source)
			dmx->demux.dmx.remove_frontend(&dmx->demux.dmx, &dmx->hw_fe[i]);
	}
	dvb_dmxdev_release(&dmx->dmxdev);
error_dmxdev_init:
	dvb_dmx_release(&dmx->demux);
error_dmx_init:
	return ret;
}
struct aml_dvb* aml_get_dvb_device(void)
{
	return &aml_dvb_device;
}

EXPORT_SYMBOL(aml_get_dvb_device);


static int aml_dvb_asyncfifo_init(struct aml_dvb *advb, struct aml_asyncfifo *asyncfifo, int id)
{
#ifndef CONFIG_OF
	struct resource *res;
	char buf[32];
#endif

	if(id == 0)
		asyncfifo->asyncfifo_irq = INT_ASYNC_FIFO_FLUSH;
	else
		asyncfifo->asyncfifo_irq = INT_ASYNC_FIFO2_FLUSH;

#ifndef CONFIG_OF
	snprintf(buf, sizeof(buf), "dvr%d_irq", id);
	res = platform_get_resource_byname(advb->pdev, IORESOURCE_IRQ, buf);
	if (res) {
		asyncfifo->asyncfifo_irq = res->start;
	}
#endif

	asyncfifo->dvb = advb;
	asyncfifo->id = id;
	asyncfifo->init = 0;
	asyncfifo->flush_size = 256*1024;

	return aml_asyncfifo_hw_init(asyncfifo);
}


static void aml_dvb_asyncfifo_release(struct aml_dvb *advb, struct aml_asyncfifo *asyncfifo)
{
	aml_asyncfifo_hw_deinit(asyncfifo);
}


/*Show the STB input source*/
static ssize_t stb_show_source(struct class *class, struct class_attribute *attr,char *buf)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	ssize_t ret = 0;
	char *src;

	switch(dvb->stb_source) {
		case AM_TS_SRC_TS0:
		case AM_TS_SRC_S_TS0:
			src = "ts0";
		break;
		case AM_TS_SRC_TS1:
		case AM_TS_SRC_S_TS1:
			src = "ts1";
		break;
		case AM_TS_SRC_TS2:
		case AM_TS_SRC_S_TS2:
			src = "ts2";
		break;
		case AM_TS_SRC_HIU:
			src = "hiu";
		break;
		case AM_TS_SRC_DMX0:
			src = "dmx0";
		break;
		case AM_TS_SRC_DMX1:
			src = "dmx1";
		break;
		case AM_TS_SRC_DMX2:
			src = "dmx2";
		break;
		default:
			src = "disable";
		break;
	}

	ret = sprintf(buf, "%s\n", src);
	return ret;
}



/*Set the STB input source*/
static ssize_t stb_store_source(struct class *class,struct class_attribute *attr, const char *buf, size_t size)
{
    dmx_source_t src = -1;
    if(!strncmp("ts0", buf, 3)) {
    	src = DMX_SOURCE_FRONT0;
    } else if(!strncmp("ts1", buf, 3)) {
    	src = DMX_SOURCE_FRONT1;
    } else if(!strncmp("ts2", buf, 3)) {
    	src = DMX_SOURCE_FRONT2;
    } else if(!strncmp("hiu", buf, 3)) {
    	src = DMX_SOURCE_DVR0;
    } else if(!strncmp("dmx0", buf, 4)) {
        src = DMX_SOURCE_FRONT0+100;
    } else if(!strncmp("dmx1", buf, 4)) {
        src = DMX_SOURCE_FRONT1+100;
    } else if(!strncmp("dmx2", buf, 4)) {
        src = DMX_SOURCE_FRONT2+100;
    }
    if(src!=-1) {
    	aml_stb_hw_set_source(&aml_dvb_device, src);
    }
    return size;
}

/*Show the descrambler's input source*/
static ssize_t dsc_show_source(struct class *class,struct class_attribute *attr, char *buf)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	ssize_t ret = 0;
	char *src;

	switch(dvb->dsc_source+7) {
		case AM_TS_SRC_DMX0:
			src = "dmx0";
		break;
		case AM_TS_SRC_DMX1:
			src = "dmx1";
		break;
		case AM_TS_SRC_DMX2:
			src = "dmx2";
		break;
		default:
			src = "bypass";
		break;
	}

	ret = sprintf(buf, "%s\n", src);
	return ret;
}



/*Set the descrambler's input source*/
static ssize_t dsc_store_source(struct class *class, struct class_attribute *attr,
                          const char *buf,
                          size_t size)
{
	dmx_source_t src = -1;

	if(!strncmp("dmx0", buf, 4)) {
		src = DMX_SOURCE_FRONT0; 	//	src = DMX_SOURCE_FRONT0+100;
	} else if(!strncmp("dmx1", buf, 4)) {
		src = DMX_SOURCE_FRONT1;	//	src = DMX_SOURCE_FRONT1+100;
	} else if(!strncmp("dmx2", buf, 4)) {
		src = DMX_SOURCE_FRONT2;	//	src = DMX_SOURCE_FRONT2+100;
	}
	aml_dsc_hw_set_source(&aml_dvb_device, src);

	return size;
}


/*Show free descramblers count*/
static ssize_t dsc_show_free_dscs(struct class *class,
				  struct class_attribute *attr,
				  char *buf)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	int fid, count;
	ssize_t ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&dvb->slock, flags);
	count = 0;
	for(fid = 0; fid < DSC_COUNT; fid++){
		if(!dvb->dsc[fid].used)
			count++;
	}
	spin_unlock_irqrestore(&dvb->slock, flags);

	ret = sprintf(buf, "%d\n", count);
	return ret;
}


/*Show the TS output source*/
static ssize_t tso_show_source(struct class *class, struct class_attribute *attr,char *buf)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	ssize_t ret = 0;
	char *src;

	switch(dvb->tso_source) {
		case AM_TS_SRC_TS0:
		case AM_TS_SRC_S_TS0:
			src = "ts0";
		break;
		case AM_TS_SRC_TS1:
		case AM_TS_SRC_S_TS1:
			src = "ts1";
		break;
		case AM_TS_SRC_TS2:
		case AM_TS_SRC_S_TS2:
			src = "ts2";
		break;
		case AM_TS_SRC_HIU:
			src = "hiu";
		break;
		case AM_TS_SRC_DMX0:
			src = "dmx0";
		break;
		case AM_TS_SRC_DMX1:
			src = "dmx1";
		break;
		case AM_TS_SRC_DMX2:
			src = "dmx2";
		break;
		default:
			src = "default";
		break;
	}

	ret = sprintf(buf, "%s\n", src);
	return ret;
}



/*Set the TS output source*/
static ssize_t tso_store_source(struct class *class,struct class_attribute *attr,
                          const char *buf,
                          size_t size)
{
    dmx_source_t src = -1;

    if(!strncmp("ts0", buf, 3)) {
    	src = DMX_SOURCE_FRONT0;
    } else if(!strncmp("ts1", buf, 3)) {
    	src = DMX_SOURCE_FRONT1;
    } else if(!strncmp("ts2", buf, 3)) {
    	src = DMX_SOURCE_FRONT2;
    } else if(!strncmp("hiu", buf, 3)) {
    	src = DMX_SOURCE_DVR0;
    } else if(!strncmp("dmx0", buf, 4)) {
        src = DMX_SOURCE_FRONT0+100;
    } else if(!strncmp("dmx1", buf, 4)) {
        src = DMX_SOURCE_FRONT1+100;
    } else if(!strncmp("dmx2", buf, 4)) {
        src = DMX_SOURCE_FRONT2+100;
    }

	aml_tso_hw_set_source(&aml_dvb_device, src);

    return size;
}

/*Show PCR*/
#define DEMUX_PCR_FUNC_DECL(i)  \
static ssize_t demux##i##_show_pcr(struct class *class,  struct class_attribute *attr,char *buf)\
{\
	int f = 0;\
	if(i == 0)\
		f = READ_MPEG_REG(PCR_DEMUX);\
	else if(i==1)\
		f = READ_MPEG_REG(PCR_DEMUX_2);\
	else if(i==2)\
		f = READ_MPEG_REG(PCR_DEMUX_3);\
	return sprintf(buf, "%08x\n", f);\
}

/*Show the STB input source*/
#define DEMUX_SOURCE_FUNC_DECL(i)  \
static ssize_t demux##i##_show_source(struct class *class,  struct class_attribute *attr,char *buf)\
{\
	struct aml_dvb *dvb = &aml_dvb_device;\
	struct aml_dmx *dmx = &dvb->dmx[i];\
	ssize_t ret = 0;\
	char *src;\
	switch(dmx->source) {\
		case AM_TS_SRC_TS0:\
		case AM_TS_SRC_S_TS0:\
			src = "ts0";\
		break;\
		case AM_TS_SRC_TS1:\
		case AM_TS_SRC_S_TS1:\
			src = "ts1";\
		break;\
		case AM_TS_SRC_TS2:\
		case AM_TS_SRC_S_TS2:\
			src = "ts2";\
		break;\
		case AM_TS_SRC_HIU:\
			src = "hiu";\
		break;\
		default:\
			src = "";\
		break;\
	}\
	ret = sprintf(buf, "%s\n", src);\
	return ret;\
}\
static ssize_t demux##i##_store_source(struct class *class,  struct class_attribute *attr,const char *buf, size_t size)\
{\
    dmx_source_t src = -1;\
	if(!strncmp("ts0", buf, 3)) {\
    	src = DMX_SOURCE_FRONT0;\
    } else if(!strncmp("ts1", buf, 3)) {\
    	src = DMX_SOURCE_FRONT1;\
    } else if(!strncmp("ts2", buf, 3)) {\
    	src = DMX_SOURCE_FRONT2;\
    } else if(!strncmp("hiu", buf, 3)) {\
    	src = DMX_SOURCE_DVR0;\
    }\
    if(src!=-1) {\
    	aml_dmx_hw_set_source(aml_dvb_device.dmx[i].dmxdev.demux, src);\
    }\
    return size;\
}

/*Show free filters count*/
#define DEMUX_FREE_FILTERS_FUNC_DECL(i)  \
static ssize_t demux##i##_show_free_filters(struct class *class,  \
				struct class_attribute *attr, char *buf)\
{\
	struct aml_dvb *dvb = &aml_dvb_device;\
	struct dvb_demux *dmx = &dvb->dmx[i].demux;\
	int fid, count;\
	ssize_t ret = 0;\
	if (mutex_lock_interruptible(&dmx->mutex)) \
		return -ERESTARTSYS; \
	count = 0;\
	for (fid = 0; fid < dmx->filternum; fid++) {\
		if (!dmx->filter[fid].state != DMX_STATE_FREE)\
			count++;\
	} \
	mutex_unlock(&dmx->mutex);\
	ret = sprintf(buf, "%d\n", count);\
	return ret;\
}

/*Show filter users count*/
#define DEMUX_FILTER_USERS_FUNC_DECL(i)  \
static ssize_t demux##i##_show_filter_users(struct class *class,  struct class_attribute *attr,char *buf)\
{\
	struct aml_dvb *dvb = &aml_dvb_device;\
	struct aml_dmx *dmx = &dvb->dmx[i];\
	int dmxdevfid, count;\
	ssize_t ret = 0;\
	unsigned long flags;\
	spin_lock_irqsave(&dvb->slock, flags);\
	count = 0;\
	for (dmxdevfid = 0; dmxdevfid < dmx->dmxdev.filternum; dmxdevfid++) {\
		if (dmx->dmxdev.filter[dmxdevfid].state >= DMXDEV_STATE_ALLOCATED)\
			count++;\
	}\
	if (count > dmx->demux_filter_user) {\
		count = dmx->demux_filter_user;\
	} else{\
		dmx->demux_filter_user = count;\
	}\
	spin_unlock_irqrestore(&dvb->slock, flags);\
	ret = sprintf(buf, "%d\n", count);\
	return ret;\
}\
static ssize_t demux##i##_store_filter_used(struct class *class,  struct class_attribute *attr,const char *buf, size_t size)\
{\
	struct aml_dvb *dvb = &aml_dvb_device;\
	struct aml_dmx *dmx = &dvb->dmx[i];\
	unsigned long filter_used;\
	unsigned long flags;/*char *endp;*/\
	/*filter_used = simple_strtol(buf, &endp, 0);*/\
	int ret = kstrtol(buf, 0, &filter_used);\
	spin_lock_irqsave(&dvb->slock, flags);\
	if (ret == 0 && filter_used) {\
		if(dmx->demux_filter_user < FILTER_COUNT)\
			dmx->demux_filter_user++;\
	}else{\
		if(dmx->demux_filter_user > 0)\
			dmx->demux_filter_user--;\
	}\
	spin_unlock_irqrestore(&dvb->slock, flags);\
	return size;\
}


/*Show ts header*/
#define DEMUX_TS_HEADER_FUNC_DECL(i)  \
static ssize_t demux##i##_show_ts_header(struct class *class,  struct class_attribute *attr,char *buf)\
{\
	int hdr = 0;\
	if(i == 0)\
		hdr = READ_MPEG_REG(TS_HEAD_1);\
	else if(i==1)\
		hdr = READ_MPEG_REG(TS_HEAD_1_2);\
	else if(i==2)\
		hdr = READ_MPEG_REG(TS_HEAD_1_3);\
	return sprintf(buf, "%08x\n", hdr);\
}

/*Show channel activity*/
#define DEMUX_CHANNEL_ACTIVITY_FUNC_DECL(i)  \
static ssize_t demux##i##_show_channel_activity(struct class *class,  struct class_attribute *attr,char *buf)\
{\
	int f = 0;\
	if(i == 0)\
		f = READ_MPEG_REG(DEMUX_CHANNEL_ACTIVITY);\
	else if(i==1)\
		f = READ_MPEG_REG(DEMUX_CHANNEL_ACTIVITY_2);\
	else if(i==2)\
		f = READ_MPEG_REG(DEMUX_CHANNEL_ACTIVITY_3);\
	return sprintf(buf, "%08x\n", f);\
}

/*DVR record mode*/
#define DVR_MODE_FUNC_DECL(i)  \
static ssize_t dvr##i##_show_mode(struct class *class,  struct class_attribute *attr,char *buf)\
{\
	struct aml_dvb *dvb = &aml_dvb_device;\
	struct aml_dmx *dmx = &dvb->dmx[i];\
	ssize_t ret = 0;\
	char *mode;\
	if(dmx->dump_ts_select) {\
		mode = "ts";\
	}else{\
		mode = "pid";\
	}\
	ret = sprintf(buf, "%s\n", mode);\
	return ret;\
}\
static ssize_t dvr##i##_store_mode(struct class *class,  struct class_attribute *attr,const char *buf, size_t size)\
{\
    struct aml_dvb *dvb = &aml_dvb_device;\
    struct aml_dmx *dmx = &dvb->dmx[i];\
    int dump_ts_select = -1;\
    \
    if(!strncmp("pid", buf, 3) && dmx->dump_ts_select) {\
    	dump_ts_select = 0;\
    } else if(!strncmp("ts", buf, 2) && !dmx->dump_ts_select) {\
    	dump_ts_select = 1;\
    }\
    if(dump_ts_select!=-1) {\
    	aml_dmx_hw_set_dump_ts_select(aml_dvb_device.dmx[i].dmxdev.demux, dump_ts_select);\
    }\
    return size;\
}

#if DMX_DEV_COUNT>0
	DEMUX_PCR_FUNC_DECL(0)
	DEMUX_SOURCE_FUNC_DECL(0)
	DEMUX_FREE_FILTERS_FUNC_DECL(0)
	DEMUX_FILTER_USERS_FUNC_DECL(0)
	DVR_MODE_FUNC_DECL(0)
	DEMUX_TS_HEADER_FUNC_DECL(0)
	DEMUX_CHANNEL_ACTIVITY_FUNC_DECL(0)
#endif
#if DMX_DEV_COUNT>1
	DEMUX_PCR_FUNC_DECL(1)
	DEMUX_SOURCE_FUNC_DECL(1)
	DEMUX_FREE_FILTERS_FUNC_DECL(1)
	DEMUX_FILTER_USERS_FUNC_DECL(1)
	DVR_MODE_FUNC_DECL(1)
	DEMUX_TS_HEADER_FUNC_DECL(1)
	DEMUX_CHANNEL_ACTIVITY_FUNC_DECL(1)
#endif
#if DMX_DEV_COUNT>2
	DEMUX_PCR_FUNC_DECL(2)
	DEMUX_SOURCE_FUNC_DECL(2)
	DEMUX_FREE_FILTERS_FUNC_DECL(2)
	DEMUX_FILTER_USERS_FUNC_DECL(2)
	DVR_MODE_FUNC_DECL(2)
	DEMUX_TS_HEADER_FUNC_DECL(2)
	DEMUX_CHANNEL_ACTIVITY_FUNC_DECL(2)
#endif

/*Show the async fifo source*/
#define ASYNCFIFO_SOURCE_FUNC_DECL(i)  \
static ssize_t asyncfifo##i##_show_source(struct class *class,  struct class_attribute *attr,char *buf)\
{\
	struct aml_dvb *dvb = &aml_dvb_device;\
	struct aml_asyncfifo *afifo = &dvb->asyncfifo[i];\
	ssize_t ret = 0;\
	char *src;\
	switch(afifo->source) {\
		case AM_DMX_0:\
			src = "dmx0";\
		break;\
		case AM_DMX_1:\
			src = "dmx1";\
		break;\
		case AM_DMX_2:\
			src = "dmx2";\
		break;\
		default:\
			src = "";\
		break;\
	}\
	ret = sprintf(buf, "%s\n", src);\
	return ret;\
}\
static ssize_t asyncfifo##i##_store_source(struct class *class,  struct class_attribute *attr,const char *buf, size_t size)\
{\
    enum aml_dmx_id_t src = -1;\
	if(!strncmp("dmx0", buf, 4)) {\
    	src = AM_DMX_0;\
    } else if(!strncmp("dmx1", buf, 4)) {\
    	src = AM_DMX_1;\
    } else if(!strncmp("dmx2", buf, 4)) {\
    	src = AM_DMX_2;\
    }\
    if(src!=-1) {\
    	aml_asyncfifo_hw_set_source(&aml_dvb_device.asyncfifo[i], src);\
    }\
    return size;\
}

#if ASYNCFIFO_COUNT>0
	ASYNCFIFO_SOURCE_FUNC_DECL(0)
#endif
#if ASYNCFIFO_COUNT>1
	ASYNCFIFO_SOURCE_FUNC_DECL(1)
#endif

/*Show the async fifo flush size*/
#define ASYNCFIFO_FLUSHSIZE_FUNC_DECL(i)  \
static ssize_t asyncfifo##i##_show_flush_size(struct class *class,  \
				struct class_attribute *attr, char *buf)\
{\
	struct aml_dvb *dvb = &aml_dvb_device;\
	struct aml_asyncfifo *afifo = &dvb->asyncfifo[i];\
	ssize_t ret = 0;\
	ret = sprintf(buf, "%d\n", afifo->flush_size);\
	return ret;\
} \
static ssize_t asyncfifo##i##_store_flush_size(struct class *class,  \
					struct class_attribute *attr, \
					const char *buf, size_t size)\
{\
	struct aml_dvb *dvb = &aml_dvb_device;\
	struct aml_asyncfifo *afifo = &dvb->asyncfifo[i];\
	/*int fsize = simple_strtol(buf, NULL, 10);*/\
	int fsize = 0;\
	long value;\
	int ret = kstrtol(buf, 0, &value);\
	if (ret == 0)\
		fsize = value;\
	if (fsize != afifo->flush_size) {\
		afifo->flush_size = fsize;\
	aml_asyncfifo_hw_reset(&aml_dvb_device.asyncfifo[i]);\
	} \
	return size;\
}

#if ASYNCFIFO_COUNT > 0
ASYNCFIFO_FLUSHSIZE_FUNC_DECL(0)
#endif

#if ASYNCFIFO_COUNT > 1
	ASYNCFIFO_FLUSHSIZE_FUNC_DECL(1)
#endif
/*Reset the Demux*/
static ssize_t demux_do_reset(struct class *class,struct class_attribute *attr,
                          const char *buf,
                          size_t size)
{
	if(!strncmp("1", buf, 1)) {
		struct aml_dvb *dvb = &aml_dvb_device;
		unsigned long flags;

		spin_lock_irqsave(&dvb->slock, flags);
		pr_dbg("Reset demux, call dmx_reset_hw\n");
		dmx_reset_hw_ex(dvb, 0);
		spin_unlock_irqrestore(&dvb->slock, flags);
	}

	return size;
}





/*Show the Video PTS value*/
static ssize_t demux_show_video_pts(struct class *class,  struct class_attribute *attr,char *buf)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	ssize_t ret = 0;

	ret = sprintf(buf, "%u\n", aml_dmx_get_video_pts(dvb));

	return ret;
}


/*Show the Audio PTS value*/
static ssize_t demux_show_audio_pts(struct class *class,  struct class_attribute *attr,char *buf)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	ssize_t ret = 0;

	ret = sprintf(buf, "%u\n", aml_dmx_get_audio_pts(dvb));

	return ret;
}



/*Show the First Video PTS value*/
static ssize_t demux_show_first_video_pts(struct class *class,  struct class_attribute *attr,char *buf)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	ssize_t ret = 0;

	ret = sprintf(buf, "%u\n", aml_dmx_get_first_video_pts(dvb));

	return ret;
}



/*Show the First Audio PTS value*/
static ssize_t demux_show_first_audio_pts(struct class *class,  struct class_attribute *attr,char *buf)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	ssize_t ret = 0;

	ret = sprintf(buf, "%u\n", aml_dmx_get_first_audio_pts(dvb));

	return ret;
}


static ssize_t stb_show_hw_setting(struct class *class, struct class_attribute *attr, char *buf)
{
	int r, total = 0;
	int i;
	struct aml_dvb *dvb = &aml_dvb_device;
	int invert, ctrl;

	for(i=0; i<TS_IN_COUNT; i++){
		struct aml_ts_input *ts = &dvb->ts[i];

		if(ts->s2p_id != -1){
			invert = dvb->s2p[ts->s2p_id].invert;
		}else{
			invert = 0;
		}

		ctrl = ts->control;

		r = sprintf(buf, "ts%d %s control: 0x%x invert: 0x%x\n", i,
				ts->mode==AM_TS_DISABLE?"disable":(ts->mode==AM_TS_SERIAL?"serial":"parallel"),
				ctrl, invert);
		buf += r;
		total += r;
	}

	return total;
}



static ssize_t stb_store_hw_setting(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	int id, ctrl, invert, r, mode;
	char mname[32];
	char pname[32];
	unsigned long flags;
	struct aml_ts_input *ts;
	struct aml_dvb *dvb = &aml_dvb_device;

	r = sscanf(buf, "%d %s %x %x", &id, mname, &ctrl, &invert);
	if(r != 4)
		return -EINVAL;

	if(id < 0 || id >= TS_IN_COUNT)
		return -EINVAL;

	if((mname[0] == 's') || (mname[0] == 'S')){
		sprintf(pname, "s_ts%d", id);
		mode = AM_TS_SERIAL;
	}else if((mname[0] == 'p') || (mname[0] == 'P')){
		sprintf(pname, "p_ts%d", id);
		mode = AM_TS_PARALLEL;
	}else{
		mode = AM_TS_DISABLE;
	}

	spin_lock_irqsave(&dvb->slock, flags);

	ts = &dvb->ts[id];

	if((mode == AM_TS_SERIAL) && (ts->mode != AM_TS_SERIAL)){
		int i;
		int scnt = 0;

		for(i = 0; i < TS_IN_COUNT; i++){
			if(dvb->ts[i].s2p_id != -1){
				scnt++;
			}
		}

		if(scnt >= S2P_COUNT){
			pr_error("no free s2p\n");
		}else{
			ts->s2p_id = scnt;
		}
	}

	if((mode != AM_TS_SERIAL) || (ts->s2p_id != -1)){
		if(ts->pinctrl){
			devm_pinctrl_put(ts->pinctrl);
			ts->pinctrl = NULL;
		}

		ts->pinctrl  = devm_pinctrl_get_select(&dvb->pdev->dev, pname);
		ts->mode     = mode;
		ts->control  = ctrl;

		if(mode == AM_TS_SERIAL){
			dvb->s2p[ts->s2p_id].invert = invert;
		}else{
			ts->s2p_id = -1;
		}
	}

	spin_unlock_irqrestore(&dvb->slock, flags);

	return count;
}
#ifdef CONFIG_OF
static const struct of_device_id aml_dvb_dt_match[]={
	{
		.compatible = "amlogic,dvb",
	},

	{},
};
#endif /*CONFIG_OF*/

/*Get the STB source demux*/
static struct aml_dmx* get_stb_dmx(void)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx = NULL;
	int i;

	switch(dvb->stb_source){
		case AM_TS_SRC_DMX0:
			dmx = &dvb->dmx[0];
		break;
		case AM_TS_SRC_DMX1:
			dmx = &dvb->dmx[1];
		break;
		case AM_TS_SRC_DMX2:
			dmx = &dvb->dmx[2];
		break;
		default:
			for(i=0; i<DMX_DEV_COUNT; i++) {
				dmx = &dvb->dmx[i];
				if(dmx->source==dvb->stb_source) {
					return dmx;
				}
			}
		break;
	}

	return dmx;
}

#define DEMUX_RESET_FUNC_DECL(i)  \
static ssize_t demux##i##_reset_store(struct class *class,  struct class_attribute *attr,const char *buf, size_t size)\
{\
	if(!strncmp("1", buf, 1)) { \
		struct aml_dvb *dvb = &aml_dvb_device; \
		pr_info("Reset demux["#i"], call dmx_reset_dmx_hw\n"); \
		dmx_reset_dmx_id_hw_ex(dvb, i, 0); \
	} \
	return size; \
}
#if DMX_DEV_COUNT>0
	DEMUX_RESET_FUNC_DECL(0)
#endif
#if DMX_DEV_COUNT>1
	DEMUX_RESET_FUNC_DECL(1)
#endif
#if DMX_DEV_COUNT>2
	DEMUX_RESET_FUNC_DECL(2)
#endif

static struct class_attribute aml_stb_class_attrs[] = {
	__ATTR(hw_setting, S_IRUGO|S_IWUSR, stb_show_hw_setting, stb_store_hw_setting),
	__ATTR(source,  S_IRUGO | S_IWUSR | S_IWGRP, stb_show_source, stb_store_source),
	__ATTR(dsc_source,  S_IRUGO | S_IWUSR, dsc_show_source, dsc_store_source),
	__ATTR(tso_source,  S_IRUGO | S_IWUSR, tso_show_source, tso_store_source),
#define DEMUX_SOURCE_ATTR_PCR(i)\
		__ATTR(demux##i##_pcr,  S_IRUGO | S_IWUSR, demux##i##_show_pcr, NULL)
#define DEMUX_SOURCE_ATTR_DECL(i)\
		__ATTR(demux##i##_source,  S_IRUGO | S_IWUSR | S_IWGRP, demux##i##_show_source, demux##i##_store_source)
#define DEMUX_FREE_FILTERS_ATTR_DECL(i)\
		__ATTR(demux##i##_free_filters,  S_IRUGO | S_IWUSR, demux##i##_show_free_filters, NULL)
#define DEMUX_FILTER_USERS_ATTR_DECL(i)\
		__ATTR(demux##i##_filter_users,  S_IRUGO | S_IWUSR, demux##i##_show_filter_users, demux##i##_store_filter_used)
#define DVR_MODE_ATTR_DECL(i)\
		__ATTR(dvr##i##_mode,  S_IRUGO | S_IWUSR, dvr##i##_show_mode, dvr##i##_store_mode)
#define DEMUX_TS_HEADER_ATTR_DECL(i)\
		__ATTR(demux##i##_ts_header,  S_IRUGO | S_IWUSR, demux##i##_show_ts_header, NULL)
#define DEMUX_CHANNEL_ACTIVITY_ATTR_DECL(i)\
		__ATTR(demux##i##_channel_activity,  S_IRUGO | S_IWUSR, demux##i##_show_channel_activity, NULL)
#define DMX_RESET_ATTR_DECL(i)\
		__ATTR(demux##i##_reset,  S_IRUGO | S_IWUSR, NULL, demux##i##_reset_store)
#if DMX_DEV_COUNT>0
	DEMUX_SOURCE_ATTR_PCR(0),
	DEMUX_SOURCE_ATTR_DECL(0),
	DEMUX_FREE_FILTERS_ATTR_DECL(0),
	DEMUX_FILTER_USERS_ATTR_DECL(0),
	DVR_MODE_ATTR_DECL(0),
	DEMUX_TS_HEADER_ATTR_DECL(0),
	DEMUX_CHANNEL_ACTIVITY_ATTR_DECL(0),
	DMX_RESET_ATTR_DECL(0),
#endif
#if DMX_DEV_COUNT>1
	DEMUX_SOURCE_ATTR_PCR(1),
	DEMUX_SOURCE_ATTR_DECL(1),
	DEMUX_FREE_FILTERS_ATTR_DECL(1),
	DEMUX_FILTER_USERS_ATTR_DECL(1),
	DVR_MODE_ATTR_DECL(1),
	DEMUX_TS_HEADER_ATTR_DECL(1),
	DEMUX_CHANNEL_ACTIVITY_ATTR_DECL(1),
	DMX_RESET_ATTR_DECL(1),
#endif
#if DMX_DEV_COUNT>2
	DEMUX_SOURCE_ATTR_PCR(2),
	DEMUX_SOURCE_ATTR_DECL(2),
	DEMUX_FREE_FILTERS_ATTR_DECL(2),
	DEMUX_FILTER_USERS_ATTR_DECL(2),
	DVR_MODE_ATTR_DECL(2),
	DEMUX_TS_HEADER_ATTR_DECL(2),
	DEMUX_CHANNEL_ACTIVITY_ATTR_DECL(2),
	DMX_RESET_ATTR_DECL(2),
#endif
#define ASYNCFIFO_SOURCE_ATTR_DECL(i)\
		__ATTR(asyncfifo##i##_source,  S_IRUGO | S_IWUSR | S_IWGRP, asyncfifo##i##_show_source, asyncfifo##i##_store_source)
#define ASYNCFIFO_FLUSHSIZE_ATTR_DECL(i)\
		__ATTR(asyncfifo##i##_flush_size,  S_IRUGO | S_IWUSR | S_IWGRP, asyncfifo##i##_show_flush_size, asyncfifo##i##_store_flush_size)
#if ASYNCFIFO_COUNT>0
	ASYNCFIFO_SOURCE_ATTR_DECL(0),
	ASYNCFIFO_FLUSHSIZE_ATTR_DECL(0),
#endif
#if ASYNCFIFO_COUNT>1
	ASYNCFIFO_SOURCE_ATTR_DECL(1),
	ASYNCFIFO_FLUSHSIZE_ATTR_DECL(1),
#endif
	__ATTR(demux_reset,  S_IRUGO | S_IWUSR, NULL, demux_do_reset),
	__ATTR(video_pts,  S_IRUGO | S_IWUSR | S_IWGRP, demux_show_video_pts, NULL),
	__ATTR(audio_pts,  S_IRUGO | S_IWUSR | S_IWGRP, demux_show_audio_pts, NULL),
	__ATTR(first_video_pts,  S_IRUGO | S_IWUSR, demux_show_first_video_pts, NULL),
	__ATTR(first_audio_pts,  S_IRUGO | S_IWUSR, demux_show_first_audio_pts, NULL),
	__ATTR(free_dscs,  S_IRUGO | S_IWUSR, dsc_show_free_dscs, NULL),
	__ATTR_NULL
};

static struct class aml_stb_class = {
	.name = "stb",
	.class_attrs = aml_stb_class_attrs,
};

extern int aml_regist_dmx_class(void);
extern int aml_unregist_dmx_class(void);

static int aml_dvb_probe(struct platform_device *pdev)
{
	struct aml_dvb *advb;
	int i, ret = 0;
	struct devio_aml_platform_data *pd_dvb;

	pr_inf("probe amlogic dvb driver\n");

	/*switch_mod_gate_by_name("demux", 1); */

	aml_dvb_demux_reset_ctl =
		devm_reset_control_get(&pdev->dev, "demux");
	pr_inf("dmx rst ctl = %p\n", aml_dvb_demux_reset_ctl);
	reset_control_deassert(aml_dvb_demux_reset_ctl);

	aml_dvb_afifo_reset_ctl =
		devm_reset_control_get(&pdev->dev, "asyncfifo");
	pr_inf("asyncfifo rst ctl = %p\n", aml_dvb_afifo_reset_ctl);
	reset_control_deassert(aml_dvb_afifo_reset_ctl);

	aml_dvb_ahbarb0_reset_ctl =
		devm_reset_control_get(&pdev->dev, "ahbarb0");
	pr_inf("ahbarb0 rst ctl = %p\n", aml_dvb_ahbarb0_reset_ctl);
	reset_control_deassert(aml_dvb_ahbarb0_reset_ctl);

	aml_dvb_uparsertop_reset_ctl =
		devm_reset_control_get(&pdev->dev, "uparsertop");
	pr_inf("uparsertop rst ctl = %p\n", aml_dvb_uparsertop_reset_ctl);
	reset_control_deassert(aml_dvb_uparsertop_reset_ctl);

	advb = &aml_dvb_device;
	memset(advb, 0, sizeof(aml_dvb_device));

	spin_lock_init(&advb->slock);

	advb->dev = &pdev->dev;
	advb->pdev = pdev;
	advb->stb_source = -1;
	advb->dsc_source = AM_TS_SRC_S_TS0;
	advb->tso_source = -1;

	for (i = 0; i < DMX_DEV_COUNT; i++) {
		advb->dmx[i].dmx_irq = -1;
		advb->dmx[i].dvr_irq = -1;
	}

#ifdef CONFIG_OF
	if(pdev->dev.of_node){
		int s2p_id = 0;

		char buf[32];
		const char *str;
		u32 value;
		for (i=0; i<TS_IN_COUNT; i++){

			advb->ts[i].mode   = AM_TS_DISABLE;
			advb->ts[i].s2p_id = -1;

			snprintf(buf, sizeof(buf), "ts%d", i);
			ret = of_property_read_string(pdev->dev.of_node, buf, &str);
			if(!ret){
				if(!strcmp(str, "serial")){
					pr_dbg("%s: serial\n", buf);

					if(s2p_id >= S2P_COUNT){
						pr_error("no free s2p\n");
					}else{
						snprintf(buf, sizeof(buf), "s_ts%d", i);
						advb->ts[i].mode    = AM_TS_SERIAL;
						advb->ts[i].pinctrl = devm_pinctrl_get_select(&pdev->dev, buf);
						advb->ts[i].s2p_id  = s2p_id;

						s2p_id++;
					}
				}else if(!strcmp(str, "parallel")){
					pr_dbg("%s: parallel\n", buf);
					snprintf(buf, sizeof(buf), "p_ts%d", i);
					advb->ts[i].mode    = AM_TS_PARALLEL;
					advb->ts[i].pinctrl = devm_pinctrl_get_select(&pdev->dev, buf);
				}else{
					advb->ts[i].mode    = AM_TS_DISABLE;
					advb->ts[i].pinctrl = NULL;
				}

		//		if(IS_ERR_VALUE(advb->ts[i].pinctrl))
		//			advb->ts[i].pinctrl = NULL;

				control_ts_on_csi_port(i, (advb->ts[i].mode == AM_TS_DISABLE)? 0 : 1);
			}

			snprintf(buf, sizeof(buf), "ts%d_control", i);
			ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
			if(!ret){
				pr_dbg("%s: 0x%x\n", buf, value);
				advb->ts[i].control = value;
			}

			if(advb->ts[i].s2p_id != -1){
				snprintf(buf, sizeof(buf), "ts%d_invert", i);
				ret = of_property_read_u32(pdev->dev.of_node, buf, &value);
				if(!ret){
					pr_dbg("%s: 0x%x\n", buf, value);
					advb->s2p[advb->ts[i].s2p_id].invert = value;
				}
			}
		}
		snprintf(buf, sizeof(buf), "ts_out_invert");
		ret =
			of_property_read_u32(pdev->dev.of_node, buf,
				&value);
		if (!ret) {
				pr_inf("%s: 0x%x\n", buf, value);
				advb->ts_out_invert = value;
		}
	}
#endif
	pd_dvb = (struct devio_aml_platform_data *)advb->dev->platform_data;

	ret =
	    dvb_register_adapter(&advb->dvb_adapter, CARD_NAME, THIS_MODULE,
				 advb->dev, adapter_nr);
	if (ret < 0)
		return ret;
	pr_inf("Registered adpter: %s\n", CARD_NAME);

	for (i = 0; i < DMX_DEV_COUNT; i++)
		advb->dmx[i].id = -1;

	advb->dvb_adapter.priv = advb;
	dev_set_drvdata(advb->dev, advb);

	for (i = 0; i < DMX_DEV_COUNT; i++) {
		ret = aml_dvb_dmx_init(advb, &advb->dmx[i], i);
		if (ret < 0)
			goto error;
	}
	/*Init the async fifos */
	for (i = 0; i < ASYNCFIFO_COUNT; i++) {
		ret = aml_dvb_asyncfifo_init(advb, &advb->asyncfifo[i], i);
		if (ret < 0)
			goto error;

	}
	aml_asyncfifo_hw_set_source(&advb->asyncfifo[0], AM_DMX_0);

	aml_regist_dmx_class();

	if (class_register(&aml_stb_class) < 0) {
		pr_error("register class error\n");
		goto error;
	}

	tsdemux_set_ops(&aml_tsdemux_ops);

	return ret;
error:
	for (i = 0; i < ASYNCFIFO_COUNT; i++) {
		if (advb->asyncfifo[i].id != -1)
			aml_dvb_asyncfifo_release(advb, &advb->asyncfifo[i]);
	}

	for (i = 0; i < DMX_DEV_COUNT; i++) {
		if (advb->dmx[i].id != -1)
			aml_dvb_dmx_release(advb, &advb->dmx[i]);
	}

	dvb_unregister_adapter(&advb->dvb_adapter);

	return ret;
}

static int aml_dvb_remove(struct platform_device *pdev)
{
	struct aml_dvb *advb = (struct aml_dvb *)dev_get_drvdata(&pdev->dev);
	int i;

	tsdemux_set_ops(NULL);

	aml_unregist_dmx_class();
	class_unregister(&aml_stb_class);


	for (i = 0; i < DMX_DEV_COUNT; i++)
		aml_dvb_dmx_release(advb, &advb->dmx[i]);

	dvb_unregister_adapter(&advb->dvb_adapter);

	for (i = 0; i < TS_IN_COUNT; i++) {
		if (advb->ts[i].pinctrl)
			devm_pinctrl_put(advb->ts[i].pinctrl);
	}

	/*switch_mod_gate_by_name("demux", 0); */
	reset_control_assert(aml_dvb_uparsertop_reset_ctl);
	reset_control_assert(aml_dvb_ahbarb0_reset_ctl);
	reset_control_assert(aml_dvb_afifo_reset_ctl);					
	reset_control_assert(aml_dvb_demux_reset_ctl);

	return 0;
}

static struct platform_driver aml_dvb_driver = {
	.probe		= aml_dvb_probe,
	.remove		= aml_dvb_remove,
	.driver		= {
		.name	= "amlogic-dvb",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = aml_dvb_dt_match,
#endif
	}
};

int __init aml_dvb_init(void)
{
	pr_dbg("aml dvb init\n");
	return platform_driver_register(&aml_dvb_driver);
}

void __exit aml_dvb_exit(void)
{
	pr_dbg("aml dvb exit\n");
	platform_driver_unregister(&aml_dvb_driver);
}

static int aml_tsdemux_reset(void)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	unsigned long flags;

	spin_lock_irqsave(&dvb->slock, flags);
	if(dvb->reset_flag) {
		struct aml_dmx *dmx = get_stb_dmx();
		dvb->reset_flag = 0;
		if(dmx)
			dmx_reset_dmx_hw_ex_unlock(dvb, dmx, 0);
	}
	spin_unlock_irqrestore(&dvb->slock, flags);

	return 0;
}

static int aml_tsdemux_set_reset_flag(void)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	unsigned long flags;

	spin_lock_irqsave(&dvb->slock, flags);
	dvb->reset_flag = 1;
	spin_unlock_irqrestore(&dvb->slock, flags);

	return 0;

}

/*Add the amstream irq handler*/
static int aml_tsdemux_request_irq(irq_handler_t handler, void *data)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	unsigned long flags;

	spin_lock_irqsave(&dvb->slock, flags);

	dmx = get_stb_dmx();
	if(dmx) {
		dmx->irq_handler = handler;
		dmx->irq_data = data;
	}

	spin_unlock_irqrestore(&dvb->slock, flags);

	return 0;
}

/*Free the amstream irq handler*/
static int aml_tsdemux_free_irq(void)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	unsigned long flags;

	spin_lock_irqsave(&dvb->slock, flags);

	dmx = get_stb_dmx();
	if(dmx) {
		dmx->irq_handler = NULL;
		dmx->irq_data = NULL;
	}

	spin_unlock_irqrestore(&dvb->slock, flags);

	return 0;
}

/*Reset the video PID*/
static int aml_tsdemux_set_vid(int vpid)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&dvb->slock, flags);

	dmx = get_stb_dmx();

	spin_unlock_irqrestore(&dvb->slock, flags);

	if(dmx) {
		mutex_lock(&dmx->dmxdev.mutex);

		spin_lock_irqsave(&dvb->slock, flags);

		if(dmx->vid_chan!=-1) {
			dmx_free_chan(dmx, dmx->vid_chan);
			dmx->vid_chan = -1;
		}

		if((vpid>=0) && (vpid<0x1FFF)) {
			dmx->vid_chan = dmx_alloc_chan(dmx, DMX_TYPE_TS, DMX_PES_VIDEO, vpid);
			if(dmx->vid_chan==-1) {
				ret = -1;
			}
		}

		spin_unlock_irqrestore(&dvb->slock, flags);

		mutex_unlock(&dmx->dmxdev.mutex);
	}

	return ret;
}

/*Reset the audio PID*/
static int aml_tsdemux_set_aid(int apid)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&dvb->slock, flags);

	dmx = get_stb_dmx();

	spin_unlock_irqrestore(&dvb->slock, flags);

	if(dmx) {
		mutex_lock(&dmx->dmxdev.mutex);

		spin_lock_irqsave(&dvb->slock, flags);

		if(dmx->aud_chan!=-1) {
			dmx_free_chan(dmx, dmx->aud_chan);
			dmx->aud_chan = -1;
		}

		if((apid>=0) && (apid<0x1FFF)) {
			dmx->aud_chan = dmx_alloc_chan(dmx, DMX_TYPE_TS, DMX_PES_AUDIO, apid);
			if(dmx->aud_chan==-1) {
				ret = -1;
			}
		}

		spin_unlock_irqrestore(&dvb->slock, flags);

		mutex_unlock(&dmx->dmxdev.mutex);
	}

	return ret;
}

/*Reset the subtitle PID*/
static int aml_tsdemux_set_sid(int spid)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&dvb->slock, flags);

	dmx = get_stb_dmx();

	spin_unlock_irqrestore(&dvb->slock, flags);

	if(dmx) {
		mutex_lock(&dmx->dmxdev.mutex);

		spin_lock_irqsave(&dvb->slock, flags);

		if(dmx->sub_chan!=-1) {
			dmx_free_chan(dmx, dmx->sub_chan);
			dmx->sub_chan = -1;
		}

		if((spid>=0) && (spid<0x1FFF)) {
			dmx->sub_chan = dmx_alloc_chan(dmx, DMX_TYPE_TS, DMX_PES_SUBTITLE, spid);
			if(dmx->sub_chan==-1) {
				ret = -1;
			}
		}

		spin_unlock_irqrestore(&dvb->slock, flags);

		mutex_unlock(&dmx->dmxdev.mutex);
	}

	return ret;
}

static int aml_tsdemux_set_pcrid(int pcrpid)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	struct aml_dmx *dmx;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&dvb->slock, flags);

	dmx = get_stb_dmx();

	spin_unlock_irqrestore(&dvb->slock, flags);

	if(dmx) {
		mutex_lock(&dmx->dmxdev.mutex);

		spin_lock_irqsave(&dvb->slock, flags);

		if(dmx->pcr_chan!=-1) {
			dmx_free_chan(dmx, dmx->pcr_chan);
			dmx->pcr_chan = -1;
		}

		if((pcrpid>=0) && (pcrpid<0x1FFF)) {
			dmx->pcr_chan = dmx_alloc_chan(dmx, DMX_TYPE_TS, DMX_PES_PCR, pcrpid);
			if(dmx->pcr_chan==-1) {
				ret = -1;
			}
		}

		spin_unlock_irqrestore(&dvb->slock, flags);

		mutex_unlock(&dmx->dmxdev.mutex);
	}

	return ret;
}

static int aml_tsdemux_set_skipbyte(int skipbyte)
{
	struct aml_dvb *dvb = &aml_dvb_device;
	unsigned long flags;

	spin_lock_irqsave(&dvb->slock, flags);
	aml_dmx_set_skipbyte(dvb, skipbyte);
	spin_unlock_irqrestore(&dvb->slock, flags);

	return 0;
}

static int aml_tsdemux_set_demux(int id)
{
	struct aml_dvb *dvb = &aml_dvb_device;

	aml_dmx_set_demux(dvb, id);
	return 0;
}

module_init(aml_dvb_init);
module_exit(aml_dvb_exit);

MODULE_DESCRIPTION("driver for the AMLogic DVB card");
MODULE_AUTHOR("AMLOGIC");
MODULE_LICENSE("GPL");



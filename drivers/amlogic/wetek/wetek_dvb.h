#ifndef _WETEK_DVB_H_
#define _WETEK_DVB_H_

#include <linux/interrupt.h>
#include <linux/socket.h>
#include <linux/netdevice.h>
#include <linux/i2c.h>

#include <linux/dvb/video.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/ca.h>
#include <linux/dvb/osd.h>
#include <linux/dvb/net.h>
#include <linux/dvb/frontend.h>

#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif


#include "dvbdev.h"
#include "demux.h"
#include "dvb_demux.h"
#include "dmxdev.h"
#include "dvb_filter.h"
#include "dvb_net.h"
#include "dvb_ringbuffer.h"
#include "dvb_frontend.h"

#include <linux/of.h>
#include <linux/pinctrl/consumer.h>

#define TS_IN_COUNT       3
#define S2P_COUNT         2

#define DMX_DEV_COUNT     3
#define FE_DEV_COUNT      2
#define CHANNEL_COUNT     31
#define FILTER_COUNT      31
#define FILTER_LEN        15
#define DSC_DEV_COUNT     1
#define DSC_COUNT         8
#define SEC_BUF_GRP_COUNT 4
#define SEC_BUF_BUSY_SIZE 4
#define SEC_BUF_COUNT     (SEC_BUF_GRP_COUNT*8)
#define ASYNCFIFO_COUNT   2


enum wetek_dmx_id_t {
	AM_DMX_0 = 0,
	AM_DMX_1,
	AM_DMX_2,
	AM_DMX_MAX,
};

enum wetek_ts_source_t {
	AM_TS_SRC_TS0,
	AM_TS_SRC_TS1,
	AM_TS_SRC_TS2,
	AM_TS_SRC_S_TS0,
	AM_TS_SRC_S_TS1,
	AM_TS_SRC_S_TS2,
	AM_TS_SRC_HIU,
	AM_TS_SRC_DMX0,
	AM_TS_SRC_DMX1,
	AM_TS_SRC_DMX2
};

struct wetek_sec_buf {
	unsigned long        addr;
	int                  len;
};

struct wetek_channel {
	int                  type;
	enum dmx_ts_pes	pes_type;
	int                  pid;
	int                  used;
	int                  filter_count;
	struct dvb_demux_feed     *feed;
	struct dvb_demux_feed     *dvr_feed;
};

struct wetek_filter {
	int                  chan_id;
	int                  used;
	struct dmx_section_filter *filter;
	u8                   value[FILTER_LEN];
	u8                   maskandmode[FILTER_LEN];
	u8                   maskandnotmode[FILTER_LEN];
	u8                   neq;
};

struct wetek_dsc {
	int                  pid;
	u8                   even[8];
	u8                   odd[8];
	int                  used;
	int                  set;
	int                  id;
	struct wetek_dvb      *dvb;
};

struct wetek_smallsec {
	struct wetek_dmx *dmx;

	int	enable;
	int	bufsize;
#define SS_BUFSIZE_DEF (16*4*256) /*16KB*/
	long	buf;
	long	buf_map;
};

struct wetek_dmxtimeout {
	struct wetek_dmx *dmx;

	int	enable;

	int	timeout;
#define DTO_TIMEOUT_DEF (9000)       /*0.5s*/
	u32	ch_disable;
#define DTO_CHDIS_VAS   (0xfffffff8) /*v/a/s only*/
	int	match;

	int     trigger;
};

struct wetek_dmx {
	struct dvb_demux     demux;
	struct dmxdev        dmxdev;
	int                  id;
	int                  feed_count;
	int                  chan_count;
	enum wetek_ts_source_t      source;
	int                  init;
	int                  record;
	struct dmx_frontend  hw_fe[DMX_DEV_COUNT];
	struct dmx_frontend  mem_fe;
	struct dvb_net       dvb_net;
	int                  dmx_irq;
	int                  dvr_irq;
	struct tasklet_struct     dmx_tasklet;
	struct tasklet_struct     dvr_tasklet;
	unsigned long        sec_pages;
	unsigned long        sec_pages_map;
	int                  sec_total_len;
	struct wetek_sec_buf   sec_buf[SEC_BUF_COUNT];
	unsigned long        pes_pages;
	unsigned long        pes_pages_map;
	int                  pes_buf_len;
	unsigned long        sub_pages;
	unsigned long        sub_pages_map;
	int                  sub_buf_len;
	struct wetek_channel   channel[CHANNEL_COUNT];
	struct wetek_filter    filter[FILTER_COUNT];
	irq_handler_t        irq_handler;
	void                *irq_data;
	int                  aud_chan;
	int                  vid_chan;
	int                  sub_chan;
	int                  pcr_chan;
	u32                  section_busy[SEC_BUF_BUSY_SIZE];
	struct dvb_frontend *fe;
	int                  int_check_count;
	u32                  int_check_time;
	int                  in_tune;
	int                  error_check;
	int                  dump_ts_select;
	int                  sec_buf_watchdog_count[SEC_BUF_COUNT];

	struct wetek_smallsec  smallsec;
	struct wetek_dmxtimeout timeout;

	int                  demux_filter_user;
};

struct wetek_asyncfifo {
	int	id;
	int	init;
	int	asyncfifo_irq;
	enum wetek_dmx_id_t	source;
	unsigned long	pages;
	unsigned long   pages_map;
	int	buf_len;
	int	buf_toggle;
	int buf_read;
	int flush_size;
	struct tasklet_struct     asyncfifo_tasklet;
	struct wetek_dvb *dvb;
};

enum{
	AM_TS_DISABLE,
	AM_TS_PARALLEL,
	AM_TS_SERIAL
};

struct wetek_ts_input {
	int                  mode;
	struct pinctrl      *pinctrl;
	int                  control;
	int                  s2p_id;
};

struct wetek_s2p {
	int    invert;
};

struct wetek_swfilter {
	int    user;
	struct wetek_dmx *dmx;
	struct wetek_asyncfifo *afifo;

	struct dvb_ringbuffer rbuf;
#define SF_BUFFER_SIZE (10*188*1024)

	u8     wrapbuf[188];
	int    track_dmx;
};

struct wetek_dvb {
	struct dvb_device    dvb_dev;
	struct wetek_ts_input  ts[TS_IN_COUNT];
	struct wetek_s2p       s2p[S2P_COUNT];
	struct wetek_dmx       dmx[DMX_DEV_COUNT];
	struct wetek_dsc       dsc[DSC_COUNT];
	struct wetek_asyncfifo asyncfifo[ASYNCFIFO_COUNT];
	struct dvb_device   *dsc_dev;
	struct dvb_adapter   dvb_adapter;
	struct device       *dev;
	struct platform_device *pdev;
	enum wetek_ts_source_t      stb_source;
	enum wetek_ts_source_t      dsc_source;
	enum wetek_ts_source_t      tso_source;
	int                  dmx_init;
	int                  reset_flag;
	spinlock_t           slock;
	struct timer_list    watchdog_timer;
	int                  dmx_watchdog_disable[DMX_DEV_COUNT];
	struct wetek_swfilter  swfilter;
	int		     ts_out_invert;
};


/*AMLogic demux interface*/
extern int wetek_dmx_hw_init(struct wetek_dmx *dmx);
extern int wetek_dmx_hw_deinit(struct wetek_dmx *dmx);
extern int wetek_dmx_hw_start_feed(struct dvb_demux_feed *dvbdmxfeed);
extern int wetek_dmx_hw_stop_feed(struct dvb_demux_feed *dvbdmxfeed);
extern int wetek_dmx_hw_set_source(struct dmx_demux *demux, dmx_source_t src);
extern int wetek_stb_hw_set_source(struct wetek_dvb *dvb, dmx_source_t src);
extern int wetek_dsc_hw_set_source(struct wetek_dvb *dvb, dmx_source_t src);
extern int wetek_tso_hw_set_source(struct wetek_dvb *dvb, dmx_source_t src);

extern int wetek_dmx_set_skipbyte(struct wetek_dvb *dvb, int skipbyte);
extern int wetek_dmx_set_demux(struct wetek_dvb *dvb, int id);
extern int wetek_dmx_hw_set_dump_ts_select
		(struct dmx_demux *demux, int dump_ts_select);

extern int  dmx_alloc_chan(struct wetek_dmx *dmx, int type,
				int pes_type, int pid);
extern void dmx_free_chan(struct wetek_dmx *dmx, int cid);

extern int dmx_get_ts_serial(enum wetek_ts_source_t src);


/*AMLogic dsc interface*/
extern int dsc_set_pid(struct wetek_dsc *dsc, int pid);
extern int dsc_set_key(struct wetek_dsc *dsc, int type, u8 *key);
extern int dsc_release(struct wetek_dsc *dsc);

/*AMLogic ASYNC FIFO interface*/
extern int wetek_asyncfifo_hw_init(struct wetek_asyncfifo *afifo);
extern int wetek_asyncfifo_hw_deinit(struct wetek_asyncfifo *afifo);
extern int wetek_asyncfifo_hw_set_source(struct wetek_asyncfifo *afifo,
					enum wetek_dmx_id_t src);
extern int wetek_asyncfifo_hw_reset(struct wetek_asyncfifo *afifo);

/*Get the Audio & Video PTS*/
extern u32 wetek_dmx_get_video_pts(struct wetek_dvb *dvb);
extern u32 wetek_dmx_get_audio_pts(struct wetek_dvb *dvb);
extern u32 wetek_dmx_get_first_video_pts(struct wetek_dvb *dvb);
extern u32 wetek_dmx_get_first_audio_pts(struct wetek_dvb *dvb);

/*Get the DVB device*/
extern struct wetek_dvb *wetek_get_dvb_device(void);

/*Demod interface*/
extern void wetek_dmx_register_frontend(enum wetek_ts_source_t src,
					struct dvb_frontend *fe);
extern void wetek_dmx_before_retune(enum wetek_ts_source_t src,
					struct dvb_frontend *fe);
extern void wetek_dmx_after_retune(enum wetek_ts_source_t src,
					struct dvb_frontend *fe);
extern void wetek_dmx_start_error_check(enum wetek_ts_source_t src,
					struct dvb_frontend *fe);
extern int  wetek_dmx_stop_error_check(enum wetek_ts_source_t src,
					struct dvb_frontend *fe);
extern int wetek_regist_dmx_class(void);
extern int wetek_unregist_dmx_class(void);
extern void dvb_frontend_retune(struct dvb_frontend *fe);

struct devio_wetek_platform_data {
	int (*io_setup)(void *);
	int (*io_cleanup)(void *);
	int (*io_power)(void *, int enable);
	int (*io_reset)(void *, int enable);
};

/*Reset the demux device*/
void dmx_reset_hw(struct wetek_dvb *dvb);
void dmx_reset_hw_ex(struct wetek_dvb *dvb, int reset_irq);

/*Reset the individual demux*/
void dmx_reset_dmx_hw(struct wetek_dvb *dvb, int id);
void dmx_reset_dmx_id_hw_ex(struct wetek_dvb *dvb, int id, int reset_irq);
void dmx_reset_dmx_id_hw_ex_unlock(struct wetek_dvb *dvb, int id, int reset_irq);
void dmx_reset_dmx_hw_ex(struct wetek_dvb *dvb,
				struct wetek_dmx *dmx,
				int reset_irq);
void dmx_reset_dmx_hw_ex_unlock(struct wetek_dvb *dvb,
				struct wetek_dmx *dmx,
				int reset_irq);

#endif


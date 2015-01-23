#ifndef   _OSD_SYNC_H
#define _OSD_SYNC_H
#include  <sw_sync.h>
#include  <sync.h>

typedef  struct{
	unsigned int  xoffset;
	unsigned int  yoffset;
	int  in_fen_fd;
	int  out_fen_fd;
}fb_ext_sync_request_t;

typedef  struct{
	struct sw_sync_timeline  *timeline;
}osd_ext_sync_obj;

#endif

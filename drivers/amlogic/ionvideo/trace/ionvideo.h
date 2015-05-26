#undef TRACE_SYSTEM
#define TRACE_INCLUDE_PATH ../../drivers/amlogic/ionvideo/trace
#define TRACE_SYSTEM ionvideo

#if !defined(_TRACE_IONVIDEO_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_IONVIDEO_H

#include "../ionvideo.h"
#include <linux/tracepoint.h>

TRACE_EVENT(qdqbuf,
    TP_PROTO(int idx, int que, u64 ts),

    TP_ARGS(idx, que, ts),

    TP_STRUCT__entry(
            __field(u32, idx)
            __field(u32, que)
            __field(u64, ts)
    ),

    TP_fast_assign(
            __entry->idx = idx;
            __entry->que = que;
            __entry->ts = ts;
    ),

    TP_printk("%s: buf%d", __entry->que? "[Q]": "\t\t[D]",
            __entry->idx)
);

TRACE_EVENT(fillbuf,
    TP_PROTO(int idx, int begin, int vf_wait),

    TP_ARGS(idx, begin, vf_wait),

    TP_STRUCT__entry(
            __field(u32, idx)
            __field(u32, begin)
            __field(u32, vf_wait)
    ),

    TP_fast_assign(
            __entry->idx = idx;
            __entry->begin = begin;
            __entry->vf_wait = vf_wait;
    ),

    TP_printk("\t%s buf%d W:%d",
                    __entry->begin?"[>>]":"[<<]",
                    __entry->idx, __entry->vf_wait)
);

TRACE_EVENT(onoffbuf,
    TP_PROTO(int que),

    TP_ARGS(que),

    TP_STRUCT__entry(
            __field(u32, que)
    ),

    TP_fast_assign(
            __entry->que = que;
    ),

    TP_printk("\t\t @[%s]@", __entry->que? "StreamON": "SteamOFF")
);

#endif /* if !defined(_TRACE_SYNC_H) || defined(TRACE_HEADER_MULTI_READ) */

/* This part must be outside protection */
#include <trace/define_trace.h>


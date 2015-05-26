#undef TRACE_SYSTEM
#define TRACE_INCLUDE_PATH ../../drivers/amlogic/video_dev/trace
#define TRACE_SYSTEM amlvideo

#if !defined(_TRACE_IONVIDEO_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_IONVIDEO_H

#include <linux/tracepoint.h>

TRACE_EVENT(qbuf,
    TP_PROTO(int idx),

    TP_ARGS(idx),

    TP_STRUCT__entry(
            __field(u32, idx)
    ),

    TP_fast_assign(
            __entry->idx = idx;
    ),

    TP_printk("[Q]: buf%d", __entry->idx)
);

TRACE_EVENT(dqbuf,
    TP_PROTO(int idx, int wf, int wb, int wq),

    TP_ARGS(idx, wf, wb, wq),

    TP_STRUCT__entry(
            __field(u32, idx)
            __field(u32, wf)
            __field(u32, wb)
            __field(u32, wq)
    ),

    TP_fast_assign(
            __entry->idx = idx;
            __entry->wf = wf;
            __entry->wb = wb;
            __entry->wq = wq;
	),

    TP_printk("\t\t[D]: buf%d wf:%d, wb:%d, qlv:%d", __entry->idx,
        __entry->wf, __entry->wb, __entry->wq)
);

TRACE_EVENT(onoff,
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


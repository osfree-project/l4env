/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/includes/ARCH-x86/L4API-l4v2/ktrace_events.h
 * \brief   L4 kernel event tracing, event memory layout
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_KTRACE_EVENTS_H__
#define __L4_KTRACE_EVENTS_H__

#include <l4/sys/types.h>

enum
{
    l4_ktrace_tbuf_unused             = 0,
    l4_ktrace_tbuf_pf                 = 1,
    l4_ktrace_tbuf_ipc                = 2,
    l4_ktrace_tbuf_ipc_res            = 3,
    l4_ktrace_tbuf_ipc_trace          = 4,
    l4_ktrace_tbuf_ke                 = 5,
    l4_ktrace_tbuf_ke_reg             = 6,
    l4_ktrace_tbuf_unmap              = 7,
    l4_ktrace_tbuf_shortcut_failed    = 8,
    l4_ktrace_tbuf_shortcut_succeeded = 9,
    l4_ktrace_tbuf_context_switch     = 10,
    l4_ktrace_tbuf_exregs             = 11,
    l4_ktrace_tbuf_breakpoint         = 12,
    l4_ktrace_tbuf_trap               = 13,
    l4_ktrace_tbuf_pf_res             = 14,
    l4_ktrace_tbuf_sched              = 15,
    l4_ktrace_tbuf_preemption         = 16,
    l4_ktrace_tbuf_lipc               = 17,
    l4_ktrace_tbuf_jean1              = 18,
    l4_ktrace_tbuf_task_new           = 19,
    l4_ktrace_tbuf_max                = 32,
    l4_ktrace_tbuf_hidden             = 0x20,
};

// IA-32 typedefs...
typedef void               Context; // We don't have a class Context
                                    // here...  But that's fine, we
                                    // can calculate the taskid and
                                    // the threadid from this void
                                    // pointer...
typedef void               Space;
typedef void               Sched_context;
//typedef l4_threadid_t L4_threadid;
typedef l4_threadid_t      L4_uid;
typedef l4_fpage_t         L4_fpage;
typedef void *             Address;
typedef unsigned int       Mword;
typedef unsigned int       L4_snd_desc;
typedef unsigned int       L4_rcv_desc;
typedef l4_uint64_t        Global_id;
typedef l4_msgdope_t       L4_msgdope;
typedef unsigned int       L4_timeout;
typedef unsigned char      Unsigned8;
typedef unsigned short     Unsigned16;
typedef unsigned int       Unsigned32;
typedef unsigned long long Unsigned64;

#define TCB_MAGIC 0xC0000000

L4_INLINE unsigned l4_ktrace_get_l4_taskid(Context *context);
L4_INLINE unsigned l4_ktrace_get_l4_lthreadid(Context *context);
L4_INLINE l4_threadid_t l4_ktrace_get_l4_threadid(Context *context);

L4_INLINE unsigned l4_ktrace_get_l4_taskid(Context *context)
{
    return (((unsigned) context) - TCB_MAGIC) / 0x40000;
}

L4_INLINE unsigned l4_ktrace_get_l4_lthreadid(Context *context)
{
    return ((((unsigned) context) - TCB_MAGIC) / 0x800) & 127;
}

L4_INLINE l4_threadid_t l4_ktrace_get_l4_threadid(Context *context)
{
    l4_threadid_t id;
    id.raw     = 0;
    id.id.task    = l4_ktrace_get_l4_taskid(context);
    id.id.lthread = l4_ktrace_get_l4_lthreadid(context);
    return id;
}

typedef struct __attribute__((packed))
{
    Mword      number;                    // event number
    Mword      eip;                       // instruction pointer
    Context  * context;                   // current Context
    Unsigned64 tsc;                       // time stamp counter
    Unsigned32 pmc1;                      // performance counter 1
    Unsigned32 pmc2;                      // performance counter 2
    Unsigned32 kclock;                    // lower 32 bit of kernel clock
    Unsigned8  type;                      // type of entry
    union __attribute__((__packed__))     // event specific data
    {
        struct __attribute__((__packed__))
        {
            char    _pad[3];
            Address pfa;
            Mword   err;
        } pf;  // logged page fault
        struct __attribute__((__packed__))
        {
            char        _pad[3];
            L4_snd_desc snd_desc;
            L4_rcv_desc rcv_desc;
            Mword       dword[2];
            L4_uid      dest;
            L4_timeout  timeout;
        } ipc;  // logged ipc
        struct __attribute__((__packed__))
        {
            Unsigned8   have_sent;
            char        _pad[2];
            Mword       dword[2];
            L4_msgdope  result;
            L4_uid      rcv_src;
            L4_rcv_desc rcv_desc;
            Mword       pair_event;
        } ipc_res;  // logged ipc result
        struct __attribute__((__packed__))
        {
            Unsigned8  send_desc;
            Unsigned8  recv_desc;
            char       dummy;
            L4_msgdope result;
            Unsigned64 send_tsc;
            L4_uid     send_dest;
            L4_uid     recv_dest;
        } ipc_trace;  // traced ipc
        struct __attribute__((__packed__))
        {
            char msg[31];
        } ke;   // logged kernel event
        struct __attribute__((__packed__))
        {
            char msg[19];
            Mword eax;
            Mword ecx;
            Mword edx;
        } ke_reg;  // logged kernel event plus register content
        struct __attribute__((__packed__))
        {
            char     _pad[3];
            L4_fpage fpage;
            Mword    mask;
            int      result;  // was bool
        } unmap;  // logged unmap operation
        struct __attribute__((__packed__))
        {
            char         _pad[3];
            L4_snd_desc snd_desc;
            L4_rcv_desc rcv_desc;
            L4_timeout  timeout;
            L4_uid      dest;
            Unsigned8   is_irq;
            Unsigned8   snd_lst;
            Unsigned8   dst_ok;
            Unsigned8   dst_lck;
        } shortcut_failed;  // logged short-cut ipc failed
        struct __attribute__((__packed__))
        {
            // fixme: this seems unused in fiasco
        } shortcut_succeeded;  // logged short-cut ipc succeeded
        struct __attribute__((__packed__))
        {
            char            _pad[3];
            Context       * dest;
            Context       * dest_orig;
            Mword           kernel_ip;
            Mword           lock_cnt;
            Space         * from_space;
            Sched_context * from_sched;
            Mword           from_prio;
        } context_switch;  // logged context switch
        struct __attribute__((__packed__))
        {
            char    _pad[3];
            Mword   lthread;
            Mword   task;
            Address old_esp;
            Address new_esp;
            Address old_eip;
            Address new_eip;
            Mword   failed;
        } exregs;  // logged thread_ex_regs operation
        struct __attribute__((__packed__))
        {
            char    _pad[3];
            Address addr;
            int     len;
            Mword   value;
            int     mode;
        } breakpoint;  // logged breakpoint
        struct __attribute__((__packed__))
        {
            char       trapno;
            Unsigned16 _errno;
            Mword      ebp;
            Mword      edx;
            Mword      cr2;
            Mword      eax;
            Mword      eflags;
            Mword      esp;
            Unsigned16 cs;
            Unsigned16 ds;
        } trap;  // logged trap
        struct __attribute__((__packed__))
        {
            char _pad[3];
            Mword pfa;
            L4_msgdope err;
            L4_msgdope ret;
        } pf_res;  // logged page fault result
        struct __attribute__((__packed__))
        {
            char           _pad[1];
            unsigned short mode;
            Context 	 * owner;
            unsigned short id;
            unsigned short prio;
            signed long	   left;
            unsigned long  quantum;
        } sched;
        struct __attribute__((__packed__))
        {
            Context * preempter;
        } preemption;
        struct __attribute__((__packed__))
        {
            short int type;
            Global_id _old;
            Global_id _new;
            Address   c_utcb_ptr;
        } lipc;
        struct __attribute__((__packed__))
        {
            Context * sched_owner1;
            Context * sched_owner2;
        } jean1;
        struct __attribute__((__packed__))
        {
            char      _pad[3];
            Global_id task;
            Global_id pager;
            Address   new_esp;
            Address   new_eip;
            Mword     mcp_or_chief;
        } task_new;
        struct __attribute__((__packed__))
        {
            char _pad[31];
        } fit;
    } m;
} l4_tracebuffer_entry_t;

#endif

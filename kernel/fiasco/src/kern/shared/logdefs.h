#ifndef LOGDEFS_H
#define LOGDEFS_H

// ### How to create a tracebuffer entry ###
//
// If you only need a temporary debugging aid then you can use one of
// the standard kernel logging events: 
// 
//   LOG_MSG(Context *context, const char *msg)
//     - context is something like context_of(this) or current_context()
//       or 0 if there is no context available
//     - msg should be displayed in the tracebuffer view
//
//   LOG_MSG_3VAL(Context *context, const char *msg,
//                Mword val1, Mword val2, Mword val3)
//     - context and msg can be used the same way LOG_MSG does
//     - val1, val2, and val3 are values that will be displayed in
//       the tracebuffer view as hexadecimal values
//
// If you want to create a permanent log event xyz, you have to follow
// these instructions:
// - create enum Log_event_xyz (see jdb_ktrace.cpp)
// - create class Tb_entry_xyz derived from Tb_entry (see tb_entry.cpp)
//   with an appropriate ::set method and with accessor methods
// - create function formatter_xyz (see tb_entry_output.cpp) and don't
//   forget to register it (add an appropriate line to init_formatters)
// - create macro LOG_XYZ (see the following example)
//      #define LOG_XYZ
//        BEGIN_LOG_EVENT(log_xyz)
//        Lock_guard <Cpu_lock> guard (&cpu_lock);
//        Tb_entry_xyz *tb =
//           static_cast<Tb_entry_ctx_sw*>(Jdb_tbuf::new_entry());
//        tb->set (this, <some log specific variables>)
//        Jdb_tbuf::commit_entry();
//        END_LOG_EVENT
//   (grabbing the cpu_lock isn't necessary if it is still grabbed)
// - create an empty macro declaration for CONFIG_JDB_LOGGING=n
// - insert the macro call into the code
// - WARNING: permanent log events should _not_ be placed into an inline
//            function!
// - add
//     DECLARE_PATCH (lp<nn>, log_xyz);
//     static Log_event le<mm>(<description for the 'O' command>,
//                             Log_event_xyz, 1, &lp<nn>);
//   and create an entry le<mm> for Jdb_tbuf_events::log_events[] 
//   (see jdb_tbuf_events.cpp)

#include "globalconfig.h"

#if defined(CONFIG_JDB)

#include "globals.h"
#include "jdb_tbuf.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "processor.h"

#define LOG_CONTEXT_SWITCH                                              \
  BEGIN_LOG_EVENT(log_context_switch)                                   \
  Tb_entry_ctx_sw *tb =                                                 \
     static_cast<Tb_entry_ctx_sw*>(Jdb_tbuf::new_entry());              \
  tb->set(this, space(), regs()->ip(), t, t_orig,	\
          t_orig->lock_cnt(), current_sched(cpu()),                     \
          current_sched(cpu()) ? current_sched(cpu())->prio() : 0,      \
          (Mword)__builtin_return_address(0));                          \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT

#define LOG_THREAD_EX_REGS                                              \
  BEGIN_LOG_EVENT(log_thread_ex_regs)                                   \
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(regs);               \
  Lock_guard <Cpu_lock> guard (&cpu_lock);                              \
  Tb_entry_ex_regs *tb =                                                \
     static_cast<Tb_entry_ex_regs*>(Jdb_tbuf::new_entry());             \
  tb->set(this, ef->ip(), regs, dst->regs()->sp(),                      \
                                dst->regs()->ip(), 0);                  \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT

#define LOG_THREAD_EX_REGS_FAILED                                       \
  BEGIN_LOG_EVENT(log_thread_ex_regs_failed)                            \
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(regs);               \
  Lock_guard <Cpu_lock> guard (&cpu_lock);                              \
  Tb_entry_ex_regs *tb =                                                \
     static_cast<Tb_entry_ex_regs*>(Jdb_tbuf::new_entry());             \
  tb->set(this, ef->ip(), regs, 0, 0, 1);                               \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT

#define LOG_IRQ(irq)                                                    \
  BEGIN_LOG_EVENT(log_irq)                                              \
  Tb_entry_ipc *tb =                                                    \
     static_cast<Tb_entry_ipc*>(Jdb_tbuf::new_entry());                 \
  Context *_log_current = current();                                    \
  tb->set_irq(_log_current, ip, irq);           			\
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT

#define LOG_TIMER_IRQ(irq)                                              \
  BEGIN_LOG_EVENT(log_timer_irq)                                        \
  Tb_entry_ipc *tb =                                                    \
     static_cast<Tb_entry_ipc*>(Jdb_tbuf::new_entry());                 \
  Context *_log_current = current();                                    \
  tb->set_irq(_log_current, ip, irq);                                   \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT

#define LOG_SHORTCUT_FAILED_1                                           \
  BEGIN_LOG_EVENT(log_shortcut_failed_1)                                \
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(regs);               \
  Tb_entry_ipc_sfl *tb =                                                \
     static_cast<Tb_entry_ipc_sfl*>(Jdb_tbuf::new_entry());             \
  tb->set(this, ef->ip(), regs->snd_desc(), regs->rcv_desc(),           \
                regs->timeout(), regs->snd_dst(),                       \
                _irq!=0, sender_list()->head()!=0, 0, 0, 0);            \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT

#define LOG_SHORTCUT_FAILED_2                                           \
  BEGIN_LOG_EVENT(log_shortcut_failed_2)                                \
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(regs);               \
  Tb_entry_ipc_sfl *tb =                                                \
     static_cast<Tb_entry_ipc_sfl*>(Jdb_tbuf::new_entry());             \
  tb->set(this, ef->ip(), regs->snd_desc(), regs->rcv_desc(),           \
                regs->timeout(), regs->snd_dst(), 0, 0,                 \
		!dst->is_tcb_mapped() || !dst->sender_ok(this),         \
                dst->is_tcb_mapped()?(dst->thread_lock()->test()):0,	\
		can_preempt);                                           \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT

#define LOG_SHORTCUT_SUCCESS                                            \
  BEGIN_LOG_EVENT(log_shortcut_succeeded)                               \
  Entry_frame *ef           = reinterpret_cast<Entry_frame*>(regs);     \
  Tb_entry_ipc *tb =                                                    \
     static_cast<Tb_entry_ipc*>(Jdb_tbuf::new_entry());                 \
  tb->set_sc(this, ef->ip(), regs, sched_context()->left());            \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT

#define LOG_TRAP                                                        \
  BEGIN_LOG_EVENT(log_trap)                                             \
  if (ts->_trapno != 1 && ts->_trapno != 3)                             \
    {                                                                   \
      Tb_entry_trap *tb =                                               \
         static_cast<Tb_entry_trap*>(Jdb_tbuf::new_entry());            \
      tb->set(this, ts->ip(), ts);                                      \
      Jdb_tbuf::commit_entry();                                         \
    }                                                                   \
  END_LOG_EVENT

#define LOG_TRAP_N(n)                                                   \
  BEGIN_LOG_EVENT(log_trap_n)                                           \
  Tb_entry_trap *tb =                                                   \
     static_cast<Tb_entry_trap*>(Jdb_tbuf::new_entry());                \
  Mword ip = (Mword)(__builtin_return_address(0));                      \
  tb->set(current(), ip, n);                                            \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT

#define LOG_PF_RES_USER                                                 \
  BEGIN_LOG_EVENT(log_pf_res)                                           \
  Tb_entry_pf_res *tb =                                                 \
     static_cast<Tb_entry_pf_res*>(Jdb_tbuf::new_entry());              \
  tb->set(this, regs()->ip(), pfa, err, ret);                           \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT

#define LOG_SCHED_SAVE							\
  BEGIN_LOG_EVENT(log_sched_save)					\
  Tb_entry_sched *tb =							\
     static_cast<Tb_entry_sched*>(Jdb_tbuf::new_entry());		\
  tb->set (current(), current()->regs()->ip(), 0,			\
           current_sched(cpu())->owner(),				\
           current_sched(cpu())->id(),					\
           current_sched(cpu())->prio(),				\
           current_sched(cpu())->left(),				\
           current_sched(cpu())->quantum());				\
  Jdb_tbuf::commit_entry();						\
  END_LOG_EVENT

#define LOG_SCHED_LOAD							\
  BEGIN_LOG_EVENT(log_sched_load)					\
  Tb_entry_sched *tb =							\
     static_cast<Tb_entry_sched*>(Jdb_tbuf::new_entry());		\
  tb->set (current(), current()->regs()->ip(), 1,			\
           current_sched(cpu())->owner(),				\
           current_sched(cpu())->id(),					\
           current_sched(cpu())->prio(),				\
           current_sched(cpu())->left(),				\
           current_sched(cpu())->quantum());				\
  Jdb_tbuf::commit_entry();						\
  END_LOG_EVENT

#define LOG_SCHED_INVALIDATE						\
  BEGIN_LOG_EVENT(log_sched_invalidate)					\
  Tb_entry_sched *tb =							\
     static_cast<Tb_entry_sched*>(Jdb_tbuf::new_entry());		\
  tb->set (current(), current()->regs()->ip(), 2,			\
           current_sched(cpu())->owner(),				\
           current_sched(cpu())->id(),					\
           current_sched(cpu())->prio(),				\
           timeslice_timeout.cpu(cpu())->get_timeout(),			\
           current_sched(cpu())->quantum());				\
  Jdb_tbuf::commit_entry();						\
  END_LOG_EVENT

#define LOG_SEND_PREEMPTION						\
  BEGIN_LOG_EVENT(log_preemption)					\
  Lock_guard <Cpu_lock> guard (&cpu_lock);				\
  Tb_entry_preemption *tb = 						\
     static_cast<Tb_entry_preemption*>(Jdb_tbuf::new_entry());		\
  tb->set (context_of(this), _receiver, current()->regs()->ip());	\
  Jdb_tbuf::commit_entry();						\
  END_LOG_EVENT

#define LOG_ID_NEAREST							\
  BEGIN_LOG_EVENT(log_id_nearest)					\
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(regs);               \
  Lock_guard <Cpu_lock> guard (&cpu_lock);				\
  Tb_entry_id_nearest *tb =						\
     static_cast<Tb_entry_id_nearest*>(Jdb_tbuf::new_entry());		\
  tb->set (this, ef->ip(), dst_id_long);				\
  Jdb_tbuf::commit_entry();						\
  END_LOG_EVENT

#define LOG_TASK_NEW							\
  BEGIN_LOG_EVENT(log_task_new)						\
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(regs);               \
  Lock_guard <Cpu_lock> guard (&cpu_lock);				\
  Tb_entry_task_new *tb =						\
     static_cast<Tb_entry_task_new*>(Jdb_tbuf::new_entry());		\
  tb->set (this, ef->ip(), regs);					\
  Jdb_tbuf::commit_entry();						\
  END_LOG_EVENT

/*
 * Kernel instrumentation macro used by jw5. Do not remove!
 */
#define LOG_JEAN1(context, sched1, sched2)				\
  do {									\
    Lock_guard <Cpu_lock> guard (&cpu_lock);				\
    Tb_entry_jean1 *tb =						\
      static_cast<Tb_entry_jean1*>(Jdb_tbuf::new_entry());		\
    tb->set (context, Proc::program_counter(),				\
	     sched1->owner(), sched2->owner());				\
    Jdb_tbuf::commit_entry();						\
  } while (0)

/*
 * Kernel instrumentation macro used by fm3. Do not remove!
 */
#define LOG_MSG(context, text)						\
  do {									\
    /* The cpu_lock is needed since virq::hit() depends on it */	\
    Lock_guard <Cpu_lock> guard (&cpu_lock);				\
    Tb_entry_ke *tb = static_cast<Tb_entry_ke*>(Jdb_tbuf::new_entry());	\
    tb->set_const(context, Proc::program_counter(), text);		\
    Jdb_tbuf::commit_entry();						\
  } while (0)

/*
 * Kernel instrumentation macro used by fm3. Do not remove!
 */
#define LOG_MSG_3VAL(context, text, v1, v2, v3)				\
  do {									\
    /* The cpu_lock is needed since virq::hit() depends on it */	\
    Lock_guard <Cpu_lock> guard (&cpu_lock);				\
    Tb_entry_ke_reg *tb =						\
       static_cast<Tb_entry_ke_reg*>(Jdb_tbuf::new_entry());		\
    tb->set_const(context, Proc::program_counter(), text, v1, v2, v3);	\
    Jdb_tbuf::commit_entry();						\
  } while (0)

#else

#define LOG_CONTEXT_SWITCH		do { } while (0)
#define LOG_THREAD_EX_REGS		do { } while (0)
#define LOG_THREAD_EX_REGS_FAILED	do { } while (0)
#define LOG_IRQ(irq)			do { } while (0)
#define LOG_TIMER_IRQ(irq)		do { } while (0)
#define LOG_SHORTCUT_FAILED_1		do { } while (0)
#define LOG_SHORTCUT_FAILED_2		do { } while (0)
#define LOG_SHORTCUT_SUCCESS		do { } while (0)
#define LOG_TRAP			do { } while (0)
#define LOG_TRAP_N(n)			do { } while (0)
#define LOG_PF_RES_USER			do { } while (0)
#define LOG_SCHED			do { } while (0)
#define LOG_SCHED_SAVE			do { } while (0)
#define LOG_SCHED_LOAD			do { } while (0)
#define LOG_SCHED_INVALIDATE		do { } while (0)
#define LOG_SEND_PREEMPTION		do { } while (0)
#define LOG_ID_NEAREST			do { } while (0)
#define LOG_TASK_NEW			do { } while (0)

#endif // CONFIG_JDB

#if defined(CONFIG_JDB) && defined(CONFIG_JDB_ACCOUNTING)

#define CNT_CONTEXT_SWITCH	\
  Jdb_tbuf::status()->kerncnts[Kern_cnt_context_switch]++;
#define CNT_ADDR_SPACE_SWITCH	\
  Jdb_tbuf::status()->kerncnts[Kern_cnt_addr_space_switch]++;
#define CNT_SHORTCUT_FAILED	\
  Jdb_tbuf::status()->kerncnts[Kern_cnt_shortcut_failed]++;
#define CNT_SHORTCUT_SUCCESS	\
  Jdb_tbuf::status()->kerncnts[Kern_cnt_shortcut_success]++;
#define CNT_IRQ			\
  Jdb_tbuf::status()->kerncnts[Kern_cnt_irq]++;
#define CNT_IPC_LONG		\
  Jdb_tbuf::status()->kerncnts[Kern_cnt_ipc_long]++;
#define CNT_PAGE_FAULT		\
  Jdb_tbuf::status()->kerncnts[Kern_cnt_page_fault]++;
#define CNT_IO_FAULT		\
  Jdb_tbuf::status()->kerncnts[Kern_cnt_io_fault]++;
#define CNT_TASK_CREATE		\
  Jdb_tbuf::status()->kerncnts[Kern_cnt_task_create]++;
#define CNT_SCHEDULE		\
  Jdb_tbuf::status()->kerncnts[Kern_cnt_schedule]++;
#define CNT_IOBMAP_TLB_FLUSH	\
  Jdb_tbuf::status()->kerncnts[Kern_cnt_iobmap_tlb_flush]++;

#else

#define CNT_CONTEXT_SWITCH	do { } while (0)
#define CNT_ADDR_SPACE_SWITCH	do { } while (0)
#define CNT_SHORTCUT_FAILED	do { } while (0)
#define CNT_SHORTCUT_SUCCESS	do { } while (0)
#define CNT_IRQ			do { } while (0)
#define CNT_IPC_LONG		do { } while (0)
#define CNT_PAGE_FAULT		do { } while (0)
#define CNT_IO_FAULT		do { } while (0)
#define CNT_TASK_CREATE		do { } while (0)
#define CNT_SCHEDULE		do { } while (0)
#define CNT_IOBMAP_TLB_FLUSH	do { } while (0)

#endif // CONFIG_JDB && CONFIG_JDB_ACCOUNTING

#endif

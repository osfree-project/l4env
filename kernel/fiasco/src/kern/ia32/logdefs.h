#ifndef LOGDEFS_H
#define LOGDEFS_H

#if defined(CONFIG_JDB)

#include "jdb_tbuf.h"

#define LOG_CONTEXT_SWITCH                                              \
  BEGIN_LOG_EVENT(log_context_switch)                                   \
  Tb_entry_ctx_sw *tb =                                                 \
     static_cast<Tb_entry_ctx_sw*>(Jdb_tbuf::new_entry());              \
  tb->set(this, regs()->eip, t, (Mword)t->kernel_sp,                    \
          (Mword)__builtin_return_address(0));                          \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT(log_context_switch)

#define LOG_THREAD_EX_REGS                                              \
  BEGIN_LOG_EVENT(log_thread_ex_regs)                                   \
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(regs);               \
  Lock_guard <Cpu_lock> guard (&cpu_lock);                              \
  Tb_entry_ex_regs *tb =                                                \
     static_cast<Tb_entry_ex_regs*>(Jdb_tbuf::new_entry());             \
  tb->set(this, ef->eip, regs, new_thread ? 0 : dst->regs()->esp,       \
                               new_thread ? 0 : dst->regs()->eip);      \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT(log_thread_ex_regs)

#define LOG_IRQ(irq)                                                    \
  BEGIN_LOG_EVENT(log_irq)                                              \
  Tb_entry_ipc *tb =                                                    \
     static_cast<Tb_entry_ipc*>(Jdb_tbuf::new_entry());                 \
  Context *_log_current = current();                                    \
  tb->set_irq(_log_current, _log_current->regs()->eip, irq);            \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT(log_irq)

#define LOG_TIMER_IRQ(irq)                                              \
  BEGIN_LOG_EVENT(log_timer_irq)                                        \
  Tb_entry_ipc *tb =                                                    \
     static_cast<Tb_entry_ipc*>(Jdb_tbuf::new_entry());                 \
  Context *_log_current = current();                                    \
  Mword eip = (Mword)(__builtin_return_address(0));                    \
  tb->set_irq(_log_current, eip, irq);                                  \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT(log_timer_irq)

#define LOG_SHORTCUT_FAILED_1                                           \
  BEGIN_LOG_EVENT(log_shortcut_failed_1)                                \
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(regs);               \
  Tb_entry_ipc_sfl *tb =                                                \
     static_cast<Tb_entry_ipc_sfl*>(Jdb_tbuf::new_entry());             \
  tb->set(this, ef->eip, regs->snd_desc(), regs->rcv_desc(),            \
                regs->timeout(), regs->snd_dest(),                      \
                _irq!=0, *sender_list()!=0, 0, 0);                      \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT(log_shortcut_failed_1)

#define LOG_SHORTCUT_FAILED_2                                           \
  BEGIN_LOG_EVENT(log_shortcut_failed_2)                                \
  Entry_frame *ef = reinterpret_cast<Entry_frame*>(regs);               \
  Tb_entry_ipc_sfl *tb =                                                \
     static_cast<Tb_entry_ipc_sfl*>(Jdb_tbuf::new_entry());             \
  tb->set(this, ef->eip, regs->snd_desc(), regs->rcv_desc(),            \
                regs->timeout(), regs->snd_dest(), 0, 0,                \
		!dest->sender_ok(this), dest->thread_lock()->test());   \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT(log_shortcut_failed_2)

#define LOG_SHORTCUT_SUCCESS                                            \
  BEGIN_LOG_EVENT(log_shortcut_succeeded)                               \
  Entry_frame *ef           = reinterpret_cast<Entry_frame*>(regs);     \
  Tb_entry_ipc *tb =                                                    \
     static_cast<Tb_entry_ipc*>(Jdb_tbuf::new_entry());                 \
  tb->set_sc(this, ef->eip, regs);                                      \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT(log_shortcut_succeeded)

#define LOG_TRAP                                                        \
  BEGIN_LOG_EVENT(log_trap)                                             \
  if ((ts->trapno != 1) && (ts->trapno != 3))                           \
    {                                                                   \
      Tb_entry_trap *tb =                                               \
         static_cast<Tb_entry_trap*>(Jdb_tbuf::new_entry());            \
      tb->set(this, ts->eip, ts);                                       \
      Jdb_tbuf::commit_entry();                                         \
    }                                                                   \
  END_LOG_EVENT(log_trap)

#define LOG_TRAP_N(n)                                                   \
  BEGIN_LOG_EVENT(log_trap_n)                                           \
  Tb_entry_trap *tb =                                                   \
     static_cast<Tb_entry_trap*>(Jdb_tbuf::new_entry());                \
  Mword eip = (Mword)(__builtin_return_address(0));                     \
  tb->set(current(), eip, n);                                           \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT(log_trap_n)

#define LOG_PF_RES_USER                                                 \
  BEGIN_LOG_EVENT(log_pf_res)                                           \
  Tb_entry_pf_res *tb =                                                 \
     static_cast<Tb_entry_pf_res*>(Jdb_tbuf::new_entry());              \
  tb->set(this, regs()->eip, pfa, err, ret);                            \
  Jdb_tbuf::commit_entry();                                             \
  END_LOG_EVENT(log_pf_res)

#define LOG_MSG(context, text)                                          \
  do {                                                                  \
    /* The cpu_lock is needed since virq::hit() depends on it */        \
    Lock_guard <Cpu_lock> guard (&cpu_lock);                            \
    Tb_entry_ke *tb = static_cast<Tb_entry_ke*>(Jdb_tbuf::new_entry()); \
    tb->set_const(context, 0, text);                                    \
    Jdb_tbuf::commit_entry();                                           \
  } while(0)

#define LOG_MSG_3VAL(context, text, val1, val2, val3)                   \
  do {                                                                  \
    /* The cpu_lock is needed since virq::hit() depends on it */        \
    Lock_guard <Cpu_lock> guard (&cpu_lock);                            \
    Tb_entry_ke_reg *tb =                                               \
       static_cast<Tb_entry_ke_reg*>(Jdb_tbuf::new_entry());            \
    tb->set_const(context, 0, text, val1, val2, val3);                  \
    Jdb_tbuf::commit_entry();                                           \
  } while(0)

#else

#define LOG_CONTEXT_SWITCH	do { } while (0)
#define LOG_THREAD_EX_REGS	do { } while (0)
#define LOG_IRQ(irq)		do { } while (0)
#define LOG_TIMER_IRQ(irq)	do { } while (0)
#define LOG_SHORTCUT_FAILED_1	do { } while (0)
#define LOG_SHORTCUT_FAILED_2	do { } while (0)
#define LOG_SHORTCUT_SUCCESS	do { } while (0)
#define LOG_TRAP		do { } while (0)
#define LOG_TRAP_N(n)		do { } while (0)
#define LOG_PF_RES_USER		do { } while (0)
#define LOG_MSG(text)		do { } while (0)

#endif

#endif

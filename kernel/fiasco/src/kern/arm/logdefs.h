#ifndef LOGDEFS_H
#define LOGDEFS_H

/* This is some logging for the poor and crippled. */

#define X_LOG_CONTEXT_SWITCH						\
  do {									\
    static int volatile tb_ctw_sw_enabled = 0;				\
    if (tb_ctw_sw_enabled)						\
      printf("tb_ctx_sw: ctx=%p, ip=%p\n", this, (void *)regs()->ip()); \
  } while (0)


// XXX x0 thread ID usage
#define X_LOG_THREAD_EX_REGS						\
  do {									\
    printf("tb_ex_regs: ctx=%p to %x.%x new ip=%p new sp=%p\n", this,	\
           dst->id().task(), dst->id().lthread(),			\
           (void *)regs->ip(), (void *)regs->sp());			\
  } while (0)

#define X_LOG_PF_RES_USER							\
  do {									\
    printf("tb_pf_usr_res: ctx=%p pc=%p pfa=%p e=%x snd=%x rcv=%x\n",	\
           this, (void *)regs()->ip(), (void *)pfa, error_code,		\
           err.error(), ret.error());					\
  } while (0)

#ifndef LOG_CONTEXT_SWITCH
#define LOG_CONTEXT_SWITCH		do { } while (0)
#endif
#ifndef LOG_THREAD_EX_REGS
#define LOG_THREAD_EX_REGS		do { } while (0)
#define LOG_THREAD_EX_REGS_FAILED	do { } while (0)
#endif
#define LOG_IRQ(irq)			do { } while (0)
#define LOG_TIMER_IRQ(irq)		do { } while (0)
#define LOG_SHORTCUT_FAILED_1		do { } while (0)
#define LOG_SHORTCUT_FAILED_2		do { } while (0)
#define LOG_SHORTCUT_SUCCESS		do { } while (0)
#ifndef LOG_PF_RES_USER
#define LOG_PF_RES_USER			do { } while (0)
#endif
#define LOG_SCHED_LOAD			do { } while (0)
#define LOG_SCHED_SAVE			do { } while (0)
#define LOG_SCHED_INVALIDATE		do { } while (0)
#define LOG_MSG(a,b)			do { } while (0)
#define LOG_MSG_3VAL(a,b,c,d,e)		do { } while (0)
#define LOG_SEND_PREEMPTION		do { } while (0)
#define LOG_TASK_NEW			do { } while (0)

#define CNT_CONTEXT_SWITCH		do { } while (0)
#define CNT_ADDR_SPACE_SWITCH		do { } while (0)
#define CNT_SHORTCUT_FAILED		do { } while (0)
#define CNT_SHORTCUT_SUCCESS		do { } while (0)
#define CNT_IRQ				do { } while (0)
#define CNT_IPC_LONG			do { } while (0)
#define CNT_PAGE_FAULT			do { } while (0)
#define CNT_TASK_CREATE			do { } while (0)
#define CNT_SCHEDULE			do { } while (0)
#define CNT_IOBMAP_TLB_FLUSH		do { } while (0)

#endif

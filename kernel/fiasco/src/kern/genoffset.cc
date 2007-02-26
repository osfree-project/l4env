
#include <cstdio>
#include <cstring>

using namespace std;

#define class     struct
#define private   public
#define protected public 

#include "thread.h"
#include "types.h"

#ifdef CONFIG_LOCAL_IPC
#include "utcb.h"
#include "utask_status.h"
#endif

typedef Stack<Context>		stack_ctxt;
typedef Stack_top<Context>	stack_top_ctxt;

bool dumpzero;

#define NAME_LEN 65

#define DUMP_CAST_OFFSET(type, subtype) ({				\
  unsigned offset = reinterpret_cast<Address>(				\
                    static_cast<subtype *>(				\
                    reinterpret_cast<type *>(1))) - 1;			\
  char buffer[NAME_LEN];                                                \
  snprintf (buffer, sizeof (buffer), "CAST_%s_TO_%s", #type, #subtype);	\
  printf (offset || dumpzero ? "#define %-*s %u\n" : "#define %-*s\n",	\
          NAME_LEN, buffer, offset); })

#define DUMP_OFFSET(prefix,name,offset) ({				\
  char buffer[NAME_LEN];						\
  snprintf (buffer, sizeof (buffer), "OFS__%s__%s", #prefix, #name);	\
  printf (offset || dumpzero ? "#define %-*s %u\n" : "#define %-*s\n",	\
          NAME_LEN, buffer, (unsigned) offset); })

#define GET_PTR(ptr,member) (&(ptr)->member)

#define GET_MEMBER_PTR(type,member) ({					\
  void *type::*p = (void *type::*) &type::member;			\
  &(((type *) 0)->*p); })

#define DUMP_MEMBER1(prefix,						\
                     type1,member1,					\
                     name) ({						\
  DUMP_OFFSET (prefix, name, GET_MEMBER_PTR (type1, member1)); })

#define DUMP_MEMBER2(prefix,						\
                     type1,member1,					\
                     type2,member2,					\
                     name) ({						\
  type2 *m2 = (type2 *) GET_MEMBER_PTR (type1, member1);		\
  DUMP_OFFSET (prefix, name, GET_PTR (m2, member2)); })

#define DUMP_MEMBER3(prefix,						\
                     type1,member1,					\
                     type2,member2,					\
                     type3,member3,					\
                     name) ({						\
  type2 *m2 = (type2 *) GET_MEMBER_PTR (type1, member1);		\
  type3 *m3 = (type3 *) GET_PTR (m2, member2);				\
  DUMP_OFFSET (prefix, name, GET_PTR (m3, member3)); })

#define DUMP_MEMBER4(prefix,						\
                     type1,member1,					\
                     type2,member2,					\
                     type3,member3,					\
                     type4,member4,					\
                     name) ({						\
  type2 *m2 = (type2 *) GET_MEMBER_PTR (type1, member1);		\
  type3 *m3 = (type3 *) GET_PTR (m2, member2);				\
  type4 *m4 = (type4 *) GET_PTR (m3, member3);				\
  DUMP_OFFSET (prefix, name, GET_PTR (m4, member4)); })

#define DUMP_MEMBER5(prefix,						\
                     type1,member1,					\
                     type2,member2,					\
                     type3,member3,					\
                     type4,member4,					\
                     type5,member5,					\
                     name) ({						\
  type2 *m2 = (type2 *) GET_MEMBER_PTR (type1, member1);		\
  type3 *m3 = (type3 *) GET_PTR (m2, member2);				\
  type4 *m4 = (type4 *) GET_PTR (m3, member3);				\
  type5 *m5 = (type5 *) GET_PTR (m4, member4);				\
  DUMP_OFFSET (prefix, name, GET_PTR (m5, member5)); })

void generate_fiasco_headers (void)
{
  puts ("\n/* Thread */\n");

  DUMP_MEMBER1 (THREAD, Context, _space_context,	SPACE_CONTEXT);
  DUMP_MEMBER1 (THREAD, Context, _stack_link,		STACK_LINK);
  DUMP_MEMBER1 (THREAD, Context, _donatee,		DONATEE);
  DUMP_MEMBER1 (THREAD, Context, _lock_cnt,		LOCK_CNT);
  DUMP_MEMBER1 (THREAD, Context, _thread_lock,		THREAD_LOCK_PTR);
  DUMP_MEMBER1 (THREAD, Context, _sched,		SCHED);
  DUMP_MEMBER1 (THREAD, Context, _mcp,			MCP);
  DUMP_MEMBER1 (THREAD, Context, _fpu_state,		FPU_STATE);
  DUMP_MEMBER1 (THREAD, Context, _state,		STATE);
  DUMP_MEMBER1 (THREAD, Context, ready_next,		READY_NEXT);
  DUMP_MEMBER1 (THREAD, Context, ready_prev,		READY_PREV);
  DUMP_MEMBER1 (THREAD, Context, kernel_sp,		KERNEL_SP);
  DUMP_MEMBER1 (THREAD, Receiver, _partner,		PARTNER);
  DUMP_MEMBER1 (THREAD, Receiver, _receive_regs,	RECEIVE_REGS);
  DUMP_MEMBER1 (THREAD, Receiver, _pagein_addr,		PAGEIN_ADDR);
  DUMP_MEMBER1 (THREAD, Receiver, _pagein_error_code,	PAGEIN_ERROR_CODE);
  DUMP_MEMBER1 (THREAD, Receiver, _pagein_applicant,	PAGEIN_APPLICANT);
  DUMP_MEMBER1 (THREAD, Receiver, _sender_first,	SENDER_FIRST);
  DUMP_MEMBER1 (THREAD, Thread, _timeout,		TIMEOUT);
  DUMP_MEMBER1 (THREAD, Thread, _id,			ID);
  DUMP_MEMBER1 (THREAD, Thread, _send_partner,		SEND_PARTNER);
  DUMP_MEMBER1 (THREAD, Thread, sender_next,		SENDER_NEXT);
  DUMP_MEMBER1 (THREAD, Thread, sender_prev,		SENDER_PREV);
  DUMP_MEMBER1 (THREAD, Thread, _preemption,		PREEMPTION);

  DUMP_MEMBER2 (THREAD, Thread,				
			_preemption, Preemption,
			_id,				PREEMPTION__ID);

  DUMP_MEMBER2 (THREAD, Thread,				
			_preemption, Preemption,
			_send_partner,			PREEMPTION__SEND_PARTNER);

  DUMP_MEMBER2 (THREAD, Thread,				
			_preemption, Preemption,
			sender_next,			PREEMPTION__SENDER_NEXT);

  DUMP_MEMBER2 (THREAD, Thread,				
			_preemption, Preemption,
			sender_prev,			PREEMPTION__SENDER_PREV);

  DUMP_MEMBER2 (THREAD, Thread,				
			_preemption, Preemption,
			_pending_preemption,		PREEMPTION__PENDING_PREEMPTION);

  DUMP_MEMBER1 (THREAD, Thread, _space,			SPACE);
  DUMP_MEMBER1 (THREAD, Thread, _thread_lock,		THREAD_LOCK);

  DUMP_MEMBER2 (THREAD, Thread,
			_thread_lock, Thread_lock,
                        _switch_lock,			THREAD_LOCK__SWITCH_LOCK);

  DUMP_MEMBER3 (THREAD, Thread,
			_thread_lock, Thread_lock,
                        _switch_lock, Switch_lock,
                        _lock_owner,			THREAD_LOCK__SWITCH_LOCK__LOCK_OWNER);

  DUMP_MEMBER3 (THREAD, Thread,
			_thread_lock, Thread_lock,
			_switch_lock, Switch_lock,
			_lock_stack,			THREAD_LOCK__SWITCH_LOCK__LOCK_STACK);

  DUMP_MEMBER4 (THREAD, Thread,
			_thread_lock, Thread_lock,
			_switch_lock, Switch_lock,
			_lock_stack, stack_ctxt,
			_head,				THREAD_LOCK__SWITCH_LOCK_LOCK_STACK__HEAD);

  DUMP_MEMBER5 (THREAD, Thread,
			_thread_lock, Thread_lock,
			_switch_lock, Switch_lock,
			_lock_stack, stack_ctxt,
			_head, stack_top_ctxt,
			_version,			THREAD_LOCK__SWITCH_LOCK_LOCK_STACK__HEAD__VERSION);

  DUMP_MEMBER5 (THREAD, Thread,
			_thread_lock, Thread_lock,
			_switch_lock, Switch_lock,
			_lock_stack, stack_ctxt,
			_head, stack_top_ctxt,
			_next,				THREAD_LOCK__SWITCH_LOCK_LOCK_STACK__HEAD__NEXT);

  DUMP_MEMBER2 (THREAD, Thread,
			_thread_lock, Thread_lock,
			_switch_hint,			THREAD_LOCK__SWITCH_HINT);

  DUMP_MEMBER1 (THREAD, Thread, _pager,			PAGER);
  DUMP_MEMBER1 (THREAD, Thread, _preempter,		PREEMPTER);
  DUMP_MEMBER1 (THREAD, Thread, _ext_preempter,		EXT_PREEMPTER);
  DUMP_MEMBER1 (THREAD, Thread, present_next,		PRESENT_NEXT);
  DUMP_MEMBER1 (THREAD, Thread, present_prev,		PRESENT_PREV);
  DUMP_MEMBER1 (THREAD, Thread, _irq,			IRQ);
  DUMP_MEMBER1 (THREAD, Thread, _target_desc,		TARGET_DESC);
  DUMP_MEMBER1 (THREAD, Thread, _pagein_status_code,	PAGEIN_STATUS_CODE);
  DUMP_MEMBER1 (THREAD, Thread, _vm_window0,		VM_WINDOW0);
  DUMP_MEMBER1 (THREAD, Thread, _vm_window1,		VM_WINDOW1);
  DUMP_MEMBER1 (THREAD, Thread, _recover_jmpbuf,	RECOVER_JMPBUF);
  DUMP_MEMBER1 (THREAD, Thread, _pf_timeout,		PF_TIMEOUT);
  DUMP_MEMBER1 (THREAD, Thread, _last_pf_address,	LAST_PF_ADDRESS);
  DUMP_MEMBER1 (THREAD, Thread, _last_pf_error_code,	LAST_PF_ERROR_CODE);
  DUMP_MEMBER1 (THREAD, Thread, _magic,			MAGIC);
  DUMP_MEMBER1 (THREAD, Thread, _idt,			IDT);
  DUMP_MEMBER1 (THREAD, Thread, _idt_limit,		IDT_LIMIT);
  DUMP_OFFSET  (THREAD, MAX, sizeof (Thread));

  puts ("\n/* Scheduling Context */\n");

  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_owner,		OWNER);
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_id,		ID);
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_prio,		PRIO);
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_timeslice,	TIMESLICE);
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_ticks_left,	TICKS_LEFT);
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_preemption_time,	PREEMPTION_TIME);
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_start_cputime,	START_CPUTIME);
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_total_cputime,	TOTAL_CPUTIME);
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_prev,		PREV);
  DUMP_MEMBER1 (SCHED_CONTEXT, Sched_context,_next,		NEXT);
  DUMP_OFFSET  (SCHED_CONTEXT, MAX, sizeof (Sched_context));

#ifdef CONFIG_LOCAL_IPC
  dumpzero = true;

  puts ("\n/* utask_status */\n");
  
  DUMP_MEMBER1 (UTCB, Utask_status, current_tcb,		CUR_UTCB);
  DUMP_MEMBER1 (UTCB, Utask_status, head,			UTCB_HEAD);
  DUMP_OFFSET  (USERSTATUS, MAX, sizeof (Utask_status));

  puts("\n/* Utcb */\n");

  DUMP_MEMBER1 (UTCB, Utcb, id,					UTCB_ID);
  DUMP_MEMBER1 (UTCB, Utcb, m_esp, 				ESP);
  DUMP_MEMBER1 (UTCB, Utcb, m_eip, 				EIP);
  DUMP_MEMBER1 (UTCB, Utcb, m_prev, 				PREV);
  DUMP_MEMBER1 (UTCB, Utcb, status, 				STATUSWORD);
  DUMP_MEMBER1 (UTCB, Utcb, fpu, 				FPU);
  DUMP_OFFSET  (USERTCB, MAX, sizeof (Utcb));
  
  dumpzero = false;
#endif
}

void generate_cast_offsets (void)
{
  puts ("\n/* Cast Offsets */\n");

  DUMP_CAST_OFFSET (Thread, Receiver);
  DUMP_CAST_OFFSET (Thread, Sender);
}

int main (void)
{
  generate_fiasco_headers();
  generate_cast_offsets();
  return 0;
}

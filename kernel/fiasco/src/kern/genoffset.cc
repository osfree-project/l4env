
#include <cstdio>
#include <cstring>


using namespace std;

#define class     struct
#define private   public
#define protected public 

#include "thread.h"

// vm_offset_t comes from "types.h"
#include "types.h"

#define NAME_LEN 40

#define GET_PTR(ptr,member) (&(ptr)->member)

#define GET_MEMBER_PTR(type,member) ({              \
    void *type::*p = (void *type::*) &type::member; \
    &(((type *) 0)->*p);             })



namespace Proc {
  Status volatile virtual_processor_state;
};



#define DUMP_MEMBER(type,member,name)          ({          \
  printf("#define OFS__THREAD__%s", #name);                \
  for(int i=strlen(#name); i<NAME_LEN; i++) printf(" ");   \
  unsigned p = (unsigned) GET_MEMBER_PTR(type,member);     \
  if (p != 0) printf("%d",p);                              \
  printf("\n");                                            \
                                             0;  })





#define DUMP_MEMBER2(type,member, member_type, sub_member, name)  ({ \
  printf("#define OFS__THREAD__%s ",#name);                          \
  for(int i=strlen(#name); i<NAME_LEN; i++) printf(" ");             \
  member_type *p = (member_type *) GET_MEMBER_PTR(type,member);      \
  printf("%d\n",(unsigned) GET_PTR(p,sub_member));;                  \
  0; })



#define DUMP_MEMBER3(type,member, member_type, sub_member, sub_type, sub_member2,name)  ({ \
  printf("#define OFS__THREAD__%s ", #name);                         \
  for(int i=strlen(#name); i<NAME_LEN; i++) printf(" ");             \
  member_type *p = (member_type *) GET_MEMBER_PTR(type,member);      \
  sub_type    *p1= (sub_type    *) GET_PTR(p,sub_member);            \
  printf("%d\n", (unsigned) GET_PTR(p1,sub_member2));                \
  0; })

#define DUMP_MEMBER4(             type,                              \
                     member,      member_type,                       \
                     sub_member,  sub_type,                          \
                     sub_member2, sub_type2,                         \
                     sub_member3,                                    \
                     name)  ({                                       \
  printf("#define OFS__THREAD__%s ", #name);                         \
  for(int i=strlen(#name); i<NAME_LEN; i++) printf(" ");             \
  member_type *p = (member_type *) GET_MEMBER_PTR(type,member);      \
  sub_type    *p1= (sub_type    *) GET_PTR( p,sub_member);           \
  sub_type2   *p2= (sub_type2   *) GET_PTR(p1,sub_member2);          \
  printf("%d\n",(unsigned) GET_PTR(p2,sub_member3));                 \
  0; })

#define DUMP_MEMBER5(             type,                              \
                     member,      member_type,                       \
                     sub_member,  sub_type,                          \
                     sub_member2, sub_type2,                         \
                     sub_member3, sub_type3,                         \
                     sub_member4,                                    \
                     name)  ({                                       \
  printf("#define OFS__THREAD__%s ", #name);                         \
  for(int i=strlen(#name); i<NAME_LEN; i++) printf(" ");             \
  member_type *p = (member_type *) GET_MEMBER_PTR(type,member);      \
  sub_type    *p1= (sub_type    *) GET_PTR( p,sub_member);           \
  sub_type2   *p2= (sub_type2   *) GET_PTR(p1,sub_member2);          \
  sub_type3   *p3= (sub_type3   *) GET_PTR(p2,sub_member3);          \
  printf("%d\n",(unsigned) GET_PTR(p3,sub_member4));                 \
  0; })

#define GET_CAST_OFFSET(type, sub_type)                              \
     (reinterpret_cast< vm_offset_t >(                               \
            static_cast<sub_type * >(                                \
                 reinterpret_cast<type * >(0x10)))- 0x10)


#define DUMP_CAST_OFFSET(type, sub_type) ({                          \
  printf("#define CAST_%s_TO_%s ", #type,#sub_type);                 \
  for(int i=strlen("#define CAST_" #type "_TO_" #sub_type " ");      \
         i<(NAME_LEN + 22); i++) printf(" ");                        \
  printf("%d\n",GET_CAST_OFFSET(type, sub_type));                    \
  0; })

#ifdef FIASCO_SMP
typedef Stack<Context,int>	stack_ctxt_int;
typedef Stack_top<Context>	stack_top_ctxt;

#else
typedef Stack<Context>	stack_ctxt;
typedef Stack_top<Context>	stack_top_ctxt;
#endif

void generate_fiasco_smp_headers(void);
void generate_fiasco_headers(void);
void generate_cast_offsets(void);

int
main(void)
{
#ifdef FIASCO_SMP
  generate_fiasco_smp_headers();
#else
  generate_fiasco_headers();
#endif
  printf("#define OFS__THREAD__MAX");

  for(int i=strlen("MAX")-1; i<NAME_LEN; i++) printf(" ");
  printf("%d\n",sizeof(Thread));

  generate_cast_offsets();
  return 0;

}


void 
generate_fiasco_headers()
{
#ifndef FIASCO_SMP
  DUMP_MEMBER(Context, _state,STATE);


  DUMP_MEMBER(Context, ready_next, READY_NEXT);
  DUMP_MEMBER(Context, ready_prev, READY_PREV);


  DUMP_MEMBER(Context, kernel_sp, KERNEL_SP);
  DUMP_MEMBER(Context, _space_context, SPACE_CONTEXT);
  DUMP_MEMBER(Context, _stack_link, STACK_LINK);
  DUMP_MEMBER(Context, _donatee, DONATEE);


  DUMP_MEMBER(Thread, _lock_cnt, LOCK_CNT);
  DUMP_MEMBER(Context, _thread_lock, THREAD_LOCK_PTR);

  DUMP_MEMBER(Context,_sched,SCHED);

  DUMP_MEMBER2(Thread,_sched,sched_t,_prio, SCHED__PRIO);
  DUMP_MEMBER2(Thread,_sched,sched_t,_mcp, SCHED__MCP);
  DUMP_MEMBER2(Thread,_sched,sched_t,_timeslice, SCHED__TIMESLICE);
  DUMP_MEMBER2(Thread,_sched,sched_t,_ticks_left, SCHED__TICKS_LEFT);

  DUMP_MEMBER(Context,_fpu_state,FPU_STATE);

  DUMP_MEMBER(Receiver, _partner,          PARTNER);
  DUMP_MEMBER(Receiver, _receive_regs,     RECEIVE_REGS);
  DUMP_MEMBER(Receiver, _pagein_request,   PAGIN_REQUEST);
  DUMP_MEMBER(Receiver, _pagein_applicant, PAGEIN_APPLICANT);
  DUMP_MEMBER(Receiver, _sender_first,     SENDER_FIRST);

  DUMP_MEMBER(Thread, _id, ID);
  DUMP_MEMBER(Thread, _send_partner, SEND_PARTNER);
  DUMP_MEMBER(Thread, sender_next,     SENDER_NEXT);
  DUMP_MEMBER(Thread, sender_prev,     SENDER_PREV);

  DUMP_MEMBER(Thread, _space, SPACE);
  DUMP_MEMBER(Thread, _thread_lock, THREAD_LOCK);

  DUMP_MEMBER2(Thread, _thread_lock, Thread_lock,_switch_lock,THREAD_LOCK__SWITCH_LOCK);



  DUMP_MEMBER3(            Thread,                \
             _thread_lock, Thread_lock,           \
             _switch_lock,  Switch_lock,            \
	     _lock_owner,                                 \
             THREAD_LOCK__SWITCH_LOCK__LOCK_OWNER)  ;


  DUMP_MEMBER3(            Thread,                \
             _thread_lock, Thread_lock,           \
             _switch_lock,  Switch_lock,            \
	     _lock_stack,                                 \
             THREAD_LOCK__SWITCH_LOCK__LOCK_STACK)  ;


  DUMP_MEMBER4(            Thread,                \
             _thread_lock, Thread_lock,           \
	     _switch_lock, Switch_lock,           \
	     _lock_stack,  stack_ctxt,              \
             _head,                                 \
             THREAD_LOCK__SWITCH_LOCK_LOCK_STACK__HEAD)  ;


  DUMP_MEMBER5(            Thread,                \
             _thread_lock, Thread_lock,           \
	     _switch_lock, Switch_lock,           \
	     _lock_stack,  stack_ctxt,              \
             _head,        stack_top_ctxt,          \
   	     _version,	                            \
             THREAD_LOCK__SWITCH_LOCK_LOCK_STACK__HEAD__VERSION)  ;

  DUMP_MEMBER5(            Thread,                \
             _thread_lock, Thread_lock,           \
	     _switch_lock, Switch_lock,           \
	     _lock_stack,  stack_ctxt,              \
             _head,        stack_top_ctxt,          \
   	     _next,	                            \
             THREAD_LOCK__SWITCH_LOCK_LOCK_STACK__HEAD__NEXT)  ;


  DUMP_MEMBER2(Thread, _thread_lock, Thread_lock,_switch_hint,THREAD_LOCK__SWITCH_HINT);

  DUMP_MEMBER(Thread, _timeout, TIMEOUT);

  DUMP_MEMBER(Thread, _pager, PAGER);
  DUMP_MEMBER(Thread, _preempter, PREEMPTER);
  DUMP_MEMBER(Thread, _ext_preempter, EXT_PREEMPTER);

  DUMP_MEMBER(Thread, present_next, PRESENT_NEXT);
  DUMP_MEMBER(Thread, present_prev, PRESENT_PREV);

  DUMP_MEMBER(Thread, _irq,IRQ);

  DUMP_MEMBER(Thread, _idt,IDT);
  DUMP_MEMBER(Thread, _idt_limit,IDT_LIMIT);
  
  DUMP_MEMBER(Thread, _target_desc, TARGET_DESC);
  DUMP_MEMBER(Thread, _pagein_error_code, PAGEIN_ERROR_CODE);

  DUMP_MEMBER(Thread, _vm_window0, VM_WINDOW0);
  DUMP_MEMBER(Thread, _vm_window1, VM_WINDOW1);

  DUMP_MEMBER(Thread, _recover_jmpbuf, RECOVER_JMPBUF);
  DUMP_MEMBER(Thread, _pf_timeout, PF_TIMEOUT);

  DUMP_MEMBER(Thread, _last_pf_address, LAST_PF_ADDRESS);
  DUMP_MEMBER(Thread, _last_pf_error_code, LAST_PF_ERROR_CODE);

  DUMP_MEMBER(Thread, _magic, MAGIC);


#endif
  
}

void
generate_fiasco_smp_headers()
{
#ifdef FIASCO_SMP
  DUMP_MEMBER(Context, _state,STATE);

  DUMP_MEMBER(Context, _lock_owner, LOCK_OWNER);

  DUMP_MEMBER(Context, _ready_next, READY_NEXT);
  DUMP_MEMBER(Context, _ready_prev, READY_PREV);


  DUMP_MEMBER(Context, kernel_sp, KERNEL_SP);
  DUMP_MEMBER(Context, _space_context, SPACE_CONTEXT);
  DUMP_MEMBER(Context, _no_switch_req, NO_SWITCH_REQ);
  DUMP_MEMBER(Context, _stack_link, STACK_LINK);
  DUMP_MEMBER(Context, _enqueue_link, ENQUEUE_LINK);
  DUMP_MEMBER(Context, _dequeue_link, DEQUEUE_LINK);
  DUMP_MEMBER(Context, _switch_link, SWITCH_LINK);



  DUMP_MEMBER(Context,_counter,COUNTER);
  DUMP_MEMBER(Context,_enq_counter,ENQ_COUNTER);
  DUMP_MEMBER(Context,_enq_pend_cnt,ENQ_PEND_CNT);

  DUMP_MEMBER(Context,_spin_cnt,SPIN_CNT);
  DUMP_MEMBER(Context,_lock_cnt, LOCK_CNT);
  DUMP_MEMBER(Context,_thread_lock,THREAD_LOCK_PTR);



  DUMP_MEMBER(Context,_sched,SCHED);

  DUMP_MEMBER2(Thread,_sched,sched_t,_prio, SCHED__PRIO);
  DUMP_MEMBER2(Thread,_sched,sched_t,_mcp, SCHED__MCP);
  DUMP_MEMBER2(Thread,_sched,sched_t,_timeslice, SCHED__TIMESLICE);
  DUMP_MEMBER2(Thread,_sched,sched_t,_ticks_left, SCHED__TICKS_LEFT);

  DUMP_MEMBER(Context,_fpu_state,FPU_STATE);

  DUMP_MEMBER(Context, _stack_low      , STACK_LOW);
  DUMP_MEMBER(Context, _switch_fail_cnt, SWITCH_FAIL_CNT);
  DUMP_MEMBER(Context, _entry_cnt      , ENTRY_CNT);
  DUMP_MEMBER(Context, _pf_cnt         , PF_CNT);
  DUMP_MEMBER(Context, _switch_to_cnt  , SWITCH_TO_CNT);
  DUMP_MEMBER(Context, _pf_in_progress , PF_IN_PROGRESS);
  DUMP_MEMBER(Context, _user_space_access, USER_SPACE_ACCESS);
  DUMP_MEMBER(Context, _ctxt_log       , CTXT_LOG);
  DUMP_MEMBER(Context, _timer_intr_in_progress, TIMER_INTR_IN_PROGRESS);
  DUMP_MEMBER(Context, _last_entry_eip , LAST_ENTRY_EIP);
  DUMP_MEMBER(Context, _last_entry_eip_cnt, LAST_ENTRY_EIP_CNT);

  DUMP_MEMBER(Receiver, _partner,          PARTNER);
  DUMP_MEMBER(Receiver, _receive_regs,     RECEIVE_REGS);
  DUMP_MEMBER(Receiver, _pagein_request,   PAGIN_REQUEST);
  DUMP_MEMBER(Receiver, _pagein_applicant, PAGEIN_APPLICANT);
  DUMP_MEMBER(Receiver, _sender_first,     SENDER_FIRST);

  DUMP_MEMBER(Thread, _id, ID);
  DUMP_MEMBER(Thread, _send_partner, SEND_PARTNER);
  DUMP_MEMBER(Thread, sender_next,     SENDER_NEXT);
  DUMP_MEMBER(Thread, sender_prev,     SENDER_PREV);

  DUMP_MEMBER(Thread, _space, SPACE);
  DUMP_MEMBER(Thread, _thread_lock, THREAD_LOCK);



  DUMP_MEMBER2(Thread, _thread_lock, Thread_lock,_lock_stack,THREAD_LOCK__LOCK_STACK);
  DUMP_MEMBER3(            Thread,                \
             _thread_lock, Thread_lock,           \
             _lock_stack,  stack_ctxt_int,          \
	     _head,                                 \
             THREAD_LOCK__LOCK_STACK__HEAD)  ;

  DUMP_MEMBER4(            Thread,                \
             _thread_lock, Thread_lock,           \
             _lock_stack,  stack_ctxt_int,          \
	     _head,        stack_top_ctxt,          \
             _version,                              \
             THREAD_LOCK__LOCK_STACK__HEAD__VERSION)  ;

  DUMP_MEMBER4(            Thread,                \
             _thread_lock, Thread_lock,           \
             _lock_stack,  stack_ctxt_int,          \
	     _head,        stack_top_ctxt,          \
             _next,                                 \
             THREAD_LOCK__LOCK_STACK__HEAD__NEXT)  ;

  DUMP_MEMBER3(            Thread,                \
             _thread_lock, Thread_lock,           \
             _lock_stack,  stack_ctxt_int,          \
	     _arg,                                 \
             THREAD_LOCK__LOCK_STACK__ARG)  ;

  DUMP_MEMBER2(Thread, _thread_lock, Thread_lock,_switch_hint,THREAD_LOCK__SWITCH_HINT);
  DUMP_MEMBER2(              Thread, 
	       _thread_lock, Thread_lock,
               _was_executing,
	       THREAD_LOCK__WAS_EXECUTING);


  DUMP_MEMBER(Thread, _timeout, TIMEOUT);
  DUMP_MEMBER(Thread, _pager,   PAGER);
  DUMP_MEMBER(Thread, _preempter, PREEMPTER);
  DUMP_MEMBER(Thread, _ext_preempter, EXT_PREEMPTER);

  DUMP_MEMBER(Thread, present_next, PRESENT_NEXT);
  DUMP_MEMBER(Thread, present_prev, PRESENT_PREV);
  DUMP_MEMBER(Thread, _irq, IRQ);
  DUMP_MEMBER(Thread, _idt, IDT);
  DUMP_MEMBER(Thread, _idt_limit, IDT_LIMIT);
  DUMP_MEMBER(Thread, _target_desc, TARGET_DESC);
  DUMP_MEMBER(Thread, _pagein_error_code, PAGEIN_ERROR_CODE);
  DUMP_MEMBER(Thread, _vm_window0, VM_WINDOW0);
  DUMP_MEMBER(Thread, _vm_window1, VM_WINDOW1);
  DUMP_MEMBER(Thread, _recover_jmpbuf, RECOVER_JMPBUF);

  DUMP_MEMBER(Thread, _pf_timeout, PF_TIMEOUT);
  DUMP_MEMBER(Thread, _last_pf_address, LAST_PF_ADDRESS);
  DUMP_MEMBER(Thread, _last_pf_error_code, LAST_PF_ERROR_CODE);

  DUMP_MEMBER(Thread, _magic, MAGIC);
  DUMP_MEMBER(Thread, _magic1, MAGIC1);
  DUMP_MEMBER(Thread, _magic2, MAGIC2);
  DUMP_MEMBER(Thread, _magic3, MAGIC3);
  DUMP_MEMBER(Thread, _magic4, MAGIC4);
  DUMP_MEMBER(Thread, _magic5, MAGIC5);
  DUMP_MEMBER(Thread, _magic6, MAGIC6);
  DUMP_MEMBER(Thread, _magic7, MAGIC7);
  DUMP_MEMBER(Thread, _magic8, MAGIC8);
#endif

}

void
generate_cast_offsets()
{
  DUMP_CAST_OFFSET(Thread,Sender);
  DUMP_CAST_OFFSET(Thread,Receiver);
}

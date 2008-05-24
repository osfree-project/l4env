#include <stdio.h>

#undef L4V2_IPC_SYSENTER
#undef L4X0_IPC_SYSENTER
#undef CONFIG_L4_CALL_SYSCALLS

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/ktrace.h>
#include <l4/util/util.h>
#include <l4/util/rdtsc.h>
#include <l4/util/bitops.h>
#include <l4/util/idt.h>
#include <l4/util/macros.h>
#include <l4/sys/utcb.h>
#include <l4/rmgr/librmgr.h>

#include "global.h"
#include "ipc_buffer.h"
#include "worker.h"
#include "pingpong.h"
#include "helper.h"

#undef PREFIX
#define PREFIX(a) int30_ ## a
#include "worker_inc_x86.h"

static unsigned long shared_data[1024] __attribute__((aligned(4096)));

void dummy_exception13_handler(void);
asm (".align 16                                   \n\t"
     "dummy_exception13_handler:                  \n\t"
     "add    $4, %esp   # pop error code          \n\t"
     "add    $2, (%esp) # next instr after int 13 \n\t"
     "iret                                        \n\t");

void exception13_handler_with_ipc_reflection(void);
asm (".align 16					\n\t"
     "exception13_handler_with_ipc_reflection:	\n\t"
     "add $4, %esp  # pop error code		\n\t"
     //"add $2, (%esp) # jump over instruction	\n\t"
     "pushl %eax				\n\t"
     "pushl %ecx				\n\t"
     "pushl %edx				\n\t"
     "pushl %esp				\n\t"
     "call ipc_refl_func			\n\t"
     "add $4, %esp				\n\t"
     "popl %edx					\n\t"
     "popl %ecx					\n\t"
     "popl %eax					\n\t"
     "iret					\n\t");

/** Exception handler in another AS for reflected exceptions */
void __attribute__((noreturn))
exception_reflection_pong_handler(void)
{
  l4_umword_t d0, d1;
  l4_msgdope_t dope;
  l4_threadid_t src_id;
  int err;

  touch_pages();

  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

wait_only:
  err = l4_ipc_wait(&src_id, L4_IPC_SHORT_MSG, &d0, &d1,
                    L4_IPC_NEVER, &dope);

  if (err)
    {
      printf("%s: l4_ipc_wait = %x\n", __func__, err);
      goto wait_only;
    }

  while (1)
    {
      d0 += 2;
      d1 += 4;

      /* jump over instruction */
      shared_data[0] += 2;

      err = l4_ipc_reply_and_wait(src_id,  L4_IPC_SHORT_MSG,  d0,  d1,
	                          &src_id, L4_IPC_SHORT_MSG, &d0, &d1,
				  L4_IPC_NEVER, &dope);
      if (err)
	{
	  printf("%s: l4_ipc_reply_and_wait = %x\n", __func__, err);
	  goto wait_only;
	}
    }
}

static void __attribute__((used))
ipc_refl_func(l4_umword_t esp)
{
  static int count = 0;
  int err;
  l4_msgdope_t result;
  l4_umword_t d0, d1;

  /* we saved some stuff on the stack */
  esp += sizeof(long)*3;

  memcpy(shared_data, (void *)esp, 48);

  err = l4_ipc_call(pong_id,
                    L4_IPC_SHORT_MSG, count, 0x15,
		    L4_IPC_SHORT_MSG, &d0, &d1,
		    L4_IPC_NEVER, &result);

  if (err)
    printf("%s: ipc call error = %x\n", __func__, err);
  else
    memcpy((void *)esp, shared_data, 48);
}

/** Exception handler for exception IPC */
void __attribute__((noreturn))
exception_IPC_pong_handler(void)
{
  l4_umword_t d0, d1;
  l4_msgdope_t dope;
  l4_threadid_t src_id;
  int err;
  l4_msgtag_t tag;
  l4_utcb_t *utcb = l4_utcb_get();

  touch_pages();

  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

wait_only:
  err = l4_ipc_wait_tag(&src_id, L4_IPC_SHORT_MSG, &d0, &d1,
                    L4_IPC_NEVER, &dope, &tag);

  if (err)
    {
      printf("%s: l4_ipc_wait = %x\n", __func__, err);
      goto wait_only;
    }

  while (1)
    {
      if (l4_msgtag_is_exception(tag))
	{
	  /* Resolve fault */
	  utcb->exc.eip += 2;
          tag = l4_msgtag(0, L4_UTCB_EXCEPTION_REGS_SIZE, 0, 0);

	  /* reply and receive exception IPC */
	  err = l4_ipc_reply_and_wait_tag(src_id,  L4_IPC_SHORT_MSG,  d0,  d1, tag,
				      &src_id, L4_IPC_SHORT_MSG, &d0, &d1,
				      L4_IPC_NEVER, &dope, &tag);
	  if (err)
	    {
	      printf("%s: l4_ipc_reply_and_wait = %x\n", __func__, err);
	      goto wait_only;
	    }

	}
      else
	{
	  printf("%s: Received unknown message (PF?) from " l4util_idfmt "! (%lx,%lx)\n",
	         __func__, l4util_idstr(src_id), d0, d1);
	  goto wait_only;
	}
    }
}

/* Raise exception, idt method */
static void __attribute__((noreturn))
ping_exception_idt_thread(void *func)
{
  int i;
  l4_cpu_time_t in,out;
  static struct
    {
      l4util_idt_header_t header;
      l4util_idt_desc_t   desc[0x20];
    } __attribute__((packed)) idt;

  touch_pages();

  l4util_idt_init (&idt.header, 0x20);
  l4util_idt_entry(&idt.header, 13, func);
  l4util_idt_load (&idt.header);

  PREFIX(recv)(main_id);
  PREFIX(send)(main_id);

  in = l4_rdtsc();
  for (i = global_rounds; i; i--)
    {
      asm volatile ("int $13" : : : "memory");
      asm volatile ("int $13" : : : "memory");
      asm volatile ("int $13" : : : "memory");
      asm volatile ("int $13" : : : "memory");
      asm volatile ("int $13" : : : "memory");
      asm volatile ("int $13" : : : "memory");
      asm volatile ("int $13" : : : "memory");
      asm volatile ("int $13" : : : "memory");
    }
  out = l4_rdtsc();

  printf("reflection       :  %10u cycles / %5lu rounds >> %u <<\n",
	 (l4_uint32_t)(out-in), global_rounds*8,
	 (l4_uint32_t)((out-in)/(global_rounds*8)));

  l4util_idt_init (&idt.header, 0x20);
  l4util_idt_load (&idt.header);

  /* tell main that we are finished */
  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

/** Raise exceptions intra address space, idt method */
void __attribute__((noreturn))
ping_exception_intraAS_idt_thread(void)
{
  ping_exception_idt_thread(dummy_exception13_handler);
}

/** Raise exceptions inter address space, idt method */
/*  IPC to another task (simulates trad. L4Linux behavior) */
void __attribute__((noreturn))
ping_exception_interAS_idt_thread(void)
{
  ping_exception_idt_thread(exception13_handler_with_ipc_reflection);
}

void __attribute__((noreturn))
ping_exception_IPC_thread(void)
{
  int i;
  l4_cpu_time_t in, out;
  l4_threadid_t preempter = L4_INVALID_ID, pager = pong_id;
  l4_umword_t dummy;

  touch_pages();

  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

  /* change my pager to pong_thread */
  l4_thread_ex_regs(l4_myself(), (l4_umword_t)-1, (l4_umword_t)-1,
                    &preempter, &pager, &dummy, &dummy, &dummy);

  in = l4_rdtsc();
  for (i = global_rounds; i; i--)
    {
      asm volatile ("int $0x80" : : : "memory");
      asm volatile ("int $0x80" : : : "memory");
      asm volatile ("int $0x80" : : : "memory");
      asm volatile ("int $0x80" : : : "memory");
      asm volatile ("int $0x80" : : : "memory");
      asm volatile ("int $0x80" : : : "memory");
      asm volatile ("int $0x80" : : : "memory");
      asm volatile ("int $0x80" : : : "memory");
    }
  out = l4_rdtsc();

  printf("exception IPC    :  %10u cycles / %5lu rounds >> %u <<\n",
	 (l4_uint32_t)(out-in), global_rounds*8,
	 (l4_uint32_t)((out-in)/(global_rounds*8)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}



/* UTCB IPC test */
static int utcb_data_size;

void __attribute__((noreturn))
pong_utcb_ipc_thread(void)
{
  l4_umword_t d0, d1;
  l4_msgdope_t dope;
  l4_threadid_t src_id;
  int err;
  l4_msgtag_t tag;

  touch_pages();

  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

wait_only:
  err = l4_ipc_wait_tag(&src_id, L4_IPC_SHORT_MSG, &d0, &d1,
                    L4_IPC_NEVER, &dope, &tag);

  if (err)
    {
      printf("%s: l4_ipc_wait = %x\n", __func__, err);
      goto wait_only;
    }

  while (1)
    {
      tag = l4_msgtag(0, utcb_data_size, 0, 0);

      /* reply and receive exception IPC */
      err = l4_ipc_reply_and_wait_tag(src_id,  L4_IPC_SHORT_MSG,  d0,  d1, tag,
                                  &src_id, L4_IPC_SHORT_MSG, &d0, &d1,
                                  L4_IPC_NEVER, &dope, &tag);
      if (err)
        {
          printf("%s: l4_ipc_reply_and_wait = %x\n", __func__, err);
          goto wait_only;
        }
    }

  /* done, sleep */
  l4_sleep_forever();
}

void __attribute__((noreturn))
ping_utcb_ipc_thread(void)
{
  int i;
  l4_umword_t d0 = 0, d1 = 0;
  l4_msgdope_t dope;
  l4_cpu_time_t in, out;
  l4_msgtag_t tag;
  l4_msgtag_t rtag;

  touch_pages();

  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

  for (utcb_data_size = 0; utcb_data_size < L4_UTCB_GENERIC_DATA_SIZE;
       utcb_data_size++)
    {
      tag = l4_msgtag(0, utcb_data_size, 0, 0);

      in = l4_rdtsc();
      for (i = global_rounds; i; i--)
        {
          l4_ipc_call_tag(pong_id,
                      L4_IPC_SHORT_MSG, d0, d1, tag,
                      L4_IPC_SHORT_MSG, &d0, &d1,
                      L4_IPC_NEVER, &dope, &rtag);
        }
      out = l4_rdtsc();

      printf("UTCB IPC (%02d)  :  %10u cycles / %5lu rounds >> %u <<\n",
             utcb_data_size,
             (l4_uint32_t)(out-in), global_rounds,
             (l4_uint32_t)((out-in)/(global_rounds)));
    }

  /* tell main that we are finished */
  PREFIX(send)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}

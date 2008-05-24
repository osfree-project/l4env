#include <stdio.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/ktrace.h>
#include <l4/util/util.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>
#include <l4/sys/utcb.h>
#include <l4/rmgr/librmgr.h>

#include "global.h"
#include "ipc_buffer.h"
#include "worker.h"
#include "pingpong.h"
#include "helper.h"

#undef PREFIX
#define PREFIX(a) generic_ ## a
#include "worker_inc_generic.h"

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

  in = get_clocks();
  for (i = global_rounds; i; i--)
    {
#ifdef ARCH_arm
      asm volatile ("swi $0x123" : : : "memory");
      asm volatile ("swi $0x123" : : : "memory");
      asm volatile ("swi $0x123" : : : "memory");
      asm volatile ("swi $0x123" : : : "memory");
      asm volatile ("swi $0x123" : : : "memory");
      asm volatile ("swi $0x123" : : : "memory");
      asm volatile ("swi $0x123" : : : "memory");
      asm volatile ("swi $0x123" : : : "memory");
#else
      asm volatile ("int $0x13" : : : "memory");
      asm volatile ("int $0x13" : : : "memory");
      asm volatile ("int $0x13" : : : "memory");
      asm volatile ("int $0x13" : : : "memory");
      asm volatile ("int $0x13" : : : "memory");
      asm volatile ("int $0x13" : : : "memory");
      asm volatile ("int $0x13" : : : "memory");
      asm volatile ("int $0x13" : : : "memory");
#endif
    }
  out = get_clocks();

  printf("CLK:exception IPC    :  %10u cycles / %5lu rounds >> %u <<\n",
	 (l4_uint32_t)(out-in), global_rounds*8,
	 (l4_uint32_t)((out-in)/(global_rounds*8)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

  /* done, sleep */
  l4_sleep_forever();
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
#ifdef ARCH_x86
	  utcb->exc.eip += 2;
#elif ARCH_amd64
	  utcb->exc.rip += 2;
#elif ARCH_arm
	  utcb->exc.pc += 4;
#else
#error...
#endif
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

      in = get_clocks();
      for (i = global_rounds; i; i--)
        {
          l4_ipc_call_tag(pong_id,
                      L4_IPC_SHORT_MSG, d0, d1, tag,
                      L4_IPC_SHORT_MSG, &d0, &d1,
                      L4_IPC_NEVER, &dope, &rtag);
        }
      out = get_clocks();

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

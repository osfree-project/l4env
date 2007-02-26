#include <stdio.h>

#undef L4V2_IPC_SYSENTER
#undef L4X0_IPC_SYSENTER

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/util.h>
#include <l4/util/rdtsc.h>
#include <l4/util/bitops.h>

#include "global.h"
#include "idt.h"
#include "ipc_buffer.h"
#include "worker.h"
#include "pingpong.h"
#include "helper.h"

#undef PREFIX
#define PREFIX(a) int30_ ## a
#include "worker_inc.h"


/** Raise exceptions */
void __attribute__((noreturn))
ping_exception_thread(void)
{
  int i;
  l4_cpu_time_t in,out;
  static idt_t idt;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  idt.limit = 0x20*8 - 1;
  idt.base = &idt.desc;

  for (i=0; i<0x20; i++)
    MAKE_IDT_DESC(idt, i, 0);

  MAKE_IDT_DESC(idt, 13, dummy_exception13_handler);

#if DO_DEBUG
  printf("Installing IDT...\n");
#endif

  asm volatile ("lidt (%%eax)\n\t" : : "a" (&idt));

#if DO_DEBUG
  hello("ping_exceptioner");
#endif

  in = l4_rdtsc();
  for (i = 8*ROUNDS; i; i--)
    {
      // this int is reflected to int 13 because the user must not
      // direct call int 13
      asm volatile ("int $13" : : : "memory");
    }
  out = l4_rdtsc();
  
  printf("  %9u cycles / %5u rounds >> %u <<\n",
	 (l4_uint32_t)(out-in), ROUNDS*8, (l4_uint32_t)((out-in)/(ROUNDS*8)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}


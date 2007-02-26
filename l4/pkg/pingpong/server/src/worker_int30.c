#include <stdio.h>

#undef L4V2_IPC_SYSENTER
#undef L4X0_IPC_SYSENTER

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/ktrace.h>
#include <l4/util/util.h>
#include <l4/util/rdtsc.h>
#include <l4/util/bitops.h>
#include <l4/util/idt.h>

#include "global.h"
#include "ipc_buffer.h"
#include "worker.h"
#include "pingpong.h"
#include "helper.h"

#undef PREFIX
#define PREFIX(a) int30_ ## a
#include "worker_inc.h"

void dummy_exception13_handler(void);
asm (".align 16                                   \n\t"
     "dummy_exception13_handler:                  \n\t"
     "add    $4, %esp   # pop error code          \n\t"
     "add    $2, (%esp) # next instr after int 13 \n\t"
     "iret                                        \n\t");

/** Raise exceptions */
void __attribute__((noreturn))
ping_exception_thread(void)
{
  int i;
  l4_cpu_time_t in,out;
  static struct
    {
      l4util_idt_header_t header;
      l4util_idt_desc_t   desc[0x20];
    } __attribute__((packed)) idt;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  l4util_idt_init (&idt.header, 0x20);
  l4util_idt_entry(&idt.header, 13, dummy_exception13_handler);
  l4util_idt_load (&idt.header);

  in = l4_rdtsc();
  for (i = 8*global_rounds; i; i--)
    {
      // this int is reflected to int 13 because the user must not
      // direct call int 13
      asm volatile ("int $13" : : : "memory");
    }
  out = l4_rdtsc();
  
  printf("  %9u cycles / %5u rounds >> %u <<\n",
	 (l4_uint32_t)(out-in), global_rounds*8, 
	 (l4_uint32_t)((out-in)/(global_rounds*8)));

  /* tell main that we are finished */
  PREFIX(send)(main_id);
  PREFIX(recv)(main_id);

  /* done, sleep */
  l4_sleep_forever();
}


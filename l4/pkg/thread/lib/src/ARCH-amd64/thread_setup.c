#include "__thread.h"

void
l4th_thread_setup_arch(l4th_tcb_t * tcb)
{
  // initialize the FPU (disable exceptions), because sse registers are used
  // by compiler generated code
  unsigned mx = 0x1f80;
  __asm__ __volatile__ (" finit; ldmxcsr %0" : : "m"(mx) );
}


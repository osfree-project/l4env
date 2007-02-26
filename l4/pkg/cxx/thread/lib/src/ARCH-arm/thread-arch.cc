#include <l4/cxx/thread.h>
#include <l4/util/util.h>

void L4_cxx_start(void);

void L4_cxx_start(void)
{
  asm volatile (".global L4_Thread_start_cxx_thread \n"
                "L4_Thread_start_cxx_thread:        \n"
                "ldmib sp!, {r0}                    \n"
                "ldr pc,1f                          \n"
                "1: .word L4_Thread_execute         \n");
}


/*!
 * \file   log/examples/threads/l4.c
 * \brief  Showing that the loglib is thread-safe.
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * So the outputs of one programm wont demolish the others. But it still can
 * happen that you get in mess with other tasks. See threads.server-example
 * for that.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/log/l4log.h>

#define TASKS 10
#define STACKSIZE 4096

asm(".globl printf;");

char stack[STACKSIZE*TASKS];

static void work(void*data){
  int i=0;
  
  while(1){
    LOGL("we are running, iteration is %d, thread 0x%02X",i++,l4_myself().id.lthread);
  }
}

int main(int argc, char**argv){
  int i;
  
  l4_uint32_t dummy;
  l4_threadid_t tid, preempter, pager;

  LOG("we are alive, creating other threads now");
  memset(stack,0,sizeof(stack));
  tid=l4_myself();
  for(i=0;i<TASKS;i++){
    tid.id.lthread++;
    preempter = L4_INVALID_ID;
    pager = L4_INVALID_ID;
    l4_thread_ex_regs(tid, (l4_umword_t)work, ((l4_umword_t)(&stack))+STACKSIZE*(i+1),
                      &preempter, &pager, &dummy, &dummy, &dummy);
    LOG("Thread %d is running", tid.id.lthread);
  }
  LOG("Sleeping now");

  return 0;
}

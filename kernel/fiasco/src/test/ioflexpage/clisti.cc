/* $Id$
 *
 * this is a l4 testcase (a theoretical contribution to Fiasco ;-)
 * 
 */

#include "iotest.h"

char * io_test_name = "clisti";

// save EFLAGS
inline
unsigned
eflags_save(void)
{
  unsigned eflags;
  
  asm volatile("pushfl ; popl %0" : "=g" (eflags));
  return eflags;
}


void test_task(void)
{
  l4_threadid_t my_pager;
  // unsigned error;
  // dword_t ignore;
  // l4_msgdope_t result;

  get_thread_ids(NULL,NULL,&my_pager,NULL);

  /* start test */
  unsigned fl = eflags_save();
  send_to_controller(ERROR_TASK_IOPL, fl & EFL_IOPL);
  if(debugcontroller)
    sit_and_wait(10);
  printf("testing: IOPL : %d\n", ((fl & EFL_IOPL) >> 12) & 3);

  cli();
  fl = eflags_save();
  if((fl & EFL_IF) == 0)
    {
      send_to_controller(ERROR_TASK_INT_DISABLE,0);
      printf("testing: Ints disabled\n");
    }
  else
    {
      send_to_controller(ERROR_TASK_INT_ENABLE,0);
      printf("testing: Ints enabled\n");
    }
  
  fl = eflags_save();
  send_to_controller(ERROR_TASK_IOPL, fl & EFL_IOPL);
  printf("testing: IOPL : %d\n", ((fl & EFL_IOPL) >> 12) & 3);

  sti();
  fl = eflags_save();
  if((fl & EFL_IF) == 0)
    {
      send_to_controller(ERROR_TASK_INT_DISABLE,0);
      printf("testing: Ints disabled\n");
    }
  else
    {
      send_to_controller(ERROR_TASK_INT_ENABLE,0);
      printf("testing: Ints enabled\n");
    }
  /* end test */
}


/* controller thread */

void controller_thread(void)
{
  controller_message_t code;
  dword_t w1;
  l4_fpage_t fp;

  hello("controller");

  // debugcontroller = 1;

  /* consume code/stack page faults */
  do
    receive(&code, &w1);
  while(code == CODE_PAGE_FAULT 
	|| code == BSS_PAGE_FAULT
	|| code == STACK_PAGE_FAULT);

  // check IOPL
  controller_check(code,w1,
		   (code == ERROR_TASK_IOPL) && (w1 == EFL_IOPL_KERNEL));

  // become privileged
  receive(&code, &w1);
  fp.fpage = w1;
  controller_check(code,w1,
		   code == IO_FAULT &&
		   fp.iofp.f == 0xf &&
		   fp.iofp.iopage == 0 &&
		   fp.iofp.iosize == L4_WHOLE_IOADDRESS_SPACE);

  // cli
  receive(&code, &w1);
  controller_check(code,w1,
		   code == ERROR_TASK_INT_DISABLE);

  // check IOPL
  receive(&code, &w1);
  controller_check(code,w1,
		   (code == ERROR_TASK_IOPL) && (w1 == EFL_IOPL_USER));

  // sti
  receive(&code, &w1);
  controller_check(code,w1,
		   code == ERROR_TASK_INT_ENABLE);



  /* expect finish */
  receive(&code, &w1);
  controller_check(code, w1, code == ERROR_TASK_FINISHED);
  sit_and_wait(8);
  printf("TEST PASSED TEST PASSED TEST PASSED TEST PASSED\n");    
  wait_forever();
}
  

/* handle the exceptions */

bool handle_interrupt(void)
{
  // check for cli
  if(regs.exception_number == 13
     && (*reinterpret_cast<byte_t *>(regs.eip)) == 0xfa)
    {
      printf("GP with cli at %08lx detected, skipping instruction\n",
	     regs.eip);
      regs.eip += 1;
      send_to_controller(INT_CLI,0);
      return true;
    }

  // check for sti
  if(regs.exception_number == 13
     && (*reinterpret_cast<byte_t *>(regs.eip)) == 0xfb)
    {
      printf("GP with sti at %08lx detected, skipping instruction\n",
	     regs.eip);
      regs.eip += 1;
      send_to_controller(INT_STI,0);
      return true;
    }

  // otherwise
  return false;
}


bool handle_page_fault(dword_t fault_address, dword_t fault_eip,
		      bool write_page_fault, 
		      l4_snd_fpage_t *fp, void ** snd_desc)
{
  return false;
}



int main(void)
{
  return generic_main();
}

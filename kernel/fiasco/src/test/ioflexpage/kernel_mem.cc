/* $Id$
 *
 * this is a l4 testcase (a theoretical contribution to Fiasco ;-)
 * 
 * Test all kernel memory for reading. Successful if a 
 * page fault happens for every byte in the kernel reagion
 * 
 */

#include "iotest.h"

char * io_test_name = "kernel mem";

/* address where to start reading */
#define KERNEL_START 0xc0000000


void test_task(void)
{
  l4_threadid_t my_pager;
  unsigned error;
  dword_t ignore;
  l4_msgdope_t result;

  /* start test */
  get_thread_ids(NULL, NULL, &my_pager, NULL);
				/* IO fpage for full IO space */
  l4_fpage_t fp = l4_iofpage(0, L4_WHOLE_IOADDRESS_SPACE,0);

  printf("error task: start IO get\n");
  error = l4_i386_ipc_call(my_pager, L4_IPC_SHORT_MSG,
			   fp.fpage,
			   0,
			   L4_IPC_MAPMSG(0, L4_WHOLE_ADDRESS_SPACE),
			   &ignore, &fp.fpage,
			   L4_IPC_NEVER, &result);
  printf("error task: end IO get %08x\n", result.msgdope);
  
  if(error)
    ipc_error("error IO", error);

  if(l4_ipc_fpage_received(result) /* got something */
     && fp.iofp.f == 0xf	   /* got IO ports */
     && fp.iofp.iosize == L4_WHOLE_IOADDRESS_SPACE
     && fp.iofp.iopage == 0        /* got whole IO space */
     )
    send_to_controller(ERROR_TASK_GOT_ALL_IO,0);
  else
    send_to_controller(ERROR_TASK_GOT_SOME_IO,0);

  char * p = reinterpret_cast<char *>(KERNEL_START);
  char x;
  for( ; p != 0; p += PAGE_SIZE)
    {
      x = *p;
      send_to_controller(ERROR_INFO, x);
    }
  /* end test */
}



void controller_thread(void)
{
  dword_t w1, i;
  controller_message_t code;
  l4_fpage_t fp;

  hello("controller");

  /* consume code/stack page faults */
  do
    receive(&code, &w1);
  while(code == CODE_PAGE_FAULT 
	|| code == BSS_PAGE_FAULT
	|| code == STACK_PAGE_FAULT);

  // become privileged
  // code = receive(&w1);
  fp.fpage = w1;
  controller_check(code,w1,
		   code == IO_FAULT &&
		   fp.iofp.f == 0xf &&
		   fp.iofp.iopage == 0 &&
		   fp.iofp.iosize == L4_WHOLE_IOADDRESS_SPACE);

  receive(&code, &w1);
  controller_check(code,w1,
		   code == ERROR_TASK_GOT_ALL_IO);
  
  receive(&code, &w1);
  for(i = KERNEL_START; i != 0; i += PAGE_SIZE)
    {
      controller_check(code,w1,
		       code == INT_ACCESS
		       && w1 == i);
      receive_no_info(&code, &w1);
    }

  /* expect finish */
  // code = receive(&w1);
  controller_check(code, w1, code == ERROR_TASK_FINISHED);
  sit_and_wait(8);
  printf("TEST PASSED TEST PASSED TEST PASSED TEST PASSED\n");    
  wait_forever();
}
  

/* thread catching all the exceptions */

bool handle_interrupt(void)
{

  // check for GP in  movsbl (%ebx),%eax opcode 0f be 03
  if(regs.exception_number == 13
     && ((*reinterpret_cast<dword_t *>(regs.eip)) & 0x00ffffff) 
     == 0x0003be0f)
    {
      if((regs.ebx & 0xfffff) == 0)
	printf("Access at %08lx, skipping\n",
	       regs.ebx);
      send_to_controller(INT_ACCESS, regs.ebx);
      regs.eip += 3;
      return true;
    }

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

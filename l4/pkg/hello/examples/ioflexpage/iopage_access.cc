/* $Id$
 *
 * this is a l4 testcase (a theoretical contribution to Fiasco ;-)
 * 
 * access three ports, one in the first page of the io bitmap, 
 * one in the second, and the last port.
 * Access also the memory where the IO bitmap lives.
 * When accessing the last port the kernel must handle a page fault 
 * for the first byte after the IO bitmap.
 */

#include <l4/util/port_io.h>
#include "iotest.h"

#define IO_BITMAP_BASE 0xefc00000
#define IOPORT_FIRSTPAGE 0x0400
#define IOPORT_SECONDPAGE 0x8400
#define FIRST_ACCESS (IOPORT_FIRSTPAGE / 8)
#define SECOND_ACCESS (IOPORT_SECONDPAGE / 8)

char * io_test_name = "io page";

void test_task(void)
{

  /* start test */
  char * p;
  char x;
  x = 0;
  p = reinterpret_cast<char *>( IO_BITMAP_BASE + FIRST_ACCESS );
  x = *p;
  send_to_controller(ERROR_TASK_READ,x);

  
  l4util_in8(IOPORT_FIRSTPAGE);
  p = reinterpret_cast<char *>( IO_BITMAP_BASE + FIRST_ACCESS );
  x = *p;
  send_to_controller(ERROR_TASK_READ,x);

  p = reinterpret_cast<char *>( IO_BITMAP_BASE + SECOND_ACCESS );
  x = *p;
  send_to_controller(ERROR_TASK_READ,x);

  l4util_in8(IOPORT_SECONDPAGE);
  p = reinterpret_cast<char *>( IO_BITMAP_BASE + SECOND_ACCESS );
  x = *p;
  send_to_controller(ERROR_TASK_READ,x);

  p = reinterpret_cast<char *>( IO_BITMAP_BASE + L4_IOPORT_MAX / 8 );
  x = *p;
  send_to_controller(ERROR_TASK_READ,x);

  l4util_in8(L4_IOPORT_MAX -1);
  p = reinterpret_cast<char *>( IO_BITMAP_BASE + L4_IOPORT_MAX / 8 );
  x = *p;
  send_to_controller(ERROR_TASK_READ,x);
  /* end test */
}


void controller_thread(void)
{
  controller_message_t code;
  l4_umword_t w1;
  l4_fpage_t fp;

  hello("controller");

  /* consume code/stack page faults */
  do
    receive(&code, &w1);
  while(code == CODE_PAGE_FAULT 
	|| code == BSS_PAGE_FAULT
	|| code == STACK_PAGE_FAULT);

  /* expect excess on IO_BITMAP_BASE + FIRST_ACCESS */
  controller_check(code, w1, 
		   code == INT_ACCESS && 
		   w1 == IO_BITMAP_BASE + FIRST_ACCESS);
  
  receive(&code, &w1);
  controller_check(code, w1, code == ERROR_TASK_READ);

  receive(&code,&w1);
  fp.fpage = w1;
  controller_check(code, w1, 
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == IOPORT_FIRSTPAGE
		   && fp.iofp.iosize == 0);

  receive(&code, &w1);
  controller_check(code, w1, 
		   code == INT_ACCESS && 
		   w1 == IO_BITMAP_BASE + FIRST_ACCESS);
  
  receive(&code, &w1);
  controller_check(code, w1, code == ERROR_TASK_READ);


  /* expect excess on IO_BITMAP_BASE + SECOND_ACCESS */
  receive(&code, &w1);
  controller_check(code, w1, 
		   code == INT_ACCESS && 
		   w1 == IO_BITMAP_BASE + SECOND_ACCESS);
  
  receive(&code, &w1);
  controller_check(code, w1, code == ERROR_TASK_READ);

  receive(&code, &w1);
  fp.fpage = w1;
  controller_check(code, w1, 
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == IOPORT_SECONDPAGE
		   && fp.iofp.iosize == 0);

  receive(&code, &w1);
  controller_check(code, w1, 
		   code == INT_ACCESS && 
		   w1 == IO_BITMAP_BASE + SECOND_ACCESS);
  
  receive(&code, &w1);
  controller_check(code, w1, code == ERROR_TASK_READ);

  receive(&code, &w1);
  controller_check(code, w1, 
		   code == INT_ACCESS && 
		   w1 == IO_BITMAP_BASE + L4_IOPORT_MAX / 8);
  
  receive(&code, &w1);
  controller_check(code, w1, code == ERROR_TASK_READ);

  receive(&code, &w1);
  fp.fpage = w1;
  controller_check(code, w1, 
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == L4_IOPORT_MAX -1
		   && fp.iofp.iosize == 0);

  receive(&code, &w1);
  controller_check(code, w1, 
		   code == INT_ACCESS && 
		   w1 == IO_BITMAP_BASE + L4_IOPORT_MAX / 8);
  
  receive(&code, &w1);
  controller_check(code, w1, code == ERROR_TASK_READ);

  /* expect finish */
  receive(&code, &w1);
  controller_check(code, w1, code == ERROR_TASK_FINISHED);
  sit_and_wait(9);
  printf("TEST PASSED TEST PASSED TEST PASSED TEST PASSED\n");    
  wait_forever();
}
  


bool handle_interrupt(void)
{
  // printf("%06x %08x\n",
  // (* reinterpret_cast<l4_umword_t *>(regs.eip)) & 0x00ffffff,
  // (* reinterpret_cast<l4_umword_t *>(regs.eip +3)));

  // check for movsbl imm32, ??
  if(((* reinterpret_cast<l4_umword_t *>(regs.eip)) & 
      (0x0000ffff |		// op code
       0x00c70000)		// ModR/M without register
      ) ==
     (0x0000be0f |		// op code
      0x00050000)		// disp32: Mod == 00, R/M == 101
     )
    {
      l4_umword_t address = (* reinterpret_cast<l4_umword_t *>(regs.eip +3));
      printf("Acessing %08lx, skip instruction\n", address);
      regs.eip += 7;
      regs.eax = 0;
      
      send_to_controller(INT_ACCESS, address);
      return true;
    }
  return false;
}


bool handle_page_fault(l4_umword_t fault_address, l4_umword_t fault_eip,
		      bool write_page_fault, 
		      l4_snd_fpage_t *fp, void ** snd_desc)
{
  return false;
}


int main(void)
{
  return generic_main();
}

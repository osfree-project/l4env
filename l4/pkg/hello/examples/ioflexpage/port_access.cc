/* $Id$
 *
 * this is a l4 testcase (a theoretical contribution to Fiasco ;-)
 * 
 * access one IO port
 */

#include <l4/util/port_io.h>
#include "iotest.h"

char * io_test_name = "port access";

/* config data */
#define MAPPING_BASE 0x80000000

#define INB_ADDR   0x0401
#define OUTB_ADDR  0x0603
#define INW_ADDR   0x0807
#define OUTW_ADDR  0x0a0b
#define INDW_ADDR  0x0c0f
#define OUTDW_ADDR 0x0e11

#define INSB_PORT  0x0f09
#define INSB_ADDR  0x40000ff8
#define INSB_COUNT 0x10

#define INSW_PORT  0x1102
#define INSW_ADDR  0x40010ff8
#define INSW_COUNT 0x10
	   
#define INSDW_PORT  0x1204
#define INSDW_ADDR  0x40020ff8
#define INSDW_COUNT 0x10


void test_task(void)
{
  sit_and_wait(9);
  /* start test */
  l4util_in8(INB_ADDR);

  l4util_out8(77, OUTB_ADDR);

  l4util_in16(INW_ADDR);

  l4util_out16(0x1111, OUTW_ADDR);

  l4util_in32(INDW_ADDR);

  l4util_out32(0x22222222, OUTDW_ADDR);

  l4util_ins8(INSB_PORT, INSB_ADDR, INSB_COUNT);

  l4util_ins16(INSW_PORT, INSW_ADDR, INSW_COUNT);

  l4util_ins32(INSDW_PORT, INSDW_ADDR, INSDW_COUNT);
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
    receive(&code,&w1);
  while(code == CODE_PAGE_FAULT || 
	code == STACK_PAGE_FAULT ||
	code == BSS_PAGE_FAULT);
  
  // debugcontroller =1;

  /* inb(INB_ADDR); */
  fp.fpage = w1;
  controller_check(code, w1,
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == INB_ADDR
		   && fp.iofp.iosize == 0);

  /* outb(OUTB_ADDR, 77); */
  receive(&code,&w1);
  fp.fpage = w1;
  controller_check(code, w1,
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == OUTB_ADDR
		   && fp.iofp.iosize == 0);

  /* inw(INW_ADDR); */
  receive(&code,&w1);
  fp.fpage = w1;
  controller_check(code, w1,
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == INW_ADDR
		   && fp.iofp.iosize == 1);

  /* outw(OUTW_ADDR, 0x11111111); */
  receive(&code,&w1);
  fp.fpage = w1;
  controller_check(code, w1,
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == OUTW_ADDR
		   && fp.iofp.iosize == 1);

  /* inl(INDW_ADDR); */
  receive(&code,&w1);
  fp.fpage = w1;
  controller_check(code, w1,
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == INDW_ADDR
		   && fp.iofp.iosize == 2);

  /* outl(OUTDW_ADDR, 0x22222222); */
  receive(&code,&w1);
  fp.fpage = w1;
  controller_check(code, w1,
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == OUTDW_ADDR
		   && fp.iofp.iosize == 2);

  /* insb(INSB_PORT, INSB_ADDR, INSB_COUNT); */
  /* IO Port */
  receive(&code,&w1);
  fp.fpage = w1;
  controller_check(code, w1,
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == INSB_PORT
		   && fp.iofp.iosize == 0);

  /* Memory l */
  receive(&code,&w1);
  controller_check(code, w1, code == INSB_PAGE_FAULT);

  /* Memory h */
  receive(&code,&w1);
  controller_check(code, w1, code == INSB_PAGE_FAULT);


  /* insw(INSW_PORT, INSW_ADDR, INSW_COUNT); */
  /* IO Port */
  receive(&code,&w1);
  fp.fpage = w1;
  controller_check(code, w1,
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == INSW_PORT
		   && fp.iofp.iosize == 1);

  /* Memory l */
  receive(&code,&w1);
  controller_check(code, w1, code == INSW_PAGE_FAULT);

  /* Memory h */
  receive(&code,&w1);
  controller_check(code, w1, code == INSW_PAGE_FAULT);


  /* insl(INSDW_PORT, INSDW_ADDR, INSDW_COUNT); */
  /* IO Port */
  receive(&code,&w1);
  fp.fpage = w1;
  controller_check(code, w1,
		   code == IO_FAULT && fp.iofp.f == 0xf 
		   && fp.iofp.iopage == INSDW_PORT
		   && fp.iofp.iosize == 2);

  /* Memory l */
  receive(&code,&w1);
  controller_check(code, w1, code == INSDW_PAGE_FAULT);

  /* Memory h */
  receive(&code,&w1);
  controller_check(code, w1, code == INSDW_PAGE_FAULT);


  /* expect finish */
  receive(&code,&w1);
  controller_check(code, w1, code == ERROR_TASK_FINISHED);
  sit_and_wait(9);
  printf("TEST PASSED TEST PASSED TEST PASSED TEST PASSED\n");    
  wait_forever();
}
  

bool handle_interrupt(void)
{
  return false;
}


static l4_umword_t mapaddr = MAPPING_BASE;

bool handle_page_fault(l4_umword_t fault_address, l4_umword_t fault_eip,
		      bool write_page_fault, 
		      l4_snd_fpage_t *fp, void ** snd_desc)
{
  bool handle_it = false;

  /* insw(INSW_PORT, INSW_ADDR, INSW_COUNT); */
  if(INSB_ADDR <= fault_address 
     && l4_round_page(INSB_ADDR) > fault_address)
    {
      printf("INSBl");
      handle_it = true;
      send_to_controller(INSB_PAGE_FAULT,0);
    }
  else if(l4_trunc_page(INSB_ADDR + INSB_COUNT) <= fault_address 
     && (INSB_ADDR + INSB_COUNT) > fault_address)
    {
      printf("INSBh");
      handle_it = true;
      send_to_controller(INSB_PAGE_FAULT,0);
    }


  /* insw(INSW_PORT, INSW_ADDR, INSW_COUNT); */
  else if(INSW_ADDR <= fault_address 
     && l4_round_page(INSW_ADDR) > fault_address)
    {
      printf("INSWl");
      handle_it = true;
      send_to_controller(INSW_PAGE_FAULT,0);
    }
  else if(l4_trunc_page(INSW_ADDR + INSW_COUNT) <= fault_address 
     && (INSW_ADDR + INSW_COUNT) > fault_address)
    {
      printf("INSWh");
      handle_it = true;
      send_to_controller(INSW_PAGE_FAULT,0);
    }


  /* insl(INSDW_PORT, INSDW_ADDR, INSDW_COUNT); */
  if(INSDW_ADDR <= fault_address 
     && l4_round_page(INSDW_ADDR) > fault_address)
    {
      printf("INSDWl");
      handle_it = true;
      send_to_controller(INSDW_PAGE_FAULT,0);
    }
  else if(l4_trunc_page(INSDW_ADDR + INSDW_COUNT) <= fault_address 
     && (INSDW_ADDR + INSDW_COUNT) > fault_address)
    {
      printf("INSDWh");
      handle_it = true;
      send_to_controller(INSDW_PAGE_FAULT,0);
    }


  if(handle_it)
    {
      if(! get_page_at(mapaddr, L4_INVALID_ID))
	wait_forever();

      // got one page at mapaddr

      fp->fpage = l4_fpage(mapaddr, L4_LOG2_PAGESIZE, 
			  L4_FPAGE_RW, L4_FPAGE_MAP);
      mapaddr += L4_PAGESIZE;
      fp->snd_base = l4_trunc_page(fault_address);
      *snd_desc = L4_IPC_SHORT_FPAGE;

      return true;
    }
  return false;
}


int main(void)
{
  return generic_main();
}

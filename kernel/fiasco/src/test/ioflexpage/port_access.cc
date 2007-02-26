/* $Id$
 *
 * this is a l4 testcase (a theoretical contribution to Fiasco ;-)
 * 
 * access one IO port
 */

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


inline void insb(word_t port,dword_t addr, dword_t count) 
{
  int dummy1,dummy2, dummy3;
  asm volatile("REP insb \
               " : "=d" (dummy1), "=D" (dummy2), "=c" (dummy3)
	       : "d" (port), "D" (addr), "c" (count));
}

inline void insw(word_t port,dword_t addr, dword_t count) 
{
  int dummy1,dummy2, dummy3;
  asm volatile("REP insw \
               " : "=d" (dummy1), "=D" (dummy2), "=c" (dummy3)
	       : "d" (port), "D" (addr), "c" (count));
}

inline void insl(word_t port,dword_t addr, dword_t count) 
{
  int dummy1,dummy2, dummy3;
  asm volatile("REP insl \
               " : "=d" (dummy1), "=D" (dummy2), "=c" (dummy3)
	       : "d" (port), "D" (addr), "c" (count));
}




void test_task(void)
{
  sit_and_wait(7);
  /* start test */
  inb(INB_ADDR);

  outb(OUTB_ADDR, 77);

  inw(INW_ADDR);

  outw(OUTW_ADDR, 0x1111);

  inl(INDW_ADDR);

  outl(OUTDW_ADDR, 0x22222222);

  insb(INSB_PORT, INSB_ADDR, INSB_COUNT);

  insw(INSW_PORT, INSW_ADDR, INSW_COUNT);

  insl(INSDW_PORT, INSDW_ADDR, INSDW_COUNT);
  /* end test */
}


void controller_thread(void)
{
  controller_message_t code;
  dword_t w1;
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
  sit_and_wait(8);
  printf("TEST PASSED TEST PASSED TEST PASSED TEST PASSED\n");    
  wait_forever();
}
  

bool handle_interrupt(void)
{
  return false;
}


static dword_t mapaddr = MAPPING_BASE;

bool handle_page_fault(dword_t fault_address, dword_t fault_eip,
		      bool write_page_fault, 
		      l4_snd_fpage_t *fp, void ** snd_desc)
{
  bool handle_it = false;

  /* insw(INSW_PORT, INSW_ADDR, INSW_COUNT); */
  if(INSB_ADDR <= fault_address 
     && round_page(INSB_ADDR) > fault_address)
    {
      printf("INSBl");
      handle_it = true;
      send_to_controller(INSB_PAGE_FAULT,0);
    }
  else if(trunc_page(INSB_ADDR + INSB_COUNT) <= fault_address 
     && (INSB_ADDR + INSB_COUNT) > fault_address)
    {
      printf("INSBh");
      handle_it = true;
      send_to_controller(INSB_PAGE_FAULT,0);
    }


  /* insw(INSW_PORT, INSW_ADDR, INSW_COUNT); */
  else if(INSW_ADDR <= fault_address 
     && round_page(INSW_ADDR) > fault_address)
    {
      printf("INSWl");
      handle_it = true;
      send_to_controller(INSW_PAGE_FAULT,0);
    }
  else if(trunc_page(INSW_ADDR + INSW_COUNT) <= fault_address 
     && (INSW_ADDR + INSW_COUNT) > fault_address)
    {
      printf("INSWh");
      handle_it = true;
      send_to_controller(INSW_PAGE_FAULT,0);
    }


  /* insl(INSDW_PORT, INSDW_ADDR, INSDW_COUNT); */
  if(INSDW_ADDR <= fault_address 
     && round_page(INSDW_ADDR) > fault_address)
    {
      printf("INSDWl");
      handle_it = true;
      send_to_controller(INSDW_PAGE_FAULT,0);
    }
  else if(trunc_page(INSDW_ADDR + INSDW_COUNT) <= fault_address 
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
      mapaddr += PAGE_SIZE;
      fp->snd_base = trunc_page(fault_address);
      *snd_desc = L4_IPC_SHORT_FPAGE;

      return true;
    }
  return false;
}


int main(void)
{
  return generic_main();
}

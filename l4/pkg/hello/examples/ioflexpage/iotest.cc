/* $Id$
 *
 * small library for writing l4 testcases
 * 
 * The general idea is as follows:
 * 
 *   We start a task in a fully self controled environment, that is 
 *   with page fault handler interrupt handler. Whenever someting 
 *   interesting happens (a page fault, an interrupt, ...) a messages 
 *   is sent to a special thread, the controller thread. 
 * 
 *   If the controller receives the right messages in the right order 
 *   it prints 
 * 	TEST PASSED TEST PASSED TEST PASSED TEST PASSED
 * 
 *   otherwise the system halts/crashes or prints 
 * 
 * 	TEST FAILED TEST FAILED TEST FAILED TEST FAILED
 * 
 * 
 * This file contains the following utilities:
 * 
 * - definition of the controller protocol
 * 
 * - utility functions translate_code, ipc_error, sit_and_wait,
 *   wait_forever, get_next_stack, get_thread_ids, hello, 
 *   send_to_controller, receive, receive_no_info, controller_check, 
 *   page_fault_handler
 * 
 * - there is also a main function, that does the following:
 *   - fill the idt in idt_table
 *   - start the interrupt thread idt_thread that treats interrupts
 *   - start the pager
 *   - start the controller thread
 *   - get a task number via the rmgr library
 *   - start the test task, the error task
 * 
 * 
 * What you have to do to write you own test case is:
 * 
 * - write the error thread
 * - write the controller thread
 * - write interrupt handler
 * 
 * see port_access.cc for a minimal example
 */

#include "iotest.h"
#include "ptrace.h"


/* config data */
unsigned debugpager = 0;
unsigned debugcontroller = 0;

/* thread and task numbers */
const unsigned pager_thread_number = 1;
const unsigned idt_thread_number = 2;
const unsigned controller_thread_number = 3;

const unsigned error_task_number = 10;

/* controller protocol 
   send an opcode in w0 and optional additional data in w1
*/

char * translate_code(controller_message_t code)
{
  switch(code)
    {
    case ERROR_TASK_FINISHED:  
      return "ERROR_TASK_FINISHED";
    case CODE_PAGE_FAULT:  
      return "CODE_PAGE_FAULT";
    case UNHANDLED_INT:  
      return "UNHANDLED_INT";
    case STACK_PAGE_FAULT:  
      return "STACK_PAGE_FAULT";
    case UNHANDLED_PAGE_FAULT:  
      return "UNHANDLED_PAGE_FAULT";
    case IO_FAULT:  
      return "IO_FAULT";
    case BSS_PAGE_FAULT:  
      return "BSS_PAGE_FAULT";
    case INT_ACCESS:  
      return "INT_ACCESS";
    case ERROR_TASK_READ:  
      return "ERROR_TASK_READ";
    case ERROR_TASK_INT_DISABLE:
      return "ERROR_TASK_INT_DISABLE";
    case ERROR_TASK_INT_ENABLE: 
      return "ERROR_TASK_INT_ENABLE";
    case ERROR_TASK_GOT_ALL_IO: 
      return "ERROR_TASK_GOT_ALL_IO";
    case ERROR_TASK_GOT_SOME_IO: 
      return "ERROR_TASK_GOT_SOME_IO";
    case ERROR_TASK_IOPL: 
      return "ERROR_TASK_IOPL";
    case INT_CLI: 
      return "INT_CLI";
    case INT_STI: 
      return "INT_STI";
    case ERROR_INFO:
      return "ERROR_INFO";
    case INSB_PAGE_FAULT:
      return "INSB_PAGE_FAULT";
    case INSW_PAGE_FAULT:
      return "INSW_PAGE_FAULT";
    case INSDW_PAGE_FAULT:
      return "INSDW_PAGE_FAULT";
    default :
      return "??";
    }
}



/* global data */

/* register file */
pt_regs_t regs;

/* the table of interrupt entries defined in int_entry.S */
const unsigned idt_gates = 20;
extern l4_addr_t idt_address_table[idt_gates];

/* the interrupt descripor table */
static x86_gate idt_table[idt_gates];

/* global thread ids */
l4_threadid_t idt_thread_id;
l4_threadid_t controller_thread_id;

/* stack base of error task */
l4_addr_t error_task_stack_base = 0;


/* utility functions */

/* nicely report ipc errors */
void ipc_error(const char * msg, int error)
{
  if(error == 0){
    printf("%s : ok\n", msg);
  }
  else {
    if( error == L4_IPC_ENOT_EXISTENT ){
      printf("%s : L4_IPC_ENOT_EXISTENT "
	     "(Non-existing destination or source)\n", msg);
    }
    if( error == L4_IPC_RETIMEOUT ){
      printf("%s : L4_IPC_RETIMEOUT (Timeout during receive)\n", msg);
    }
    if( error == L4_IPC_SETIMEOUT ){
      printf("%s : L4_IPC_SETIMEOUT (Timeout during send)\n", msg);
    }
    if( error == L4_IPC_RECANCELED ){
      printf("%s : L4_IPC_RECANCELED (Receive operation cancelled)\n", msg);
    }
    if( error == L4_IPC_SECANCELED ){
      printf("%s : L4_IPC_SECANCELED (Send operation cancelled)\n", msg);
    }
    if( error == L4_IPC_REMAPFAILED ){
      printf("%s : L4_IPC_REMAPFAILED (Map failed in send)\n", msg);
    }
    if( error == L4_IPC_SEMAPFAILED ){
      printf("%s : L4_IPC_SEMAPFAILED (Map failed in receive)\n", msg);
    }
    if( error == L4_IPC_RESNDPFTO ){
      printf("%s : L4_IPC_RESNDPFTO (Send pagefault timeout)\n", msg);
    }
    if( error == L4_IPC_SESNDPFTO ){
      printf("%s : L4_IPC_SESNDPFTO (?)\n", msg);
    }
    if( error == L4_IPC_RERCVPFTO ){
      printf("%s : L4_IPC_RERCVPFTO (?)\n", msg);
    }
    if( error == L4_IPC_SERCVPFTO ){
      printf("%s : L4_IPC_SERCVPFTO (Receive pagefault timeout)\n", msg);
    }
    if( error == L4_IPC_REABORTED ){
      printf("%s : L4_IPC_REABORTED (Receive operation aborted)\n", msg);
    }
    if( error == L4_IPC_SEABORTED ){
      printf("%s : L4_IPC_SEABORTED (Send operation aborted)\n", msg);
    }
    if( error == L4_IPC_REMSGCUT ){
      printf("%s : L4_IPC_REMSGCUT (Received message cut)\n", msg);
    }
    if( error == L4_IPC_SEMSGCUT ){
      printf("%s : L4_IPC_SEMSGCUT (?)\n", msg);
    }
  }
}


/* wait for 255 * 4 ^ (15 - exp)  micro seconds 
 * on my platform inside bochs exp==8 gives about one second
 */
void sit_and_wait(int exp)
{
  l4_ipc_sleep(L4_IPC_TIMEOUT(255,exp,255,exp,0,0));
}


/* shut down this thread */
void wait_forever(void)
{
  l4_ipc_sleep(L4_IPC_NEVER);
}
     

bool get_page_at(l4_umword_t mapaddr, l4_threadid_t sigma0_id)
{
  l4_snd_fpage_t fp;
  l4_threadid_t pager;
  l4_msgdope_t dope;
  int res;

  if(l4_is_invalid_id(sigma0_id))
    get_thread_ids(0, 0, &pager, 0);
  else
    pager = sigma0_id;

  res = l4_ipc_call(pager,			     // dest
			 L4_IPC_SHORT_MSG,	     // snd desc
			 0xfffffffc,		     // get arb. page
			 0,			     // ~ w1
						     // rcv desc
			 L4_IPC_MAPMSG(mapaddr, L4_LOG2_PAGESIZE),
			 & fp.snd_base,
			 & fp.fpage.fpage,
			 L4_IPC_NEVER,		     // timeout
			 &dope);		     // dope
  if(res || !l4_ipc_fpage_received(dope))
    {
      ipc_error( l4_ipc_fpage_received(dope) ? 
		 "get page" : "get page : no fpage",
		 res);
      return false;
    }
  if(fp.fpage.fp.size != L4_LOG2_PAGESIZE)
    {
      printf("get_page : sent more than one page ??\n");
      wait_forever();
    }
  return true;
}


/* some spare stacks */
const unsigned max_stack = 10;
const unsigned stack_size = L4_PAGESIZE;
char stack2[max_stack * stack_size];

/* no static initialization at the moment */
l4_addr_t stack_pointer = 0;

l4_addr_t get_next_stack(void)
{
  if(stack_pointer <= reinterpret_cast<l4_addr_t>(stack2) + stack_size)
    {
      printf("Assertion stack pointer underflow\n");
      printf("stack2 %08lx stack_pointer %08lx\n",
	     reinterpret_cast<l4_addr_t>(stack2), stack_pointer);
      wait_forever();
    }

  l4_addr_t res = stack_pointer;
  stack_pointer -= stack_size;
  // printf("next stack %08x\n", res);
  return res;
}
  



/* get thread number, preempter, pager, and ESP of calling thread */
int get_thread_ids(l4_threadid_t *my_id, 
		   l4_threadid_t *my_preempter, 
		   l4_threadid_t *my_pager,
		   l4_umword_t *esp)
{
  l4_umword_t ignore, _esp;
  l4_threadid_t _my_id, _my_preempter, _my_pager;

  _my_id = l4_myself(); 

  _my_preempter = L4_INVALID_ID;
  _my_pager = L4_INVALID_ID;

  l4_thread_ex_regs( _my_id, ~0, ~0,	     /* id, EIP, ESP */
		     &_my_preempter,	     /* preempter */
		     &_my_pager,	     /* pager */
		     &ignore,		     /* flags */
		     &ignore,		     /* old ip */
		     &_esp		     /* old sp */
		     );

  if(my_id) *my_id = _my_id;
  if(my_preempter) *my_preempter = _my_preempter;
  if(my_pager) *my_pager = _my_pager;
  if(esp) *esp = _esp;
  return 0;
}


/* say hello */
void hello(char * id)
{
  l4_threadid_t my_id, my_preempter, my_pager;
  l4_umword_t esp;

  if(get_thread_ids(&my_id, &my_preempter, &my_pager, &esp))
    printf("get_thread_ids faild\n");

  printf("This is %s, task %x, thread %u, "
	 "preempter %x.%02x, pager %x.%02x, stack %08lx\n",
	 id,
	 my_id.id.task, my_id.id.lthread, 
	 my_preempter.id.task, my_preempter.id.lthread, 
	 my_pager.id.task, my_pager.id.lthread,
	 esp);
}


void send_to_controller(controller_message_t w0, l4_umword_t w1)
{
  int res;
  l4_msgdope_t dope;

  res = l4_ipc_send(controller_thread_id,
			 L4_IPC_SHORT_MSG,
			 w0, w1,
			 L4_IPC_TIMEOUT(255,10,255,10,0,0),
			 &dope);

  if(!res)
    return;

  if(res == L4_IPC_SETIMEOUT || res == L4_IPC_RESNDPFTO)
    printf("\ncontroller timeout with %d(%s) (%08lx)\n", 
	   w0, translate_code(w0), w1);
  else
    ipc_error("send to crtl", res);
  printf("TEST FAILED TEST FAILED TEST FAILED TEST FAILED\n");
  wait_forever();
}


void receive(controller_message_t * code, l4_umword_t * w1)
{
  int res;
  l4_msgdope_t dope;
  l4_threadid_t src;
  l4_umword_t _w0, _w1;

  res = l4_ipc_wait(&src,
			 L4_IPC_SHORT_MSG,
			 & _w0, &_w1,
			 L4_IPC_NEVER,
			 &dope);
  if(res)
    ipc_error("crtl receive", res);
  *code = static_cast<controller_message_t>(_w0);
  if(w1) *w1 = _w1;
  if(debugcontroller)
    printf("\nReceived %ld(%s) (%08lx)\n", _w0, 
	   translate_code(static_cast<controller_message_t>(_w0)),
	   _w1);
}

void receive_no_info(controller_message_t *code, l4_umword_t *w1)
{
  do
    receive(code, w1);
  while(*code == ERROR_INFO);
}


/* controller thread */

void controller_check(controller_message_t code, l4_umword_t w1, bool b)
{
  if(! b)
    {
      sit_and_wait(9);
      printf("\ncontroller got unexpected %d(%s) (%08lx)\n", code, 
	     translate_code(code), w1); 
      printf("TEST FAILED TEST FAILED TEST FAILED TEST FAILED\n");
      wait_forever();
    }
}

/* thread catching all the exceptions */

void idt_thread(void)
{
  int res;
  l4_threadid_t src;
  l4_umword_t ignore;
  l4_msgdope_t dope;

  hello("idt");

  l4_threadid_t my_id = l4_myself();
  idt_thread_id = my_id;

  /* get first int */
  res = l4_ipc_wait(&src,			 /* source */
			 L4_IPC_SHORT_MSG,	 /* rcv desc */
			 & ignore,
			 & ignore,
			 L4_IPC_NEVER,		 /* timeout */
			 &dope);		 /* dope */

  if(res)
    ipc_error("first idt", res);

  while(1)
    {
      //      printf("%06x %08x\n",
      //     (* reinterpret_cast<l4_umword_t *>(regs.eip)) & 0x00ffffff,
      //     (* reinterpret_cast<l4_umword_t *>(regs.eip +3)));
      

      if( ! handle_interrupt() )
	{
	  printf("Int %ld, error %08lx in thread %x.%02x at EIP %08lx\n",
		 regs.exception_number,
		 regs.error_code,
		 src.id.task, src.id.lthread,
		 regs.eip
		 );
	  printf("State\n"
		 "EAX : %08lx   EBX : %08lx   ECX : %08lx   EDX : %08lx\n"
		 "EAX': %08lx   ESI : %08lx   EDI : %08lx \n"
		 "EIP : %08lx   CS  :     %04x   ESP : %08lx   EBP : %08lx\n"
		 "EFLAGS : %08lx   EXC No : 0x%lx   Code : %08lx\n",
		 regs.eax, regs.ebx, regs.ecx, regs.edx,
		 regs.orig_eax, regs.esi, regs.edi,
		 regs.eip, regs.__xcs, regs.esp, regs.ebp,
		 regs.eflags, regs.exception_number, regs.error_code);

	  send_to_controller(UNHANDLED_INT, regs.exception_number);
	}

      /* send reply and wait for next int */
      res = l4_ipc_reply_and_wait
	(src,					 /* destination */
	 L4_IPC_SHORT_MSG,			 /* snd desc */
	 0,					 /* snd data 0 */
	 0 ,					 /* snd data 1 */
	 &src,					 /* source */
	 L4_IPC_SHORT_MSG,			 /* rcv desc */
	 &ignore,
	 &ignore,
	 L4_IPC_NEVER,
	 &dope);

      if(res)
	ipc_error("idt", res);
    }
}

#define my_set_idt(pseudo_desc) \
    ({ int dummy; \
	asm volatile("\
             lidt (%%eax) \
        " : "=a" (dummy) : "a" (&((pseudo_desc)->limit))); \
    })

void setup_test_task(void)
{
  pseudo_descriptor desc;

  /* set the interrupt table */
  desc.limit = idt_gates * 8 - 1;
  desc.linear_base = reinterpret_cast<l4_addr_t>(idt_table);
  my_set_idt(&desc);
  hello("tester");

  /* do test */
  test_task();

  send_to_controller(ERROR_TASK_FINISHED,0);
  wait_forever();
}



/* The pager for the error task >= 6 */

static void pager_thread(void)
{
  int res, rw_page_fault;
  l4_threadid_t src, my_pager;
  l4_umword_t fault_address, fault_eip, ignore;
  l4_snd_fpage_t fp;
  void * snd_desc = L4_IPC_SHORT_MSG;
  l4_msgdope_t dope;
  bool handled_by_client;

  hello("pager");
  get_thread_ids(0,0,&my_pager,0);

  /* get first page fault */
  res = l4_ipc_wait(&src,			 /* source */
			 L4_IPC_SHORT_MSG,	 /* rcv desc */
			 & fault_address,
			 & fault_eip,		 /* data */
			 L4_IPC_NEVER,		 /* timeout */
			 &dope);		 /* dope */

  if(res)
    ipc_error("first PF", res);

  while(1)
    {
      rw_page_fault = fault_address & 0x02;
      fault_address &= 0xfffffffc;
      handled_by_client = false;

      printf("[%s page fault in %x.%02x at %08lx (EIP %08lx) ",
	     rw_page_fault ? "write" : "read",
	     src.id.task, src.id.lthread,
	     fault_address, fault_eip);

      if( handle_page_fault(fault_address, fault_eip, rw_page_fault, 
			    & fp, &snd_desc ))
	handled_by_client = true;     // PF consumed

				// IO fault ?
      else if( l4_is_io_page_fault(fault_address))
	{
	  l4_fpage_t io;
	  io.fpage = fault_address;

	  printf("IO port 0x%04x", io.iofp.iopage);
	  if(io.iofp.iosize != 0)
	    printf("-0x%04x", io.iofp.iopage + (1 << io.iofp.iosize) -1);

	  fp.snd_base = 0;			 // offset is ignored
	  fp.fpage = l4_iofpage(io.iofp.iopage, io.iofp.iosize, 
				L4_FPAGE_MAP);
	  snd_desc = L4_IPC_SHORT_FPAGE;

	  send_to_controller(IO_FAULT, fp.fpage.fpage);
	}

				/* in Code ? */
      else if((l4_umword_t) & _start <= fault_address 
	 && ((l4_umword_t) & _etext > fault_address)
	 && !rw_page_fault)
	{
	  printf("Code");
	  fp.fpage = l4_fpage(fault_address, L4_LOG2_PAGESIZE, 
			      L4_FPAGE_RO, L4_FPAGE_MAP);
	  fp.snd_base = l4_trunc_page(fault_address);
	  snd_desc = L4_IPC_SHORT_FPAGE;

	  send_to_controller(CODE_PAGE_FAULT,0);
	}
       else if((l4_umword_t) & _etext <= fault_address 
	 && ((l4_umword_t) & _edata > fault_address)
	 && !rw_page_fault)
	{
	  printf("Data");
	  fp.fpage = l4_fpage(fault_address, L4_LOG2_PAGESIZE, 
			      L4_FPAGE_RW, L4_FPAGE_MAP);
	  fp.snd_base = l4_trunc_page(fault_address);
	  snd_desc = L4_IPC_SHORT_FPAGE;

	  send_to_controller(CODE_PAGE_FAULT,0);
	}
      
				/* in BSS ? */
      else if((l4_umword_t) & _edata <= fault_address 
	 && ((l4_umword_t) & _end > fault_address))
	{
	  printf("BSS");
	  fp.fpage = l4_fpage(fault_address, L4_LOG2_PAGESIZE, 
			      L4_FPAGE_RW, L4_FPAGE_MAP);
	  fp.snd_base = l4_trunc_page(fault_address);
	  snd_desc = L4_IPC_SHORT_FPAGE;

	  send_to_controller(BSS_PAGE_FAULT,0);
	}

				/* in Stack ? */
      else if ( fault_address < error_task_stack_base &&
		fault_address >= error_task_stack_base - stack_size)
	{
	  printf("Stack");
	  
	  fp.fpage = l4_fpage(error_task_stack_base - stack_size, 
			      L4_LOG2_PAGESIZE, L4_FPAGE_RW, L4_FPAGE_MAP);
	  fp.snd_base = l4_trunc_page(fault_address);
	  snd_desc = L4_IPC_SHORT_FPAGE;

	  send_to_controller(STACK_PAGE_FAULT,0);
	}

				/* somewhere else !! */
      else
	{
	  printf("\n stop pager]\n");
	  
	  send_to_controller(UNHANDLED_PAGE_FAULT, fault_address);

	  wait_forever();
	}
      

      // Sometimes we get a page fault for a page even we have not
      // accessed yet. Just get every page from our pager to be on 
      // the safe side.
      if((!handled_by_client) && snd_desc == L4_IPC_SHORT_FPAGE)
	{
	  l4_umword_t w0;

	  if(fp.fpage.iofp.f == 0xf)
	    w0 = fp.fpage.fpage;
	  else
	    w0 = fp.fpage.fp.page << L4_PAGESHIFT | fp.fpage.fp.write;

	  /* get this page from my pager */
	  res = l4_ipc_call(my_pager,		  // destination
				 L4_IPC_SHORT_MSG,	  // send descriptor
				 w0,			  // page fault address
				 0,			  // faulting EIP
							  // rcv descriptor
				 L4_IPC_MAPMSG(0, L4_WHOLE_ADDRESS_SPACE), 
				 &ignore,		  // rcv words
				 &ignore,
				 L4_IPC_NEVER,		  // timeout
				 &dope);
	  if(res | !l4_ipc_fpage_received(dope)){
	    ipc_error( l4_ipc_fpage_received(dope) ? 
		       "\nget page" : "\npager(no fpage)",
		       res);
	  }
	}
      
      if(debugpager)
	{
	  if(snd_desc == L4_IPC_SHORT_FPAGE)
	    if(fp.fpage.iofp.f == 0xf)
	      printf("\n io fpage %s, port : %08x, size : %d]\n",
		   fp.fpage.iofp.grant ? "grant" : "map",
		   fp.fpage.iofp.iopage,
		   fp.fpage.iofp.iosize
		   );
	    else
	      printf("\n mem fpage %s (%s), base : %08x, size : %d, "
		     "offset : %08lx]\n",
		     fp.fpage.fp.grant ? "grant" : "map",
		     fp.fpage.fp.write ? "WR" : "RO",
		     fp.fpage.fp.page << L4_PAGESHIFT,
		     fp.fpage.fp.size,
		     fp.snd_base
		     );
	  else
	    printf("\n word0 : %08lx, word 1 : %08lx]\n",
		   fp.snd_base,
		   fp.fpage.fpage);
	}
      else
	printf("]\n");

#define EFFICIENT_PAGER
#ifdef EFFICIENT_PAGER
      /* send reply and wait for next page fault */
      res = l4_ipc_reply_and_wait
	(src,					 /* destination */
	 snd_desc,				 /* snd desc */
	 fp.snd_base,				 /* snd data 0 */
	 fp.fpage.fpage ,			 /* snd data 1 */
	 &src,					 /* source */
	 L4_IPC_SHORT_MSG,			 /* rcv desc */
	 &fault_address,
	 &fault_eip,
	 L4_IPC_NEVER,
	 &dope);

      if(res)
	ipc_error("PF", res);
#else
      res = l4_ipc_send(src,
			     snd_desc,
			     fp.snd_base,
			     fp.fpage.fpage,
			     L4_IPC_NEVER,
			     &dope);
      if(res)
	ipc_error("PF send", res);

      res = l4_ipc_wait(&src,		 /* source */
			     L4_IPC_SHORT_MSG,	 /* rcv desc */
			     & fault_address,
			     & fault_eip,	 /* data */
			     L4_IPC_NEVER,	 /* timeout */
			     &dope);		 /* dope */

      if(res)
	ipc_error("PF rcv", res);
#endif /* EFFICIENT_PAGER */
    }
}


void init_get_next_stack(void)
{
  // do some initialization
  stack_pointer = (reinterpret_cast<l4_addr_t>(stack2) +
		   max_stack * stack_size) 
    & L4_PAGEMASK;
}

l4_threadid_t start_idt_thread(void)
{
  l4_umword_t ignore;
  l4_threadid_t my_id, my_preempter, my_pager, idt_id;

  get_thread_ids(&my_id,&my_preempter,&my_pager,0);

  idt_id = my_id;
  idt_id.id.lthread = idt_thread_number;
  l4_thread_ex_regs( idt_id,			 /* dest thread */
		     (l4_umword_t)idt_thread,	 /* EIP */
		     get_next_stack(),
						 /* ESP */
		     &my_preempter,		 /* preempter */
		     &my_pager,			 /* pager */
		     &ignore,			 /* flags */
		     &ignore,			 /* old ip */
		     &ignore			 /* old sp */
		     );
  return idt_id;
}

l4_threadid_t start_pager_thread(void)
{
  l4_umword_t ignore;
  l4_threadid_t my_id, my_preempter, my_pager;
  l4_threadid_t pager_thread_id;

  get_thread_ids(&my_id,&my_preempter,&my_pager,0);

  pager_thread_id = my_id;
  pager_thread_id.id.lthread = pager_thread_number;
  l4_thread_ex_regs( pager_thread_id,		 /* dest thread */
		     (l4_umword_t)pager_thread,	 /* EIP */
		     get_next_stack(),
						 /* ESP */
		     &my_preempter,		 /* preempter */
		     &my_pager,			 /* pager */
		     &ignore,			 /* flags */
		     &ignore,			 /* old ip */
		     &ignore			 /* old sp */
		     );

  return pager_thread_id;
}


l4_threadid_t start_controller_thread(void)
{
  l4_umword_t ignore;
  l4_threadid_t my_id, my_preempter, my_pager;

  get_thread_ids(&my_id,&my_preempter,&my_pager,0);

  controller_thread_id = my_id;
  controller_thread_id.id.lthread = controller_thread_number;
  l4_thread_ex_regs( controller_thread_id,	 /* dest thread */
		     (l4_umword_t)controller_thread, /* EIP */
		     get_next_stack(),
						 /* ESP */
		     &my_preempter,		 /* preempter */
		     &my_pager,			 /* pager */
		     &ignore,			 /* flags */
		     &ignore,			 /* old ip */
		     &ignore			 /* old sp */
		     );

  return controller_thread_id;
}

int generic_main(void)
{
  int res;
  unsigned i;
  l4_threadid_t my_id;
  l4_threadid_t pager_thread_id, error_task_id;

  hello(io_test_name);
  get_thread_ids(&my_id,0,0,0);
  printf("start %08lx, end %08lx, edata %08lx\n",
	 reinterpret_cast<l4_umword_t>(&_start), 
	 reinterpret_cast<l4_umword_t>(&_end), 
	 reinterpret_cast<l4_umword_t>(&_edata));

  init_get_next_stack();

  // fill the idt 
  for(i = 0; i<idt_gates; i++)
    {
      fill_gate(idt_table +i, idt_address_table[i], 0,
		      SZ_32 | ACC_TRAP_GATE, 0);
    }
  // disable int3 reflection -- this is used for terminal IO
  fill_gate(idt_table +3, 0,0,0,0);
  // disable int1 reflection -- allow break points and kernel debugger
  fill_gate(idt_table +1, 0,0,0,0);

  /* start the interrupt thread */
  start_idt_thread();
  sit_and_wait(9);

  /* start pager */
  pager_thread_id = start_pager_thread();
  sit_and_wait(9);

  /* start control thread */
  start_controller_thread();
  sit_and_wait(9);

  /* get a task number */
  res = rmgr_init();
  if( res != 1){
    printf("rmgr_init failed with %x\n", res);
    wait_forever();
  }

  res = rmgr_get_task(error_task_number);
  if(res)
    {
      printf("could not aquire rights for task %d\n", error_task_number);
      wait_forever();
    }

  /* start error task */
  error_task_id = my_id;
  error_task_id.id.task = error_task_number;
  error_task_stack_base = get_next_stack();
  printf("Starting test task, idt : %08lx-%08lx, ESP : %08lx\n",
	 reinterpret_cast<l4_umword_t>(idt_table), 
	 reinterpret_cast<l4_umword_t>(idt_table+idt_gates), 
	 error_task_stack_base);
  error_task_id = 
    l4_task_new(error_task_id,			 /* dest task */
		0,				 /* prio */
		error_task_stack_base,		 /* ESP */
		(l4_umword_t)setup_test_task,	 /* EIP */
		pager_thread_id );		 /* pager */

  wait_forever();
  return 0;
}


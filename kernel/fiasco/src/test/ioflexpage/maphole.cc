/* $Id$
 *
 * this is a l4 testcase (a theoretical contribution to Fiasco ;-)
 * 
 */

#include "iotest.h"
#include <assert.h>

#ifdef BOCHS
#define LONG_WAIT 7
#define SHORT_WAIT 10
#else
#define LONG_WAIT 8
#define SHORT_WAIT 12
#endif

bool handle_interrupt(void)
{
  return false;
}


bool handle_page_fault(dword_t fault_address, dword_t fault_eip,
		      bool write_page_fault, 
		      l4_snd_fpage_t *fp, void ** snd_desc)
{
  return false;
}


/* the table of interrupt entries defined in int_entry.S */
const unsigned idt_gates = 20;
extern vm_offset_t idt_address_table[idt_gates];

/* the interrupt descripor table */
static x86_gate idt_table[idt_gates];


l4_threadid_t main_thread_id, pager_thread_id, interrupt_server_thread_id;
l4_threadid_t sigma0_id, friedrich_id, hans_id, annemarie_id;
l4_threadid_t axel_id, susi_id;

#define my_set_idt(pseudo_desc) \
    ({ int dummy; \
	asm volatile("\
             lidt (%%eax) \
        " : "=a" (dummy) : "a" (&((pseudo_desc)->limit))); \
    })

void setup_child_task(char * name)
{
  pseudo_descriptor desc;

  /* set the interrupt table */
  desc.limit = idt_gates * 8 - 1;
  desc.linear_base = reinterpret_cast<vm_offset_t>(idt_table);
  my_set_idt(&desc);
  hello(name);
}


const int debugcommunication = 2;

char * task_to_name(l4_threadid_t task)
{
  if(task.id.task == main_thread_id.id.task)
    return "main";
  else if(task.id.task == friedrich_id.id.task)
    return "friedrich";
  else if(task.id.task == hans_id.id.task)
    return "hans";
  else if(task.id.task == annemarie_id.id.task)
    return "annemarie";
  else if(task.id.task == axel_id.id.task)
    return "axel";
  else if(task.id.task == susi_id.id.task)
    return "susi";
  else
    return "??";
}

void sent_to(l4_threadid_t dest, dword_t w0)
{
  int res;
  l4_msgdope_t dope;
  l4_threadid_t my_id;
  
  if(debugcommunication > 10)
    {
      get_thread_ids(&my_id, 0,0,0);
      printf("%s : send %d to %s\n",
	     task_to_name(my_id), w0, task_to_name(dest));
    }

  res = l4_i386_ipc_send(dest,
			 L4_IPC_SHORT_MSG, // send desc
			 w0,
			 0,
			 L4_IPC_NEVER,
			 &dope);
  if(res){
    char msg[100];
    get_thread_ids(&my_id, 0,0,0);
    sprintf(msg, "sent %s -> %s",
	    task_to_name(my_id), task_to_name(dest));
    ipc_error(msg, res);
  }

  if(debugcommunication > 10)
    sit_and_wait(SHORT_WAIT);
}


void sent_page_to(l4_threadid_t dest, dword_t virt)
{
  int res;
    l4_threadid_t my_id;
  l4_msgdope_t dope;
  l4_snd_fpage_t fp;

  fp.snd_base = 0;
  fp.fpage.fpage = 
    (l4_fpage(virt, L4_LOG2_PAGESIZE, L4_FPAGE_RW, L4_FPAGE_MAP)).fpage;

  if(debugcommunication)
    {
      get_thread_ids(&my_id, 0,0,0);
      printf("%s : send page %08x to %s\n",
	     task_to_name(my_id), virt, task_to_name(dest));
    }

  res = l4_i386_ipc_send(dest,
			 L4_IPC_SHORT_FPAGE, // send desc
			 fp.snd_base,
			 fp.fpage.fpage,
			 L4_IPC_NEVER,
			 &dope);
  if(res){
    char msg[100];
    get_thread_ids(&my_id, 0,0,0);
    sprintf(msg, "sent %s -> %s",
	    task_to_name(my_id), task_to_name(dest));
    ipc_error(msg, res);
  }

  if(debugcommunication > 10)
    sit_and_wait(SHORT_WAIT);
}


dword_t receive_from(l4_threadid_t src, dword_t expect)
{
  int res;
  l4_msgdope_t dope;
  dword_t _w0, _w1;
  l4_threadid_t my_id;

  res = l4_i386_ipc_receive(src,
			    L4_IPC_SHORT_MSG, // send desc
			    &_w0,
			    &_w1,
			    L4_IPC_NEVER,
			    &dope);
  if(res){
    char msg[100];
    get_thread_ids(&my_id, 0,0,0);
    sprintf(msg, "received %s -> %s",
	    task_to_name(src), task_to_name(my_id));
    ipc_error(msg, res);
  }

  if(debugcommunication > 10)
    {
      get_thread_ids(&my_id, 0,0,0);
      printf("%s : receive %d from %s\n",
	     task_to_name(my_id), _w0, task_to_name(src));
    }

  if(_w0 != expect){
    get_thread_ids(&my_id, 0,0,0);

    printf("%s : receiving unexpected %d (%s -> %s)\n",
	   task_to_name(my_id), _w0, task_to_name(src), task_to_name(my_id));
    wait_forever();
  }
  
  return _w1;
}


void receive_page_from(l4_threadid_t src, dword_t mapaddr)
{
  l4_msgdope_t dope;
  int res;
  dword_t ignore;
  l4_fpage_t fp;
  l4_threadid_t my_id;

  res = l4_i386_ipc_receive(src,                     // dest
						     // rcv desc
			    L4_IPC_MAPMSG(mapaddr, L4_LOG2_PAGESIZE),
			    & ignore,
			    & fp.fpage,
			    L4_IPC_NEVER,	     // timeout
			    &dope);		     // dope
  if(res || !l4_ipc_fpage_received(dope))
    {
      ipc_error( l4_ipc_fpage_received(dope) ? 
		 "received page" : "receive page : no fpage",
		 res);
    }
  if(debugcommunication)
    {
      get_thread_ids(&my_id, 0,0,0);
      printf("%s : receive page %08x from %s\n",
	     task_to_name(my_id), fp.fp.page << PAGE_SHIFT, task_to_name(src));
    }
}



void friedrich_task(void)
{
  dword_t mappaddr = 0x20001000;

  setup_child_task("friedrich");
  sit_and_wait(SHORT_WAIT);

  receive_from(main_thread_id, 0);
  get_page_at(mappaddr, sigma0_id);

  sent_to(hans_id, 1);
  sent_page_to(hans_id, mappaddr);

  receive_from(hans_id, 100 );
  printf("mapping tree has hole\n");
  sit_and_wait(LONG_WAIT);

  sent_to(annemarie_id, 101);
  sent_page_to(annemarie_id, mappaddr);
  receive_from(annemarie_id, 102);

  printf("mapping tree is wrong\n");
  sit_and_wait(LONG_WAIT);
  
  sent_to(annemarie_id, 103);
  receive_from(annemarie_id, 104);
  printf("flushed to much\n");
  sit_and_wait(LONG_WAIT);
  
  sent_to(susi_id, 105);
  receive_from(susi_id, 106);

  send_to_controller(ERROR_TASK_FINISHED, 0);

  wait_forever();
}


void hans_task(void)
{
  dword_t mappaddr = 0x20002000;

  setup_child_task("hans");
  sit_and_wait(SHORT_WAIT);

  receive_from(friedrich_id, 1);
  receive_page_from(friedrich_id, mappaddr);

  sent_to(axel_id, 2);
  sent_page_to(axel_id, mappaddr);

  sit_and_wait(SHORT_WAIT);
  sent_to(susi_id,3);
  sent_page_to(susi_id, mappaddr);

  sit_and_wait(SHORT_WAIT);
  sent_to(axel_id,4);
  receive_from(axel_id, 5);

  sit_and_wait(SHORT_WAIT);
  sent_to(friedrich_id, 100);

  wait_forever();
}


void axel_task(void)
{
  dword_t mappaddr = 0x20003000;

  setup_child_task("axel");
  sit_and_wait(SHORT_WAIT);

  receive_from(hans_id, 2);
  receive_page_from(hans_id, mappaddr);

  receive_from(hans_id, 4);
  l4_fpage_unmap(l4_fpage(mappaddr, L4_LOG2_PAGESIZE, 0,0),
		 L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);
  printf("axel: flush page %08x\n",
	 mappaddr);
  sent_to(hans_id, 5);


  wait_forever();
}


void susi_task(void)
{
  dword_t mappaddr = 0x20004000;

  setup_child_task("susi");
  sit_and_wait(SHORT_WAIT);

  receive_from(hans_id, 3);
  receive_page_from(hans_id, mappaddr);

  receive_from(friedrich_id, 105);
  printf("susi: accessing %08x : ", mappaddr);
  dword_t x = * reinterpret_cast<dword_t *>(mappaddr);
  printf("%08x\n", x);
  sent_to(friedrich_id, 106);

  wait_forever();
}



void annemarie_task(void)
{
  dword_t mappaddr = 0x20005000;

  setup_child_task("annemarie");
  sit_and_wait(SHORT_WAIT);

  receive_from(friedrich_id, 101);
  receive_page_from(friedrich_id, mappaddr);

  sent_to(friedrich_id, 102);

  receive_from(friedrich_id, 103);
  l4_fpage_unmap(l4_fpage(mappaddr, L4_LOG2_PAGESIZE, 0,0),
		 L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);
  printf("annemarie: flush page %08x\n",
	 mappaddr);
  sent_to(friedrich_id, 104);

  wait_forever();
}



void controller_thread(void)
{
  controller_message_t code;
  dword_t w1;
  // l4_fpage_t fp;

  hello("controller");

  // debugcontroller =1;

  /* consume code/stack page faults */
  do
    receive(&code,&w1);
  while(code == CODE_PAGE_FAULT || 
	code == STACK_PAGE_FAULT ||
	code == BSS_PAGE_FAULT);
  
  /* expect finish */
  // receive(&code,&w1);
  controller_check(code, w1, code == ERROR_TASK_FINISHED);
  sit_and_wait(SHORT_WAIT);
  printf("TEST PASSED TEST PASSED TEST PASSED TEST PASSED\n");    
  wait_forever();
}


/* region of acquired task numbers, get initialised in get_tasks */
static int task_min = -1;
static int task_max = -1;
#define TASK_MAX 30

/* get all tasks */
static int get_tasks(void)
{
  int res, task;
  int got_flag = 0;
  int count = 0;

  /* use the rmgr library now */
  res = rmgr_init();
  if( res != 1){
    printf("rmgr_init failed with %x\n", res);
  }

  for(task = 0; task < TASK_MAX; task++){
    res = rmgr_get_task(task);
    if(res == 0)
      { 
	count++;
	if(got_flag == 0){
	  printf("got %d - ", task);
	  got_flag = 1;
	  if(task_min == -1) 
	    task_min = task;
	}
      }
    else {
      if(got_flag == 1){
	printf("%d\n", task);
	if(task_max == -1) 
	  task_max = task;
	got_flag = 0;
	break;
      }
    }
  }
  
  if(got_flag == 1){
    printf("%d\n", task);
    if(task_max == -1) 
      task_max = TASK_MAX +1;
  }
  return count;
}


int main(void)
{
  int tasks;
  unsigned i;
  l4_threadid_t pager_thread_id;

  dword_t friedrich_stack_base, hans_stack_base, annemarie_stack_base;
  dword_t axel_stack_base, susi_stack_base;
  
  hello("maphole");
  printf("start %08x, end %08x, edata %08x\n",
	 reinterpret_cast<dword_t>(&_start), 
	 reinterpret_cast<dword_t>(&_end), 
	 reinterpret_cast<dword_t>(&_edata));

  get_thread_ids(& main_thread_id, 0, &sigma0_id,0);
  init_get_next_stack();

  // fill the idt 
  for(i = 0; i<idt_gates; i++)
    {
      fill_gate(idt_table +i, idt_address_table[i], 0,
		      SZ_32 | ACC_TRAP_GATE, 0);
    }
  // disable int3 reflection -- this is used for terminal IO
  fill_gate(idt_table +3, 0,0,0,0);

  /* start the interrupt thread */
  interrupt_server_thread_id = start_idt_thread();
  sit_and_wait(SHORT_WAIT);

  /* start pager */
  pager_thread_id = start_pager_thread();
  sit_and_wait(SHORT_WAIT);

  
  /* start control thread */
  start_controller_thread();
  sit_and_wait(SHORT_WAIT);

  /* get task numbers */
  tasks = get_tasks();
  printf("Got %d tasks\n", tasks);


  /* start task friedrich */
  friedrich_id = main_thread_id;
  friedrich_id.id.task = task_min++;
  friedrich_stack_base = get_next_stack();
  printf("Starting task friedrich (%d), idt : %08x-%08x, ESP : %08x\n",
	 friedrich_id.id.task,
	 reinterpret_cast<dword_t>(idt_table), 
	 reinterpret_cast<dword_t>(idt_table+idt_gates), 
	 friedrich_stack_base);
  friedrich_id = 
    l4_task_new(friedrich_id,			 /* dest task */
		0,				 /* prio */
		friedrich_stack_base,		 /* ESP */
		(dword_t)friedrich_task,	 /* EIP */
		pager_thread_id );		 /* pager */
  sit_and_wait(SHORT_WAIT);

  /* start task hans */
  hans_id = main_thread_id;
  hans_id.id.task = task_min++;
  hans_stack_base = get_next_stack();
  printf("Starting task hans (%d), idt : %08x-%08x, ESP : %08x\n",
	 hans_id.id.task,
	 reinterpret_cast<dword_t>(idt_table), 
	 reinterpret_cast<dword_t>(idt_table+idt_gates), 
	 hans_stack_base);
  hans_id = 
    l4_task_new(hans_id,			 /* dest task */
		0,				 /* prio */
		hans_stack_base,		 /* ESP */
		(dword_t)hans_task,		 /* EIP */
		pager_thread_id );		 /* pager */
  sit_and_wait(SHORT_WAIT);

  /* start task axel */
  axel_id = main_thread_id;
  axel_id.id.task = task_min++;
  axel_stack_base = get_next_stack();
  printf("Starting task axel (%d), idt : %08x-%08x, ESP : %08x\n",
	 axel_id.id.task,
	 reinterpret_cast<dword_t>(idt_table), 
	 reinterpret_cast<dword_t>(idt_table+idt_gates), 
	 axel_stack_base);
  axel_id = 
    l4_task_new(axel_id,			 /* dest task */
		0,				 /* prio */
		axel_stack_base,		 /* ESP */
		(dword_t)axel_task,		 /* EIP */
		pager_thread_id );		 /* pager */
  sit_and_wait(SHORT_WAIT);

  /* start task susi */
  susi_id = main_thread_id;
  susi_id.id.task = task_min++;
  susi_stack_base = get_next_stack();
  printf("Starting task susi (%d), idt : %08x-%08x, ESP : %08x\n",
	 susi_id.id.task,
	 reinterpret_cast<dword_t>(idt_table), 
	 reinterpret_cast<dword_t>(idt_table+idt_gates), 
	 susi_stack_base);
  susi_id = 
    l4_task_new(susi_id,			 /* dest task */
		0,				 /* prio */
		susi_stack_base,		 /* ESP */
		(dword_t)susi_task,		 /* EIP */
		pager_thread_id );		 /* pager */
  sit_and_wait(SHORT_WAIT);

  /* start task annemarie */
  annemarie_id = main_thread_id;
  annemarie_id.id.task = task_min++;
  annemarie_stack_base = get_next_stack();
  printf("Starting task annemarie (%d), idt : %08x-%08x, ESP : %08x\n",
	 annemarie_id.id.task,
	 reinterpret_cast<dword_t>(idt_table), 
	 reinterpret_cast<dword_t>(idt_table+idt_gates), 
	 annemarie_stack_base);
  annemarie_id = 
    l4_task_new(annemarie_id,			 /* dest task */
		0,				 /* prio */
		annemarie_stack_base,		 /* ESP */
		(dword_t)annemarie_task,	 /* EIP */
		pager_thread_id );		 /* pager */
  sit_and_wait(SHORT_WAIT);


  sent_to(friedrich_id, 0);

  wait_forever();
}



/* privide these to satisfy the linker */
void test_task(void)
{
  assert(false);
}

char * io_test_name = "";


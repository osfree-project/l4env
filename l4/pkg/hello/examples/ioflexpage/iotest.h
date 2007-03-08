#ifndef IOTEST
#define IOTEST

#include <flux/x86/seg.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/rmgr/librmgr.h> 

#include <stdio.h>
#include "ptrace.h"

/* set to nonzero to get more verbose messages from the pager */
extern unsigned debugpager;

/* set to nonzero to get more verbose messages from receive */
extern unsigned debugcontroller;


/* the virtual address range of this program */
extern char _end, _start, _etext, _edata;


/* if you need new messages:
   1. append them at the end
   2. adjust translate_code in iotest.cc
*/
typedef enum
{
  ERROR_TASK_FINISHED,			 // no data
  CODE_PAGE_FAULT,			 // no data
  UNHANDLED_INT,			 // int number
  STACK_PAGE_FAULT,			 // no data
  UNHANDLED_PAGE_FAULT,			 // pfa address
  IO_FAULT,				 // io fpag
  BSS_PAGE_FAULT,			 // no data
  INT_ACCESS,				 // address
  ERROR_TASK_READ,			 // byte read
  ERROR_TASK_INT_DISABLE,		 // no data
  ERROR_TASK_INT_ENABLE,		 // no data
  ERROR_TASK_GOT_ALL_IO,		 // no data
  ERROR_TASK_GOT_SOME_IO,		 // no data
  ERROR_TASK_IOPL,			 // IOPL
  INT_CLI,				 // no data
  INT_STI,				 // no data
  ERROR_INFO,				 // byte read in mem
  INSB_PAGE_FAULT,			 // no data
  INSW_PAGE_FAULT,			 // no data
  INSDW_PAGE_FAULT			 // no data
} controller_message_t;


/* translate an controllerprotocoll code into something readable */
char * translate_code(controller_message_t code);

/* translate the ipc error into something readable */
void ipc_error(const char * msg, int error);

/* pause for some time, reasonable values are exp=10 (very very short)
   till exp=7 (about 4s)
*/
void sit_and_wait(int exp);

/* pause forever */
void wait_forever(void);

/* get and return the following information: threadid, id of preempter, 
   id of pager, stack pointer, 
   pass NULL for things you are not interested in
*/
int get_thread_ids(l4_threadid_t *my_id, 
		   l4_threadid_t *my_preempter, 
		   l4_threadid_t *my_pager,
		   l4_umword_t *esp);

/* get a page at mapaddr from sigma0 handler sigma0_id,
 * if sigma0_id == L4_INVALID_ID then current pager is used
 */
bool get_page_at(l4_umword_t mapaddr, l4_threadid_t sigma0_id);

/* allocate a stackframe from some static field */
l4_addr_t get_next_stack(void);

/* Call this init function before the first use of get_next_stack.
 * Necessary because somehow static initialization does not work here.
 * This function is called from main in iotest.cc, so call it only
 * if you provide your own main.
 */
void init_get_next_stack(void);

/* start generic pager thread, this is called in main */
l4_threadid_t start_pager_thread(void);

/* start generic interrupt server thread, this is called in main */
l4_threadid_t start_idt_thread(void);

/* start controller thread, this is called in generic_main */
l4_threadid_t start_controller_thread(void);


/* print the following message
   This is id, task 5, thread 7, preempter 5(0), pager 5(0), stack 00..."
*/
void hello(char * id);

/* send a message to controller thread
 * w1 is only meaningful for some message codes
 */
void send_to_controller(controller_message_t w0, l4_umword_t w1);

/* receive a message for the controller thread
 * w1 is only meaningful for some message codes
 * code must be a nonnull pointer
 */
void receive(controller_message_t *w0, l4_umword_t * w1);


/* receive until messages != ERROR_INFO
 * code must be a nonnull pointer
 */
void receive_no_info(controller_message_t *w0, l4_umword_t *w1);

/* check for condition cond, if false print some diagnostics and 
 * TEST FAILED TEST FAILED TEST FAILED TEST FAILED
 */
void controller_check(controller_message_t code, l4_umword_t w1, bool cond);

/* the controller thread*/
void controller_thread(void);

/* the function performing the tests */
void test_task(void);

/* handle an interrupt and return true if consumed, false otherwise
 * all info, including trap number and task state is in the global 
 * pt_regs struct regs
 */

extern pt_regs_t regs;

bool handle_interrupt(void);

/* set this string in your test to identify your test */
extern char * io_test_name;

/* Handle an page fault. The predefined page fault handler handles 
 * page faults in the programm image and IO page faults.
 * So this function is only necessary for special needs.
 */

extern bool handle_page_fault(l4_umword_t fault_address, l4_umword_t fault_eip,
			     bool write_page_fault, 
			     l4_snd_fpage_t *fp, void ** snd_desc);


/* A generic main function */
int generic_main(void);

#endif /* IOTEST */


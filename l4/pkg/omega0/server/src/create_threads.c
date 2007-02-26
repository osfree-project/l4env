#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/stack.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "create_threads.h"
#include "irq_threads.h"

/*!\brief Separated IRQ thread creation */
int create_threads_sync(){
    int error, i;
    l4_msgdope_t result;
    l4_umword_t dummy;
    l4_addr_t esp;
    l4_threadid_t thread = l4_myself();
    l4_threadid_t preempter = L4_INVALID_ID, 
	pager = L4_INVALID_ID,
	new_preempter, new_pager;
    char name[16];

    /* get my own preempter and pager */
    l4_thread_ex_regs_flags(l4_myself(), -1, -1, &preempter, &pager,
		            &dummy, &dummy, &dummy,
			    L4_THREAD_EX_REGS_NO_CANCEL);
    if(l4_is_invalid_id(pager)) return -1;	// no pager!
  
    for(i = 0; i < IRQ_NUMS; i++){
	thread.id.lthread = i+2;
	new_preempter=preempter;
	new_pager = pager;

	/* krishna: use dynamic stack allocation - please */
	if (!(irqs[i].stack = malloc(STACKSIZE))) {
	    LOGl("error getting %d bytes of memory", STACKSIZE);
	    if(ENTER_KDEBUG_ON_ERRORS){
		enter_kdebug("!");
	    }
	    return -1;
	}
	irqs[i].stack += STACKSIZE/sizeof(l4_umword_t);

	esp = (l4_addr_t)irqs[i].stack;

	/* put some parameters on the stack */
	l4util_stack_push_mword(&esp, i);
	l4util_stack_push_mword(&esp, 0); // kind of return address
      
	l4_thread_ex_regs(thread, (l4_umword_t)irq_handler, 
			  esp, &preempter, &pager,
			  &dummy, &dummy, &dummy);

	snprintf(name, sizeof(name), "omega0.irq%.2X", i);
	names_register_thread_weak(name, thread);

	/* receive an ipc from the thread. This informs us the
	 * corresponding irq field is initialized correctly. 
	 * After receiving this ipc it is not valid to
	 * communicate with the thread if
	 * irqs[i].available==0. */
	error = l4_ipc_receive(thread, L4_IPC_SHORT_MSG,
			       &dummy, &dummy,
			       L4_IPC_NEVER, &result);
	if(error) return -1;
    }
    return 0;
}

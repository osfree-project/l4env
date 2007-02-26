/* L4 includes */
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/log/l4log.h>

/* OSKit */
#include <malloc.h>

/* local */
#include "globals.h"
#include "create_threads.h"
#include "irq_threads.h"

/* krishna: separate thread creation */
int create_threads_sync()
{
	int error, i;
	l4_msgdope_t result;
	l4_umword_t dummy, *esp;
	l4_threadid_t thread = l4_myself();
	l4_threadid_t preempter = L4_INVALID_ID, 
		pager = L4_INVALID_ID,
                new_preempter, new_pager;

	/* get my own preempter and pager */
	l4_thread_ex_regs(l4_myself(), -1, -1, &preempter, &pager,
			  &dummy, &dummy, &dummy);
	if(l4_is_invalid_id(pager)) return -1;	// no pager!
  
	for(i = 0; i < IRQ_NUMS; i++){
		thread.id.lthread = i+1;
		new_preempter=preempter;
		new_pager = pager;

/* krishna: use dynamic stack allocation - please */
		if (!(irqs[i].stack = malloc(STACKSIZE))) {
			LOGl("error getting %d bytes of memory", STACKSIZE);
#ifdef ENTER_KDEBUG_ON_ERRORS
			enter_kdebug("!");
#endif
			return -1;
		}
		(l4_umword_t)irqs[i].stack += STACKSIZE;

		esp = irqs[i].stack;

		/* put some parameters on the stack */
		*--esp = (l4_umword_t) i;
		*--esp = 0;		// kind of return address
      
		l4_thread_ex_regs(thread, (l4_umword_t)irq_handler, 
				  (l4_umword_t) esp, &preempter, &pager,
				  &dummy, &dummy, &dummy);
                      
		/* receive an ipc from the thread. This informs us the corresponding
		   irq field is initialized correctly. After receiving this ipc it is
		   not valid to communicate with the thread if irqs[i].available==0. */
		error = l4_i386_ipc_receive(thread, L4_IPC_SHORT_MSG, &dummy, &dummy,
					    L4_IPC_NEVER, &result);
		if(error) return -1;
	}
	return 0;
}

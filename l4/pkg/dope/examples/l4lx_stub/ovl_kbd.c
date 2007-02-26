
/*** LINUX KERNEL INCLUDES ***/
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#include <l4linux/x86/l4_thread.h>
#include <l4linux/x86/ids.h>
#include <l4linux/x86/rmgr.h>

#include <linux/malloc.h>

#else

#include <asm/api/config.h>
#include <linux/slab.h>
#include <asm/l4lxapi/thread.h>

#endif /* version check */

#include <linux/input.h>

/*** L4 INCLUDES ***/
#include <l4/sys/kdebug.h>
#include <l4/names/libnames.h>
#include <l4/overlay_wm/overlay-client.h>
#include <l4/overlay_wm/input_listener-server.h>

/*** LOCAL INCLUDES ***/
#include "dropscon.h"
#include "xf86if.h"


l4_threadid_t ev_ovl_l4id;   /* thread id of the overlay input event listener */

extern int mx,my;            /* absolute mouse position, defined in dope_kbd.c */
extern struct input_dev *keybd_inp_dev;
extern struct input_dev *mouse_inp_dev;

static CORBA_Environment env;
//static char retbuf[256];
//char       *retp = retbuf;


/**********************************************************
 *** CALLBACK FUNCTIONS FOR INPUT EVENT LISTENER-SERVER ***
 **********************************************************/

/*** LISTENER SERVER FUNCTION: RECEIVE BUTTON EVENT ***/
void input_listener_button_component(CORBA_Object _dice_corba_obj,
                                     int type,
                                     int code,
                                     CORBA_Environment *_dice_corba_env) {
	/* press or release? */
	if (type == 2) type = 0;
	else type = 1;
	
	switch (code) {
	case BTN_LEFT:
	case BTN_RIGHT:
	case BTN_MIDDLE:
		input_report_key(mouse_inp_dev, code, type);
		break;
	default:
		input_report_key(keybd_inp_dev, code, type);
	}
}


/*** LISTENER SERVER FUNCTION: RECEIVE MOTION EVENT ***/
void input_listener_motion_component(CORBA_Object _dice_corba_obj,
                                     int abs_x, int abs_y,
                                     int rel_x, int rel_y,
                                     CORBA_Environment *_dice_corba_env) {
	mx = abs_x;
	my = abs_y;
	input_report_abs(mouse_inp_dev, ABS_X, mx);
	input_report_abs(mouse_inp_dev, ABS_Y, my);
}


/***********************************/
/*** ACTION LISTENER SERVER LOOP ***/
/***********************************/

void
#ifdef L4L22
ev_loop_ovl(l4_threadid_t creator_id) {
#else
ev_loop_ovl(void *data)
{ 
	l4_threadid_t creator_id = *(l4_threadid_t *)data;
#endif
	int 			ret;
	l4_uint32_t		dummy;
	l4_msgdope_t 	result;

	l4_ipc_call(creator_id,
		   L4_IPC_SHORT_MSG, 0, 0,
		   L4_IPC_SHORT_MSG, &dummy, &dummy,
		   L4_IPC_NEVER, &result);

	printk("ovl_kbd.o: serving events as %x.%02x\n",
	       ev_ovl_l4id.id.task, ev_ovl_l4id.id.lthread);

	input_listener_server_loop(NULL);
}


/*** INIT OVERLAY INPUT EVENT LISTENER ***/
int ovl_kbd_init() {
	int i;
	l4_uint32_t dummy;
	l4_msgdope_t result;
	l4_threadid_t me = l4_myself();
	static char 	listener_ident[64];

#ifdef L4L22
	unsigned int *esp;
	unsigned int stack_page;

#ifndef CONFIG_L4_THREADMANAGEMENT
#error Enable CONFIG_L4_THREADMANAGEMENT in L4Linux config!
#endif

	/* allocate stack */
	if (!(stack_page = __get_free_pages(GFP_KERNEL, 1))) {
		printk("ovl_kbd.o: can't alloc stack page for service thread\n");
		return -ENOMEM;
	}

	memset((void*)stack_page, 'S', 8192);

	esp = (unsigned int*)(stack_page + 2*PAGE_SIZE - sizeof(l4_threadid_t));

	*--((l4_threadid_t*)esp) = me;
	*--esp = 0;

  
	/* let's start the event handler */
	ev_ovl_l4id = create_thread(LTHREAD_NO_KERNEL_ANY,(void(*)(void))ev_loop_ovl, esp);

	if (l4_is_invalid_id(ev_l4id)) {
		printk("ovl_kbd.o: failed to created service thread\n");
		return -EBUSY;
	}

	put_l4_id_to_stack((unsigned int)esp, ev_ovl_l4id);

	/* the following is needed because we want to do "current->need_resched=1" */
#ifdef need_resched
#undef need_resched
#endif
	((struct task_struct*)stack_page)->tss.need_resched = &global_need_resched;

	if (rmgr_set_prio(ev_ovl_l4id, 129))  {
		destroy_thread(ev_ovl_l4id.id.lthread);
		printk("ovl_kbd.o: error setting priority of service thread\n");
		return -EBUSY;
	}
#else /* kernel version >= 2.4 */
	/*
	 *    * LTHREAD_NO_KERNEL_ANY/L4_INVALID_ID is currently not
	 *    supported
	 *       * by L4Env-L4Linux.
	 *          */
	ev_ovl_l4id = l4lx_thread_create(ev_loop_ovl,
				     NULL,
				     &me, sizeof(me),
				     129,
				     "OVL EV");
#endif

	/* shake hands with new thread */
	l4_ipc_receive(ev_ovl_l4id,
    		L4_IPC_SHORT_MSG, &dummy, &dummy,
    		L4_IPC_NEVER, &result);
	l4_ipc_send   (ev_ovl_l4id,
    		L4_IPC_SHORT_MSG, 0, 0,
    		L4_IPC_NEVER, &result);
  
	printk("ovl_kbd.o: started overlay input event listener (%x.%x)\n",
	       ev_ovl_l4id.id.task, ev_ovl_l4id.id.lthread);

	printk("ovl_kbd.o: register input listener at Overlay Server\n");
	overlay_input_listener_call(&ovl_l4id, &ev_ovl_l4id, &env);

	return 0;
}


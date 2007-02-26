/* Client for con server */

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/kernel.h>

typedef l4_uint32_t dword_t;

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)

#include <l4linux/x86/l4_thread.h>
#include <l4linux/x86/ids.h>
#include <l4linux/x86/rmgr.h>
#include <linux/malloc.h>

#else /* kernel version >= 2.4 */

#include <asm/api/config.h>
#include <linux/slab.h>
#include <asm/l4lxapi/thread.h>

#endif /* kernel version check */

#include <linux/input.h>

// prevent compiler warning
#undef ABS_MAX

// L4 includes
#include <l4/con/stream-server.h>
#include <l4/con/l4con_ev.h>
#include <l4/input/libinput.h>

#include "dropscon.h"
#include "xf86if.h"

l4_threadid_t ev_l4id;

static struct input_dev *kbd_dev   = 0;
static struct input_dev *mouse_dev = 0;

void stream_io_server_push(sm_request_t *request, 
			   const stream_io_input_event_t *event, 
			   sm_exc_t *_ev)
{
  l4input_t *input_ev = (l4input_t*) event;
  static unsigned short last_code;
  static unsigned char last_down = 0;
  
  switch(input_ev->type) 
    {
    case EV_KEY:
#ifndef CONFIG_INPUT_KEYBDEV
#warning "Enable CONFIG_INPUT_KEYBDEV to get keyboard events"
#endif
      /* filter out repeat keys */
      if (   last_down
      	  && input_ev->value
	  && (last_code == input_ev->code))
	return;
      
      last_code = input_ev->code;
      last_down = input_ev->value;
      
      input_report_key(input_ev->code > 255 ? mouse_dev : kbd_dev, 
		       input_ev->code, input_ev->value);
      break;

    case EV_REL:
#ifndef CONFIG_INPUT_MOUSEDEV
#warning "Enable CONFIG_INPUT_MOUSEDEV to get mouse events"
#endif
      /* mouse event */
      input_report_rel(mouse_dev, input_ev->code, input_ev->value);
      break;

    case EV_CON:
      switch(input_ev->code) 
	{
	case EV_CON_REDRAW:
	  if (init_done)
	    {
	      if (!xf86if_handle_redraw_event())
		dropscon_redraw_all();
	    }
	  else
	    redraw_pending = 1;
 	  break;
	case EV_CON_BACKGROUND:
	  /* turn of key repeat mode */
	  del_timer(&kbd_dev->timer);
	  xf86if_handle_background_event();
	  break;
  	}
    }
}

void
#ifdef L4L22
ev_loop(l4_threadid_t creator_id)
{
#else
ev_loop(void *data)
{
  l4_threadid_t creator_id = *(l4_threadid_t *)data;
#endif
  int ret;
  l4_umword_t dummy;
  l4_msgdope_t result;
  sm_request_t request;
  l4_ipc_buffer_t ipc_buf;

  l4_i386_ipc_call(creator_id,
		   L4_IPC_SHORT_MSG, 0, 0,
		   L4_IPC_SHORT_MSG, &dummy, &dummy,
		   L4_IPC_NEVER, &result);

  printk("kbd.o: serving events as %x.%02x\n",
	   ev_l4id.id.task, ev_l4id.id.lthread);

  flick_init_request(&request, &ipc_buf);

  /* IDL server loop */
  while (1)
    {
      result = flick_server_wait(&request);

      while (!L4_IPC_IS_ERROR(result))
	{
	  /* dispatch request */
	  ret = stream_io_server(&request);

  	  switch(ret) 
	    {
	    case DISPATCH_ACK_SEND:
	      /* reply and wait for next request */
	      result = flick_server_reply_and_wait(&request);
	      break;
				
	    default:
	      printk("kbd.o: Flick dispatch error (%d)!\n", ret);
	      
	      /* wait for next request */
	      result = flick_server_wait(&request);
	      break;
	    }
	} /* !L4_IPC_IS_ERROR(result) */

      /* Ooops, we got an IPC error -> do something */
      printk("kbd.o: Flick IPC error (%#x)", L4_IPC_ERROR(result));
    }
}

int 
con_kbd_init()
{
  int i;
  l4_umword_t dummy;
  l4_msgdope_t result;
  l4_threadid_t me = l4_myself();

#ifdef L4L22
  unsigned int *esp;
  unsigned int stack_page;

#ifndef CONFIG_L4_THREADMANAGEMENT
#error Enable CONFIG_L4_THREADMANAGEMENT in L4Linux config!
#endif
  
  /* allocate stack */
  if (!(stack_page = __get_free_pages(GFP_KERNEL, 1)))
    {
      printk("kbd.o: can't alloc stack page for service thread\n");
      return -ENOMEM;
    }

  memset((void*)stack_page, 'S', 8192);

  esp = (unsigned int*)(stack_page + 2*PAGE_SIZE - sizeof(l4_threadid_t));

  *--((l4_threadid_t*)esp) = me;
  *--esp = 0;

  /* let's start the event handler */
  ev_l4id = create_thread(LTHREAD_NO_KERNEL_ANY,
			  (void(*)(void))ev_loop, esp);

  if (l4_is_invalid_id(ev_l4id))
    {
      printk("kbd.o: failed to created service thread\n");
      return -EBUSY;
    }

  put_l4_id_to_stack((unsigned int)esp, ev_l4id);

  /* the following is needed because we want to do "current->need_resched=1" */
#ifdef need_resched
#undef need_resched
#endif
  ((struct task_struct*)stack_page)->tss.need_resched = &global_need_resched;

  if (rmgr_set_prio(ev_l4id, 129))
    {
      destroy_thread(ev_l4id.id.lthread);
      printk("kbd.o: error setting priority of service thread\n");
      return -EBUSY;
    }

#else /* kernel version >= 2.4 */

  ev_l4id = l4lx_thread_create(ev_loop,
			       NULL,
			       &me, sizeof(me),
			       129,
			       "Con EV");

#endif /* kernel version >= 2.4 */

  /* shake hands with new thread */
  l4_i386_ipc_receive(ev_l4id,
		      L4_IPC_SHORT_MSG, &dummy, &dummy,
		      L4_IPC_NEVER, &result);
  l4_i386_ipc_send   (ev_l4id,
		      L4_IPC_SHORT_MSG, 0, 0,
		      L4_IPC_NEVER, &result);
  
  if (!(kbd_dev = kmalloc(sizeof(struct input_dev), GFP_KERNEL)))
    return -ENOMEM;

  memset(kbd_dev, 0, sizeof(struct input_dev));

  kbd_dev->evbit[0]  = BIT(EV_KEY) | BIT(EV_REP);

  for (i=0; i<255; i++)
    set_bit(i, kbd_dev->keybit);
  clear_bit(0, kbd_dev->keybit);

  kbd_dev->private = 0;
  
  kbd_dev->name = "con_kbd";
  kbd_dev->idbus = BUS_USB;
  kbd_dev->idvendor = 0;
  kbd_dev->idproduct = 0;
  kbd_dev->idversion = 0;

  input_register_device(kbd_dev);

  if (!(mouse_dev = kmalloc(sizeof(struct input_dev), GFP_KERNEL)))
    return -ENOMEM;

  memset(mouse_dev, 0, sizeof(struct input_dev));

  mouse_dev->evbit[0]  = BIT(EV_KEY) | BIT(EV_REL);
  mouse_dev->relbit[0] = BIT(REL_X)  | BIT(REL_Y);

  mouse_dev->keybit[LONG(BTN_MOUSE)] = BIT(BTN_LEFT) | BIT(BTN_RIGHT) | 
		     BIT(BTN_MIDDLE) | BIT(BTN_SIDE) | BIT(BTN_EXTRA);

  mouse_dev->private = 0;
  mouse_dev->name = "con_mouse";
  mouse_dev->idbus = BUS_USB;
  mouse_dev->idvendor = 0;
  mouse_dev->idproduct = 0;
  mouse_dev->idversion = 0;

  input_register_device(mouse_dev);

  return 0;
}

void
con_kbd_exit()
{
#ifdef L4L22
  destroy_thread(ev_l4id.id.lthread);
#else
  l4lx_thread_shutdown(ev_l4id);
#endif
  input_unregister_device(kbd_dev);
  input_unregister_device(mouse_dev);
  kfree(kbd_dev);
  kfree(mouse_dev);
}


/* $Id$ */
/**
 * \file	l4con/examples/linux_stub/kbd.c
 * \brief	event handling
 *
 * \date	01/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/kernel.h>
#include <l4/util/l4_macros.h>

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
#undef KEY_F13
#undef KEY_F14
#undef KEY_F15
#undef KEY_F16
#undef KEY_F17
#undef KEY_F18
#undef KEY_F19
#undef KEY_F20
#undef KEY_F21
#undef KEY_F22
#undef KEY_F23
#undef KEY_F24
#undef KEY_KPCOMMA

// L4 includes
#include <l4/l4con/stream-server.h>
#include <l4/l4con/l4con_ev.h>
#include <l4/input/libinput.h>

#include "dropscon.h"
#include "xf86if.h"

l4_threadid_t ev_l4id;

static struct input_dev *kbd_dev   = 0;
static struct input_dev *mouse_dev = 0;

void 
stream_io_push_component(CORBA_Object _dice_corba_obj,
			 const stream_io_input_event_t *event,
			 CORBA_Server_Environment *_dice_corba_env)
{
  struct l4input *input_ev = (struct l4input*) event;
  static unsigned short last_code;
  static unsigned char  last_down = 0;

  switch(input_ev->type) 
    {
    case EV_KEY:
#ifndef CONFIG_INPUT_KEYBDEV
#warning "Enable CONFIG_INPUT_KEYBDEV to get keyboard events"
#endif
      /* filter out repeat keys */
      if (last_down && input_ev->value && last_code == input_ev->code)
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
	  foreground = 1;
	  if (!init_done)
	    {
	      redraw_pending = 1;
	      break;
	    }
	  if (!xf86if_handle_redraw_event())
    	    dropscon_redraw_all();
 	  break;

	case EV_CON_BACKGROUND:
	  foreground = 0;
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

  l4_umword_t dummy;
  l4_msgdope_t result;

  l4_ipc_call(creator_id,
		   L4_IPC_SHORT_MSG, 0, 0,
		   L4_IPC_SHORT_MSG, &dummy, &dummy,
		   L4_IPC_NEVER, &result);

  printk("kbd.o: serving events as "l4util_idfmt"\n", l4util_idstr(ev_l4id));

  stream_io_server_loop(NULL);
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

  esp = (unsigned*)(stack_page + 2*PAGE_SIZE - L4LINUX_RESERVED_STACK_TOP);
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

  put_l4_id_to_stack((unsigned)esp, ev_l4id);
  put_l4_prio_to_stack((unsigned)esp, 129);

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
  l4_ipc_receive(ev_l4id,
		      L4_IPC_SHORT_MSG, &dummy, &dummy,
		      L4_IPC_NEVER, &result);
  l4_ipc_send   (ev_l4id,
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
#ifdef L4LX26
  kbd_dev->id.bustype = BUS_USB;
  kbd_dev->id.vendor = 0;
  kbd_dev->id.product = 0;
  kbd_dev->id.version = 0;
#else
  kbd_dev->idbus = BUS_USB;
  kbd_dev->idvendor = 0;
  kbd_dev->idproduct = 0;
  kbd_dev->idversion = 0;
#endif

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
#ifdef L4LX26
  mouse_dev->id.bustype = BUS_USB;
  mouse_dev->id.vendor = 0;
  mouse_dev->id.product = 0;
  mouse_dev->id.version = 0;
#else
  mouse_dev->idbus = BUS_USB;
  mouse_dev->idvendor = 0;
  mouse_dev->idproduct = 0;
  mouse_dev->idversion = 0;
#endif

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


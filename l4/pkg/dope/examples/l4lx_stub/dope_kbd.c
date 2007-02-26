
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
#include <l4/dope/pslim-client.h>
#include <l4/dope/dope-client.h>
#include <l4/dope/dopeapp-server.h>

/*** LOCAL INCLUDES ***/
#include "dropscon.h"
#include "xf86if.h"


#define EVENT_TYPE_MOTION  2
#define EVENT_TYPE_PRESS   3
#define EVENT_TYPE_RELEASE 4

l4_threadid_t ev_l4id;
static long dope_app_id;

struct input_dev *keybd_inp_dev = 0;
struct input_dev *mouse_inp_dev = 0;

int mx, my;

static char retbuf[256];
char *retp = retbuf;

/*****************************************************/
/*** CALLBACK FUNCTIONS FOR ACTION LISTENER-SERVER ***/
/*****************************************************/


/*** RECEIVE EVENT THAT IS ASSOCIATED WITH THE PSLIM WIDGET ***/
long dopeapp_listener_event_component(CORBA_Object _dice_corba_obj,
                                      const dope_event_u *e,
                                      const char *bindarg,
                                      CORBA_Environment *_dice_corba_env)
{
  CORBA_Environment env;

  switch (e->_d)
    {
    case EVENT_TYPE_PRESS:
      switch (e->_u.press.code) {
	case BTN_LEFT:
	case BTN_RIGHT:
	case BTN_MIDDLE:
	  input_report_key(mouse_inp_dev, e->_u.press.code, 1);
	  return 0;
	case KEY_F12:
	  dope_manager_exec_cmd_call(&con_l4id, dope_app_id,
	      "pw.set(-x 300 -y 200 -w 650 -h 507 -background off)", &env);
	  return 0;
	default:
	  input_report_key(keybd_inp_dev, e->_u.press.code, 1);
	  return 0;
      }
    case EVENT_TYPE_RELEASE:
      switch (e->_u.press.code) {
	case BTN_LEFT:
	case BTN_RIGHT:
	case BTN_MIDDLE:
	  input_report_key(mouse_inp_dev, e->_u.press.code, 0);
	  return 0;
	case KEY_F12:
	  return 0;
	default:
	  input_report_key(keybd_inp_dev, e->_u.press.code, 0);
	  return 0;
      }
    case EVENT_TYPE_MOTION:
      mx = e->_u.motion.abs_x;
      my = e->_u.motion.abs_y;
      input_report_abs(mouse_inp_dev, ABS_X, mx);
      input_report_abs(mouse_inp_dev, ABS_Y, my);		
      break;
    }
  return 0;
}

static int inp_dev_open(struct input_dev *dev)
{
  printk("kbd.o: inp_dev_open for %s called\n", dev->name);
  return 0;
}

static void inp_dev_close(struct input_dev *dev)
{
  printk("kbd.o: inp_dev_close for %s called\n", dev->name);
}

/***********************************/
/*** ACTION LISTENER SERVER LOOP ***/
/***********************************/

void
#ifdef L4L22
ev_loop_dope(l4_threadid_t creator_id)
{
#else
ev_loop_dope(void *data)
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

  printk("kbd.o: serving events as %x.%02x\n",ev_l4id.id.task, ev_l4id.id.lthread);

  dopeapp_listener_server_loop(NULL);
}

static char msgbuf[4096];
void *CORBA_alloc(unsigned long size)
{
  return &msgbuf[0];
}


/*** INIT ACTION LISTENER AND DOPE-CLIENT ***/
int dope_kbd_init(void)
{
  int i;
  l4_uint32_t dummy;
  l4_msgdope_t result;
  l4_threadid_t me = l4_myself();
  CORBA_Environment env;
  static char 	listener_ident[64];

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
  ev_l4id = create_thread(LTHREAD_NO_KERNEL_ANY,(void(*)(void))ev_loop_dope, esp);

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

  ev_l4id = l4lx_thread_create(ev_loop_dope,
                               NULL,
			       &me, sizeof(me),
			       129,
			       "DOpE EV");
#endif

  /* shake hands with new thread */
  l4_ipc_receive(ev_l4id,
                 L4_IPC_SHORT_MSG, &dummy, &dummy,
		 L4_IPC_NEVER, &result);
  l4_ipc_send   (ev_l4id,
                 L4_IPC_SHORT_MSG, 0, 0,
		 L4_IPC_NEVER, &result);

  printk("kbd.o: connected to DOpE (%x.%x)\n", ev_l4id.id.task, ev_l4id.id.lthread);

  sprintf(listener_ident, "%lu %lu", (unsigned long)ev_l4id.lh.high,
                          (unsigned long)ev_l4id.lh.low);
  printk("DOpEstub(ev_loop_dope): listener_ident = %s\n", listener_ident);

  printk("DOpEstub(ev_loop_dope): register DOpE application\n");
  dope_app_id = dope_manager_init_app_call(&con_l4id,"L4Linux",
                                           listener_ident, &env);

  printk("DOpEstub(ev_loop_dope): set up pSLIM window\n");
  dope_manager_exec_cmd_call(&con_l4id,dope_app_id,"pw=new Window()", &env);
  dope_manager_exec_cmd_call(&con_l4id,dope_app_id,"pslim=new PSLIM()", &env);
  dope_manager_exec_cmd_call(&con_l4id,dope_app_id,"pslim.setmode(640,480,16)", &env);
  dope_manager_exec_cmd_call(&con_l4id,dope_app_id,"pslim.bind(\"press\",\"!\")", &env);
  dope_manager_exec_cmd_call(&con_l4id,dope_app_id,"pslim.bind(\"release\",\"!\")", &env);
  dope_manager_exec_cmd_call(&con_l4id,dope_app_id,"pslim.bind(\"motion\",\"!\")", &env);
  dope_manager_exec_cmd_call(&con_l4id,dope_app_id,"pw.set(-fitx yes -fity yes -content pslim)", &env);
  //dope_manager_exec_cmd_call(&con_l4id,dope_app_id,"pw.set(-scrollx yes -scrolly yes -content pslim)", &env);
  dope_manager_exec_cmd_call(&con_l4id,dope_app_id,"pw.set(-x 30 -y 20 -w 650 -h 507 -background off)", &env);
  dope_manager_exec_cmd_call(&con_l4id,dope_app_id,"pw.open()", &env);

  /* request identifier for the pSLIM server (written to retp) */
    {
      int retlen = sizeof(retbuf);
      dope_manager_exec_req_call(&con_l4id, dope_app_id,
	                         "pslim.getserver()",
				 retbuf, &retlen, &env);
    }

  printk("DOpEstub(ev_loop_dope): ask for pSLIM widget server at names\n");
  if (!names_waitfor_name(retbuf, &pslim_l4id, 5000))
    {
      printf("dropscon.o: pSLIM-server not registered at names\n");
      return -ENODEV;
    }
  if (l4_is_invalid_id(pslim_l4id))
    {
      printf("Got invalid pslim_l4id!\n");
      return -ENODEV;
    }
	
  fn_x  = 8;
  fn_y  = 12;
  xres  = 640;
  yres  = 480;
  depth = 16;	

  /* register DOpE keyboard device driver */
  if (!(keybd_inp_dev = kmalloc(sizeof(struct input_dev), GFP_KERNEL)))
    return -ENOMEM;
		
  memset(keybd_inp_dev, 0, sizeof(struct input_dev));
  keybd_inp_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_REP);
  for (i = 0; i < 255; i++)
    set_bit(i, keybd_inp_dev->keybit);
  clear_bit(0, keybd_inp_dev->keybit);
  keybd_inp_dev->open      = inp_dev_open;
  keybd_inp_dev->close     = inp_dev_close;
  keybd_inp_dev->name      = "DOpE_keyboard";
  keybd_inp_dev->idbus     = BUS_USB;
  input_register_device(keybd_inp_dev);

  /* register DOpE mouse device driver */
  if (!(mouse_inp_dev = kmalloc(sizeof(struct input_dev), GFP_KERNEL)))
    return -ENOMEM;

  memset(mouse_inp_dev, 0, sizeof(struct input_dev));
  mouse_inp_dev->evbit[0]      = BIT(EV_KEY) | BIT(EV_REL) | BIT(EV_ABS);
  mouse_inp_dev->relbit[0]     = BIT(REL_X) | BIT(REL_Y) | BIT(REL_WHEEL);
  mouse_inp_dev->absbit[0]     = BIT(ABS_X) | BIT(ABS_Y);
  mouse_inp_dev->keybit[LONG(BTN_MOUSE)] = BIT(BTN_LEFT) | BIT(BTN_RIGHT) | BIT(BTN_MIDDLE);
  mouse_inp_dev->keybit[LONG(BTN_MOUSE)] |= BIT(BTN_SIDE) | BIT(BTN_EXTRA);
  mouse_inp_dev->absmax[ABS_X] = xres;
  mouse_inp_dev->absmax[ABS_Y] = yres;
  mouse_inp_dev->open          = inp_dev_open;
  mouse_inp_dev->close         = inp_dev_close;
  mouse_inp_dev->name          = "DOpE_mouse";
  mouse_inp_dev->idbus         = BUS_USB;
  input_register_device(mouse_inp_dev);

  return 0;
}


void dope_kbd_exit(void)
{
#ifdef L4L22
    destroy_thread(ev_l4id.id.lthread);
#else
    l4lx_thread_shutdown(ev_l4id);
#endif
    input_unregister_device(keybd_inp_dev);
    input_unregister_device(mouse_inp_dev);
    kfree(keybd_inp_dev);
    kfree(mouse_inp_dev);
}


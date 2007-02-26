/* $Id$ */

/*	con/server/src/ev.c
 *
 *	event stuff
 */

/* L4 includes */
#include <l4/con/l4con.h>
#include <l4/thread/thread.h>
#include <l4/util/thread.h>
#include <l4/input/libinput.h>

/* DROPS includes */
#include "stream-client.h"

/* OSKit includes */
#include <stdlib.h>
#include <stdio.h>

/* local includes */
#include "ev.h"
#include "vc.h"
#include "ipc.h"
#include "con_config.h"
#include "con_macros.h"

/* in main.c */
extern int want_vc;
extern l4_threadid_t ev_partner_l4id;
extern l4lock_t want_vc_lock;
extern l4_uint8_t vc_mode;

static int use_omega0 = 0;

/******************************************************************************
 * handle_keyevent                                                            *
 *                                                                            *
 * Key event handling -> distribution and switch                              *
 ******************************************************************************/
static void
handle_event(l4input_t *ev)
{
  static int altgr_down = 0;
  static int shift_down = 0;
  static int f_key;

  static sm_exc_t _ev;
  static stream_io_input_event_t ev_struct;

  if (ev->type == EV_KEY)
    {
      unsigned keycode = ev->code;
      unsigned down    = ev->value;
      /* virtual console switching */
      if (   keycode >= KEY_F1 
	  && keycode <= KEY_F10
	  && down
	  && (altgr_down || shift_down))
	{
	  f_key = keycode - 58;
	  l4lock_lock(&want_vc_lock);
	  want_vc = f_key;
	  l4lock_unlock(&want_vc_lock);
	  return;
	}
      /* F11/Shift F11: increase/decrase brightness */
      if (keycode == KEY_F11 && altgr_down && down)
	{
	  vc_brightness_contrast(shift_down ? -100 : 100,0);
	  return;
	}
      /* F12/Shift F12: increase/decrase contrast */
      if (keycode == KEY_F12 && altgr_down && down)
	{
	  vc_brightness_contrast(0,shift_down ? -100 : 100);
	  return;
	}
      /* Magic SysReq -> enter_kdebug() */
      if (keycode == KEY_SYSRQ && altgr_down) 
	{
	  enter_kdebug("AltGr + SysRq");
	  return;
	}
      if (keycode == KEY_RIGHTALT) 
	{
	  altgr_down = down;
	}
      if (keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT)
	{
	  shift_down = down;
	}

      /* ev_struct.time is not used */
      ev_struct.type  = EV_KEY;
      ev_struct.code  = keycode;
      ev_struct.value = down;
    }
  else if (ev->type == EV_REL || ev->type == EV_ABS)
    {
      /* mouse event */
      ev_struct.type  = ev->type;
      ev_struct.code  = ev->code;
      ev_struct.value = ev->value;
    }
  else
    {
      printf("handle_event: Unknown event type %d\n", ev->type);
      return;
    }

  l4lock_lock(&want_vc_lock);
  if (  (vc_mode & CON_IN)
      &&!l4_is_nil_id(ev_partner_l4id))
    {
      /* krishna: Flick is dumb. No Flick client timeouts means a security 
       * problem here because the untrusted part is the Flick server! */
      stream_io_push(ev_partner_l4id, &ev_struct, &_ev);
      if (   _ev._type == exc_l4_system_exception
	  && _ev._except == (void *)5)
	{
	  printf("handle_key_event: Target thread %x.%x dead?\n",
	      ev_partner_l4id.id.task, ev_partner_l4id.id.lthread);
	  /* close current console and switch to another */
	  want_vc = -2;
	}
    }
  l4lock_unlock(&want_vc_lock);
}

/******************************************************************************
 * ev_init                                                                    *
 *                                                                            *
 * event driver initialization                                                *
 ******************************************************************************/
void ev_init()
{
  l4input_init(use_omega0, 255, handle_event);
}

void
ev_irq_use_omega0(void)
{
  use_omega0 = 1;
}


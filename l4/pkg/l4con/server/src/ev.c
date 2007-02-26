/* $Id$ */
/**
 * \file	con/server/src/ev.c
 * \brief	mouse, keyboard, etc event stuff
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

/* L4 includes */
#include <l4/l4con/l4con.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/util/thread.h>
#include <l4/util/l4_macros.h>
#include <l4/input/libinput.h>

/* DROPS includes */
#include "stream-client.h"

/* OSKit includes */
#include <stdlib.h>
#include <stdio.h>

/* local includes */
#include "ev.h"
#include "main.h"
#include "vc.h"
#include "ipc.h"
#include "con_config.h"
#include "con_macros.h"

int use_omega0;
int nomouse;

/** brief Key event handling -> distribution and switch */
static void
handle_event(struct l4input *ev)
{
  static int altgr_down;
  static int shift_down;

  static CORBA_Environment env = dice_default_environment;
  static stream_io_input_event_t ev_struct;

  int loop, resend;

  if (ev->type == EV_KEY)
    {
      unsigned keycode = ev->code;
      unsigned down    = ev->value;

      if (nomouse && keycode >= BTN_MOUSE && keycode <= BTN_TASK)
	return;

      /* virtual console switching */
      if (keycode >= KEY_F1 && keycode <= KEY_F10 && 
	  down && (altgr_down || shift_down))
	{
	  request_vc(keycode - 58);
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
	altgr_down = down;

      if (keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT)
	shift_down = down;

      /* ev_struct.time is not used */
      ev_struct.type  = EV_KEY;
      ev_struct.code  = keycode;
      ev_struct.value = down;
    }
  else if (ev->type == EV_REL || ev->type == EV_ABS)
    {
      /* mouse event */
      if (nomouse)
	return;

      ev_struct.type  = ev->type;
      ev_struct.code  = ev->code;
      ev_struct.value = ev->value;
    }
  else if (ev->type == EV_MSC)
    {
      // ignored
      return;
    }
  else
    {
      printf("handle_event: Unknown event type %d\n", ev->type);
      return;
    }

  for (loop=0, resend=1; resend && loop<10; loop++)
    {
      l4lock_lock(&want_vc_lock);
      if ((vc_mode & CON_IN) && !l4_is_nil_id(ev_partner_l4id))
	{
	  env.timeout = EVENT_TIMEOUT;
	  stream_io_push_call(&ev_partner_l4id, &ev_struct, &env);
	  if (env.major != CORBA_NO_EXCEPTION)
	    {
	      switch (env._p.ipc_error)
		{
		case L4_IPC_ENOT_EXISTENT:
		  /* close current console and switch to another */
		  LOG("Target thread "l4util_idfmt" dead",
		      l4util_idstr(ev_partner_l4id));
		  want_vc = -2;
		  break;
		case L4_IPC_SETIMEOUT:
		case L4_IPC_SECANCELED:
		  /* send key again */
		  break;
		case L4_IPC_RETIMEOUT:
		case L4_IPC_RECANCELED:
		  /* assume that the key was successfully delivered */
		  resend = 0;
		  break;
		default:
		  /* no idea what to do */
		  LOG("Error %d sending event to "l4util_idfmt,
		      env._p.ipc_error, l4util_idstr(ev_partner_l4id));
		  want_vc = -2;
		  break;
		}
	    }
	  else
	    resend = 0;

	  /* wait a short time before trying to resend */
	  if (resend)
	    l4_sleep(50);
	}
      l4lock_unlock(&want_vc_lock);
    }
}

/** \brief event driver initialization */
void
ev_init()
{
  l4input_init(use_omega0, 254, handle_event);
}

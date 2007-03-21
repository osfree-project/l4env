/* $Id$ */
/**
 * \file	l4con/server/src/ev.c
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
#include "config.h"
#include "ev.h"
#include "main.h"
#include "vc.h"

int use_omega0;
int nomouse;
  
static void
send_event_client(struct l4input *ev)
{
  static CORBA_Environment env = dice_default_environment;
  int loop, resend;
  stream_io_input_event_t stream_event = { .type  = ev->type, 
					   .code  = ev->code,
					   .value = ev->value };

  for (loop=0, resend=1; resend && loop<10; loop++)
    {
      l4lock_lock(&want_vc_lock);
      if ((vc_mode & CON_IN) && !l4_is_nil_id(ev_partner_l4id))
	{
	  env.timeout = EVENT_TIMEOUT;
	  stream_io_push_call(&ev_partner_l4id, &stream_event, &env);
	  if (DICE_HAS_EXCEPTION(&env))
	    {
	      switch (DICE_IPC_ERROR(&env))
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
		      DICE_IPC_ERROR(&env), l4util_idstr(ev_partner_l4id));
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

/** brief Key event handling -> distribution and switch */
static void
handle_event(struct l4input *ev)
{
  static int altgr_down;
  static int shift_down;
  static struct l4input special_ev = { .type = 0xff };

  if (ev->type == EV_KEY)
    {
      l4_umword_t keycode      = ev->code;
      l4_umword_t down         = ev->value;
      l4_umword_t special_down = 0;

      if (nomouse && keycode >= BTN_MOUSE && keycode < BTN_TASK)
	return;

      if (keycode == KEY_RIGHTALT)
	altgr_down = special_down = down;
      else if (keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT)
      	shift_down = special_down = down;

      if (special_down)
	{
	  /* Defer sending of the special key until we know if we handle
	   * the next key completely in the server */
	  special_ev = *ev;
	  return;
	}

      if (down && (altgr_down || shift_down))
	{
	  /* virtual console switching */
	  if (keycode >= KEY_F1 && keycode <= KEY_F10)
	    {
	      request_vc(keycode - KEY_F1 + 1);
	      special_ev.type = 0xff;
	      return;
	    }
	  if (keycode == KEY_LEFT)
	    {
	      request_vc_delta(-1);
	      special_ev.type = 0xff;
	      return;
	    }
	  if (keycode == KEY_RIGHT)
	    {
	      request_vc_delta(1);
	      special_ev.type = 0xff;
	      return;
	    }
	  if (keycode == KEY_F11 && altgr_down)
	    {
	      /* F11/Shift F11: increase/decrase brightness */
	      vc_brightness_contrast(shift_down ? -100 : 100, 0);
	      special_ev.type = 0xff;
	      return;
	    }
	  if (keycode == KEY_F12 && altgr_down)
	    {
	      /* F12/Shift F12: increase/decrase contrast */
	      vc_brightness_contrast(0, shift_down ? -100 : 100);
	      special_ev.type = 0xff;
	      return;
	    }
	  if (keycode == KEY_PAUSE && altgr_down)
	    {
	      cpu_load_history = 1-cpu_load_history;
	      return;
	    }
#ifndef L4BID_RELEASE_MODE
	  if (keycode == KEY_SYSRQ && altgr_down)
	    {
	      /* Magic SysReq -> enter_kdebug() */
	      enter_kdebug("AltGr + SysRq");
	      special_ev.type = 0xff;
	      return;
	    }
#endif
	}

      /* No special key, send deferred key event */
      if (special_ev.type != 0xff)
	{
	  send_event_client(&special_ev);
	  special_ev.type = 0xff;
	}
    }
  else if (ev->type == EV_REL || ev->type == EV_ABS)
    {
      /* mouse event */
      if (nomouse)
	return;
    }
  else if (ev->type == EV_MSC)
    {
      /* ignored */
      return;
    }
  else
    {
      printf("handle_event: Unknown event type %d\n", ev->type);
      return;
    }

  send_event_client(ev);
}

/** \brief event driver initialization
 */
void
ev_init()
{
  l4input_init(use_omega0, 254, handle_event);
}

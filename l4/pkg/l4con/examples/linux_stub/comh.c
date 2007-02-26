/* $Id$ */
/**
 * \file	con/examples/linux_stub/comh.c
 * \brief	command handler for the L4Linux stub
 *
 * \date	01/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <linux/version.h>
#include <linux/tty.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/kd.h>
#endif
#include <linux/console_struct.h>

#include "comh.h"
#include "dropscon.h"
#include "xf86if.h"

#define SHUTDOWN_SIGS	(sigmask(SIGKILL)|sigmask(SIGINT)|sigmask(SIGTERM))

l4_uint32_t comh_sleep_state = 0;

/* this should be enough */
static unsigned setjmp_buf[16];

void
comh_wakeup(void)
{
  __builtin_longjmp (&setjmp_buf, 1);
}

static inline void
_comh_fill(comh_fill_t *fill) 
{
  CORBA_Environment _env = dice_default_environment;
   
  if (con_vc_pslim_fill_call(&dropsvc_l4id, 
			&fill->rect, 
			fill->color, 
			&_env)
      || _env.major != CORBA_NO_EXCEPTION)
    printk("comh.c: pslim_fill failed\n");
}


static inline void 
_comh_puts(comh_puts_t *puts) 
{
  CORBA_Environment _env = dice_default_environment;
   
  if (con_vc_puts_attr_call(&dropsvc_l4id, 
		  puts->str,
	          puts->str_size,
		  puts->x, 
		  puts->y,
		  &_env)
      || _env.major != CORBA_NO_EXCEPTION)
    printk("comh.c: pslim_puts failed\n");
}


static inline void 
_comh_putc(comh_putc_t *putc) 
{
  CORBA_Environment _env = dice_default_environment;

  if (con_vc_puts_attr_call(&dropsvc_l4id,
		  &putc->ch,
	          2,
		  putc->x,
		  putc->y,
		  &_env)
      || _env.major != CORBA_NO_EXCEPTION)
    printk("comh.c: pslim_putc failed\n");
}


static inline void 
_comh_redraw(comh_redraw_t *redraw)
{
  while (redraw->lines--)
    {
      CORBA_Environment _env = dice_default_environment;
       
      if (con_vc_puts_attr_call(&dropsvc_l4id,
			   redraw->p,
	                   redraw->columns*2,
			   redraw->x,
			   redraw->y,
			   &_env)
	  || _env.major != CORBA_NO_EXCEPTION)
	printk("comh.c: pslim_puts failed\n");

      redraw->p += dropscon_num_columns;
      redraw->y += DROPSCON_BITY(1);
    }
}


static inline void 
_comh_copy(comh_copy_t *copy)
{
  CORBA_Environment _env = dice_default_environment;
   
  if (con_vc_pslim_copy_call(&dropsvc_l4id,
			&copy->rect,
			copy->dx, 
			copy->dy,
			&_env)
      || _env.major != CORBA_NO_EXCEPTION)
    printk("comh.c: pslim_copy failed\n");
}

void
comh_thread(void *data)
{
  l4_umword_t dummy;
  l4_msgdope_t result;

  /* hand shake with creator */
  l4_ipc_call(main_l4id, L4_IPC_SHORT_MSG, 0, 0,
			 L4_IPC_SHORT_MSG, &dummy, &dummy,
			 L4_IPC_NEVER, &result);

  for (;;)
    {
      comh_proto_t *comh;

      if (__builtin_setjmp(&setjmp_buf))
	comh_sleep_state = 0;

      comh = comh_list + tail;

      comh_sleep_state = 1;
      if (!atomic_read(&comh->valid))
	{
	  /* next entry is still not valid -- goto sleep */
	  l4_umword_t dummy;
	  l4_msgdope_t result;

	  l4_ipc_receive(L4_NIL_ID, L4_IPC_SHORT_MSG, &dummy, &dummy,
			 L4_IPC_NEVER, &result);
	}
      comh_sleep_state = 0;

      atomic_dec(&comh->valid);

      if (stop_comh_thread)
	goto done;

      /* only write to console if X does _not_ ! */
      if (!xf86used)
	{
	  switch(comh->ftype)
	    {
	    case COMH_NIL:
	      /* nothing to do */
	      break;
	    case COMH_FILL:
	      _comh_fill(&comh->func.fill);
	      break;
	    case COMH_PUTS:
	      _comh_puts(&comh->func.puts);
	      break;
	    case COMH_PUTC:
	      _comh_putc(&comh->func.putc);
	      break;
	    case COMH_REDRAW:
	      _comh_redraw(&comh->func.redraw);
	      break;
	    case COMH_COPY:
	      _comh_copy(&comh->func.copy);
	      break;
	    default:
	      printk("comh.c: unknown function %d!\n", comh->ftype);
	    }
	}
      
      /* finished current request */
      tail = (tail + 1) % DROPSCON_COMLIST_SIZE;
      
      if (flush_comh_requests)
	{
	  /* main thread asked us to flush the request queue because the
	   * there is no element available */
	  comh_redraw_t redraw;
	  
	  while (tail != head)
	    {
	      atomic_dec(&comh_list[tail].valid);
	      tail = (tail + 1) % DROPSCON_COMLIST_SIZE;
	    }
	  
	  /* while redrawing screen, allow new requests to be enqueued */
	  flush_comh_requests = 0;

	  /* redraw whole screen */
	  redraw.x = redraw.y = 0;
	  redraw.columns = dropscon_num_columns;
	  redraw.lines = dropscon_num_lines;
	  redraw.p = (u16 *) dropscon_display_fg->vc_visible_origin;
	  _comh_redraw(&redraw);
	}
    }

done:
  printk("dropscon.o: comh_thread: exit\n");

  /* shake hands with main thread */
  up(&exit_notify_sem);
}

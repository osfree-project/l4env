/*
 * command handler for the L4Linux stub
 */

#include <linux/tty.h>
#include <linux/console_struct.h>

#include "comh.h"
#include "dropscon.h"
#include "xf86if.h"

#define SHUTDOWN_SIGS	(sigmask(SIGKILL)|sigmask(SIGINT)|sigmask(SIGTERM))

atomic_t comh_sleep_state = ATOMIC_INIT(0);
l4_uint32_t  comh_sleep_esp, comh_sleep_eip;

extern CORBA_long (*pslim_fill) (CORBA_Object, pslim_rect_t *, pslim_color_t, CORBA_Environment *);
extern CORBA_long (*pslim_copy) (CORBA_Object, pslim_rect_t *, CORBA_short, CORBA_short, CORBA_Environment *);
extern CORBA_long (*pslim_puts_attr) (CORBA_Object, const CORBA_char_ptr,
				CORBA_long,CORBA_short, CORBA_short, CORBA_Environment *);


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
  CORBA_Environment env;
   
  if (pslim_fill(&pslim_l4id, 
			&fill->rect, 
			fill->color, 
			&env)
      || (env.major != CORBA_NO_EXCEPTION))
    printk("comh.c: pslim_fill failed\n");
}


static inline void 
_comh_puts(comh_puts_t *puts) 
{
  CORBA_Environment env;

  if (pslim_puts_attr(&pslim_l4id, 
          (char *)puts->str, 
		  puts->str_size,
		  puts->x, 
		  puts->y,
		  &env)
      || (env.major != CORBA_NO_EXCEPTION))
    printk("comh.c: pslim_puts failed\n");
}


static inline void 
_comh_putc(comh_putc_t *putc) 
{
  CORBA_Environment env;

  if (pslim_puts_attr(&pslim_l4id,
		  (char *)&putc->ch, 2,
		  putc->x,
		  putc->y,
		  &env)
      || (env.major != CORBA_NO_EXCEPTION))
    printk("comh.c: pslim_putc failed\n");
}


static inline void 
_comh_redraw(comh_redraw_t *redraw)
{
  while (redraw->lines--)
    {
      CORBA_Environment env;
       
      if (pslim_puts_attr(&pslim_l4id,
			   (char *)redraw->p, redraw->columns*2,
			   redraw->x,
			   redraw->y,
			   &env)
	  || (env.major != CORBA_NO_EXCEPTION))
	printk("comh.c: pslim_puts failed\n");

      redraw->p += dropscon_num_columns;
      redraw->y += DROPSCON_BITY(1);
    }
}


static inline void 
_comh_copy(comh_copy_t *copy)
{
//  sm_exc_t _ev;
  CORBA_Environment env;
   
  if (pslim_copy(&pslim_l4id,
			&copy->rect,
			copy->dx, 
			copy->dy,
			&env)
      || (env.major != CORBA_NO_EXCEPTION))
    printk("comh.c: pslim_copy failed\n");
}

void
comh_thread(void *data)
{
#ifdef COMH_KERNEL_THREAD
  
  /* release unneeded resources */
  exit_files(current);
  daemonize();
  
  siginitsetinv(&current->blocked, SHUTDOWN_SIGS);
  
  /* set name of thread */
  strncpy (current->comm, "dropscon", sizeof(current->comm) - 1);
  current->comm[sizeof(current->comm) - 1] = '\0';

#else
  
  l4_uint32_t dummy;
  l4_msgdope_t result;

  /* hand shake with creator */
  l4_ipc_call(main_l4id,
		   L4_IPC_SHORT_MSG, 0, 0,
		   L4_IPC_SHORT_MSG, &dummy, &dummy,
		   L4_IPC_NEVER, &result);
  
#endif
  
  while (1)
    {
      comh_proto_t *comh = comh_list + tail;

#ifdef COMH_KERNEL_THREAD
      
      /* wait until next element is acknowledged */
      down_interruptible(&comh->sem);
      if (signal_pending(current))
	break;
      
#else

      /* save return point for wakeup */
      if (!__builtin_setjmp(&setjmp_buf))
	{
	  if (!atomic_read(&comh->valid))
	    {
	      /* next entry is still not valid -- goto sleep */
	      l4_uint32_t dummy;
	      l4_msgdope_t result;
	      
	      atomic_inc(&comh_sleep_state);
	      l4_ipc_receive(L4_NIL_ID, L4_IPC_SHORT_MSG, &dummy, &dummy,
				  L4_IPC_NEVER, &result);
	    }
	}

      atomic_dec(&comh->valid);
      
#endif /* COMH_KERNEL_THREAD */

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
	      printf("comh.c: unknown function!\n");
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
#ifdef COMH_KERNEL_THREAD
	      atomic_dec(&comh_list[tail].sem.count);
#else
	      atomic_dec(&comh_list[tail].valid);
#endif
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


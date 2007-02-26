/*!
 * \file	con/examples/linux_stub/main.c
 * \brief	DROPS textbased console driver for L4Linux
 *
 * \date	01/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#include <linux/malloc.h>
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/vt_kern.h>
#include <linux/vt_buffer.h>
#include <linux/console_struct.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)

#include <l4linux/x86/ids.h>
#include <l4linux/x86/rmgr.h>
#include <l4linux/x86/l4_thread.h>

#else /* kernel version >= 2.4 */

#include <asm/api/config.h>
#include <asm/l4lxapi/thread.h>

#endif /* kernel version >= 2.4 */

/* L4 includes */
#include <l4/names/libnames.h>
#include <l4/sys/l4int.h>
#include <l4/util/atomic.h>
#include <l4/con/l4con.h>

/* local includes */
#include "dropscon.h"
#include "comh.h"
#include "xf86if.h"
#include "bootlog.h"

/* color definition */
#define black		0x000000
#define white		0xffffff


l4_threadid_t dropsvc_l4id;
l4_threadid_t con_l4id;
l4_threadid_t main_l4id;
comh_proto_t comh_list[DROPSCON_COMLIST_SIZE];
unsigned int init_done = 0;
unsigned int redraw_pending = 0;

static unsigned int dropscon_cursor_x = 0;
static unsigned int dropscon_cursor_y = 0;
static int cursor_erased = 0;
static int module_init_done = 0;

int flush_comh_requests = 0;
volatile int stop_comh_thread = 0;

DECLARE_MUTEX_LOCKED(exit_notify_sem);

struct vc_data *dropscon_display_fg = NULL;
volatile unsigned int tail = 0, head = 0;
unsigned int fg_color = white;
unsigned int bg_color = black;
unsigned int dropscon_num_columns;		/* Number of text columns */
unsigned int dropscon_num_lines;		/* Number of text lines */
unsigned int accel_flags = 0;
unsigned int fn_x, fn_y;

static unsigned int xres, yres;

static int  first_vc = DROPSCON_FIRST_VC;
static int  last_vc  = DROPSCON_LAST_VC;

static const char *dropscon_startup(void);
static void dropscon_init     (struct vc_data*, int);
static void dropscon_deinit   (struct vc_data*);
static void dropscon_clear    (struct vc_data*, int, int, int, int);
static void dropscon_putc     (struct vc_data *conp, int c, int ypos, int xpos);
static void dropscon_putcs    (struct vc_data *conp, const unsigned short *s, 
			       int count, int ypos, int xpos);
static void dropscon_cursor   (struct vc_data *conp, int mode);
static int  dropscon_scroll   (struct vc_data *conp, int t, int b, int dir,
			       int count);
static void dropscon_bmove    (struct vc_data *conp, int sy, int sx, 
			       int dy, int dx, int height, int width);
static int  dropscon_switch   (struct vc_data *conp);
static int  dropscon_blank    (struct vc_data *conp, int blank);
static int  dropscon_font_op  (struct vc_data *conp, 
			       struct console_font_op *op);
static int  dropscon_set_palette(struct vc_data *conp, unsigned char *table);
static int  dropscon_scrolldelta(struct vc_data *conp, int lines);

static void dropscon_redraw(unsigned int x, unsigned int y,
			    unsigned int w, unsigned int h);

#ifndef COMH_KERNEL_THREAD
static l4_threadid_t comh_l4id;
#endif

static const char *
dropscon_startup(void)
{
  const char *display_desc = "DROPScon";
  /* evt. setze cursor (oder hardware cursor) */
   
  return display_desc;
}

static void 
dropscon_init(struct vc_data *c, 
	      int init) 
{   
  struct vc_data *vc;
  
  c->vc_complement_mask = 0x7700;
  c->vc_can_do_color = 1;
  c->vc_display_fg = &dropscon_display_fg;

  if (init) 
    {
      c->vc_cols = dropscon_num_columns;
      c->vc_rows = dropscon_num_lines;
    } 
  else 
    vc_resize_con(dropscon_num_lines, dropscon_num_columns, c->vc_num);
      
  if (dropscon_display_fg == NULL)
    dropscon_display_fg = c;

  vc = dropscon_display_fg;

  if (dropscon_bootlog_init_done)
    {
      char c;
      int x=0, y=0;
      
      dropscon_bootlog_done();

      /* clear console */
      scr_memsetw(vc->vc_screenbuf, 0x0720, vc->vc_rows*vc->vc_cols*2);
      
      /* print out logged text before we was initialized */
      while ((c = dropscon_bootlog_read()))
	{
	  switch (c)
	    {
	    case '\n':
	      if (y >= dropscon_num_lines-1)
      		dropscon_scroll(vc, 0, vc->vc_rows, SM_UP, 1);
	      else
		y++;
	      // fall through
	    case '\r':
	      x = 0;
	      break;
	    default:
	      if (x > vc->vc_cols)
		{
		  x = 0;
		  y++;
		  /* XXX more than 25 lines? */
		}
	      vc->vc_screenbuf[y*dropscon_num_columns + x] = c | 0x700;
	      x++;
	      break;
	    }
	}
      vc->vc_pos = vc->vc_visible_origin + y*dropscon_num_columns + x;
      vc->vc_x   = x;
      vc->vc_y   = y;
    }
  else
    {
      /* In case the screenbuffer does nothing contain when clear it. This
       * depends on the graphics card */
      if (   (vc->vc_screenbuf[ 0] == 0xffff)
	  && (vc->vc_screenbuf[ 1] == 0xffff)
	  && (vc->vc_screenbuf[20] == 0xffff)
	  && (vc->vc_screenbuf[21] == 0xffff))
	scr_memsetw(vc->vc_screenbuf, 0x0720, vc->vc_rows*vc->vc_cols*2);
    }

  MOD_INC_USE_COUNT;

  init_done = 1;

  if (redraw_pending)
    dropscon_redraw_all();
}


static void 
dropscon_deinit(struct vc_data *c)
{
  printk("dropscon.o: deinit num:%d\n",c->vc_num);
   
  if (dropscon_display_fg == c)
    dropscon_display_fg = NULL;

  MOD_DEC_USE_COUNT;
}

/**************************************************************
 ****  dropscon_XXX routines - interface used by the world ****
 **************************************************************/

/* must not block !! */
static comh_proto_t*
get_comh_req(void)
{
  unsigned int old, new;

  /* if X owns the console, do nothing */
  if (xf86used)
    return NULL;
  
  do
    {
      /* request queue is about to be flushed */
      if (flush_comh_requests)
	return NULL;
      
      old = head;
      new = (old + 1) % DROPSCON_COMLIST_SIZE;
      
      if (new == tail)
	{
//	  printk("dropscon.o: request queue overrun, flushing\n");
	  flush_comh_requests = 1;
	  return NULL;
	}
    
      /* try to write new index; if failed, retry */
    } while (! l4util_cmpxchg32(&head, old, new));

  return comh_list + old;
}


static void
ack_comh_req(comh_proto_t *comh)
{
#ifdef COMH_KERNEL_THREAD
 
  up(&comh->sem);

#else
  int state;

  atomic_inc(&comh->valid);

  /* wakeup comh thread? */
  while ((state = atomic_read(&comh_sleep_state)) > 0)
    {
      if (l4util_cmpxchg32((l4_uint32_t*)&comh_sleep_state, state, state-1))
	{
	  l4_threadid_t preempter, pager;
	  unsigned old_flags, old_eip, old_esp;
	  
	  preempter = pager = L4_INVALID_ID;
	  l4_thread_ex_regs(comh_l4id,
			    (l4_umword_t)comh_wakeup, -1,
			    &preempter, &pager,
			    &old_flags, &old_eip, &old_esp);
	  break;
	}
    }
  
#endif
}


static void 
dropscon_clear(struct vc_data *conp, 
	       int sy, 
	       int sx, 
	       int height,
	       int width)
{
  l4con_pslim_rect_t _rect;
  comh_proto_t *comh;
         
  if (!height || !width)
    return;
   
  _rect.x = DROPSCON_BITX(sx);
  _rect.y = DROPSCON_BITY(sy);
  _rect.h = DROPSCON_BITY(height);
  _rect.w = DROPSCON_BITX(width);
  
  if ((comh = get_comh_req()))
    {
      comh->func.fill.rect = _rect;
      comh->func.fill.color = black;
      comh->ftype = COMH_FILL;
      ack_comh_req(comh);
    }
}


static void 
dropscon_putc(struct vc_data *conp, 
	      int c, 
	      int y, 
	      int x)
{
  comh_proto_t *comh;
  
  if ((comh = get_comh_req()))
    {
      comh->func.putc.ch = c;
      comh->func.putc.x = DROPSCON_BITX(x);
      comh->func.putc.y = DROPSCON_BITY(y);
      comh->ftype = COMH_PUTC;
      ack_comh_req(comh);
    }
}


static void 
dropscon_putcs(struct vc_data *conp, 
	       const unsigned short *s,
	       int count,
	       int y,
	       int x)
{
  /* we have to copy here because until the request is served in
   * comh_thread, the screen buffer could be changed */
  int size;
  
  /* make sure we fit into request size */
  for (; count; count -= size/2, s += size/2, x += size/2)
    {
      comh_proto_t *comh;

      size = count*2;

      if (size > sizeof(comh->func.puts.str))
	size = sizeof(comh->func.puts.str);
  
      if ((comh = get_comh_req()))
	{
	  memcpy(comh->func.puts.str, s, size);
	  comh->func.puts.x = DROPSCON_BITX(x);
	  comh->func.puts.y = DROPSCON_BITY(y);
	  comh->func.puts.str_size = size;
	  comh->ftype = COMH_PUTS;
	  ack_comh_req(comh);
	}
      else
	break;
    }
}


static void 
dropscon_cursor(struct vc_data *conp, 
		int mode)
{   
  u16 pos = scr_readw((u16*) conp->vc_pos);
  u16 invert = ((pos & 0x0F00) << 4) | ((pos & 0xF000) >> 4);

  if(dropscon_cursor_x == conp->vc_x && dropscon_cursor_y == conp->vc_y) 
    {
      if (mode == CM_ERASE) 
	{
	  dropscon_putc(conp, pos, conp->vc_y, conp->vc_x);
	  cursor_erased ^= 1;
	}
      if (!cursor_erased)
	return;
    }
  
  if (mode == CM_ERASE)
    return;
  
  pos &= 0x00FF;
  pos |= invert;
  dropscon_putc(conp, pos, conp->vc_y, conp->vc_x);
  cursor_erased = 0;
  dropscon_cursor_x = conp->vc_x;
  dropscon_cursor_y = conp->vc_y;
  
  return;
}


static int
dropscon_scroll(struct vc_data *c, 
		int t, 
		int b, 
		int dir, 
		int lines)
{
  register u16 *p;
  register u32 n;
  
  if (!lines)
    return 0;
   
  if (lines > c->vc_rows)   /* maximum realistic size */
    lines = c->vc_rows;

  switch (dir) 
    {
    case SM_UP:
      /* update buffer before doing accellerated/slow scroll operation */
      p = (u16 *)(dropscon_display_fg->vc_visible_origin)
	+ t*dropscon_num_columns;
      n = lines*dropscon_num_columns;
      scr_memmovew(p, p+n, (b-t-lines)*dropscon_num_columns*2);
      scr_memsetw(p+(b-t-lines)*dropscon_num_columns, 
		  dropscon_display_fg->vc_video_erase_char, n*2);
      
      if (accel_flags & L4CON_FAST_COPY)
	{
	  /* copy is faster than redraw */
	  l4con_pslim_rect_t _rect;
	  comh_proto_t *comh;

	  _rect.x = 0;
      	  _rect.y = DROPSCON_BITY(t+lines);
	  _rect.h = DROPSCON_BITY(b-t-lines);
	  _rect.w = xres;
	  if ((comh = get_comh_req()))
	    {
	      comh->ftype = COMH_COPY;
	      comh->func.copy.rect = _rect;
	      comh->func.copy.dx = 0;
	      comh->func.copy.dy = DROPSCON_BITY(t);
	      ack_comh_req(comh);
	    }
	  else
	    break;
	  
	  _rect.y = DROPSCON_BITY(b-lines);
      	  _rect.h = DROPSCON_BITY(lines);
	  if ((comh = get_comh_req()))
	    {
	      comh->ftype = COMH_FILL;
	      comh->func.fill.rect = _rect;
	      comh->func.fill.color = black;
	      ack_comh_req(comh);
	    }
	}
      else
	{
	  /* redraw ist faster than copy */
	  dropscon_redraw(0, t, dropscon_num_columns, b-t);
	}
      break;
       
    case SM_DOWN:
      /* update buffer before doing accellerated/slow scroll operation */
      p = (u16 *)(dropscon_display_fg->vc_visible_origin) 
	+ (t * dropscon_num_columns);
      n = lines*dropscon_num_columns;
      scr_memmovew(p+n, p, (b-t-lines)*dropscon_num_columns*2);
      scr_memsetw(p, dropscon_display_fg->vc_video_erase_char, n*2);
      
      if (accel_flags & L4CON_FAST_COPY)
	{
	  /* copy is faster than redraw */
	  l4con_pslim_rect_t _rect;
	  comh_proto_t *comh;

	  _rect.x = 0;
	  _rect.y = DROPSCON_BITY(t);
	  _rect.h = DROPSCON_BITY(b-t-lines);
	  _rect.w = xres;      
      
	  if ((comh = get_comh_req()))
	    {
	      comh->func.copy.rect = _rect;
	      comh->func.copy.dx = 0;
	      comh->func.copy.dy = DROPSCON_BITY(t+lines);
	      comh->ftype = COMH_COPY;
	      ack_comh_req(comh);
	    }
	  else
	    break;
       
	  _rect.y = DROPSCON_BITY(t);
	  _rect.h = DROPSCON_BITY(lines);
      
	  if ((comh = get_comh_req()))
	    {
	      comh->func.fill.rect = _rect;
	      comh->func.fill.color = black;
	      comh->ftype = COMH_FILL;
	      ack_comh_req(comh);
	    }
	}
      else
	{
	  /* redraw is faster than copy */
	  dropscon_redraw(0, t, dropscon_num_columns, b-t);
	}
      break;
    }
   
  /* return 0 means unsuccessful */
  return 1;
}


static void 
dropscon_bmove(struct vc_data *conp, 
	       int sy, 
	       int sx, 
	       int dy, 
	       int dx, 
	       int height, 
	       int width)
{
  if (accel_flags & L4CON_FAST_COPY)
    {
      /* copy is faster than redraw */
      l4con_pslim_rect_t _rect;
      comh_proto_t *comh;
      
      _rect.x = DROPSCON_BITX(sx);
      _rect.y = DROPSCON_BITY(sy);
      _rect.w = DROPSCON_BITX(width);
      _rect.h = DROPSCON_BITY(height);
      
      if ((comh = get_comh_req()))
	{
	  comh->func.copy.rect = _rect;
	  comh->func.copy.dx = DROPSCON_BITX(dx);
	  comh->func.copy.dy = DROPSCON_BITY(dy);
	  comh->ftype = COMH_COPY;
	  ack_comh_req(comh);
	}
    }
  else
    {
      /* redraw is faster than copy */
      
      /* determine rectangle to redraw */
      if (dx < sx)
	{
	  /* move left */
	  width += sx-dx;
	  sx = dx;
	}
      else
	/* move right */
	width += dx-sx;
      if (dy < sy)
	{
	  /* move up */
	  height += sy-dy;
	  sy = dy;
	}
      else
	/* move down */
	height += dy-sy;
      
      dropscon_redraw(sx, sy, width, height);
    }
}


static int 
dropscon_switch(struct vc_data *conp)
{
  dropscon_cursor_x = 0;
  dropscon_cursor_y = 0;
  return 1;    /* redrawing needed */
}


static int 
dropscon_blank(struct vc_data *conp, 
	       int blank)
{
  CORBA_Environment _env = dice_default_environment;
  l4con_pslim_rect_t rect;   
   
  if (blank) 
    {
      rect.x = 0;
      rect.y = 0;
      rect.h = yres;
      rect.w = xres;
      
      if (con_vc_pslim_fill_call(&dropsvc_l4id, &rect, black, &_env)
	  || (_env.major != CORBA_NO_EXCEPTION))
	{
	  printk("dropscon.o: blank fill failed\n");
	  return 0;
	}
    }
  /* Tell console.c that it has to restore the screen itself */
  return 1;
}


static int 
dropscon_font_op(struct vc_data *conp, 
		 struct console_font_op *op)
{
  /* font operation like SET_FONT, GET_FONT, COPY_FONT ... here */
  return -ENOSYS;
}


static int 
dropscon_set_palette(struct vc_data *conp, 
		     unsigned char *table)
{
  return -EINVAL; /* setzte farbtabelle */
}


static void 
dropscon_invert_region(struct vc_data *conp, 
		       u16 *p, 
		       int count)
{
  while (count--)
    {
      u16 a = scr_readw(p);
      
      a = (a & 0x88ff) | ((a & 0x7000) >> 4) | ((a & 0x0700) << 4);
      scr_writew(a, p++);
    }
}


static int 
dropscon_scrolldelta(struct vc_data *conp, 
		     int lines)
{
  return 0;  /* scrollen um lines */
}


struct consw drops_con = {
    con_startup: 	dropscon_startup, 
    con_init: 		dropscon_init,
    con_deinit: 	dropscon_deinit,
    con_clear: 		dropscon_clear,
    con_putc: 		dropscon_putc,
    con_putcs: 		dropscon_putcs,
    con_cursor: 	dropscon_cursor,
    con_scroll: 	dropscon_scroll,
    con_bmove: 		dropscon_bmove,
    con_switch: 	dropscon_switch,
    con_blank: 		dropscon_blank,
    con_font_op:	dropscon_font_op,
    con_set_palette: 	dropscon_set_palette,
    con_scrolldelta: 	dropscon_scrolldelta,
    con_set_origin: 	NULL,
    con_save_screen: 	NULL,
    con_build_attr:	NULL,
    con_invert_region:	dropscon_invert_region,
    con_screen_pos:	NULL,
    con_getxy:          NULL,
};


void 
dropscon_redraw(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
  comh_proto_t *comh;
  
  if ((comh = get_comh_req()))
    {
      comh->func.redraw.p = (u16 *)(dropscon_display_fg->vc_visible_origin)
			    + (y * dropscon_num_columns) + x;
      comh->func.redraw.x = DROPSCON_BITX(x);
      comh->func.redraw.y = DROPSCON_BITY(y);
      comh->func.redraw.columns = w;
      comh->func.redraw.lines = h;
      comh->ftype = COMH_REDRAW;
      ack_comh_req(comh);
    }
  
  cursor_erased = 1;
  dropscon_cursor(dropscon_display_fg, CM_DRAW);
}

void
dropscon_clear_gap(void)
{
  int bottom = DROPSCON_BITY(dropscon_num_lines);
  comh_proto_t *comh;  
  
  if (bottom < yres)
    {
      /* clear rest of screen */
      if ((comh = get_comh_req()))
	{
	  l4con_pslim_rect_t rect = { 0, bottom, xres, yres };
	  
	  comh->func.fill.rect = rect;
	  comh->func.fill.color = black;
	  comh->ftype = COMH_FILL;
	  ack_comh_req(comh);
	}
    }
}

void
dropscon_redraw_all(void)
{
  dropscon_redraw(0, 0, dropscon_num_columns, dropscon_num_lines);
  dropscon_clear_gap();
}

int
dropsconsole_init(void)
{
  comh_proto_t *comh;
  l4_uint8_t gmode;
  CORBA_Environment _env = dice_default_environment;
  int error;
  unsigned bits_per_pixel;   /* ignored since we don't need it */
  unsigned bytes_per_pixel;  /* ignored since we don't need it */
  unsigned bytes_per_line;   /* ignored since we don't need it */

  /* only init one time */
  if (module_init_done)
    return 0;

  main_l4id = l4_myself();

  /* ask for 'con' (timeout = 5000 ms) */
  if (!names_waitfor_name(CON_NAMES_STR, &con_l4id, 5000)) 
    {
      printk("dropscon.o: con not registered at names\n");
      goto fail3;
    }
  
  /* open con console, string buffer is 65536 bytes */
  if (con_if_openqry_call(&con_l4id, DROPSCON_MAX_SBUF_SIZE,  0, 0,
		     PRIO_KERNEL /*from linux22/include/l4linux/x86/config.h*/,
		     &dropsvc_l4id, CON_NOVFB, &_env)
      || (_env.major != CORBA_NO_EXCEPTION))
    {
      printk("dropscon.o: open vc failed\n");
      goto fail3;
    }

  /* initialize interface for xfree86 stub */
  if ((error = xf86if_init()))
    goto fail2;

  /* init keyboard stub */
  con_kbd_init();

  if (con_vc_smode_call(&dropsvc_l4id, CON_INOUT, 
		    &ev_l4id, &_env)) 
    {
      printk("dropscon.o: setup vc failed\n");
      goto fail1;
    }

  /* set up lines and rows */
  if (con_vc_graph_gmode_call(&dropsvc_l4id, &gmode, &xres, &yres, 
			 &bits_per_pixel, &bytes_per_pixel,
			 &bytes_per_line, &accel_flags, 
			 &fn_x, &fn_y, &_env)
      || (_env.major != CORBA_NO_EXCEPTION))
    {
      printk("dropscon.o: get graph_gmode failed\n");
      goto fail1;
    }

  dropscon_num_columns = xres / fn_x;
  dropscon_num_lines = yres / fn_y;

#ifdef COMH_KERNEL_THREAD
  
  /* empty event list */
  for (comh = comh_list; 
       comh < comh_list + DROPSCON_COMLIST_SIZE; 
       comh++)
    init_MUTEX_LOCKED(&comh->sem);

  /* startup kernel thread for command handling */
  kernel_thread(comh_thread, NULL, 0);
  
#else
  
  /* empty event list */
  for (comh = comh_list;
       comh < comh_list + DROPSCON_COMLIST_SIZE;
       comh++)
    atomic_set(&comh->valid, 0);
  
  /* start L4 thread */
    {
      l4_msgdope_t result;
      l4_umword_t dummy;

#ifdef L4L22
      unsigned *esp;
      unsigned long stack_page;

#ifndef CONFIG_L4_THREADMANAGEMENT
#error Enable CONFIG_L4_THREADMANAGEMENT in L4Linux config!
#endif

      if (!(stack_page = __get_free_pages(GFP_KERNEL, 1)))
	{
	  printk("dropscon.o: Can't alloc stack for comh thread\n");
	  return -ENOMEM;
	}

      memset((void*)stack_page, 0, 2*PAGE_SIZE);

      esp = (unsigned*)(stack_page+(2*PAGE_SIZE)-sizeof(l4_threadid_t));

      *--esp = 0; /* param */
      *--esp = 0; /* fake return address */

      comh_l4id = create_thread(LTHREAD_NO_KERNEL_ANY,
			       (void (*)(void))comh_thread, esp);

      if (rmgr_set_prio(comh_l4id, 144 /*127*/))
	{
	  destroy_thread(ev_l4id.id.lthread);
	  printk("kbd.o: error setting priority of service thread\n");
	  return -EBUSY;
	}

#else /* kernel version >= 2.4 */

      comh_l4id = l4lx_thread_create(comh_thread,
				     NULL,
				     NULL, 0,
				     144 /*127*/,
				     "Con COMH");

#endif /* kernel version >= 2.4 */
      
      /* We don't have to take care that about a thread id on top of stack
       * and need_resched is set because comh_thread will be never call
       * L4Linux */

      l4_ipc_receive(comh_l4id, L4_IPC_SHORT_MSG, &dummy, &dummy,
			  L4_IPC_NEVER, &result);
      l4_ipc_send   (comh_l4id, L4_IPC_SHORT_MSG, 0, 0,
			  L4_IPC_NEVER, &result);
    }
  
#endif /* ! COMH_KERNEL_THREAD */
  
  /* allocate inital consoles, make our module default for new consoles */
  take_over_console(&drops_con, first_vc, last_vc, 1);

  /* don't init a second time */
  module_init_done = 1;

  return 0;

fail1:
  con_kbd_exit();
  
fail2:
  con_vc_close_call(&dropsvc_l4id, &_env);

fail3:
  return -EINVAL;
}


/* The exit function is less important because mostly we can not
 * stop the module because we own consoles */
static void 
dropsconsole_exit(void)
{
  CORBA_Environment _env = dice_default_environment;

  xf86if_done();
   
  if (con_vc_close_call(&dropsvc_l4id, &_env))
    printk("Close vc failed.");
   
  con_kbd_exit();
  
  printk("dropscon.o: closing vc's\n");
  give_up_console(&drops_con);

  printk("dropscon.o: stopping dropscon thread\n");
  stop_comh_thread = 1;

#ifdef COMH_KERNEL_THREAD
  /* ask dropscon kernel thread to terminate */
  up(&comh_list[tail].sem);
#endif

  /* wait until kernel thread is terminated */
  down(&exit_notify_sem);

  printk("dropscon.o: dropscon thread terminated\n");
}


module_init(dropsconsole_init);
module_exit(dropsconsole_exit);


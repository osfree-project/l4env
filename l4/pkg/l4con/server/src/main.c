/* $Id$ */
/**
 * \file	con/server/src/main.c		
 * \brief	'main' of the con server
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

/* L4 includes */
#include <l4/con/l4con.h>
#include <l4/con/l4con_ev.h>
#include <l4/con/con-client.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/thread/thread.h>
#include <l4/util/getopt.h>
#include <l4/lock/lock.h>
#include <l4/env/errno.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>

/* DROPS includes */
#include "stream-client.h"

/* OSKit includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* local includes */
#include "con.h"
#include "vc.h"
#include "ev.h"
#include "ipc.h"
#include "con_config.h"
#include "con_macros.h"
#include "con-server.h"

char LOG_tag[9] = CONTAG;
l4_ssize_t l4libc_heapsize = CONFIG_MALLOC_MAX_SIZE;

/* con stuff */
l4_uint16_t XRES = 640;
l4_uint16_t YRES = 480;
l4_uint8_t MY_BITS = 16;
l4_uint16_t VESA_XRES = 0;
l4_uint16_t VESA_YRES = 0;
l4_uint16_t VESA_BPL  = 0;
l4_uint8_t VESA_BITS = 0;
l4_uint8_t FONT_XRES = 0;
l4_uint8_t FONT_YRES = 0;

int noaccel = 0;			/* 1=disable pslim HW acceleration */
int nolog   = 0;			/* 1=disable logging to logserver */
int pan     = 0;			/* 1=pan display to 4MB boundary */

struct l4con_vc *vc[MAX_NR_L4CONS];	/* virtual consoles */
int fg_vc = -1;				/* current foreground vc */
int want_vc = 0;			/* the vc we want to switch to */
					/* krishna: if we set want_vc = -1,
					 * con_loop must look for an open vc in 
					 * list; don't switch to USED VCs, only 
					 * to that in OUT/INOUT mode */
int update_id;				/* 1=redraw vc id */
int use_l4io = 0;			/* Use L4 IO server, dfl: no */
l4lock_t want_vc_lock = L4LOCK_UNLOCKED;/* mutex for want_vc */
l4_threadid_t ev_partner_l4id = L4_NIL_ID;/* current event handler */
l4_uint8_t vc_mode = CON_CLOSED;		/* mode of current console */

static void do_switch(int i_new, int i_old);
static void con_loop(void) __attribute__ ((noreturn));

/** \brief command line parameter handling */
static void 
do_args(int argc, char *argv[])
{
  char c;
  char* endp;
  l4_uint32_t a;

  static struct option long_options[] =
    {
	{"xres", 1, 0, 'x'},
	{"yres", 1, 0, 'y'},
	{"bitsperpixel", 1, 0, 'b'},
	{"omega0", 0, 0, 'o'},
	{"noaccel", 0, 0, 'a'},
	{"nolog", 0, 0, 'l'},
	{"pan", 0, 0, 'p'},
	{"l4io", 0, 0, 'i'},
	{0, 0, 0, 0}
    };
  
  /* read command line arguments */
  while (1) 
    {
      c = getopt_long (argc, argv, "x:y:b:aolpi", long_options, NULL);

      if (c == -1)
	break;

      switch(c) 
	{
	case 'x':
	  /* x-resolution */
	  if (optarg) 
	    {
	      a = strtoul(optarg,&endp,0);
	      if (*endp != 0)	
      		break;
	      XRES = a;
    	    }
	  break;
	case 'y':
  	  /* y-resolution */
	  if (optarg) 
	    {
	      a = strtoul(optarg,&endp,0);
	      if (*endp != 0)
		break;
	      YRES = a;
	    }
	  break;
	case 'b':
	  /* bit per pixel */
	  if (optarg) 
	    {
	      a = strtoul(optarg,&endp,0);
	      if (*endp != 0)
		break;
	      MY_BITS = a;
	    }
	  break;
	case 'o':
	  /* use omega0 for interrupt handling */
	  ev_irq_use_omega0();
	  break;
	case 'a':
	  noaccel = 1;
	  break;
	case 'l':
	  nolog = 1;
	  break;
	case 'p':
	  pan = 1;
	  break;
	case 'i':
	  use_l4io = 1;
	  break;
	}
    }
}

/** Find first available (non-opened) vc in list */
static int
get_free_vc(void)
{
  int i;

  for (i=0; i < MAX_NR_L4CONS; i++) 
    {
      if (vc[i]->mode == CON_CLOSED) 
	return i;
    }

  LOG("Ooops, all vcs occupied");
  return -CON_EFREE;
}

/** \brief send con_event redraw */
static int
redraw_vc(void)
{
   int ret = 0;
   static CORBA_Environment env = dice_default_environment;
   static stream_io_input_event_t ev_struct;
      
   l4lock_lock(&want_vc_lock);
   if (!l4_is_nil_id(ev_partner_l4id)) 
     {
       /* ev_struct.time is not used */
       ev_struct.type = EV_CON;
       ev_struct.code = EV_CON_REDRAW;
       ev_struct.value = 0;
       
       /* XXX Flick is dumb. No Flick client timeouts means a security problem
       	* here because the untrusted part is the Flick server! */
       env.timeout = L4_IPC_NEVER;
       stream_io_push_call(&ev_partner_l4id, &ev_struct, &env);

       /* XXX check if Flick generated an EIO exception. In that case the
	* communication failed -- that normally means that the other thread
	* doesn't exists anymore */
	if ((env.major == CORBA_SYSTEM_EXCEPTION) &&
	    (env.ipc_error == L4_IPC_ENOT_EXISTENT))
	 {
	   printf("redraw_vc: Target thread %x.%x dead?\n",
	       ev_partner_l4id.id.task, ev_partner_l4id.id.lthread);
	   ret = -1;
	 }
     }
   l4lock_unlock(&want_vc_lock);
   
   return ret;
}


/** \brief send con_event background */
static int
background_vc(void)
{
   static CORBA_Environment env = dice_default_environment;
   static stream_io_input_event_t ev_struct;
      
   l4lock_lock(&want_vc_lock);
   if (!l4_is_nil_id(ev_partner_l4id)) 
     {
       /* ev_struct.time is not used */
       ev_struct.type = EV_CON;
       ev_struct.code = EV_CON_BACKGROUND;
       ev_struct.value = 0;
       
       /* XXX Flick is dumb. No Flick client timeouts means a security problem
       	* here because the untrusted part is the Flick server! */
       env.timeout = L4_IPC_NEVER;
       stream_io_push_call(&ev_partner_l4id, &ev_struct, &env);

       /* ignore errors */
       vc[fg_vc]->fb_mapped = 0;
     }
   l4lock_unlock(&want_vc_lock);
   
   return 0;
}


/** \brief Switch to other console (want_vc != fg_vc)
 * @pre have want_vc_lock */
static void 
switch_vc(void)
{
  int i;

retry:
  if ((want_vc > 0) && (want_vc & 0x1000))
    {
      /* special case: close current console */
      CORBA_Environment env = dice_default_environment;
      l4lock_unlock(&want_vc_lock);
      con_vc_close_call(&(vc[want_vc & ~0x1000]->vc_l4id), &env);
      l4lock_lock(&want_vc_lock);
      want_vc = -1;
    }

  if (want_vc < 0) 
    {
      for (i=0; i < MAX_NR_L4CONS; i++) 
	{
	  if (    vc[i]->mode & (CON_OUT | CON_OUT)
	      && !(vc[i]->mode & CON_MASTER))
	    {
  	      
	      /* open next closed VC */
	      LOG("switch to next vc %02d", i);
  	      
	      /* need fb_lock for that */
	      do_switch(i, 0);
	      if (redraw_vc() != 0)
		{
		  /* redraw failed => close vc */
		  CORBA_Environment env = dice_default_environment;
		  printf("Closing vc %d\n", i);
		  l4lock_unlock(&want_vc_lock);
		  con_vc_close_call(&(vc[i]->vc_l4id), &env);
		  l4lock_lock(&want_vc_lock);
		  continue;
		}
	      
	      fg_vc = i;
	      want_vc = fg_vc;
	      break;
	    }
	}
      if (i == MAX_NR_L4CONS) 
	{
	  LOG("All vc's closed");
	  /* switch to vc 0 (master) */
	  do_switch(0, -1);
	  want_vc = fg_vc;
	}
    }
  else 
    {	
      /* request for explicit switch (event or open) */
      if (want_vc >= MAX_NR_L4CONS) 
	{
	  /* return to sane state */
	  want_vc = fg_vc;
	  ev_partner_l4id = vc[want_vc]->ev_partner_l4id;
	  vc_mode = vc[want_vc]->mode;
	  return;
	}
      
      /* check if want_vc is open */
      if (vc[want_vc]->mode & CON_OUT)
	{
	  do_switch(want_vc, fg_vc);
	  if (redraw_vc() != 0)
	    {
	      /* redraw failure, close vc */
	      CORBA_Environment env = dice_default_environment;
	      printf("Closing vc %d\n", want_vc);
	      l4lock_unlock(&want_vc_lock);
	      con_vc_close_call(&(vc[want_vc]->vc_l4id), &env);
	      l4lock_lock(&want_vc_lock);
	      /* switch to another open console */
	      want_vc = -1;
	      goto retry;
	    }
	  
	  /* set a sane state */
	  fg_vc = want_vc;
	}
      else 
	{
	  /* set a sane state */
	  want_vc = fg_vc;
	}
    }
}

static void 
do_switch(int i_new, int i_old)
{
  struct l4con_vc *old = (i_old >= 0) ? vc[i_old] : 0;
  struct l4con_vc *new = (i_new >= 0) ? vc[i_new] : 0;

  /* save old vc */
  if (old != 0) 
    {
      register unsigned dummy;
      l4lock_lock(&old->fb_lock);
      if(old->vfb_used)
	/* save screen */
	asm volatile ("cld ; rep movsl"
	             :"=S"(dummy), "=D"(dummy), "=c"(dummy)
		     :"0"(vis_vmem), "1"(old->vfb),"2"(old->vfb_size/4));
      old->fb      = (old->vfb_used) ? old->vfb : 0;
      old->do_copy = bg_do_copy;
      old->do_fill = bg_do_fill;
      old->do_sync = bg_do_sync;
      l4lock_unlock(&old->fb_lock);
    }

  /* setup new vc */
  if (new != 0)
    {
      l4lock_lock(&new->fb_lock);
      new->fb      = gr_vmem;
      new->do_copy = fg_do_copy;
      new->do_fill = fg_do_fill;
      new->do_sync = fg_do_sync;
  
      if ((i_new == 0) || (!new->vfb_used))
	vc_clear(new);
      
      if(new->vfb_used)
	/* restore screen */
	memcpy(vis_vmem, new->vfb, new->vfb_size);

      /* force redraw of changed screen content (needed by VMware) */
      if (new->do_drty)
	new->do_drty(0, 0, VESA_XRES, VESA_YRES);
      
      update_id = 1;
      l4lock_unlock(&new->fb_lock);
    }
  
  /* tell old console that we flash its video memory */
  background_vc();
  
  /* flush video memory of old console.
   * XXX does not work on current Fiasco implementation because the
   * mapping database does not record mappings of adapter pages */
  l4_fpage_unmap(l4_fpage((l4_addr_t)gr_vmem & L4_SUPERPAGEMASK, 
			   L4_LOG2_SUPERPAGESIZE, 0, 0),
		 L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);
  
  ev_partner_l4id = vc[i_new]->ev_partner_l4id;
  vc_mode = vc[i_new]->mode;
}

/******************************************************************************
 * con_if - IDL server functions                                              *
 ******************************************************************************/

/** \brief Query for new (not opened) virtual console
 *
 * in:  request	      ... Flick request structure
 *      sbuf_size     ... max IPC string buffer
 * out: vcid          ... threadid of vc_thread
 *      _ev           ... Flick exception (unused)
 * ret: 0             ... success
 *      -???          ... if no VC unused */
l4_int32_t 
con_if_openqry_component(CORBA_Object _dice_corba_obj,
			 l4_uint32_t sbuf1_size,
			 l4_uint32_t sbuf2_size,
			 l4_uint32_t sbuf3_size,
			 l4_uint8_t priority,
			 l4_threadid_t *vcid,
			 l4_int16_t vfbmode,
			 CORBA_Environment *_dice_corba_env)
{
  l4thread_t vc_tid;
  l4_threadid_t vc_l4id;
  l4dm_dataspace_t ds;
  l4_offs_t ds_offs;
  l4_addr_t ds_map_addr;
  l4_size_t ds_map_size;
  int vc_num;
  char name[32];

  /* check sbuf_size */
  if (!sbuf1_size
      || ((sbuf2_size != 0) && (sbuf3_size == 0))
      || ((sbuf2_size == 0) && (sbuf3_size != 0))
      || (sbuf1_size + sbuf2_size + sbuf3_size) > CONFIG_MAX_SBUF_SIZE)
    {
      LOG("Wrong string buffer size");
      return -CON_EXPARAM;
    }

  /* find first available (non-opened) vc in list */
  if ((vc_num = get_free_vc()) == -CON_EFREE)
    return vc_num;

  /* allocate memory for sbuf */
  sprintf(name, "ipcbuf1 for %x.%x", _dice_corba_obj->id.task, _dice_corba_obj->id.lthread);
  if (!(vc[vc_num]->sbuf1 = l4dm_mem_allocate_named(sbuf1_size, L4RM_MAP,
						    name)))
    {
      LOG("Ooops, not enough memory for 1st string buffer");
      return -CON_ENOMEM;
    }

  vc[vc_num]->sbuf2 = 0;
  if (sbuf2_size > 0)
    {
      sprintf(name, "ipcbuf2 for %x.%x", 
	      _dice_corba_obj->id.task, _dice_corba_obj->id.lthread);
      if (sbuf2_size)
	{
	  if (!(vc[vc_num]->sbuf2 = l4dm_mem_allocate_named(
						sbuf2_size, L4RM_MAP, name)))
	    {
	      LOG("Ooops, not enough memory for 2nd string buffer");
	      l4dm_mem_release(vc[vc_num]->sbuf1);
	      return -CON_ENOMEM;
	    }
	}
    }

  vc[vc_num]->sbuf3 = 0;
  if (sbuf3_size > 0)
    {
      sprintf(name, "ipcbuf3 for %x.%x", 
	      _dice_corba_obj->id.task, _dice_corba_obj->id.lthread);
      if (sbuf3_size)
	{
	  if (!(vc[vc_num]->sbuf3 = l4dm_mem_allocate_named(
						  sbuf3_size, L4RM_MAP, name)))
	    {
	      LOG("Ooops, not enough memory for 3rd string buffer");
	      l4dm_mem_release(vc[vc_num]->sbuf1);
	      l4dm_mem_release(vc[vc_num]->sbuf2);
	      return -CON_ENOMEM;
	    }
	}
    }

  vc[vc_num]->fb = 0;
  vc[vc_num]->vfb = 0;
  vc[vc_num]->vfb_used = vfbmode;
  vc[vc_num]->mode = CON_OPENING;
  vc[vc_num]->vc_partner_l4id = *_dice_corba_obj;
  vc[vc_num]->sbuf1_size = sbuf1_size;
  vc[vc_num]->sbuf2_size = sbuf2_size;
  vc[vc_num]->sbuf3_size = sbuf3_size;
  vc[vc_num]->fb_mapped = 0;

  vc_tid = l4thread_create_long(L4THREAD_INVALID_ID,
				(l4thread_fn_t) vc_loop,
				L4THREAD_INVALID_SP,
				L4THREAD_DEFAULT_SIZE,
				priority,
				(void *) vc[vc_num],
				L4THREAD_CREATE_SYNC);

  vc_l4id = l4thread_l4_id(vc_tid);

  vc[vc_num]->vc_l4id = vc_l4id;
  vc[vc_num]->ev_partner_l4id = L4_NIL_ID;

  // transfer ownership of ipc buffers to client thread
  if (0 == l4rm_lookup(vc[vc_num]->sbuf1, &ds, &ds_offs, 
		       &ds_map_addr, &ds_map_size))
    l4dm_transfer(&ds, vc_l4id);
  if (0 == l4rm_lookup(vc[vc_num]->sbuf2, &ds, &ds_offs,
		       &ds_map_addr, &ds_map_size))
    l4dm_transfer(&ds, vc_l4id);
  if (0 == l4rm_lookup(vc[vc_num]->sbuf3, &ds, &ds_offs, 
		       &ds_map_addr, &ds_map_size))
    l4dm_transfer(&ds, vc_l4id);

  /* reply vc_thread id */
  *vcid  = vc_l4id;
  
  LOG("Your vc is %02d. (%x.%02x)",
      vc_num, vc_l4id.id.task, vc_l4id.id.lthread);
  
  return 0;
}

l4_int32_t 
con_if_screenshot_component(CORBA_Object _dice_corba_obj,
			    l4_int16_t vc_nr,
			    l4dm_dataspace_t *ds,
			    l4_uint32_t *xres,
			    l4_uint32_t *yres,
			    l4_uint32_t *bpp,
			    CORBA_Environment *_dice_corba_env)
{
  void *addr;
  struct l4con_vc *vc_shoot;
  
  if (vc_nr >= MAX_NR_L4CONS)
    return -L4_EINVAL;

  if (vc_nr == 0)
    vc_nr = fg_vc;
  
  vc_shoot = vc[vc_nr];

  if (!(vc_shoot->mode & CON_INOUT))
    return -L4_EINVAL;

  if (!vc_shoot->vfb_used && vc_nr != fg_vc)
    return -L4_EINVAL;

  l4lock_lock(&vc_shoot->fb_lock);
  
  if (!(addr = l4dm_mem_ds_allocate_named(vc_shoot->vfb_size, 0, "screenshot", 
	  				  (l4dm_dataspace_t*)ds)))
    {
      printf("Allocating dataspace failed\n");
      return -CON_ENOMEM;
    }

  /* XXX consider situations where 
   * bytes_per_line != bytes_per_pixel * xres !!! */
  memcpy(addr, (vc_shoot->vfb_used) ? vc_shoot->vfb : vc_shoot->fb,
         vc_shoot->vfb_size);
      
  l4lock_unlock(&vc_shoot->fb_lock);

  *xres = VESA_XRES;
  *yres = VESA_YRES;
  *bpp  = VESA_BITS;
  
  l4rm_detach(addr);

  /* transfer ds to caller so that it can be freed again */
  l4dm_transfer((l4dm_dataspace_t*)ds, *_dice_corba_obj);
  
  return 0;
}


/** \brief Close all virtual consoles of a client
 *
 * in:  request	      ... Flick request structure
 *      client        ... id of client to close all vcs off
 * out: _ev           ... Flick exception (unused)
 * ret: 0             ... success
 *      -???          ... if some error occured on closing a vc */
l4_int32_t 
con_if_close_all_component(CORBA_Object _dice_corba_obj,
			   const l4_threadid_t *client,
			   CORBA_Environment *_dice_corba_env)
{
  int i;

  for (i=0; i<MAX_NR_L4CONS; i++)
    {
      if (   (vc[i]->mode != CON_CLOSED)
	  && (vc[i]->mode != CON_CLOSING))
	{
	  /* don't use l4_task_equal here since we only know the task number */
	  if (vc[i]->vc_partner_l4id.id.task == client->id.task)
	    {
	      /* found console bound to client -- close it */
	      CORBA_Environment env = dice_default_environment;
	      con_vc_close_call(&(vc[i]->vc_l4id), &env);
	    }
	}
    }

  return 0;
}

void
switch_vc_on_timer(l4_msgdope_t result)
{
  if (L4_IPC_ERROR(result) == L4_IPC_RETIMEOUT) 
    {
      // switch to another console?
      l4lock_lock(&want_vc_lock);
      if (want_vc != fg_vc)
	switch_vc();
      l4lock_unlock(&want_vc_lock);

      // update status bar
      if (update_id && (fg_vc >= 0))
	{
	  l4lock_lock(&vc[fg_vc]->fb_lock);
	  vc_show_id(vc[fg_vc]);
	  l4lock_unlock(&vc[fg_vc]->fb_lock);
	  update_id = 0;
	}

      // update DROPS logo
      if (vc[fg_vc]->logo_x != 100000)
	{
	  l4lock_lock(&vc[fg_vc]->fb_lock);
	  vc_show_drops_cscs_logo();
	  l4lock_unlock(&vc[fg_vc]->fb_lock);
	}
    }
  else if (L4_IPC_ERROR(result) == L4_IPC_SETIMEOUT)
    {
      /* client did not wait, perhaps it was lthread_ex_regs'd */
      LOG("Timeout replying last client");
    }
  else
    {
      /* Ooops, we got an IPC error -> do something */
      PANIC("IDL IPC error (%#x, %s)", 
	  L4_IPC_ERROR(result), l4env_strerror(L4_IPC_ERROR(result)));
    }
}

/******************************************************************************
 * con_loop                                                                   *
 *                                                                            *
 * con_if - IDL server loop                                                   *
 ******************************************************************************/
static void __attribute__((noreturn))
con_loop(void)
{
  CORBA_Environment env = dice_default_environment;
  env.timeout = WAIT_TIMEOUT;
  con_if_server_loop(&env);
  for (;;);
}

extern int console_puts(const char *s);

static void
my_LOG_outstring(const char *s)
{
  console_puts(s);
}

/******************************************************************************
 * main                                                                       *
 *                                                                            *
 * Main function                                                              *
 ******************************************************************************/
int
main(int argc, char *argv[])
{
  l4_threadid_t me = l4thread_l4_id(l4thread_myself());
  
  /* command line params */
  do_args(argc, argv);
        
  /* do not use logserver (in case the log goes to a console) */
  if (nolog)
    LOG_outstring = my_LOG_outstring;

  /* vc */
  vc_init();

  /* ev */
  ev_init();

  /* switch to master console: initial screen output */
  do_switch(0, -1);
  vc_show_id(vc[0]);
  fg_vc = want_vc = 0;
  update_id = 0;

  /* we are up -> register at names */
  if (!names_register(CON_NAMES_STR))
    {
      PANIC("can't register at names");
      exit(-1);
    }

  printf("Running as %x.%02x. Video mode is %dx%d@%d.\n",
	 me.id.task, me.id.lthread,
	 VESA_XRES, VESA_YRES, VESA_BITS);

  /* idl service loop */
  con_loop();
  
  exit(0);
}

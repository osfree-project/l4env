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
#include <l4/l4con/l4con.h>
#include <l4/l4con/l4con_ev.h>
#include <l4/l4con/l4con-client.h>
#include <l4/l4con/l4con-server.h>
#include <l4/l4con/stream-client.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/thread/thread.h>
#include <l4/util/parse_cmd.h>
#include <l4/util/kip.h>
#include <l4/lock/lock.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>

/* OSKit includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* local includes */
#include "l4con.h"
#include "main.h"
#include "vc.h"
#include "ev.h"
#include "ipc.h"
#include "con_config.h"
#include "con_macros.h"
#include "events.h"

char LOG_tag[9] = CONTAG;
l4_ssize_t l4libc_heapsize = CONFIG_MALLOC_MAX_SIZE;

l4_uint16_t VESA_XRES, VESA_YRES, VESA_BPL;
l4_uint8_t  VESA_BITS, VESA_RES;
l4_uint8_t  VESA_RED_OFFS, VESA_GREEN_OFFS, VESA_BLUE_OFFS;
l4_uint8_t  VESA_RED_SIZE, VESA_GREEN_SIZE, VESA_BLUE_SIZE;
l4_uint8_t  FONT_XRES, FONT_YRES;
l4_uint32_t FONT_CHRS;

int noaccel;				/* 1=disable pslim HW acceleration */
int nolog;				/* 1=disable logging to logserver */
int pan;				/* 1=pan display to 4MB boundary */
int use_fastmemcpy;			/* 1=fast memcpy using SSE2 */
int cpu_load;
int vbemode;

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

static void
fast_memcpy(char *dst, char *src, l4_size_t size)
{
#ifdef ARCH_x86
  l4_umword_t dummy;

  asm volatile ("cld ; rep movsl"
		:"=S"(dummy), "=D"(dummy), "=c"(dummy)
                :"S"(src), "D"(dst),"c"(size/4));
#else
  memcpy(dst, src, size);
#endif
}

/* fast memcpy using movntq (moving using non-temporal hint)
 * cache line size is 32 bytes */
static void
fast_memcpy_mmx2_32(char *dst, char *src, l4_size_t size)
{
#ifdef ARCH_x86
  l4_mword_t dummy;

  /* don't execute emms in side the timer loop because at this point the
   * fpu state is lazy allocated so we may have a kernel entry here */
  asm ("emms");

  asm volatile ("lea    (%%esi,%%edx,8),%%esi     \n\t"
		"lea    (%%edi,%%edx,8),%%edi     \n\t"
		"neg    %%edx                     \n\t"
		".align 8                         \n\t"
		"0:                               \n\t"
		"mov    $64,%%ecx                 \n\t"
		".align 16                        \n\t"
		"# block prefetch 4096 bytes      \n\t"
		"1:                               \n\t"
		"movl   (%%esi,%%edx,8),%%eax     \n\t"
		"movl   32(%%esi,%%edx,8),%%eax   \n\t"
		"add    $8,%%edx                  \n\t"
		"dec    %%ecx                     \n\t"
		"jnz    1b                        \n\t"
		"sub    $(32*16),%%edx            \n\t"
		"mov    $128,%%ecx                \n\t"
		".align 16                        \n\t"
		"2: # copy 4096 bytes             \n\t"
		"movq   (%%esi,%%edx,8),%%mm0     \n\t"
		"movq   8(%%esi,%%edx,8),%%mm1    \n\t"
		"movq   16(%%esi,%%edx,8),%%mm2   \n\t"
		"movq   24(%%esi,%%edx,8),%%mm3   \n\t"
		"movntq %%mm0,(%%edi,%%edx,8)     \n\t"
		"movntq %%mm1,8(%%edi,%%edx,8)    \n\t"
		"movntq %%mm2,16(%%edi,%%edx,8)   \n\t"
		"movntq %%mm3,24(%%edi,%%edx,8)   \n\t"
		"add    $4,%%edx                  \n\t"
		"dec    %%ecx                     \n\t"
		"jnz    2b                        \n\t"
		"or     %%edx,%%edx               \n\t"
		"jnz    0b                        \n\t"
		"sfence                           \n\t"
		"emms                             \n\t"
		: "=d" (dummy)
		: "S" (src), "D" (dst), "d" (size/8)
		: "eax", "ebx", "ecx", "memory");
#else
  memcpy(dst, src, size);
#endif
}

/** Find first available (non-opened) vc in list. */
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

/** send con_event redraw. */
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

       env.timeout = EVENT_TIMEOUT;
       stream_io_push_call(&ev_partner_l4id, &ev_struct, &env);

	if (env.major == CORBA_SYSTEM_EXCEPTION &&
	    env.repos_id == CORBA_DICE_EXCEPTION_IPC_ERROR)
	  {
	    if (env._p.ipc_error == L4_IPC_ENOT_EXISTENT)
	      {
		printf("redraw_vc: Target thread "l4util_idfmt" dead?\n",
		    l4util_idstr(ev_partner_l4id));
		ret = -1;
	      }
	    else
	      printf("redraw_vc: IPC error %02x\n", env._p.ipc_error);
	  }
     }
   l4lock_unlock(&want_vc_lock);

   return ret;
}


/** send con_event background. */
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

       env.timeout = EVENT_TIMEOUT;
       stream_io_push_call(&ev_partner_l4id, &ev_struct, &env);

       /* ignore errors */
       vc[fg_vc]->fb_mapped = 0;
     }
   l4lock_unlock(&want_vc_lock);

   return 0;
}

/** Announce that we want to switch to another console.
 *  This function should not block for a longer time. */
void
request_vc(int nr)
{
  l4lock_lock(&want_vc_lock);
  want_vc = nr;
  l4lock_unlock(&want_vc_lock);
}

/** Switch to other console (want_vc != fg_vc).
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
      l4lock_lock(&old->fb_lock);
      if(old->vfb_used)
	{
	  /* save screen */
	  if (use_fastmemcpy && (new->vfb_size % 4096 == 0))
	    /* superfast memcpy */
	    fast_memcpy_mmx2_32(old->vfb, vis_vmem, old->vfb_size);
	  else
	    fast_memcpy        (old->vfb, vis_vmem, old->vfb_size);
	}
      old->fb      = old->vfb_used ? old->vfb : 0;
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

      if (i_new == 0 || !new->vfb_used)
	vc_clear(new);

      if(new->vfb_used)
	/* restore screen */
	{
	  if (use_fastmemcpy && (new->vfb_size % 4096 == 0))
	    /* superfast memcpy */
	    fast_memcpy_mmx2_32(vis_vmem, new->vfb, new->vfb_size);
	  else
	    fast_memcpy        (vis_vmem, new->vfb, new->vfb_size);
	}

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
			 CORBA_Server_Environment *_dice_corba_env)
{
  l4thread_t vc_tid;
  l4_threadid_t vc_l4id, dummy;
  l4dm_dataspace_t ds;
  l4_offs_t ds_offs;
  l4_addr_t ds_map_addr;
  l4_size_t ds_map_size;
  int vc_num;
  char name[32];

  /* check sbuf_size */
  if (!sbuf1_size
      || (sbuf2_size != 0 && sbuf3_size == 0)
      || (sbuf2_size == 0 && sbuf3_size != 0)
      || (sbuf1_size + sbuf2_size + sbuf3_size) > CONFIG_MAX_SBUF_SIZE)
    {
      LOG("Wrong string buffer size");
      return -CON_EXPARAM;
    }

  /* find first available (non-opened) vc in list */
  if ((vc_num = get_free_vc()) == -CON_EFREE)
    return vc_num;

  /* allocate memory for sbuf */
  sprintf(name, "ipcbuf1 for "l4util_idfmt"", l4util_idstr(*_dice_corba_obj));
  if (!(vc[vc_num]->sbuf1 =
	l4dm_mem_allocate_named(sbuf1_size, L4RM_MAP, name)))
    {
      LOG("Ooops, not enough memory for 1st string buffer");
      return -CON_ENOMEM;
    }

  vc[vc_num]->sbuf2 = 0;
  if (sbuf2_size > 0)
    {
      sprintf(name, "ipcbuf2 for "l4util_idfmt"",
	  l4util_idstr(*_dice_corba_obj));
      if (sbuf2_size)
	{
	  if (!(vc[vc_num]->sbuf2 =
		l4dm_mem_allocate_named(sbuf2_size, L4RM_MAP, name)))
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
      sprintf(name, "ipcbuf3 for "l4util_idfmt"",
	      l4util_idstr(*_dice_corba_obj));
      if (sbuf3_size)
	{
	  if (!(vc[vc_num]->sbuf3 =
		l4dm_mem_allocate_named(sbuf3_size, L4RM_MAP, name)))
	    {
	      LOG("Ooops, not enough memory for 3rd string buffer");
	      l4dm_mem_release(vc[vc_num]->sbuf1);
	      l4dm_mem_release(vc[vc_num]->sbuf2);
	      return -CON_ENOMEM;
	    }
	}
    }

  vc[vc_num]->fb              = 0;
  vc[vc_num]->vfb             = 0;
  vc[vc_num]->vfb_used        = vfbmode;
  vc[vc_num]->mode            = CON_OPENING;
  vc[vc_num]->vc_partner_l4id = *_dice_corba_obj;
  vc[vc_num]->sbuf1_size      = sbuf1_size;
  vc[vc_num]->sbuf2_size      = sbuf2_size;
  vc[vc_num]->sbuf3_size      = sbuf3_size;
  vc[vc_num]->fb_mapped       = 0;

  sprintf(name, ".vc-%.2d", vc_num);
  vc_tid = l4thread_create_long(L4THREAD_INVALID_ID,
				(l4thread_fn_t) vc_loop,
				name,
				L4THREAD_INVALID_SP,
				L4THREAD_DEFAULT_SIZE,
				priority,
				(void *) vc[vc_num],
				L4THREAD_CREATE_SYNC);

  vc_l4id = l4thread_l4_id(vc_tid);

  vc[vc_num]->vc_l4id         = vc_l4id;
  vc[vc_num]->ev_partner_l4id = L4_NIL_ID;

  // transfer ownership of ipc buffers to client thread
  if (L4RM_REGION_DATASPACE == l4rm_lookup(vc[vc_num]->sbuf1, &ds_map_addr,
                                           &ds_map_size, &ds, &ds_offs, &dummy))
    l4dm_transfer(&ds, vc_l4id);
  if (L4RM_REGION_DATASPACE == l4rm_lookup(vc[vc_num]->sbuf2, &ds_map_addr,
                                           &ds_map_size, &ds, &ds_offs, &dummy))
    l4dm_transfer(&ds, vc_l4id);
  if (L4RM_REGION_DATASPACE == l4rm_lookup(vc[vc_num]->sbuf3, &ds_map_addr,
                                           &ds_map_size, &ds, &ds_offs, &dummy))
    l4dm_transfer(&ds, vc_l4id);

  /* reply vc_thread id */
  *vcid  = vc_l4id;

  return 0;
}

l4_int32_t
con_if_screenshot_component(CORBA_Object _dice_corba_obj,
			    l4_int16_t vc_nr,
			    l4dm_dataspace_t *ds,
			    l4_uint32_t *xres,
			    l4_uint32_t *yres,
			    l4_uint32_t *bpp,
			    CORBA_Server_Environment *_dice_corba_env)
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
			   CORBA_Server_Environment *_dice_corba_env)
{
  int i;

  for (i=0; i<MAX_NR_L4CONS; i++)
    {
      if (vc[i]->mode != CON_CLOSED && vc[i]->mode != CON_CLOSING)
	{
	  /* don't use l4_task_equal here since we only know the task number */
	  if (vc[i]->vc_partner_l4id.id.task == client->id.task)
	    {
	      /* found console bound to client -- close it */
	      CORBA_Environment env = dice_default_environment;
	      int ret = con_vc_close_call(&(vc[i]->vc_l4id), &env);
	      if (ret || env.major != CORBA_NO_EXCEPTION)
		printf("Error %d (env=%02x) closing app "l4util_idfmt
		       " service thread "l4util_idfmt,
		    ret, env._p.ipc_error, l4util_idstr(*client),
		    l4util_idstr(vc[i]->vc_l4id));
	    }
	}
    }

  return 0;
}

static void
test_periodic(void)
{
  static l4_umword_t last_active_fast;
  static l4_umword_t last_active_slow;
  l4_umword_t clock = l4util_kip()->clock;

  if (clock - last_active_fast >= 25000)
    {
      // switch to another console?
      l4lock_lock(&want_vc_lock);
      if (want_vc != fg_vc)
	switch_vc();
      l4lock_unlock(&want_vc_lock);

      // update status bar
      if (update_id && fg_vc >= 0)
	{
	  l4lock_lock(&vc[fg_vc]->fb_lock);
	  vc_show_id(vc[fg_vc]);
	  l4lock_unlock(&vc[fg_vc]->fb_lock);
	  update_id = 0;
	  // force updating load indicator
	  last_active_slow = clock - 1000000;
	}

      // update DROPS logo
      if (vc[fg_vc]->logo_x != 100000)
	{
	  l4lock_lock(&vc[fg_vc]->fb_lock);
	  vc_show_drops_cscs_logo();
	  l4lock_unlock(&vc[fg_vc]->fb_lock);
	}

      last_active_fast = clock;
    }

  if (clock - last_active_slow >= 1000000)
    {
      l4lock_lock(&vc[fg_vc]->fb_lock);

      // update memory information
      vc_show_dmphys_poolsize(vc[fg_vc]);

      if (cpu_load)
	// update load indicator
	vc_show_cpu_load(vc[fg_vc]);

      l4lock_unlock(&vc[fg_vc]->fb_lock);
      last_active_slow = clock;
    }
}

void
switch_vc_on_timer(l4_msgdope_t result, CORBA_Server_Environment* env)
{
  if (L4_IPC_ERROR(result) == L4_IPC_RETIMEOUT)
    test_periodic();

  else if (L4_IPC_ERROR(result) == L4_IPC_SETIMEOUT)
    /* client did not wait, perhaps it was lthread_ex_regs'd */
    LOG("Timeout replying last client");

  else
    /* Ooops, we got an IPC error -> do something */
    PANIC("IDL IPC error (%#x, %s)",
	  L4_IPC_ERROR(result), l4env_strerror(L4_IPC_ERROR(result)));
}

static void
server_loop(void)
{
  CORBA_Server_Environment corba_env = dice_default_server_environment;
  CORBA_Object_base corba_obj;
  con_if_msg_buffer_t msg_buffer;
  l4_int16_t reply;
  l4_int32_t opcode;

  corba_env.timeout          = REQUEST_TIMEOUT;
  msg_buffer._dice_size_dope = L4_IPC_DOPE(8, 0);

  for (;;)
    {
      opcode = con_if_wait_any(&corba_obj, &msg_buffer, &corba_env);
      for (;;)
	{
	  reply = con_if_dispatch(&corba_obj, opcode, &msg_buffer, &corba_env);
	  /* Make sure we call this function even if the receive timeout is
	   * never triggered. */
	  test_periodic();
	  if (reply != DICE_REPLY)
	    break;
	  if (corba_env.major    == CORBA_SYSTEM_EXCEPTION &&
	      corba_env.repos_id == CORBA_DICE_EXCEPTION_WRONG_OPCODE)
	    {
	      /* XXX Most probably a stream_io_push_call() failed and now we
	       * got the answer. Ignore it. */
	      printf("Wrong opcode %08x from "l4util_idfmt" -- ignoring\n",
		  opcode, l4util_idstr(corba_obj));
	      break;
	    }
	  opcode = con_if_reply_and_wait(&corba_obj, &msg_buffer, &corba_env);
	}
    }
}

extern int console_puts(const char *s);
static void
my_LOG_outstring(const char *s)
{
  console_puts(s);
}

/* Make sure that the jiffies symbol is taken from libio.a not libinput.a. */
asm (".globl jiffies");

/** \brief Main function
 */
int
main(int argc, const char *argv[])
{
  l4_threadid_t me = l4thread_l4_id(l4thread_myself());
  int error;
  int use_events = 0;

#ifdef ARCH_arm
  noaccel = 1;
#endif

  l4util_kip_map();

  if ((error = parse_cmdline(&argc, &argv,
		    'a', "noaccel", "disable hardware acceleration",
		    PARSE_CMD_SWITCH, 1, &noaccel,
#ifdef ARCH_x86
		    'c', "cpuload", "show CPU load using rdtsc and rdpmc(0)",
		    PARSE_CMD_SWITCH, 1, &cpu_load,
#endif
		    'e', "events", "use event server to free resources",
		    PARSE_CMD_SWITCH, 1, &use_events,
#ifdef ARCH_x86
		    'f', "fastmemcpy", "use fast memcpy with SSE2",
		    PARSE_CMD_SWITCH, 1, &use_fastmemcpy,
#endif
		    'i', "l4io", "use L4 I/O server for PCI management",
		    PARSE_CMD_SWITCH, 1, &use_l4io,
		    'l', "nolog", "don't connect to logserver",
		    PARSE_CMD_SWITCH, 1, &nolog,
		    'm', "nomouse", "don't transmit mouse events to clients",
		    PARSE_CMD_SWITCH, 1, &nomouse,
		    'o', "omega0", "use omega0 for IRQ management",
		    PARSE_CMD_SWITCH, 1, &use_omega0,
		    'p', "pan", "use panning to restrict client window",
		    PARSE_CMD_SWITCH, 1, &pan,
		    'v', "vbemode", "set VESA mode",
		    PARSE_CMD_INT, 0, &vbemode,
		    0)))
    {
      switch (error)
	{
	case -1: printf("Bad parameter for parse_cmdline()\n"); break;
	case -2: printf("Out of memory in parse_cmdline()\n"); break;
	case -4: return 1;
	default: printf("Error %d in parse_cmdline()\n", error); break;
	}
    }

  /* do not use logserver (in case the log goes to a console) */
  if (nolog)
    LOG_outstring = my_LOG_outstring;

  vc_init();

  /* switch to master console: initial screen output (DROPS logo) */
  do_switch(0, -1);
  vc_show_id(vc[0]);
  fg_vc = want_vc = 0;
  update_id = 0;

  ev_init();

  /* start thread listening for exit events */
  if (use_events)
    init_events();

  /* we are up -> register at names */
  if (!names_register(CON_NAMES_STR))
    {
      PANIC("can't register at names");
      exit(-1);
    }

  printf("Running as "l4util_idfmt". Video mode is %dx%d@%d.\n",
	 l4util_idstr(me), VESA_XRES, VESA_YRES, VESA_BITS);

  /* idl service loop */
  server_loop();

  exit(0);
}

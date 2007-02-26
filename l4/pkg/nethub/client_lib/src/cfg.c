/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/nethub/cfg.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>

#include <l4/nethub/base.h>

int nh_sadb_map_notify_buffer(Hub hub, struct Nh_sadb_notify_head *buffer)
{
  CORBA_Environment e = dice_default_environment;
  l4_snd_fpage_t fp;
  int ret;

  e.rcv_fpage.fp.write = 1;
  e.rcv_fpage.fp.size = 12;
  e.rcv_fpage.fp.page = (unsigned long)buffer >> 12;

  ret = Nh_cfg_listen_call(&hub,&fp,&e);

  return ret;
}

struct Nh_sadb_notify_head *
nh_sadb_next_notification(struct Nh_sadb_notify_head *last)
{
  char *buffer_base, *next;
  struct Nh_sadb_notify_head *nh;
  
  buffer_base = (char*)(((unsigned long)last) 
                        & ~(NH_SADB_NOTIFY_BUFFER_ALIGN - 1));

  next = (char*)last + NH_SADB_NOTIFY_SZ_EXPIRE;
  if (next >= (buffer_base + NH_SADB_NOTIFY_SZ_BUFFER))
    next = buffer_base;

  nh = (struct Nh_sadb_notify_head*)last;
  if (nh->type & NH_SADB_NOTIFY_COMMITED)
    return nh;
  else
    return NULL;
}

void 
nh_sadb_clear_notification(struct Nh_sadb_notify_head *nfy)
{
  nfy->type = NH_SADB_NOTIFY_EMPTY;
}


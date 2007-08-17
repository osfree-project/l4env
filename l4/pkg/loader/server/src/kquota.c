/******************************************************************************
 * \file        loader/server/src/kquota.c
 * \brief       kernel memory quotas
 *
 * \date        08-2007
 * \author      Bjoern Doebel <doebel@os.inf.tu-dresden.de>
 *
 * (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <l4/env/errno.h>

#include "kquota.h"

cfg_kquota_t *kquota_list;

int add_kquota(char *name, unsigned size)
{
  cfg_kquota_t *new_quota = malloc(sizeof(cfg_kquota_t));

  if (!new_quota)
    {
      printf("Could not allocate new quota.\n");
      return -L4_ENOMEM;
    }

  printf("Adding new quota, name = '%s', size = %u\n",
         name, size);

  strncpy(new_quota->name, name, KQUOTA_NAME_LENGTH);
  new_quota->name[KQUOTA_NAME_LENGTH-1] = '\0';
  new_quota->size = size;
  new_quota->users = NULL;

  // push_front()
  new_quota->next = kquota_list;
  kquota_list = new_quota;

  return 0;
}

cfg_kquota_t *get_kquota(const char *name)
{
  cfg_kquota_t *ret = kquota_list;

  while (ret)
    {
      if (!strcmp(ret->name, name))
        break;
      ret = ret->next;
    }

  return ret;
}

int kquota_add_user(cfg_kquota_t *kq, l4_threadid_t user)
{
  kquota_user_t *u = malloc(sizeof(kquota_user_t));

  if (!u)
    {
      printf("Cannot allocate memory for kquota user.\n");
      return -L4_ENOMEM;
    }

  u->tid = user;
  // push_front()
  u->next = kq->users;
  kq->users = u;

  return 0;
}

void kquota_del_user(cfg_kquota_t *kq, l4_threadid_t user)
{
  kquota_user_t *u = kq->users;
  kquota_user_t *p = kq->users;

  if (!u)
    return;

  /* Need to remove first entry? */
  if (l4_thread_equal(kq->users->tid, user))
    kq->users = kq->users->next;
  else
    {
      /* Find entry to remove. */
      while (u)
        {
          if (l4_thread_equal(u->tid, user))
            {
              p->next = u->next;
              break;
            }
          p = u;
          u = u->next;
        }
    }

  if (u)
    free(u);
}

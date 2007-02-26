/* $Id$ */
/**
 * \file	sigma0/lib/src/kip.c
 * \brief	map kernel info page using sigma0 protocol
 *
 * \date	02/2006
 * \author	Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 * 		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kernel.h>
#include <l4/sigma0/kip.h>
#include <l4/sigma0/sigma0.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char kip_area[];
extern int rmgr_init(void);
extern l4_threadid_t rmgr_pager_id;
static l4_kernel_info_t *kip;

/**
 * Map kernel info page into kip_area.
 */
l4_kernel_info_t*
l4sigma0_kip_map(l4_threadid_t pager)
{
  l4_snd_fpage_t fp;
  l4_msgdope_t dope;
  int e;

  if (kip && kip->magic == L4_KERNEL_INFO_MAGIC)
    return kip;

  l4sigma0_kip_unmap();

  if (l4_is_invalid_id(pager))
    {
      if (!rmgr_init())
	{
	  printf("rmgr_init() failed\n");
	  return 0;
	}
      pager = rmgr_pager_id;
    }

  e = l4_ipc_call(pager, L4_IPC_SHORT_MSG, SIGMA0_REQ_KIP, 0,
                  L4_IPC_MAPMSG((l4_addr_t)kip_area, L4_LOG2_PAGESIZE),
                  &fp.snd_base, &fp.snd_base, L4_IPC_NEVER, &dope);

  if (e || !l4_ipc_fpage_received(dope))
    return NULL;

  kip = (l4_kernel_info_t*)kip_area;
  if (kip->magic != L4_KERNEL_INFO_MAGIC)
    kip = 0;

  return kip;
}

/**
 * Unmap kernel info page from kip_area
 */
void
l4sigma0_kip_unmap()
{
  l4_fpage_unmap(l4_fpage((l4_addr_t)kip_area, L4_LOG2_PAGESIZE,
			  L4_FPAGE_RW, L4_FPAGE_MAP),
		 L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);

  kip = 0;
}

/**
 * Return pointer to mapped kernel info page.
 */
l4_kernel_info_t*
l4sigma0_kip()
{
  return kip;
}

/**
 * Return version as found in the kernel info page.
 */
l4_umword_t
l4sigma0_kip_version()
{
  return l4sigma0_kip_map(L4_INVALID_ID) ? kip->version : 0;
}

const char*
l4sigma0_kip_version_string()
{
  return l4sigma0_kip_map(L4_INVALID_ID) 
	    ? kip_area + l4_kernel_info_version_offset(kip) : 0;
}

int
l4sigma0_kip_kernel_is_ux(void)
{
  const char *s = l4sigma0_kip_version_string();

  if (s && strstr(s, "(ux)"))
    return 1;
  return 0;
}

int
l4sigma0_kip_kernel_has_feature(const char *str)
{
  const char *s = l4sigma0_kip_version_string();

  l4sigma0_kip_for_each_feature(s)
    {
      if (strcmp(s, str) == 0)
	return 1;
    }

  return 0;
}

unsigned long
l4sigma0_kip_kernel_abi_version(void)
{
  const char *s = l4sigma0_kip_version_string();

  if (!kip)
    return 0;

  l4sigma0_kip_for_each_feature(s)
    {
      if (strncmp(s, "abiver:", 7) == 0)
	return strtoul(s + 7, NULL, 0);
    }

  return 0;
}

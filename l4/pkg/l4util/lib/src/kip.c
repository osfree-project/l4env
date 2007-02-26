/* 
 * \brief  Some very basic functions to get the KIP (and to get rid of it again)
 *
 * \data   11/2003
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 */

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/kip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char kip_area[];
static l4_kernel_info_t *kip;

l4_kernel_info_t*
l4util_kip_map()
{
  l4_snd_fpage_t fp;
  l4_msgdope_t dope;
  int e;

  if (kip && kip->magic == L4_KERNEL_INFO_MAGIC)
    return kip;

  if (!rmgr_init())
    {
      printf("rmgr_init failed!\n");
      return 0;
    }

  l4util_kip_unmap();

  e = l4_ipc_call(rmgr_pager_id, L4_IPC_SHORT_MSG, 1, 1,
                  L4_IPC_MAPMSG((l4_addr_t)kip_area, L4_LOG2_PAGESIZE),
                  &fp.snd_base, &fp.snd_base, L4_IPC_NEVER, &dope);

  if (e || !l4_ipc_fpage_received(dope))
    return NULL;

  kip = (l4_kernel_info_t*)kip_area;
  if (kip->magic != L4_KERNEL_INFO_MAGIC)
    kip = 0;

  return kip;
}

void
l4util_kip_unmap()
{
  l4_fpage_unmap(l4_fpage((l4_addr_t)kip_area, L4_LOG2_PAGESIZE,
			  L4_FPAGE_RW, L4_FPAGE_MAP),
		 L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);

  kip = 0;
}

l4_kernel_info_t*
l4util_kip()
{
  return kip;
}

l4_umword_t
l4util_kip_version()
{
  return kip ? kip->version : 0;
}

const char*
l4util_kip_version_string()
{
  return kip ? kip_area + l4_kernel_info_version_offset(kip) : 0;
}

int
l4util_kip_kernel_is_ux(void)
{
  const char *s = l4util_kip_version_string();

  if (s && strstr(s, "(ux)"))
    return 1;
  return 0;
}

int
l4util_kip_kernel_has_feature(const char *str)
{
  const char *s = l4util_kip_version_string();

  if (!kip)
    return 0;

  l4util_kip_for_each_feature(s)
    {
      if (strcmp(s, str) == 0)
	return 1;
    }

  return 0;
}

unsigned long
l4util_kip_kernel_abi_version(void)
{
  const char *s = l4util_kip_version_string();

  if (!kip)
    return 0;

  l4util_kip_for_each_feature(s)
    {
      if (strncmp(s, "abiver:", 7) == 0)
	return strtoul(s + 7, NULL, 0);
    }

  return 0;
}

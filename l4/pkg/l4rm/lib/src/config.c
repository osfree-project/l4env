/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/config.c
 * \brief  Request configuration data
 *
 * \date   01/25/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/sys/kernel.h>
#include <l4/env/env.h>
#include <l4/util/macros.h>
#include <l4/util/memdesc.h>
#include <l4/names/libnames.h>

/* L4RM includes */
#include <l4/l4rm/l4rm.h>
#include "__config.h"

/*****************************************************************************
 *** external configuration
 *****************************************************************************/

/// heap map area start address, if set to -1, a suitable address is used
const l4_addr_t l4rm_heap_start_addr __attribute__ ((weak)) = -1;

/*****************************************************************************
 *** L4RM internal library functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Get VM start address
 *
 * \return VM start address
 */
/*****************************************************************************/
l4_addr_t
l4rm_get_vm_start(void)
{
  l4_addr_t vm_start;

  /* query environment */
  if (l4env_request_config_u32(L4ENV_VM_LOW, &vm_start) == 0)
    return vm_start;
  else
    return L4RM_VM_START;
}

/*****************************************************************************/
/**
 * \brief Get VM end address
 *
 * \return VM end address
 */
/*****************************************************************************/
l4_addr_t
l4rm_get_vm_end(void)
{
  l4_addr_t vm_end;

  /* query environment */
  if (l4env_request_config_u32(L4ENV_VM_HIGH, &vm_end) == 0)
    return vm_end;
  else
#if defined ARCH_x86 || ARCH_amd64
    return L4RM_VM_END - 1;
#else
    {
      if ((vm_end = l4util_memdesc_vm_high()))
	return vm_end;

      Panic("No virtual memory descriptor available in the KIP!");
    }
#endif
}

/*****************************************************************************/
/**
 * \brief Get dataspace manager.
 *
 * \return Dataspace manager id, L4_INVALID_ID if not found.
 */
/*****************************************************************************/
l4_threadid_t
l4rm_get_dsm(void)
{
  l4_threadid_t dsm_id;

  /* request dm from environment lib */
  dsm_id = l4env_get_default_dsm();
  if (l4_is_invalid_id(dsm_id))
    {
      Panic("L4RM: no dataspace manager found!");
      return L4_INVALID_ID;
   }

  /* done */
  return dsm_id;
}

/*****************************************************************************/
/**
 * \brief Get Sigma0.
 *
 * \return Sigma0 id, L4_INVALID_ID if not found.
 */
/*****************************************************************************/
l4_threadid_t
l4rm_get_sigma0(void)
{
  int ret;
  l4_threadid_t sigma0_id;

  /* request sigma0 from environment lib */
  ret = l4env_request_service(L4ENV_SIGMA0, &sigma0_id);
  if (ret < 0)
    {
      /* no id provided by the environment */
      Panic("L4RM: sigma0 not found!");
      return L4_INVALID_ID;
    }

  /* done */
  return sigma0_id;
}

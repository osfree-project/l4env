/**
 * \file   l4util/include/kip.h
 * \brief  KIP acquiration
 *
 * \date   11/2003
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2 as
 * published by the Free Software Foundation (see the file COPYING).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For different licensing schemes please contact
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/
#ifndef _L4UTIL_KIP_H
#define _L4UTIL_KIP_H

#include <l4/sys/kernel.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

#define L4UTIL_KIP_VERSION_FIASCO	0x01004444

/** \defgroup kip Kernel Info Page handling functions */

/**
 * \brief  Return the address of the kernel info page.
 * \ingroup kip
 *
 * \return Address to the kernel info page or 0 if page is invalid.
 */
l4_kernel_info_t *l4util_kip(void);

/**
 * \brief  Map the kernel info page.
 * \ingroup kip
 *
 * Maps the kernel info page to a library private address.
 *
 * \return Address to the kernel info page or 0 if page is invalid.
 */
l4_kernel_info_t *l4util_kip_map(void);

/**
 * \brief  Unmap the kernel info page.
 * \ingroup kip
 *
 * Unmaps the kernel info page from the library private address.
 */
void l4util_kip_unmap(void);

/**
 * \brief  Get the kernel version.
 * \ingroup kip
 *
 * Returns the kernel version. The KIP has to be mapped already.
 *
 * \return Kernel version string.
 */
l4_umword_t l4util_kip_version(void);

/**
 * \brief  Get the kernel version string.
 * \ingroup kip
 *
 * Returns the kernel version banner string. The KIP has to be mapped
 * already via l4util_kip_map().
 *
 * \return Kernel version string.
 */
const char *l4util_kip_version_string(void);

/**
 * \brief Return whether the kernel is running native or under UX.
 * \ingroup kip
 *
 * Returns whether the kernel is running natively or under UX. The KIP has
 * to be mapped already.
 *
 * \return 1 when running under UX, 0 if not running under UX
 */
int l4util_kip_kernel_is_ux(void);

/**
 * \brief Check if kernel supports a feature.
 * \ingroup kip
 *
 * \param str   Feature name to check.
 *
 * \return 1 if the kernel supports the feature, 0 if not.
 *
 * Checks the feature field in the KIP for the given string. The KIP
 * has to be mapped already via l4util_kip_map().
 */
int l4util_kip_kernel_has_feature(const char *str);

/**
 * \brief Return kernel ABI version.
 * \ingroup kip
 *
 * \return Kernel ABI version.
 */
unsigned long l4util_kip_kernel_abi_version(void);

EXTERN_C_END

/**
 * \brief Cycle through kernel features given in the KIP.
 * \ingroup kip
 *
 * Cycles through all KIP kernel feature strings. s must be a character
 * pointer (char *) initialized with l4util_kip_version_string().
 */
#define l4util_kip_for_each_feature(s)				\
		for (s += strlen(s) + 1; *s; s += strlen(s) + 1)

#endif /* ! _L4UTIL_KIP_H */


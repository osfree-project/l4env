#ifndef __L4_SIGMA0_KIP_H
#define __L4_SIGMA0_KIP_H

#include <l4/sys/kernel.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

#define L4SIGMA0_KIP_VERSION_FIASCO	0x87004444

/** \defgroup kip Kernel Info Page handling functions 
 * This group of functions should be moved to l4sys!
 */

/**
 * \brief  Return the address of the kernel info page.
 * \ingroup kip
 *
 * \return Address to the kernel info page or 0 if page is invalid.
 */
l4_kernel_info_t *l4sigma0_kip(void);

/**
 * \brief  Map the kernel info page.
 * \ingroup kip
 *
 * Maps the kernel info page to a library private address.
 *
 * \param  pager  pager implementing the Sigma0 protocol
 *                (use L4_INVALID_ID to call rmgr_init() and use
 *                 rmgr_pager_id)
 * \return Address to the kernel info page or 0 if page is invalid.
 */
l4_kernel_info_t *l4sigma0_kip_map(l4_threadid_t pager);

/**
 * \brief  Unmap the kernel info page.
 * \ingroup kip
 *
 * Unmaps the kernel info page from the library private address.
 */
void l4sigma0_kip_unmap(void);

/**
 * \brief  Get the kernel version.
 * \ingroup kip
 *
 * Returns the kernel version. The KIP will be mapped if not already mapped.
 * The KIP will not be unmapped again.
 *
 * \return Kernel version string. 0 if KIP could not be mapped.
 */
l4_umword_t l4sigma0_kip_version(void);

/**
 * \brief  Get the kernel version string.
 * \ingroup kip
 *
 * Returns the kernel version banner string. The KIP will be mapped if not
 * already mapped. The KIP will not be unmapped again.
 *
 * \return Kernel version string.
 */
const char *l4sigma0_kip_version_string(void);

/**
 * \brief Return whether the kernel is running native or under UX.
 * \ingroup kip
 *
 * Returns whether the kernel is running natively or under UX. The KIP will
 * be mapped if not already mapped. The KIP will not be unmapped again.
 *
 * \return 1 when running under UX, 0 if not running under UX
 */
int l4sigma0_kip_kernel_is_ux(void);

/**
 * \brief Check if kernel supports a feature.
 * \ingroup kip
 *
 * \param str   Feature name to check.
 *
 * \return 1 if the kernel supports the feature, 0 if not.
 *
 * Checks the feature field in the KIP for the given string. The KIP will be
 * mapped if not already mapped. The KIP will not be unmapped again.
 */
int l4sigma0_kip_kernel_has_feature(const char *str);

/**
 * \brief Return kernel ABI version.
 * \ingroup kip
 *
 * \return Kernel ABI version.
 */
unsigned long l4sigma0_kip_kernel_abi_version(void);

EXTERN_C_END

/**
 * \brief Cycle through kernel features given in the KIP.
 * \ingroup kip
 *
 * Cycles through all KIP kernel feature strings. s must be a character
 * pointer (char *) initialized with l4sigma0_kip_version_string().
 */
#define l4sigma0_kip_for_each_feature(s)				\
		for (s += strlen(s) + 1; *s; s += strlen(s) + 1)

#endif /* ! __L4_SIGMA0_H */

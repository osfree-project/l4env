/*****************************************************************************/
#ifndef __L4UTIL__INCLUDE__MEMDESC_H__
#define __L4UTIL__INCLUDE__MEMDESC_H__

#include <l4/sys/linkage.h>
#include <l4/sys/types.h>

/** \defgroup memdesc Memory descriptor convenience functions */

EXTERN_C_BEGIN
/**
 * \brief Return end of virtual memory.
 * \ingroup memdesc
 *
 * \return 0 if memory descriptor could not be found,
 *         last address of address space otherwise
 */
L4_CV l4_addr_t l4util_memdesc_vm_high(void);

EXTERN_C_END

#endif /* ! __L4UTIL__INCLUDE__MEMDESC_H__ */

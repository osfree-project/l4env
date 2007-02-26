/*****************************************************************************/
#ifndef __L4UTIL__INCLUDE__MEMDESC_H__
#define __L4UTIL__INCLUDE__MEMDESC_H__

#include <l4/sys/types.h>

/* Obey to X.2 style?  */

/** \defgroup memdesc Memory descriptor convenience functions */

/**
 * \brief Return end of virtual memory.
 * \ingroup memdesc
 *
 * \return 0 if memory descriptor could not be found,
 *         last address of address space otherwise
 */
l4_addr_t l4util_memdesc_vm_high(void);

#endif /* ! __L4UTIL__INCLUDE__MEMDESC_H__ */

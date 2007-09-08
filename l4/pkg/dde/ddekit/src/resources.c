#include <l4/dde/ddekit/resources.h>

#include <l4/generic_io/libio.h>

#include "config.h"

int ddekit_request_dma(int nr) {
	return l4io_request_dma(nr);
}

int ddekit_release_dma(int nr) {
	return l4io_release_dma(nr);
}

/** Request an IO region
 *
 * \return 0 	success
 * \return -1   error
 */
int ddekit_request_io(ddekit_addr_t start, ddekit_addr_t count) {
	return l4io_request_region(start, count);
}

/** Release an IO region.
 *
 * \return 0 	success
 * \return <0   error
 */
int ddekit_release_io(ddekit_addr_t start, ddekit_addr_t count) {
	return l4io_release_region(start, count);
}

/** Request a memory region.
 *
 * \return vaddr virtual address of memory region
 * \return 0	 success
 * \return -1	 error
 */
int ddekit_request_mem(ddekit_addr_t start, ddekit_addr_t count, ddekit_addr_t *vaddr) {
	ddekit_addr_t v;

	v = l4io_request_mem_region(start, count, 0);
	if (v) {
		*vaddr = v;
		return 0;
	} else
		return -1;
}

/** Release memory region.
 *
 * \return 0 success
 * \return <0 error
 */
int ddekit_release_mem(ddekit_addr_t start, ddekit_addr_t count) {
	return l4io_release_mem_region(start, count);
}

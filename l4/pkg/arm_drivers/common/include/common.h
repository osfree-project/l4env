#ifndef __ARM_DRIVERS__COMMON__INCLUDE__COMMON_H__
#define __ARM_DRIVERS__COMMON__INCLUDE__COMMON_H__

#include <l4/sys/types.h>
#include <l4/crtx/ctor.h>

int arm_driver_reserve_region(l4_addr_t addr, l4_size_t size);
l4_addr_t arm_driver_map_io_region(l4_addr_t phys, l4_size_t size);

#endif /* ! __ARM_DRIVERS__COMMON__INCLUDE__COMMON_H__ */

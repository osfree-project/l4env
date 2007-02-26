#ifndef __ARCH_AMD64_TYPES_H__
#define __ARCH_AMD64_TYPES_H__

#include <l4/sys/l4int.h>

typedef l4_uint8_t owner_t;		/* an owner is a task number < 256 */

#define O_MAX (255)
#define O_FREE (0)
#define O_RESERVED (1)

#endif /* ! __ARCH_AMD64_TYPES_H__ */

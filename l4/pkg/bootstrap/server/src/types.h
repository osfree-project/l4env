#ifndef __TYPES_H__
#define __TYPES_H__

#include "l4/sys/types.h"

typedef l4_uint8_t owner_t;		/* an owner is a task number < 256 */

#define O_MAX (255)
#define O_FREE (0)
#define O_RESERVED (1)

#define MODS_MAX 64
#define CMDLINE_MAX 1024
#define MOD_NAME_MAX 1024

typedef char __mb_mod_name_str[MOD_NAME_MAX];

#endif /* ! __TYPES_H__ */

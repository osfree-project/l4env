#ifndef TYPES_H
#define TYPES_H

#include <l4/sys/types.h>

typedef l4_uint8_t owner_t;		/* an owner is a task number < 256 */

#define O_MAX		(255)
#define O_FREE		(0)
#define O_RESERVED	(1)
#define O_DEBUG		(2)
#define O_USER		(3)

#define MODS_MAX	64
#define CMDLINE_MAX	1024
#define MOD_NAME_MAX	1024

typedef char mb_mod_name_t[MOD_NAME_MAX];

#endif

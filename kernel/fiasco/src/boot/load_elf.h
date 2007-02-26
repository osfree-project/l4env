#ifndef LOAD_ELF_H__
#define LOAD_ELF_H__ 1

#include "types.h"

typedef void (*Startup_func)(void);

Startup_func load_elf( const char *name, void *address, Mword *vma_start, Mword *vma_end );

#endif

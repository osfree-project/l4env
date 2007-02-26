#ifndef LOAD_ELF_H__
#define LOAD_ELF_H__ 1

#include "types.h"

typedef void (*Startup_func)(void);

l4_uint32_t load_elf(void *addr, l4_uint32_t *vma_start, l4_uint32_t *vma_end);

#endif

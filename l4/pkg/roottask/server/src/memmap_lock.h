#ifndef MEMMAP_LOCK_H
#define MEMMAP_LOCK_H

void enter_memmap_functions(l4_uint32_t me, l4_threadid_t locker);
void leave_memmap_functions(l4_uint32_t me, l4_threadid_t candidate);

#endif /* ! MEMMAP_LOCK_H */

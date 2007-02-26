INTERFACE:

#include "types.h"

class Mem_unit
{};


IMPLEMENTATION:

PUBLIC static inline void Mem_unit::tlb_flush () {}
PUBLIC static inline void Mem_unit::tlb_flush (Address) {}

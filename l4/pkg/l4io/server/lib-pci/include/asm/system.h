#include_next <asm/system.h>

#undef __save_flags
#undef __restore_flags
#undef __cli
#undef __sti

#define __save_flags(x)     x = 0
#define __restore_flags(x)  x = x
#define __cli()
#define __sti()

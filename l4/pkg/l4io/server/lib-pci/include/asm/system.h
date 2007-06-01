#include_next <asm/system.h>

#undef __save_flags
#undef __restore_flags
#undef __cli
#undef __sti

#undef local_irq_save
#undef local_irq_set
#undef local_irq_restore

#define __save_flags(x)     x = 0
#define __restore_flags(x)  ((void)x)
#define __cli()
#define __sti()

/* For spinlocks etc */
#define local_irq_save(x) x = 0
#define local_irq_set(x) x = 0
#define local_irq_restore(x) ((void)x)


#ifndef __L4UTIL__ARCH_ARCH__IRQ_H__
#define __L4UTIL__ARCH_ARCH__IRQ_H__

#include <l4/sys/kdebug.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

/** Disable all interrupts
 */
static inline
void
l4util_cli (void)
{
  l4_sys_cli();
}

/** Enable all interrupts
 */
static inline
void
l4util_sti (void)
{
  l4_sys_sti();
}

/*
 * !!!!!!!
 *  We probably need some primitive like in linux here which
 *    enable/disable interrupts on l4util_flags_restore
 *
 */

static inline
void
l4util_flags_save(l4_umword_t *flags)
{
  enter_kdebug("l4util_flags_save");
}

/** Restore processor flags. Can be used to restore the interrupt flag
 *  */
static inline
void
l4util_flags_restore(l4_umword_t *flags)
{
  enter_kdebug("l4util_flags_restore");
}

EXTERN_C_END

#endif /* ! __L4UTIL__ARCH_ARCH__IRQ_H__ */

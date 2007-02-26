#ifndef __L4UTIL__INCLUDE__ARCH_ARM__STACK_IMPL_H__
#define __L4UTIL__INCLUDE__ARCH_ARM__STACK_IMPL_H__

#ifndef _L4UTIL_STACK_H
#error Do not include stack_impl.h directly, use stack.h instead
#endif

L4_INLINE l4_addr_t l4util_stack_get_sp(void)
{
  register l4_addr_t sp asm ("sp");
  return sp;
}

#endif /* ! __L4UTIL__INCLUDE__ARCH_ARM__STACK_IMPL_H__ */

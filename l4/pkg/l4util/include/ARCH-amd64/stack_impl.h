#ifndef __L4UTIL__INCLUDE__ARCH_AMD64__STACK_IMPL_H__
#define __L4UTIL__INCLUDE__ARCH_AMD64__STACK_IMPL_H__

#ifndef _L4UTIL_STACK_H
#error Do not include stack_impl.h directly, use stack.h instead
#endif

L4_INLINE l4_addr_t l4util_stack_get_sp(void)
{
  l4_addr_t rsp;

  asm("movq   %%rsp, %0\n\t" : "=r" (rsp) : );
  return rsp;
}

#endif /* ! __L4UTIL__INCLUDE__ARCH_AMD64__STACK_IMPL_H__ */

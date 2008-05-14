#ifndef __L4__SYS__ARCH_AMD64__LINKAGE_H__
#define __L4__SYS__ARCH_AMD64__LINKAGE_H__

#ifdef __ASSEMBLY__

#ifndef ENTRY
#define ENTRY(name) \
  .globl name; \
  .p2align(2); \
  name:

#endif /* __ASSEMBLY__ */
#endif /* ! ENTRY */

#define FASTCALL(x)     x
#define fastcall

/**
 * Define calling convention.
 * \ingroup api_types_compiler
 * \hideinitializer
 */
#define L4_CV

#endif /* ! __L4__SYS__ARCH_AMD64__LINKAGE_H__ */

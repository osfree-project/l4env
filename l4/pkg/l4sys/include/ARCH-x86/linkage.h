#ifndef __L4__SYS__ARCH_X86__LINKAGE_H__
#define __L4__SYS__ARCH_X86__LINKAGE_H__

#ifdef __ASSEMBLY__

#ifndef ENTRY
#define ENTRY(name) \
  .globl name; \
  .p2align(2); \
  name:

#endif /* ! ENTRY */
#endif /* __ASSEMBLY__ */

#define FASTCALL(x)	x __attribute__((regparm(3)))
#define fastcall	__attribute__((regparm(3)))

/**
 * Define calling convention.
 * \ingroup api_types_compiler
 * \hideinitializer
 */
#define L4_CV		__attribute__((regparm(0)))

#endif /* ! __L4__SYS__ARCH_X86__LINKAGE_H__ */

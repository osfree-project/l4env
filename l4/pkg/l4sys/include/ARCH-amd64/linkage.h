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

#endif /* ! __L4__SYS__ARCH_AMD64__LINKAGE_H__ */

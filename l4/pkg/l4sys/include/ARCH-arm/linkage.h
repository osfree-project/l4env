#ifndef __L4__SYS__ARCH_ARM__LINKAGE_H__
#define __L4__SYS__ARCH_ARM__LINKAGE_H__

#if defined(__PIC__) \
    && ((__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 2))
# define PIC_SAVE_ASM    "str r10, [sp, #-4]!  \n\t"
# define PIC_RESTORE_ASM "ldr r10, [sp], #4    \n\t"
# define PIC_CLOBBER
#else
# define PIC_SAVE_ASM
# define PIC_RESTORE_ASM
# define PIC_CLOBBER ,"r10"
#endif

#ifdef __ASSEMBLY__
#ifndef ENTRY
#define ENTRY(name) \
  .globl name; \
  .p2align(2); \
  name:
#endif
#endif

#define FASTCALL(x)	x
#define fastcall

/**
 * Define calling convention.
 * \ingroup api_types_compiler
 * \hideinitializer
 */
#define L4_CV

#ifdef __PIC__
# define LONG_CALL
#else
# define LONG_CALL __attribute__((long_call))
#endif

#endif /* ! __L4__SYS__ARCH_ARM__LINKAGE_H__ */

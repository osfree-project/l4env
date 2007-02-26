#ifndef __L4__SYS__ARCH_ARM__LINKAGE_H__
#define __L4__SYS__ARCH_ARM__LINKAGE_H__

#ifdef __PIC__
# define PIC_SAVE_ASM    "str r10, [sp, #-4]!  \n\t"
# define PIC_RESTORE_ASM "ldr r10, [sp], #4    \n\t"
# define PIC_CLOBBER
#else
# define PIC_SAVE_ASM
# define PIC_RESTORE_ASM
# define PIC_CLOBBER ,"r10"
#endif


#ifdef __ASSEMBLER__

#define ENTRY(name) \
  .globl name; \
  .p2align(2); \
  name:

#endif /* __ASSEMBLER__ */

#define FASTCALL(x)	x
#define fastcall

#endif /* ! __L4__SYS__ARCH_ARM__LINKAGE_H__ */

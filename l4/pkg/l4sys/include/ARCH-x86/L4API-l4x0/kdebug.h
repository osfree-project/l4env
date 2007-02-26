/* 
 * $Id$
 *
 * kdebug.h provides some useful makros to acces the functionality 
 * of the kernel debugger
 */

#ifndef __L4_KDEBUG_H__ 
#define __L4_KDEBUG_H__ 

#include <l4/sys/compiler.h>

#define enter_kdebug(text) \
asm(\
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\""text "\"\n\t"\
    "1:			\n\t"\
    )

#define asm_enter_kdebug(text) \
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\""text "\"\n\t"\
    "1:			\n\t"

#define kd_display(text) \
asm(\
    "int	$3	\n\t"\
    "nop		\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\""text "\"\n\t"\
    "1:			\n\t"\
    )

#define ko(c) 					\
  asm(						\
      "int	$3	\n\t"			\
      "cmpb	%0,%%al	\n\t"			\
      : /* No output */				\
      : "N" (c)					\
      )

/*
 * prototypes
 */
L4_INLINE void fiasco_profile_start(void);
L4_INLINE void fiasco_profile_stop_and_dump(void);
L4_INLINE void fiasco_profile_stop(void);
L4_INLINE void fiasco_watchdog_enable(void);
L4_INLINE void fiasco_watchdog_disable(void);
L4_INLINE void fiasco_watchdog_takeover(void);
L4_INLINE void fiasco_watchdog_giveback(void);
L4_INLINE void fiasco_watchdog_touch(void);
L4_INLINE void outchar(char c);
L4_INLINE void outstring(char *text);
L4_INLINE void outhex32(int number);
L4_INLINE void outhex20(int number);
L4_INLINE void outhex16(int number);
L4_INLINE void outhex12(int number);
L4_INLINE void outhex8(int number);
L4_INLINE void outdec(int number);
L4_INLINE char kd_inchar(void);

L4_INLINE void outchar(char c)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$0,%%al	\n\t"
      : /* No output */
      : "a" (c)
      );
}

/* actually outstring is outcstring */
L4_INLINE void outstring(char *text)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$2,%%al \n\t"
      : /* No output */
      : "a" (text)
      );
}

L4_INLINE void outhex32(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$5,%%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void outhex20(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$6,%%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void outhex16(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$7, %%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void outhex12(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$8, %%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void outhex8(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$9, %%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void outdec(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$11, %%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE char kd_inchar(void)
{
  char c;
  asm volatile ("int $3; cmpb $13, %%al" : "=a" (c));
  return c;
}


L4_INLINE void 
fiasco_profile_start(void)
{
  asm("int $3; cmpb $24, %%al");
}

L4_INLINE void 
fiasco_profile_stop_and_dump(void)
{
  asm("int $3; cmpb $25, %%al");
}

L4_INLINE void 
fiasco_profile_stop(void)
{
  asm("int $3; cmpb $26, %%al");
}

L4_INLINE void 
fiasco_watchdog_enable(void)
{
  asm("int $3; cmpb $31, %%al" : : "c" (1));
}

L4_INLINE void 
fiasco_watchdog_disable(void)
{
  asm("int $3; cmpb $31, %%al" : : "c" (2));
}

L4_INLINE void 
fiasco_watchdog_takeover(void)
{
  asm("int $3; cmpb $31, %%al" : : "c" (3));
}

L4_INLINE void 
fiasco_watchdog_giveback(void)
{
  asm("int $3; cmpb $31, %%al" : : "c" (4));
}

L4_INLINE void 
fiasco_watchdog_touch(void)
{
  asm("int $3; cmpb $31, %%al" : : "c" (5));
}

#endif /* __L4_KDEBUG_H__ */

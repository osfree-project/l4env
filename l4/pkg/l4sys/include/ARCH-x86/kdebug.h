/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-x86/kdebug.h
 * \brief   Kernel debugger macros
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_KDEBUG_H__
#define __L4_KDEBUG_H__

#include <l4/sys/compiler.h>

/**
 * Enter L4 kernel debugger
 * \ingroup api_calls_kdebug
 * \hideinitializer
 *
 * \param   text         Text to be shown at kernel debugger prompt
 */
#define enter_kdebug(text) \
asm(\
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\""text "\"\n\t"\
    "1:			\n\t"\
    )

/**
 * Enter L4 kernel debugger (plain assembler version)
 * \ingroup api_calls_kdebug
 * \hideinitializer
 *
 * \param   text         Text to be shown at kernel debugger prompt
 */
#define asm_enter_kdebug(text) \
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\""text "\"\n\t"\
    "1:			\n\t"

/**
 * Show message with L4 kernel debugger, but do not enter debugger
 * \ingroup api_calls_kdebug
 * \hideinitializer
 *
 * \param   text         Text to be shown
 */
#define kd_display(text) \
asm(\
    "int	$3	\n\t"\
    "nop		\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\""text "\"\n\t"\
    "1:			\n\t"\
    )

/**
 * Output character with L4 kernel debugger
 * \ingroup api_calls_kdebug
 * \hideinitializer
 *
 * \param   c            Character to be shown
 */
#define ko(c) 					\
  asm(						\
      "int	$3	\n\t"			\
      "cmpb	%0,%%al	\n\t"			\
      : /* No output */				\
      : "N" (c)					\
      )

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

/**
 * Print character
 * \ingroup api_calls_kdebug
 *
 * \param   c            Character
 */
L4_INLINE void
outchar(char c);

/**
 * Print character string
 * \ingroup api_calls_kdebug
 *
 * \param   text         Character string
 */
L4_INLINE void
outstring(const char * text);

/**
 * Print character string
 * \ingroup api_calls_kdebug
 *
 * \param   text         Character string
 * \param   len          Number of charachters
 */
L4_INLINE void
outnstring(char const *text, unsigned len);

/**
 * Print 32 bit number (hexadecimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       32 bit number
 */
L4_INLINE void
outhex32(int number);

/**
 * Print 20 bit number (hexadecimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       20 bit number
 */
L4_INLINE void
outhex20(int number);

/**
 * Print 16 bit number (hexadecimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       16 bit number
 */
L4_INLINE void
outhex16(int number);

/**
 * Print 12 bit number (hexadecimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       12 bit number
 */
L4_INLINE void
outhex12(int number);

/**
 * Print 8 bit number (hexadecimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       8 bit number
 */
L4_INLINE void
outhex8(int number);

/**
 * Print number (decimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       Number
 */
L4_INLINE void
outdec(int number);

/**
 * Read character from console, non blocking
 * \ingroup api_calls_kdebug
 *
 * \return Input character, -1 if no character to read
 */
L4_INLINE char
l4kd_inchar(void);

/**
 * Start profiling
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_profile_start(void);

/**
 * Stop profiling and dump result to console
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_profile_stop_and_dump(void);

/**
 * Stop profiling
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_profile_stop(void);

/**
 * Enable Fiasco watchdog
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_watchdog_enable(void);

/**
 * Disable Fiasco watchdog
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_watchdog_disable(void);

/**
 * Disable automatic resetting of watchdog. User is responsible to call
 * \c fiasco_watchdog_touch from time to time to ensure that the watchdog
 * does not trigger.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_watchdog_takeover(void);

/**
 * Reenable automatic resetting of watchdog.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_watchdog_giveback(void);

/**
 * Reset watchdog from userland. This function \b must be called from time
 * to time to prevent the watchdog from triggering if the watchdog is
 * activated and if \c fiasco_watchdog_takeover was performed.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_watchdog_touch(void);


/*****************************************************************************
 *** Implementation
 *****************************************************************************/

L4_INLINE void
outchar(char c)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$0,%%al	\n\t"
      : /* No output */
      : "a" (c)
      );
}

/* actually outstring is outcstring */
L4_INLINE void
outstring(const char *text)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$2,%%al \n\t"
      : /* No output */
      : "a" (text), "m" (*text)
      );
}

/* actually outstring is outcstring */
L4_INLINE void
outnstring(char const *text, unsigned len)
{
  asm("pushl    %%ebx        \n\t"
      "movl     %%ecx, %%ebx \n\t"
      "int	$3	     \n\t"
      "cmpb	$1,%%al      \n\t"
      "popl     %%ebx        \n\t"
      : /* No output */
      : "a" (text), "c"(len), "m" (*text)
      );
}

L4_INLINE void
outhex32(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$5,%%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void
outhex20(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$6,%%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void
outhex16(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$7, %%al\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void
outhex12(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$8, %%al\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void
outhex8(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$9, %%al\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void
outdec(int number)
{
  asm(
      "int	$3	\n\t"
      "cmpb	$11, %%al\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE char
l4kd_inchar(void)
{
  char c;
  asm volatile ("int $3; cmpb $13, %%al" : "=a" (c));
  return c;
}

L4_INLINE void
fiasco_profile_start(void)
{
  asm("int $3; cmpb $24, %al");
}

L4_INLINE void
fiasco_profile_stop_and_dump(void)
{
  asm("int $3; cmpb $25, %al");
}

L4_INLINE void
fiasco_profile_stop(void)
{
  asm("int $3; cmpb $26, %al");
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

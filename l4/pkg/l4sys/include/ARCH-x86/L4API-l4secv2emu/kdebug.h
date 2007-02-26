#ifndef __L4_KDEBUG_H__
#define __L4_KDEBUG_H__

#include <l4/sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define enter_kdebug(text) \
asm(\
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\""text "\"\n\t"\
    "1:			\n\t"\
    )

void
outnstring(char const *text, unsigned len);

/**
 * Print character
 * \ingroup api_calls_kdebug
 *
 * \param   c            Character
 */
void
outchar(char c);

/**
 * Print character string
 * \ingroup api_calls_kdebug
 *
 * \param   text         Character string
 */
void
outstring(const char * text);

/**
 * Print character string
 * \ingroup api_calls_kdebug
 *
 * \param   text         Character string
 * \param   len          Number of charachters
 */
void
outnstring(char const *text, unsigned len);

/**
 * Print 32 bit number (hexadecimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       32 bit number
 */
void
outhex32(int number);

/**
 * Print 20 bit number (hexadecimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       20 bit number
 */
void
outhex20(int number);

/**
 * Print 16 bit number (hexadecimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       16 bit number
 */
void
outhex16(int number);

/**
 * Print 12 bit number (hexadecimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       12 bit number
 */
void
outhex12(int number);

/**
 * Print 8 bit number (hexadecimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       8 bit number
 */
void
outhex8(int number);

/**
 * Print number (decimal)
 * \ingroup api_calls_kdebug
 *
 * \param   number       Number
 */
void
outdec(int number);


void
fiasco_register_symbols(l4_taskid_t tid, l4_addr_t addr, l4_size_t size);

void
fiasco_register_lines(l4_taskid_t tid, l4_addr_t addr, l4_size_t size);

void
fiasco_register_thread_name(l4_threadid_t tid, const char *name);


#ifdef __cplusplus
}
#endif

#endif /* __L4_KDEBUG_H__ */

#ifndef _ddekit_panic_h
#define _ddekit_panic_h

/** \defgroup DDEKit_util */

/** Panic - print error message and enter the kernel debugger.
 * \ingroup DDEKit_util
 */
void ddekit_panic(char *fmt, ...) __attribute__((noreturn));

/** Print a debug message.
 * \ingroup DDEKit_util
 */
void ddekit_debug(char *fmt, ...);

#endif

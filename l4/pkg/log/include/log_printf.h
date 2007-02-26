/*!
 * \file   log/include/log_printf.h
 * \brief  prototypes for loglib printf implementations
 *
 * \date   02/19/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __LOG_INCLUDE_LOG_PRINTF_H_
#define __LOG_INCLUDE_LOG_PRINTF_H_

#include <stdarg.h>

/* Ugly workaround for the different stdarg versions */
#ifdef __L4__
typedef oskit_va_list LOG_va_list;
#else
typedef va_list LOG_va_list;
#endif

extern int LOG_vprintf(const char*,LOG_va_list);
extern int LOG_printf(const char*, ...);
extern int LOG_putchar(int);
extern int LOG_puts(const char*);  // does append '\n'
extern int LOG_fputs(const char*); // does not append '\n'
extern int LOG_putstring(const char*);
extern int LOG_sprintf(char*, const char*, ...);
extern int LOG_snprintf(char*, unsigned size, const char*, ...);
extern int LOG_vsprintf(char*, const char*, LOG_va_list);
extern int LOG_vsnprintf(char*, unsigned size, const char*, LOG_va_list);

extern void LOG_printf_flush(void);

#endif

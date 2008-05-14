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
#include <l4/sys/linkage.h>

typedef va_list LOG_va_list;

L4_CV int LOG_vprintf(const char*,LOG_va_list);
L4_CV int LOG_printf(const char*, ...) 
  __attribute__((format (printf, 1, 2)));
L4_CV int LOG_putchar(int);
L4_CV int LOG_puts(const char*);  // does append '\n'
L4_CV int LOG_fputs(const char*); // does not append '\n'
L4_CV int LOG_putstring(const char*);
L4_CV int LOG_sprintf(char*, const char*, ...) 
  __attribute__((format (printf, 2, 3)));
L4_CV int LOG_snprintf(char*, unsigned size, const char*, ...)
  __attribute__((format (printf, 3, 4)));
L4_CV int LOG_vsprintf(char*, const char*, LOG_va_list);
L4_CV int LOG_vsnprintf(char*, unsigned size, const char*, LOG_va_list);

L4_CV void LOG_printf_flush(void);

#endif

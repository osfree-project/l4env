/*!
 * \file   log/lib/src/sprintf.c
 * \brief  mapping of sprintf and friends
 *
 * \date   02/27/2003
 * \author Jork Loeser <jork_loeser@inf.tu-dresden.de>
 *
 * This file implements: sprintf, vsprintf, snrprintf, vsnprintf
 * 
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/log/l4log.h>
#include <l4/log/log_printf.h>
#include "internal.h"

int sprintf(char *s, const char*format, ...){
    va_list list;
    int err;

    va_start(list, format);
    err=LOG_vsprintf(s, format, list);
    va_end(args);
    return err;
}
int snprintf(char *s, oskit_size_t size, const char*format, ...){
    va_list list;
    int err;

    va_start(list, format);
    err=LOG_vsnprintf(s, size, format, list);
    va_end(args);
    return err;
}
int vsprintf(char *s, const char*format, oskit_va_list list){
    return LOG_vsprintf(s, format, list);
}
int vsnprintf(char *s, oskit_size_t size, const char*format,
              oskit_va_list  list){
    return LOG_vsnprintf(s, size, format, list);
}

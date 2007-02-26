/*!
 * \file   log/lib/src/printf.c
 * \brief  mapping of printf and friends
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork_loeser@inf.tu-dresden.de>
 *
 * This file implements: printf, fprintf, vprintf, vfprintf, putchar,
 *			 puts, fputs
 * 
 */

#include <l4/log/l4log.h>
#include <l4/log/log_printf.h>
#include "internal.h"

int vprintf(const char*format, oskit_va_list list){
    return LOG_vprintf(format, list);
}

int vfprintf(FILE *stream, const char*format, oskit_va_list list){
    return LOG_vprintf(format, list);
}

int printf(const char *format, ...){
    va_list list;
    int err;

    va_start(list, format);
    err=LOG_vprintf(format, list);
    va_end(args);
    return err;
}

int fprintf(FILE *__stream, const char *format, ...){
    va_list list;
    int err;

    va_start(list, format);
    err=LOG_vprintf(format, list);
    va_end(args);
    return err;
}

#ifndef putc
int putc(int c, FILE*stream){
    LOG_putchar(c);
    return c;
}
#endif

int putchar(int c){
    LOG_putchar(c);
    return c;
}

int fputc(int c, FILE*stream){
    LOG_putchar(c);
    return c;
}

int puts(const char*s){
    return LOG_puts(s);
}

int fputs(const char*s, FILE*stream){
    return LOG_fputs(s);
}


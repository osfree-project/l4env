/**
 * \file   libc_backends_l4env/lib/syslog/syslog.c
 * \brief  syslog implementation using LOG server
 *
 * \date   2004-06-18
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* we have to include l4log at first and
 * undefine LOG_DEBUG because syslog also
 * defines it as a priority
*/
#include <l4/log/l4log.h>
#undef LOG_DEBUG

/*** GENERAL INCLUDES ***/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define SYSLOG_NAMES
#include <syslog.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/sys/syscalls.h>
#include <l4/thread/thread.h>
#include <l4/util/l4_macros.h>

#define LOG_TAG_SIZE 9
#define BUF_SIZE 2048

#ifdef USE_DIETLIBC
/* taken from linux syslog and dietlibc arranged */
CODE prioritynames[] =
{
    { "Emerg", LOG_EMERG },
    { "Alert", LOG_ALERT },
    { "Crit", LOG_CRIT },
    { "Error", LOG_ERR },
    { "Warn", LOG_WARNING },
    { "Notice", LOG_NOTICE },
    { "Info", LOG_INFO },
    { "Debug", LOG_DEBUG },
    { NULL, -1 }
};
#endif

static int LogStat;                 /* status bits, set by openlog() */
static int LogFacility = LOG_USER;  /* default facility code */
static volatile int LogMask = 0xff; /* mask of priorities to be logged */

static char old_LOG_tag[LOG_TAG_SIZE];

/* currently do nothing */
void closelog(void)
{
    /* restore old name */
    memcpy(LOG_tag, old_LOG_tag, LOG_TAG_SIZE);
}

void openlog(const char *ident, int option, int facility)
{
    /* save old name */
    memcpy(old_LOG_tag, LOG_tag, LOG_TAG_SIZE);

    strncpy(LOG_tag,ident,LOG_TAG_SIZE);
    LOG_tag[LOG_TAG_SIZE-1] = 0;
    LogStat = option;

    if (facility && ((facility & ~LOG_FACMASK) == 0))
    {
        LogFacility = facility;
    }
}

int setlogmask(int mask)
{
    int old = LogMask;
    if (mask)
    {
        LogMask = mask;
    }
    return old;
}

void vsyslog(int priority, const char *format, va_list arg_ptr)
{
    int len;
    int headerlen = 0;
    char buffer[BUF_SIZE];

    /* check for invalid priority/facility bits */
    if (priority & ~(LOG_PRIMASK|LOG_FACMASK))
    {
        LOG_Error("unknown facility/priority: 0x%x", priority);
        priority &= LOG_PRIMASK|LOG_FACMASK;
    }

    /* check priority against setlogmask */
    if ((LOG_MASK(LOG_PRI(priority)) && LogMask) == 0)
    {
        return;
    }

    /* set default facility if none specified. */
    if ((priority & LOG_FACMASK) == 0)
    {
        priority |= LogFacility;
    }

    /* include task and thread id */
    if (LogStat & LOG_PID)
    {
        headerlen = snprintf(buffer, 32, "["l4util_idfmt"] ",
                             l4util_idstr(l4_myself()));
        if (headerlen > 31)
            headerlen = 31;
    }

    /* include priority name */
    if (LOG_PRI(priority))
    {
        int len;
	len = snprintf(buffer + headerlen, 130, "%s: ",
                       prioritynames[LOG_PRI(priority)].c_name);
        if (len > 129)
            len = 129;
        headerlen += len;
    }

    vsnprintf(buffer+headerlen, BUF_SIZE - headerlen, format, arg_ptr);

    len = strlen(buffer);

    /* if last character is not newline add such one */
    if (buffer[len-1] != '\n')
    {
        buffer[len] = '\n';
        buffer[len+1] = 0;
    }

    LOG_printf("%s",buffer);
}

void syslog(int priority, const char *format, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, format);
    vsyslog(priority, format, arg_ptr);
    va_end(arg_ptr);
}

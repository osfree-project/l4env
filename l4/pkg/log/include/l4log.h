/*!
 * \file   log/include/l4log.h
 * \brief  Logging facility - Macros and function declaration
 *
 * \date   03/14/2001
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __LOG_INCLUDE_L4LOG_H_
#define __LOG_INCLUDE_L4LOG_H_

#if defined L4API_l4v2 || defined L4API_l4x0
#include <l4/log/server.h>
#include <l4/sys/ktrace.h>
#endif
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

#include <l4/log/log_printf.h>

/*!\brief Symbol defining the logtag.
 *
 * This is a weak symbol which can be overwritten to define the logtag.  The
 * intention is that you can set the logtag on compile-time, if you cannot
 * guarantee to call LOG_init() prior to first output for whatever reasons.
 *
 * Use one of the two methods to define the logtag, either by defining this
 * symbol or by calling LOG_init().
 *
 * \note The length of the symbol must be 9 characters in length, and it
 *	 must be 0-terminated.
 * \see  LOG_init()
 */
extern char LOG_tag[9];

#if 0
/*!\brief Initialize the lib.
 *
 * \param tag	the logtag
 *
 * This function sets the logtag, which is writted at the beginning
 * of each line.
 *
 * \see  LOG_tag.
 */
extern void LOG_init(const char *tag);
#else
#define LOG_init(arg) LOG_init_is_deprecated=0
/* LOG_setup_tag is called by setup code, don't call this from your program. */
extern void LOG_setup_tag(void);
#endif


/*!\brief variable containing the text output function
 *
 * This variable holds the function to output the formated string. You
 * can re-initialize it to use your own function. In non-server-mode this
 * points to a putchar-loop, in server-mode a string-ipc-function will
 * be used.
 */
extern void (*LOG_outstring)(const char*log_message);

/*!\brief Log-output function printing to the fiasco tracebuffer
 *
 * This specific version of a text output function uses the fiasco
 * tracebuffer as output channel.
 */
extern void LOG_outstring_fiasco_tbuf(const char*log_message);

/*!\brief flush all buffered data.
 *
 * This function ensures that all data logged so far is actually printed.
 * If used with the logserver, the logserver flushs all its buffered data.
 */
extern void LOG_flush(void);

/*!\brief an allways-defined DEBUG flag
 */
#ifdef DEBUG
#define LOG_DEBUG 1
#else
#define LOG_DEBUG 0
#endif

#if 1
/* format_check: just for the compiler to check the format & args */
extern void LOG_format_check(const char*format,...)
  __attribute__((format(printf,1,2)));

extern void LOG_log(const char*function, const char*format,...);
extern void LOG_logl(const char*file, int line, const char*function,
		     const char*format,...);
extern void LOG_logL(const char*file, int line, const char*function,
		     const char*format,...);

#define STRINGIFY_HELPER(nr)		#nr
#define LINE_PRESCAN_SUBST(ln)		STRINGIFY_HELPER(ln)
#define __LINE_STR__			LINE_PRESCAN_SUBST(__LINE__)

#define LOG(a...) do {				\
  if(0)LOG_format_check(a);			\
  LOG_log(__FUNCTION__, a);			\
} while(0)

#define LOGl(a...) do {				\
  if(0)LOG_format_check(a);			\
  LOG_logl(__FILE__,__LINE__,__FUNCTION__, a);	\
} while(0)

#define LOGL(a...) do {				\
  if(0)LOG_format_check(a);			\
  LOG_logL(__FILE__,__LINE__,__FUNCTION__, a);	\
} while(0)

#define LOG_Enter(a...) do {			\
  if(0)LOG_format_check("called "a);		\
  LOG_log(__FUNCTION__, "called "a);		\
} while(0)

#define LOGk(a...) do {				\
  if(0)LOG_format_check(a);			\
  LOG_logk(a);					\
} while(0)

#define LOGd_Enter(doit, msg...) if(doit) LOG_Enter(msg)
#define LOGd(doit, msg...) if(doit) LOG(msg)
#define LOGdl(doit,msg...) if(doit) LOGl(msg)
#define LOGdL(doit,msg...) if(doit) LOGL(msg)
#define LOGdk(doit,msg...) if(doit) LOGk(msg)
#define LOG_Error(msg...) LOGL("Error: " msg)

#else
#define LOG(a...)
#define LOGl(a...)
#define LOGL(a...)
#define LOGk(a...)
#define LOG_Enter(a...)
#define LOGd_Enter(doit, msg...)
#define LOGd(doit, msg...)
#define LOGdl(doit,msg...)
#define LOGdL(doit,msg...)
#define LOGdk(doit,msg...)
#define LOG_Error(msg...)
#endif

#if defined L4API_l4v2 || defined L4API_l4x0
#define LOG_logk(format...) do{				\
    char buf[50];					\
    LOG_snprintf(buf,sizeof(buf),format);		\
    fiasco_tbuf_log(buf);				\
}while(0)
#endif /* defined L4API_l4v2 || defined L4API_l4x0 */

EXTERN_C_END
#endif

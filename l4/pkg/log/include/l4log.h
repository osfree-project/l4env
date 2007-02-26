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
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

/*!\brief variable containing the text output function
 *
 * This variable holds the function to output the formated string. You
 * can re-initialize it to use your own function. In non-server-mode this
 * points to a putchar-loop, in server-mode a string-ipc-function will
 * be used.
 */
extern void (*LOG_outstring)(const char*log_message);

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

extern void LOG_log(const char*function, const char*format,...);
extern void LOG_logl(const char*file, int line, const char*function,
		     const char*format,...);
extern void LOG_logL(const char*file, int line, const char*function,
		     const char*format,...);

#define STRINGIFY_HELPER(nr)		#nr
#define LINE_PRESCAN_SUBST(ln)		STRINGIFY_HELPER(ln)
#define __LINE_STR__			LINE_PRESCAN_SUBST(__LINE__)

#define LOG(a...)	\
  LOG_log(__FUNCTION__, a)

#define LOGl(a...)	\
  LOG_logl(__FILE__,__LINE__,__FUNCTION__, a)

#define LOGL(a...)	\
  LOG_logL(__FILE__,__LINE__,__FUNCTION__, a)

#define LOG_Enter(a...)	\
  LOG_log(__FUNCTION__, "called "a)

#define LOGd_Enter(doit, msg...) if(doit) LOG_Enter(msg)
#define LOGd(doit, msg...) if(doit) LOG(msg)
#define LOGdl(doit,msg...) if(doit) LOGl(msg)
#define LOGdL(doit,msg...) if(doit) LOGL(msg)
#define LOG_Error(msg...) LOGL("Error: " msg)

#else
#define LOG(a...)
#define LOGl(a...)
#define LOGL(a...)
#define LOG_Enter(a...)
#define LOGd_Enter(doit, msg...)
#define LOGd(doit, msg...)
#define LOGdl(doit,msg...)
#define LOGdL(doit,msg...)
#define LOG_Error(msg...)
#endif

#ifdef __cplusplus
}
#endif
#endif

#ifndef _ASSERT_H
#define _ASSERT_H

#include <cdefs.h>

__BEGIN_DECLS
/* This prints an "Assertion failed" message and aborts.  */
void __assert_fail (const char *__assertion, const char *__file,
			   unsigned int __line, const char *__function)
     __attribute__ ((__noreturn__));

__END_DECLS

#ifdef __PRETTY_FUNCTION__
#define __ASSERT_FUNCTION __PRETTY_FUNCTION__
#else
#  if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#   define __ASSERT_FUNCTION	__func__
#  else
#   define __ASSERT_FUNCTION	((const char *) 0)
#  endif
#endif

#undef assert
#ifdef NDEBUG
#define assert(expr)
#define check(expr) (void)(expr)
#else
# define assert(expr)							      \
  ((void) ((expr) ? 0 :							      \
	   (__assert_fail (#expr,				      \
			   __FILE__, __LINE__, __ASSERT_FUNCTION), 0)))
# define check(expr) assert(expr)
#endif



#endif

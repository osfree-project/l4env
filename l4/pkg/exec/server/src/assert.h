#ifndef __L4_EXEC_SERVER_ASSERT_H
#define __L4_EXEC_SERVER_ASSERT_H

#include <l4/sys/kdebug.h>
#include <stdio.h>

#define Panic(format, args...) do {		\
    printf(format "\n" , ## args);		\
    enter_kdebug("Panic");			\
} while (0)

#define Error(format, args...) do {		\
    printf("In %s:\n"format "\n", __PRETTY_FUNCTION__ , ## args); \
    enter_kdebug("Error");			\
} while (0)

#ifdef NDEBUG

#define Assert(expr) ((void)0)

#else

#define Assert(expr) do {			\
    if (!(expr)) { 				\
	printf(#expr "\n");			\
	Panic("[%s:%d]: Assertion failed",	\
	    __FILE__, __LINE__);		\
    }						\
} while (0)

#endif /* NDEBUG */

#endif /* __L4_EXEC_SERVER_ASSERT_H */


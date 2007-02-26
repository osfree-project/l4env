INTERFACE:

#include "globalconfig.h"

#ifdef CONFIG_INLINE
#  define FIASCO_INLINE inline
#else
#  define FIASCO_INLINE static
#endif

#if (__GNUC__>=3)
#  define BUILTIN_EXPECT(exp,c)	__builtin_expect((exp),(c))
#  define EXPECT_TRUE(exp)	__builtin_expect((exp),true)
#  define EXPECT_FALSE(exp)	__builtin_expect((exp),false)
#else
#  define BUILTIN_EXPECT(exp,c)	(exp)
#  define EXPECT_TRUE(exp)	(exp)
#  define EXPECT_FALSE(exp)	(exp)
#endif

IMPLEMENTATION:
//-

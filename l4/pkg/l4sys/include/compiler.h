/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/include/compiler.h
 * \brief   L4 compiler defines
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef __L4_COMPILER_H__ 
#define __L4_COMPILER_H__ 

#if !defined(__ASSEMBLY__) && !defined(__ASSEMBLER__)

#ifndef __GNUC__
#error "The libl4sys library must be used with Gcc."
#endif

/**
 * L4 Inline function attribute
 * \ingroup api_types_compiler
 * \hideinitializer
 */
#ifndef __cplusplus
#  ifdef __OPTIMIZE__
#    ifdef STATIC_L4_INLINE
#      define L4_INLINE static __inline__
#    else
#      define L4_INLINE extern __inline__
#    endif
#  else /* ! __OPTIMIZE__ */
#    define L4_INLINE static
#  endif /* ! __OPTIMIZE__ */
#else /* __cplusplus */
#  define L4_INLINE inline
#endif  /* __cplusplus */

/**
 * Start section with C types and functions
 * \def     EXTERN_C_BEGIN
 * \ingroup api_types_compiler
 */
/**
 * End section with C types and functions
 * \def     EXTERN_C_END
 * \ingroup api_types_compiler
 */
#ifndef __cplusplus
#  define EXTERN_C_BEGIN
#  define EXTERN_C_END
#else /* __cplusplus */
#  define EXTERN_C_BEGIN extern "C" {
#  define EXTERN_C_END }
#endif /* __cplusplus */

/**
 * Noreturn function attribute.
 * \ingroup api_types_compiler
 * \hideinitializer
 */
#define L4_NORETURN __attribute__((noreturn))

#endif /* !__ASSEMBLY__ */

#include <l4/sys/linkage.h>

#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#define __builtin_expect(x, expected_value) (x)
#endif

#define EXPECT_TRUE(x)	__builtin_expect((x),1)
#define EXPECT_FALSE(x)	__builtin_expect((x),0)

#endif /* !__L4_COMPILER_H__ */

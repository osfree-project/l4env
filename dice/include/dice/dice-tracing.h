#ifndef __DICE_DICE_TRACING_H__
#define __DICE_DICE_TRACING_H__

/* PARAMS is a macro used to wrap function prototypes, so that
 *         compilers that don't understand ANSI C prototypes still work,
 *         and ANSI C compilers can issue warnings about type
 *         mismatches. */
#undef PARAMS
#if defined (__STDC__) || defined (_AIX) \
    || (defined (__mips) && defined (_SYSTYPE_SVR4)) \
|| defined(WIN32) || defined(__cplusplus)
# define PARAMS(protos) protos
#else
# define PARAMS(protos) ()
#endif

/* The tracing plugin only works because it creates an instance of the tracing
 * class. Not using C++ to build that plugin is probably impossible.
 */
#ifdef __cplusplus
class CTrace;

extern "C" {

/**
 * \brief initialize the tracing library
 * \param argc the number of Dice arguments
 * \param argv the actual Dice arguments
 */
extern
void 
dice_tracing_init PARAMS((int argc, char *argv[]));

/**
 * \brief create a new tracing class
 * \return a reference to a CBETrace derived class
 */
extern
CTrace* 
dice_tracing_new_class PARAMS((void));

}
#else
#warning The tracing module only works as C++ lib.
#endif

#endif /* __DICE_DICE_TRACING_H__ */

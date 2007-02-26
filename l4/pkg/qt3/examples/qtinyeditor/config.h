/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if you have the CoreAudio API */
#undef HAVE_COREAUDIO

/* Define to 1 if you have the <crt_externs.h> header file. */
#undef HAVE_CRT_EXTERNS_H

/* Defines if your system has the crypt function */
#undef HAVE_CRYPT

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define if you have libjpeg */
#undef HAVE_LIBJPEG

/* Define if you have libpng */
#undef HAVE_LIBPNG

/* Define if you have a working libpthread (will enable threaded code) */
#undef HAVE_LIBPTHREAD

/* Define if you have libz */
#undef HAVE_LIBZ

/* Define to 1 if you have the <memory.h> header file. */
#undef HAVE_MEMORY_H

/* Define if your system needs _NSGetEnviron to set up the environment */
#undef HAVE_NSGETENVIRON

/* Define to 1 if you have the <pthread/linuxthreads/pthread.h> header file.
   */
#undef HAVE_PTHREAD_LINUXTHREADS_PTHREAD_H

/* Define if you have the res_init function */
#undef HAVE_RES_INIT

/* Define to 1 if you have the `snprintf' function. */
#undef HAVE_SNPRINTF

/* Define to 1 if you have the <stdint.h> header file. */
#undef HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#undef HAVE_STDLIB_H

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#undef HAVE_STRING_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#undef HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/types.h> header file. */
#undef HAVE_SYS_TYPES_H

/* C++ compiler supports template repository */
#undef HAVE_TEMPLATE_REPOSITORY

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define to 1 if you have the `vsnprintf' function. */
#undef HAVE_VSNPRINTF

/* Define if you want Xinerama support */
#undef HAVE_XINERAMA

/* Suffix for lib directories */
#undef KDELIBSUFF

/* Name of package */
#undef PACKAGE

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* The size of a `char', as computed by sizeof. */
#undef SIZEOF_CHAR

/* The size of a `char *', as computed by sizeof. */
#undef SIZEOF_CHAR_P

/* The size of a `int', as computed by sizeof. */
#undef SIZEOF_INT

/* The size of a `long', as computed by sizeof. */
#undef SIZEOF_LONG

/* Define to 1 if you have the ANSI C header files. */
#undef STDC_HEADERS

/* Version number of package */
#define VERSION "0.1.2"

/*
 * jpeg.h needs HAVE_BOOLEAN, when the system uses boolean in system
 * headers and I'm too lazy to write a configure test as long as only
 * unixware is related
 */
#ifdef _UNIXWARE
#define HAVE_BOOLEAN
#endif



/*
 * AIX defines FD_SET in terms of bzero, but fails to include <strings.h>
 * that defines bzero.
 */

#if defined(_AIX)
#include <strings.h>
#endif



/*
 * On HP-UX, the declaration of vsnprintf() is needed every time !
 */

#if !defined(HAVE_VSNPRINTF) || defined(hpux)
#if __STDC__
#include <stdarg.h>
#include <stdlib.h>
#else
#include <varargs.h>
#endif
#ifdef __cplusplus
extern "C"
#endif
int vsnprintf(char *str, size_t n, char const *fmt, va_list ap);
#ifdef __cplusplus
extern "C"
#endif
int snprintf(char *str, size_t n, char const *fmt, ...);
#endif



#if defined(__SVR4) && !defined(__svr4__)
#define __svr4__ 1
#endif


/* Compatibility define */
#undef ksize_t

/* Define the real type of socklen_t */
#undef socklen_t

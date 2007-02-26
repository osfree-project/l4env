/*
 * Automatically generated C config: don't edit
 */
#if !defined __FEATURES_H && !defined __need_uClibc_config_h
#error Never include <bits/uClibc_config.h> directly; use <features.h> instead
#endif
#define AUTOCONF_INCLUDED

/*
 * Version Number
 */
#define __UCLIBC_MAJOR__ 0
#define __UCLIBC_MINOR__ 9
#define __UCLIBC_SUBLEVEL__ 28
#undef __TARGET_alpha__
#undef __TARGET_arm__
#undef __TARGET_bfin__
#undef __TARGET_cris__
#undef __TARGET_e1__
#undef __TARGET_frv__
#undef __TARGET_h8300__
#undef __TARGET_hppa__
#define __TARGET_i386__ 1
#undef __TARGET_i960__
#undef __TARGET_ia64__
#undef __TARGET_m68k__
#undef __TARGET_microblaze__
#undef __TARGET_mips__
#undef __TARGET_nios__
#undef __TARGET_nios2__
#undef __TARGET_powerpc__
#undef __TARGET_sh__
#undef __TARGET_sh64__
#undef __TARGET_sparc__
#undef __TARGET_v850__
#undef __TARGET_vax__
#undef __TARGET_x86_64__

/*
 * Target Architecture Features and Options
 */
#define __TARGET_ARCH__ "i386"
#undef __ARCH_SUPPORTS_BIG_ENDIAN__
#define __ARCH_SUPPORTS_LITTLE_ENDIAN__ 1
#undef __CONFIG_GENERIC_386__
#undef __CONFIG_386__
#undef __CONFIG_486__
#undef __CONFIG_586__
#undef __CONFIG_586MMX__
#undef __CONFIG_686__
#undef __CONFIG_PENTIUMII__
#undef __CONFIG_PENTIUMIII__
#define __CONFIG_PENTIUM4__ 1
#undef __CONFIG_K6__
#undef __CONFIG_K7__
#undef __CONFIG_ELAN__
#undef __CONFIG_CRUSOE__
#undef __CONFIG_WINCHIPC6__
#undef __CONFIG_WINCHIP2__
#undef __CONFIG_CYRIXIII__
#undef __CONFIG_NEHEMIAH__
#define __ARCH_LITTLE_ENDIAN__ 1
#undef __ARCH_BIG_ENDIAN__
#undef __ARCH_HAS_NO_MMU__
#define __ARCH_HAS_MMU__ 1
#define __ARCH_USE_MMU__ 1
#define __UCLIBC_HAS_FLOATS__ 1
/* FM3: For arm. Does not influence x86. */
#define __UCLIBC_HAS_SOFT_FLOAT__ 1
#define __HAS_FPU__ 1
#define __DO_C99_MATH__ 1
#define __KERNEL_SOURCE__ "/home/linux"
#define __C_SYMBOL_PREFIX__ ""
#define __HAVE_DOT_CONFIG__ 1

/*
 * General Library Settings
 */
#undef __HAVE_NO_PIC__
#define __DOPIC__ 1
#define __HAVE_NO_SHARED__
#undef __HAVE_SHARED__
#undef __ARCH_HAS_NO_LDSO__
#undef __BUILD_UCLIBC_LDSO__
#undef __DL_FINI_CRT_COMPAT__
#define __UCLIBC_CTOR_DTOR__ 1
#undef __HAS_NO_THREADS__
#ifdef L4_THREAD_SAFE
#define __UCLIBC_HAS_THREADS__ 1
#else
#undef __UCLIBC_HAS_THREADS
#endif
#define __UCLIBC_HAS_LFS__ 1
#define __UCLIBC_STATIC_LDCONFIG__ 1
#define __MALLOC__ 1
#undef __MALLOC_SIMPLE__
#undef __MALLOC_STANDARD__
#undef __MALLOC_GLIBC_COMPAT__
#undef __UCLIBC_DYNAMIC_ATEXIT__
#undef __HAS_SHADOW__
#define __UNIX98PTY_ONLY__ 1
#define __ASSUME_DEVPTS__ 1
#define __UCLIBC_HAS_TM_EXTENSIONS__ 1
#define __UCLIBC_HAS_TZ_CACHING__ 1
#undef __UCLIBC_HAS_TZ_FILE__
#undef __UCLIBC_HAS_TZ_FILE_READ_MANY__
#undef __UCLIBC_TZ_FILE_PATH__

/*
 * Networking Support
 */
#undef __UCLIBC_HAS_IPV6__
#undef __UCLIBC_HAS_RPC__
#undef __UCLIBC_USE_NETLINK__

/*
 * String and Stdio Support
 */
#define __UCLIBC_HAS_STRING_GENERIC_OPT__ 1
#define __UCLIBC_HAS_STRING_ARCH_OPT__ 1
#define __UCLIBC_HAS_CTYPE_TABLES__ 1
#define __UCLIBC_HAS_CTYPE_SIGNED__ 1
#define __UCLIBC_HAS_CTYPE_UNSAFE__ 1
#undef __UCLIBC_HAS_CTYPE_CHECKED__
#undef __UCLIBC_HAS_CTYPE_ENFORCED__
#define __UCLIBC_HAS_WCHAR__ 1
#undef __UCLIBC_HAS_LOCALE__
#undef __UCLIBC_HAS_HEXADECIMAL_FLOATS__
#undef __UCLIBC_HAS_GLIBC_CUSTOM_PRINTF__
#undef __USE_OLD_VFPRINTF__
#define __UCLIBC_PRINTF_SCANF_POSITIONAL_ARGS__ 9
#undef __UCLIBC_HAS_SCANF_GLIBC_A_FLAG__
#undef __UCLIBC_HAS_STDIO_BUFSIZ_NONE__
#define __UCLIBC_HAS_STDIO_BUFSIZ_256__ 1
#undef __UCLIBC_HAS_STDIO_BUFSIZ_512__
#undef __UCLIBC_HAS_STDIO_BUFSIZ_1024__
#undef __UCLIBC_HAS_STDIO_BUFSIZ_2048__
#undef __UCLIBC_HAS_STDIO_BUFSIZ_4096__
#undef __UCLIBC_HAS_STDIO_BUFSIZ_8192__
#define __UCLIBC_HAS_STDIO_BUILTIN_BUFFER_NONE__ 1
#undef __UCLIBC_HAS_STDIO_BUILTIN_BUFFER_4__
#undef __UCLIBC_HAS_STDIO_BUILTIN_BUFFER_8__
#undef __UCLIBC_HAS_STDIO_SHUTDOWN_ON_ABORT__
#define __UCLIBC_HAS_STDIO_GETC_MACRO__ 1
#define __UCLIBC_HAS_STDIO_PUTC_MACRO__ 1
#define __UCLIBC_HAS_STDIO_AUTO_RW_TRANSITION__ 1
#undef __UCLIBC_HAS_FOPEN_LARGEFILE_MODE__
#undef __UCLIBC_HAS_FOPEN_EXCLUSIVE_MODE__
#undef __UCLIBC_HAS_GLIBC_CUSTOM_STREAMS__
#undef __UCLIBC_HAS_PRINTF_M_SPEC__
#define __UCLIBC_HAS_ERRNO_MESSAGES__ 1
#undef __UCLIBC_HAS_SYS_ERRLIST__
#define __UCLIBC_HAS_SIGNUM_MESSAGES__ 1
#undef __UCLIBC_HAS_SYS_SIGLIST__
#define __UCLIBC_HAS_GNU_GETOPT__ 1

/*
 * Big and Tall
 */
#define __UCLIBC_HAS_REGEX__ 1
#undef __UCLIBC_HAS_WORDEXP__
#undef __UCLIBC_HAS_FTW__
#define __UCLIBC_HAS_GLOB__ 1

/*
 * Library Installation Options
 */
#define __SYSTEM_LDSO__ "/lib/ld-linux.so.2"
#define __RUNTIME_PREFIX__ "/usr/$(TARGET_ARCH)-linux-uclibc/"
#define __DEVEL_PREFIX__ "/usr/$(TARGET_ARCH)-linux-uclibc/usr/"

/*
 * uClibc security related options
 */
#undef __UCLIBC_SECURITY__

/*
 * uClibc development/debugging options
 */
#define __CROSS_COMPILER_PREFIX__ ""
#undef __DODEBUG__
#undef __DOASSERTS__
#define __UCLIBC_MALLOC_DEBUGGING__ 1
#define __WARNINGS__ "-Wall"
#undef __UCLIBC_MJN3_ONLY__

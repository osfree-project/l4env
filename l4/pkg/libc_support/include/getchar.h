#ifndef __LIBC_SUPPORT__INCLUDE__GETCHAR_H__
#define __LIBC_SUPPORT__INCLUDE__GETCHAR_H__

/* dietlibc/uClibc hack to be able to overwrite getchar */
#undef getchar

extern int getchar(void);

#endif /* ! __LIBC_SUPPORT__INCLUDE__GETCHAR_H__ */

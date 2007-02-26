#ifndef __LIBC_SUPPORT__INCLUDE__PANIC_H__
#define __LIBC_SUPPORT__INCLUDE__PANIC_H__

#ifdef __cplusplus
extern "C"
#endif
void panic(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif /* ! __LIBC_SUPPORT__INCLUDE__PANIC_H__ */

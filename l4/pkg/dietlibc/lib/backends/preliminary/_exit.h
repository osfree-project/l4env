#if !defined(___EXIT_H__)
#define ___EXIT_H___

void _exit(int code) __attribute__ ((__noreturn__));
void exit_sleep(void) __attribute__ ((__noreturn__));

void __thread_doexit(int code);

#endif // ___EXIT_H__

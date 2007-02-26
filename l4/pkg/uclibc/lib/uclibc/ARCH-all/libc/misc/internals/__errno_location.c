#include <errno.h>
#undef errno

#ifdef L4_THREAD_SAFE

int * __tls_location (volatile int *tls_key, int tls_field[], int tls_used[]);

static volatile int errno_key = -1;
static int          errno_field[128];
static int          errno_used[sizeof(errno_field)/32];

int * __errno_location (void)
{
    return __tls_location (&errno_key, errno_field, errno_used);
}

#else

int * weak_const_function __errno_location (void)
{
    return &errno;
}

#endif

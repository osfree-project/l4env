#define __FORCE_GLIBC
#include <features.h>
#include <netdb.h>
#undef h_errno

#ifdef L4_THREAD_SAFE

int * __tls_location (volatile int *tls_key, int tls_field[], int tls_used[]);

static volatile int h_errno_key = -1;
static int          h_errno_field[128];
static int          h_errno_used[sizeof(h_errno_field)/32];

int * __h_errno_location (void)
{
    return __tls_location (&h_errno_key, h_errno_field, h_errno_used);
}

#else

int * weak_const_function __h_errno_location (void)
{
    return &h_errno;
}

#endif

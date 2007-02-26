/* Idea shamelessly stolen from l4/pkg/zlib/lib/stdio-ux/stdio.c: DUP(x) ... */
#define PROTO_IMPL(x) x; x

// dummies to make it link, until I fixed it
PROTO_IMPL(int getsockopt(void)){ return 0; }
PROTO_IMPL(int getpeername(void)){ return -1; }




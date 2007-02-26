#ifndef L4_FASTCALL_H__
#define L4_FASTCALL_H__

#undef FASTCALL
#define FASTCALL(x) __attribute__((regparm(3))) x 

#endif


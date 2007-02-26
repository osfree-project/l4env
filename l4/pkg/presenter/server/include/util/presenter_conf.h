

#include <l4/log/l4log.h>

#ifndef NULL
#define NULL (void *)0
#endif

int printf( const char *format, ...);

#define PRESDEBUG(x) x

#define u8              unsigned char
#define s8              signed char
#define u16     unsigned short
#define s16             signed short
#define u32             unsigned long
#define s32             signed long
#define adr             unsigned long

struct presenter_services {
	void *(*get_module)		(char *name);
	long  (*register_module)	(char *name,void *services); 
};

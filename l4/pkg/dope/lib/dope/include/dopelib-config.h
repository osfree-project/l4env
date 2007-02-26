#if !defined(NULL)
#define NULL (void *)0
#endif

#define SHOW_INFOS 1
#define SHOW_WARNINGS 1
#define SHOW_ERRORS 1

#if SHOW_WARNINGS
	#define WARNING(x) x
#else
	#define WARNING(x) /* x */
#endif

#if SHOW_INFOS
	#define INFO(x) x
#else
	#define INFO(x) /* x */
#endif

#if SHOW_ERRORS
	#define ERROR(x) x
#else
	#define ERROR(x) /* x */
#endif

#define uint8	unsigned char
#define sint8	signed char
#define uint16 	unsigned short
#define sint16	signed int
#define uint32	unsigned long
#define sint32	signed long


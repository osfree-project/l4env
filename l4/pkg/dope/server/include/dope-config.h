#if !defined(NULL)
#define NULL (void *)0
#endif


#define SHOW_INFOS 0
#define SHOW_WARNINGS 0
#define SHOW_ERRORS 1

#define DEBUGMODE 0

int printf( const char *format, ...);

#if DEBUGMODE
	#define DOPEDEBUG(x) x
#else
	#define DOPEDEBUG(x) /* x */
#endif

#if SHOW_WARNINGS
	#define WARNING(x) x
#else
	#define WARNING(x) /* x */
#endif

#undef INFO
#if SHOW_INFOS
	#define INFO(x) x
#else
	#define INFO(x) /* x */
#endif

#undef ERROR
#if SHOW_ERRORS
	#define ERROR(x) x
#else
	#define ERROR(x) /* x */
#endif

#define u8		unsigned char
#define s8		signed char
#define u16 	unsigned short
#define s16		signed short
#define u32		unsigned long
#define s32		signed long
#define adr		unsigned long

struct dope_services {
	void	*(*get_module) 		(char *name);
	long	 (*register_module)	(char *name,void *services);
};

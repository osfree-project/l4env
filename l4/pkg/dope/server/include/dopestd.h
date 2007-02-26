/*
 * \brief   Standard types and functions used by DOpE
 * \date    2003-08-01
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


/*** TYPES USED BY DOpE ***
 *
 * Within DOpE the following fixed-width types should be
 * used.
 */

#define u8  unsigned char
#define s8    signed char
#define u16 unsigned short
#define s16   signed short
#define u32 unsigned long
#define s32   signed long
#define adr unsigned long

#if !defined(NULL)
#define NULL (void *)0
#endif


/*** STANDARD FUNCTIONS USED BY DOPE ***
 *
 * Normally, these functions are provided by the underlying
 * libC but they can also be implemented in a dopestd.c file.
 * This way DOpE can easily be ported even to platforms with
 * no libC.
 */

#if !defined(PETZE_POOLNAME)
void  *malloc(unsigned int size);
void   free(void *addr);
#endif

/*** RELYING ON LIBC (SOMEDAY I WILL KICK THEM OUT) ***/
int    snprintf(char *str, unsigned int size, const char *format, ...);
int    strcmp(const char *s1, const char *s2);
double strtod(const char *nptr, char **endptr);
void  *memmove(void *dest, const void *src, unsigned int n);
void  *memcpy(void *dest, const void *src, unsigned int n);
void  *memset(void *s, int c, unsigned int n);
int    printf( const char *format, ...);

/*** IMPLEMENTED IN DOPESTD.C ***/
extern int dope_ftoa(float v, int prec, char *dst, int max_len);
extern int dope_streq(char *s1, char *s2, int max_len);

/*** DEBUG MACROS USED IN DOPE ***
 *
 * Within the code of DOpE the following macros for filtering
 * debug output are used.
 *
 * INFO    - for presenting status information
 * WARNING - for informing the user about non-serious problems
 * ERROR   - for reporting really worse stuff
 */

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


/*** DOPE SERVICE STRUCTURE ***
 *
 * DOpE provides the following service structure to all
 * its components. Via this structure components can
 * access functionality of other components or make an
 * interface available to other components.
 */
struct dope_services {
	void *(*get_module)      (char *name);
	long  (*register_module) (char *name,void *services);
};

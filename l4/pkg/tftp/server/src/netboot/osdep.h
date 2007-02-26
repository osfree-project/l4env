#ifndef __OSDEP_H__
#define __OSDEP_H__

# define ETHERBOOT32
# include "byteorder.h"
# include "io.h"

typedef	unsigned long Address;

/* ANSI prototyping macro */
#ifdef	__STDC__
#define	P(x)	x
#else
#define	P(x)	()
#endif

#endif

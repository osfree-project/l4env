INTERFACE:

#include <cstdio>

#include "config.h"

enum Warn_level
{
  Error   = 0,
  Warning = 1,
  Info    = 2,
};

// We should use something like printf here to take care of the Fiasco-UX
// stack usage
#define WARNX(level,fmt...) \
  do {						\
       if (level   < Config::warn_level)	\
	 {					\
	   printf("\n\033[31mKERNEL: ");	\
	   printf(fmt);				\
	   printf("\033[m\n");			\
	 }					\
     } while (0)

#define WARN(fmt...) WARNX(Warning, fmt)


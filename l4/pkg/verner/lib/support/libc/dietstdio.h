/* cr7 - auch hier gekürzt */

/* diet stdio */

//#include <sys/cdefs.h>
//#include <stdio.h>

#include <sys/types.h>
#include "dietfeatures.h"
#include <stdarg.h>


struct arg_printf {
  void *data;
  int (*put)(void*,size_t,void*);
};





#include "assert.h"
#include "stdio.h"
#include "stdlib.h"

void __assert_fail (const char *__assertion, const char *__file,
			   unsigned int __line, const char *__function)
{
	printf("ASSERTION_FAILED (%s) in function %s in file %s:%d\n",__assertion,__function,__file,__line);
	exit (EXIT_FAILURE);
}

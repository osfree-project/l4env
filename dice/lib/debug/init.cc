#include <stdio.h>
#include "dice/dice-tracing.h"
// our derived tracing class
#include "BETrace.h"
#include "L4BETrace.h"
#include "L4V4BETrace.h"

#include "Compiler.h"

extern "C" {
void *__dso_handle __attribute__ ((weak));
}

/* for Dice includes be sure to set the correct include path in your Makefile
 * ($(DICEDIR)/src).
 */

/* When defining global variables, please use static variables. */

void dice_tracing_init(int argc, char *argv[])
{
    /* This is the right place to parse the Dice arguments and check if there
     * is something for me in it.
     */
}

CTrace* dice_tracing_new_class(void)
{
    if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_V4))
	return new CL4V4BETrace();
    if (CCompiler::IsBackEndInterfaceSet(PROGRAM_BE_V2))
	return new CL4BETrace();
    return new CBETrace();
}

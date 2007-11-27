/* Linux */
#include <linux/gfp.h>
#include <linux/string.h>
#include <asm/page.h>

/* DDEKit */
#include <l4/dde/ddekit/memory.h>
#include <l4/dde/ddekit/assert.h>
#include <l4/dde/ddekit/panic.h>

#include "local.h"

int ioprio_best(unsigned short aprio, unsigned short bprio)
{
	WARN_UNIMPL;
	return 0;
}

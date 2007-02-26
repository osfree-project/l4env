#include <l4/log/l4log.h>
#include <l4/oskit10_l4env/support.h>
#include "startup.h"

void native_startup(int argc, char **argv) {
	LOG_init("DOpEcmd");
	OSKit_libc_support_init(500*1024);
}

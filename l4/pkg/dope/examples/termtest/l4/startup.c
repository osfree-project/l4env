#include <l4/log/l4log.h>
#include "startup.h"

char LOG_tag[9] = "termtest";
l4_ssize_t l4libc_heapsize = 500*1024;

void native_startup(int argc, char **argv) {
}

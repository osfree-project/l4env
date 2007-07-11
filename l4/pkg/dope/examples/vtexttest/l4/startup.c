#include <l4/log/l4log.h>
#include <l4/sys/l4int.h>
#include "startup.h"

char LOG_tag[9] = "YUV-Test";
l4_ssize_t l4libc_heapsize = 500*1024;

void native_startup(int argc, char **argv) {
}

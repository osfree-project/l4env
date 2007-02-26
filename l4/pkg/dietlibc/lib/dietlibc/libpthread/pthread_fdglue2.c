#include <l4/log/l4log.h>
#include <l4/semaphore/semaphore.h>

#include "dietstdio.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

extern int __stdio_atexit;
extern FILE * __stdio_init_file_nothreads(int fd, int closeonerror, int mode);

FILE *__stdio_init_file(int fd, int closeonerror, int mode)
{
    FILE *tmp = __stdio_init_file_nothreads(fd, closeonerror, mode);
    if (tmp)
    {
        tmp->m = L4SEMAPHORE_UNLOCKED;
    }
    return tmp;
}

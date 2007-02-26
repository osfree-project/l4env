#include <l4/log/l4log.h>
#include <l4/semaphore/semaphore.h>

#include "dietstdio.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

int fputc(int c, FILE *stream)
{
    int ret;

    l4semaphore_down(&(stream->m));
    ret = fputc_unlocked(c, stream); 
    l4semaphore_up(&(stream->m));
    
    return ret;
}

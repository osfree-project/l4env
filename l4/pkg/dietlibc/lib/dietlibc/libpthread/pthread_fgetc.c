#include <l4/log/l4log.h>
#include <l4/semaphore/semaphore.h>

#include "dietstdio.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

int fgetc(FILE *stream)
{
    int ret;

    l4semaphore_down(&(stream->m));
    ret = fgetc_unlocked(stream); 
    l4semaphore_up(&(stream->m));
    
    return ret;
}

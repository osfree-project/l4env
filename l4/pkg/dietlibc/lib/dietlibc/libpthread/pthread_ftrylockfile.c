#include <l4/semaphore/semaphore.h>

#include "dietstdio.h"

int ftrylockfile(FILE *f)
{
    return l4semaphore_try_down(&f->m);
}

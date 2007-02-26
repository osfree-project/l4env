#include <l4/semaphore/semaphore.h>

#include "dietstdio.h"

void funlockfile(FILE *f)
{
    l4semaphore_up(&f->m);
}

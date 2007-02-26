#include <l4/semaphore/semaphore.h>

#include "dietstdio.h"

void flockfile(FILE *f)
{
    l4semaphore_down(&f->m);
}

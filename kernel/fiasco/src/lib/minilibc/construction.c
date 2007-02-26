
#include "initfini.h"
#include "types.h"

typedef void (ctor_t)(void);

__BEGIN_DECLS

ctor_t *__CTOR_LIST__;
ctor_t *__DTOR_LIST__;

__END_DECLS


static int construction_done = 0;

void static_construction()
{
  ctor_t **cons = &__CTOR_LIST__;
  Mword count = (Mword)*(--cons);
  while(count--)
    if(*(--cons)) {
      (**(cons))();
    }
  construction_done = 1;
}


void static_destruction()
{
  ctor_t **cons = &__DTOR_LIST__;
  Mword count = (Mword)*(--cons);
  if(!construction_done)
    return;

  while(count--) 
    if(*(--cons)) { 
      (**(cons))();
  }
}

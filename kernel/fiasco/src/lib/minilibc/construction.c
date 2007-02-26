
#include "initfini.h"
#include "types.h"

typedef void (ctor_t)(void);

__BEGIN_DECLS

ctor_t *__CTOR_LIST__;
ctor_t *__DTOR_LIST__;

__END_DECLS



//#include "stdio.h"

static int construction_done = 0;

void static_construction()
{
  ctor_t **cons = &__CTOR_LIST__;
  Mword count = (Mword)*(--cons);
  while(count--)
    if(*(--cons)) {
      //      printf("cons: @%p\n",*cons);
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

  //puts("Call destructors");
  //printf(" %d\n",count);
  while(count--) {
    //printf("  call dtor: %p\n",*(cons-1));
    if(*(--cons)) 
      (**(cons))();
  }
}

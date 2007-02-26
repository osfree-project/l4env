#include <l4/sys/l4int.h>
#include <l4/crtx/crt0.h>

// external prototype cause we don't want to include stdlib.h cause we
// use the plain mode without the path to any C library
int atexit(void (*__function)(void));

typedef void (ctor_t)(void);

extern ctor_t *__CTOR_LIST__;
extern ctor_t *__DTOR_LIST__;
extern ctor_t *__C_CTOR_LIST__;
extern ctor_t *__C_DTOR_LIST__;
extern ctor_t *__CTOR_END__;
extern ctor_t *__DTOR_END__;
extern ctor_t *__C_CTOR_END__;
extern ctor_t *__C_DTOR_END__;
extern ctor_t *__C_DTOR_SYS_END__;

static inline void
static_construction(void)
{
  ctor_t **cons;

  /* call constructors made with L4_C_CTOR */
  for (cons = &__C_CTOR_LIST__; cons < &__C_CTOR_END__; cons++)
    if(*(cons))
      (**(cons))();

  /* call constructors made with __attribute__((constructor)) 
   * and static C++ constructors */
  cons = &__CTOR_END__;
  while(cons > &__CTOR_LIST__)
    if(*(--cons))
      (**(cons))();
}


static void
static_destruction(void)
{
  ctor_t **cons;

  /* call destructors made with __attribute__((destructor)) 
   * and static C++ destructors */
  for (cons = &__DTOR_LIST__; cons < &__DTOR_END__; cons++)
    if(*(cons))
      (**(cons))();

  /* call destructors made with L4_C_DTOR except system destructors */
  cons = &__C_DTOR_END__;
  while(cons > &__C_DTOR_SYS_END__)
    if(*(--cons))
      (**(cons))();
}

/* call system destructors */
void
crt0_sys_destruction()
{
  ctor_t **cons;

  cons = &__C_DTOR_SYS_END__;
  while(cons > &__C_DTOR_LIST__)
    if(*(--cons))
      (**(cons))();
}

/* is called by crt0 immediately before calling __main() */
void
crt0_construction()
{
  static_construction();
  atexit(&static_destruction);
}


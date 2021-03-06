#ifndef L4_TYPES_API_H
#define L4_TYPES_API_H

#include <l4/sys/compiler.h>
#include <l4/sys/l4int.h>

/**
 * L4 thread id structure
 */
typedef struct {
  l4_umword_t version_low :10;
  l4_umword_t lthread :7;
  l4_umword_t task    :11;
  l4_umword_t version_high:4;
} l4_threadid_struct_t;

/*
 * L4 unique identifiers
 */
typedef union l4_threadid_t {
  l4_uint32_t raw;
  l4_threadid_struct_t id;
} l4_threadid_t;

typedef l4_threadid_t l4_taskid_t;

typedef union {
  struct {
    l4_umword_t intr      :8;
    l4_umword_t zero      :L4_MWORD_BITS-8;
  } id;
  l4_umword_t raw;
} l4_intrid_t;

#define L4_NIL_ID_INIT	     {raw:0}
#define L4_NIL_ID	     ((l4_threadid_t)L4_NIL_ID_INIT)
#define L4_INVALID_ID_INIT   {raw:(l4_umword_t)-1}
#define L4_INVALID_ID        ((l4_threadid_t)L4_INVALID_ID_INIT)

L4_INLINE int           l4_is_nil_id              (l4_threadid_t id);
L4_INLINE int           l4_is_invalid_id          (l4_threadid_t id);
L4_INLINE l4_threadid_t l4_get_taskid             (l4_threadid_t t);
L4_INLINE int           l4_thread_equal           (l4_threadid_t t1,l4_threadid_t t2);
L4_INLINE int           l4_task_equal             (l4_threadid_t t1,l4_threadid_t t2);
L4_INLINE void          l4_make_taskid_from_irq   (int irq, l4_threadid_t *t);

#define l4_tasknum_equal(t1, t2) l4_task_equal(t1, t2)

/*-----------------------------------------------------------------------------
 * IMPLEMENTATION
 *---------------------------------------------------------------------------*/
L4_INLINE int l4_is_nil_id(l4_threadid_t id)
{
  return id.raw == 0;
}

L4_INLINE int l4_is_invalid_id(l4_threadid_t id)
{
  return id.raw == (l4_umword_t)-1;
}

L4_INLINE l4_threadid_t
l4_get_taskid(l4_threadid_t t)
{
  t.id.lthread = 0;
  return t;
}

L4_INLINE int
l4_thread_equal(l4_threadid_t t1,l4_threadid_t t2)
{
  return t1.raw == t2.raw;
}

L4_INLINE int
l4_task_equal(l4_threadid_t t1,l4_threadid_t t2)
{
  return t1.id.task == t2.id.task;
}

L4_INLINE void
l4_make_taskid_from_irq(int irq, l4_threadid_t *t)
{
  t->raw = irq+1;
}

#endif /* L4_TYPES_API_H */

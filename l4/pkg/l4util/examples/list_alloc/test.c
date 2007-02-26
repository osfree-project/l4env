#include <l4/util/list_alloc.h>
#include <l4/rmgr/librmgr.h>
#include <stdio.h>

static l4la_free_t *l;

#define test(f,w)  { g=(l4_addr_t)f; if (g!=w) failed(__LINE__, w, g); }

static void
failed(unsigned long line, l4_addr_t w, l4_addr_t g)
{
  printf("failed test at line %ld: expected %08lx got %08lx\n",
      line, (unsigned long)w, (unsigned long)g);
}

int
main(void)
{
  l4_addr_t addr, g;
  l4_size_t size;

  l4la_init(&l);
  printf("avail=%d\n", l4la_avail(&l));

  size = 4096;
  addr = rmgr_reserve_mem(size, 0, 0, 0, 0);
  l4la_free(&l, (void*)addr, size);

  test (l4la_alloc (&l,   10, 2), addr);
  test (l4la_alloc (&l,   10, 2), addr + 16);
  test (l4la_alloc (&l,   10, 2), addr + 32);
  test (l4la_alloc (&l, 4049, 2), 0);
  test (l4la_alloc (&l, 4048, 2), addr + 48);
  l4la_free (&l, (void*)(addr + 48), 4048);
  test (l4la_avail (&l)         ,      4048);
  
  return 0;
}

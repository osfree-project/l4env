#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <l4/env_support/panic.h>
#include <l4/util/l4_macros.h>

#include "region.h"
#include "module.h"

static region_t __region[MAX_REGION];
static int size;

static unsigned less_than(region_t *l, region_t *r)
{
  return l->end <= r->begin;
}

static int test_fit(l4_addr_t start, unsigned long _size)
{
  unsigned i;
  region_t r = {start, start + _size};
  for (i = 0; i < size; ++i)
    {
#if 0
      printf("test [%p-%p] [%p-%p]\n", (char*)start, (char*)start + _size,
	  (char*)__region[i].begin, (char*)__region[i].end);
#endif

      if (!less_than(__region + i, &r) && !less_than(&r, __region + i))
	return 0;
    }
  return 1;
}

static l4_addr_t next_free(l4_addr_t start)
{
  unsigned i;
  l4_addr_t s = ~0UL;
  for (i = 0; i < size; ++i)
    {
      if (__region[i].end > start && __region[i].end < s)
	s = __region[i].end;
    }

  return s;
}

void 
region_name(l4_addr_t begin, l4_addr_t end, const char *name)
{
  unsigned i;
  for (i = 0; i < size; ++i)
    if (__region[i].begin == begin && __region[i].end == end)
      {
	__region[i].name = name;
	break;
      }
}

l4_addr_t region_find_free(l4_addr_t start, l4_addr_t end, unsigned long _size,
    unsigned align)
{
  while (1)
    {
      start = (start + (1UL << align) -1) & ~((1UL << align)-1);
     
      if (start + _size > end)
	return 0;

      // printf("try start %p\n", (void*)start);
      if (test_fit(start, _size))
	return start;

      start = next_free(start);

      if (start == ~0UL)
	return 0;
    }
}

void
region_add(l4_addr_t begin, l4_addr_t end, const __mb_mod_name_str name)
{
  int r;

  /* Do not add empty regions */
  if (begin == end)
    return;

  if (size >= MAX_REGION)
    panic("Bootstrap: %s: Region overflow\n", __func__);

  if ((r = region_overlaps(begin, end)) != REGION_NO_OVERLAP)
    {
      printf("  loaded module section:   ["l4_addr_fmt"-"l4_addr_fmt") %s\n",
	     begin, end, name);
      printf("  overlaps with:         ");
      region_print(r);

      regions_dump();
      panic("region overlap");
    }

  __region[size].begin = begin;
  __region[size].end = end;
  __region[size].name = name;

  size++;
}

int
region_overlaps(l4_addr_t begin, l4_addr_t end)
{
  int i;

  for (i = 0; i < size; i++)
    if (__region[i].begin <= end && __region[i].end > begin)
      return i;

  return REGION_NO_OVERLAP;
}

void
region_print(int i)
{
  if (i >= size)
    return;

  printf("  ["l4_addr_fmt"-"l4_addr_fmt") ", __region[i].begin, __region[i].end);
  if (*__region[i].name == '.')
    printf("%s", __region[i].name+1);
  else
    print_module_name(__region[i].name, "");
  putchar('\n');
}

void
regions_dump(void)
{
  int i, j;
  int mark = 0, min, min_idx;

  for (i = 0; i < size; i++)
    {
      min = ~0;
      min_idx = -1;
      for (j = 0; j < size; j++)
	if (__region[j].begin < min && __region[j].begin >= mark)
	  {
	    min     = __region[j].begin;
	    min_idx = j;
	  }
      if (min_idx == -1)
	printf("Check region dump\n");
      region_print(min_idx);
      mark = __region[min_idx].end;
    }
}

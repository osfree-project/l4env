#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifndef USE_OSKIT
#include <l4/env_support/panic.h>
#endif

#include "region.h"
#include "module.h"

static region_t __region[MAX_REGION];
static int size;

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
      printf("  loaded module section:   [%08x-%08x) %s\n",
	     begin, end, name);
      printf("  overlaps with:         ");
      region_print(r);

      regions_dump();
      panic("");
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

  printf("  [%08x-%08x) ", __region[i].begin, __region[i].end);
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

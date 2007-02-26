#include <stdio.h>
#include <string.h>

#include "region.h"

static region_t __region[MAX_REGION];
static int size;

void
region_init(void)
{
  size = 1;
}

region_t*
get_region(int i)
{
  if (i < size)
    return &__region[i];

  return 0;
}

int
region_add(l4_addr_t begin, l4_addr_t end, int task_no, const char *name)
{
  const char *n;
  char *p;
  int i;

  if (size >= MAX_REGION)
    return 0;

  __region[size].begin = begin;
  __region[size].end   = end;

  /* search for end-of-filename */
  for (n=name; *n!='\0' && *n!=' '; n++)
    ;
  /* go back until start of filename */
  for (i=0; n>name && *(n-1)!='/' && i<=sizeof(__region[0].name); i++, n--)
    ;
  p = __region[size].name;
  i = sizeof(__region[0].name);
  if (task_no != -1)
    {
      snprintf(p, i, "#%02x ", task_no);
      p += 4;
      i -= 4;
    }
  snprintf(p, i, "%s", n);
  return size++;
}

int
region_overlaps(l4_addr_t begin, l4_addr_t end)
{
  int i;

  for (i = 1; i < size; i++)
    if (__region[i].begin <= end && __region[i].end >= begin)
      return i;

  return 0;
}

int
region_find(l4_addr_t begin, l4_addr_t end)
{
  int i;
  int j = -1;

  for (i = 1; i < size; i++)
    {
      if (__region[i].begin >= begin && __region[i].end <= end)
	{
	  if (j > -1)
	    {
	      if (__region[i].begin < __region[j].begin)
		j = i;
	    }
	  else
	    j = i;
	}
    }

  return j;
}

void
region_print(int i)
{
  if (i > 0 && i < size)
    printf("  [%08x-%08x) %s\n",
	__region[i].begin, __region[i].end, __region[i].name);
}

void
regions_dump(void)
{
  int i;

  printf("Roottask regions:\n");
  for (i = 1; i < size; i++)
    region_print(i);
}

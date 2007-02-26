#ifndef REGION_H
#define REGION_H

#include <l4/sys/compiler.h>
#include <l4/sys/types.h>

#include "types.h"

#define MAX_REGION 16

#define REGION_NO_OVERLAP -1

typedef struct
{
  l4_addr_t begin;
  l4_addr_t end;
  const char *name;
} region_t;

void region_add(l4_addr_t begin, l4_addr_t end, const char *name);
int  region_overlaps(l4_addr_t begin, l4_addr_t end);
void region_print(int i);
void regions_dump(void);

#endif

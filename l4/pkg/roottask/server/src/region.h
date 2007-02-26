#ifndef REGION_H
#define REGION_H

#include <l4/sys/compiler.h>
#include <l4/sys/types.h>

#define MAX_REGION 64

typedef struct
{
  l4_addr_t begin;
  l4_addr_t end;
  char      name[44];
} region_t;

void      region_init(void);
region_t* get_region(int i);
int       region_add(l4_addr_t begin, l4_addr_t end,
		     int task_no, const char *name);
int       region_overlaps(l4_addr_t begin, l4_addr_t end);
int       region_find(l4_addr_t begin, l4_addr_t end);
void      region_print(int i);
void      regions_dump(void);

#endif

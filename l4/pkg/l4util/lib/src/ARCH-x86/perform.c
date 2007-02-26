/*
 
 $Id$
 
 lib for performance mesaurement counters. You need a bsearch function,
 which is not included in standard-oskit. Use the oskit 1.0 and the
 freebsd-libc (liboskit_freebsd_c.a).

*/

#include <l4/sys/types.h>

#define CONFIG_PERFORM_ONLY_PROTOTYPES
#include <l4/util/perform.h>

extern void *bsearch(const void *key, const void *base, unsigned nmemb,
              unsigned size, int (*compar)(const void *, const void *));
static int compare(const void*key, const void*arr_elem);

typedef struct{
         l4_uint32_t index;
         const char *ev_string;
         } event_entry;

static const event_entry event_array[]={
	#include "pmc_events.h"
	};

static int compare(const void*key, const void*arr_elem){
  return(*((l4_uint32_t*)key) - ((event_entry*)arr_elem)->index);
}

const char*strp6pmc_event(l4_uint32_t event){
  event_entry *found;
  l4_uint32_t key = event & 0xff;  
  
  found = bsearch(&key, event_array, sizeof(event_array)/sizeof(event_entry),
                  sizeof(event_entry), compare);
  if(found) return found->ev_string;
  return "unkown";
}

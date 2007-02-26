#ifndef SLMAP_H
#define SLMAP_H

#include <stdlib.h>

/*
 * the map structure
 *
 * its basically a single linked list with a
 * key and a data entry
 */
typedef struct slmap_t
{
  struct slmap_t* next;    /* pointer to next entry */
  void *key;             /* the key of the map entry */
  unsigned key_size;     /* the size of the key entry */
  void *data;            /* the data of the map entry */
} slmap_t;

/*
 * function prototypes
 */

slmap_t*
map_new_entry(void* key, unsigned key_size, void *data);

slmap_t*
map_new_sentry(char* key, void* data);

slmap_t*
map_append(slmap_t* list, slmap_t* new_entry);

slmap_t*
map_remove(slmap_t* list, slmap_t* entry);

void
map_free_entry(slmap_t** entry);

static inline void
map_free(slmap_t** list);

static inline unsigned char
map_is_empty(slmap_t* list);

slmap_t*
map_get_at(slmap_t* list, int n);

slmap_t*
map_add(slmap_t* list, slmap_t* new_entry);

void
map_insert_after(slmap_t* after, slmap_t* new_entry);

static inline int
map_elements(slmap_t* list);

slmap_t*
map_find(slmap_t* list, void* key, unsigned key_size);

slmap_t*
map_sfind(slmap_t* list, const char* key);

/*
 * implementatic of static inline
 */

static inline unsigned char
map_is_empty(slmap_t* list)
{
  return (list) ? 0 : 1;
}

static inline void
map_free(slmap_t **list)
{
  while (*list)
    *list = map_remove(*list, *list);
}

static inline int
map_elements(slmap_t* list)
{
  register int n;
  for (n=0; list; list=list->next) 
      n++;
  return n;
}

#endif /* SLMAP_H */


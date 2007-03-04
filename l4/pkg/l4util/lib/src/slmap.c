#include <l4/util/slmap.h>
#include <string.h>

/*
 * function implementations
 */

slmap_t*
map_new_entry(void* key, unsigned key_size, void* data)
{
  slmap_t* map;

  map = malloc(sizeof(slmap_t));
  if (!map)
    return (slmap_t*)0;

  map->key = key;
  map->key_size = key_size;
  map->data = data;
  map->next = 0;

  if ((!map->key) && map->key_size)
    map->key_size = 0;

  return map;
}

slmap_t*
map_new_sentry(char *key, void* data)
{
  if (!key)
    return (slmap_t*)0;
  return map_new_entry((void*)key, strlen(key), data);
}

slmap_t*
map_append(slmap_t* list, slmap_t* new_entry)
{
  slmap_t *ret = list;
  if (!list)
    return new_entry;

  while (list->next) list = list->next;
  list->next = new_entry;
  return ret;
}

slmap_t*
map_remove(slmap_t* list, slmap_t* entry)
{
  slmap_t* ret = list;
  if (!list)
    return list;
  if (!entry)
    return list;

  // search entry
  if (list == entry)
    ret = list->next;
  else
    {
      while (list && (list->next != entry))
	list = list->next;
      if (list)
	list->next = entry->next;
      else
	return ret;
    }
 map_free_entry(&entry);
 return ret;
}

void
map_free_entry(slmap_t **entry)
{
  if (*entry)
    {
      free(*entry);
      *entry = 0;
    }
}

slmap_t*
map_get_at(slmap_t* list, int n)
{
  int i=0;
  while (list)
    {
      i++;
      if (i==n)
	break;
      list = list->next;
    }
  return list;
}

slmap_t*
map_add(slmap_t* list, slmap_t* entry)
{
  if (!entry)
    return list;

  entry->next = list;
  return entry;
}

void
map_insert_after(slmap_t* after, slmap_t* new_entry)
{
  if (!new_entry)
    return;
  if (!after)
    return;
  new_entry->next = after->next;
  after->next = new_entry;
}

slmap_t*
map_find(slmap_t* list, void* key, unsigned key_size)
{
  while (list)
    {
      if (memcmp(list->key, key, (key_size < list->key_size) ? key_size : list->key_size) == 0)
	return list;
      list = list->next;
    }
  return (slmap_t*)0;
}

slmap_t*
map_sfind(slmap_t* list, const char* key)
{
  if (!key)
    return (slmap_t*)0;
  return map_find(list, (void*)key, strlen(key));
}


#include <string.h>

#include <l4/util/slmap.h>
#include <l4/log/l4log.h>
#include <l4/sys/l4int.h>

l4_ssize_t l4libc_heapsize = 100*1024;

int main(void)
{
  slmap_t *list, *entry;

  list = map_new_entry("key0", strlen("key0"), "data0");
  list = map_append(list, map_new_entry("key1", strlen("key1"), "data1"));
  list = map_append(list, map_new_sentry("key2", "data2"));
  list = map_append(list, map_new_entry("key3", strlen("key3"), "data3"));

  LOG("list created");
  
  if (map_is_empty(list))
    LOG("empty list");
  else
    LOG("list is not empty");

  LOG("list has %d entries", map_elements(list));

  for (entry = list; entry; entry = entry->next)
    LOG("List element: \"%s\":\"%s\"", (char*)entry->key, (char*)entry->data);

  entry = map_get_at(list, 2);

  LOG("String of entry 2 is \"%s\"", entry ? (char*)entry->data : "");

  list = map_add(list, map_new_entry("key-1", strlen("key-1"), "data-1"));

  LOG("after insert entry");
  for (entry = list; entry; entry = entry->next)
    LOG("List element: \"%s\":\"%s\"", (char*)entry->key, (char*)entry->data);

  entry = map_get_at(list, 3);
  map_insert_after(entry, map_new_entry("key6", strlen("key6"), "data6"));
  list = map_remove(list, entry); // entry is automatically freed

  LOG("after element exchange");
  for (entry = list; entry; entry = entry->next)
    LOG("List element: \"%s\":\"%s\"", (char*)entry->key, (char*)entry->data);

  entry = map_find(list, "key3", strlen("key3"));
  LOG("data of \"key3\" is \"%s\"", (char*)entry->data);
  entry = map_sfind(list, "key2");
  LOG("data of \"key2\" is \"%s\"", (char*)entry->data);

  // empty list
  map_free(&list);

  LOG("after delete:");
  for (entry = list; entry; entry = entry->next)
    LOG("List element: \"%s\":\"%s\"", (char*)entry->key, (char*)entry->data);

  return 0;
}


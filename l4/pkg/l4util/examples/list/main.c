#include <l4/util/sll.h>
#include <l4/log/l4log.h>

l4_ssize_t l4libc_heapsize = 100*1024;

int main(void)
{
  slist_t *list, *node;

  list = list_new_entry("string0");
  list = list_append(list, list_new_entry("string1"));
  list = list_append(list, list_new_entry("string2"));
  list = list_append(list, list_new_entry("string3"));

  LOG("list created");
  
  if (list_is_empty(list))
    LOG("empty list\n");

  LOG("list has %d entries", list_elements(list));

  for (node = list; node; node = node->next)
    LOG("List element: \"%s\"", (char*)node->data);

  node = list_get_at(list, 2);

  LOG("String of node 2 is \"%s\"", node ? (char*)node->data : "");

  list = list_add(list, list_new_entry("string-1"));

  LOG("after insert node");
  for (node = list; node; node = node->next)
    LOG("List element: \"%s\"", (char*)node->data);

  node = list_get_at(list, 3);
  list_insert_after(node, list_new_entry("string6"));
  list = list_remove(list, node); // node is automatically freed

  LOG("after element exchange");
  for (node = list; node; node = node->next)
    LOG("List element: \"%s\"", (char*)node->data);

  // empty list
  while (list)
    {
      list = list_remove(list, list);
    }

  LOG("after delete:");
  for (node = list; node; node = node->next)
    LOG("List element: \"%s\"", (char*)node->data);

  return 0;
}


IMPLEMENTATION[v2x0]:

#include "space_index.h"


PUBLIC static
Space*
Jdb::lookup_space(Task_num task)
{
  return task == 0 ? (Space*)Kmem::dir() : Space_index(task).lookup();
}


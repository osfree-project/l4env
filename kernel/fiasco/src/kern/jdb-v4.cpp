IMPLEMENTATION[v4]:

PUBLIC static
Space*
Jdb::lookup_space(Task_num task)
{
  // XXX
  return task == 0 ? (Space*)Kmem::dir() : 0;
}


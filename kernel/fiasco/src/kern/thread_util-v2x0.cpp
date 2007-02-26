IMPLEMENTATION[v2x0]:

inline NEEDS[lookup_thread]
Thread*
lookup_first_thread(unsigned space)
{
  return lookup_thread( L4_uid( space, 0 ) );
}

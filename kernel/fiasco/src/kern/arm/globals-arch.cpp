/* ARM Dependent Globals! */

INTERFACE:

IMPLEMENTATION[arch]:

inline NEEDS [context_of]
Context *current()
{
  void* esp;
  __asm__ __volatile__ 
    ("mov %0, sp" : "=r" (esp));

  return context_of (esp);
}

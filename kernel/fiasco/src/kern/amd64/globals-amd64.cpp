IMPLEMENTATION[amd64]:

inline NEEDS [context_of]
Context *current()
{
  void* esp;
  __asm__ __volatile__ 
    ("movq %%rsp, %0" : "=q" (esp));

  return context_of (esp);
}

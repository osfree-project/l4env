IMPLEMENTATION[ia32,ux]:

inline NEEDS [context_of]
Context *current()
{
  void* esp;
  __asm__ __volatile__ 
    ("movl %%esp, %0" : "=q" (esp));

  return context_of (esp);
}

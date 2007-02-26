IMPLEMENTATION [amd64]:

inline
void
atomic_add (Mword *l, Mword value)
{
  asm volatile ("addq %1, %2" : "=m"(*l) : "ir"(value), "m"(*l));
}

inline
void
atomic_and (Mword *l, Mword mask)
{
  asm volatile ("andq %1, %2" : "=m"(*l) : "ir"(mask), "m"(*l));
}

inline
void
atomic_or (Mword *l, Mword bits)
{
  asm volatile ("orq %1, %2" : "=m"(*l) : "ir"(bits), "m"(*l));
}

// ``unsafe'' stands for no safety according to the size of the given type.
// There are type safe versions of the cas operations in the architecture
// independent part of atomic that use the unsafe versions and make a type
// check.

inline
bool
cas_unsafe (Mword *ptr, Mword oldval, Mword newval)
{
  Mword tmp;

  asm volatile
    ("cmpxchgq %1, %2"
     : "=a" (tmp)
     : "r" (newval), "m" (*ptr), "a" (oldval)
     : "memory");

  return tmp == oldval;
}

inline
bool
cas2_unsafe (Mword *ptr, Mword *oldval, Mword *newval)
{
  char ret;
  asm volatile
    ("cmpxchg8b %3 ; sete %%cl"
     : "=c" (ret), 
       "=a" (* oldval), 
       "=d" (*(oldval+1))
     : "m" (*ptr) , 
       "a" (* oldval), "d" (*(oldval+1)), 
       "b" (* newval), "c" (*(newval+1))
     : "memory");

  return ret;
}

inline
bool
tas (Mword *l)
{
  Mword tmp;
  asm volatile ("xchg %0, %1" : "=r"(tmp) : "m"(*l), "0"(1) : "memory");
  return tmp;
}


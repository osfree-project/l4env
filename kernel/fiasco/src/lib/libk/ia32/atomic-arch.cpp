INTERFACE:

#include "types.h"

IMPLEMENTATION[arch]:


// atomic operations
// the unsave stands for no safety according to the 
// size of the given type. There are type safe versions
// of the cas operations in the architecture independent 
// part of atomic that use the unsave versions and make a 
// type check.

inline 
bool up_cas_unsave( Mword *ptr, Mword oldval, Mword newval )
{
  Mword tmp;

  asm volatile
    ("cmpxchgl %1,%2"
     : "=a" (tmp)
     : "r" (newval), "m" (*ptr), "0" (oldval)
     : "memory");

  return tmp == oldval;
}

inline 
bool smp_cas_unsave( Mword *ptr, Mword oldval, Mword newval )
{
  return up_cas_unsave(ptr,oldval,newval);
}

inline 
bool up_cas2_unsave( Mword *ptr, Mword *oldval, Mword *newval )
{
  char ret;
  asm volatile
    ("cmpxchg8b %3 ; sete %%cl"
     : "=c" (ret),
       "=a" (* oldval),
       "=d" (*(oldval+1))
     : "m" (*ptr) ,
       "1" (* oldval),
       "2" (*(oldval+1)), 
       "b" (* newval),
       "0" (*(newval+1))
     : "memory");

  return ret;
}


inline 
bool smp_cas2_unsave( Mword *ptr, Mword *oldval, Mword *newval )
{
  return up_cas2_unsave(ptr,oldval,newval);
}


inline 
bool up_tas( Mword *l )
{
  Mword tmp;
  asm volatile
    ("xchg %0,%1" : "=r" (tmp) : "m" (*l), "0" (1)
     : "memory");

  return tmp;
}

inline 
bool smp_tas( Mword *l )
{
  return up_tas(l);
}
 

INTERFACE:

#include "types.h"


IMPLEMENTATION[arch]:

inline
bool up_cas_unsave( Mword *ptr, Mword oldval, Mword newval )
{
  Mword ret;
  asm volatile ( "    mrs    r5, cpsr    \n"
		 "    mov    r6,r5       \n"
		 "    orr    r6,r6,#0x80 \n"
		 "    msr    cpsr_c, r6  \n"
		 "    \n"
		 "    ldr    r6, [%1]    \n"
		 "    cmp    r6, %2      \n"
		 "    streq  %3, [%1]    \n"
		 "    moveq  %0, #1      \n"
		 "    movne  %0, #0      \n"
		 "    msr    cpsr_c, r5  \n"
		 : "=r"(ret)
		 : "r"(ptr), "r"(oldval), "r"(newval)
		 : "r5", "r6", "memory"
		 );
  return ret;

}

inline
bool up_cas2_unsave( Mword *ptr, Mword *oldval, Mword *newval )
{
  Mword ret;
  asm volatile ( "    mrs    r5, cpsr    \n"
		 "    mov    r6,r5       \n"
		 "    orr    r6,r6,#128  \n"
		 "    msr    cpsr_c, r6  \n"
		 "    \n"
		 "    ldr    r6, [%1]    \n"
		 "    ldr    r7, [%1,#4] \n"
		 "    cmp    r6, %2      \n"
		 "    cmpeq  r7, %3      \n"
		 "    streq  %4, [%1]    \n"
		 "    streq  %5, [%1,#4] \n"
		 "    moveq  %0, #1      \n"
		 "    movne  %0, #0      \n"
		 "    msr    cpsr_c, r5  \n"
		 : "=r"(ret)
		 : "r"(ptr), "r"(*oldval), "r"(*(oldval+1)), "r"(*newval),
   		   "r"(*(newval+1))
		 : "r5", "r6", "r7", "memory"
		 );
  return ret;

}

inline 
bool smp_cas_unsave( Mword *ptr, Mword oldval, Mword newval )
{
  return up_cas_unsave(ptr,oldval,newval);
}

inline 
bool smp_cas2_unsave( Mword *ptr, Mword *oldval, Mword *newval )
{
  return up_cas2_unsave(ptr,oldval,newval);
}



bool up_tas( Mword *l )
{
  Mword ret;
  asm volatile ( "    swp %0, %2, [%1] \n "
		 : "=r"(ret)
		 : "r"(l), "r" (1)
		 : "memory"
		 );
  return ret;
}

inline
bool smp_tas( Mword *l )
{
  return up_tas(l);
}


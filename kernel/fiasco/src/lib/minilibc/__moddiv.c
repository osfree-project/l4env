
#include "moddiv.h"
__BEGIN_DECLS

_moddiv_t moddiv(unsigned long int a, unsigned long int b) 
{
  _moddiv_t r;
  unsigned long int tmp;
  asm volatile
    (
     "    mov ar.lc= 63                \n"
     "    mov %1= r0                   \n"
     "    sub %4= r0, %3               \n"
     "    mov %0= %2;;                 \n"
     "1:  tbit.nz p6,p0=%0,63          \n"
     "    shladd %1=%1,1,%4            \n"
     "    shl %0= %0,1;;               \n"
     "(p6)adds %1= 1,%1;;              \n"
     "    tbit.nz p6,p7=%1,63;;        \n"
     "(p6)add %1= %1, %3               \n"
     "(p7)or %0= 1, %0                 \n"
     "    br.cloop.dptk 1b;;           \n"
     : "=r"(r.div), "=r"(r.mod)
     : "r"(a), "r"(b), "r"(tmp)
     : "ar.lc", "p6", "p7"

     );
  
  return r;
}

unsigned long __udivdi3(unsigned long a, unsigned long b)
{
  unsigned long r,tmp,tmp2;
  asm volatile
    (
     "    mov ar.lc= 63                \n"
     "    mov %1= r0                   \n"
     "    sub %2= r0, %4               \n"
     "    mov %0= %3;;                 \n"
     "1:  tbit.nz p6,p0=%0,63          \n"
     "    shladd %1=%1,1,%2            \n"
     "    shl %0= %0,1;;               \n"
     "(p6)adds %1= 1,%1;;              \n"
     "    tbit.nz p6,p7=%1,63;;        \n"
     "(p6)add %1= %1, %4               \n"
     "(p7)or %0= 1, %0                 \n"
     "    br.cloop.dptk 1b;;           \n"
     : "=r"(r), "=r"(tmp2), "=r"(tmp)
     : "r"(a), "r"(b)
     : "ar.lc", "p6", "p7"

     );
  return r;
}


__END_DECLS


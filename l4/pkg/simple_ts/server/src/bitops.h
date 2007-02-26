#ifndef _L4_SIMPLE_TS_SERVER_BITOPS_H
#define _L4_SIMPLE_TS_SERVER_BITOPS_H

#define LOCK_PREFIX

extern inline int  find_first_zero_bit(void *addr, unsigned size);
extern inline void set_bit(int nr, volatile void *addr);
extern inline int  test_bit(int nr, volatile void *addr);
extern inline int  test_and_set_bit(int nr, volatile void *addr);
extern inline int  test_and_clear_bit(int nr, volatile void *addr);
  
extern inline int
find_first_zero_bit(void *addr, unsigned size)
{
  int d0, d1, d2;
  int res;
  if (!size)
    return 0;
  __asm__ __volatile__ ("cld			\n\t"
			"movl  $-1,%%eax	\n\t"
			"xorl  %%edx,%%edx	\n\t"
			"repe; scasl		\n\t"
			"je    1f		\n\t"
			"xorl  -4(%%edi),%%eax	\n\t"
			"subl  $4,%%edi		\n\t"
			"bsfl  %%eax,%%edx	\n\t"
			"1:    subl %%ebx,%%edi	\n\t"
			"shll  $3,%%edi		\n\t"
			"addl  %%edi,%%edx	\n\t"
			:"=d" (res), "=&c" (d0), "=&D" (d1), "=&a" (d2)
			:"1" ((size + 31) >> 5), "2" (addr), "b" (addr));
  return res;
}

struct __dummy { unsigned long a[100]; };
#define ADDR (*(volatile struct __dummy *) addr)

extern inline void
set_bit(int nr, volatile void *addr)
{
  __asm__ __volatile__ (LOCK_PREFIX "btsl %1, %0"
			:"=m" (ADDR) 
			:"Ir" (nr));
}

extern inline int
test_bit(int nr, volatile void *addr)
{
  int oldbit;

  __asm__ __volatile__ ("btl %2,%1\n\tsbbl %0,%0"
			:"=r" (oldbit)
			:"m" (ADDR), "Ir" (nr));
  return oldbit;
}

extern inline int
test_and_set_bit(int nr, volatile void *addr)
{
    int oldbit;

    __asm__ __volatile__( LOCK_PREFIX
			"btsl %2,%1\n\tsbbl %0,%0"
			:"=r" (oldbit),"=m" (ADDR)
			:"Ir" (nr));
    return oldbit;
}

extern inline int
test_and_clear_bit(int nr, volatile void *addr)
{
    int oldbit;

    __asm__ __volatile__( LOCK_PREFIX
			 "btrl %2,%1\n\tsbbl %0,%0"
		 	 :"=r" (oldbit),"=m" (ADDR)
			 :"Ir" (nr));
    return oldbit;
}

#endif /* _L4_SIMPLE_TS_SERVER_BITOPS_H */


IMPLEMENTATION [ia32,ux]:

PUBLIC static inline
Unsigned32
Cpu::muldiv (Unsigned32 val, Unsigned32 mul, Unsigned32 div)
{
  Unsigned32 dummy;

  asm volatile ("mull %3 ; divl %4\n\t"
               :"=a" (val), "=d" (dummy)
               : "0" (val),  "d" (mul),  "c" (div));
  return val;
}

PUBLIC static inline
Unsigned64
Cpu::ns_to_tsc (Unsigned64 ns)
{
  Unsigned32 dummy;
  Unsigned64 tsc;
  asm volatile
	("movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	:"=A" (tsc), "=&c" (dummy)
	: "0" (ns),  "b" (scaler_ns_to_tsc)
	);
  return tsc;
}

PUBLIC static inline
Unsigned64
Cpu::tsc_to_ns (Unsigned64 tsc)
{
  Unsigned32 dummy;
  Unsigned64 ns;
  asm volatile
	("movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	:"=A" (ns), "=&c" (dummy)
	: "0" (tsc), "b" (scaler_tsc_to_ns)
	);
  return ns;
}

PUBLIC static inline
Unsigned64
Cpu::tsc_to_us (Unsigned64 tsc)
{
  Unsigned32 dummy;
  Unsigned64 us;
  asm volatile
	("movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	:"=A" (us), "=&c" (dummy)
	: "0" (tsc), "S" (scaler_tsc_to_us)
	);
  return us;
}

PUBLIC static inline
void
Cpu::tsc_to_s_and_ns (Unsigned64 tsc, Unsigned32 *s, Unsigned32 *ns)
{
    Unsigned32 dummy;
    __asm__
	("				\n\t"
	 "movl  %%edx, %%ecx		\n\t"
	 "mull	%4			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%4			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "movl  $1000000000, %%ecx	\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	 "divl  %%ecx			\n\t"
	:"=a" (*s), "=d" (*ns), "=&c" (dummy)
	: "A" (tsc), "g" (scaler_tsc_to_ns)
	);
}

PUBLIC static inline
Unsigned64
Cpu::rdtsc (void)
{
  Unsigned64 tsc;
  asm volatile ("rdtsc" : "=A" (tsc));
  return tsc;
}

PUBLIC static inline
Unsigned32
Cpu::get_flags()
{ Unsigned32 efl; asm volatile ("pushfl ; popl %0" : "=rm"(efl)); return efl; }

PUBLIC static inline
void
Cpu::set_flags (Unsigned32 efl)
{ asm volatile ("pushl %0 ; popfl" : : "rm" (efl) : "memory"); }

PUBLIC static inline
Unsigned32
Cpu::get_es()
{ Unsigned32 val; asm volatile ("mov %%es, %0" : "=rm" (val)); return val; }

PUBLIC static inline
Unsigned32
Cpu::get_ss()
{ Unsigned32 val; asm volatile ("mov %%ss, %0" : "=rm" (val)); return val; }

PUBLIC static inline
Unsigned32
Cpu::get_fs()
{ Unsigned32 val; asm volatile ("mov %%fs, %0" : "=rm" (val)); return val; }

PUBLIC static inline
void
Cpu::set_ds (Unsigned32 val)
{ asm volatile ("mov %0, %%ds" : : "rm" (val)); }

PUBLIC static inline
void
Cpu::set_es (Unsigned32 val)
{ asm volatile ("mov %0, %%es" : : "rm" (val)); }

PUBLIC static inline
void
Cpu::set_fs (Unsigned32 val)
{ asm volatile ("mov %0, %%fs" : : "rm" (val)); }

PUBLIC static inline
void
Cpu::memset_mwords (void *dst, Mword value, size_t n)
{
  Mword dummy1, dummy2;
  asm volatile ("cld					\n\t"
		"repz stosl %%es:(%%edi)		\n\t"
		: "=c"(dummy1), "=D"(dummy2)
		: "a"(value), "c"(n), "D"(dst)
		: "memory");
}

PUBLIC static inline
void
Cpu::memcpy_bytes (void *dst, void const *src, size_t n)
{
  unsigned dummy1, dummy2, dummy3;

  asm volatile ("cld					\n\t"
                "repz movsl %%ds:(%%esi), %%es:(%%edi)	\n\t"
                "mov %%edx, %%ecx			\n\t"
                "repz movsb %%ds:(%%esi), %%es:(%%edi)	\n\t"
                : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
                : "c" (n >> 2), "d" (n & 3), "S" (src), "D" (dst)
                : "memory");
}

PUBLIC static inline
void
Cpu::memcpy_mwords (void *dst, void const *src, size_t n)
{
  unsigned dummy1, dummy2, dummy3;

  asm volatile ("cld					\n\t"
                "rep movsl %%ds:(%%esi), %%es:(%%edi)	\n\t"
                : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
                : "c" (n), "S" (src), "D" (dst)
                : "memory");
}

PUBLIC static inline
void
Cpu::memcpy_bytes_fs (void *dst, void const *src, size_t n)
{
  unsigned dummy1, dummy2, dummy3;

  asm volatile ("cld					\n\t"
                "rep movsl %%fs:(%%esi), %%es:(%%edi)	\n\t"
                "mov %%edx, %%ecx			\n\t"
                "repz movsb %%fs:(%%esi), %%es:(%%edi)	\n\t"
                : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
                : "c" (n >> 2), "d" (n & 3), "S" (src), "D" (dst)
                : "memory");
}

PUBLIC static inline
void
Cpu::memcpy_mwords_fs (void *dst, void const *src, size_t n)
{
  unsigned dummy1, dummy2, dummy3;

  asm volatile ("cld					\n\t"
                "rep movsl %%fs:(%%esi), %%es:(%%edi)	\n\t"
                : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
                : "c" (n), "S" (src), "D" (dst)
                : "memory");
}

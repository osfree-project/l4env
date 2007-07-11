IMPLEMENTATION [amd64]:

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
  Unsigned64 tsc, dummy;
  __asm__
      ("                              \n\t"
       "mulq	%3			\n\t"
       "shrd  $27, %%rdx, %%rax       \n\t"
       :"=a" (tsc), "=d" (dummy)
       :"a" (ns), "r" ((Unsigned64)scaler_ns_to_tsc)
      );
  return tsc;
}

PUBLIC static inline
Unsigned64
Cpu::tsc_to_ns (Unsigned64 tsc)
{
  Unsigned64 ns, dummy;
  __asm__
      ("				\n\t"
       "mulq	%3			\n\t"
       "shrd  $27, %%rdx, %%rax       \n\t"
       :"=a" (ns), "=d"(dummy)
       :"a" (tsc), "r" ((Unsigned64)scaler_tsc_to_ns)
      );
  return ns;
}

PUBLIC static inline
Unsigned64
Cpu::tsc_to_us (Unsigned64 tsc)
{
  Unsigned64 ns, dummy;
  __asm__
      ("				\n\t"
       "mulq	%3			\n\t"
       "shrd  $32, %%rdx, %%rax       \n\t"
       :"=a" (ns), "=d" (dummy)
       :"a" (tsc), "r" ((Unsigned64)scaler_tsc_to_us)
      );
  return ns;
}

PUBLIC static inline
void
Cpu::tsc_to_s_and_ns (Unsigned64 tsc, Unsigned32 *s, Unsigned32 *ns)
{
  __asm__
      ("				\n\t"
       "mulq	%3			\n\t"
       "shrd  $27, %%rdx, %%rax         \n\t"
       "xorq  %%rdx, %%rdx              \n\t"
       "divq  %4			\n\t"
       :"=a" (*s), "=&d" (*ns)
       : "a" (tsc), "r" ((Unsigned64)scaler_tsc_to_ns),
         "rm"(1000000000ULL)
      );
}

PUBLIC static inline
Unsigned64
Cpu::rdtsc (void)
{
  Unsigned64 tsc;
  asm volatile (
         "rdtsc				\n\t"
	 "and   %1,%%rax		\n\t"
	 "shl   $32,%%rdx		\n\t"
	 "or	%%rdx,%%rax		\n\t"
	: "=&a" (tsc)
	: "r" (0xffffffffUL)
	:"rdx"
	);
  return tsc;
}

PUBLIC static inline
Unsigned64
Cpu::get_flags()
{
  Unsigned64 efl;
  asm volatile ("pushf ; popq %0" : "=rm"(efl));
  return efl;
}

PUBLIC static inline
void
Cpu::set_flags (Unsigned64 efl)
{
  asm volatile ("pushq %0 ; popf" : : "rm" (efl) : "memory");
}

PUBLIC static inline
Unsigned32
Cpu::get_ss()
{
  Unsigned32 val;
  asm volatile ("movl %%ss, %0" : "=rm" (val));
  return val;
}

PUBLIC static inline
void
Cpu::set_gs (Unsigned32 val)
{
  asm volatile ("movl %0, %%gs" : : "rm" (val));
}

PUBLIC static inline
void
Cpu::memset_mwords (void *dst, Mword value, size_t n)
{
  Mword dummy1, dummy2;

  asm volatile ("cld					\n\t"
                "rep stosq (%%rdi)			\n\t"
                : "=c" (dummy1), "=D" (dummy2)
                : "a"(value), "c" (n), "D" (dst)
                : "memory");
}

PUBLIC static inline
void
Cpu::memcpy_bytes (void *dst, void const *src, size_t n)
{
  Mword dummy1, dummy2, dummy3;

  asm volatile ("cld					\n\t"
                "repz movsq (%%rsi), (%%rdi)		\n\t"
                "mov %%rdx, %%rcx			\n\t"
                "repz movsb (%%rsi), (%%rdi)		\n\t"
                : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
                : "c" (n >> 3), "d" (n & 7), "S" (src), "D" (dst)
                : "memory");
}

PUBLIC static inline
void
Cpu::memcpy_mwords (void *dst, void const *src, size_t n)
{
  Mword dummy1, dummy2, dummy3;

  asm volatile ("cld					\n\t"
                "rep movsq (%%rsi), (%%rdi)	\n\t"
                : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
                : "c" (n), "S" (src), "D" (dst)
                : "memory");
}

PUBLIC static inline
void
Cpu::memcpy_bytes_fs (void *dst, void const *src, size_t n)
{
  Mword dummy1, dummy2, dummy3;

  asm volatile ("cld					\n\t"
                "rep movsl (%%rsi), (%%rdi)	\n\t"
                "mov %%rdx, %%rcx			\n\t"
                "repz movsb (%%rsi), (%%rdi)	\n\t"
                : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
                : "c" (n >> 2), "d" (n & 3), "S" (src), "D" (dst)
                : "memory");
}

PUBLIC static inline
void
Cpu::memcpy_mwords_fs (void *dst, void const *src, size_t n)
{
  Mword dummy1, dummy2, dummy3;

  asm volatile ("cld					\n\t"
                "rep movsq (%%rsi), (%%rdi)	\n\t"
                : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
                : "c" (n), "S" (src), "D" (dst)
                : "memory");
}

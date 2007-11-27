
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/util/util.h>
#include <l4/util/rand.h>
#ifdef BENCH_x86
#include <l4/util/idt.h>
#endif

#include <stdio.h>
#include <setjmp.h>
#include <string.h>

#include "global.h"
#include "helper.h"

typedef l4_cpu_time_t (*memcpy_t)(char *dst, char const *src, l4_size_t size);

static unsigned char memcpy_stack[STACKSIZE] __attribute__((aligned(4096)));
static jmp_buf memcpy_jmp_buf;


static l4_cpu_time_t
memcpy_c_byte(char *dst, char const *src, l4_size_t size)
{
  l4_cpu_time_t start, stop;
  register char *d=dst;
  register const char *s=src;

  printf("   %-38s","memcpy_c_byte:");

  start = get_clocks();
  ++size;
  while (--size)
    {
      *d = *s;
      ++d; ++s;
    }
  stop = get_clocks();

  return stop - start;
}

static l4_cpu_time_t
memcpy_c_word(char *dst, char const *src, l4_size_t size)
{
  l4_cpu_time_t start, stop;
  register unsigned int *d = (unsigned int *)dst;
  register const unsigned int *s = (unsigned int *)src;

  printf("   %-38s","memcpy_c_word:");

  start = get_clocks();
  while (size > 4)
    {
      *d = *s;
      ++d; ++s;
      size -= 4;
    }
  stop = get_clocks();

  return stop - start;
}

static l4_cpu_time_t
memcpy_c_word_unroll(char *dst, char const *src, l4_size_t size)
{
  l4_cpu_time_t start, stop;
  register unsigned int *d = (unsigned int *)dst;
  register const unsigned int *s = (unsigned int *)src;

  printf("   %-38s","memcpy_c_word_unroll:");

  start = get_clocks();
  while (size > 16)
    {
      register unsigned int r1 = *(s + 0);
      register unsigned int r2 = *(s + 1);
      register unsigned int r3 = *(s + 2);
      register unsigned int r4 = *(s + 3);
      *(d + 0) = r1;
      *(d + 1) = r2;
      *(d + 2) = r3;
      *(d + 3) = r4;
      d += 4; s += 4;
      size -= 16;
    }
  while (size > 4)
    {
      *d = *s;
      ++d; ++s;
      size -= 4;
    }
  stop = get_clocks();

  return stop - start;
}

#ifdef BENCH_x86
static l4_cpu_time_t
memcpy_standard_rep_byte(char *dst, char const *src, l4_size_t size)
{
  l4_cpu_time_t start, stop;
  l4_mword_t dummy1, dummy2, dummy3;

  printf("   %-38s","memcpy_standard_rep_byte:");

  start = l4_rdtsc();
  asm volatile ("repz  movsb %%ds:(%%esi),%%es:(%%edi)\n\t"
		: "=S" (dummy1), "=D" (dummy2), "=c" (dummy3)
		:  "S" (src), "D" (dst), "c" (size)
		: "memory");
  stop = l4_rdtsc();

  return stop - start;
}

static l4_cpu_time_t
memcpy_standard_rep_word(char *dst, char const *src, l4_size_t size)
{
  l4_cpu_time_t start, stop;
  l4_mword_t dummy1, dummy2, dummy3;

  printf("   %-38s","memcpy_standard_rep_word:");

  start = l4_rdtsc();
  asm volatile ("repz  movsl %%ds:(%%esi),%%es:(%%edi)\n\t"
		: "=S" (dummy1), "=D" (dummy2), "=c" (dummy3)
		:  "S" (src), "D" (dst), "c" (size/4)
		: "memory");
  stop = l4_rdtsc();

  return stop - start;
}

static l4_cpu_time_t
memcpy_standard_rep_word_prefetch(char *dst, char const *src, l4_size_t size)
{
  l4_cpu_time_t start, stop;
  l4_mword_t dummy1, dummy2, dummy3;

  printf("   %-38s","memcpy_standard_rep_word_prefetch:");

  start = l4_rdtsc();
  asm volatile ("lea    (%%esi,%%edx,8),%%ebx     \n\t"
		"neg    %%edx                     \n\t"
		"0:                               \n\t"
		"mov    $(4096/32),%%ecx          \n\t"
		".align 8                         \n\t"
		"1:                               \n\t"
		"prefetchnta (%%ebx,%%edx,8)      \n\t"
		"add    $4,%%edx                  \n\t"
		"dec    %%ecx                     \n\t"
		"jnz    1b                        \n\t"
		"mov    $(4096/4),%%ecx           \n\t"
		"repz  movsl %%ds:(%%esi),%%es:(%%edi)\n\t"
		"or     %%edx,%%edx               \n\t"
		"jnz    0b                        \n\t"
		: "=S" (dummy1), "=D" (dummy2), "=d" (dummy3)
		:  "S" (src), "D" (dst), "d" (size/8)
		: "ebx", "ecx", "memory");
  stop = l4_rdtsc();

  return stop - start;
}

static l4_cpu_time_t
memcpy_standard_word(char *dst, char const *src, l4_size_t size)
{
  l4_cpu_time_t start, stop;
  l4_mword_t dummy1, dummy2, dummy3;

  printf("   %-38s","memcpy_standard_word:");

  start = l4_rdtsc();
  asm volatile ("sub  %%esi,%%edi                 \n\t"
	        ".align 16                        \n\t"
		"1:                               \n\t"
		"mov  (%%esi),%%eax               \n\t"
		"dec  %%ecx                       \n\t"
		"mov  %%eax,(%%esi,%%edi)         \n\t"
		"lea  4(%%esi),%%esi              \n\t"
		"jnz  1b                          \n\t"
		: "=S" (dummy1), "=D" (dummy2), "=c" (dummy3)
		:  "S" (src), "D" (dst), "c" (size/4)
		: "eax", "memory");
  stop = l4_rdtsc();

  return stop - start;
}

/* fast memcpy without using any special instruction */
static l4_cpu_time_t
fast_memcpy_standard(char *dst, const char *src, l4_size_t size)
{
  l4_cpu_time_t start, stop;
  l4_mword_t dummy;

  printf("   %-38s","memcpy_fast:");

  start = l4_rdtsc();
  asm volatile ("lea    (%%esi,%%edx,8),%%esi     \n\t"
	    	"lea    (%%edi,%%edx,8),%%edi     \n\t"
    		"neg    %%edx                     \n\t"
		".align 8                         \n\t"
		"0:                               \n\t"
		"mov   $(4096/64),%%ecx           \n\t"
	    	".align 16                        \n\t"
		"# block prefetch 4096 bytes      \n\t"
		"1:                               \n\t"
		"movl  (%%esi,%%edx,8),%%eax      \n\t"
		"movl  32(%%esi,%%edx,8),%%eax    \n\t"
		"add   $8,%%edx                   \n\t"
		"dec   %%ecx                      \n\t"
		"jnz   1b                         \n\t"
		"sub   $(4096/8),%%edx            \n\t"
		"mov   $(4096/8),%%ecx            \n\t"
		".align 16                        \n\t"
		"2: # copy 4096 bytes             \n\t"
		"movl  (%%esi,%%edx,8),%%eax      \n\t"
		"movl  4(%%esi,%%edx,8),%%ebx     \n\t"
		"movl  %%eax,(%%edi,%%edx,8)      \n\t"
		"movl  %%ebx,4(%%edi,%%edx,8)     \n\t"
		"inc   %%edx                      \n\t"
		"dec   %%ecx                      \n\t"
	        "jnz   2b                         \n\t"
		"or    %%edx,%%edx                \n\t"
		"jnz   0b                         \n\t"
		: "=d" (dummy)
		: "S" (src), "D" (dst), "d" (size/8)
		: "eax", "ebx", "ecx", "memory");
  stop = l4_rdtsc();

  return stop - start;
}

/* fast memcpy using movntq (moving using non-temporal hint)
 * cache line size is 32 bytes */
static l4_cpu_time_t
fast_memcpy_mmx2_32(char *dst, char const *src, l4_size_t size)
{
  l4_cpu_time_t start, stop;
  l4_mword_t dummy;

  printf("   %-38s","memcpy_fast_mmx2_32:");

  /* don't execute emms in side the timer loop because at this point the
   * fpu state is lazy allocated so we may have a kernel entry here */
  asm ("emms");

  start = l4_rdtsc();
  asm volatile ("lea    (%%esi,%%edx,8),%%esi     \n\t"
	    	"lea    (%%edi,%%edx,8),%%edi     \n\t"
    		"neg    %%edx                     \n\t"
		".align 8                         \n\t"
		"0:                               \n\t"
		"mov    $64,%%ecx                 \n\t"
		".align 16                        \n\t"
		"# block prefetch 4096 bytes      \n\t"
		"1:                               \n\t"
		"movl   (%%esi,%%edx,8),%%eax     \n\t"
		"movl   32(%%esi,%%edx,8),%%eax   \n\t"
		"add    $8,%%edx                  \n\t"
		"dec    %%ecx                     \n\t"
		"jnz    1b                        \n\t"
		"sub    $(32*16),%%edx            \n\t"
		"mov    $128,%%ecx                \n\t"
		".align 16                        \n\t"
		"2: # copy 4096 bytes             \n\t"
		"movq   (%%esi,%%edx,8),%%mm0     \n\t"
		"movq   8(%%esi,%%edx,8),%%mm1    \n\t"
		"movq   16(%%esi,%%edx,8),%%mm2   \n\t"
		"movq   24(%%esi,%%edx,8),%%mm3   \n\t"
		"movntq %%mm0,(%%edi,%%edx,8)     \n\t"
		"movntq %%mm1,8(%%edi,%%edx,8)    \n\t"
		"movntq %%mm2,16(%%edi,%%edx,8)   \n\t"
		"movntq %%mm3,24(%%edi,%%edx,8)   \n\t"
		"add    $4,%%edx                  \n\t"
		"dec    %%ecx                     \n\t"
		"jnz    2b                        \n\t"
		"or     %%edx,%%edx               \n\t"
		"jnz    0b                        \n\t"
		"sfence                           \n\t"
		"emms                             \n\t"
		: "=d" (dummy)
		: "S" (src), "D" (dst), "d" (size/8)
		: "eax", "ebx", "ecx", "memory");
  stop = l4_rdtsc();

  return stop - start;
}

/* Fast memcpy using movntq (moving using non-temporal hint)
 * cache line size is 64 bytes */
static l4_cpu_time_t
fast_memcpy_mmx2_64(char *dst, char const *src, l4_size_t size)
{
  l4_cpu_time_t start, stop;
  l4_mword_t dummy;

  printf("   %-38s","memcpy_fast_mmx2_64:");

  /* don't execute emms in side the timer loop because at this point the
   * fpu state is lazy allocated so we may have a kernel entry here */
  asm ("emms");

  start = l4_rdtsc();
  asm volatile ("lea    (%%esi,%%edx,8),%%esi     \n\t"
	    	"lea    (%%edi,%%edx,8),%%edi     \n\t"
    		"neg    %%edx                     \n\t"
		"0:                               \n\t"
		"mov    $32,%%ecx                 \n\t"
		".align 16                        \n\t"
		"# block prefetch 4096 bytes      \n\t"
		"1:                               \n\t"
		"movl   (%%esi,%%edx,8),%%eax     \n\t"
		"movl   64(%%esi,%%edx,8),%%eax   \n\t"
		"add    $16,%%edx                 \n\t"
		"dec    %%ecx                     \n\t"
		"jnz    1b                        \n\t"
		"sub    $(32*16),%%edx            \n\t"
		"mov    $128,%%ecx                \n\t"
		".align 16                        \n\t"
		"2: # copy 4096 bytes             \n\t"
		"movq   (%%esi,%%edx,8),%%mm0     \n\t"
		"movq   8(%%esi,%%edx,8),%%mm1    \n\t"
		"movq   16(%%esi,%%edx,8),%%mm2   \n\t"
		"movq   24(%%esi,%%edx,8),%%mm3   \n\t"
		"movntq %%mm0,(%%edi,%%edx,8)     \n\t"
		"movntq %%mm1,8(%%edi,%%edx,8)    \n\t"
		"movntq %%mm2,16(%%edi,%%edx,8)   \n\t"
		"movntq %%mm3,24(%%edi,%%edx,8)   \n\t"
		"add    $4,%%edx                  \n\t"
		"dec    %%ecx                     \n\t"
		"jnz    2b                        \n\t"
		"or     %%edx,%%edx               \n\t"
		"jnz    0b                        \n\t"
		"sfence                           \n\t"
		"emms                             \n\t"
		: "=d" (dummy)
		: "S" (src), "D" (dst), "d" (size/8)
		: "eax", "ebx", "ecx", "memory");
  stop = l4_rdtsc();

  return stop - start;
}

/* Fast memcpy using movntq (moving using non-temporal hint)
 * cache line size is 64 bytes */
static l4_cpu_time_t
fast_memcpy_sse(char *dst, char const *src, l4_size_t size)
{
  l4_cpu_time_t start, stop;
  l4_mword_t dummy;

  printf("   %-38s","memcpy_fast_sse:");

  /* don't execute emms in side the timer loop because at this point the
   * fpu state is lazy allocated so we may have a kernel entry here */
  asm ("emms");

  start = l4_rdtsc();
  asm volatile ("lea    (%%esi,%%edx,8),%%esi     \n\t"
	    	"lea    (%%edi,%%edx,8),%%edi     \n\t"
    		"neg    %%edx                     \n\t"
		"0:                               \n\t"
		"mov    $128,%%ecx                 \n\t"
		".align 16                        \n\t"
		"# block prefetch 4096 bytes      \n\t"
		"1:                               \n\t"
		"prefetchnta (%%esi,%%edx,8)      \n\t"
		"add    $4,%%edx                  \n\t"
		"dec    %%ecx                     \n\t"
		"jnz    1b                        \n\t"
		"sub    $(4096/8),%%edx           \n\t"
		"mov    $64,%%ecx                 \n\t"
		".align 16                        \n\t"
		"2: # copy 4096 bytes             \n\t"
		"movaps (%%esi,%%edx,8),%%xmm0    \n\t"
		"movaps 16(%%esi,%%edx,8),%%xmm1  \n\t"
		"movaps 32(%%esi,%%edx,8),%%xmm2  \n\t"
		"movaps 48(%%esi,%%edx,8),%%xmm3  \n\t"
		"movntps %%xmm0,(%%edi,%%edx,8)   \n\t"
		"movntps %%xmm1,16(%%edi,%%edx,8) \n\t"
		"movntps %%xmm2,32(%%edi,%%edx,8) \n\t"
		"movntps %%xmm3,48(%%edi,%%edx,8) \n\t"
		"add    $8,%%edx                  \n\t"
		"dec    %%ecx                     \n\t"
		"jnz    2b                        \n\t"
		"or     %%edx,%%edx               \n\t"
		"jnz    0b                        \n\t"
		"sfence                           \n\t"
		"emms                             \n\t"
		: "=d" (dummy)
		: "S" (src), "D" (dst), "d" (size/8)
		: "ecx", "memory");
  stop = l4_rdtsc();

  return stop - start;
}
#endif

static void
test_memcpy(memcpy_t memcpy)
{
  l4_size_t size = SCRATCH_MEM_SIZE/2;
  char *src = (char*)scratch_mem;
  char *dst = (char*)scratch_mem + size;
  l4_umword_t *m;
  l4_cpu_time_t time;

  for (m=(l4_umword_t*)scratch_mem;
       m<(l4_umword_t*)(scratch_mem+2*size/sizeof(l4_umword_t));
       m++)
    *m = l4util_rand();

  if (!setjmp(memcpy_jmp_buf))
    {
      time = memcpy(dst, src, size);

      if (0 != memcmp((void*)scratch_mem, (void*)scratch_mem+size, size))
	puts("Implementation error!");
      else
	{
#ifdef BENCH_x86
	  unsigned mb_s     = (unsigned)(((l4_uint64_t)SCRATCH_MEM_SIZE/2) / 
				         clocks_to_us(time));
	  unsigned cy1000_b = (unsigned)(1000*time/
				         ((l4_uint64_t)SCRATCH_MEM_SIZE/2));
	  unsigned cy_b     = cy1000_b / 1000;

	  cy1000_b -= 1000*cy_b;
	  printf("Memory (copy): %4uMB/s (%u.%03ucy/B)\n", 
	      mb_s, cy_b, cy1000_b);
#else
	  printf("Memory (copy): %4llukB/s\n",
	      ((l4_uint64_t)SCRATCH_MEM_SIZE/2)*1024 / clocks_to_us(time));
#endif
	}
    }
  else
    puts("Not applicable (invalid opcode)");
}

#ifdef BENCH_x86
void
exception6_c_handler(void)
{
  longjmp(memcpy_jmp_buf, 0);
}

void exception6_handler(void);
asm ("exception6_handler:          \n\t"
     "pusha                        \n\t"
     "call   exception6_c_handler  \n\t"
     "popa                         \n\t"
     "iret                         \n\t");
#endif

static void
test_mem_bandwidth_thread(void)
{
#ifdef BENCH_x86
  static struct
    {
      l4util_idt_header_t header;
      l4util_idt_desc_t   desc[0x20];
    } __attribute__((packed)) idt;

  l4util_idt_init (&idt.header, 0x20);
  l4util_idt_entry(&idt.header, 6, exception6_handler);
  l4util_idt_load (&idt.header);
#endif

  printf(">> m: Testing memory bandwidth (CPU %dMHz):\n", mhz);

  test_memcpy(memcpy_c_byte);
  test_memcpy(memcpy_c_word);
  test_memcpy(memcpy_c_word_unroll);
#ifdef BENCH_x86
  test_memcpy(memcpy_standard_rep_byte);
  test_memcpy(memcpy_standard_rep_word);
  test_memcpy(memcpy_standard_rep_word_prefetch);
  test_memcpy(memcpy_standard_word);
  test_memcpy(fast_memcpy_standard);
  test_memcpy(fast_memcpy_mmx2_32);
  test_memcpy(fast_memcpy_mmx2_64);
  test_memcpy(fast_memcpy_sse);
#endif

  call(main_id);

  l4_sleep_forever();
}

void
test_memory_bandwidth(int nr)
{
  callmode = 0;

  create_thread(memcpy_id,(l4_umword_t)test_mem_bandwidth_thread,
 	        (l4_umword_t)memcpy_stack + STACKSIZE,pager_id);

  /* wait until memcpy thread finished */
  recv(memcpy_id);
  send(memcpy_id);
}


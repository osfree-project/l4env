#include <stdio.h>
#include <stdlib.h>
#include <l4/sys/kdebug.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>

/* WARNING: malloc() is needed by libdl/ldso! */
const l4_ssize_t l4libc_heapsize = 64*1024;

void (*foo_in_library_1)(void) __attribute__((weak,nocommon));
void (*foo_in_library_2)(void) __attribute__((weak,nocommon));
void bar_in_binary(void);

void bar_in_binary(void)
{
  printf("This is output from bar_in_binary()\n");
}

extern void *dlopen(const char *fname, int flag);
extern void *dlsym(void *handle, const char *symbol);

int
main(int argc, char** argv)
{
  int error;
  void *addr;
  l4dm_dataspace_t ds;
  int i;
  void *handle1, *handle2, *func;

  printf("Hello World!\n");

  for (i=0; i<64; i++)
    {
      if ((error = l4dm_mem_open(L4DM_DEFAULT_DSM, L4_PAGESIZE, 0, 0, 
			        "dummy", &ds)))
	{
	  printf("Error %d allocating ds\n", error);
	  exit(-1);
	}
      if ((error = l4rm_attach(&ds, L4_PAGESIZE, 0, L4DM_RW, &addr)))
	{
	  printf("Error %d attaching ds\n", error);
	  exit(-1);
	}
      printf("%02d: Attached 4kB region to %08lx\n", i, (l4_addr_t)addr);
      memset(addr, i, L4_PAGESIZE);
    }

  printf("calling dl_open at %08lx\n", (l4_addr_t)dlopen);
  handle1 = dlopen("libsimple1_dyn.s.so", 2);
  printf("handle lib1=%08lx\n", (l4_addr_t)handle1);
  func    = dlsym(handle1, "foo_in_library_1");
  printf("func1=%08lx\n", (l4_addr_t)func);
  foo_in_library_1 = func;
  foo_in_library_1();

  handle2 = dlopen("libsimple2_dyn.s.so", 2);
  printf("handle lib2=%08lx\n", (l4_addr_t)handle2);
  func    = dlsym(handle2, "foo_in_library_2");
  printf("func2=%08lx\n", (l4_addr_t)func);
  foo_in_library_2 = func;
  foo_in_library_2();

  enter_kdebug("BEFORE EXIT");
  return 0;
}

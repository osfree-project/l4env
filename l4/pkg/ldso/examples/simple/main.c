#include <stdio.h>
#include <stdlib.h>
#include <l4/sys/kdebug.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>

void foo_in_library_1(void);
void foo_in_library_2(void);
void bar_in_binary(void);

static void __attribute__((constructor))
init(void)
{
  printf("This is output from main::init\n");
}

void
bar_in_binary(void)
{
  printf("This is output from bar_in_binary()\n");
}

int
main(int argc, char** argv)
{
  int error;
  void *addr;
  l4dm_dataspace_t ds;
  int i;

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
      printf("%02d: Attached 4kB region to %08x\n", i, (l4_addr_t)addr);
      memset(addr, i, L4_PAGESIZE);
    }

  foo_in_library_1();
  foo_in_library_2();

  printf("got %d arguments%s", argc, argc>0 ? ":" : "");
  for (i=0; i<argc; i++)
    printf("  argv[%d] = \"%s\"\n", i, argv[i]);

  enter_kdebug("BEFORE EXIT");
  return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <l4/thread/thread.h>
#include <l4/util/util.h>
#include <l4/sys/kdebug.h>
#include <l4/util/rdtsc.h>

static void
thread(void *id)
{
  printf("\033[31merrno_location[%d] = %08x\033[m\n", 
      (unsigned)id, (l4_addr_t)__errno_location());
  for (;;)
    {
      if (id == (void*)1)
	puts("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
      else
	puts("ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ");
    }
}

int
write(int fd, const void *buf, size_t count)
{
  const char *b = buf;
  size_t      c = count;

  if (fd == STDOUT_FILENO || fd == STDERR_FILENO)
    {
      while (c--)
	{
	  outchar(*b++);
	  l4_busy_wait_ns(1000000);
	}
      return count;
    }

  errno = EBADF;
  return -1;
}

int
main(void)
{
  l4_calibrate_tsc();
  l4thread_create(thread, (void*)1, L4THREAD_CREATE_ASYNC);
  l4thread_create(thread, (void*)2, L4THREAD_CREATE_ASYNC);
  l4_sleep_forever();
}

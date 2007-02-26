#include <names.h>
#include <l4/sys/syscalls.h>
#include <l4/util/util.h>

#include <l4/sys/kdebug.h>
extern int names_query_name(const char* name, l4_threadid_t* id);

int
names_waitfor_name(const char* name, l4_threadid_t* id,
		   const int timeout)
{
  int		ret;
  signed int	rem = timeout;
  signed int	to = 0;

  if (rem)
    to = 10;
  do
    {
      ret = names_query_name(name, id);
#if 0
      kd_display("ret="); outdec(ret); kd_display(" ");
      kd_display("rem="); outdec(rem); kd_display("\\r\\n");
#endif

      if ((rem == to) || (ret))
	return ret;
#if 0
      kd_display("to="); outdec(to); kd_display(" ");
#endif
      l4_sleep(to);

      rem -= to;
      if (to < 1024) /* 4s */
	to += to;
      if (to > rem)
	to = rem;
    } while (486);
};

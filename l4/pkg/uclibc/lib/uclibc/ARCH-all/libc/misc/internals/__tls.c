#ifdef L4_THREAD_SAFE

#include <stdlib.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/util/atomic.h>
#include <l4/util/bitops.h>

/* XXX key/data is not freed when thread is deleted. */
int * __tls_location (volatile int *tls_key, int tls_field[], int tls_used[])
{
  if (EXPECT_FALSE(*tls_key == -1))
    {
      /* have to allocate a field in the thread local storage pointer array */
      int key;
      
      if ((key = l4thread_data_allocate_key()) == -L4_ENOKEY)
	{
	  LOG("No free key for tls available");
	  exit(-1);
	}
      if (!l4util_cmpxchg(tls_key, -1, key))
	/* someone else was faster allocating a key */
	l4thread_data_release_key(key);
    }

  if (EXPECT_FALSE(l4thread_data_get_current(*tls_key) == 0))
    {
      /* current thread still does not have a pointer to tls */
      int bit;

      do
	{
	  if ((bit = l4util_find_first_zero_bit(tls_used, 128)) >= 128)
	    {
	      LOG("No free field in tls[] available");
	      exit(-1);
	    }
	}
      while (l4util_bts(bit, tls_used));
      
      l4thread_data_set_current(*tls_key, tls_field + bit);
    }

  return l4thread_data_get_current(*tls_key);
}

#endif

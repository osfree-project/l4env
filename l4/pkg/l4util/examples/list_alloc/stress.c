/* standard includes */
#include <string.h>

/* L4/L4Env includes */
#include <l4/util/rand.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/util/list_alloc.h>
#include <l4/dm_phys/dm_phys.h>

#define MAX_SIZE    (1 * 1024 * 1024)
#define MAX_ALLOCS  16

static l4_uint8_t   __malloc_pool[2 * MAX_ALLOCS * MAX_SIZE];
static l4la_free_t *__malloc_list;
static void * chunks[MAX_ALLOCS];

static void *
malloc(unsigned int size)
{
  void *a;
  l4_size_t *s;

  size += sizeof(l4_size_t);
  if (EXPECT_FALSE(!(a = l4la_alloc(&__malloc_list, size, 2))))
    {
      LOG("malloc memory exhausted");
      return 0;
    }
  s  = (l4_size_t*)a;
  *s = size;
  return s+1;
}

static void
free(void *ptr)
{
  l4_size_t *s = (l4_size_t*)ptr - 1;
  l4la_free(&__malloc_list, s, *s);
}

int
main(int argc, char * argv[])
{
  unsigned long size;
  int cur_allocs = 0;
  int num, i, j;

  memset(chunks, 0, MAX_ALLOCS * sizeof(void *));
  l4la_init(&__malloc_list);
  l4la_free(&__malloc_list, __malloc_pool, sizeof(__malloc_pool));

  while (1)
    {
      /* alloc some memory */
      num = (int) ((double)(MAX_ALLOCS - cur_allocs) * 
	  ((double)l4util_rand() / L4_RAND_MAX));
      LOG("allocate %d chunks, %d currently allocated", num, cur_allocs);

      for (i = 0; i < num; i++)
	{
	  do
	    {
	      size = (unsigned long)((double)MAX_SIZE * 
	  	  ((double)l4util_rand() / L4_RAND_MAX));
	    }
	  while (size == 0);

	  LOG("allocate %lu bytes", size);

	  // find free place to store pointer
	  j = 0;
	  while ((j < MAX_ALLOCS) && (chunks[j] != NULL))
	    j++;
	  Assert(j != MAX_ALLOCS);

	  // do alloc and check if != NULL
	  chunks[j] = malloc(size);
	  if (chunks[j] == NULL)
	    {
	      LOG_Error("allocation failed");
	      enter_kdebug("-");
	    }
	  else
	    cur_allocs++;
	}

      /* free some memory */
      num = (int)((double)cur_allocs * ((double)l4util_rand() / L4_RAND_MAX));
      LOG("free %d chunks of %d", num, cur_allocs);

      for (i = 0; i < num; i++)
	{
	  do
	    {
	      j = (int)((double)MAX_ALLOCS * ((double)l4util_rand() 
		    / L4_RAND_MAX));
	    }
	  while ((j >= MAX_ALLOCS) || (chunks[j] == NULL));

	  LOG("free chunk %d", j);

	  free(chunks[j]);
	  chunks[j] = NULL;

	  cur_allocs--;
	}
    }
}

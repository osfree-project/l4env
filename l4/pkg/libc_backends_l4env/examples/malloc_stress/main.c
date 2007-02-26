/* standard includes */
#include <string.h>
#include <stdlib.h>

/* L4/L4Env includes */
#include <l4/util/rand.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/dm_phys/dm_phys.h>

#define MAX_SIZE    (1 * 1024 * 1024)
#define MAX_ALLOCS  16

l4_ssize_t l4libc_heapsize = (2 * MAX_ALLOCS * MAX_SIZE);

void * chunks[MAX_ALLOCS];

int
main(int argc, char * argv[])
{
    unsigned long size;
    int cur_allocs = 0;
    int num, i, j;

    memset(chunks, 0, MAX_ALLOCS * sizeof(void *));

    while (1)
    {
        /* alloc some memory */
        num = (int) ((double)(MAX_ALLOCS - cur_allocs) * 
                     ((double)l4util_rand() / L4_RAND_MAX));
        LOG("allocate %d chunks, %d currently allocated", num, cur_allocs);

        for (i = 0; i < num; i++)
        {
            do
                size = (unsigned long)((double)MAX_SIZE * 
                                       ((double)l4util_rand() / L4_RAND_MAX));
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
                l4dm_ds_list_all(L4DM_DEFAULT_DSM);
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
                j = (int)((double)MAX_ALLOCS * ((double)l4util_rand() / L4_RAND_MAX));
            while ((j >= MAX_ALLOCS) || (chunks[j] == NULL));

            LOG("free chunk %d", j);

            free(chunks[j]);
            chunks[j] = NULL;

            cur_allocs--;
        }
    }
}

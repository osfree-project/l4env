/** 
 *  \file   l4/pkg/semaphore/examples/ulockflex/main.c
 *  \brief  Implements the Ulockflex benchmark
 *
 *  \date   20.03.2007
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <l4/util/cpu.h>
#include <l4/util/util.h>
#include <l4/util/rand.h>
//#include <l4/util/rdtsc.h>
#include <l4/util/getopt.h>
#include <l4/util/l4_macros.h>
#include <l4/util/atomic.h>
#include <l4/log/l4log.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/sigma0/kip.h>
#include <l4/sys/syscalls.h>

#include "lock_impl.h"
#include "__debug.h"


static inline unsigned long long
rdtsc(void)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  unsigned res[2];
  __asm__ __volatile__ ("xorl %%eax, %%eax; cpuid \n\t" : : : "ebx", "ecx", "edx", "eax" );
  __asm__ __volatile__ ("rdtsc \n\t" : "=a" (res[0]), "=d" (res[1]));
  __asm__ __volatile__ ("xorl %%eax, %%eax; cpuid \n\t" : : : "ebx", "ecx", "edx", "eax" );

  return *(unsigned long long*)res;
#else
  return 0;
#endif
}

struct lock_stats
{
    unsigned long lock;
    unsigned long long runs;
    unsigned long long hold, nohold;
    unsigned long long min_lock, max_lock;
    unsigned long long min_unlock, max_unlock;
    double mean_lock, dev_lock, mean_unlock, dev_unlock;
};

static long num_locks;
static long num_clients;
static long run_time = 60;
static long use_threads;
static long verbose;
//static double rounds_per_msec = 0.0;
static int running = 1;
static long long start_time;

#define STACK_SIZE 4096*4
static l4dm_dataspace_t stacks_ds_id;
static void* share_addr;
static unsigned long share_size;
static unsigned long stats_offset;

static l4_threadid_t master;

const int l4thread_max_threads = 128;

static void work(void)
{
    unsigned int a=0, b=0;
    memcpy(&a,&b,1);
}

typedef struct {
    l4_umword_t thread_nb;
    l4_umword_t lock_nb;
} thread_data_t;

static inline LOCK_TYPE *lock(unsigned long l)
{ return (LOCK_TYPE*)(share_addr + sizeof (l4_uint32_t) + l * LOCK_SIZE); }

static
struct lock_stats *
lock_stats(unsigned long client)
{
  return (struct lock_stats*)(share_addr + stats_offset) + client;
}

static void worker_thread(void* param)
{
    int i;
    thread_data_t *data = (thread_data_t*)param;
    l4_msgdope_t status;
    unsigned long runs = 0;
    unsigned long long diff, start;
    unsigned long long min_lock = -1ULL, max_lock = 0;
    unsigned long long min_unlock = -1ULL, max_unlock = 0;
    unsigned long long sum_lock = 0, sum_unlock = 0;
    unsigned long long sum_sq_lock = 0, sum_sq_unlock = 0;

    unsigned long long overhead = 0;

    l4_uint32_t *running = (l4_uint32_t*)share_addr;
    LOCK_TYPE *my_lock = lock(data->lock_nb);

    LOGd(DEBUG_WORKER, "started (running @%p is %d)", running, *running);
    LOGd(DEBUG_WORKER, "share_addr is %p", share_addr);
    LOGd(DEBUG_WORKER, "thread_data_t @ %p (%p)", param, data);
    LOGd(DEBUG_WORKER, "thread_nb is %ld", data->thread_nb);
    LOGd(DEBUG_WORKER, "lock_nb is %ld", data->lock_nb);
    LOGd(DEBUG_WORKER, "lock @%p", my_lock);

    while (!(*running))
	l4_sleep(1);

    for (i = 0; i < 1000; ++i)
      {
	unsigned long long s = rdtsc();
	overhead += rdtsc() - s;
      }

    overhead /= 1000;

    // test run, to ensure that we have everything in cache and mapped.
    printf("Client %ld started.\n", data->thread_nb);
    runs = 10000;
    while (runs--)
    {
	int nlht, lht;

	// compute nlht, lht
	nlht = ((double)l4util_rand()/(double)L4_RAND_MAX + 0.5) * 300;
	lht  = ((double)l4util_rand()/(double)L4_RAND_MAX + 0.5) * 300;

	// work for nlht
	for (i=0; i<nlht; i++)
	    work();
	// acquire lock
	start = rdtsc();
	LOCK_DOWN(my_lock);
	diff = rdtsc() - start -overhead;
	// work for lht
	for (i=0; i<lht; i++)
	    work();
	// release lock
	start = rdtsc();
	LOCK_UP(my_lock);
	diff = rdtsc() - start -overhead;
    }
    runs = 0;

    unsigned long d;
    l4_ipc_receive(master, 0, &d, &d, l4_timeout(L4_IPC_TIMEOUT_NEVER, l4_timeout_abs(start_time, L4_TIMEOUT_ABS_V16_s)), &status);



    while (*running)
    {
	int nlht, lht;

	runs++;
	// compute nlht, lht
	nlht = ((double)l4util_rand()/(double)L4_RAND_MAX + 0.5) * 300;
	lht  = ((double)l4util_rand()/(double)L4_RAND_MAX + 0.5) * 300;

	LOGd(DEBUG_WORKER_LOOP, "nlht:%d, lht:%d",nlht, lht);
#if defined (DO_WORK)
	// work for nlht
	for (i=0; i<nlht; i++)
	    work();
#endif

	LOGd(DEBUG_WORKER_LOOP, "get lock");

	// acquire lock
	start = rdtsc();
	LOCK_DOWN(my_lock);
	diff = rdtsc() - start - overhead;
	sum_lock += diff;
	if (min_lock > diff)
	    min_lock = diff;
	if (max_lock < diff)
	    max_lock = diff;
	sum_sq_lock += diff*diff;

	LOGd(DEBUG_WORKER_LOOP, "got lock, work");
#if defined (DO_WORK)
	// work for lht
	for (i=0; i<lht; i++)
	    work();
#endif

	LOGd(DEBUG_WORKER_LOOP, "release lock");

	// release lock
	start = rdtsc();
	LOCK_UP(my_lock);
	diff = rdtsc() - start -overhead;
	sum_unlock += diff;
	if (min_unlock > diff)
	    min_unlock = diff;
	if (max_unlock < diff)
	    max_unlock = diff;
	sum_sq_unlock += diff*diff;
    }

    struct lock_stats *stats = lock_stats(data->thread_nb-1);
    stats->lock = data->lock_nb;
    stats->runs = runs;
    stats->min_lock = min_lock;
    stats->min_unlock = min_unlock;
    stats->max_lock = max_lock;
    stats->max_unlock = max_unlock;
    stats->mean_lock = (long double)sum_lock / (double)runs;
    stats->mean_unlock = (long double)sum_unlock / (double)runs;
    stats->dev_lock = sqrt((runs*sum_sq_lock - sum_lock*sum_lock)/((double)runs*(runs-1)));
    stats->dev_unlock = sqrt((runs*sum_sq_unlock - sum_unlock*sum_unlock)/((double)runs*(runs-1)));


    // when finished, send message to master
    l4_ipc_send(master, L4_IPC_SHORT_MSG, 1, 1, L4_IPC_NEVER, &status);

    while(1) ;

}

#define MAX_THREADS 124
static thread_data_t thread_data[MAX_THREADS];

static void fork_threads(void)
{
    l4thread_t t;
    char name[8] = "work000";
    int i;

    if (num_clients > MAX_THREADS)
    {
	printf("Number of specified threads (%ld) too large for thread-test.\n",
	    num_clients);
	printf("Restricting number to %d.\n", MAX_THREADS);
	num_clients = MAX_THREADS;
    }

    // simple create the threads.
    for (i = 1; i <= num_clients; i++)
    {
	// create thread
	name[4] = '0' + i/100;
	name[5] = '0' + (i - i/100)/10;
	name[6] = '0' + (i - i/100)%10;
	thread_data[i-1].thread_nb = i;
	thread_data[i-1].lock_nb = (i-1) % num_locks;
	LOGd(DEBUG_WORKER,
	    "Create worker thread \"%s\" with nb %ld and lock %ld",
	    name, thread_data[i-1].thread_nb, thread_data[i-1].lock_nb);
	t = l4thread_create_named(worker_thread, name, (void*)(&thread_data[i-1]),
		L4THREAD_CREATE_ASYNC);
	if (l4_is_nil_id(l4thread_l4_id(t)))
	{
	    printf("Could not create thread %d\n", i);
	    break;
	}
    }
}

static void worker_task(void)
{
    l4_threadid_t m;
    l4_msgdope_t status;
    thread_data_t data;

    // wait for start IPC from master
    l4_ipc_wait(&m, L4_IPC_SHORT_MSG, &data.thread_nb, &data.lock_nb, L4_IPC_NEVER, &status);

    // ensure that shared memory is mapped
    l4_touch_rw(share_addr, share_size);
    
    int j;
    for (j = 0; j < num_locks; ++j)
      lock_rcv_map(lock(j)); 

    worker_thread((void*)&data);
    while (1);
}

static l4dm_dataspace_t share_ds_id;

static void pager_thread(void* p)
{
    l4_addr_t addr, eip;
    l4_threadid_t source;
    l4_msgdope_t status;
    l4_snd_fpage_t snd_fpage;

    l4_ipc_wait(&source, L4_IPC_SHORT_MSG, &addr, &eip,
	L4_IPC_NEVER, &status);

    while (1)
    {
        if (!running)
	  l4_sleep_forever();

	int rw = addr & 2;
	LOGd(DEBUG_PAGER, "PF from "l4util_idfmt" @%p (%lx) %s\n", l4util_idstr(source), 
 	    (void*)addr, status.raw, rw?"write":"read");

	addr &= ~3;

	if (rw)
	    l4_touch_rw((void*)addr, 4);
	else
	    l4_touch_ro((void*)addr, 4);

	// task had a page-fault, sent to us from kernel, which set the whole
	// address space as receive window. Therefore, set the snd_base
	// correctly.
	snd_fpage.fpage = l4_fpage(addr, L4_LOG2_PAGESIZE, 
	    rw ? L4_FPAGE_RW : L4_FPAGE_RO, L4_FPAGE_MAP);
	snd_fpage.snd_base = addr & L4_PAGEMASK;

	l4_ipc_reply_and_wait(source, L4_IPC_SHORT_FPAGE, snd_fpage.snd_base, 
	    snd_fpage.fpage.raw, &source, L4_IPC_SHORT_MSG, &addr, &eip, 
	    L4_IPC_NEVER, &status);
    }

}

/** 
 * \brief start the clients as tasks
 *
 * To keep it simple, we use our own pager to page the new tasks.
 */
static void fork_tasks(void)
{
    int i;
    l4thread_t tp;
    l4_taskid_t pager;
    l4_msgdope_t status;
    char name[8] = "work000";
    unsigned char* stacks;

    // start pager thread in out address space and use it for new tasks
    tp = l4thread_create(pager_thread, 0, L4THREAD_CREATE_ASYNC);
    pager = l4thread_l4_id(tp);
    // allocate memory for the stacks
    stacks = l4dm_mem_ds_allocate(num_clients * STACK_SIZE,
	L4DM_CONTIGUOUS | L4RM_MAP | L4RM_LOG2_ALIGNED | L4RM_LOG2_ALLOC,
	&stacks_ds_id);
    // start tasks
    for (i = 1; i <= num_clients; i++)
    {
	l4_taskid_t t;
	// allocate task id
	if (l4ts_allocate_task(&t))
	{
	    printf("Could not allocate task %d\n", i);
	    break;
	}
	// create task
	l4_umword_t s = (l4_umword_t)(stacks + STACK_SIZE * i);
	name[4] = '0' + i/100;
	name[5] = '0' + (i - i/100)/10;
	name[6] = '0' + (i - i/100)%10;
	if (l4ts_create_task(&t, (l4_addr_t)(&worker_task), 
		s, 255, &pager, 0xa, name, 0))
	{
	    printf("Could not create task %d\n", i);
	    break;
	}
	// send startup IPC
	printf("send startup to %x.%x\n", t.id.task, t.id.lthread);
	l4_ipc_send(t, L4_IPC_SHORT_MSG, i, (i-1)%num_locks, L4_IPC_NEVER, &status);
	printf("  result = %lx\n", status.raw);
	int j;
	for (j = 0; j < num_locks; ++j)
	  lock_map_to(t, lock(j)); 
    }
}

/**
 * \brief print usage of tool
 */
static void usage(void)
{
    printf("ulockflex -c <num-clients> -l <num-locks> [ -r <run-time> ] [ -t ]\n");
    printf("    num-clients: how many client should compete for locks\n");
    printf("    num-locks:   number of locks\n");
    printf("    run-time:    time to let test run in s (default: 60)\n"); 
    printf("    -t:          use threads instead of tasks\n");
}

/**
 * \brief parse the tool's argument list
 * \param argc the number of arguments
 * \param argv the argument strings
 */
static void parse_args(int argc, char* argv[])
{
    char c;

    static struct option long_options[] =
    {
	{"locks",       1, 0, 'l'},
	{"clients",     1, 0, 'c'},
	{"runtime",     1, 0, 'r'},
	{"threads",     0, 0, 't'},
	{"verbose",     0, 0, 'v'},
	{0, 0, 0, 0}
    };

    /* read command line arguments */
    while (1)
    {
	c = getopt_long(argc, argv, "l:c:r:tv", long_options, NULL);

	if (c == (char) -1)
	    break;

	switch (c)
	{
	case 'l':
	    num_locks = strtol(optarg, 0, 10);
	    break;

	case 'c':
	    num_clients = strtol(optarg, 0, 10);
	    break;

	case 'r':
	    run_time = strtol(optarg, 0, 10);
	    break;

	case 't':
	    use_threads = 1;
	    break;

	case 'v':
	    verbose = 1;
	    break;

	default:
	    printf("Invalid option: %c\n", c);
	}
    }
}

/**
 * \brief setup the shared memory region for the semaphores
 * 
 * This function simply aquires a dataspace from the dataspace manager. It
 * calculates its size from the number of locks and their size.
 *
 * We allocate the first lock to synchronize the startup of the workers.
 */
static void setup_shared(void)
{
    stats_offset = LOCK_SIZE * (num_locks + 1);
    share_size = stats_offset + num_clients * sizeof(struct lock_stats);
    // open dataspace
    share_addr = l4dm_mem_ds_allocate(share_size,
	L4DM_CONTIGUOUS | L4RM_MAP | L4RM_LOG2_ALIGNED | L4RM_LOG2_ALLOC, 
	&share_ds_id);

    printf("  Shared memory @%p ds=%d\n", (void*)share_addr, share_ds_id.id);
    if (!share_addr)
    {
	printf("error, could not open dataspace of size %ld\n", share_size);
	return;
    }
    // touch memory to be sure its mapped
    l4_touch_rw(share_addr, share_size);
}


/**
 * \brief init the locks
 *
 * We iterate the locks in the shared memory region and call the appropriate
 * init function.
 *
 * We also reset the running variable.
 */
static void init_locks(void)
{
    int i;

    LOGd(DEBUG_INIT, "share_addr is %p", share_addr);

    for (i = 0; i < num_locks; i++)
    {
	LOGd(DEBUG_INIT, "lock %d @%p", i, lock(i));
	LOCK_INIT(lock(i));
    }

    // running variable
    *(l4_uint32_t*)share_addr = 0;
}

static void free_locks(void)
{
    int i;

    for (i = 0; i < num_locks; i++)
    {
	LOGd(DEBUG_CLEANUP, "lock %d @%p", i, lock(i));
	free_lock(lock(i));
    }

    // running variable
    *(l4_uint32_t*)share_addr = 0;
}


#if 0
/**
 * \brief calibrate run to determine factor for work loops
 * \param secs the number of seconds to run the calibration test
 * \return a factor of how many times the work() function is called per ms
 */
static double calibrate(int secs)
{
    /* figure out how many loops we can execute per micro second */
    int i;
    int count = 0;
    unsigned long n_initial = 100000;
    unsigned long clock1, clock2, clockterm;

    l4_tsc_init(L4_TSC_INIT_AUTO);

    clock1 = l4_rdtsc_32();
    clockterm = clock1 + l4_ns_to_tsc((unsigned long long)secs * 1000000);
    do {
	for(i=0; i<n_initial; i++)
	{
	    work();
	}
	clock2 = l4_rdtsc_32();
	count++;
    } while (clock2 < clockterm);

    n_initial *= count; // how many times was work() called

    return (double)n_initial / (double)(l4_tsc_to_us(clock2 - clock1));
}
#endif

int main(int argc, char* argv[])
{
    int i;
    stacks_ds_id = L4DM_INVALID_DATASPACE;
    share_ds_id  = L4DM_INVALID_DATASPACE;

    l4sigma0_kip_map(L4_INVALID_ID);

    // options:
    // -l # - number of locks
    // -c # - number of clients
    // -t   - use threads instead of tasks
    // -r # - runtime
    parse_args(argc, argv);
    if (num_locks == 0 && num_clients == 0)
    {
	usage();
	return 1;
    }

    // setup shared memory
    setup_shared();

    // store my id
    master = l4_myself();

    // init the locks
    init_locks();
#if 0
    // calibrate ticks
    rounds_per_msec = calibrate(2);
#endif

    printf("Starting clients.\n");
    start_time = l4sigma0_kip()->clock + 10000000;
    // fork workers
    if (use_threads)
	fork_threads();
    else
	fork_tasks();

    // start test run
    l4_sleep(10);
    printf("Starting test. It will run for %ld seconds!\n", run_time);
    l4util_xchg32((l4_uint32_t*)share_addr, 1);
    // wait a little
    l4_sleep(run_time*1000);
    l4util_xchg32((l4_uint32_t*)share_addr, 0);

    // wait for completion
    for (i = 1; i <= num_clients; i++)
    {
	l4_threadid_t t;
	l4_msgdope_t status;
	l4_umword_t d1, d2;

	LOGd(DEBUG_CLEANUP, "wait for Client %d to finish", i);
	l4_ipc_wait(&t, L4_IPC_SHORT_MSG, &d1, &d2, L4_IPC_NEVER, &status);
    }


    running = 0; // stop pager_thread
    free_locks();

    for (i = 0; i < num_clients; ++i)
      {
	struct lock_stats *s = lock_stats(i);

	// print statistics
	printf("Client %d: %lld runs.\n", i+1, s->runs);
	printf("LOCK(%ld): mean %04.2f, dev %04.2f, min %llu, max %llu.\n",
	    s->lock, s->mean_lock, s->dev_lock, s->min_lock, s->max_lock);
	printf("UNLOCK(%ld): mean %04.2f, dev %04.2f, min %llu, max %llu.\n",
	    s->lock, s->mean_unlock, s->dev_unlock, s->min_unlock, s->max_unlock);
      }

    
    // cleanup
    if (!l4dm_is_invalid_ds(share_ds_id))
	l4dm_close(&share_ds_id);
    if (!l4dm_is_invalid_ds(stacks_ds_id))
	l4dm_close(&stacks_ds_id);

    return 0;
}

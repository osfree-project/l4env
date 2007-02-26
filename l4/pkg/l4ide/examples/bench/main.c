/*****************************************************************************/
/**
 * \file   l4ide/examples/bench/main.c
 * \brief  L4IDE driver benchmark based on DROPS SCSI driver benchmark application
 *
 * \date   01/27/2004
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* standard includes */
#include <stdlib.h>
#include <string.h>

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/sys/kernel.h>
#include <l4/log/l4log.h>
#include <l4/sigma0/kip.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/generic_blk/blk.h>
#include <l4/util/rand.h>
#include <l4/util/rdtsc.h>
#include <l4/util/macros.h>
#include <l4/util/getopt.h>
#include <l4/env/mb_info.h>

#include <driver/ctrl.h>

#include "main.h"

#define NUM             10001
#define USE_PHYS_ADDR	1

char LOG_tag[9] = "l4idebe";
l4_ssize_t l4libc_heapsize = 8*1024*1024;


static int BLK_SIZE  =  2*1024;
static int MAX_BLOCK =  1000;     /* set to actual disk size later */

#define RANDOM          1
#define WRITE           0
#define DEVICE          0x300000	/* this is the devices major/minor value
					 * as computed by Linux. This way we get
					 * 0x0300000 for /dev/hda and
					 * 0x1600000 for /dev/hdc
					 */

#define DUMP_DIST       0

/* number of simultaneous requests */
#define NUM_REQUESTS    4
#define DEBUG_BUFFER    0

#define MEASURE_SINGLE  0

typedef struct req_desc
{
  void *         buf;
  l4_addr_t      p_addr;
  l4dm_dataspace_t ds;
  int            done;
} req_desc_t;

/* benchmark */
unsigned long next_request = 0;
unsigned long next_block = 0;
unsigned long total = 0;

l4blk_driver_t driver;
static double ns_per_cycle;
l4_kernel_info_t * kinfo;
l4semaphore_t done_wait = L4SEMAPHORE_LOCKED;
l4semaphore_t next_done = L4SEMAPHORE_LOCKED;

l4_cpu_time_t s_start,s_end;
unsigned long s_num;
l4_cpu_time_t s_total;

// for pattern import
char * pattern_name;
void * pattern_start = NULL;
void * pattern_end = NULL;
disc_request_t * start_request = NULL;

/* prototypes */
void
done(l4blk_request_t * request, int status, int error);


/*****************************************************************************/
/**
 * \brief Map L4 kernel info page
 *	
 * \return 0 on success (mapped kernel info page), -1 on error
 *
 * Map L4 kernel info page, we use the L4 kernel timer as common time base.
 */
/*****************************************************************************/ 
static void
__map_kernel_info_page(void)
{
#if 0
  l4_threadid_t chief;
  int error;
  l4_umword_t dummy;
  l4_msgdope_t result;
  l4_uint32_t area;
  l4_addr_t addr;

  l4_nchief(L4_INVALID_ID,&chief);

  /* reserve map area */
  error = l4rm_area_reserve(4096,0,&addr,&area);
  if (error)
    {
      Panic("no area available to map kernel info page (%d)\n",error);
      return;
    }
  LOG_printf("mapping kernel info page to 0x%08lx\n",addr);
  
  /* map kernel info page */
  error = l4_ipc_call(chief,L4_IPC_SHORT_MSG,1,1,
                      L4_IPC_MAPMSG(addr,L4_LOG2_PAGESIZE),
                      &dummy,&dummy,L4_IPC_NEVER,&result);
  if (error)
    {
      Panic("error calling pager: 0x%02x",error);
      return;
    }
  
  if ((!l4_ipc_fpage_received(result)) ||
      (((l4_kernel_info_t *)addr)->magic != L4_KERNEL_INFO_MAGIC))
    {
      Panic("error mapping kernel info page");
      return;
    }
  
  /* done */
  kinfo = (l4_kernel_info_t *)addr;
#else
  kinfo = l4sigma0_kip_map(L4_INVALID_ID);
#endif
}

/*****************************************************************************/
/**
 * \brief Allocate buffer.
 * 
 * \param  size          Size
 * \param  req           Request descriptor
 *
 * \return 0 on success (allocated buffer), -1 otherwise
 */
/*****************************************************************************/ 
static int
alloc_buffer(l4_size_t size, req_desc_t * req)
{
  int ret;
  l4_size_t psize;

  /* allocate memory */
  req->buf = l4dm_mem_ds_allocate(size,L4DM_CONTIGUOUS | L4RM_MAP,&req->ds);
  if (req->buf == NULL)
    {
      Panic("memory allocation failed");
      return -1;
    }

#if USE_PHYS_ADDR
  /* get phys. address */
  ret = l4dm_mem_ds_phys_addr(&req->ds,0,size,&req->p_addr,&psize);
  if ((ret < 0) || (psize != size))
    {
      Panic("get phys. address failed\n");
      return -1;
    }
#else
  ret = l4dm_share(&req->ds, l4blk_get_driver_thread(driver), L4DM_RW);
  if (ret < 0)
    LOG_Error("share dataspace with driver failed\n");
#endif

#if DEBUG_BUFFER
  LOG("ds %d at "l4util_idfmt,req->ds.id,l4util_idstr(req->ds.manager));
  LOG("phys 0x%08x, map 0x%08x",req->p_addr,addr);
#endif

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Generate next request.
 * 
 * \param  desc          Request descriptor
 */
/*****************************************************************************/ 
static void
do_next_request(l4blk_request_t * request)
{
  req_desc_t * desc = (req_desc_t *)request->data;
#if USE_PHYS_ADDR
  l4blk_sg_phys_elem_t sg;
#else
  l4blk_sg_ds_elem_t sg;
#endif
  int ret;

while (1) {
  // check for end
  if (next_request++ >= NUM) return;

  l4semaphore_down(&next_done);
  l4thread_sleep(3);

  /* setup request structure */
  request->driver = driver;
  request->request.device = DEVICE;
  request->request.count = BLK_SIZE >> 10;
  request->request.stream = L4BLK_INVALID_STREAM;	      

  /* buffer */
#if USE_PHYS_ADDR
  sg.addr = desc->p_addr;
  sg.size = BLK_SIZE;

  request->sg_list = &sg;
  request->sg_num = 1;
  request->sg_type = L4BLK_SG_PHYS;
#else
  sg.ds = desc->ds;
  sg.size = BLK_SIZE;
  sg.offs = 0;

  request->sg_list = &sg;
  request->sg_num = 1;
  request->sg_type = L4BLK_SG_DS;
#endif

  request->wait = NULL;
  request->done = done;
  request->data = desc;

#if WRITE
  request->request.cmd = L4BLK_REQUEST_WRITE;
#else
  request->request.cmd = L4BLK_REQUEST_READ;
#endif

#if RANDOM
  request->request.block = 
    (l4_uint32_t)((double)MAX_BLOCK * 
                  ((double)l4util_rand() / (double)L4_RAND_MAX));
  request->request.block &= ~((BLK_SIZE >> 10) - 1);
#else
  request->request.block = next_block;
  next_block += (BLK_SIZE >> 10)+2048;
  if (next_block > MAX_BLOCK)
    next_block = 0;
#endif
  
#if MEASURE_SINGLE
  s_start = l4_rdtsc();
#endif
  
  /* send request to driver */
  ret = l4blk_put_request(request);
  if (ret)
    Panic("l4blk_put_requests failed: %d",ret);
 }
}

/*****************************************************************************/
/**
 * \brief Request done function
 * 
 * \param  request       Block request structure
 * \param  status        Request status
 * \param  error         Error code
 */
/*****************************************************************************/ 
void
done(l4blk_request_t * request, int status, int error)
{
  if (total<NUM) {
    total++;
    switch(status)
      {
      case L4BLK_DONE:
        /* do nothing */
        break;
      
      case L4BLK_SKIPPED:
      case L4BLK_ERROR:
      case L4BLK_UNPROCESSED:
        LOG_Error("request failed (status %02x, error %d)!",status,error);
        break;
      
      default:
        LOG_Error("invalid request status: %08x",status);
      }
  
#if MEASURE_SINGLE
      if (total>1) {
        s_end = l4_rdtsc();

        s_total += (s_end - s_start);
        s_num++;
      }
#endif
  }
  
  if (total>=NUM) {
    /* wakeup main */
    l4semaphore_up(&done_wait);
    return;
  } else {
    /* do next request */
    l4semaphore_up(&next_done);
  }
}

/*****************************************************************************/
/**
 * \brief Benchmark
 */
/*****************************************************************************/ 
static void
bench(l4blk_driver_t driver)
{
  int ret,i;
  req_desc_t desc[NUM_REQUESTS];
  l4blk_request_t requests[NUM_REQUESTS];
  l4_cpu_time_t t_start,t_end;
  l4_uint32_t mus; 
  double bw;
	
  /* allocate buffers */
  for (i = 0; i < NUM_REQUESTS; i++)
    {
      /* alloc buffer */
      ret = alloc_buffer(BLK_SIZE,&desc[i]);
      if (ret)
				Panic("buffer allocation failed!");
			

      /* fill buffer */
      memset(desc[i].buf,l4util_rand(),BLK_SIZE);
      requests[i].data = &desc[i];
    }
	
  total = 0;
  t_start = l4_rdtsc();
	
  s_total = 0;
  s_num = 0;

  l4util_srand(1);	

  for (i = 0; i < NUM_REQUESTS; i++)
    l4semaphore_up(&next_done);
	
  for (i = 0; i < NUM_REQUESTS; i++)
    do_next_request(&requests[i]);
	
  /* wait for benchmark end */
  l4semaphore_down(&done_wait);
	
  /* done */
  t_end = l4_rdtsc();
  mus = (l4_uint32_t)(((t_end - t_start) * ns_per_cycle) / 1000.0);
  bw = ((double)total * BLK_SIZE);
	
  LOG("%lu bytes",(unsigned long)bw);
  
  bw /= ((double)mus / 1000.0 / 1000.0);
  bw /= 1024.0;
  
  if (WRITE)
    {
      if (RANDOM)
				LOG("%lu requests total (%u kbyte blocks, write, random), %2u.%03ums",
						 total,BLK_SIZE >> 10,mus / 1000, mus % 1000);
      else
				LOG("%lu requests total (%u kbyte blocks, write, linear), %2u.%03ums",
						 total,BLK_SIZE >> 10,mus / 1000, mus % 1000);
    }
  else
    {
      if (RANDOM)
				LOG("%lu requests total (%u kbyte blocks, read, random), %2u.%03ums",
						 total,BLK_SIZE >> 10,mus / 1000, mus % 1000);
      else
				LOG("%lu requests total (%u kbyte blocks, read, linear), %2u.%03ums",
						 total,BLK_SIZE >> 10,mus / 1000, mus % 1000);
    }
	
  LOG("bandwidth: %u.%03u KB/s",(unsigned)bw,
       (unsigned)((bw - (unsigned)bw) * 1000.0));
	
#if MEASURE_SINGLE
  LOG("%lu kcycles in %lu requests", (unsigned long)(s_total/(unsigned long long)1000), s_num);
#else
  LOG("%lu kcycles in %lu requests", (unsigned long)((t_end - t_start)/(unsigned long long)1000), total);
#endif
  
  LOG_flush();
}

/*****************************************************************************/
/**
 * \brief Main function
 */
/*****************************************************************************/ 
int
main(int argc, char *argv[])
{
  int ret, i, m;
  double mhz;
#if PATTERN
  disc_request_t * temp;
  int max_blk_size = -1;
#endif

  /* init block device driver lib */
  l4blk_init();
  
  /* map L4 kernel info page */
  __map_kernel_info_page();
  
  /* init random number generator */
  l4util_srand(1); // Init with l4_rdtsc_32() for 'real' random behaviour
  
  /* init time measuring */
  l4_calibrate_tsc();
  ns_per_cycle = l4_tsc_to_ns(10000000ULL) / 10000000.0;
  mhz = 1000.0 / ns_per_cycle;
  LOG_printf("running on a %u.%02u MHz machine (%u.%03u ns/cycle)\n",
         (unsigned)mhz,(unsigned)((mhz - (unsigned)mhz) * 100),
         (unsigned)ns_per_cycle,
         (unsigned)((ns_per_cycle - (unsigned)ns_per_cycle) * 1000));
  
  /* open driver */
  ret = l4blk_open_driver("L4IDE",&driver, NULL);
  if (ret < 0)
    {
      Panic("failed to open driver (%d)\n",ret);
      return -1;
    }
  LOG("driver handle: %d",driver);
  
  /* get actual disk size */
  MAX_BLOCK = l4blk_ctrl_get_disk_size(driver,DEVICE);
  if (MAX_BLOCK < 0)
    {
      Panic("failed to get disk size (%d)",MAX_BLOCK);
      l4blk_close_driver(driver);
      return -1;
    }
  LOG("max block: %d",MAX_BLOCK);
  
  /* get actual disk count */
  ret = l4blk_ctrl_get_num_disks(driver);
  if (ret < 0)
    {
      Panic("failed to get disk count (%d)",ret);
      l4blk_close_driver(driver);
      return -1;
    }
  LOG("disk count: %d",ret);

  /* get disk names */
  if ((m = ret)) {
      for (i = 0; i < m; i++) {
	  ret = l4blk_ctrl(driver, L4IDE_CTRL_DISK_NAME, &i, sizeof(i), NULL, 0);
	  if (ret < 0)
	    {
	      Panic("failed to get disk count (%d)",ret);
	      l4blk_close_driver(driver);
	      return -1;
	    }
	  LOG("disk %i's name: %x", i, ret);
      }
  }
  
  /* do benchmark */
  bench(driver);
  
  l4thread_sleep(2000);
  l4blk_close_driver(driver);
  LOG("done");
  
  /* done */
  return 0;
}

/* $Id$ */
/*****************************************************************************/
/**
 * \file   scsi/examples/bench/main.c
 * \brief  DROPS SCSI driver benchmark application
 *
 * \date   02/14/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4/DROPS includes */
#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/util/rand.h>
#include <l4/util/rdtsc.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>

/* generic block / examples include */
#include <l4/generic_blk/blk.h>
#include "blksrv.h"

/*****************************************************************************
 *** benchmark config
 *****************************************************************************/

/* driver name */
#define DRIVER_NAME     NAMES_OSKITBLK

/* number of requests for benchmark */
#define NUM             1000

static int BLK_SIZE  =  65536;
static int MAX_BLOCK =  1000;     /* set to actual disk size later */

#define RANDOM          0
#define WRITE           0
#define DEVICE          0x00

/* number of simultaneous requests */
#define NUM_REQUESTS    64

/* use dataspace id in block request */
#define DS_REQUEST      1

/* measure execution time of each request */
#define MEASURE_SINGLE  1

/*****************************************************************************
 *** global benchmark stuff
 *****************************************************************************/

/* request / buffer descriptor */
typedef struct req_desc
{
  l4dm_dataspace_t  ds;     /* dataspace id */
  l4_size_t         size;   /* buffer size */
  void *            buf;    /* mapp address */
  l4_addr_t         p_addr; /* phys. address */
  
  int               done;
} req_desc_t;

/* benchmark */
unsigned long next_request = 0;
unsigned long next_block = 0;
unsigned long total;

l4blk_driver_t driver;
static double ns_per_cycle;
l4semaphore_t done_wait = L4SEMAPHORE_LOCKED;

l4_cpu_time_t s_start,s_end;
unsigned long s_num;
l4_cpu_time_t s_total;

/* prototypes */
void
done(l4blk_request_t * request, int status, int error);

/* debugging */
#define DEBUG_BUFFER    0

/*****************************************************************************
 *** other global stuff
 *****************************************************************************/

char LOG_tag[9] = "blkbench";
l4_ssize_t l4libc_heapsize = 16*1024*1024;

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
  req->buf = l4dm_mem_ds_allocate(size, L4DM_CONTIGUOUS | L4RM_MAP, &req->ds);
  if (req->buf == NULL)
    {
      Panic("memory allocation failed");
      return -1;
    }

  /* get phys. address */
  ret = l4dm_mem_ds_phys_addr(&req->ds, 0, size, &req->p_addr, &psize);
  if ((ret < 0) || (psize != size))
    {
      Panic("get phys. address failed\n");
      l4dm_mem_release(req->buf);
      return -1;
    }
  req->size = size;

#if DS_REQUEST
  /* share dataspace with driver */
  ret = l4dm_share(&req->ds, l4blk_get_driver_thread(driver), L4DM_RW);
  if (ret < 0)
    LOG_Error("share dataspace with driver failed: %s (%d)", 
              l4env_errstr(ret), ret);
#endif

  LOGdL(DEBUG_BUFFER, "ds %d at "IdFmt", phys 0x%08x, map %p, size %u", 
        req->ds.id, IdStr(req->ds.manager), req->p_addr, req->buf, size);

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
  int ret;
#if DS_REQUEST
  l4blk_sg_ds_elem_t sg_list;
#else
  l4blk_sg_phys_elem_t sg_list;
#endif  

  // check for end
  if (next_request >= NUM)
    {
      /* wakeup main */
      l4semaphore_up(&done_wait);
      return;
    }
  
  /* setup request structure */
  request->driver = driver;
  request->request.device = DEVICE;
  request->request.count = BLK_SIZE >> 10;
  request->request.stream = L4BLK_INVALID_STREAM;	      

  /* buffer */
#if DS_REQUEST
  sg_list.ds = desc->ds;
  sg_list.offs = 0;
  sg_list.size = desc->size;
  request->sg_type = L4BLK_SG_DS;
#else
  sg_list.addr = desc->p_addr;
  sg_list.size = desc->size;
  request->sg_type = L4BLK_SG_PHYS;
#endif
  request->sg_list = &sg_list;
  request->sg_num = 1;
  
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
  next_block += (BLK_SIZE >> 10);
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
  
  /* done */
  next_request++;
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
  total++;
  switch(status)
    {
    case L4BLK_DONE:
      /* do nothing */
      break;
      
    case L4BLK_SKIPPED:
    case L4BLK_ERROR:
    case L4BLK_UNPROCESSED:
      Error("request failed (status %02x, error %d)!",status,error);
      break;
      
    default:
      Error("invalid request status: %08x",status);
    }
  
#if MEASURE_SINGLE
  s_end = l4_rdtsc();
  
  s_total += (s_end - s_start);
  s_num++;
#endif
  
  /* do next request */
  do_next_request(request);
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
	
  for (i = 0; i < NUM_REQUESTS; i++)
    do_next_request(&requests[i]);
	
  /* wait for benchmark end */
  for (i = 0; i < NUM_REQUESTS; i++)
    l4semaphore_down(&done_wait);
	
  /* done */
  t_end = l4_rdtsc();
  mus = (l4_uint32_t)(((t_end - t_start) * ns_per_cycle) / 1000.0);
  bw = ((double)total * BLK_SIZE);
	
  LOGL("%lu bytes",(unsigned long)bw);
  
  bw /= ((double)mus / 1000.0 / 1000.0);
  bw /= 1024.0;
  
  if (WRITE)
    {
      if (RANDOM)
        LOGL("%lu requests total (%u kbyte blocks, write, random), %2u.%03ums",
             total,BLK_SIZE >> 10,mus / 1000, mus % 1000);
      else
        LOGL("%lu requests total (%u kbyte blocks, write, linear), %2u.%03ums",
             total,BLK_SIZE >> 10,mus / 1000, mus % 1000);
    }
  else
    {
      if (RANDOM)
        LOGL("%lu requests total (%u kbyte blocks, read, random), %2u.%03ums",
             total,BLK_SIZE >> 10,mus / 1000, mus % 1000);
      else
        LOGL("%lu requests total (%u kbyte blocks, read, linear), %2u.%03ums",
             total,BLK_SIZE >> 10,mus / 1000, mus % 1000);
    }
	
#if MEASURE_SINGLE
  LOGL("bandwidth: %u.%03u KB/s, %lu cycles/request",(unsigned)bw,
       (unsigned)((bw - (unsigned)bw) * 1000.0), 
       (unsigned long)(s_total / (unsigned long long)s_num));
#else 
  LOGL("bandwidth: %u.%03u KB/s",(unsigned)bw,
       (unsigned)((bw - (unsigned)bw) * 1000.0));
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
  int ret;
  double mhz;

  /* init block device driver lib */
  l4blk_init();
  
  /* init random number generator */
  l4util_srand(l4_rdtsc_32());
  
  /* init time measuring */
  l4_calibrate_tsc();
  ns_per_cycle = l4_tsc_to_ns(10000000ULL) / 10000000.0;
  mhz = 1000.0 / ns_per_cycle;
  printf("running on a %u.%02u MHz machine (%u.%03u ns/cycle)\n",
         (unsigned)mhz,(unsigned)((mhz - (unsigned)mhz) * 100),
         (unsigned)ns_per_cycle,
         (unsigned)((ns_per_cycle - (unsigned)ns_per_cycle) * 1000));
  
  /* open driver */
  ret = l4blk_open_driver(DRIVER_NAME, &driver);
  if (ret < 0)
    {
      Panic("failed to open driver (%d)\n",ret);
      return -1;
    }
  LOGL("driver handle: %d",driver);
  
  /* get actual disk size */
  MAX_BLOCK = l4blk_ctrl_get_disk_size(driver, DEVICE);
  if (MAX_BLOCK < 0)
    {
      Panic("failed to get disk size (%d)", MAX_BLOCK);
      l4blk_close_driver(driver);
      return -1;
    }
  LOGL("max block: %d", MAX_BLOCK);
  
  /* do benchmark */
  bench(driver);
  
  l4thread_sleep(2000);
  l4blk_close_driver(driver);
  Kdebug("done");
  
  /* done */
  return 0;
}

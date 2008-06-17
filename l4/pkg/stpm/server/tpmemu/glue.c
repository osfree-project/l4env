#include "tpm/tpm_emulator.h"

#include <l4/log/l4log.h>
#include <l4/util/rdtsc.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/env/errno.h>
#include <l4/env/env.h>
#include <l4/dm_mem/dm_mem.h>

#include <l4/generic_fprov/fprov_ext-client.h>

#include <tcg/rand.h> //STPM_GetRandom
#include "local.h"

#include <stdio.h> //printf

void tpm_log(int priority, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  if (priority < LOG_INFO)
    LOG_vprintf(fmt, ap);
  va_end(ap);
}

/**
 * Returns random bytes.
 * First another (real) TPM service is requested (e.g. STPM).
 * If this fails the standard libc random() function is used,
 * which is of course bad. After some invocations (default: 10)
 * this function will try to use the TPM service again.
 */
void tpm_get_random_bytes(void *buf, size_t nbytes)
{
  static int waitretry = 0;
  unsigned long count = nbytes;
  int error;
  unsigned long result, bits;
  unsigned long i;
  char * buffer = (char *)buf;
  char * endbuf = buffer + nbytes;
  int size = sizeof(result);

  //if TPM_GetRandom failed some time ago increment retry countdown
  //until a value and then try again    
  //for the meantime we are using random() of libc
  if (waitretry == 50)
    waitretry = 0;
  
  if (waitretry == 0)
  { 
    error = STPM_GetRandom(nbytes, &count, buf);
    if (error == 0)
      // all fine
      return;

    LOG("WARNING: failed (%d), use random() instead", error);
  }

  //real TPM seems to be not available
  //use random function

  //increase wait counter
  waitretry ++;

  //detect count of bits which are not part of randomization
  i = ~RAND_MAX;
  bits = 0;
  while (i != 0)
  {
    i = i << 1; bits++;
  }
//  printf("random %x bits shift=%lu", RAND_MAX, bits);
  
  while (buffer + size < endbuf)
  {
    result = (random() << bits) + random();
    *(int long*)buffer = result;

    buffer += size;
//    printf("%lx ", result);
  }
 
  result = random();
  size = endbuf - buffer;
  for (i=0; i < size; i++)
  {
    *buffer = ((char *)&result)[i];
    buffer ++;
  }
//  printf("%lx\n", result);
}

uint64_t tpm_get_ticks(void)
{
  static l4_cpu_time_t old = 0;
  l4_cpu_time_t tmp = old;

  old = l4_tsc_to_ns(l4_rdtsc()) / 1000;

  return ((old > tmp) ? old - tmp : 0 );
}

int tpm_write_to_file(uint8_t *data, size_t data_length)
{
  DICE_DECLARE_ENV(env);
  int error;
  int flags = 0;
  l4_threadid_t tftp_id;
  l4_threadid_t dm_id;
  l4dm_dataspace_t ds;
  void *addr;
  l4_size_t size = data_length;
  char * vtpmname = vtpm_get_name();
  int namesize = strlen(vtpmname);
  char * fname;
 
  if (!names_waitfor_name("TFTP", &tftp_id, 40000))
    {
      printf("TFTP not found\n");
      return -1;
    }

  dm_id = l4env_get_default_dsm();
  if (l4_is_invalid_id(dm_id))
    {
      printf("No dataspace manager found\n");
      return -1;
    }

  fname = malloc( 10 + namesize);
  if (fname == 0)
    {
      return -L4_ENOMEM;
    }

  memcpy(fname, "incoming/", 9);
  memcpy(fname + 9, vtpmname, namesize);
  fname[ 9 + namesize ] = 0;

  if (!(addr = l4dm_mem_ds_allocate_named(size, flags, fname, &ds)))
    {
      printf("Allocating dataspace of size %d failed\n", size);
      free(fname);
      return -L4_ENOMEM;
    }

  memcpy(addr, data, data_length);

  if ((error = l4rm_detach(addr)))
    {
      printf("Error %d attaching dataspace\n", error);
      free(fname);
      return -L4_ENOMEM;
    }

  /* set dataspace owner to server */
  if ((error = l4dm_transfer(&ds, tftp_id)))
    {
      printf("Error transfering dataspace ownership: %s (%d)\n",
             l4env_errstr(error), error);
      l4dm_close(&ds);
      free(fname);
      return -L4_EINVAL;
    }

  if ((error = l4fprov_file_ext_write_call(&tftp_id, fname,
                                           &ds, size, &env)))
    {
      printf("Error opening file from tftp\n");
      return -1;
    }

  printf("File %s was written\n", fname);
  free(fname);

  return 0;
}

int tpm_read_from_file(uint8_t **data, size_t *data_length)
{
  DICE_DECLARE_ENV(env);
  l4dm_dataspace_t ds;
  void *addr;
  size_t size;
  long error;
  l4_threadid_t fprov_id;
  l4_threadid_t dm_id;
  char * fname;
  char * vtpmname = vtpm_get_name();
  int namesize = strlen(vtpmname);

  /* dataspace manager */
  dm_id = l4env_get_default_dsm();
  if (l4_is_invalid_id(dm_id))
    {
      printf("No dataspace manager found!\n");
      return -L4_ENODM;
    }

  if (!names_waitfor_name("TFTP", &fprov_id, 5000))
    {
      LOG("Failed to lookup specified file provider.");
      return -1;
    }

  fname = malloc( 10 + namesize);
  if (fname == 0)
    {
      return -L4_ENOMEM;
    }

  memcpy(fname, "(nd)/incoming/", 14);
  memcpy(fname + 14, vtpmname, namesize);
  fname[ 14 + namesize ] = 0;

  error = l4fprov_file_open_call(&fprov_id, fname, &dm_id, 0,
                                 &ds, &size, &env);

  if (DICE_HAS_EXCEPTION(&env))
  {
    free(fname);
    return -L4_EIPC;
  }
  if (error < 0)
  {
    free(fname);
    return error;
  }

  if ((error = l4rm_attach(&ds, size, 0, L4DM_RO, &addr)))
    {
      LOG("Error %ld attaching dataspace for module %s", error, fname);
      l4rm_detach(addr);
      l4dm_close(&ds);
      free(fname);
      return -1;
    }

  if (!(*data = malloc(size)))
  {
    free(fname);
    return -L4_ENOMEM;
  }

  memcpy(*data, addr, size);
  *data_length = size;

  l4rm_detach(addr);
  l4dm_close(&ds);
  free(fname);

  return 0;
}

#include "tpm/tpm_emulator.h"

#include <l4/log/l4log.h>
#include <l4/util/rdtsc.h>
#include <tcg/tpm.h>       //TPM_GetRandom

#define GLUE_TPM_TRANSMIT_FUNC(NAME,TEST,PARAMS,PRECOND,POSTCOND,FMT,...) \
unsigned long TPM_##NAME##_##TEST PARAMS; \
unsigned long TPM_##NAME##_##TEST PARAMS { \
  unsigned char buffer[TCG_MAX_BUFF_SIZE]; /* request/response buffer */ \
  unsigned long ret;         \
  PRECOND               \
  ret = buildbuff("00 C1 T L " FMT, buffer, TPM_ORD_##NAME, ##__VA_ARGS__ ); \
  if (ret < 0)          \
     return -1;         \
  ret = TPM_Transmit(buffer, #NAME ); \
  if (ret != 0)         \
      return ret;       \
  POSTCOND              \
  return ret;           \
}

GLUE_TPM_TRANSMIT_FUNC(GetRandom, STPM,
                  (unsigned long count,
                   unsigned long *length,
                   unsigned char *random),
                  if (length == NULL || random == NULL)
                    return -1;
                  ,
                  *length = TPM_EXTRACT_LONG(0);
                  TPM_COPY_FROM(random, 4, *length);
                  ,
                  "L",
                  count);

void tpm_log(int priority, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    LOG_vprintf(fmt, ap);
    va_end(ap);
}

void tpm_get_random_bytes(void *buf, size_t nbytes)
{
  unsigned long count = nbytes;
  int error = TPM_GetRandom_STPM(nbytes, &count, buf);

  if (error != 0)
    LOG("tpm_get_random_bytes failed (error=%d)\n", error);
}

uint64_t tpm_get_ticks(void)
{
  static l4_cpu_time_t old = 0;
  l4_cpu_time_t tmp = old;

  old = l4_tsc_to_ns(l4_rdtsc()) / 1000;
  LOG("timestamp diff %llu vs %llu", old - tmp, (old != 0 && old > tmp) ? old - tmp : 0 );
  return ((old > tmp) ? old - tmp : 0 );
}

int tpm_write_to_file(uint8_t *data, size_t data_length)
{
  LOG("implement me tpm_write_to_file");
  return -1;
}

int tpm_read_from_file(uint8_t **data, size_t *data_length)
{
  LOG("implement me tpm_read_from_file");
  return -1;
}



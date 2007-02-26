#if !defined(__bsdi__) && !defined(__FreeBSD__)
#include <malloc.h>
#endif // BSD
#include <stdio.h>
#include <string.h>
#include <ogg/ogg.h>

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif


  u_int16_t get_uint16 (const void *buf)
  {
    u_int16_t ret;
    unsigned char *tmp;

      tmp = (unsigned char *) buf;

      ret = tmp[1] & 0xff;
      ret = (ret << 8) + (tmp[0] & 0xff);

      return ret;
  }

  u_int32_t get_uint32 (const void *buf)
  {
    u_int32_t ret;
    unsigned char *tmp;

    tmp = (unsigned char *) buf;

    ret = tmp[3] & 0xff;
    ret = (ret << 8) + (tmp[2] & 0xff);
    ret = (ret << 8) + (tmp[1] & 0xff);
    ret = (ret << 8) + (tmp[0] & 0xff);

    return ret;
  }

  u_int64_t get_uint64 (const void *buf)
  {
    u_int64_t ret;
    unsigned char *tmp;

    tmp = (unsigned char *) buf;

    ret = tmp[7] & 0xff;
    ret = (ret << 8) + (tmp[6] & 0xff);
    ret = (ret << 8) + (tmp[5] & 0xff);
    ret = (ret << 8) + (tmp[4] & 0xff);
    ret = (ret << 8) + (tmp[3] & 0xff);
    ret = (ret << 8) + (tmp[2] & 0xff);
    ret = (ret << 8) + (tmp[1] & 0xff);
    ret = (ret << 8) + (tmp[0] & 0xff);

    return ret;
  }

  void put_uint16 (void *buf, u_int16_t val)
  {
    unsigned char *tmp;

    tmp = (unsigned char *) buf;

    tmp[0] = val & 0xff;
    tmp[1] = (val >>= 8) & 0xff;
  }

  void put_uint32 (void *buf, u_int32_t val)
  {
    unsigned char *tmp;

    tmp = (unsigned char *) buf;

    tmp[0] = val & 0xff;
    tmp[1] = (val >>= 8) & 0xff;
    tmp[2] = (val >>= 8) & 0xff;
    tmp[3] = (val >>= 8) & 0xff;
  }

  void put_uint64 (void *buf, u_int64_t val)
  {
    unsigned char *tmp;

    tmp = (unsigned char *) buf;

    tmp[0] = val & 0xff;
    tmp[1] = (val >>= 8) & 0xff;
    tmp[2] = (val >>= 8) & 0xff;
    tmp[3] = (val >>= 8) & 0xff;
    tmp[4] = (val >>= 8) & 0xff;
    tmp[5] = (val >>= 8) & 0xff;
    tmp[6] = (val >>= 8) & 0xff;
    tmp[7] = (val >>= 8) & 0xff;
  }

#ifdef __cplusplus
}				// extern "C"
#endif

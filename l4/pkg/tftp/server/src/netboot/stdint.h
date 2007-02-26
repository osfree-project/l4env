#ifndef STDINT_H
#define STDINT_H
/* 
 * I'm architecture depended. Check me before port GRUB
 */

#include <l4/sys/l4int.h>

typedef l4_uint8_t         uint8_t;
typedef l4_uint16_t        uint16_t;
typedef l4_uint32_t        uint32_t;
typedef l4_uint64_t        uint64_t;

typedef l4_int8_t          int8_t;
typedef l4_int16_t         int16_t;
typedef l4_int32_t         int32_t;
typedef l4_int64_t         int64_t;

#endif /* STDINT_H */

#ifndef __BOOTSTRAP__UNCOMPRESS_H__
#define __BOOTSTRAP__UNCOMPRESS_H__

#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

void *decompress(const char *name, void *start, void *destbuf,
                 int size, int size_uncompressed);

EXTERN_C_END

#endif /* ! __BOOTSTRAP__UNCOMPRESS_H__ */

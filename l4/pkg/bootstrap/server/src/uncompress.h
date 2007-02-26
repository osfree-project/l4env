#ifndef __BOOTSTRAP__UNCOMPRESS_H__
#define __BOOTSTRAP__UNCOMPRESS_H__

void *decompress(const char *name, void *start,
                 int size, int size_uncompressed);

#endif /* ! __BOOTSTRAP__UNCOMPRESS_H__ */

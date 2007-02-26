
#include <l4/log/l4log.h>
#include <stdio.h>

FILE *fopen(const char *path, const char *mode)
{
  LOG("fopen(%s) called but not implemented", path);
  return NULL;
}

int fclose(FILE *fp)
{
  LOG("fclose called but not implemented");
  return 0;
}

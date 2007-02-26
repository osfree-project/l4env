#include <l4/l4vfs/types.h>

char *dirname(char *);
char *basename(char *);

char *dirname(char *path)
{
  int c;
  for (c = 0; path[c]; c++) ;
  for (; c >= 0 && path[c] != L4VFS_PATH_SEPARATOR; c--) ;
  if (c > 0)
    path[c] = '\0';
  else if (c == 0)
    path = "/";
  else
    path = ".";
  return path;
}

char *basename(char *path)
{
  int c;
  for (c = 0; path[c]; c++) ;
  for (; c >= 0 && path[c] != L4VFS_PATH_SEPARATOR; c--) ;
  if (c >= 0)
    return path + c + 1;
  else
    return path;
}

#include <stdio.h>
#include <libc_backend.h>


char *fgets(char *s, int size, FILE *)
{
  if(s!=NULL) {
    size_t num;
    num = __libc_backend_ins(s,size);
    s[num-1]=0;
  }
  return s;
}

#include <l4/sys/kdebug.h>
#include <stdio.h>
#include "debug_glue.h"


//XXX we need a better solution that the circled dependency between
// dietlibc and l4sys
void
outchar(char c)
{
  __libc_backend_outs(&c, 1);
}


void
outstring(const char * text)
{
  puts(text);
}


void
outnstring(char const *text, unsigned len)
{
  __libc_backend_outs(text, len);
}


void
outhex32(int number)
{
  printf("%08x", (unsigned) number);
}


void
outhex20(int number)
{
  printf("%05x", (unsigned) number);
}

void
outhex16(int number)
{
  printf("%04x", (unsigned short) number);
}


void
outhex12(int number)
{
  printf("%03x", (unsigned short) number);
}


void
outhex8(int number)
{
  printf("%02x", (unsigned char) number);
}


void
outdec(int number)
{
  printf("%d", number);
}


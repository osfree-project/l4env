#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <l4/sys/kdebug.h>
#include <l4/util/list_alloc.h>
#include <l4/log/l4log.h>
#include "malloc.h"
#include "emul_linux.h"

#define MALLOC_POOL	(64<<10)

#if DEBUG_LEVEL>0
#define DBG	1
#else
#define DBG	0
#endif

static l4la_free_t *__malloc_list;
static void *__malloc_pool;

/* helper for demangle library */
void __assert(const char *a, const char *f, int l, const char *fn);
void
__assert(const char *a, const char *f, int l, const char *fn)
{
  printf("%s: %d: %s: Assertion `%s' failed.\n", f, l, fn, a);
}
__typeof(__assert) __assert_fail __attribute__((alias("__assert")));

/* helper for demangle library */
void xexit(int code);
void
xexit(int code)
{
  for (;;)
    enter_kdebug("xexit");
}

/* helper for demangle library */
void abort(void);
void
abort(void)
{
  for (;;)
    enter_kdebug("abort");
}

/* helper for demangle library */
int atoi (const char *s)
{
  long v=0;
  int  sign=1;
  while (*s == ' ' || (unsigned)(*s - 9) < 5u)
    s++;
  switch (*s)
    {
    case '-':
      sign=-1;
    case '+':
      ++s;
    }
  while ((unsigned) (*s - '0') < 10u)
    {
      v=v*10+*s-'0';
      ++s;
    }
  return sign==-1?-v:v;
}

/* helper for demangle library */
char *
strcat (char *dest, const char *src)
{
  char c, *d = dest;
  do
    c = *d++;
  while (c != '\0');
  d -= 2;
  do
    {
      c = *src++;
      *++d = c;
    }
  while (c != '\0');
  return dest;
}

/* helper for demangle library */
char *
strncat (char *dest, const char *src, size_t n)
{
  char c, *d = dest;
  do
    c = *d++;
  while (c != '\0');
  d -= 2;

  while (n > 0)
    {
      c = *src++;
      *++d = c;
      if (c == '\0')
	return dest;
      n--;
    }
  if (c != '\0')
    *++d = '\0';

  return dest;
}

/* helper for demangle library */
char *
strpbrk (const char *s, const char *accept)
{
  while (*s != '\0')
    {
      const char *a = accept;
      while (*a != '\0')
	if (*a++ == *s)
	  return (char *)s;
      s++;
    }
  return 0;
}

/* helper for demangle library */
size_t
strspn (const char *s, const char *accept)
{
  const char *p;
  const char *a;
  size_t count = 0;

  for (p = s; *p != '\0'; ++p)
    {
      for (a = accept; *a != '\0'; ++a)
	if (*p == *a)
	  break;
      if (*a == '\0')
	return count;
      else
	++count;
    }
  return count;
}

size_t
strcspn (const char *s, const char *reject)
{
  size_t count = 0;

  while (*s!='\0' && strchr(reject, *s++))
    ++count;

  return count;
}

int vsscanf(const char *str, const char *format, va_list arg_ptr);
int
vsscanf(const char *str, const char *format, va_list arg_ptr)
{
  unsigned long d, v = 0, *l;

  if (strcmp(format, "%x"))
    enter_kdebug("vsscanf");

  while (*str)
    {
      v = v*16;
      d = *str++ | 0x20;
      if (d >= '0' && d <= '9')
	v += d - '0';
      else if (d >= 'a' && d <= 'z')
	v += d - 'a' + 10;
      else
	break;
    }

  l = (unsigned long*)va_arg(arg_ptr, unsigned long);
  *l = v;
  return 0;
}

int
sscanf(const char *str, const char *format, ...)
{
  int n;
  va_list arg_ptr;
  va_start(arg_ptr, format);
  n = vsscanf(str, format, arg_ptr);
  va_end(arg_ptr);
  return n;
}

int
demangle_malloc_reset(void)
{
  if (!__malloc_pool)
    {
      if (!(__malloc_pool = _dl_alloc_pages(MALLOC_POOL, 0, "[malloc pool]")))
	{
	  LOGd(DBG, "Cannot allocate memory for demangle_malloc");
	  return 0;
	}
    }
  l4la_init(&__malloc_list);
  l4la_free(&__malloc_list, __malloc_pool, MALLOC_POOL);
  return 1;
}

void *
demangle_malloc(size_t size)
{
  void *a;
  l4_size_t *s;

  size += sizeof(l4_size_t);
  if (EXPECT_FALSE(!(a = l4la_alloc(&__malloc_list, size, 2))))
    {
      LOGd(DBG, "demangle_malloc memory exhausted.");
      return 0;
    }
  s  = (l4_size_t*)a;
  *s = size;
  return s+1;
}

void *
demangle_calloc(size_t nmemb, size_t size)
{
  size_t sz = size*nmemb;
  return (nmemb && (sz/nmemb != size)) 
    ? 0 
    : demangle_malloc(sz);
}

void *
demangle_realloc(void *ptr, size_t size)
{
  void *p = demangle_malloc(size);
  if (ptr)
    memcpy(p, ptr, size);
  return p;
}

void demangle_free(void *ptr)
{
  l4_size_t *s = (l4_size_t*)ptr - 1;
  l4la_free(&__malloc_list, s, *s);
}

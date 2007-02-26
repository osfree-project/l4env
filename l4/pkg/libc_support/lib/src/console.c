
#include <l4/sys/kdebug.h>

/* Prototypes */
int console_putchar(int c);
int console_puts(const char *s);



/* Implementations */

int console_putchar(int c)
{
  outchar(c);

  if (c == '\n')
    outchar('\r');

  return c;
}

int console_puts(const char *s)
{
  int i = 0;

  while (s[i])
    console_putchar(s[i++]);

  return 0;
}


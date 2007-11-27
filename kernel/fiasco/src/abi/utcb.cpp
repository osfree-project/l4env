INTERFACE:

class Utcb
{
};

IMPLEMENTATION:

PUBLIC
bool Utcb::inherit_fpu() const
{ return buffers[0] & 2; }


// ----------------------------------------------------------------------------
IMPLEMENTATION[debug]:

#include <cstdio>

PUBLIC
void
Utcb::print() const
{
  puts("Values:");
  for (unsigned i = 0; i < Max_words; ++i)
    printf("%2d:%16lx%c", i, values[i], !((i+1) % 4) ? '\n' : ' ');
  if (Max_words % 4)
    puts("");

  puts("Buffers:");
  for (unsigned i = 0; i < sizeof(buffers) / sizeof(buffers[0]); ++i)
    printf("%2d:%16lx%c", i, buffers[i], !((i+1) % 4) ? '\n' : ' ');
  if ((sizeof(buffers) / sizeof(buffers[0])) % 4)
    puts("");

  printf("Xfer timeout: ");
  xfer.print();
  puts("");
}

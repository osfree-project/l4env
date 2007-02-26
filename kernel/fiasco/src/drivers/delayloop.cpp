INTERFACE:

#include "std_macros.h"
#include "initcalls.h"

class Delay
{
private:
  static unsigned count;

public:
  static void init() FIASCO_INIT;
};

IMPLEMENTATION:

#include "kip.h"

unsigned Delay::count;

IMPLEMENT void
Delay::init()
{
  Cpu_time t = Kip::k()->clock;
  Cpu_time t1;
  count = 0;

  Kip *k = Kip::k();
  while (t == (t1 = k->clock))
    ;
  while (t1 == k->clock)
    ++count;
}

/**
 * Hint: ms is actually the timer granularity, which
 *       currently happens to be milliseconds
 */
PUBLIC static void
Delay::delay(unsigned ms)
{
  Kip *k = Kip::k();
  while (ms--)
    {
      unsigned c = count;
      while (c--)
	(void)k->clock;
    }
}

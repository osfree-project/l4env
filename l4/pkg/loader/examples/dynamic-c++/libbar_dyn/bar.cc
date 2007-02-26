#include <stdio.h>

class bar_t
{
public:
  bar_t::bar_t();
};

// Note that static constructors are not supported by the L4 loader!
bar_t::bar_t()
{
  printf("This is bar_t::bar_t\n");
}

static bar_t bar;

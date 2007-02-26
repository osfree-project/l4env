
#include <stdlib.h>

void __main(void) __attribute__((noreturn));
int main(void);

void __main()
{
  exit(main());
}

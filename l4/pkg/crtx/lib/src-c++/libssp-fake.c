/* Some distributions ship their libs with the stack protector feature
 * enabled. So when linking against those libs, we need provide the symbols.
 */

#include <unistd.h>

void
__stack_chk_fail_local (void);

void
__stack_chk_fail_local (void)
{
  // should we add a printf here? Someone will not like it...
  _exit(1);
}

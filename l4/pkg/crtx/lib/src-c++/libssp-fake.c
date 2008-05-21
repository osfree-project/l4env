/* Some distributions ship their libs with the stack protector feature
 * enabled. So when linking against those libs, we need provide the symbols.
 */

#include <unistd.h>

#ifdef ARCH_x86
#define FUNCNAME __stack_chk_fail_local
#elif defined(ARCH_amd64)
#define FUNCNAME __stack_chk_fail
#endif

void FUNCNAME (void);
void FUNCNAME (void)
{
  // should we add a printf here? Someone will not like it...
  _exit(1);
}

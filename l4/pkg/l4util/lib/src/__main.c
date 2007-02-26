
#include <stdlib.h>
#include <l4/crtx/crt0.h>
#include <l4/util/mbi_argv.h>

extern int main(int argc, char *argv[]);

void
__main(void)
{
  /* parse command line and initialize l4util_argc/l4util_argv */
  l4util_mbi_to_argv(crt0_multiboot_flag, crt0_multiboot_info);

  /* call constructors */
  crt0_construction();

  /* call main */
  exit(main(l4util_argc, l4util_argv));
}


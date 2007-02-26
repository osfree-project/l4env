// now we have a nice infrastructure to setup all the pre-main stuff
// thanks frank!!!

#include <l4/dietlibc/l4xvfs_lib.h>
#include <l4/crtx/crt0.h>
#include <l4/util/mbi_argv.h>

int l4libc_init_mem(void);
int l4libc_init_mem(void)
{
    l4xvfs_init();
    return 0;
}

#if 0
extern int main(int argc, char *argv[]);
extern void exit(int code);

void l4libc_startup_main(void);
void l4libc_startup_main(void)
{
  crt0_call_constructors();

  exit(main(l4util_argc, l4util_argv));
}
#endif

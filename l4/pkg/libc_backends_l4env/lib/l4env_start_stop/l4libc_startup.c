/**
 * \file   libc_backends_l4env/lib/l4env_start_stop/l4libc_startup.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/crtx/crt0.h>
#include <l4/util/mbi_argv.h>

int l4libc_init_mem(void) __attribute((weak));
int l4libc_init_mem(void)
{
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

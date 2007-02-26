/*!
 * \file   dietlibc/lib/backends/l4env_base/startup.c
 * \brief  
 *
 * \date   08/19/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#if 0
#include <stdlib.h>
#include <l4/crtx/crt0.h>
#include <l4/util/mbi_argv.h>

extern int main(int argc, char *argv[]);

void l4libc_startup_main(void);
void l4libc_startup_main(void)
{
  crt0_call_constructors();
  exit(main(l4util_argc, l4util_argv));
}
#endif


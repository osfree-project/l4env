/*!
 * \file   dietlibc/lib/backends/l4env_base/log-output.c
 * \brief  Output to logserver
 *
 * \date   08/16/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/log/l4log.h>
#include <l4/log/log_printf.h>
#include <l4/dietlibc/fops.h>
#include <l4/env/errno.h>

/* what is written to stderr goes to logserver */
static int log_write(struct l4diet_vfs_file*file, const void*buf, size_t len){
    LOG_printf("%.*s", len, buf);
    return len;
}
l4diet_vfs_file l4diet_vfs_file1={write: log_write};
l4diet_vfs_file l4diet_vfs_file2={write: log_write};

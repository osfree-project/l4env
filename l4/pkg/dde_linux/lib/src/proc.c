/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux/lib/src/proc.c
 *
 * \brief	/proc fs stubs (not implemented yet)
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4 */
#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/proc_fs.h>

/*****************************************************************************/
/** Linux' /proc Entry Creation
 * \ingroup mod_misc
 *
 * \todo implementation 
 */
/*****************************************************************************/
struct proc_dir_entry *create_proc_entry(const char *name, mode_t mode,
					 struct proc_dir_entry *parent)
{
  return (struct proc_dir_entry *) 0;
}

/*****************************************************************************/
/** Linux' /proc Entry Removal
 * \ingroup mod_misc
 *
 * \todo implementation
 */
/*****************************************************************************/
void remove_proc_entry(const char *name, struct proc_dir_entry *parent)
{
}

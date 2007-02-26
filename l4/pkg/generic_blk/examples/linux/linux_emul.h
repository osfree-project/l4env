/* $Id$ */
/*****************************************************************************/
/**
 * \file   genric_blk/examples/linux/linux_emul.c
 * \brief  L4Linux block device stub, emulation stuff
 *
 * \date   02/25/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _L4BLK_LINUX_EMUL_H
#define _L4BLK_LINUX_EMUL_H

/* prototypes */

extern void *
l4blk_allocate_stack(void);

extern void
l4blk_release_stack(void * sp);

#endif /* _L4BLK_LINUX_EMUL_H */

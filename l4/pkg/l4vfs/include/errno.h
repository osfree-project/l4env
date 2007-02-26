/**
 * \file   l4vfs/include/errno.h
 * \brief  Error codes for l4vfs
 *
 * \date   11/08/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_ERRNO_H_
#define __L4VFS_INCLUDE_ERRNO_H_

#define L4VFS_ENOT_ABS_PATH  2000  /**< path given was not an absolute path  */
#define L4VFS_EVOL_NOT_REG   2001  /**< volume is not reg. at name_server    */
#define L4VFS_ERESOLVE       2002  /**< could not resolve path               */
#define L4VFS_EMOUNT_BUSY    2003  /**< mountpoint is busy                   */
#define L4VFS_ENO_SPACE_LEFT 2004  /**< mount table overflow                 */

#endif

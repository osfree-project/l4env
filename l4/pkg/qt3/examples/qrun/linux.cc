/* $Id$ */
/*****************************************************************************/
/**
 * \file   examples/qrun/linux.cc
 * \brief  QRun Linux dummy routines.
 *
 * \date   03/10/2005
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2005-2006 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

#include <qglobal.h>

bool init(const char *prefix) {

  return true;
}

bool loaderRun(const char *binary) {

    qDebug("running %s", binary);
    return true;
}

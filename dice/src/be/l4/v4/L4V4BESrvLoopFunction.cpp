/**
 *  \file     dice/src/be/l4/v4/L4V4BESrvLoopFunction.cpp
 *  \brief    contains the implementation of the class CL4V4BESrvLoopFunction
 *
 *  \date     Tue Jul 6 2004
 *  \author   Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004 by
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2 as
 * published by the Free Software Foundation (see the file COPYING).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For different licensing schemes please contact
 * <contact@os.inf.tu-dresden.de>.
 */
#include "L4V4BESrvLoopFunction.h"

CL4V4BESrvLoopFunction::CL4V4BESrvLoopFunction()
 : CL4BESrvLoopFunction()
{
}

/** destroys instances of this class */
CL4V4BESrvLoopFunction::~CL4V4BESrvLoopFunction()
{
}

/**    \brief writes the server's startup notification
 *    \param pFile the file to write to
 *    \param pContext the context of this operation
 *
 * For V4 do nothing
 */
void
CL4V4BESrvLoopFunction::WriteServerStartupInfo(CBEFile *pFile,
    CBEContext *pContext)
{
}

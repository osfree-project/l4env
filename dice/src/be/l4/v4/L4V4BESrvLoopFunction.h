/**
 *  \file     dice/src/be/l4/v4/L4V4BESrvLoopFunction.h
 *  \brief    contains the declaration of the class CL4V4BESrvLoopFunction
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
#ifndef L4V4BESRVLOOPFUNCTION_H
#define L4V4BESRVLOOPFUNCTION_H

#include <be/l4/L4BESrvLoopFunction.h>

/** \class CL4V4BESrvLoopFunction
 *    \ingroup backend
 *    \brief contains V4 specific implementation of server loop
 */
class CL4V4BESrvLoopFunction : public CL4BESrvLoopFunction
{
public:
    /** creates a new instance of this class */
    CL4V4BESrvLoopFunction();
    virtual ~CL4V4BESrvLoopFunction();

protected:
    virtual void WriteServerStartupInfo(CBEFile *pFile, CBEContext *pContext);

};

#endif

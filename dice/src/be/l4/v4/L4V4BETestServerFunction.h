/**
 *  \file     dice/src/be/l4/v4/L4V4BETestServerFunction.h
 *  \brief    contains the declaration of the class CL4V4BETestServerFunction
 *
 *  \date     Sun Jul 4 2004
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
#ifndef L4V4BETESTSERVERFUNCTION_H
#define L4V4BETESTSERVERFUNCTION_H

#include <be/l4/L4BETestServerFunction.h>

/** \class CL4V4BETestServerFunction
 *    \ingroup backend
 *    \brief L4 version 4 specific implementation
 */
class CL4V4BETestServerFunction : public CL4BETestServerFunction
{
public:
    /** creates an instance of this class */
    CL4V4BETestServerFunction();
    virtual ~CL4V4BETestServerFunction();

protected:
    virtual void Write(CBEImplementationFile * pFile, CBEContext * pContext);
    virtual void WriteBody(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteGlobalVariableDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableInitialization(CBEFile * pFile, CBEContext *pContext);
    virtual void WriteStartServerLoop(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteStopServerLoop(CBEFile *pFile, CBEContext *pContext);

    virtual void WriteStartPagerThread(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteWrapperFunctions(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteStartupNotification(CBEFile * pFile, CBEContext * pContext);
};

#endif

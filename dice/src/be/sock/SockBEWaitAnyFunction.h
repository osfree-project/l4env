/**
 *    \file    dice/src/be/sock/SockBEWaitAnyFunction.h
 *    \brief   contains the declaration of the class CSockBEWaitAnyFunction
 *
 *    \date    11/28/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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

#ifndef SOCKBEWAITANYFUNCTION_H
#define SOCKBEWAITANYFUNCTION_H

#include "be/BEWaitAnyFunction.h"

/** \class CSockBEWaitAnyFunction
 *  \ingroup backend
 *  \brief contains the platform specific code of the wait-any function
 *
 *  This class represents the wait-any function, which is used to wait for
 *  any message from any sender. It extracts the opcode from the message and
 *  returns it.
 */
class CSockBEWaitAnyFunction : public CBEWaitAnyFunction
{
// Constructor
public:
    /** \brief constructor
     *    \param bOpenWait true if wait for any sender
     *    \param bReply true if reply is sent before wait
     */
    CSockBEWaitAnyFunction(bool bOpenWait, bool bReply);
    virtual ~CSockBEWaitAnyFunction();

protected:
    /**    \brief copy constructor */
    CSockBEWaitAnyFunction(CSockBEWaitAnyFunction &src);

    virtual void WriteInvocation(CBEFile * pFile, CBEContext * pContext);
    virtual void WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext);
    virtual bool AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext);
};

#endif

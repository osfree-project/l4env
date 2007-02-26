/**
 *    \file    dice/src/be/l4/v4/L4V4BEClassFactory.h
 *  \brief   contains the declaration of the class CL4V4BEClassFactory
 *
 *    \date    01/06/2004
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

/** preprocessing symbol to check header file */
#ifndef CL4V4BECLASSFACTORY_H
#define CL4V4BECLASSFACTORY_H

#include <be/l4/L4BEClassFactory.h>

/**
 *  \class CL4V4BEClassFactory
 *  \ingroup backend
 *  \brief specializes the class factory for the V4 backend
 */
class CL4V4BEClassFactory : public CL4BEClassFactory
{
// Constructor
public:
    /** \brief constructor
     */
    CL4V4BEClassFactory();
    virtual ~CL4V4BEClassFactory();

public:
    virtual CBECommunication* GetNewCommunication();
    virtual CBESizes* GetNewSizes();
    virtual CBECallFunction* GetNewCallFunction();
    virtual CBEMarshalFunction* GetNewMarshalFunction();
    virtual CBEWaitAnyFunction* GetNewWaitAnyFunction();
    virtual CBEWaitAnyFunction* GetNewRecvAnyFunction();
    virtual CBEWaitAnyFunction * GetNewReplyAnyWaitAnyFunction();
    virtual CBESndFunction* GetNewSndFunction();
    virtual CBEWaitFunction* GetNewWaitFunction();
    virtual CBEWaitFunction* GetNewRcvFunction();
    virtual CBEMsgBuffer* GetNewMessageBuffer();
    virtual CBETrace* GetNewTrace();
    virtual CBEMarshaller* GetNewMarshaller();
    virtual CBESrvLoopFunction* GetNewSrvLoopFunction();
};

#endif

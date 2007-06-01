/**
 *    \file    dice/src/be/l4/L4BEClassFactory.h
 *  \brief   contains the declaration of the class CL4BEClassFactory
 *
 *    \date    02/07/2002
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
#ifndef __DICE_L4BEClassFactory_H__
#define __DICE_L4BEClassFactory_H__

#include <be/BEClassFactory.h>

/** \class CL4BEClassFactory
 *  \ingroup backend
 *  \brief the class factory for the back-end classes
 *
 * We use seperate functions for each class, because the alternative is to use
 * some sort of identifier to find out which class to generate. This involves
 * writing a big switch statement.
 */
class CL4BEClassFactory : public CBEClassFactory
{
// Constructor
public:
    /** \brief constructor
     */
    CL4BEClassFactory();
    virtual ~CL4BEClassFactory();

public:
    virtual CBEHeaderFile* GetNewHeaderFile();
    virtual CBEWaitAnyFunction* GetNewWaitAnyFunction();
    virtual CBEUnmarshalFunction* GetNewUnmarshalFunction();
    virtual CBESrvLoopFunction* GetNewSrvLoopFunction();
    virtual CBECallFunction* GetNewCallFunction();
    virtual CBESndFunction * GetNewSndFunction();
    virtual CBEWaitFunction * GetNewRcvFunction();
    virtual CBEWaitFunction * GetNewWaitFunction();
    virtual CBEWaitAnyFunction * GetNewRcvAnyFunction();
    virtual CBETypedDeclarator* GetNewTypedDeclarator();
    virtual CBEWaitAnyFunction * GetNewReplyAnyWaitAnyFunction();
    virtual CBEReplyFunction * GetNewReplyFunction();
    virtual CBEClass * GetNewClass();
    virtual CBEMarshalFunction* GetNewMarshalFunction();
    virtual CBEMarshalExceptionFunction* GetNewMarshalExceptionFunction();
    virtual CBEDispatchFunction* GetNewDispatchFunction();
    virtual CBESwitchCase* GetNewSwitchCase();
    virtual CBEMsgBuffer* GetNewMessageBuffer();
    virtual CBEMsgBufferType* GetNewMessageBufferType();
    virtual CBEMarshaller* GetNewMarshaller();
};

#endif // !__DICE_L4BEClassFactory_H__

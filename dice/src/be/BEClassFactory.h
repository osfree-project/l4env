/**
 *  \file    dice/src/be/BEClassFactory.h
 *  \brief   contains the declaration of the class CBEClassFactory
 *
 *  \date    01/10/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#ifndef __DICE_BECLASSFACTORY_H__
#define __DICE_BECLASSFACTORY_H__

#include "BEObject.h"

// the classes to be created
class CBERoot;
class CBEClient;
class CBEComponent;
class CBEHeaderFile;
class CBEImplementationFile;
class CBESndFunction;
class CBEWaitFunction;
class CBEReplyRcvFunction;
class CBEReplyFunction;
class CBEReplyWaitFunction;
class CBECallFunction;
class CBECppCallWrapperFunction;
class CBEUnmarshalFunction;
class CBEMarshalFunction;
class CBEMarshalExceptionFunction;
class CBEComponentFunction;
class CBESwitchCase;
class CBEWaitAnyFunction;
class CBEDispatchFunction;
class CBESrvLoopFunction;
class CBEAttribute;
class CBEType;
class CBEOpcodeType;
class CBEReplyCodeType;
class CBEUserDefinedType;
class CBETypedDeclarator;
class CBETypedef;
class CBEException;
class CBEUnionCase;
class CBEDeclarator;
class CBEExpression;
class CBEConstant;
class CBEClass;
class CBENameSpace;
class CBEMarshaller;
class CBEContext;
class CBESizes;
class CBECommunication;
class CBEMsgBuffer;
class CBEMsgBufferType;
class CTrace;

/** \class CBEClassFactory
 *  \ingroup backend
 *  \brief the class factory for the back-end classes
 *
 * We use seperate functions for each class, because the alternative is to use
 * some sort of identifier to find out which class to generate. This involves
 * writing a big switch statement.
 */
class CBEClassFactory : public CBEObject
{
// Constructor
public:
    /** \brief constructor
     */
    CBEClassFactory();
    ~CBEClassFactory();

    virtual CBEClass* GetNewClass();
    virtual CBEUserDefinedType* GetNewUserDefinedType();
    virtual CBESwitchCase* GetNewSwitchCase();
    virtual CBEComponentFunction* GetNewComponentFunction();
    virtual CBEOpcodeType* GetNewOpcodeType();
    virtual CBEReplyCodeType* GetNewReplyCodeType();
    virtual CBEUnmarshalFunction* GetNewUnmarshalFunction();
    virtual CBEMarshalFunction* GetNewMarshalFunction();
    virtual CBEMarshalExceptionFunction* GetNewMarshalExceptionFunction();
    virtual CBEWaitAnyFunction* GetNewWaitAnyFunction();
    virtual CBEWaitAnyFunction* GetNewRcvAnyFunction();
    virtual CBESrvLoopFunction* GetNewSrvLoopFunction();
    virtual CBEConstant* GetNewConstant();
    virtual CBECallFunction* GetNewCallFunction();
    virtual CBECppCallWrapperFunction* GetNewCppCallWrapperFunction();
    virtual CBETypedef* GetNewTypedef();
    virtual CBEExpression* GetNewExpression();
    virtual CBEDeclarator* GetNewDeclarator();
    virtual CBEUnionCase* GetNewUnionCase();
    virtual CBEException* GetNewException();
    virtual CBETypedDeclarator* GetNewTypedDeclarator();
    virtual CBEType* GetNewType(int nType);
    virtual CBEAttribute* GetNewAttribute();
    virtual CBEReplyFunction* GetNewReplyFunction();
    virtual CBEWaitFunction* GetNewWaitFunction();
    virtual CBEWaitFunction* GetNewRcvFunction();
    virtual CBESndFunction* GetNewSndFunction();
    virtual CBEImplementationFile* GetNewImplementationFile();
    virtual CBEHeaderFile* GetNewHeaderFile();
    virtual CBEComponent* GetNewComponent();
    virtual CBEClient* GetNewClient();
    virtual CBERoot* GetNewRoot();
    virtual CBEMarshaller* GetNewMarshaller();
    virtual CBENameSpace* GetNewNameSpace();
    virtual CBEWaitAnyFunction* GetNewReplyAnyWaitAnyFunction();
    virtual CBESizes* GetNewSizes();
    /** \brief provide a new communication class
     *  \return reference to new communication class
     */
    virtual CBECommunication* GetNewCommunication() = 0;
    virtual CBEDispatchFunction* GetNewDispatchFunction();
    virtual CBEMsgBuffer* GetNewMessageBuffer();
    virtual CBEMsgBufferType* GetNewMessageBufferType();
    virtual CTrace* GetNewTrace();
};

#endif // !__DICE_BECLASSFACTORY_H__

/**
 *	\file	dice/src/be/BEClassFactory.h
 *	\brief	contains the declaration of the class CBEClassFactory
 *
 *	\date	01/10/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#include "be/BEObject.h"

// the classes to be created
class CBERoot;
class CBEClient;
class CBEComponent;
class CBETestsuite;
class CBEHeaderFile;
class CBEImplementationFile;
class CBEOperationFunction;
class CBESndFunction;
class CBERcvFunction;
class CBEWaitFunction;
class CBEReplyRcvFunction;
class CBEReplyWaitFunction;
class CBECallFunction;
class CBEUnmarshalFunction;
class CBEComponentFunction;
class CBESwitchCase;
class CBETestFunction;
class CBETestServerFunction;
class CBETestMainFunction;
class CBERcvAnyFunction;
class CBEWaitAnyFunction;
class CBEReplyAnyWaitAnyFunction;
class CBESrvLoopFunction;
class CBEAttribute;
class CBEType;
class CBEOpcodeType;
class CBEMsgBufferType;
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

/**	\class CBEClassFactory
 *	\ingroup backend
 *	\brief the class factory for the back-end classes
 *
 * We use seperate functions for each class, because the alternative is to use some sort of identifier to find out
 * which class to generate. This involves writing a big switch statement. 
 */
class CBEClassFactory : public CBEObject  
{
DECLARE_DYNAMIC(CBEClassFactory);
// Constructor
public:
	/**	\brief constructor
	 *	\param bVerbose true if class should print status output
	 */
	CBEClassFactory(bool bVerbose = false);
	virtual ~CBEClassFactory();
	
    virtual CBETestMainFunction* GetNewTestMainFunction();
    virtual CBETestServerFunction* GetNewTestServerFunction();
    virtual CBETestFunction* GetNewTestFunction();
    virtual CBEOperationFunction* GetNewOperationFunction();
    virtual CBEClass* GetNewClass();
    virtual CBEUserDefinedType* GetNewUserDefinedType();
    virtual CBEMsgBufferType* GetNewMessageBufferType();
    virtual CBESwitchCase* GetNewSwitchCase();
    virtual CBEComponentFunction* GetNewComponentFunction();
    virtual CBEOpcodeType* GetNewOpcodeType();
    virtual CBEUnmarshalFunction* GetNewUnmarshalFunction();
    virtual CBEWaitAnyFunction* GetNewWaitAnyFunction();
    virtual CBERcvAnyFunction* GetNewRcvAnyFunction();
    virtual CBESrvLoopFunction* GetNewSrvLoopFunction();
    virtual CBEConstant* GetNewConstant();
    virtual CBECallFunction* GetNewCallFunction();
    virtual CBETypedef* GetNewTypedef();
    virtual CBEExpression* GetNewExpression();
    virtual CBEDeclarator* GetNewDeclarator();
    virtual CBEUnionCase* GetNewUnionCase();
    virtual CBEException* GetNewException();
    virtual CBETypedDeclarator* GetNewTypedDeclarator();
    virtual CBEType* GetNewType(int nType);
    virtual CBEAttribute* GetNewAttribute();
    virtual CBEReplyWaitFunction* GetNewReplyWaitFunction();
    virtual CBEReplyRcvFunction* GetNewReplyRcvFunction();
    virtual CBEWaitFunction* GetNewWaitFunction();
    virtual CBERcvFunction* GetNewRcvFunction();
    virtual CBESndFunction* GetNewSndFunction();
    virtual CBEImplementationFile* GetNewImplementationFile();
    virtual CBEHeaderFile* GetNewHeaderFile();
    virtual CBETestsuite* GetNewTestsuite();
    virtual CBEComponent* GetNewComponent();
    virtual CBEClient* GetNewClient();
    virtual CBERoot* GetNewRoot();
    virtual CBEMarshaller* GetNewMarshaller(CBEContext *pContext);
    virtual CBENameSpace* GetNewNameSpace();
    virtual CBEReplyAnyWaitAnyFunction* GetNewReplyAnyWaitAnyFunction();
    virtual CBESizes* GetNewSizes();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBEClassFactory(CBEClassFactory &src);

protected:
	/**	\var bool m_bVerbose
	 *	\brief is true if this class should output verbose stuff
	 */
	bool m_bVerbose;
};

#endif // !__DICE_BECLASSFACTORY_H__

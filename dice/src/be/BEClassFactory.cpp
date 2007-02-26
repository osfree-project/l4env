/**
 *	\file	dice/src/be/BEClassFactory.cpp
 *	\brief	contains the implementation of the class CBEClassFactory
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

#include "be/BEClassFactory.h"
#include "be/BENameFactory.h"
#include "be/BEClassFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BETarget.h"
#include "be/BERoot.h"
#include "be/BEClient.h"
#include "be/BEComponent.h"
#include "be/BETestsuite.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEFunction.h"
#include "be/BEOperationFunction.h"
#include "be/BEInterfaceFunction.h"
#include "be/BESndFunction.h"
#include "be/BERcvFunction.h"
#include "be/BEWaitFunction.h"
#include "be/BEReplyRcvFunction.h"
#include "be/BEReplyWaitFunction.h"
#include "be/BECallFunction.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BEComponentFunction.h"
#include "be/BESwitchCase.h"
#include "be/BETestFunction.h"
#include "be/BETestMainFunction.h"
#include "be/BETestServerFunction.h"
#include "be/BERcvAnyFunction.h"
#include "be/BEWaitAnyFunction.h"
#include "be/BEReplyAnyWaitAnyFunction.h"
#include "be/BESrvLoopFunction.h"
#include "be/BEAttribute.h"
#include "be/BEType.h"
#include "be/BEOpcodeType.h"
#include "be/BETypedDeclarator.h"
#include "be/BETypedef.h"
#include "be/BEException.h"
#include "be/BEStructType.h"
#include "be/BEMsgBufferType.h"
#include "be/BEUnionType.h"
#include "be/BEUnionCase.h"
#include "be/BEUserDefinedType.h"
#include "be/BEDeclarator.h"
#include "be/BEExpression.h"
#include "be/BEConstant.h"
#include "be/BEClass.h"
#include "be/BEMarshaller.h"
#include "be/BEO1Marshaller.h"
#include "be/BENameSpace.h"
#include "be/BEEnumType.h"
#include "be/BESizes.h"
#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CBEClassFactory);

CBEClassFactory::CBEClassFactory(bool bVerbose)
{
    m_bVerbose = bVerbose;
    IMPLEMENT_DYNAMIC_BASE(CBEClassFactory, CBEObject);
}

CBEClassFactory::CBEClassFactory(CBEClassFactory & src):CBEObject(src)
{
    m_bVerbose = src.m_bVerbose;
    IMPLEMENT_DYNAMIC_BASE(CBEClassFactory, CBEObject);
}

/**	\brief the destructor of this class */
CBEClassFactory::~CBEClassFactory()
{

}

/**	\brief creates a new instance of the class CBERoot
 *	\return a reference to the new instance
 */
CBERoot *CBEClassFactory::GetNewRoot()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CBERoot\n");
    return new CBERoot();
}

/**	\brief creates a new instance of the class CBEClient
 *	\return a reference to the new instance
 */
CBEClient *CBEClassFactory::GetNewClient()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CBEClient\n");
    return new CBEClient();
}

/**	\brief creates a new instance of the class CBEComponent
 *	\return a reference to the new instance
 */
CBEComponent *CBEClassFactory::GetNewComponent()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CBEComponent\n");
    return new CBEComponent();
}

/**	\brief creates a new instance of the class CBETestsuite
 *	\return a reference to the new instance
 */
CBETestsuite *CBEClassFactory::GetNewTestsuite()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBETestsuite\n");
    return new CBETestsuite();
}

/**	\brief creates a new instance of the class CBEHeaderFile
 *	\return a reference to the new instance
 */
CBEHeaderFile *CBEClassFactory::GetNewHeaderFile()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CBEHeaderFile\n");
    return new CBEHeaderFile();
}

/**	\brief creates a new instance of the class CBEImplementationFile
 *	\return a reference to the new instance
 */
CBEImplementationFile *CBEClassFactory::GetNewImplementationFile()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CBEImplementationFile\n");
    return new CBEImplementationFile();
}

/**	\brief creates a new instance of the class CBESndFunction
 *	\return a reference to the new instance
 */
CBESndFunction *CBEClassFactory::GetNewSndFunction()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CBESndFunction\n");
    return new CBESndFunction();
}

/**	\brief creates a new instance of the class CBERcvFunction
 *	\return a reference to the new instance
 */
CBERcvFunction *CBEClassFactory::GetNewRcvFunction()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CBERcvFunction\n");
    return new CBERcvFunction();
}

/**	\brief creates a new instance of the class CBEWaitFunction
 *	\return a reference to the new instance
 */
CBEWaitFunction *CBEClassFactory::GetNewWaitFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEWaitFunction\n");
    return new CBEWaitFunction();
}

/**	\brief creates a new instance of the class CBEReplyRcvFunction
 *	\return a reference to the new instance
 */
CBEReplyRcvFunction *CBEClassFactory::GetNewReplyRcvFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEReplyRcvFunction\n");
    return new CBEReplyRcvFunction();
}

/**	\brief creates a new instance of the class CBEReplyWaitFunction
 *	\return a reference to the new instance
 */
CBEReplyWaitFunction *CBEClassFactory::GetNewReplyWaitFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEReplyWaitFunction\n");
    return new CBEReplyWaitFunction();
}

/**	\brief creates a new instance of the class CBEAttribute
 *	\return a reference to the new instance
 */
CBEAttribute *CBEClassFactory::GetNewAttribute()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEAttribute\n");
    return new CBEAttribute();
}

/**	\brief creates a new instance of the class CBEType
 *	\param nType specifies the front-end type
 *	\return a reference to the new instance
 *
 * The front-end type defines which back-end type class is created.
 */
CBEType *CBEClassFactory::GetNewType(int nType)
{
    switch (nType)
    {
    case TYPE_STRUCT:
    case TYPE_TAGGED_STRUCT:
        if (m_bVerbose)
            printf("CBEClassFactory: created class CBEStructType\n");
        return new CBEStructType();
        break;
    case TYPE_UNION:
    case TYPE_TAGGED_UNION:
        if (m_bVerbose)
            printf("CBEClassFactory: created class CBEunionType\n");
        return new CBEUnionType();
        break;
    case TYPE_ENUM:
    case TYPE_TAGGED_ENUM:
        if (m_bVerbose)
            printf("CBEClassFactory: created class CBEEnumType\n");
        return new CBEEnumType();
        break;
    case TYPE_USER_DEFINED:
        if (m_bVerbose)
            printf("CBEClassFactory: created class CBEUserDefinedType\n");
        return GetNewUserDefinedType();
        break;
    case TYPE_NONE:
    case TYPE_FLEXPAGE:
    case TYPE_RCV_FLEXPAGE:
    case TYPE_INTEGER:
    case TYPE_VOID:
    case TYPE_FLOAT:
    case TYPE_DOUBLE:
    case TYPE_LONG_DOUBLE:
    case TYPE_WCHAR:
    case TYPE_CHAR:
    case TYPE_BOOLEAN:
    case TYPE_BYTE:
    case TYPE_VOID_ASTERISK:
    case TYPE_CHAR_ASTERISK:
    case TYPE_PIPE:
    case TYPE_HANDLE_T:
    case TYPE_ISO_LATIN_1:
    case TYPE_ISO_MULTILINGUAL:
    case TYPE_ISO_UCS:
    case TYPE_ERROR_STATUS_T:
    case TYPE_OCTET:
    case TYPE_ANY:
    case TYPE_OBJECT:
    case TYPE_STRING:
    case TYPE_WSTRING:
    case TYPE_ARRAY:
    case TYPE_REFSTRING:
    default:
        break;
    }
    if (m_bVerbose)
        printf("CBEClassFactory: created class CBEType\n");
    return new CBEType();
}

/**	\brief creates a new instance of the class CBETypedDeclarator
 *	\return a reference to the new instance
 */
CBETypedDeclarator *CBEClassFactory::GetNewTypedDeclarator()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBETypedDeclarator\n");
    return new CBETypedDeclarator();
}

/**	\brief creates a new instance of the class CBEException
 *	\return a reference to the new instance
 */
CBEException *CBEClassFactory::GetNewException()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEException\n");
    return new CBEException();
}

/**	\brief creates a new instance of the class CBEUnionCase
 *	\return a reference to the new instance
 */
CBEUnionCase *CBEClassFactory::GetNewUnionCase()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEUnionCase\n");
    return new CBEUnionCase();
}

/**	\brief creates a new instance of the class CBEDeclarator
 *	\return a reference to the new instance
 */
CBEDeclarator *CBEClassFactory::GetNewDeclarator()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEDeclarator\n");
    return new CBEDeclarator();
}

/**	\brief creates a new instance of the class CBEExpression
 *	\return a reference to the new instance
 */
CBEExpression *CBEClassFactory::GetNewExpression()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEExpression\n");
    return new CBEExpression();
}

/**	\brief creates a new instance of the class CBETypedef
 *	\return a reference to the new instance
 */
CBETypedef *CBEClassFactory::GetNewTypedef()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBETypedef\n");
    return new CBETypedef();
}

/**	\brief creates a new instance of the class CBECallFunction
 *	\return a reference to the new instance
 */
CBECallFunction *CBEClassFactory::GetNewCallFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBECallFunction\n");
    return new CBECallFunction();
}

/**	\brief creates a new instance of the class CBEConstant
 *	\return a reference to the new instance
 */
CBEConstant *CBEClassFactory::GetNewConstant()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEConstant\n");
    return new CBEConstant();
}

/**	\brief creates a new instance of the class CBESrvLoopFunction
 *	\return a reference to the new instance
 */
CBESrvLoopFunction *CBEClassFactory::GetNewSrvLoopFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBESrvLoopFunction\n");
    return new CBESrvLoopFunction();
}

/**	\brief creates a new instance of the class CBERcvAnyFunction
 *	\return a reference to the new instance
 */
CBERcvAnyFunction *CBEClassFactory::GetNewRcvAnyFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBERcvAnyFunction\n");
    return new CBERcvAnyFunction();
}

/**	\brief creates a new instance of the class CBEWaitAnyFunction
 *	\return a reference to the new instance
 */
CBEWaitAnyFunction *CBEClassFactory::GetNewWaitAnyFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEWaitAnyFunction\n");
    return new CBEWaitAnyFunction();
}

/**	\brief creates a new instance of the class CBEUnmarshalFunction
 *	\return a reference to the new instance
 */
CBEUnmarshalFunction *CBEClassFactory::GetNewUnmarshalFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEUnmarshalFunction\n");
    return new CBEUnmarshalFunction();
}

/**	\brief creates a new instance of the class CBEOpcodeType
 *	\return a reference to the new instance
 */
CBEOpcodeType *CBEClassFactory::GetNewOpcodeType()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEOpcodeType\n");
    return new CBEOpcodeType();
}

/**	\brief creates a new instance of the class CBEComponentFunction
 *	\return a reference to the new instance
 */
CBEComponentFunction *CBEClassFactory::GetNewComponentFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEComponentFunction\n");
    return new CBEComponentFunction();
}

/**	\brief creates a new instance of the class CBESwitchCase
 *	\return a reference to the new instance
 */
CBESwitchCase *CBEClassFactory::GetNewSwitchCase()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBESwitchCase\n");
    return new CBESwitchCase();
}

/**	\brief creates a new instance of the class CBEMsgBufferType
 *	\return a reference to the new instance
 */
CBEMsgBufferType *CBEClassFactory::GetNewMessageBufferType()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEMsgBufferType\n");
    return new CBEMsgBufferType();
}

/**	\brief creates a new instance of the class CBEUserDefinedType
 *	\return a reference to the new instance
 */
CBEUserDefinedType *CBEClassFactory::GetNewUserDefinedType()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CBEUserDefinedType\n");
    return new CBEUserDefinedType();
}

/**	\brief creates a new instance of the class CBEClass
 *	\return a reference to the new instance
 */
CBEClass *CBEClassFactory::GetNewClass()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CBEClass\n");
    return new CBEClass();
}

/**	\brief creates a new instance of the class CBEOperationFunction
 *	\return a reference to the new instance
 */
CBEOperationFunction *CBEClassFactory::GetNewOperationFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBEOperationFunction\n");
    return new CBEOperationFunction();
}

/**	\brief creates a new instance of the class CBETestFunction
 *	\return a reference to the new instance
 */
CBETestFunction *CBEClassFactory::GetNewTestFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBETestFunction\n");
    return new CBETestFunction();
}

/**	\brief creates a new instance of the class CBETestMainFunction
 *	\return a reference to the new instance
 */
CBETestMainFunction *CBEClassFactory::GetNewTestMainFunction()
{
    if (m_bVerbose)
	printf("CBEClassFactory: created class CBETestMainFunction\n");
    return new CBETestMainFunction();
}

/**	\brief creates a new instance of the class CBETestServerFunction
 *	\return a reference to the new instance
 */
CBETestServerFunction *CBEClassFactory::GetNewTestServerFunction()
{
    if (m_bVerbose)
	    printf("CBEClassFactory: created class CBETestServerFunction\n");
    return new CBETestServerFunction();
}

/** \brief creates a new instance of the class CBEMarshaller
 *  \return a reference to the new instance
 */
CBEMarshaller* CBEClassFactory::GetNewMarshaller(CBEContext *pContext)
{
    int nOptimizeLevel = pContext->GetOptimizeLevel();
    CBEMarshaller *pMarshaller = 0;
    switch (nOptimizeLevel)
    {
    case 0:
        pMarshaller = new CBEMarshaller();
        break;
    case 1:
    case 2:
        pMarshaller = new CBEO1Marshaller();
        break;
    default:
        pMarshaller = new CBEMarshaller();
        break;
    }
    if (m_bVerbose)
        printf("CBEClassFactory: create class %s for optimization level %d\n",
               (const char*)pMarshaller->GetClassName(), nOptimizeLevel);
    return pMarshaller;
}

/** \brief creates a new instance of the class CBENameSpace
 *  \return a reference to the new instance
 */
CBENameSpace* CBEClassFactory::GetNewNameSpace()
{
    if (m_bVerbose)
        printf("CBEClassFactory: created class CBENameSpace\n");
    return new CBENameSpace();
}

/** \brief creates a new instance of the class CBEReplyAnyWayitAnyFunction
 *  \return a reference to the new instance
 */
CBEReplyAnyWaitAnyFunction* CBEClassFactory::GetNewReplyAnyWaitAnyFunction()
{
    if (m_bVerbose)
        printf("CBEClassFactory: create class CBEReplyAnyWayitAnyFunction\n");
    return new CBEReplyAnyWaitAnyFunction();
}

/** \brief creates a new instance of the class CBESizes
 *  \return a reference to the new instance
 */
CBESizes* CBEClassFactory::GetNewSizes()
{
    if (m_bVerbose)
        printf("CBEClassFactory: create class CBESizes\n");
    return new CBESizes();
}

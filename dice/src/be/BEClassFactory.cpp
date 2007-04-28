/**
 *  \file    dice/src/be/BEClassFactory.cpp
 *  \brief   contains the implementation of the class CBEClassFactory
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

#include "BEClassFactory.h"
#include "BEClass.h"
#include "BETypedDeclarator.h"
#include "BETypedef.h"
#include "BEMsgBuffer.h"
#include "BENameSpace.h"
#include "BERoot.h"
#include "BEClient.h"
#include "BEComponent.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BESndFunction.h"
#include "BEWaitFunction.h"
#include "BEReplyFunction.h"
#include "BEAttribute.h"
#include "TypeSpec-Type.h"
#include "BEType.h"
#include "BEException.h"
#include "BEUnionCase.h"
#include "BEDeclarator.h"
#include "BEExpression.h"
#include "BECallFunction.h"
#include "BEConstant.h"
#include "BESrvLoopFunction.h"
#include "BEWaitAnyFunction.h"
#include "BEUnmarshalFunction.h"
#include "BEMarshalFunction.h"
#include "BEOpcodeType.h"
#include "BEReplyCodeType.h"
#include "BEComponentFunction.h"
#include "BESwitchCase.h"
#include "BEUserDefinedType.h"
#include "BEMarshaller.h"
#include "BESizes.h"
#include "BECommunication.h"
#include "BEDispatchFunction.h"
#include "BEMsgBufferType.h"
#include "BEStructType.h"
#include "BEIDLUnionType.h"
#include "BEEnumType.h"
#include "BETrace.h"
#include "Compiler.h"
// for dynamic loadable tracing classes
#include <dlfcn.h>
#include <iostream>

CBEClassFactory::CBEClassFactory()
{
}

/** \brief the destructor of this class */
CBEClassFactory::~CBEClassFactory()
{
}

/** \brief creates a new instance of the class CBERoot
 *  \return a reference to the new instance
 */
CBERoot *CBEClassFactory::GetNewRoot()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEClassFactory: created class CBERoot\n");
    return new CBERoot();
}

/** \brief creates a new instance of the class CBEClient
 *  \return a reference to the new instance
 */
CBEClient *CBEClassFactory::GetNewClient()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEClassFactory: created class CBEClient\n");
    return new CBEClient();
}

/** \brief creates a new instance of the class CBEComponent
 *  \return a reference to the new instance
 */
CBEComponent *CBEClassFactory::GetNewComponent()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEClassFactory: created class CBEComponent\n");
    return new CBEComponent();
}

/** \brief creates a new instance of the class CBEHeaderFile
 *  \return a reference to the new instance
 */
CBEHeaderFile *CBEClassFactory::GetNewHeaderFile()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEHeaderFile\n");
    return new CBEHeaderFile();
}

/** \brief creates a new instance of the class CBEImplementationFile
 *  \return a reference to the new instance
 */
CBEImplementationFile *CBEClassFactory::GetNewImplementationFile()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEImplementationFile\n");
    return new CBEImplementationFile();
}

/** \brief creates a new instance of the class CBESndFunction
 *  \return a reference to the new instance
 */
CBESndFunction *CBEClassFactory::GetNewSndFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBESndFunction\n");
    return new CBESndFunction();
}

/** \brief creates a new instance of the class CBEWaitFunction
 *  \return a reference to the new instance
 */
CBEWaitFunction *CBEClassFactory::GetNewRcvFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEWaitFunction\n");
    return new CBEWaitFunction(false);
}

/** \brief creates a new instance of the class CBEWaitFunction
 *  \return a reference to the new instance
 */
CBEWaitFunction *CBEClassFactory::GetNewWaitFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEWaitFunction\n");
    return new CBEWaitFunction(true);
}

/** \brief creates a new instance of the class CBEReplyFunction
 *  \return a reference to the new instance
 */
CBEReplyFunction *CBEClassFactory::GetNewReplyFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEReplyFunction\n");
    return new CBEReplyFunction();
}

/** \brief creates a new instance of the class CBEAttribute
 *  \return a reference to the new instance
 */
CBEAttribute *CBEClassFactory::GetNewAttribute()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEAttribute\n");
    return new CBEAttribute();
}

/** \brief creates a new instance of the class CBEType
 *  \param nType specifies the front-end type
 *  \return a reference to the new instance
 *
 * The front-end type defines which back-end type class is created.
 */
CBEType *CBEClassFactory::GetNewType(int nType)
{
    switch (nType)
    {
    case TYPE_ARRAY:
    case TYPE_STRUCT:
        CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
            "CBEClassFactory: created class CBEStructType\n");
        return new CBEStructType();
        break;
    case TYPE_IDL_UNION:
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	    "CBEClassFactory: created class CBEIDLUnionType\n");
	return new CBEIDLUnionType();
	break;
    case TYPE_UNION:
        CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
            "CBEClassFactory: created class CBEunionType\n");
        return new CBEUnionType();
        break;
    case TYPE_ENUM:
        CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
            "CBEClassFactory: created class CBEEnumType\n");
        return new CBEEnumType();
        break;
    case TYPE_USER_DEFINED:
        CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
            "CBEClassFactory: created class CBEUserDefinedType\n");
        return GetNewUserDefinedType();
        break;
    case TYPE_NONE:
    case TYPE_FLEXPAGE:
    case TYPE_RCV_FLEXPAGE:
    case TYPE_INTEGER:
    case TYPE_LONG:
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
    case TYPE_REFSTRING:
    default:
        break;
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEType\n");
    return new CBEType();
}

/** \brief creates a new instance of the class CBETypedDeclarator
 *  \return a reference to the new instance
 */
CBETypedDeclarator *CBEClassFactory::GetNewTypedDeclarator()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created new instance of CBETypedDeclarator\n");
    return new CBETypedDeclarator();
}

/** \brief creates a new instance of the class CBETypedef
 *  \return a reference to the new instance
 */
CBETypedef *CBEClassFactory::GetNewTypedef()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created new instance of CBETypedef\n");
    return new CBETypedef();
}


/** \brief creates a new instance of a message buffer
 *  \return a reference to the newly created instance
 */
CBEMsgBuffer* CBEClassFactory::GetNewMessageBuffer()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created new instance of CBEMsgBuffer\n");
    return new CBEMsgBuffer();
}

/** \brief creates a new instance of the class CBEException
 *  \return a reference to the new instance
 */
CBEException *CBEClassFactory::GetNewException()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEException\n");
    return new CBEException();
}

/** \brief creates a new instance of the class CBEUnionCase
 *  \return a reference to the new instance
 */
CBEUnionCase *CBEClassFactory::GetNewUnionCase()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEUnionCase\n");
    return new CBEUnionCase();
}

/** \brief creates a new instance of the class CBEDeclarator
 *  \return a reference to the new instance
 */
CBEDeclarator *CBEClassFactory::GetNewDeclarator()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEDeclarator\n");
    return new CBEDeclarator();
}

/** \brief creates a new instance of the class CBEExpression
 *  \return a reference to the new instance
 */
CBEExpression *CBEClassFactory::GetNewExpression()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEExpression\n");
    return new CBEExpression();
}

/** \brief creates a new instance of the class CBECallFunction
 *  \return a reference to the new instance
 */
CBECallFunction *CBEClassFactory::GetNewCallFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBECallFunction\n");
    return new CBECallFunction();
}

/** \brief creates a new instance of the class CBEConstant
 *  \return a reference to the new instance
 */
CBEConstant *CBEClassFactory::GetNewConstant()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEConstant\n");
    return new CBEConstant();
}

/** \brief creates a new instance of the class CBESrvLoopFunction
 *  \return a reference to the new instance
 */
CBESrvLoopFunction *CBEClassFactory::GetNewSrvLoopFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBESrvLoopFunction\n");
    return new CBESrvLoopFunction();
}

/** \brief creates a new instance of the class CBERcvAnyFunction
 *  \return a reference to the new instance
 */
CBEWaitAnyFunction *CBEClassFactory::GetNewRcvAnyFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBERcvAnyFunction\n");
    return new CBEWaitAnyFunction(false, false);
}

/** \brief creates a new instance of the class CBEWaitAnyFunction
 *  \return a reference to the new instance
 */
CBEWaitAnyFunction *CBEClassFactory::GetNewWaitAnyFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEWaitAnyFunction\n");
    return new CBEWaitAnyFunction(true, false);
}

/** \brief creates a new instance of the class CBEUnmarshalFunction
 *  \return a reference to the new instance
 */
CBEUnmarshalFunction *CBEClassFactory::GetNewUnmarshalFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEUnmarshalFunction\n");
    return new CBEUnmarshalFunction();
}

/** \brief creates a new instance of the class CBEMarshalFunction
 *  \return a reference to the new instance
 */
CBEMarshalFunction *CBEClassFactory::GetNewMarshalFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEMarshalFunction\n");
    return new CBEMarshalFunction();
}

/** \brief creates a new instance of the class CBEOpcodeType
 *  \return a reference to the new instance
 */
CBEOpcodeType *CBEClassFactory::GetNewOpcodeType()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEOpcodeType\n");
    return new CBEOpcodeType();
}

/** \brief creates a new instance of the class CBEOpcodeType
 *  \return a reference to the new instance
 */
CBEReplyCodeType *CBEClassFactory::GetNewReplyCodeType()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEReplyCodeType\n");
    return new CBEReplyCodeType();
}

/** \brief creates a new instance of the class CBEComponentFunction
 *  \return a reference to the new instance
 */
CBEComponentFunction *CBEClassFactory::GetNewComponentFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEComponentFunction\n");
    return new CBEComponentFunction();
}

/** \brief creates a new instance of the class CBESwitchCase
 *  \return a reference to the new instance
 */
CBESwitchCase *CBEClassFactory::GetNewSwitchCase()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBESwitchCase\n");
    return new CBESwitchCase();
}

/** \brief creates a new instance of the class CBEUserDefinedType
 *  \return a reference to the new instance
 */
CBEUserDefinedType *CBEClassFactory::GetNewUserDefinedType()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBEUserDefinedType\n");
    return new CBEUserDefinedType();
}

/** \brief creates a new instance of the class CBEClass
 *  \return a reference to the new instance
 */
CBEClass *CBEClassFactory::GetNewClass()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: create new CBEClass\n");
    return new CBEClass();
}

/** \brief creates a new instance of the class CBEMarshaller
 *  \return a reference to the new instance
 */
CBEMarshaller* CBEClassFactory::GetNewMarshaller()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: create class CBEMarshaller\n");
    return new CBEMarshaller();
}

/** \brief creates a new instance of the class CBENameSpace
 *  \return a reference to the new instance
 */
CBENameSpace* CBEClassFactory::GetNewNameSpace()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: created class CBENameSpace\n");
    return new CBENameSpace();
}

/** \brief creates a new instance of the class CBEReplyAnyWayitAnyFunction
 *  \return a reference to the new instance
 */
CBEWaitAnyFunction* CBEClassFactory::GetNewReplyAnyWaitAnyFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: create class CBEReplyAnyWayitAnyFunction\n");
    return new CBEWaitAnyFunction(true /* open wait*/, true /* reply */);
}

/** \brief creates a new instance of the class CBESizes
 *  \return a reference to the new instance
 */
CBESizes* CBEClassFactory::GetNewSizes()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: create class CBESizes\n");
    return new CBESizes();
}

/** \brief creates a new instance of the class CBEDispatchFunction
 *  \return a reference to the new instance
 */
CBEDispatchFunction* CBEClassFactory::GetNewDispatchFunction()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: create class CBEDispatchFunction\n");
    return new CBEDispatchFunction();
}

/** \brief creates a new instance of the message buffer type
 *  \return a reference to the newly created class
 */
CBEMsgBufferType* CBEClassFactory::GetNewMessageBufferType()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: create class CBEMsgBufferType\n");
    return new CBEMsgBufferType();
}

/** \brief creates a new instance of the tracing class CBETrace
 *  \return a reference to the tracing class
 */
CBETrace* CBEClassFactory::GetNewTraceFromLib()
{
    string sTraceLib;
    if (!CCompiler::GetBackEndOption(string("trace-lib"), sTraceLib))
	return (CBETrace*)0;

    // get handle for lib
    void *lib = dlopen(sTraceLib.c_str(), RTLD_NOW);
    if (lib == NULL)
	return (CBETrace*)0;

    // get symbol for create function
    CBETrace* (*func)(void);
    func = (CBETrace* (*)(void))dlsym(lib, "dice_tracing_new_class");
    // use error message as error indicator
    const char *errmsg = dlerror();
    if (errmsg != NULL)
    {
	std::cerr << errmsg;
	return (CBETrace*)0;
    }

    // call factory function
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBEClassFactory: create class CBETrace from lib \"%s\".\n",
	sTraceLib.c_str());
    return (*func) ();
}

/** \brief creates a new instance of the trace class
 *  \return a reference to the newly created class
 */
CBETrace* CBEClassFactory::GetNewTrace()
{
    CBETrace *pRet = GetNewTraceFromLib();
    if (pRet)
	return pRet;
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
        "CBEClassFactory: create class CBETrace\n");
    return new CBETrace();
}


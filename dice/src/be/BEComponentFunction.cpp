/**
 *  \file    dice/src/be/BEComponentFunction.cpp
 *  \brief   contains the implementation of the class CBEComponentFunction
 *
 *  \date    01/18/2002
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

#include "BEComponentFunction.h"
#include "BEContext.h"
#include "BEFile.h"
#include "BEType.h"
#include "BEDeclarator.h"
#include "BETypedDeclarator.h"
#include "BERoot.h"
#include "BEClass.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEHeaderFile.h"
#include "BEComponent.h"
#include "BEExpression.h"
#include "BEReplyCodeType.h"
#include "BEAttribute.h"
#include "Compiler.h"
#include "Error.h"
#include "Attribute-Type.h"
#include "TypeSpec-Type.h"
#include "fe/FEOperation.h"
#include "fe/FEFile.h"
#include "fe/FEExpression.h"
#include <sstream>
#include <cassert>

CBEComponentFunction::CBEComponentFunction()
 : CBEOperationFunction(FUNCTION_TEMPLATE)
{
    m_pFunction = 0;
    if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
	m_nSkipParameter = 0;
    if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
	m_nSkipParameter = 3;
}

CBEComponentFunction::CBEComponentFunction(CBEComponentFunction & src)
 : CBEOperationFunction(src)
{
    m_pFunction = 0;
    m_nSkipParameter = src.m_nSkipParameter;
}

/** \brief destructor of target class */
CBEComponentFunction::~CBEComponentFunction()
{ }

/** \brief creates the call function
 *  \param pFEOperation the front-end operation used as reference
 *  \return true if successful
 *
 * This implementation only sets the name of the function. And it stores a
 * reference to the client side function in case this implementation is
 * tested.
 */
void
CBEComponentFunction::CreateBackEnd(CFEOperation * pFEOperation)
{
    // set target file name
    SetTargetFileName(pFEOperation);
    // get own name
    SetFunctionName(pFEOperation, FUNCTION_TEMPLATE);

    CBEOperationFunction::CreateBackEnd(pFEOperation);

    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);

    // check the attribute
    FUNCTION_TYPE nFunctionType = FUNCTION_NONE;
    if (pFEOperation->m_Attributes.Find(ATTR_IN))
	nFunctionType = FUNCTION_SEND;
    else
	nFunctionType = FUNCTION_CALL;

    string exc = string(__func__);
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sFunctionName = pNF->GetFunctionName(pFEOperation, nFunctionType);
    m_pFunction = pRoot->FindFunction(sFunctionName, nFunctionType);
    if (!m_pFunction)
    {
	exc += " failed because component's function (" + sFunctionName +
	    ") could not be found.";
        throw new error::create_error(exc);
    }

    // the return value "belongs" to the client function (needed to determine
    // global test variable's name)
    CBETypedDeclarator *pReturn = GetReturnVariable();
    pReturn->SetParent(m_pFunction);

    // check for temp
    if (m_pFunction->HasVariableSizedParameters(DIRECTION_INOUT) ||
        m_pFunction->HasArrayParameters(DIRECTION_INOUT))
    {
        int nVariableSizedArrayDimensions = 0;
        vector<CBETypedDeclarator*>::iterator iterP;
	for (iterP = m_pFunction->m_Parameters.begin();
	     iterP != m_pFunction->m_Parameters.end();
	     iterP++)
        {
            // now check each decl for array dimensions
            vector<CBEDeclarator*>::iterator iterD;
	    for (iterD = (*iterP)->m_Declarators.begin();
		 iterD != (*iterP)->m_Declarators.end();
		 iterD++)
            {
                int nArrayDims = (*iterD)->GetStars();
                // get array bounds
                vector<CBEExpression*>::iterator iterB;
		for (iterB = (*iterD)->m_Bounds.begin();
		     iterB != (*iterD)->m_Bounds.end();
		     iterB++)
                {
                    if (!(*iterB)->IsOfType(EXPR_INT))
                        nArrayDims++;
                }
                // calc max
                nVariableSizedArrayDimensions =
		    (nArrayDims > nVariableSizedArrayDimensions) ?
		    nArrayDims : nVariableSizedArrayDimensions;
            }
            // if type of parameter is array, check that too
            if ((*iterP)->GetType()->GetSize() < 0)
                nVariableSizedArrayDimensions++;
            // if array dims
            // if parameter has size attributes, we assume
            // that it is an array of some sort
            if (((*iterP)->m_Attributes.Find(ATTR_SIZE_IS) ||
                (*iterP)->m_Attributes.Find(ATTR_LENGTH_IS)) &&
                (nVariableSizedArrayDimensions == 0))
                nVariableSizedArrayDimensions = 1;
        }

	string sCurr;
	// for variable sized arrays we need a temporary variable
	string sTmpVar = pNF->GetTempOffsetVariable();
	for (int i=0; i < nVariableSizedArrayDimensions; i++)
	{
	    std::ostringstream os;
	    os << i;
	    sCurr = sTmpVar + os.str();
	    AddLocalVariable(TYPE_INTEGER, true, 4, sCurr, 0);

	    CBETypedDeclarator *pVariable = m_LocalVariables.Find(sCurr);
	    pVariable->AddLanguageProperty(string("attribute"),
		string("__attribute__ ((unused))"));
	}

	// need a "pure" temp var as well
	sCurr = sTmpVar;
	AddLocalVariable(TYPE_INTEGER, true, 4, sTmpVar, 0);

	CBETypedDeclarator *pVariable = m_LocalVariables.Find(sTmpVar);
	pVariable->AddLanguageProperty(string("attribute"),
	    string("__attribute__ ((unused))"));

	sCurr = pNF->GetOffsetVariable();
	AddLocalVariable(TYPE_INTEGER, true, 4, sCurr, 0);
	pVariable = m_LocalVariables.Find(sCurr);
	pVariable->AddLanguageProperty(string("attribute"),
	    string("__attribute__ ((unused))"));
    }
}

/** \brief adds parameters before all other parameters
 *
 * The CORBA C mapping specifies a CORBA_object to appear as first parameter.
 * The component function has a _non_const CORBA_Object.
 */
void
CBEComponentFunction::AddBeforeParameters(void)
{
    CBEOperationFunction::AddBeforeParameters();
    // add no const C attribute
    CBETypedDeclarator *pObj = GetObject();
    if (pObj)
	pObj->AddLanguageProperty(string("noconst"), string());
}

/** \brief add parameters after other parameters
 *  \return true if successful
 */
void
CBEComponentFunction::AddAfterParameters()
{
    if (m_Attributes.Find(ATTR_ALLOW_REPLY_ONLY))
    {
	// return type -> set to IPC reply code
	CBEClassFactory *pCF = CCompiler::GetClassFactory();
	CBEReplyCodeType *pReplyType = pCF->GetNewReplyCodeType();
	pReplyType->CreateBackEnd();

	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sReply = pNF->GetReplyCodeVariable();
	CBETypedDeclarator *pReplyVar = new CBETypedDeclarator();
	pReplyVar->CreateBackEnd(pReplyType, sReply);
	// delete type: cloned by typed decl create function
	delete pReplyType;

	// make dice-reply a reference, so it can be set in the component
	// function
	CBEDeclarator *pDecl = pReplyVar->m_Declarators.First();
	pDecl->SetStars(1);
	// add before SetCallVariable, so this method will find the parameter
	// to set the call variable for
        m_Parameters.Add(pReplyVar);
	pReplyVar->AddLanguageProperty(string("noconst"), string());

	// we are not allowed to set the call variable here, because that
	// would start copying the parameters to the call parameter list,
	// which would omit anything defined in "AddAfterParameters"...
	// Do it later.
    }

    CBEOperationFunction::AddAfterParameters();
}

/** \brief writes the variable declarations of this function
 *  \param pFile the file to write to
 *
 * The variable declarations of the component function skeleton is usually
 * empty.  If we write the test-skeleton and have a variable sized parameter,
 * we need a temporary variable.
 */
void
CBEComponentFunction::WriteVariableDeclaration(CBEFile& pFile)
{
    pFile << "\t#warning \"" << GetName() << " is not implemented!\"\n";
    return;
}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * This implementation should initialize the message buffer and the pointers
 * of the out variables.
 */
void
CBEComponentFunction::WriteVariableInitialization(CBEFile& /*pFile*/)
{}

/** \brief writes the marshalling of the message
 *  \param pFile the file to write to
 *
 * This implementation does not use the base class' marshal mechanisms,
 * because it does something totally different. It write test-suite's compare
 * operations instead of parameter marshalling.
 */
void
CBEComponentFunction::WriteMarshalling(CBEFile& /*pFile*/)
{}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void
CBEComponentFunction::WriteInvocation(CBEFile& /*pFile*/)
{}

/** \brief writes the unmarshalling of the message
 *  \param pFile the file to write to
 *
 * This implementation should unpack the out parameters from the returned
 * message structure
 */
void
CBEComponentFunction::WriteUnmarshalling(CBEFile& /*pFile*/)
{}

/** \brief writes the return statement
 *  \param pFile the file to write to
 *
 * This implementation should write the return statement if one is necessary.
 * (return type != void)
 */
void CBEComponentFunction::WriteReturn(CBEFile& /*pFile*/)
{}

/** \brief writes the declaration of a function to the target file
 *  \param pFile the target file to write to
 *
 * For C write normal function declaration. For C++ write abstract function.
 */
void
CBEComponentFunction::WriteFunctionDeclaration(CBEFile& pFile)
{
    if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
    {
	CBEOperationFunction::WriteFunctionDeclaration(pFile);
	return;
    }

    if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
    {
	// CPP TODOs:
	// TODO: component functions at server side could be pure virtual to be
	// overloadable
	// TODO: interface functions and component functions are public,
	// everything else should be protected

	m_nParameterIndent = pFile.GetIndent();
	pFile << "\tvirtual ";
	// <return type>
	WriteReturnType(pFile);
	// in the header file we add function attributes
	WriteFunctionAttributes(pFile);
	pFile << "\n";
	// <name> (
	pFile << "\t" << GetName() << " (";
	m_nParameterIndent += GetName().length() + 2;

	// <parameter list>
	if (!WriteParameterList(pFile))
	    pFile << "void";

	// ); newline
	pFile << ") = 0;\n";
    }
}

/** \brief writes the definition of the function to the target file
 *  \param pFile the target file to write to
 *
 * If C backend then write definition as empty body with compile time warning.
 * If C++ do write abstract function declaration if header file otherwise
 * write nothing.
 */
void
CBEComponentFunction::WriteFunctionDefinition(CBEFile& pFile)
{
    if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
    {
	CBEOperationFunction::WriteFunctionDefinition(pFile);
	return;
    }
}

/** \brief test if function should be written inline
 *  \param pFile the file to write to
 *
 * Never write inline function for component function.
 */
bool
CBEComponentFunction::DoWriteFunctionInline(CBEFile& /*pFile*/)
{
    return false;
}

/** \brief add this function to the implementation file
 *  \param pImpl the implementation file
 *  \return true if successful
 *
 * A component function is only added if the create-skeleton
 * option is set.
 */
void
CBEComponentFunction::AddToImpl(CBEImplementationFile* pImpl)
{
    if (!CCompiler::IsOptionSet(PROGRAM_GENERATE_TEMPLATE))
        return;  // fake success, without adding function
    return CBEOperationFunction::AddToImpl(pImpl);
}

/** \brief set the target file name for this function
 *  \param pFEObject the front-end reference object
 *
 * The implementation file for a component-function (server skeleton) is
 * the FILETYPE_TEMPLATE file.
 */
void
CBEComponentFunction::SetTargetFileName(CFEBase * pFEObject)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    CBEOperationFunction::SetTargetFileName(pFEObject);
    if (!dynamic_cast<CFEFile*>(pFEObject))
	pFEObject = pFEObject->GetSpecificParent<CFEFile>(0);
    m_sTargetImplementation = pNF->GetFileName(pFEObject, FILETYPE_TEMPLATE);
}

/** \brief test the target file name with the locally stored file name
 *  \param pFile the file, which's file name is used for the comparison
 *  \return true if is the target file
 */
bool
CBEComponentFunction::IsTargetFile(CBEImplementationFile* pFile)
{
    long length = m_sTargetImplementation.length();
    if (!pFile->IsOfFileType(FILETYPE_TEMPLATE))
	return false;
    // check internal (local) name
    if ((m_sTargetImplementation.substr(length - 11) != "-template.c") &&
	(m_sTargetImplementation.substr(length - 12) != "-template.cc"))
        return false;
    string sBaseLocal = m_sTargetImplementation.substr(0, length - 11);
    // check filename
    string sBaseTarget = pFile->GetFileName();
    length = sBaseTarget.length();
    if (length <= 11)
        return false;
    if ((sBaseTarget.substr(length - 11) != "-template.c") &&
	(sBaseTarget.substr(length - 12) != "-template.cc"))
	return false;
    sBaseTarget = sBaseTarget.substr(0, length - 11);
    // compare common parts
    if (sBaseLocal == sBaseTarget)
	return true;
    return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if successful
 *
 * A component function is written to an implementation file only if the
 * options PROGRAM_GENERATE_TEMPLATE are set.
 * It is always written to an header file. These two conditions are only true
 * for the component's side. (The function would not have been created if the
 * attributes (IN,OUT) were not empty).
 */
bool
CBEComponentFunction::DoWriteFunction(CBEHeaderFile* pFile)
{
    if (!pFile->IsOfFileType(FILETYPE_COMPONENT))
        return false;
    if (!CBEOperationFunction::IsTargetFile(pFile))
        return false;
    return true;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if successful
 *
 * A component function is written to an implementation file only if the
 * options PROGRAM_GENERATE_TEMPLATE are set.
 * It is always written to an header file. These two conditions are only true
 * for the component's side. (The function would not have been created if the
 * attributes (IN,OUT) were not empty).
 */
bool
CBEComponentFunction::DoWriteFunction(CBEImplementationFile* pFile)
{
    if (!pFile->IsOfFileType(FILETYPE_COMPONENT))
        return false;
    if (!IsTargetFile(pFile))
        return false;
    return CCompiler::IsOptionSet(PROGRAM_GENERATE_TEMPLATE);
}

/** \brief test if a specific parameter should be tested
 *  \param pParameter the parameter to test
 *  \return true if this parameter should be tested
 */
bool
CBEComponentFunction::DoTestParameter(CBETypedDeclarator *pParameter)
{
    if (pParameter == m_pFunction->GetObject())
	return false;
    if (pParameter == m_pFunction->GetEnvironment())
	return false;
    return true;
}

/** \brief check if parameter should be written
 *  \param pParam the parameter to test
 *  \return true if writing param, false if not
 *
 * Do not write CORBA_Object and CORBA_Env depending on m_nSkipParameter map.
 */
bool
CBEComponentFunction::DoWriteParameter(CBETypedDeclarator *pParam)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"%s(%s) called, m_nSkipParameter = %d\n", __func__,
	pParam->m_Declarators.First()->GetName().c_str(), m_nSkipParameter);
    if ((m_nSkipParameter & 1) &&
	pParam == GetObject())
	return false;
    if ((m_nSkipParameter & 2) &&
	pParam == GetEnvironment())
	return false;
    return CBEOperationFunction::DoWriteParameter(pParam);
}


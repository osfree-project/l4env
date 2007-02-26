/**
 *	\file	dice/src/be/BEComponentFunction.cpp
 *	\brief	contains the implementation of the class CBEComponentFunction
 *
 *	\date	01/18/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#include "be/BEComponentFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BERoot.h"
#include "be/BETestFunction.h"
#include "be/BEClass.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEComponent.h"
#include "be/BEExpression.h"

#include "fe/FEAttribute.h"
#include "TypeSpec-Type.h"
#include "fe/FEOperation.h"
#include "fe/FEFile.h"
#include "fe/FEExpression.h"

IMPLEMENT_DYNAMIC(CBEComponentFunction);

CBEComponentFunction::CBEComponentFunction()
{
    m_pFunction = 0;
    IMPLEMENT_DYNAMIC_BASE(CBEComponentFunction, CBEOperationFunction);
}

CBEComponentFunction::CBEComponentFunction(CBEComponentFunction & src)
: CBEOperationFunction(src)
{
    m_pFunction = 0;
    IMPLEMENT_DYNAMIC_BASE(CBEComponentFunction, CBEOperationFunction);
}

/**	\brief destructor of target class */
CBEComponentFunction::~CBEComponentFunction()
{

}

/**	\brief creates the call function
 *	\param pFEOperation the front-end operation used as reference
 *	\param pContext the context of the write operation
 *	\return true if successful
 *
 * This implementation only sets the name of the function. And it stores a reference to
 * the client side function in case this implementation is tested.
 */
bool CBEComponentFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_TEMPLATE);
	// set target file name
	SetTargetFileName(pFEOperation, pContext);
    // get own name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);

    if (!CBEOperationFunction::CreateBackEnd(pFEOperation, pContext))
        return false;

    CBERoot *pRoot = GetRoot();
    assert(pRoot);

    // check the attribute
    if (pFEOperation->FindAttribute(ATTR_IN))
        pContext->SetFunctionType(FUNCTION_SEND);
    else
        pContext->SetFunctionType(FUNCTION_CALL);

    String sFunctionName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
    m_pFunction = pRoot->FindFunction(sFunctionName);
    if (!m_pFunction)
    {
        VERBOSE("CBEComponentFunction::CreateBackEnd failed because component's function (%s) could not be found\n",
                (const char*)sFunctionName);
        return false;
    }

    // the return value "belongs" to the client function (needed to determine global test variable's name)
    m_pReturnVar->SetParent(m_pFunction);

    return true;
}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The variable declarations of the component function skeleton is usually empty.
 * If we write the test-skeleton and have a variable sized parameter, we need a temporary
 * variable.
 */
void CBEComponentFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    if (!pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
		pFile->PrintIndent("#warning \"%s is not implemented!\"\n", (const char*)GetName());
   	    return;
	}

    m_pReturnVar->WriteZeroInitDeclaration(pFile, pContext);

    // check for temp
    if (m_pFunction->HasVariableSizedParameters() || m_pFunction->HasArrayParameters())
    {
		VectorElement *pIter = m_pFunction->GetFirstParameter();
		CBETypedDeclarator *pParameter;
		int nVariableSizedArrayDimensions = 0;
		while ((pParameter = m_pFunction->GetNextParameter(pIter)) != 0)
		{
			// now check each decl for array dimensions
			VectorElement *pIDecl = pParameter->GetFirstDeclarator();
			CBEDeclarator *pDecl;
			while ((pDecl = pParameter->GetNextDeclarator(pIDecl)) != 0)
			{
				int nArrayDims = pDecl->GetStars();
				// get array bounds
				VectorElement *pIArray = pDecl->GetFirstArrayBound();
				CBEExpression *pBound;
				while ((pBound = pDecl->GetNextArrayBound(pIArray)) != 0)
				{
					if (!pBound->IsOfType(EXPR_INT))
						nArrayDims++;
				}
				// calc max
				nVariableSizedArrayDimensions = (nArrayDims > nVariableSizedArrayDimensions) ? nArrayDims : nVariableSizedArrayDimensions;
			}
			// if type of parameter is array, check that too
			if (pParameter->GetType()->GetSize() < 0)
				nVariableSizedArrayDimensions++;
			// if array dims
			// if parameter has size attributes, we assume
			// that it is an array of some sort
			if ((pParameter->FindAttribute(ATTR_SIZE_IS) ||
				pParameter->FindAttribute(ATTR_LENGTH_IS)) &&
				(nVariableSizedArrayDimensions == 0))
				nVariableSizedArrayDimensions = 1;
		}

		// for variable sized arrays we need a temporary variable
		String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
		for (int i=0; i<nVariableSizedArrayDimensions; i++)
		{
			pFile->PrintIndent("unsigned %s%d __attribute__ ((unused));\n", (const char*)sTmpVar, i);
		}

		// need a "pure" temp var as well
		pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", (const char*)sTmpVar);

        String sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", (const char*)sOffsetVar);
    }
}

/**	\brief writes the variable initializations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation should initialize the message buffer and the pointers of the out variables.
 */
void CBEComponentFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{

}

/**	\brief writes the marshalling of the message
 *	\param pFile the file to write to
 *  \param nStartOffset the position in the message buffer to start marshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *	\param pContext the context of the write operation
 *
 * This implementation does not use the base class' marshal mechanisms, because it does
 * something totally different. It write test-suite's compare operations instead of parameter
 * marshalling.
 */
void CBEComponentFunction::WriteMarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    if (!pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
   	    return;

    CBETestFunction *pTestFunc = pContext->GetClassFactory()->GetNewTestFunction();
    assert(m_pFunction);
    VectorElement *pIter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pFunction->GetNextParameter(pIter)) != 0)
    {
        if (pParameter->FindAttribute(ATTR_IN))
            pTestFunc->CompareVariable(pFile, pParameter, pContext);
    }
}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBEComponentFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{

}

/**	\brief writes the unmarshalling of the message
 *	\param pFile the file to write to
 *  \param nStartOffset the position int the message buffer to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *	\param pContext the context of the write operation
 *
 * This implementation should unpack the out parameters from the returned message structure
 */
void CBEComponentFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    if (!pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
        return;

    CBETestFunction *pTestFunc = pContext->GetClassFactory()->GetNewTestFunction();
    assert(m_pFunction);
    VectorElement *pIter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pFunction->GetNextParameter(pIter)) != 0)
    {
        if (!(pParameter->FindAttribute(ATTR_OUT)))
            continue;
        pTestFunc->InitLocalVariable(pFile, pParameter, pContext);
    }
    // init return
    pTestFunc->InitLocalVariable(pFile, m_pReturnVar, pContext);
}

/**	\brief clean up the mess
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation cleans up allocated memory inside this function
 */
void CBEComponentFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{

}

/**	\brief writes the return statement
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation should write the return statement if one is necessary.
 * (return type != void)
 */
void CBEComponentFunction::WriteReturn(CBEFile * pFile, CBEContext * pContext)
{
    if (!pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
		return;
    CBEOperationFunction::WriteReturn(pFile, pContext);
}

/**	\brief writes the function definition
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Because we need a declaration of the global test variables before writing the
 * function, we overloaded this function.
 */
void CBEComponentFunction::WriteFunctionDefinition(CBEFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
		return;

    WriteGlobalVariableDeclaration(pFile, pContext);
    pFile->Print("\n");

    return CBEOperationFunction::WriteFunctionDefinition(pFile, pContext);
}

/**	\brief writes the global test variables
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The global test variables are named by the function's name and the parameter name.
 */
void CBEComponentFunction::WriteGlobalVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // only write global variables for testsuite
    if (!pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
        return;

    assert(m_pFunction);
    VectorElement *pIter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pFunction->GetNextParameter(pIter)) != 0)
    {
        pFile->Print("extern ");
        pParameter->WriteGlobalTestVariable(pFile, pContext);
    }
    // declare return variable
    if (!GetReturnType()->IsVoid())
    {
        pFile->Print("extern ");
        m_pReturnVar->WriteGlobalTestVariable(pFile, pContext);
    }
}

/** \brief add this function to the implementation file
 *  \param pImpl the implementation file
 *  \param pContext the context of the create process
 *  \return true if successful
 *
 * A component function is only added if the testsuite or create-skeleton option is set.
 */
bool CBEComponentFunction::AddToFile(CBEImplementationFile * pImpl, CBEContext * pContext)
{
	if (!pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE) &&
		!pContext->IsOptionSet(PROGRAM_GENERATE_TEMPLATE))
		return true;  // fake success, without adding function
	return CBEOperationFunction::AddToFile(pImpl, pContext);
}

/** \brief set the target file name for this function
 *  \param pFEObject the front-end reference object
 *  \param pContext the context of this operation
 *
 * The implementation file for a component-function (server skeleton) is
 * the FILETYPE_TEMPLATE file.
 */
void CBEComponentFunction::SetTargetFileName(CFEBase * pFEObject, CBEContext * pContext)
{
	CBEOperationFunction::SetTargetFileName(pFEObject, pContext);
	pContext->SetFileType(FILETYPE_TEMPLATE);
	if (pFEObject->IsKindOf(RUNTIME_CLASS(CFEFile)))
		m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject, pContext);
	else
		m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject->GetFile(), pContext);
}

/** \brief test the target file name with the locally stored file name
 *  \param pFile the file, which's file name is used for the comparison
 *  \return true if is the target file
 */
bool CBEComponentFunction::IsTargetFile(CBEImplementationFile * pFile)
{
	if (m_sTargetImplementation.Right(11) != "-template.c")
		return false;
	String sBaseLocal = m_sTargetImplementation.Left(m_sTargetImplementation.GetLength()-11);
	String sBaseTarget = pFile->GetFileName();
	if (!(sBaseTarget.Right(11) == "-template.c"))
		return false;
	sBaseTarget = sBaseTarget.Left(sBaseTarget.GetLength()-11);
	if (sBaseTarget == sBaseLocal)
		return true;
	return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if successful
 *
 * A component function is written to an implementation file only if the options
 * PROGRAM_GENERATE_TEMPLATE or  PROGRAM_GENERATE_TESTSUITE are set. It is always
 * written to an header file. These two conditions are only true for the component's
 * side. (The function would not have been created if the attributes (IN,OUT) were
 * not empty).
 */
bool CBEComponentFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (!pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBEComponent)))
		return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!CBEOperationFunction::IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	return ( ( (pContext->IsOptionSet(PROGRAM_GENERATE_TEMPLATE) || pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE)) &&
               pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile))) ||
               pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)));
}

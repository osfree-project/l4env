/**
 *    \file    dice/src/be/BEComponentFunction.cpp
 *    \brief   contains the implementation of the class CBEComponentFunction
 *
 *    \date    01/18/2002
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
#include "be/BEReplyCodeType.h"

#include "Attribute-Type.h"
#include "TypeSpec-Type.h"
#include "fe/FEOperation.h"
#include "fe/FEFile.h"
#include "fe/FEExpression.h"

CBEComponentFunction::CBEComponentFunction()
{
    m_pFunction = 0;
    m_pReplyVar = 0;
}

CBEComponentFunction::CBEComponentFunction(CBEComponentFunction & src)
: CBEOperationFunction(src)
{
    m_pFunction = 0;
    m_pReplyVar = 0;
}

/**    \brief destructor of target class */
CBEComponentFunction::~CBEComponentFunction()
{
    if (m_pReplyVar)
        delete m_pReplyVar;
}

/**    \brief creates the call function
 *    \param pFEOperation the front-end operation used as reference
 *    \param pContext the context of the write operation
 *    \return true if successful
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

    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);

    // check the attribute
    if (pFEOperation->FindAttribute(ATTR_IN))
        pContext->SetFunctionType(FUNCTION_SEND);
    else
        pContext->SetFunctionType(FUNCTION_CALL);

    string sFunctionName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
    m_pFunction = pRoot->FindFunction(sFunctionName);
    if (!m_pFunction)
    {
        VERBOSE("%s failed because component's function (%s) could not be found\n",
                __PRETTY_FUNCTION__, sFunctionName.c_str());
        return false;
    }

    /* if [allow_reply_only] then set reply variable */
    if (pFEOperation->FindAttribute(ATTR_ALLOW_REPLY_ONLY))
    {
        // return type -> set to IPC reply code
        CBEReplyCodeType *pReplyType = pContext->GetClassFactory()->GetNewReplyCodeType();
        if (!pReplyType->CreateBackEnd(pContext))
        {
            delete pReplyType;
            VERBOSE("%s failed because return var could not be set\n",
                __PRETTY_FUNCTION__);
            return false;
        }
        string sReply = pContext->GetNameFactory()->GetReplyCodeVariable(pContext);
        if (m_pReplyVar)
            delete m_pReplyVar;
        m_pReplyVar = new CBETypedDeclarator();
        if (!m_pReplyVar->CreateBackEnd(pReplyType, sReply, pContext))
        {
            delete pReplyType;
            VERBOSE("%s failed because return var could not be set\n",
                __PRETTY_FUNCTION__);
            return false;
        }

        /* clone declarator (for call) and set stars for first */
        vector<CBEDeclarator*>::iterator iterR = m_pReplyVar->GetFirstDeclarator();
        CBEDeclarator *pDecl = *iterR;
        pDecl->IncStars(1);
        SetCallVariable(m_pReplyVar, pDecl->GetName(), 0, pContext);
    }

    // the return value "belongs" to the client function (needed to determine global test variable's name)
    m_pReturnVar->SetParent(m_pFunction);

    return true;
}

/**    \brief writes the variable declarations of this function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * The variable declarations of the component function skeleton is usually empty.
 * If we write the test-skeleton and have a variable sized parameter, we need a temporary
 * variable.
 */
void CBEComponentFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    if (!pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
        pFile->PrintIndent("#warning \"%s is not implemented!\"\n", GetName().c_str());
           return;
    }

    m_pReturnVar->WriteZeroInitDeclaration(pFile, pContext);

    // check for temp
    if (m_pFunction->HasVariableSizedParameters() || m_pFunction->HasArrayParameters())
    {
        vector<CBETypedDeclarator*>::iterator iterP = m_pFunction->GetFirstParameter();
        CBETypedDeclarator *pParameter;
        int nVariableSizedArrayDimensions = 0;
        while ((pParameter = m_pFunction->GetNextParameter(iterP)) != 0)
        {
            // now check each decl for array dimensions
            vector<CBEDeclarator*>::iterator iterD = pParameter->GetFirstDeclarator();
            CBEDeclarator *pDecl;
            while ((pDecl = pParameter->GetNextDeclarator(iterD)) != 0)
            {
                int nArrayDims = pDecl->GetStars();
                // get array bounds
                vector<CBEExpression*>::iterator iterB = pDecl->GetFirstArrayBound();
                CBEExpression *pBound;
                while ((pBound = pDecl->GetNextArrayBound(iterB)) != 0)
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
        string sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        for (int i=0; i<nVariableSizedArrayDimensions; i++)
        {
            pFile->PrintIndent("unsigned %s%d __attribute__ ((unused));\n", sTmpVar.c_str(), i);
        }

        // need a "pure" temp var as well
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", sTmpVar.c_str());

        string sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", sOffsetVar.c_str());
    }
}

/**    \brief writes the variable initializations of this function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation should initialize the message buffer and the pointers of the out variables.
 */
void CBEComponentFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{

}

/**    \brief writes the marshalling of the message
 *    \param pFile the file to write to
 *  \param nStartOffset the position in the message buffer to start marshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *    \param pContext the context of the write operation
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
    vector<CBETypedDeclarator*>::iterator iter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pFunction->GetNextParameter(iter)) != 0)
    {
        if (!pParameter->FindAttribute(ATTR_IN))
            continue;
        if (pParameter->FindAttribute(ATTR_IGNORE))
            continue;
        pTestFunc->CompareVariable(pFile, pParameter, pContext);
    }
}

/**    \brief writes the invocation of the message transfer
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBEComponentFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{

}

/**    \brief writes the unmarshalling of the message
 *    \param pFile the file to write to
 *  \param nStartOffset the position int the message buffer to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *    \param pContext the context of the write operation
 *
 * This implementation should unpack the out parameters from the returned message structure
 */
void CBEComponentFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    if (!pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
        return;

    CBETestFunction *pTestFunc = pContext->GetClassFactory()->GetNewTestFunction();
    assert(m_pFunction);
    vector<CBETypedDeclarator*>::iterator iter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pFunction->GetNextParameter(iter)) != 0)
    {
        if (!(pParameter->FindAttribute(ATTR_OUT)))
            continue;
        pTestFunc->InitLocalVariable(pFile, pParameter, pContext);
    }
    // init return
    pTestFunc->InitLocalVariable(pFile, m_pReturnVar, pContext);
}

/**    \brief clean up the mess
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation cleans up allocated memory inside this function
 */
void CBEComponentFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{

}

/**    \brief writes the return statement
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation should write the return statement if one is necessary.
 * (return type != void)
 */
void CBEComponentFunction::WriteReturn(CBEFile * pFile, CBEContext * pContext)
{
    if (!pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
        return;
    /* set reply variable to DICE_REPLY */
    if (m_pReplyVar)
    {
        vector<CBEDeclarator*>::iterator iterR = m_pReplyVar->GetFirstDeclarator();
        CBEDeclarator *pDecl = *iterR;
        *pFile << "\t*" << pDecl->GetName() << " = DICE_REPLY;\n";
    }
    CBEOperationFunction::WriteReturn(pFile, pContext);
}

/**    \brief writes the function definition
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
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

/**    \brief writes the global test variables
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * The global test variables are named by the function's name and the parameter name.
 */
void CBEComponentFunction::WriteGlobalVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // only write global variables for testsuite
    if (!pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
        return;

    assert(m_pFunction);
    vector<CBETypedDeclarator*>::iterator iter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pFunction->GetNextParameter(iter)) != 0)
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
    if (dynamic_cast<CFEFile*>(pFEObject))
        m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject, pContext);
    else
        m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject->GetSpecificParent<CFEFile>(0), pContext);
}

/** \brief test the target file name with the locally stored file name
 *  \param pFile the file, which's file name is used for the comparison
 *  \return true if is the target file
 */
bool CBEComponentFunction::IsTargetFile(CBEImplementationFile * pFile)
{
    long length = m_sTargetImplementation.length();
    if ((m_sTargetImplementation.substr(length - 11) != "-template.c") &&
	(m_sTargetImplementation.substr(length - 12) != "-template.cc"))
        return false;
    string sBaseLocal = m_sTargetImplementation.substr(0, length-11);
    string sBaseTarget = pFile->GetFileName();
    length = sBaseTarget.length();
    if (length <= 11)
        return false;
    if ((sBaseTarget.substr(length-11) != "-template.c") &&
	(sBaseTarget.substr(length-12) != "-template.cc"))
        return false;
    sBaseTarget = sBaseTarget.substr(0, length-11);
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
bool CBEComponentFunction::DoWriteFunction(CBEHeaderFile * pFile, CBEContext * pContext)
{
    if (!dynamic_cast<CBEComponent*>(pFile->GetTarget()))
        return false;
    if (!CBEOperationFunction::IsTargetFile(pFile))
        return false;
    return true;
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
bool CBEComponentFunction::DoWriteFunction(CBEImplementationFile * pFile, CBEContext * pContext)
{
    if (!dynamic_cast<CBEComponent*>(pFile->GetTarget()))
        return false;
    if (!IsTargetFile(pFile))
        return false;
    return (pContext->IsOptionSet(PROGRAM_GENERATE_TEMPLATE) ||
            pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE));
}

/** \brief writes the reply code var (if set)
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the parameter
 */
void CBEComponentFunction::WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    if (m_pReplyVar)
    {
        if (bComma)
            *pFile << ",\n\t";
        WriteParameter(pFile, m_pReplyVar, pContext, false /* no const with reply var */);
        bComma = true;
    }
    CBEOperationFunction::WriteAfterParameters(pFile, pContext, bComma);
}

/** \brief writes the reply code call parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the declarators
 */
void CBEComponentFunction::WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    if (m_pReplyVar)
    {
        if (bComma)
            *pFile << ",\n\t";
        WriteCallParameter(pFile, m_pReplyVar, pContext);
        bComma = true;
    }
    CBEOperationFunction::WriteCallAfterParameters(pFile, pContext, bComma);
}

/**    \brief writes additional parameters before the parameter list
 *    \param pFile the file to print to
 *    \param pContext the context of the write operation
 *    \return true if this function wrote something
 *
 * The CORBA C mapping specifies a CORBA_object to appear as first parameter.
 */
bool CBEComponentFunction::WriteBeforeParameters(CBEFile * pFile, CBEContext * pContext)
{
    if (m_pCorbaObject)
    {
        WriteParameter(pFile, m_pCorbaObject, pContext, false /* no const with object */);
        return true;
    }
    return false;
}

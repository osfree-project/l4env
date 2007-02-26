/**
 *	\file	dice/src/be/BETestFunction.cpp
 *	\brief	contains the implementation of the class CBETestFunction
 *
 *	\date	03/08/2002
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

#include <string.h> // needed for strlen

#include "be/BETestFunction.h"
#include "be/BEContext.h"
#include "be/BERoot.h"
#include "be/BETypedDeclarator.h"
#include "be/BEUnionCase.h"
#include "be/BEType.h"
#include "be/BETypedef.h"
#include "be/BEStructType.h"
#include "be/BEUnionType.h"
#include "be/BEUserDefinedType.h"
#include "be/BEDeclarator.h"
#include "be/BEExpression.h"
#include "be/BEAttribute.h"
#include "be/BEComponent.h"
#include "be/BEClient.h"
#include "be/BETestsuite.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"

#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEFile.h"

/** simple test-string for the test-function */
#define TEST_STRING "This is a teststring for dice."

IMPLEMENT_DYNAMIC(CBETestFunction);

CBETestFunction::CBETestFunction()
{
    m_pFunction = 0;
    m_pParameter = 0;
    IMPLEMENT_DYNAMIC_BASE(CBETestFunction, CBEOperationFunction);
}

CBETestFunction::CBETestFunction(CBETestFunction & src)
: CBEOperationFunction(src)
{
    m_pFunction = src.m_pFunction;
    m_pParameter = src.m_pParameter;
//    m_vDeclaratorStack.Add(&(src.m_vDeclaratorStack));
    IMPLEMENT_DYNAMIC_BASE(CBETestFunction, CBEOperationFunction);
}

/**	\brief destructor of target class */
CBETestFunction::~CBETestFunction()
{
//    m_vDeclaratorStack.DeleteAll();
}

/**	\brief creates the test function for a client stub
 *	\param pFEOperation the corresponding front-end function
 *	\param pContext the context of the operation
 *
 * The test-function initializes the variables for the corresponding client side functions and
 * calls this funcion. To know which function to use, we create the same function. Because we
 * know that the test function either test the send or the call functions, we know that these
 * are the only possible function we can create. The send-function is created if the front-end
 * function has the IN attribute. This create function should not be called if the corresponding
 * front-end function is a pure OUT function. Thus we can assume that if the front-end function
 * is no IN functions it's a call function.
 *
 * <b>IMPORTANT</b> This Create function assumes preconditions which should always be met!
 *
 * To minimize memory usage (haha) and to operate on the same data, we search for the cooresponding
 * back-end function.
 */
bool CBETestFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    // call base class for basic initialization
    if (!CBEFunction::CreateBackEnd(pContext))
        return false;

    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    int nOldType = pContext->GetFunctionType();

    // check the attribute
    if (pFEOperation->FindAttribute(ATTR_IN))
        pContext->SetFunctionType(FUNCTION_SEND);
    else
        pContext->SetFunctionType(FUNCTION_CALL);

    String sFunctionName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
    m_pFunction = pRoot->FindFunction(sFunctionName);
    ASSERT(m_pFunction);
    // set call parameters
    VectorElement *pIter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pFunction->GetNextParameter(pIter)) != 0)
    {
        VectorElement *pIterD = pParameter->GetFirstDeclarator();
        CBEDeclarator *pDecl = pParameter->GetNextDeclarator(pIterD);
        ASSERT(pDecl);
        m_pFunction->SetCallVariable(pDecl->GetName(), pDecl->GetStars(), pDecl->GetName(), pContext);
    }

    // set target file name
	SetTargetFileName(pFEOperation, pContext);
    // get own name
    pContext->SetFunctionType(pContext->GetFunctionType() | FUNCTION_TESTFUNCTION);
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);

    pContext->SetFunctionType(nOldType);

    // set return type to void to ignore return var
    if (!SetReturnVar(false, 0, TYPE_VOID, String(), pContext))
    {
        VERBOSE("CBETestFunction::CreateBE failed because return var could not be set\n");
        return false;
    }
    // do not write CORBA parameters as references
    if (m_pCorbaObject)
    {
        VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
        CBEDeclarator *pDecl = m_pCorbaObject->GetNextDeclarator(pIter);
        pDecl->IncStars(-pDecl->GetStars());
        m_pFunction->SetCallVariable(pDecl->GetName(), 0, pDecl->GetName(), pContext);
    }
    if (m_pCorbaEnv)
    {
        VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
        CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);
        pDecl->IncStars(-pDecl->GetStars());
        m_pFunction->SetCallVariable(pDecl->GetName(), 0, pDecl->GetName(), pContext);
    }

    return true;
}

/**	\brief writes the definition of the function to the target file
 *	\param pFile the target file to write to
 *	\param pContext the context of the write operation
 *
 * This function definition has the variable declaration before the function to
 * make them globally available. It then calls the base class to write the usual
 * function definition.
 */
void CBETestFunction::WriteFunctionDefinition(CBEFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    // variable declaration
    WriteGlobalVariableDeclaration(pFile, pContext);
    pFile->Print("\n");

    pFile->Print("static ");
    CBEFunction::WriteFunctionDefinition(pFile, pContext);
}

/**	\brief writes the global variables
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The global variables are one per parameter of the member function with a globally
 * unique name. They are not initialized here. We initialize them inside the test-function.
 * That way we have different values each time we call the test function. This run-time
 * initialization can only be done usind user-provided init-function. They are defined in
 * the interface's test-configuration file.
 *
 * If no such init-functions exist, we set the values to hard-coded values.
 */
void CBETestFunction::WriteGlobalVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    ASSERT(m_pFunction);
    VectorElement *pIter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pFunction->GetNextParameter(pIter)) != 0)
    {
        pParameter->WriteGlobalTestVariable(pFile, pContext);
    }
    // write global return
    if (!m_pFunction->GetReturnType()->IsVoid())
    {
        String sReturnVar = pContext->GetNameFactory()->GetGlobalReturnVariable(m_pFunction, pContext);
        pFile->PrintIndent("");
        m_pFunction->GetReturnType()->Write(pFile, pContext);
        pFile->Print(" %s;\n", (const char *) sReturnVar);
    }
}

/**	\brief writes the declaration of the parameters
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation declares the parameters of the function as local variables.
 * These local variables are initialized with the values of the global variables
 * during variable initilization.
 *
 * Parameters with pointer are set to local indirection parameters. To achieve this we
 * can use the WriteIndirect function of the CBETypedDeclarator class, which was originally
 * implemented for the switch-case class.
 */
void CBETestFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    ASSERT(m_pFunction);
    VectorElement *pIter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    bool bHasVariableSizedParameters = false;
    while ((pParameter = m_pFunction->GetNextParameter(pIter)) != 0)
    {
        pFile->PrintIndent("");
        pParameter->WriteIndirect(pFile, pContext);
        pFile->Print(";\n");
        if (pParameter->IsVariableSized())
            bHasVariableSizedParameters = true;
        // if parameter has size attributes, we assume
        // that it is an array of some sort
        if (pParameter->FindAttribute(ATTR_SIZE_IS) ||
            pParameter->FindAttribute(ATTR_LENGTH_IS))
            bHasVariableSizedParameters = true;
    }
    // write return variable
    if (!m_pFunction->GetReturnType()->IsVoid())
    {
        pFile->PrintIndent("");
        m_pFunction->GetReturnVariable()->WriteDeclaration(pFile, pContext);
        pFile->Print(";\n");
    }
    // for variable sized arrays we need a temporary variable
    if (bHasVariableSizedParameters)
    {
        String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", (const char*)sTmpVar);
    }
}

 /**	\brief writes the initialization of the parameters
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This variable initilization sets all global variables to a random number (usually
 * by using a defined random number for the specific type or a user defined random
 * function for a user defined type). Then the local
 * IN variables are set to the value of the respective global variables.
 */
void CBETestFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    ASSERT(m_pFunction);
    // init return variable
    if (!m_pFunction->GetReturnType()->IsVoid())
    {
        m_pParameter = m_pFunction->GetReturnVariable();
        InitGlobalVariable(pFile, m_pParameter, pContext);
        m_pParameter = 0;
    }
    // init indirect local variables
    VectorElement *pIter = m_pFunction->GetFirstSortedParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pFunction->GetNextSortedParameter(pIter)) != 0)
    {
        pParameter->WriteIndirectInitialization(pFile, pContext);
    }

    // init global variables and set local variables
    pIter = m_pFunction->GetFirstSortedParameter();
    while ((pParameter = m_pFunction->GetNextSortedParameter(pIter)) != 0)
    {
        // init global variables (all)
        m_pParameter = pParameter;
        InitGlobalVariable(pFile, pParameter, pContext);
        m_pParameter = 0;
        // set local variables (only INs)
        if (!(pParameter->FindAttribute(ATTR_IN)))
            continue;
        InitLocalVariable(pFile, pParameter, pContext);
        //pParameter->WriteTestInitialization(pFile, pContext);
    }
}

/**	\brief writes the function invocation to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This function writes the function call of the member m_pFunction.
 */
void CBETestFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    ASSERT(m_pFunction);
    // get return variable
    String sReturnVar;
    if (!m_pFunction->GetReturnType()->IsVoid())
        sReturnVar = pContext->GetNameFactory()->GetReturnVariable(pContext);
    pFile->PrintIndent("");
    m_pFunction->WriteCall(pFile, sReturnVar, pContext);
    pFile->Print("\n");
}

/**	\brief writes the clean-up code to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Because we have to compare the OUT parameters with the set values and the unmarshalling
 * function is not used for this, we do it here.
 */
void CBETestFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
    ASSERT(m_pFunction);
    VectorElement *pIter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pFunction->GetNextParameter(pIter)) != 0)
    {
        WriteOutVariableCheck(pFile, pParameter, pContext);
    }
    // check return variable
    if (!m_pFunction->GetReturnType()->IsVoid())
    {
        CompareVariable(pFile, m_pFunction->GetReturnVariable(), pContext);
    }
}

/**	\brief writes the comparison code to the target file
 *	\param pFile the file to write to
 *	\param pParameter the parameter to compare
 *	\param pContext the context of the write operation
 *
 * The comparison is done between the local variable, which is used for the parameters and
 * the global variables.
 */
void CBETestFunction::WriteOutVariableCheck(CBEFile * pFile, CBETypedDeclarator *pParameter, CBEContext * pContext)
{
    if (!(pParameter->FindAttribute(ATTR_OUT)))
        return;
    CompareVariable(pFile, pParameter, pContext);
}

/** \brief check if this parameter has to be marshalled
 *  \param pParameter the parameter to check
 *  \param pContext the context of this marshalling
 *  \return true if it has to be marshalled
 *
 * Always return false, because this function does not marshal anything
 */
bool CBETestFunction::DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    return false;
}

/** \brief check if this parameter has to be unmarshalled
 *  \param pParameter the parameter to check
 *  \param pContext the context of this unmarshalling
 *  \return true if it has to be unmarshalled
 *
 * Always return false, because this function does not unmarshal anything
 */
bool CBETestFunction::DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    return false;
}

/** \brief initializes the global test variables
 *  \param pFile the file to write to
 *  \param pParameter the parameter to initialize
 *  \param pContext the context of this init procedure
 *
 * This implementation is derived from the marshaller's Marshal function. If you overload the
 * CBEMarshaller class and change the Marshal function's behaviour, then also remember this function
 * and adapt it - if necessary.
 */
void CBETestFunction::InitGlobalVariable(CBEFile * pFile, CBETypedDeclarator * pParameter, CBEContext * pContext)
{
    VectorElement *pIter = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pParameter->GetNextDeclarator(pIter)) != 0)
    {
        m_vDeclaratorStack.Push(pDecl);
        InitGlobalDeclarator(pFile, pParameter->GetType(), pContext);
        m_vDeclaratorStack.Pop();
    }
}

/** \brief initializes a single declarator of a global variable
 *  \param pFile the file to write to
 *  \param pType the type of the declarator
 *  \param pContext the context of the initialization
 *
 * The declarator itself is taken from the declarator stack. If the declarator is the size
 * attribute of another parameter, we have to divide the random value modulo the total size of
 * the array, so the index won't be too large.
 */
void CBETestFunction::InitGlobalDeclarator(CBEFile * pFile, CBEType * pType, CBEContext * pContext)
{
    if (pType->IsVoid())
        return;

    // test if string
    ASSERT(m_pParameter);
    bool bIsString = m_pParameter->IsString();

    // get current decl
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    if (!pCurrent->HasIndex())
    {
        // is first -> test for array
        if (pCurrent->pDeclarator->IsArray())
        {
            return InitGlobalArray(pFile, pType, pContext);
        }
        // a string is similar to an array, test for it
        if (bIsString)
        {
            return InitGlobalString(pFile, pType, pContext);
        }
    }

    // test user defined types
    while (pType->IsKindOf(RUNTIME_CLASS(CBEUserDefinedType)))
    {
        // search for original type and replace it
        CBERoot *pRoot = pType->GetRoot();
        ASSERT(pRoot);
        CBETypedef *pUserType = pRoot->FindTypedef(((CBEUserDefinedType *) pType)->GetName());
        // if 0: not defined in IDL files -> use user-provided init function
        if (pUserType)
        {
            if (pUserType->GetType())
                pType = pUserType->GetType();
        }
    }

    // test for struct
    if (pType->IsKindOf(RUNTIME_CLASS(CBEStructType)))
    {
        return InitGlobalStruct(pFile, (CBEStructType*)pType, pContext);
    }

    // test for union
    if (pType->IsKindOf(RUNTIME_CLASS(CBEUnionType)))
    {
        return InitGlobalUnion(pFile, (CBEUnionType*)pType, pContext);
    }

    // if this is the size-declarator of a string, it
    // should be initialized with this size instead of a random number
    VectorElement *pIter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pIsParam;
    while ((pIsParam = m_pFunction->GetNextParameter(pIter)) != 0)
    {
        if (pIsParam->FindIsAttribute(pCurrent->pDeclarator->GetName()))
            break;
    }
    if (pIsParam && pIsParam->IsString())
    {
        pFile->PrintIndent("");
        m_vDeclaratorStack.Write(pFile, false, true, pContext);
        pFile->Print(" = %d;\n", strlen(TEST_STRING)+1);
    }
    // if no IS parameter or IS parameter is not string, then use random function
    if (!pIsParam || (pIsParam && !pIsParam->IsString()))
    {
        // init variable
        pFile->PrintIndent("");
        m_vDeclaratorStack.Write(pFile, false, true, pContext);
        pFile->Print(" = random_");
        pType->Write(pFile, pContext);
        WriteModulo(pFile, pContext);
        pFile->Print(";\n");
        // make positive number
        WriteAbs(pFile, pContext);
    }
}

/** \brief writes the modulo operation for the global variable initialization
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If the last declarator is the size_is or length_is attribute of another parameter, we have to divide
 * the last declarator modulo the maximum array bound of the parameter. Thus the size parameter
 * stay in bound.
 */
void CBETestFunction::WriteModulo(CBEFile *pFile, CBEContext *pContext)
{
    // get function
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEFunction *pFunction = pCurrent->pDeclarator->GetFunction();
    if (pFunction)
    {
        // iterate over parameters
        VectorElement *pIterP = pFunction->GetFirstParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = pFunction->GetNextParameter(pIterP)) != 0)
        {
            // check if parameter has size attribute
            CBEAttribute *pSizeAttr = pParameter->FindAttribute(ATTR_SIZE_IS);
            if (!pSizeAttr)
                pSizeAttr = pParameter->FindAttribute(ATTR_LENGTH_IS);
            if (pSizeAttr)
            {
                // check if attributes declarator is current
                VectorElement *pIterA = pSizeAttr->GetFirstIsAttribute();
                CBEDeclarator *pIsDecl;
                while ((pIsDecl = pSizeAttr->GetNextIsAttribute(pIterA)) != 0)
                {
                    if (pIsDecl->GetName() == pCurrent->pDeclarator->GetName())
                    {
                        // get first declarator of parameter
                        VectorElement *pIterD = pParameter->GetFirstDeclarator();
                        CBEDeclarator *pDecl = pParameter->GetNextDeclarator(pIterD);
                        if (!pDecl)
                            break;
                        // get max bound of parameter
                        VectorElement *pIterB = pDecl->GetFirstArrayBound();
                        CBEExpression *pBound = pDecl->GetNextArrayBound(pIterB);
                        int nMax = 0;
                        if (pBound)
                            nMax = pBound->GetIntValue();
                        else
                            nMax = pContext->GetSizes()->GetMaxSizeOfType(pParameter->GetType()->GetFEType());
                        // and divide modulo
                        pFile->Print("%%%d", nMax);
                    }
                }
            }
        }
    }
}

/** \brief makes the declarator positive if it's a size declarator
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If the current declarator is a size attribute, then we have to assure that it
 * is positive, because we cannot index negative arrays. This must only be done if
 * its a signed type.
 */
void CBETestFunction::WriteAbs(CBEFile *pFile, CBEContext *pContext)
{
    // check signed/unsigned
    if (m_pParameter->GetType()->IsUnsigned())
        return;
    // get function
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEFunction *pFunction = pCurrent->pDeclarator->GetFunction();
    if (pFunction)
    {
        // iterate over parameters
        VectorElement *pIterP = pFunction->GetFirstParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = pFunction->GetNextParameter(pIterP)) != 0)
        {
            // check if parameter has size attribute
            CBEAttribute *pSizeAttr = pParameter->FindAttribute(ATTR_SIZE_IS);
            if (!pSizeAttr)
                pSizeAttr = pParameter->FindAttribute(ATTR_LENGTH_IS);
            if (pSizeAttr)
            {
                // check if attributes declarator is current
                CBEDeclarator *pIsDecl = pSizeAttr->FindIsParameter(pCurrent->pDeclarator->GetName());
                if (pIsDecl)
                {
                    pFile->PrintIndent("if (");
                    m_vDeclaratorStack.Write(pFile, false, true, pContext);
                    pFile->Print(" < 0)\n");
                    pFile->IncIndent();
                    pFile->PrintIndent("");
                    m_vDeclaratorStack.Write(pFile, false, true, pContext);
                    pFile->Print(" = -");
                    m_vDeclaratorStack.Write(pFile, false, true, pContext);
                    pFile->Print(";\n");
                    pFile->DecIndent();
                }
            }
        }
    }
}

/** \brief inits a global array variable
 *  \param pFile the file to write to
 *  \param pType the base type of the array
 *  \param pContext the context of the write operation
 *
 * because the temp variable is of type unsigned the loop to set and initialize the
 * array has to use the condition '>0'. If it would use '>=0' the last test would be
 * equal to zero and after the next loop the temp var would be decremented, but because
 * it is unsigned it actually will be a huge number ( larger than zero) => the loop would never
 * terminate.
 */
void CBETestFunction::InitGlobalArray(CBEFile *pFile, CBEType *pType, CBEContext *pContext)
{
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
    bool bFixedSize = (pDecl->GetSize() >= 0) &&
                      !(m_pParameter->FindAttribute(ATTR_SIZE_IS)) &&
                      !(m_pParameter->FindAttribute(ATTR_LENGTH_IS));
    // if fixed size
    if (bFixedSize)
    {
        // for each dimension
        VectorElement *pIter = pDecl->GetFirstArrayBound();
        CBEExpression *pBound;
        while ((pBound = pDecl->GetNextArrayBound(pIter)) != 0)
        {
            // now loop for this bound
            for (pCurrent->nIndex = 0; pCurrent->nIndex < pBound->GetIntValue(); pCurrent->nIndex++)
                InitGlobalDeclarator(pFile, pType, pContext);
        }
    }
    else
    {
        // set tmp to size
        // write iteration code
        // for (tmp=0; tmp<size; tmp++)
        // {
        //     ...
        // }

        // set tmp var to size
        String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);

        // write for loop
        pFile->PrintIndent("for (%s = 0; %s < ", (const char*)sTmpVar, (const char*)sTmpVar);
        // use the global name of the size decl for the initialization
        CBEAttribute *pAttr = m_pParameter->FindAttribute(ATTR_SIZE_IS);
        if (!pAttr)
        {
            // check length_is
            pAttr = m_pParameter->FindAttribute(ATTR_LENGTH_IS);
            if (!pAttr)
            {
                // check max-is attribute
                pAttr = m_pParameter->FindAttribute(ATTR_MAX_IS);
                if (!pAttr)
                {
                    ASSERT(false);
                }
            }
        }
        VectorElement *pIter = pAttr->GetFirstIsAttribute();
        CBEDeclarator *pSizeParam = pAttr->GetNextIsAttribute(pIter);
        if (pSizeParam)
            pSizeParam->WriteGlobalName(pFile, pContext);
        pFile->Print("; %s++)\n", (const char*)sTmpVar);
        pFile->PrintIndent("{\n");
        pFile->IncIndent();

        // set index to -2 (var size)
        pCurrent->SetIndex(sTmpVar);
        InitGlobalDeclarator(pFile, pType, pContext);

        // close loop
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
}

/** \brief initializes a global struct variable
 *  \param pFile the file to write to
 *  \param pType the struct type to initialize
 *  \param pContext the context of the write operation
 */
void CBETestFunction::InitGlobalStruct(CBEFile *pFile, CBEStructType *pType, CBEContext *pContext)
{
    VectorElement *pIter = pType->GetFirstMember();
    CBETypedDeclarator *pMember;
    while ((pMember = pType->GetNextMember(pIter)) != 0)
    {
        InitGlobalVariable(pFile, pMember, pContext);
    }
}

/** \brief inits a global union variable
 *  \param pFile the file to write to
 *  \param pType the union type to init
 *  \param pContext the context of the write operation
 */
void CBETestFunction::InitGlobalUnion(CBEFile *pFile, CBEUnionType *pType, CBEContext *pContext)
{
    if (pType->IsCStyleUnion())
    {
        // search for biggest member and set it
        int nMaxSize = 0, nCurrSize = 0;
        VectorElement *pIter = pType->GetFirstUnionCase();
        CBEUnionCase *pCase, *pMaxCase = 0;
        while ((pCase = pType->GetNextUnionCase(pIter)) != 0)
        {
            nCurrSize = pCase->GetSize();
            if (nCurrSize > nMaxSize)
            {
                nMaxSize = nCurrSize;
                pMaxCase = pCase;
            }
        }
        // init max case
        if (pMaxCase)
            InitGlobalVariable(pFile, pMaxCase, pContext);
    }
    else
    {
        // marshal switch variable
        ASSERT(pType->GetSwitchVariable());
        InitGlobalVariable(pFile, pType->GetSwitchVariable(), pContext);
        // divide this variable modulo the case number
        String sSwitchVar = pType->GetSwitchVariableName();
        pFile->PrintIndent("");
        m_vDeclaratorStack.Write(pFile, false, true, pContext);
        pFile->Print(".%s = ", (const char*)sSwitchVar);
        m_vDeclaratorStack.Write(pFile, false, true, pContext);
        pFile->Print(".%s %% %d;\n", (const char*)sSwitchVar, pType->GetUnionCaseCount());

        // write switch statement
        pFile->PrintIndent("switch(");
        m_vDeclaratorStack.Write(pFile, false, true, pContext);
        pFile->Print(".%s)\n", (const char*)sSwitchVar);
        pFile->PrintIndent("{\n");
        // add union to stack
        if (pType->GetUnionName())
            m_vDeclaratorStack.Push(pType->GetUnionName());
        // write the cases
        VectorElement *pIter = pType->GetFirstUnionCase();
        CBEUnionCase *pCase;
        while ((pCase = pType->GetNextUnionCase(pIter)) != 0)
        {
            if (pCase->IsDefault())
            {
                pFile->PrintIndent("default: \n");
            }
            else
            {
                VectorElement *pIterL = pCase->GetFirstLabel();
                CBEExpression *pLabel;
                while ((pLabel = pCase->GetNextLabel(pIterL)) != 0)
                {
                    pFile->PrintIndent("case ");
                    pLabel->Write(pFile, pContext);
                    pFile->Print(":\n");
                }
            }
            pFile->IncIndent();
            InitGlobalVariable(pFile, pCase, pContext);
            pFile->PrintIndent("break;\n");
            pFile->DecIndent();
        }
        // close the switch statement
        pFile->PrintIndent("}\n");
        // remove union name
        if (pType->GetUnionName())
            m_vDeclaratorStack.Pop();
    }
}

/** \brief initializes a test-string
 *  \param pFile the file to write to
 *  \param pType the type of the string
 *  \param pContext the context of the write operation
 *
 * A test string is initialized by setting the variable to a defined string value.
 */
void CBETestFunction::InitGlobalString(CBEFile * pFile, CBEType * pType, CBEContext * pContext)
{
    pFile->PrintIndent("");
    m_vDeclaratorStack.Write(pFile, true, true, pContext);
    pFile->Print(" = \"%s\\0\";\n", TEST_STRING);
}

/** \brief initializes a local variable
 *  \param pFile the file to write to
 *  \param pParameter the parameter to initialize
 *  \param pContext the context of the write operation
 *
 * A local variable is usually initialized with its global variable. This is easy for most types,
 * but becomes tricky, when using arrays, because int t1[20], t2[20]; cannot be assigned t1=t2;
 * This would only set t1 to point to the array of t2, but not the values. Therefore, we have to
 * iterate over the array and assign each single value.
 */
void CBETestFunction::InitLocalVariable(CBEFile *pFile, CBETypedDeclarator *pParameter, CBEContext *pContext)
{
    if (pParameter->GetType()->IsVoid())
        return;
    bool bIsString = (pParameter->IsString()) && !(pParameter->GetType()->IsPointerType());
    VectorElement *pIter = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pParameter->GetNextDeclarator(pIter)) != 0)
    {
        // set local variable
        InitLocalDeclarator(pFile, pDecl, bIsString, pContext);
    }
}

/** \brief writes the initialization for a local variable
 *  \param pFile the file to write to
 *  \param pDeclarator the declarator to write
 *  \param bUsePointer true if the variable should be a pointer
 *  \param pContext the context of the write operation
 *
 * If the array is variable sized, we have to use the size-attribute as number of elements to
 * assign.
 */
void CBETestFunction::InitLocalDeclarator(CBEFile *pFile, CBEDeclarator *pDeclarator, bool bUsePointer, CBEContext *pContext)
{
    // create declarator-stack
    CDeclaratorStack *pStack = new CDeclaratorStack();
    pStack->Push(pDeclarator);
    CDeclaratorStackLocation *pStackLoc = pStack->GetTop();

    if (pDeclarator->IsArray())
    {
        CBETypedDeclarator *pParameter = (CBETypedDeclarator*)pDeclarator->GetParent();
        bool bFixedSize = (pDeclarator->GetSize() >= 0) &&
                          !(pParameter->FindAttribute(ATTR_SIZE_IS)) &&
                          !(pParameter->FindAttribute(ATTR_LENGTH_IS));
        // if fixed size
        if (bFixedSize)
        {
            // for each dimension
            VectorElement *pIter = pDeclarator->GetFirstArrayBound();
            CBEExpression *pBound;
            while ((pBound = pDeclarator->GetNextArrayBound(pIter)) != 0)
            {
                // now loop for this bound
                int nSize = pBound->GetIntValue();
                for (int i=0; i<nSize; i++)
                {
                    pStackLoc->SetIndex(i);
                    InitLocalDeclarator(pFile, pStack, bUsePointer, pContext);
                }
            }
        }
        else
        {
            // set tmp to size
            // write iteration code
            // for (tmp=size; tmp > 0; tmp--)
            // {
            //     ...
            // }

            // set tmp var to size
            String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
            // use the global name of the size decl for the initialization
            CBEAttribute *pAttr = pParameter->FindAttribute(ATTR_SIZE_IS);
            if (!pAttr)
            {
                // check length_is attribute
                pAttr = pParameter->FindAttribute(ATTR_LENGTH_IS);
                if (!pAttr)
                {
                    // check max-is attribute
                    pAttr = pParameter->FindAttribute(ATTR_MAX_IS);
                    if (!pAttr)
                    {
                        ASSERT(false);
                    }
                }
            }
            VectorElement *pIter = pAttr->GetFirstIsAttribute();
            CBEDeclarator *pSizeParam = pAttr->GetNextIsAttribute(pIter);
            // allocate memory for local variable if it is a simple pointered array
            // with only a size or length attribute
            if (pStackLoc->pDeclarator->GetArrayDimensionCount() == 0)
            {
                if (pContext->IsWarningSet(PROGRAM_WARNING_PREALLOC))
                {
                    if (m_pFunction)
                        CCompiler::Warning("CORBA_alloc is used to allocate memory for %s in %s.",
                                           (const char*)pStackLoc->pDeclarator->GetName(), (const char*)m_pFunction->GetName());
                    else
                        CCompiler::Warning("CORBA_alloc is used to allocate memory for %s.", (const char*)pStackLoc->pDeclarator->GetName());
                }
                pFile->PrintIndent("");
                pStack->Write(pFile, true, false, pContext);
                pFile->Print(" = ");
                pParameter->GetType()->WriteCast(pFile, true, pContext);
				CBETypedDeclarator* pEnv = GetEnvironment();
				CBEDeclarator *pDecl = 0;
				if (pEnv)
				{
					VectorElement* pIter = pEnv->GetFirstDeclarator();
					pDecl = pEnv->GetNextDeclarator(pIter);
				}
				if (pDecl && !pContext->IsOptionSet(PROGRAM_FORCE_CORBA_ALLOC))
				{
				    pFile->Print("(%s", (const char*)pDecl->GetName());
				    if (pDecl->GetStars())
					    pFile->Print("->malloc)(");
				    else
					    pFile->Print(".malloc)(");
				}
				else
					pFile->Print("CORBA_alloc(");
                if (pSizeParam)
                    pSizeParam->WriteGlobalName(pFile, pContext);
                pFile->Print(");\n");
            }
            // write for loop
            pFile->PrintIndent("for (%s = 0; %s < ", (const char*)sTmpVar, (const char*)sTmpVar);
            if (pSizeParam)
                pSizeParam->WriteGlobalName(pFile, pContext);
            pFile->Print("; %s++)\n", (const char*)sTmpVar);
            pFile->PrintIndent("{\n");
            pFile->IncIndent();

            // set index to -2 (var size)
            pStackLoc->SetIndex(sTmpVar);
            InitLocalDeclarator(pFile, pStack, bUsePointer, pContext);

            // close loop
            pFile->DecIndent();
            pFile->PrintIndent("}\n");
        }
    }
    else
    {
        InitLocalDeclarator(pFile, pStack, bUsePointer, pContext);
    }
    pStack->Pop();
    delete pStack;
}

/** \brief writes the intialization code for a declarator regarding an array index
 *  \param pFile the file to write to
 *  \param pStack the declarator stack to contain the decl to write
 *  \param bUsePointer true if the declarator should be a pointer
 *  \param pContext the context of the write operation
 */
void CBETestFunction::InitLocalDeclarator(CBEFile *pFile, CDeclaratorStack *pStack, bool bUsePointer, CBEContext *pContext)
{
    pFile->PrintIndent("");
    pStack->Write(pFile, bUsePointer, false, pContext);
    pFile->Print(" = ");
    pStack->Write(pFile, bUsePointer, true, pContext);
    pFile->Print(";\n");
}

/** \brief writes the comparison code for the variables
 *  \param pFile the file to write to
 *  \param pParameter the parameter to compare
 *  \param pContext the context of the write operation
 *
 * This is similar to the  InitGlobal... functions. Except, that this is static...
 * What we have to do is to make the members parameters...
 */
void CBETestFunction::CompareVariable(CBEFile *pFile, CBETypedDeclarator *pParameter, CBEContext *pContext)
{
    CDeclaratorStack *pStack = new CDeclaratorStack();
    CompareVariable(pFile, pParameter, pStack, pContext);
    delete pStack;
}

/** \brief writes the comparison for the variable
 *  \param pFile the file to write to
 *  \param pParameter the parameter to write
 *  \param pStack the declarator stack (if pParameter is member of a struct)
 *  \param pContext the context of the write operation
 */
void CBETestFunction::CompareVariable(CBEFile * pFile, CBETypedDeclarator * pParameter, CDeclaratorStack *pStack, CBEContext * pContext)
{
    m_pParameter = pParameter;
    VectorElement *pIter = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pParameter->GetNextDeclarator(pIter)) != 0)
    {
        pStack->Push(pDecl);
        CompareDeclarator(pFile, pParameter->GetType(), pStack, pContext);
        pStack->Pop();
    }
}

/** \brief writes the test comparison for a single declarator
 *  \param pFile the file to write to
 *  \param pType the type of the currently compared declarator
 *  \param pStack the declarator stack
 *  \param pContext the context of the operation
 *
 * The current declarator is the last in the stack
 */
void CBETestFunction::CompareDeclarator(CBEFile *pFile, CBEType *pType, CDeclaratorStack *pStack, CBEContext *pContext)
{
    if (pType->IsVoid())
	    return;

    // test if string
    ASSERT(m_pParameter);
	bool bIsString = m_pParameter->IsString();

    // get current decl
    CDeclaratorStackLocation *pCurrent = pStack->GetTop();
    if (!pCurrent->HasIndex())
    {
        // is first -> test for array
        if (pCurrent->pDeclarator->IsArray())
        {
            return CompareArray(pFile, pType, pStack, pContext);
        }
        // a string is similar to an array, test for it
        if (bIsString)
        {
            return CompareString(pFile, pType, pStack, pContext);
        }
    }

    // test user defined types
    while (pType->IsKindOf(RUNTIME_CLASS(CBEUserDefinedType)))
    {
        // search for original type and replace it
        CBERoot *pRoot = pType->GetRoot();
        ASSERT(pRoot);
        CBETypedef *pUserType = pRoot->FindTypedef(((CBEUserDefinedType *) pType)->GetName());
        // if 0: not defined in IDL files -> use user-provided init function
        if (pUserType)
        {
            if (pUserType->GetType())
                pType = pUserType->GetType();
        }
    }

    // test for struct
    if (pType->IsKindOf(RUNTIME_CLASS(CBEStructType)))
    {
        return CompareStruct(pFile, (CBEStructType*)pType, pStack, pContext);
    }

    // test for union
    if (pType->IsKindOf(RUNTIME_CLASS(CBEUnionType)))
    {
        return CompareUnion(pFile, (CBEUnionType*)pType, pStack, pContext);
    }

    // compare variable
    WriteComparison(pFile, pStack, pContext);
}

/** \brief writest the test comparison for an array
 *  \param pFile the file to write to
 *  \param pType the type of the current delcarator
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void CBETestFunction::CompareArray(CBEFile *pFile, CBEType *pType, CDeclaratorStack *pStack, CBEContext *pContext)
{
    CDeclaratorStackLocation *pCurrent = pStack->GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
    CBETypedDeclarator *pParameter = (CBETypedDeclarator*)pDecl->GetParent();
    bool bFixedSize = (pDecl->GetSize() >= 0) &&
                      !(pParameter->FindAttribute(ATTR_SIZE_IS)) &&
                      !(pParameter->FindAttribute(ATTR_LENGTH_IS));
    // if fixed size
    if (bFixedSize)
    {
        // for each dimension
        VectorElement *pIter = pDecl->GetFirstArrayBound();
        CBEExpression *pBound;
        while ((pBound = pDecl->GetNextArrayBound(pIter)) != 0)
        {
            // now loop for this bound
            for (pCurrent->nIndex = 0; pCurrent->nIndex < pBound->GetIntValue(); pCurrent->nIndex++)
                CompareDeclarator(pFile, pType, pStack, pContext);
        }
    }
    else
    {
        // set tmp to size
        // write iteration code
        // for (tmp=0; tmp < size; tmp++)
        // {
        //     ...
        // }

        // set tmp var to size
        String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);

        // write for loop
        pFile->PrintIndent("for (%s = 0; %s < ", (const char*)sTmpVar, (const char*)sTmpVar);
        // use the global name of the size decl for the initialization
        CBEAttribute *pAttr = pParameter->FindAttribute(ATTR_SIZE_IS);
        if (!pAttr)
        {
            // check length-is attribute
            pAttr = pParameter->FindAttribute(ATTR_LENGTH_IS);
            if (!pAttr)
            {
                // check max-is attribute
                pAttr = pParameter->FindAttribute(ATTR_MAX_IS);
                if (!pAttr)
                {
                    ASSERT(false);
                }
            }
        }
        VectorElement *pIter = pAttr->GetFirstIsAttribute();
        CBEDeclarator *pSizeParam = pAttr->GetNextIsAttribute(pIter);
        if (pSizeParam)
            pSizeParam->WriteGlobalName(pFile, pContext);
        pFile->Print("; %s++)\n", (const char*)sTmpVar);
        pFile->PrintIndent("{\n");
        pFile->IncIndent();

        // set index to -2 (var size)
        pCurrent->SetIndex(sTmpVar);
        CompareDeclarator(pFile, pType, pStack, pContext);

        // close loop
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
}

/** \brief writes the test comparison code for a struct
 *  \param pFile the file to write to
 *  \param pType the type of the current declarator
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void CBETestFunction::CompareStruct(CBEFile *pFile, CBEStructType *pType, CDeclaratorStack *pStack, CBEContext *pContext)
{
    VectorElement *pIter = pType->GetFirstMember();
    CBETypedDeclarator *pMember;
    while ((pMember = pType->GetNextMember(pIter)) != 0)
    {
        CompareVariable(pFile, pMember, pStack, pContext);
    }
}

/** \brief writes the test comparison code for a union
 *  \param pFile the file to write to
 *  \param pType the type of the current declarator
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void CBETestFunction::CompareUnion(CBEFile *pFile, CBEUnionType *pType, CDeclaratorStack *pStack, CBEContext *pContext)
{
    if (pType->IsCStyleUnion())
    {
        // search for biggest member and compare it
        int nMaxSize = 0, nCurrSize = 0;
        VectorElement *pIter = pType->GetFirstUnionCase();
        CBEUnionCase *pCase, *pMaxCase = 0;
        while ((pCase = pType->GetNextUnionCase(pIter)) != 0)
        {
            nCurrSize = pCase->GetSize();
            if (nCurrSize > nMaxSize)
            {
                nMaxSize = nCurrSize;
                pMaxCase = pCase;
            }
        }
        // init max case
        if (pMaxCase)
            CompareVariable(pFile, pMaxCase, pStack, pContext);
    }
    else
    {
        // marshal switch variable
        ASSERT(pType->GetSwitchVariable());
        CompareVariable(pFile, pType->GetSwitchVariable(), pStack, pContext);

        // write switch statement
        String sSwitchVar = pType->GetSwitchVariableName();
        pFile->PrintIndent("switch(");
        pStack->Write(pFile, false, false, pContext);
        pFile->Print(".%s)\n", (const char*)sSwitchVar);
        pFile->PrintIndent("{\n");
        // add union name to stack
        if (pType->GetUnionName())
            pStack->Push(pType->GetUnionName());
        // write the cases
        VectorElement *pIter = pType->GetFirstUnionCase();
        CBEUnionCase *pCase;
        while ((pCase = pType->GetNextUnionCase(pIter)) != 0)
        {
            if (pCase->IsDefault())
            {
                pFile->PrintIndent("default: \n");
            }
            else
            {
                VectorElement *pIterL = pCase->GetFirstLabel();
                CBEExpression *pLabel;
                while ((pLabel = pCase->GetNextLabel(pIterL)) != 0)
                {
                    pFile->PrintIndent("case ");
                    pLabel->Write(pFile, pContext);
                    pFile->Print(":\n");
                }
            }
            pFile->IncIndent();
            CompareVariable(pFile, pCase, pStack, pContext);
            pFile->PrintIndent("break;\n");
            pFile->DecIndent();
        }
        // close the switch statement
        pFile->PrintIndent("}\n");
        // remove the union name from the stack
        if (pType->GetUnionName())
            pStack->Pop();
    }
}

/** \brief compares two strings
 *  \param pFile the file to write to
 *  \param pType the type of the string
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void CBETestFunction::CompareString(CBEFile * pFile, CBEType * pType, CDeclaratorStack * pStack, CBEContext * pContext)
{
    pFile->PrintIndent("if (strcmp(");
    // local variable
    pStack->Write(pFile, !(pType->IsPointerType()), false, pContext);
    pFile->Print(", ");
    // global variable
    pStack->Write(pFile, true, true, pContext);
    pFile->Print(") != 0)\n");
    // write error message
    WriteErrorMessage(pFile, pStack, pContext);
    // else success
    pFile->PrintIndent("else\n");
    WriteSuccessMessage(pFile, pStack, pContext);
}

/** \brief writes the real comparison
 *  \param pFile the file to write to
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operations
 *
 * comparison looks like this:
 * <code>
 *  if (<local-var> != <gloabl-var>)
 *    printf("error");
 * </code>
 *
 *   for structs comparison looks like this:
 * <code>
 *  if (<local-var>.member1 != <global-var>.member1)
 *    printf("error");
 *  if (<local-var>.member2 != <global-var>.member2)
 *    printf("error");
 * </code>
 *
 *   and array comparisons looks like:
 * <code>
 *   for (int i=0; i<max; i++)
 *     if (<local-var>[i] != <global-var>[i])
 *       printf("error for %d",i);
 * </code>
 */
void CBETestFunction::WriteComparison(CBEFile *pFile, CDeclaratorStack *pStack, CBEContext *pContext)
{
    if (!pStack)
        return;

    // get global variable
    pFile->PrintIndent("if (");
    // local variable
    pStack->Write(pFile, false, false, pContext);
    // compare
    pFile->Print(" != ");
    // global variable
    pStack->Write(pFile, false, true, pContext);
    // finished
    pFile->Print(")\n");
    // write error message
    WriteErrorMessage(pFile, pStack, pContext);
    // else success message
    pFile->PrintIndent("else\n");
    WriteSuccessMessage(pFile, pStack, pContext);
}

/** \brief prints the error message for a failed comparison
 *  \param pFile the file to print to
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void CBETestFunction::WriteErrorMessage(CBEFile * pFile, CDeclaratorStack * pStack, CBEContext * pContext)
{
    // get target name
    String sTargetName;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) || pFile->IsOfFileType(FILETYPE_SKELETON))
        sTargetName = " at server side";
    else if (pFile->IsOfFileType(FILETYPE_CLIENT) || pFile->IsOfFileType(FILETYPE_TESTSUITE))	// testsuite initiates calls from client
        sTargetName = " at client side";
    else
        sTargetName = " at unknown side";
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->GetTop();
    CBEDeclarator *pDecl = (CBEDeclarator*)(pHead->pDeclarator);
    // print error message
    pFile->PrintIndent("\tprintf(\"%s: WRONG ",
		       (const char *) pDecl->GetFunction()->GetName());
    // if struct add members
    pStack->Write(pFile, false, false, pContext);
//    if (pHead->nIndex >= 0)
//        pFile->Print(" (element %d)", pHead->nIndex);
    if (pHead->nIndex == -2)
        pFile->Print(" (element %%d)");
    pFile->Print("%s\\n\"", (const char *) sTargetName);
    if (pHead->nIndex == -2)
        pFile->Print(", %s", (const char*)pHead->sIndex);
    pFile->Print(");\n");
}

/** \brief writes the success message if transmittion was correct
 *  \param pFile the file to write to
 *  \param pStack the declarator stack
 *  \param pContext the context of the write operation
 */
void CBETestFunction::WriteSuccessMessage(CBEFile *pFile, CDeclaratorStack *pStack, CBEContext *pContext)
{
    // get target name
    String sTargetName;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) || pFile->IsOfFileType(FILETYPE_SKELETON))
        sTargetName = " at server side";
    else if (pFile->IsOfFileType(FILETYPE_CLIENT) || pFile->IsOfFileType(FILETYPE_TESTSUITE))	// testsuite initiates calls from client
        sTargetName = " at client side";
    else
        sTargetName = " at unknown side";
    // get declarator
    CDeclaratorStackLocation *pHead = pStack->GetTop();
    CBEDeclarator *pDecl = (CBEDeclarator*)(pHead->pDeclarator);
    // print error message
    pFile->PrintIndent("\tprintf(\"%s: correct ", (const char *) pDecl->GetFunction()->GetName());
    // test for pointer type
    bool bUsePointer = !m_pParameter->GetType()->IsPointerType();
    // if struct add members
    pStack->Write(pFile, bUsePointer, false, pContext);
//    if (pHead->nIndex >= 0)
//        pFile->Print(" (element %d)", pHead->nIndex);
    if (pHead->nIndex == -2)
        pFile->Print(" (element %%d)");
    pFile->Print("%s\\n\"", (const char *) sTargetName);
    if (pHead->nIndex == -2)
        pFile->Print(", %s", (const char*)pHead->sIndex);
    pFile->Print(");\n");
}

/** \brief set name of target file
 *  \param pFEObject the front-end object to use as reference
 *  \param pContext the context of this operation
 *
 * A test-function's implementation file is the test-suite.
 */
void CBETestFunction::SetTargetFileName(CFEBase * pFEObject, CBEContext * pContext)
{
	CBEOperationFunction::SetTargetFileName(pFEObject, pContext);
	pContext->SetFileType(FILETYPE_TESTSUITE);
	if (pFEObject->IsKindOf(RUNTIME_CLASS(CFEFile)))
		m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject, pContext);
	else
		m_sTargetImplementation = pContext->GetNameFactory()->GetFileName(pFEObject->GetFile(), pContext);
}

/** \brief test if the implementation file is the wished target file
 *  \param pFile the file to test
 *  \return true if successful
 */
bool CBETestFunction::IsTargetFile(CBEImplementationFile * pFile)
{
	if (m_sTargetImplementation.Right(12) != "-testsuite.c")
		return false;
	String sBaseLocal = m_sTargetImplementation.Left(m_sTargetImplementation.GetLength()-12);
	String sBaseTarget = pFile->GetFileName();
	if (!(sBaseTarget.Right(12) == "-testsuite.c"))
		return false;
	sBaseTarget = sBaseTarget.Left(sBaseTarget.GetLength()-12);
	if (sBaseTarget == sBaseLocal)
		return true;
	return false;
}

/** \brief test if this function should be written to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if function should be written
 *
 * A test function is only written to the test-suite's side. Because it's static, it
 * needs no declaration in a header file.
 */
bool CBETestFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!CBEOperationFunction::IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	return pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBETestsuite));
}

/** \brief tests if the parameter needs an additional reference making & or *
 *  \param pDeclarator the declarator to test
 *  \param pContext the context of the test
 *  \param bCall true if the parameter is a call parameter
 *  \return true if the parameter needs an additional reference
 *
 * Parameters of the test-function never need a reference.
 */
bool CBETestFunction::HasAdditionalReference(CBEDeclarator * pDeclarator, CBEContext * pContext, bool bCall)
{
    return false;
}

/** \brief we don't marshal anything
 *  \param pFile the file to write to
 *  \param nStartOffset the offset to start marshalling at
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the write
 */
void CBETestFunction::WriteMarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    /* do nothing */
}

/** \brief we don't unmarshal anything
 *  \param pFile the file to write to
 *  \param nStartOffset the offset to start marshalling at
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the write operation
 */
void CBETestFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    /* do nothing */
}

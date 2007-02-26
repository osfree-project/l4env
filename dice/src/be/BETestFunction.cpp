/**
 *	\file	dice/src/be/BETestFunction.cpp
 *	\brief	contains the implementation of the class CBETestFunction
 *
 *	\date	03/08/2002
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
#include "fe/FEExpression.h"

#include "TypeSpec-Type.h"

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
	{
        VERBOSE("%s failed because base function could not be created\n", __FUNCTION__);
        return false;
	}

    CBERoot *pRoot = GetRoot();
    assert(pRoot);
    int nOldType = pContext->GetFunctionType();

    // check the attribute
    if (pFEOperation->FindAttribute(ATTR_IN))
        pContext->SetFunctionType(FUNCTION_SEND);
    else
        pContext->SetFunctionType(FUNCTION_CALL);

    String sFunctionName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
    m_pFunction = pRoot->FindFunction(sFunctionName);
    assert(m_pFunction);
    // set call parameters
    VectorElement *pIter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = m_pFunction->GetNextParameter(pIter)) != 0)
    {
        VectorElement *pIterD = pParameter->GetFirstDeclarator();
        CBEDeclarator *pDecl = pParameter->GetNextDeclarator(pIterD);
        assert(pDecl);
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
    assert(m_pFunction);
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
    assert(m_pFunction);
    VectorElement *pIter = m_pFunction->GetFirstParameter();
    CBETypedDeclarator *pParameter;
	int nVariableSizedArrayDimensions = 0;
    while ((pParameter = m_pFunction->GetNextParameter(pIter)) != 0)
    {
        pFile->PrintIndent("");
        pParameter->WriteIndirect(pFile, pContext);
        pFile->Print(";\n");

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
    // write return variable
    if (!m_pFunction->GetReturnType()->IsVoid())
    {
        pFile->PrintIndent("");
        m_pFunction->GetReturnVariable()->WriteDeclaration(pFile, pContext);
        pFile->Print(";\n");
    }
    // for variable sized arrays we need a temporary variable
	for (int i=0; i<nVariableSizedArrayDimensions; i++)
	{
        String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
		sTmpVar += i;
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", (const char*)sTmpVar);
    }
	// need a "pure" temp var as well
	String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
	pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", (const char*)sTmpVar);
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
    assert(m_pFunction);
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
        // set local variables (only INs)
        if (!(pParameter->FindAttribute(ATTR_IN)))
            continue;
        InitLocalVariable(pFile, pParameter, pContext);
        m_pParameter = 0;
    }
	// allocate memory for [out, prealloc] variables
    pIter = m_pFunction->GetFirstSortedParameter();
    while ((pParameter = m_pFunction->GetNextSortedParameter(pIter)) != 0)
    {
        // init global variables (all)
        m_pParameter = pParameter;
        // only initialize preallocated out variables
        if (!(pParameter->FindAttribute(ATTR_OUT) &&
		      pParameter->FindAttribute(ATTR_PREALLOC)))
            continue;
	    // allocate memory
        InitPreallocVariable(pFile, pParameter, pContext);
        m_pParameter = 0;
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
    assert(m_pFunction);
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
    assert(m_pFunction);
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
	// free [out, prealloc] variable
    pIter = m_pFunction->GetFirstParameter();
    while ((pParameter = m_pFunction->GetNextParameter(pIter)) != 0)
    {
        if (!(pParameter->FindAttribute(ATTR_OUT) &&
		      pParameter->FindAttribute(ATTR_PREALLOC)))
	        continue;
	    FreePreallocVariable(pFile, pParameter, pContext);
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
    if (!m_pFunction)
	    m_pFunction = pParameter->GetFunction();
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
    assert(m_pParameter);

    // get current decl
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    if (!pCurrent->HasIndex())
    {
        // is first -> test for array
        if (pCurrent->pDeclarator->IsArray())
        {
		    InitGlobalArray(pFile, pType, pContext);
            return;
        }
		// if there are size_is or length_is attribtes and the
		// decl has stars, then this is a variable sized array
		if ((pCurrent->pDeclarator->GetStars() > 0) &&
		    (m_pParameter->FindAttribute(ATTR_SIZE_IS) ||
			 m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
		{
		    InitGlobalArray(pFile, pType, pContext);
			return;
		}
		// since the declarator can also be a member of a struct
        // check the direct parent of the declarator
		if (!pCurrent->pDeclarator->GetFunction() &&
		    pCurrent->pDeclarator->GetStructType() &&
			(pCurrent->pDeclarator->GetStars() > 0))
		{
		    CBETypedDeclarator *pMember = pCurrent->pDeclarator->GetStructType()->FindMember(pCurrent->pDeclarator->GetName());
			if (pMember &&
			    (pMember->FindAttribute(ATTR_SIZE_IS) ||
			     pMember->FindAttribute(ATTR_LENGTH_IS)))
			{
				CBETypedDeclarator *pOldParam = m_pParameter;
				m_pParameter = pMember;
			    InitGlobalArray(pFile, pType, pContext);
				m_pParameter = pOldParam;
				return;
			}
		}
        // a string is similar to an array, test for it
        if (m_pParameter->IsString())
        {
		    InitGlobalString(pFile, pType, pContext);
            return;
        }
    }

    // test user defined types
	Vector vBounds(RUNTIME_CLASS(CBEExpression));
    while (pType->IsKindOf(RUNTIME_CLASS(CBEUserDefinedType)))
    {
        // search for original type and replace it
        CBERoot *pRoot = pType->GetRoot();
        assert(pRoot);
        CBETypedef *pUserType = pRoot->FindTypedef(((CBEUserDefinedType *) pType)->GetName());
        // if 0: not defined in IDL files -> use user-provided init function
		if (!pUserType || !pUserType->GetType())
		    break;
		// assign aliased type
		pType = pUserType->GetType();
        // if alias is array?
        CBEDeclarator *pAlias = pUserType->GetAlias();
        // check if type has alias (it should, since the alias is
        // the name of the user defined type) and if it is an array
		// add the boundaries to the vector.
        if (pAlias && pAlias->IsArray())
        {
            // add array bounds of alias to temp vector
            VectorElement *pI = pAlias->GetFirstArrayBound();
            CBEExpression *pBound;
            while ((pBound = pAlias->GetNextArrayBound(pI)) != 0)
            {
                vBounds.Add(pBound);
            }
        }
    }

	// test if we added some array bounds from user defined types
	if (vBounds.GetSize() > 0)
	{
	    VectorElement *pI;
		// add bounds to top declarator
		for (pI = vBounds.GetFirst(); pI; pI = pI->GetNext())
		{
			pCurrent->pDeclarator->AddArrayBound((CBEExpression*)pI->GetElement());
		}
		// call MarshalArray
		InitGlobalArray(pFile, pType, pContext);
		// remove those array decls again
		for (pI = vBounds.GetFirst(); pI; pI = pI->GetNext())
		{
			pCurrent->pDeclarator->RemoveArrayBound((CBEExpression*)pI->GetElement());
		}
		// return (work done)
		return;
	}

    // test for struct
    if (pType->IsKindOf(RUNTIME_CLASS(CBEStructType)))
    {
	    InitGlobalStruct(pFile, (CBEStructType*)pType, pContext);
        return;
    }

    // test for union
    if (pType->IsKindOf(RUNTIME_CLASS(CBEUnionType)))
    {
	    InitGlobalUnion(pFile, (CBEUnionType*)pType, pContext);
        return;
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
	else
    {
		// if no IS parameter or IS parameter is not string, then use random function
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
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
	String sName = pCurrent->pDeclarator->GetName();

	CBETypedDeclarator *pParameter = 0;
    // get function
    CBEFunction *pFunction = pCurrent->pDeclarator->GetFunction();
    if (pFunction)
	{
	    pParameter = pFunction->FindParameterIsAttribute(ATTR_SIZE_IS, sName);
        if (!pParameter)
		    pParameter = pFunction->FindParameterIsAttribute(ATTR_LENGTH_IS, sName);
	}
	CBEStructType *pStruct = pCurrent->pDeclarator->GetStructType();
	if (!pParameter && pStruct)
	{
	    pParameter = pStruct->FindMemberIsAttribute(ATTR_SIZE_IS, sName);
		if (!pParameter)
		    pParameter = pStruct->FindMemberIsAttribute(ATTR_LENGTH_IS, sName);
	}
	if (pParameter)
    {
		// get first declarator of parameter
		VectorElement *pIterD = pParameter->GetFirstDeclarator();
		CBEDeclarator *pDecl = pParameter->GetNextDeclarator(pIterD);
		assert(pDecl);
		// get max bound of parameter
		int nMax = 1;
		// if there are multiple bound, they multiply
		// if one evaluates to 0 (e.g. variable sized),
		// they all are zero
		VectorElement *pIterB = pDecl->GetFirstArrayBound();
		CBEExpression *pBound;
		while ((pBound = pDecl->GetNextArrayBound(pIterB)) != 0)
			nMax *= pBound->GetIntValue();
		// if variable sized, get max size of type
		if (nMax <= 1)
		{
			CBEType *pType = pParameter->GetType();
			CBEAttribute *pAttr = pParameter->FindAttribute(ATTR_TRANSMIT_AS);
			if (pAttr)
				pType = pAttr->GetAttrType();
			nMax = pContext->GetSizes()->GetMaxSizeOfType(pType->GetFEType());
		}
		// and divide modulo
		pFile->Print("%%%d", nMax);
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
 *  \param pIter the iterator pointing to the current array dimension
 *  \param nLevel the number of the current array dimension
 *  \param pContext the context of the write operation
 */
void CBETestFunction::InitGlobalConstArray(CBEFile *pFile, CBEType *pType, VectorElement *pIter, int nLevel, CBEContext *pContext)
{
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
    // get array bound
	CBEExpression *pBound = pDecl->GetNextArrayBound(pIter);
	int nBound = pBound->GetIntValue();
	// iterate over elements
	for (int nIndex = 0; nIndex < nBound; nIndex++)
	{
	    pCurrent->SetIndex(nIndex, nLevel);
	    // if more levels, go down
		if (pIter)
		    InitGlobalConstArray(pFile, pType, pIter, nLevel+1, pContext);
		else
            InitGlobalDeclarator(pFile, pType, pContext);
	}
	// reset index, so the nesting of calls works
	pCurrent->SetIndex(-1, nLevel);
}

/** \brief inits a global array variable
 *  \param pFile the file to write to
 *  \param pType the base type of the array
 *  \param pIter the iterator pointing to the next array dimension
 *  \param nLevel the number of the current array dimension
 *  \param pSizeAttr the attroibute containing the size parameter for the variable sized array
 *  \param pIAttr the iterator pointing to the next parameter of the size attribute
 *  \param pContext the context of the write operation
 */
void CBETestFunction::InitGlobalVarArray(CBEFile *pFile, CBEType *pType, VectorElement *pIter, int nLevel, CBEAttribute *pSizeAttr, VectorElement *pIAttr, CBEContext *pContext)
{
    // get current decl
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
	// if the remaining number of boundaries is equal to the remaining number
	// of size attribute's parameters, then we use the size parameters
	// (its a [size_is(len)] int array[<int>])
	CBEDeclarator *pSizeParam = 0;
	if (pSizeAttr && (pDecl->GetRemainingNumberOfArrayBounds(pIter) <= pSizeAttr->GetRemainingNumberOfIsAttributes(pIAttr)))
		pSizeParam = pSizeAttr->GetNextIsAttribute(pIAttr);
	// get next array dimension
    CBEExpression *pBound = 0;
	if (pIter)
	    pBound = pDecl->GetNextArrayBound(pIter);
	// if const array dimension, we iterate over it
	if (pBound && pBound->IsOfType(EXPR_INT) && !pSizeParam)
	{
		int nBound = pBound->GetIntValue();
		// iterate over elements
		for (int nIndex = 0; nIndex < nBound; nIndex++)
		{
		    pCurrent->SetIndex(nIndex, nLevel);
			// if more levels, go down
			if (pIter)
				InitGlobalVarArray(pFile, pType, pIter, nLevel+1, pSizeAttr, pIAttr, pContext);
			else
				InitGlobalDeclarator(pFile, pType, pContext);
		}
		// reset index, so the nesting of calls works
		pCurrent->SetIndex(-1, nLevel);
	}
	// otherwise we write the iterations
	else
    {
		// get a temp var
		String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
		// add level
		sTmpVar += nLevel;
        // write for loop
        pFile->PrintIndent("for (%s = 0; %s < ", (const char*)sTmpVar, (const char*)sTmpVar);
		// might be member of a struct, that's why we also pass the stack
        if (pSizeParam)
            pSizeParam->WriteGlobalName(pFile, &m_vDeclaratorStack, pContext);
        pFile->Print("; %s++)\n", (const char*)sTmpVar);
        pFile->PrintIndent("{\n");
        pFile->IncIndent();

        // set index to -2 (var size)
        pCurrent->SetIndex(sTmpVar, nLevel);
		// if we have more array dimensions, check them as well
		if (pIter)
		    InitGlobalVarArray(pFile, pType, pIter, nLevel+1, pSizeAttr, pIAttr, pContext);
		else
			InitGlobalDeclarator(pFile, pType, pContext);
		// reset index, so the nesting of calls works
		pCurrent->SetIndex(-1, nLevel);

        // close loop
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
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
    // check if somebody is already iterating over array bounds
	int nLevel = pCurrent->GetUsedIndexCount();
	// skip array bounds already in use
	int i = nLevel;
	VectorElement *pIter = pDecl->GetFirstArrayBound();
	while (i-- > 0) pDecl->GetNextArrayBound(pIter);
    // if fixed size
    if (bFixedSize)
		InitGlobalConstArray(pFile, pType, pIter, nLevel, pContext);
    else
    {
        // set tmp to size
        // write iteration code
        // for (tmp=0; tmp<size; tmp++)
        // {
        //     ...
        // }

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
                    assert(false);
                }
            }
        }
        VectorElement *pIter = pAttr->GetFirstIsAttribute();
        CBEDeclarator *pSizeParam = pAttr->GetNextIsAttribute(pIter);
		// allocate memory if the the variable array is a member of a struct,
		// because global arrays are statically allocated for parameters only
		if ((m_vDeclaratorStack.GetSize() > 1) &&
            (pCurrent->pDeclarator->GetArrayDimensionCount() == 0))
		{
			pFile->PrintIndent("");
			m_vDeclaratorStack.Write(pFile, true, true, pContext);
			pFile->Print(" = ");
			m_pParameter->GetType()->WriteCast(pFile, true, pContext);
			pContext->WriteMalloc(pFile, this);
			pFile->Print("(");
			if (pSizeParam)
				pSizeParam->WriteGlobalName(pFile, &m_vDeclaratorStack, pContext);
			if (m_pParameter->GetType()->GetSize() != 1)
			{
			    pFile->Print("*sizeof");
				m_pParameter->GetType()->WriteCast(pFile, false, pContext);
			}
			pFile->Print("); /* glob array */\n");
		}
		// assign values
		InitGlobalVarArray(pFile, pType, pIter, nLevel, pAttr, pAttr->GetFirstIsAttribute(), pContext);
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
        assert(pType->GetSwitchVariable());
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

/** \brief initializes a preallocated variables
 *  \param pFile the file to write to
 *  \param pParameter the parameter to pre-allocate
 *  \param pContext the context of the write operation
 */
void CBETestFunction::InitPreallocVariable(CBEFile *pFile, CBETypedDeclarator *pParameter, CBEContext *pContext)
{
    CBEType *pType = pParameter->GetType();
	CBEAttribute *pAttr;
	if ((pAttr = pParameter->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
	    pType = pAttr->GetAttrType();
    VectorElement *pIter = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pParameter->GetNextDeclarator(pIter)) != 0)
    {
	    // only the top-level parameters can be preallocated
		// <var> = (type*)malloc(size);
		m_vDeclaratorStack.Push(pDecl);
		pFile->PrintIndent("");
		m_vDeclaratorStack.Write(pFile, true, false, pContext);
		pFile->Print(" = ");
		pType->WriteCast(pFile, true, pContext);
		pContext->WriteMalloc(pFile, pParameter->GetFunction());
		pFile->Print("(");
		pParameter->WriteGetSize(pFile, &m_vDeclaratorStack, pContext);
		pFile->Print("); /* test */\n");
		m_vDeclaratorStack.Pop();
    }
}

/** \brief frees a preallocated variables
 *  \param pFile the file to write to
 *  \param pParameter the parameter to free
 *  \param pContext the context of the write operation
 */
void CBETestFunction::FreePreallocVariable(CBEFile *pFile, CBETypedDeclarator *pParameter, CBEContext *pContext)
{
    CBEType *pType = pParameter->GetType();
	CBEAttribute *pAttr;
	if ((pAttr = pParameter->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
	    pType = pAttr->GetAttrType();
    VectorElement *pIter = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pParameter->GetNextDeclarator(pIter)) != 0)
    {
	    // only the top-level parameters can be preallocated
		// free(<var>);
		m_vDeclaratorStack.Push(pDecl);
		pFile->PrintIndent("");
		pContext->WriteFree(pFile, pParameter->GetFunction());
		pFile->Print("(");
		m_vDeclaratorStack.Write(pFile, true, false, pContext);
		pFile->Print(");\n");
		m_vDeclaratorStack.Pop();
    }
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
    bool bOwnParam = false;
    if (!m_pParameter)
	{
	    m_pParameter = pParameter;
		bOwnParam = true;
	}
    CBEType *pType = pParameter->GetType();
	CBEAttribute *pAttr;
	if ((pAttr = pParameter->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
	    pType = pAttr->GetAttrType();
    VectorElement *pIter = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pParameter->GetNextDeclarator(pIter)) != 0)
    {
        m_vDeclaratorStack.Push(pDecl);
        InitLocalDeclarator(pFile, pType, pContext);
        m_vDeclaratorStack.Pop();
    }
	if (bOwnParam)
	    m_pParameter = 0;
}

/** \brief initializes a single declarator of a local variable
 *  \param pFile the file to write to
 *  \param pType the type of the declarator
 *  \param pContext the context of the initialization
 *
 * The declarator itself is taken from the declarator stack. If the declarator is the size
 * attribute of another parameter, we have to divide the random value modulo the total size of
 * the array, so the index won't be too large.
 */
void CBETestFunction::InitLocalDeclarator(CBEFile * pFile, CBEType * pType, CBEContext * pContext)
{
    if (pType->IsVoid())
        return;

    // test if string
    assert(m_pParameter);
	bool bIsString = m_pParameter->IsString() && !m_pParameter->GetType()->IsPointerType();

    // get current decl
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    if (!pCurrent->HasIndex())
    {
        // is first -> test for array
        if (pCurrent->pDeclarator->IsArray())
        {
		    InitLocalArray(pFile, pType, pContext);
            return;
        }
		// if there are size_is or length_is attribtes and the
		// decl has stars, then this is a variable sized array
		if ((pCurrent->pDeclarator->GetStars() > 0) &&
		    (m_pParameter->FindAttribute(ATTR_SIZE_IS) ||
			 m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
		{
		    InitLocalArray(pFile, pType, pContext);
			return;
		}
		// since the declarator can also be a member of a struct
        // check the direct parent of the declarator
		if (!pCurrent->pDeclarator->GetFunction() &&
		     pCurrent->pDeclarator->GetStructType() &&
			(pCurrent->pDeclarator->GetStars() > 0))
		{
		    CBETypedDeclarator *pMember = pCurrent->pDeclarator->GetStructType()->FindMember(pCurrent->pDeclarator->GetName());
			if (pMember &&
			    (pMember->FindAttribute(ATTR_SIZE_IS) ||
			     pMember->FindAttribute(ATTR_LENGTH_IS)))
			{
				CBETypedDeclarator *pOldParam = m_pParameter;
				m_pParameter = pMember;
			    InitLocalArray(pFile, pType, pContext);
				m_pParameter = pOldParam;
				return;
			}
		}
    }

    // test user defined types
	Vector vBounds(RUNTIME_CLASS(CBEExpression));
    while (pType->IsKindOf(RUNTIME_CLASS(CBEUserDefinedType)))
    {
        // search for original type and replace it
        CBERoot *pRoot = pType->GetRoot();
        assert(pRoot);
        CBETypedef *pUserType = pRoot->FindTypedef(((CBEUserDefinedType *) pType)->GetName());
        // if 0: not defined in IDL files -> use user-provided init function
        if (!pUserType || !pUserType->GetType())
		    break;
	    // assign aliased type
		pType = pUserType->GetType();
        // if alias is array?
        CBEDeclarator *pAlias = pUserType->GetAlias();
        // check if type has alias (it should, since the alias is
        // the name of the user defined type)
        if (pAlias && pAlias->IsArray())
        {
            // add array bounds of alias to temp vector
            VectorElement *pI = pAlias->GetFirstArrayBound();
            CBEExpression *pBound;
            while ((pBound = pAlias->GetNextArrayBound(pI)) != 0)
            {
                vBounds.Add(pBound);
            }
        }
    }

	// test if user defined type added some array boundaries
	if (vBounds.GetSize() > 0)
	{
	    VectorElement *pI;
		// add bounds to top declarator
		for (pI = vBounds.GetFirst(); pI; pI = pI->GetNext())
		{
			pCurrent->pDeclarator->AddArrayBound((CBEExpression*)pI->GetElement());
		}
		// call MarshalArray
		InitLocalArray(pFile, pType, pContext);
		// remove those array decls again
		for (pI = vBounds.GetFirst(); pI; pI = pI->GetNext())
		{
			pCurrent->pDeclarator->RemoveArrayBound((CBEExpression*)pI->GetElement());
		}
		// return (work done)
		return;
    }

    // test for struct
    if (pType->IsKindOf(RUNTIME_CLASS(CBEStructType)))
    {
	    InitLocalStruct(pFile, (CBEStructType*)pType, pContext);
        return;
    }

    // test for union
    if (pType->IsKindOf(RUNTIME_CLASS(CBEUnionType)))
    {
	    InitLocalUnion(pFile, (CBEUnionType*)pType, pContext);
        return;
    }

    pFile->PrintIndent("");
    m_vDeclaratorStack.Write(pFile, bIsString, false, pContext);
    pFile->Print(" = ");
    m_vDeclaratorStack.Write(pFile, bIsString, true, pContext);
    pFile->Print(";\n");
}

/** \brief inits a local array variable
 *  \param pFile the file to write to
 *  \param pType the base type of the array
 *  \param pIter the iterator pointing to the next array boundary
 *  \param nLevel the number of the current array boundary
 *  \param pContext the context of the write operation
 */
void CBETestFunction::InitLocalConstArray(CBEFile *pFile, CBEType *pType, VectorElement *pIter, int nLevel, CBEContext *pContext)
{
    // get current declarator
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
    // get current boundary
	CBEExpression *pBound = pDecl->GetNextArrayBound(pIter);
	int nBound = pBound->GetIntValue();
	// iterate over elements
	for (int nIndex = 0; nIndex < nBound; nIndex++)
	{
	    pCurrent->SetIndex(nIndex, nLevel);
		// if there are more levels
		if (pIter)
		    InitLocalConstArray(pFile, pType, pIter, nLevel+1, pContext);
		else
            InitLocalDeclarator(pFile, pType, pContext);
	}
	// reset index, so the nesting of calls works
	pCurrent->SetIndex(-1, nLevel);
}

/** \brief inits a local array variable
 *  \param pFile the file to write to
 *  \param pType the base type of the array
 *  \param pIter the iterator pointing to the next array boundary
 *  \param nLevel the number of the current array boundary
 *  \param pSizeAttr the size attribute containing the size parameters for var-sized array dimensions
 *  \param pIAttr the iterator pointing to the next size parameter
 *  \param pContext the context of the write operation
 */
void CBETestFunction::InitLocalVarArray(CBEFile *pFile, CBEType *pType, VectorElement *pIter, int nLevel, CBEAttribute *pSizeAttr, VectorElement *pIAttr, CBEContext *pContext)
{
    // get current declarator
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
	// if the remaining number of boundaries is equal to the remaining number
	// of size attribute's parameters, then we use the size parameters
	// (its a [size_is(len)] int array[<int>])
	CBEDeclarator *pSizeParam = 0;
	if (pSizeAttr && (pDecl->GetRemainingNumberOfArrayBounds(pIter) <= pSizeAttr->GetRemainingNumberOfIsAttributes(pIAttr)))
		pSizeParam = pSizeAttr->GetNextIsAttribute(pIAttr);
	// get next array dimension
    CBEExpression *pBound = 0;
	if (pIter)
	    pBound = pDecl->GetNextArrayBound(pIter);
	// if const array dimension, we iterate over it
	if (pBound && pBound->IsOfType(EXPR_INT) && !pSizeParam)
	{
		int nBound = pBound->GetIntValue();
		// iterate over elements
		for (int nIndex = 0; nIndex < nBound; nIndex++)
		{
		    pCurrent->SetIndex(nIndex, nLevel);
			// if more levels, go down
			if (pIter)
				InitLocalVarArray(pFile, pType, pIter, nLevel+1, pSizeAttr, pIAttr, pContext);
			else
				InitLocalDeclarator(pFile, pType, pContext);
		}
		// reset index, so the nesting of calls works
		pCurrent->SetIndex(-1, nLevel);
	}
	// otherwise we write the iterations
	else
    {
		// get a temp var
		String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
		// add level
		sTmpVar += nLevel;
        // write for loop
        pFile->PrintIndent("for (%s = 0; %s < ", (const char*)sTmpVar, (const char*)sTmpVar);
        if (pSizeParam)
            pSizeParam->WriteGlobalName(pFile, &m_vDeclaratorStack, pContext);
        pFile->Print("; %s++)\n", (const char*)sTmpVar);
        pFile->PrintIndent("{\n");
        pFile->IncIndent();

        // set index to -2 (var size)
        pCurrent->SetIndex(sTmpVar, nLevel);
		// if we have more array dimensions, check them as well
		if (pIter)
		    InitLocalVarArray(pFile, pType, pIter, nLevel+1, pSizeAttr, pIAttr, pContext);
		else
			InitLocalDeclarator(pFile, pType, pContext);
		// reset index, so the nesting of calls works
		pCurrent->SetIndex(-1, nLevel);

        // close loop
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
	}
}

/** \brief inits a local array variable
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
void CBETestFunction::InitLocalArray(CBEFile *pFile, CBEType *pType, CBEContext *pContext)
{
    CDeclaratorStackLocation *pCurrent = m_vDeclaratorStack.GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
    bool bFixedSize = (pDecl->GetSize() >= 0) &&
                      !(m_pParameter->FindAttribute(ATTR_SIZE_IS)) &&
                      !(m_pParameter->FindAttribute(ATTR_LENGTH_IS));
    // check if somebody is already iterating over array bounds
	int nLevel = pCurrent->GetUsedIndexCount();
	// skip array bound already in use
	int i = nLevel;
	VectorElement *pIterBound = pDecl->GetFirstArrayBound();
	while (i-- > 0) pDecl->GetNextArrayBound(pIterBound);
    // if fixed size
    if (bFixedSize)
	    InitLocalConstArray(pFile, pType, pIterBound, nLevel, pContext);
    else
    {
        // set tmp to size
        // write iteration code
        // for (tmp=0; tmp<size; tmp++)
        // {
        //     ...
        // }
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
                    assert(false);
                }
            }
        }
        VectorElement *pIter = pAttr->GetFirstIsAttribute();
        CBEDeclarator *pSizeParam = pAttr->GetNextIsAttribute(pIter);
		// allocate memory for local variable if it is a simple pointered array
		// with only a size or length attribute
		if (pCurrent->pDeclarator->GetArrayDimensionCount() == 0)
		{
			pFile->PrintIndent("");
			m_vDeclaratorStack.Write(pFile, true, false, pContext);
			pFile->Print(" = ");
			m_pParameter->GetType()->WriteCast(pFile, true, pContext);
			pContext->WriteMalloc(pFile, this);
			pFile->Print("(");
			if (pSizeParam)
				pSizeParam->WriteGlobalName(pFile, &m_vDeclaratorStack, pContext);
			if (m_pParameter->GetType()->GetSize() != 1)
			{
			    pFile->Print("*sizeof");
				m_pParameter->GetType()->WriteCast(pFile, false, pContext);
			}
			pFile->Print("); /* loc array */\n");
		}
        // write for loop
		InitLocalVarArray(pFile, pType, pIterBound, nLevel, pAttr, pAttr->GetFirstIsAttribute(), pContext);
    }
}

/** \brief initializes a local struct variable
 *  \param pFile the file to write to
 *  \param pType the struct type to initialize
 *  \param pContext the context of the write operation
 */
void CBETestFunction::InitLocalStruct(CBEFile *pFile, CBEStructType *pType, CBEContext *pContext)
{
    VectorElement *pIter = pType->GetFirstMember();
    CBETypedDeclarator *pMember;
    while ((pMember = pType->GetNextMember(pIter)) != 0)
    {
        InitLocalVariable(pFile, pMember, pContext);
    }
}

/** \brief inits a local union variable
 *  \param pFile the file to write to
 *  \param pType the union type to init
 *  \param pContext the context of the write operation
 */
void CBETestFunction::InitLocalUnion(CBEFile *pFile, CBEUnionType *pType, CBEContext *pContext)
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
            InitLocalVariable(pFile, pMaxCase, pContext);
    }
    else
    {
        // marshal switch variable
        assert(pType->GetSwitchVariable());
        InitLocalVariable(pFile, pType->GetSwitchVariable(), pContext);
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
            InitLocalVariable(pFile, pCase, pContext);
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
    assert(m_pParameter);

    // get current decl
    CDeclaratorStackLocation *pCurrent = pStack->GetTop();
    if (!pCurrent->HasIndex())
    {
        // is first -> test for array
        if (pCurrent->pDeclarator->IsArray())
        {
		    CompareArray(pFile, pType, pStack, pContext);
            return;
        }
		// if there are size_is or length_is attribtes and the
		// decl has stars, then this is a variable sized array
		if ((pCurrent->pDeclarator->GetStars() > 0) &&
		    (m_pParameter->FindAttribute(ATTR_SIZE_IS) ||
			 m_pParameter->FindAttribute(ATTR_LENGTH_IS)))
		{
		    CompareArray(pFile, pType, pStack, pContext);
			return;
		}
		// since the declarator can also be a member of a struct
        // check the direct parent of the declarator
		if (!pCurrent->pDeclarator->GetFunction() &&
		     pCurrent->pDeclarator->GetStructType() &&
			(pCurrent->pDeclarator->GetStars() > 0))
		{
		    CBETypedDeclarator *pMember = pCurrent->pDeclarator->GetStructType()->FindMember(pCurrent->pDeclarator->GetName());
			if (pMember &&
			    (pMember->FindAttribute(ATTR_SIZE_IS) ||
			     pMember->FindAttribute(ATTR_LENGTH_IS)))
			{
				CBETypedDeclarator *pOldParam = m_pParameter;
				m_pParameter = pMember;
			    CompareArray(pFile, pType, pStack, pContext);
				m_pParameter = pOldParam;
				return;
			}
		}
        // a string is similar to an array, test for it
        if (m_pParameter->IsString())
        {
		    CompareString(pFile, pType, pStack, pContext);
            return;
        }
    }

    // test user defined types
	Vector vBounds(RUNTIME_CLASS(CBEExpression));
    while (pType->IsKindOf(RUNTIME_CLASS(CBEUserDefinedType)))
    {
        // search for original type and replace it
        CBERoot *pRoot = pType->GetRoot();
        assert(pRoot);
        CBETypedef *pUserType = pRoot->FindTypedef(((CBEUserDefinedType *) pType)->GetName());
        // if 0: not defined in IDL files -> use user-provided init function
        if (!pUserType || !pUserType->GetType())
			break;
	    // assign the aliased type
		pType = pUserType->GetType();
        // if alias is array?
        CBEDeclarator *pAlias = pUserType->GetAlias();
        // check if type has alias (it should, since the alias is
        // the name of the user defined type)
        if (pAlias && pAlias->IsArray())
        {
            // add array bounds of alias to temp vector
            VectorElement *pI = pAlias->GetFirstArrayBound();
            CBEExpression *pBound;
            while ((pBound = pAlias->GetNextArrayBound(pI)) != 0)
            {
                vBounds.Add(pBound);
            }
        }
    }

	// test if user defined type added some array boundaries
	// vBounds will only contain something when user defined type has been resolved
	if (vBounds.GetSize() > 0)
	{
	    VectorElement *pI;
		// add bounds to top declarator
		for (pI = vBounds.GetFirst(); pI; pI = pI->GetNext())
		{
			pCurrent->pDeclarator->AddArrayBound((CBEExpression*)pI->GetElement());
		}
		// call MarshalArray
		CompareArray(pFile, pType, pStack, pContext);
		// remove those array decls again
		for (pI = vBounds.GetFirst(); pI; pI = pI->GetNext())
		{
			pCurrent->pDeclarator->RemoveArrayBound((CBEExpression*)pI->GetElement());
		}
		// return (work done for user defined type with array)
		return;
	}

    // test for struct
    if (pType->IsKindOf(RUNTIME_CLASS(CBEStructType)))
    {
	    CompareStruct(pFile, (CBEStructType*)pType, pStack, pContext);
        return;
    }

    // test for union
    if (pType->IsKindOf(RUNTIME_CLASS(CBEUnionType)))
    {
	    CompareUnion(pFile, (CBEUnionType*)pType, pStack, pContext);
        return;
    }

    // compare variable
    WriteComparison(pFile, pStack, pContext);
}

/** \brief writest the test comparison for an array
 *  \param pFile the file to write to
 *  \param pType the type of the current delcarator
 *  \param pStack the declarator stack
 *  \param pIter the iterator pointing to the next array bounds
 *  \param nLevel the number of the current array bound
 *  \param pContext the context of the write operation
 */
void CBETestFunction::CompareConstArray(CBEFile *pFile, CBEType *pType, CDeclaratorStack *pStack, VectorElement *pIter, int nLevel, CBEContext *pContext)
{
    // get current declarator
    CDeclaratorStackLocation *pCurrent = pStack->GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
	// get current array boundary
	CBEExpression *pBound = pDecl->GetNextArrayBound(pIter);
	int nBound = pBound->GetIntValue();

	// iterate over elements
	for (int nIndex = 0; nIndex < nBound; nIndex++)
	{
	    pCurrent->SetIndex(nIndex, nLevel);
		// check if there are more array bounds
		if (pIter)
		{
            CompareConstArray(pFile, pType, pStack, pIter, nLevel+1, pContext);
		}
        else
		{
		    CompareDeclarator(pFile, pType, pStack, pContext);
		}
	}
	// reset index, so the nesting of calls works
	pCurrent->SetIndex(-1, nLevel);
}

/** \brief writest the test comparison for an array
 *  \param pFile the file to write to
 *  \param pType the type of the current delcarator
 *  \param pStack the declarator stack
 *  \param pIter the iterator pointing to the next array bounds
 *  \param nLevel the number of the current array bound
 *  \param pSizeAttr the size_is attribute
 *  \param pIAttr the iterator pointing to the next size attribute parameter
 *  \param pContext the context of the write operation
 */
void CBETestFunction::CompareVarArray(CBEFile *pFile, CBEType *pType, CDeclaratorStack *pStack, VectorElement *pIter, int nLevel, CBEAttribute *pSizeAttr, VectorElement *pIAttr, CBEContext *pContext)
{
    // get current declarator
    CDeclaratorStackLocation *pCurrent = pStack->GetTop();
    CBEDeclarator *pDecl = pCurrent->pDeclarator;
	// if the remaining number of boundaries is less or equal to the remaining number
	// of size attribute's parameters, then we use the size parameters
	// (its a [size_is(len)] int array[<int>])
	CBEDeclarator *pSizeParam = 0;
	if (pSizeAttr && (pDecl->GetRemainingNumberOfArrayBounds(pIter) <= pSizeAttr->GetRemainingNumberOfIsAttributes(pIAttr)))
		pSizeParam = pSizeAttr->GetNextIsAttribute(pIAttr);
	// an array without an array bound is a variable sized array with stars
	CBEExpression *pBound = 0;
	if (pIter)
	{
		// get next array dimension
		pBound = pDecl->GetNextArrayBound(pIter);
	}
	// if const array dimension, we iterate over it
	if (pBound && pBound->IsOfType(EXPR_INT) && !pSizeParam)
	{
		int nBound = pBound->GetIntValue();
		// iterate over elements
		for (int nIndex = 0; nIndex < nBound; nIndex++)
		{
		    pCurrent->SetIndex(nIndex, nLevel);
			// if more levels, go down
			if (pIter)
			{
				CompareVarArray(pFile, pType, pStack, pIter, nLevel+1, pSizeAttr, pIAttr, pContext);
			}
			else
			{
				CompareDeclarator(pFile, pType, pStack, pContext);
			}
		}
		// reset index, so the nesting of calls works
		pCurrent->SetIndex(-1, nLevel);
	}
	// otherwise we write the iterations
	else
    {
	    assert(pSizeParam);
		// get a temp var
		String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
		// add level
		sTmpVar += nLevel;
        // write for loop
        pFile->PrintIndent("for (%s = 0; %s < ", (const char*)sTmpVar, (const char*)sTmpVar);
		pSizeParam->WriteGlobalName(pFile, pStack, pContext);
        pFile->Print("; %s++)\n", (const char*)sTmpVar);
        pFile->PrintIndent("{\n");
        pFile->IncIndent();

        // set index to -2 (var size)
        pCurrent->SetIndex(sTmpVar, nLevel);
		// if we have more array dimensions, check them as well
		if (pIter)
		{
		    CompareVarArray(pFile, pType, pStack, pIter, nLevel+1, pSizeAttr, pIAttr, pContext);
		}
		else
		{
			CompareDeclarator(pFile, pType, pStack, pContext);
		}
		// reset index, so the nesting of calls works
		pCurrent->SetIndex(-1, nLevel);

        // close loop
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
	}
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
    // check if there already is somebody iterating over array bounds
	int nLevel = pCurrent->GetUsedIndexCount();
	// skip the already used bounds
	VectorElement *pIter = pDecl->GetFirstArrayBound();
    int i = nLevel;
	while (i-- > 0) pDecl->GetNextArrayBound(pIter);

    // if fixed size
    if (bFixedSize)
    {
	    CompareConstArray(pFile, pType, pStack, pIter, nLevel, pContext);
    }
    else
    {
        // set tmp to size
        // write iteration code
        // for (tmp=0; tmp < size; tmp++)
        // {
        //     ...
        // }

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
                    assert(false);
                }
            }
        }
		CompareVarArray(pFile, pType, pStack, pIter, nLevel, pAttr, pAttr->GetFirstIsAttribute(), pContext);
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
		{
            CompareVariable(pFile, pMaxCase, pStack, pContext);
		}
    }
    else
    {
        // marshal switch variable
        assert(pType->GetSwitchVariable());
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
	if (!pContext->IsOptionSet(PROGRAM_TESTSUITE_NO_SUCCESS_MESSAGE))
	{
		pFile->PrintIndent("else\n");
		WriteSuccessMessage(pFile, pStack, pContext);
	}
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
	if (!pContext->IsOptionSet(PROGRAM_TESTSUITE_NO_SUCCESS_MESSAGE))
    {
		// else success message
		pFile->PrintIndent("else\n");
		WriteSuccessMessage(pFile, pStack, pContext);
	}
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
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) || pFile->IsOfFileType(FILETYPE_TEMPLATE))
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
    bool bVarArray = false;
	int nIdxCnt = pHead->GetUsedIndexCount();
	if (nIdxCnt)
	{
	    for (int i=0; i<nIdxCnt; i++)
		    if (pHead->nIndex[i] == -2)
			    bVarArray = true;
    }
	if (bVarArray)
	{
	    pFile->Print(" (element ");
	    for (int i=0; i<nIdxCnt; i++)
		    if (pHead->nIndex[i] == -2)
			    pFile->Print("[%%d]");
	    pFile->Print(")");
    }
    pFile->Print("%s\\n\"", (const char *) sTargetName);
	for (int i=0; i<nIdxCnt; i++)
		if (pHead->nIndex[i] == -2)
            pFile->Print(", %s", (const char*)pHead->sIndex[i]);
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
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) || pFile->IsOfFileType(FILETYPE_TEMPLATE))
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
    CBEType *pType = m_pParameter->GetType();
	CBEAttribute *pAttr;
	if ((pAttr = m_pParameter->FindAttribute(ATTR_TRANSMIT_AS)) != 0)
	    pType = pAttr->GetAttrType();
    bool bUsePointer = !pType->IsPointerType();
    // if struct add members
    pStack->Write(pFile, bUsePointer, false, pContext);
    bool bVarArray = false;
	int nIdxCnt = pHead->GetUsedIndexCount();
	if (nIdxCnt)
	{
	    for (int i=0; i<nIdxCnt; i++)
		    if (pHead->nIndex[i] == -2)
			    bVarArray = true;
    }
	if (bVarArray)
	{
	    pFile->Print(" (element ");
	    for (int i=0; i<nIdxCnt; i++)
		    if (pHead->nIndex[i] == -2)
			    pFile->Print("[%%d]");
	    pFile->Print(")");
    }
    pFile->Print("%s\\n\"", (const char *) sTargetName);
	for (int i=0; i<nIdxCnt; i++)
		if (pHead->nIndex[i] == -2)
            pFile->Print(", %s", (const char*)pHead->sIndex[i]);
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

/**
 *	\file	dice/src/be/BEFunction.cpp
 *	\brief	contains the implementation of the class CBEFunction
 *
 *	\date	01/11/2002
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

#include "be/BEFunction.h"
#include "be/BEAttribute.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEException.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEContext.h"
#include "be/BEOpcodeType.h"
#include "be/BEMsgBufferType.h"
#include "be/BEMarshaller.h"
#include "be/BEClass.h"
#include "be/BETarget.h"
#include "be/BEUserDefinedType.h"

#include "fe/FEAttribute.h"
#include "fe/FEInterface.h"
#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CBEFunction);

CBEFunction::CBEFunction()
: m_vAttributes(RUNTIME_CLASS(CBEAttribute)),
  m_vParameters(RUNTIME_CLASS(CBETypedDeclarator)),
  m_vSortedParameters(RUNTIME_CLASS(CBETypedDeclarator)),
  m_vCallParameters(RUNTIME_CLASS(CBETypedDeclarator)),
  m_vExceptions(RUNTIME_CLASS(CBEException))
{
    m_pReturnVar = 0;
    m_pClass = 0;
    m_pTarget = 0;
    m_pMsgBuffer = 0;
    m_pCorbaObject = 0;
    m_pCorbaEnv = 0;
    m_nParameterIndent = 0;
    m_vParameters.DeleteAll();
    m_vSortedParameters.DeleteAll();
    m_vCallParameters.DeleteAll();
    m_vExceptions.DeleteAll();
    m_vAttributes.DeleteAll();
    m_bComponentSide = false;
    m_bCastMsgBufferOnCall = false;
    IMPLEMENT_DYNAMIC_BASE(CBEFunction, CBEObject);
}

CBEFunction::CBEFunction(CBEFunction & src)
: CBEObject(src),
  m_vAttributes(RUNTIME_CLASS(CBEAttribute)),
  m_vParameters(RUNTIME_CLASS(CBETypedDeclarator)),
  m_vSortedParameters(RUNTIME_CLASS(CBETypedDeclarator)),
  m_vCallParameters(RUNTIME_CLASS(CBETypedDeclarator)),
  m_vExceptions(RUNTIME_CLASS(CBEException)),
  m_pMsgBuffer(src.m_pMsgBuffer),
  m_pCorbaObject(src.m_pCorbaObject),
  m_pCorbaEnv(src.m_pCorbaEnv)
{
    m_sName = src.m_sName;
    m_pReturnVar = src.m_pReturnVar;
    m_pReturnVar->SetParent(this);
    m_pClass = src.m_pClass;
    m_pTarget = src.m_pTarget;
    if (m_pMsgBuffer)
        m_pMsgBuffer->SetParent(this);
    if (m_pCorbaObject)
        m_pCorbaObject->SetParent(this);
    if (m_pCorbaEnv)
        m_pCorbaEnv->SetParent(this);
    m_vAttributes.Add(&(src.m_vAttributes));
    m_vAttributes.SetParentOfElements(this);
    m_vParameters.Add(&(src.m_vParameters));
    m_vParameters.SetParentOfElements(this);
    m_vSortedParameters.Add(&(src.m_vSortedParameters));
    m_vSortedParameters.SetParentOfElements(this);
    m_vCallParameters.Add(&(src.m_vCallParameters));
    m_vCallParameters.SetParentOfElements(this);
    m_vExceptions.Add(&(src.m_vExceptions));
    m_vExceptions.SetParentOfElements(this);
    m_nParameterIndent = src.m_nParameterIndent;
    m_bCastMsgBufferOnCall = src.m_bCastMsgBufferOnCall;
    m_bComponentSide = src.m_bComponentSide;
    IMPLEMENT_DYNAMIC_BASE(CBEFunction, CBEObject);
}

/**	\brief destructor of target class
 *
 * Do _NOT_ delete the sorted parameters, because they are only references to the "normal" parameters.
 * Thus the sorted parameters are already deleted. The interface is also deleted somewhere else.
 */
CBEFunction::~CBEFunction()
{
    m_vAttributes.DeleteAll();
    m_vParameters.DeleteAll();
    m_vCallParameters.DeleteAll();
    m_vExceptions.DeleteAll();
    if (m_pReturnVar)
        delete m_pReturnVar;
    if (m_pMsgBuffer)
        delete m_pMsgBuffer;
    if (m_pCorbaObject)
        delete m_pCorbaObject;
    if (m_pCorbaEnv)
        delete m_pCorbaEnv;
}

/**	\brief retrieves the name of the function
 *	\return a reference to the name
 *
 * The returned pointer should not be manipulated.
 */
String CBEFunction::GetName()
{
  return m_sName;
}

/**	\brief retrieves the return type of the function
 *	\return a reference to the return type
 *
 * Do not manipulate the returned reference directly!
 */
CBEType *CBEFunction::GetReturnType()
{
    if (!m_pReturnVar)
        return 0;
    return m_pReturnVar->GetType();
}

/**	\brief sets a new return type
 *	\param pReturnType a reference to the new return type
 */
void CBEFunction::SetReturnType(CBEType * pReturnType)
{
    ASSERT(false);
    m_pReturnVar->ReplaceType(pReturnType);
}

/**	\brief adds a new attribute to the attributes vector
 *	\param pAttribute the new attribute to add
 */
void CBEFunction::AddAttribute(CBEAttribute * pAttribute)
{
    if (!pAttribute)
        return;
    m_vAttributes.Add(pAttribute);
    pAttribute->SetParent(this);
}

/**	\brief removes an attribute from the attribute vector
 *	\param pAttribute the attribute to remove
 */
void CBEFunction::RemoveAttribute(CBEAttribute * pAttribute)
{
  if (!pAttribute)
    return;
  m_vAttributes.Remove(pAttribute);
}

/**	\brief retrieves a pointer to the first attribute
 *	\return a pointer to the first attribute
 */
VectorElement *CBEFunction::GetFirstAttribute()
{
  return m_vAttributes.GetFirst();
}

/**	\brief retrieves a reference to the next attribute
 *	\param pIter the pointer to the next attribute element
 *	\return a reference to the next attribute
 */
CBEAttribute *CBEFunction::GetNextAttribute(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    CBEAttribute *pRet = (CBEAttribute *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextAttribute(pIter);
    return pRet;
}

/**	\brief adds a new parameter to the parameters vector
 *	\param pParameter a reference to the new parameter
 */
void CBEFunction::AddParameter(CBETypedDeclarator * pParameter)
{
    if (!pParameter)
        return;
    m_vParameters.Add(pParameter);
    pParameter->SetParent(this);
}

/**	\brief removes a parameter from the parameters vector
 *	\param pParameter the parameter to remove
 */
void CBEFunction::RemoveParameter(CBETypedDeclarator * pParameter)
{
    if (!pParameter)
        return;
    m_vParameters.Remove(pParameter);
}

/**	\brief retrieves a pointer to the first parameter
 *	\return a pointer to the first parameter
 */
VectorElement *CBEFunction::GetFirstParameter()
{
    return m_vParameters.GetFirst();
}

/**	\brief retrievs a reference to the next parameter
 *	\param pIter the pointer to the next parameter
 *	\return a reference to the next paramater
 */
CBETypedDeclarator *CBEFunction::GetNextParameter(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    CBETypedDeclarator *pRet = (CBETypedDeclarator *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextParameter(pIter);
    return pRet;
}

/**	\brief adds a new sorted parameter to the sorted parameters vector
 *	\param pParameter a reference to the new sorted parameter
 */
void CBEFunction::AddSortedParameter(CBETypedDeclarator * pParameter)
{
    if (!pParameter)
        return;
    m_vSortedParameters.Add(pParameter);
    pParameter->SetParent(this);
}

/**	\brief removes a parameter from the sorted parameters vector
 *	\param pParameter the sorted parameter to remove
 */
void CBEFunction::RemoveSortedParameter(CBETypedDeclarator * pParameter)
{
    if (!pParameter)
        return;
    m_vSortedParameters.Remove(pParameter);
}

/**	\brief retrieves a pointer to the first sorted parameter
 *	\return a pointer to the first sorted parameter
 */
VectorElement* CBEFunction::GetFirstSortedParameter()
{
    return m_vSortedParameters.GetFirst();
}

/**	\brief retrievs a reference to the next sorted parameter
 *	\param pIter the pointer to the next sorted parameter
 *	\return a reference to the next sorted paramater
 */
CBETypedDeclarator* CBEFunction::GetNextSortedParameter(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    CBETypedDeclarator *pRet = (CBETypedDeclarator *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextSortedParameter(pIter);
    return pRet;
}

/**	\brief adds another exception to the excpetions vector
 *	\param pException the exception to add
 */
void CBEFunction::AddException(CBEException * pException)
{
  if (!pException)
    return;
  m_vExceptions.Add(pException);
  pException->SetParent(this);
}

/**	\brief removes an exception from the exceptions vector
 *	\param pException the exception to remove
 */
void CBEFunction::RemoveException(CBEException * pException)
{
  if (!pException)
    return;
  m_vExceptions.Remove(pException);
}

/**	\brief retrieves a pointer to the first exception
 *	\return a pointer to the first exception
 */
VectorElement *CBEFunction::GetFirstException()
{
  return m_vExceptions.GetFirst();
}

/**	\brief retrieves the next exception from the exceptions vector
 *	\param pIter the pointer to the next exception in the vector
 *	\return a reference to the next excpetion
 */
CBEException *CBEFunction::GetNextException(VectorElement * &pIter)
{
  if (!pIter)
    return 0;
  CBEException *pRet = (CBEException *) pIter->GetElement();
  pIter = pIter->GetNext();
  if (!pRet)
    return GetNextException(pIter);
  return pRet;
}

/**	\brief writes the content of the function to the target header file
 *	\param pFile the header file to write to
 *	\param pContext the context of the write operation
 *
 * A function in a header file is usually only a function declaration, except the
 * PROGRAM_GENERATE_INLINE option is set.
 *
 * This implementation first test if the target file is open and then checks the
 * options of the compiler. Based on the last check it decides which internal Write
 * function to call.
 */
void CBEFunction::Write(CBEHeaderFile * pFile, CBEContext * pContext)
{
  if (!pFile->IsOpen())
    return;

  if (pContext->IsOptionSet(PROGRAM_GENERATE_INLINE))
    {
      // write inline preix
      WriteInlinePrefix(pFile, pContext);
      WriteFunctionDefinition(pFile, pContext);
    }
  else
    WriteFunctionDeclaration(pFile, pContext);
}

/**	\brief writes the content of the function to the target implementation file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBEFunction::Write(CBEImplementationFile * pFile, CBEContext * pContext)
{
  if (!pFile->IsOpen())
    return;

  WriteFunctionDefinition(pFile, pContext);
}

/**	\brief writes the declaration of a function to the target file
 *	\param pFile the target file to write to
 *	\param pContext the context of the write operation
 *
 * A function declaration looks like this:
 *
 * <code> &lt;return type&gt; &lt;name&gt;(&lt;parameter list&gt;); </code>
 *
 * The &lt;name&gt; of the function is created by the name factory
 */
void CBEFunction::WriteFunctionDeclaration(CBEFile * pFile, CBEContext * pContext)
{
  if (!pFile->IsOpen())
    return;

  m_nParameterIndent = 0;
  // <return type>
  WriteReturnType(pFile, pContext);

  // <name> (
  pFile->Print(" %s(", (const char *) m_sName);
  m_nParameterIndent += m_sName.GetLength() + 2;

  // <parameter list>
  WriteParameterList(pFile, pContext);

  // ); newline
  pFile->Print(");\n");
}

/**	\brief writes the definition of the function to the target file
 *	\param pFile the target file to write to
 *	\param pContext the context of the write operation
 *
 * A function definition looks like this:
 *
 * <code> &lt;return type&gt; &lt;name&gt;(&lt;parameter list&gt;)<br>
 * {<br>
 * &lt;function body&gt;<br>
 * }<br>
 * </code>
 */
void CBEFunction::WriteFunctionDefinition(CBEFile * pFile, CBEContext * pContext)
{
  if (!pFile->IsOpen())
    return;

  m_nParameterIndent = 0;
  // return type
  WriteReturnType(pFile, pContext);

  // <name>(
  pFile->Print(" %s(", (const char *) m_sName);
  m_nParameterIndent += m_sName.GetLength() + 2;

  // <parameter list>
  WriteParameterList(pFile, pContext);

  // ) newline { newline
  pFile->Print(")\n{\n");
  pFile->IncIndent();

  // writes the body
  WriteBody(pFile, pContext);

  // } newline
  pFile->DecIndent();
  pFile->Print("}\n\n");
}

/**	\brief writes the return type of a function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Because the return type differs for every kind of function, we use an overloadable function
 * here.
 */
void CBEFunction::WriteReturnType(CBEFile * pFile, CBEContext * pContext)
{
    CBEType *pRetType = GetReturnType();
    if (!pRetType)
    {
        pFile->Print("void");
        m_nParameterIndent += 4;
        return;
    }
    pRetType->Write(pFile, pContext);
    m_nParameterIndent += pRetType->GetStringLength();
}

/**	\brief writes the parameter list to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The parameter list contains the parameters of the function,speerated by commas.
 */
void CBEFunction::WriteParameterList(CBEFile * pFile, CBEContext * pContext)
{
    pFile->IncIndent(m_nParameterIndent);
    // write additional parameters
    bool bComma = WriteBeforeParameters(pFile, pContext);
    // write own parameters
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParam;
    while ((pParam = GetNextParameter(pIter)) != 0)
    {
        if (bComma)
        {
            pFile->Print(",\n");
            pFile->PrintIndent("");
        }
        // print parameter
        WriteParameter(pFile, pParam, pContext);
        // set comma
        bComma = true;
    }
    // write additional parameters
    WriteAfterParameters(pFile, pContext, bComma);
    // after parameter list decrement indent
    pFile->DecIndent(m_nParameterIndent);
}

/**	\brief write a single parameter to the target file
 *	\param pFile the file to write to
 *	\param pParameter the parameter to write
 *	\param pContext the context of the write operation
 *
 * This implementation igets the first declarator of the typed declarator
 * and writes its names plus type.
 */
void CBEFunction::WriteParameter(CBEFile * pFile, CBETypedDeclarator * pParameter, CBEContext * pContext)
{
    VectorElement *pIter = pParameter->GetFirstDeclarator();
    CBEDeclarator *pDecl = pParameter->GetNextDeclarator(pIter);
    ASSERT(pDecl);
    pParameter->WriteType(pFile, pContext);
    pFile->Print(" ");
    WriteParameterName(pFile, pDecl, pContext);
}

/**	\brief writes the name of a parameter
 *	\param pFile the file to write to
 *	\param pDeclarator the declarator to write
 *	\param pContext the context of the write operation
 */
void CBEFunction::WriteParameterName(CBEFile * pFile, CBEDeclarator * pDeclarator, CBEContext * pContext)
{
    if (HasAdditionalReference(pDeclarator, pContext))
        pFile->Print("*");
    pDeclarator->WriteDeclaration(pFile, pContext);
}

/**	\brief writes additional parameters before the parameter list
 *	\param pFile the file to print to
 *	\param pContext the context of the write operation
 *	\return true if this function wrote something
 *
 * The CORBA C mapping specifies a CORBA_object to appear as first parameter.
 */
bool CBEFunction::WriteBeforeParameters(CBEFile * pFile, CBEContext * pContext)
{
    if (m_pCorbaObject)
    {
        WriteParameter(pFile, m_pCorbaObject, pContext);
        return true;
    }
    return false;
}

/**	\brief writes additional parameters after the parameter list
 *	\param pFile the file to print to
 *	\param pContext the context of th write operation
 *	\param bComma true if this function has to write a comma before writing the parameter.
 *
 * The CORBA C mapping specifies a pointer to a CORBA_Environment as last parameter
 */
void CBEFunction::WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    if (m_pCorbaEnv)
    {
        if (bComma == true)
        {
            pFile->Print(",\n");
            pFile->PrintIndent("");
        }
        WriteParameter(pFile, m_pCorbaEnv, pContext);
    }
    else
    {
        if (!bComma)
            pFile->Print("void");
    }
}

/**	\brief writes the body of the function to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBEFunction::WriteBody(CBEFile * pFile, CBEContext * pContext)
{
    // variable declaration and initialization
    WriteVariableDeclaration(pFile, pContext);
    WriteVariableInitialization(pFile, pContext);
    // prepare message
    bool bUseConstOffset = true;
    WriteMarshalling(pFile, 0, bUseConstOffset, pContext);
    // invoke message transfer
    WriteInvocation(pFile, pContext);
    // unmarshal response
    bUseConstOffset = true;
    WriteUnmarshalling(pFile, 0, bUseConstOffset, pContext);
    // clean up and return
    WriteCleanup(pFile, pContext);
    WriteReturn(pFile, pContext);
}

/**	\brief writes the declaration of the variables
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * \todo check if this belongs to CBEOperationFunction
 */
void CBEFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    ASSERTC(false);
}

/**	\brief writes the initialization of the variables
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * \todo check if this belongs to CBEOperationFunction
 */
void CBEFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    ASSERTC(false);
}

/**	\brief writes the preparation of the message transfer
 *	\param pFile the file to write to
 *  \param nStartOffset the offset where the marshalling starts
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *	\param pContext the context of the write operation
 *
 * This function creates a marshaller and lets it marshal this function.
 */
void CBEFunction::WriteMarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    pMarshaller->Marshal(pFile, this, nStartOffset, bUseConstOffset, pContext);
    delete pMarshaller;
}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * \todo check if this belongs to CBEOperationFunction
 */
void CBEFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    ASSERTC(false);
}

/**	\brief writes the clean up of the function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * \todo check if this belongs to CBEOperationFunction
 */
void CBEFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
  ASSERTC(false);
}

/**	\brief writes the return statement of the function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This function writes the return statement using the m_pReturnVar member.
 * Set this member respectively or overload this function if you wish to
 * obtain different results.
 *
 * We do not write a return statement if the return type is void.
 */
void CBEFunction::WriteReturn(CBEFile * pFile, CBEContext * pContext)
{
	if (!GetReturnVariable())
		return;
    if (GetReturnType()->IsVoid())
        return;

    pFile->PrintIndent("return ");
    // get return var name, which is first declarator
    VectorElement *pIter = m_pReturnVar->GetFirstDeclarator();
    CBEDeclarator *pRetVar = m_pReturnVar->GetNextDeclarator(pIter);
    pRetVar->WriteDeclaration(pFile, pContext);
    pFile->Print(";\n");
}

/** \brief writes the unmarshlling of the message buffer
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the write operation
 *
 * This function creates a marshaller and lets it unmarshal this function
 */
void CBEFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    pMarshaller->Unmarshal(pFile, this, nStartOffset, bUseConstOffset, pContext);
    delete pMarshaller;
}

/**	\brief optimizes this function
 *	\param nLevel the optimization level
 *  \param pContext the context of the optimization
 *	\return success or failure code
 *
 * This implementation does different thing for different optimization levels.
 * - For level zero: nothing
 * - For level one: sorting the parameter by size (the smallest first, the variable sized last)
 */
int CBEFunction::Optimize(int nLevel, CBEContext *pContext)
{
    switch (nLevel)
    {
    case 0:
        return 0;
        break;
    case 1:
    case 2:
        return SortParameters(0, pContext);   // XXX FIXME
        break;
    default:
        return 0;
        break;
    }

    return 0;
}

/**	\brief writes a function call to this function
 *	\param pFile the file to write to
 *  \param sReturnVar the name of the variable, which will be assigned the return value
 *	\param pContext the context of the write operation
 *
 * We always assume that a function call uses the same parameter names as the function declaration.
 * This makes it simple to write a function call without actually knowing the variable names. The return
 * variable's name is given to us. If it is 0 the caller wants no return value.
 */
void CBEFunction::WriteCall(CBEFile * pFile, String sReturnVar, CBEContext * pContext)
{
    m_nParameterIndent = 0;
    if (GetReturnType())
    {
        if ((!sReturnVar.IsEmpty()) && (!GetReturnType()->IsVoid()))
        {
            pFile->Print("%s = ", (const char *) sReturnVar);
            m_nParameterIndent += sReturnVar.GetLength() + 3;
        }
    }

    pFile->Print("%s(", (const char *) GetName());
    m_nParameterIndent += GetName().GetLength() + 1;
    WriteCallParameterList(pFile, pContext);
    pFile->Print(");");
}

/**	\brief writes the parameter list for a function call
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBEFunction::WriteCallParameterList(CBEFile * pFile, CBEContext * pContext)
{
    pFile->IncIndent(m_nParameterIndent);

    bool bComma = WriteCallBeforeParameters(pFile, pContext);

    VectorElement *pIter = GetFirstCallParameter();
    CBETypedDeclarator *pParam;
    while ((pParam = GetNextCallParameter(pIter)) != 0)
    {
        if (bComma)
        {
            pFile->Print(",\n");
            pFile->PrintIndent("");
        }
        WriteCallParameter(pFile, pParam, pContext);
        bComma = true;
    }

    WriteCallAfterParameters(pFile, pContext, bComma);

    pFile->DecIndent(m_nParameterIndent);
}

/**	\brief writes a single parameter for the function call
 *	\param pFile the file to write to
 *	\param pParameter the parameter to be written
 *	\param pContext the context of the write operation
 */
void CBEFunction::WriteCallParameter(CBEFile * pFile, CBETypedDeclarator * pParameter, CBEContext * pContext)
{
    VectorElement *pIter = pParameter->GetFirstDeclarator();
    CBEDeclarator *pOriginalDecl = pParameter->GetNextDeclarator(pIter);
    CBEDeclarator *pCallDecl = pParameter->GetNextDeclarator(pIter);
    if (!pCallDecl)
        pCallDecl = pOriginalDecl;
    WriteCallParameterName(pFile, pOriginalDecl, pCallDecl, pContext);
}

/**	\brief writes the name of a parameter
 *	\param pFile the file to write to
 *	\param pInternalDecl the internal parameter
 *  \param pExternalDecl the calling parameter
 *	\param pContext the context of th write operation
 *
 * Because the common Write function of CBEDeclarator writes a parameter definition we need something
 * else to write the parameter for a function call. This is simply the name of the parameter.
 *
 * To know wheter  we have to reference or dereference the parameter, we compare the number of
 * stars between the original (internal) parameter and the call (external) parameter. If:
 * - the stars of external > stars of internal we need dereferencing: '*' x (stars external - stars internal)
 * - the stars of external < stars of internal we need referencing: '&' x -(stars external - stars internal)
 * - the stars of external == stars of internal we need nothing.
 */
void CBEFunction::WriteCallParameterName(CBEFile * pFile, CBEDeclarator * pInternalDecl, CBEDeclarator * pExternalDecl, CBEContext * pContext)
{
    // check stars
    int nDiffStars = pExternalDecl->GetStars() - pInternalDecl->GetStars(), nCount;
    if (HasAdditionalReference(pInternalDecl, pContext, true))
        nDiffStars--;
    for (nCount = nDiffStars; nCount < 0; nCount++)
    {
        pFile->Print("&");
        if (nCount+1 < 0)
            pFile->Print("(");
    }
    for (nCount = nDiffStars; nCount > 0; nCount--)
        pFile->Print("*");
    if (nDiffStars > 0)
        pFile->Print("(");
    pFile->Print("%s", (const char *) pExternalDecl->GetName());
    if (nDiffStars > 0)
        pFile->Print(")");
    for (nCount = nDiffStars+1; nCount < 0; nCount++)
        pFile->Print(")");
}

/**	\brief writes additional parameters before other parameters for function call
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *	\return true if something was written
 *
 * Print the parameter for the CORBA_Object.
 */
bool CBEFunction::WriteCallBeforeParameters(CBEFile * pFile, CBEContext * pContext)
{
    if (m_pCorbaObject)
    {
        WriteCallParameter(pFile, m_pCorbaObject, pContext);
        return true;
    }
    return false;
}

/**	\brief write additional parameters after other parameters for function call
 *	\param pFile the file to write to
 *	\param pContext the context of th write operation
 *	\param bComma true if there has to be a comma before the parameter
 *
 * Print the parameter for the CORBA_Environment
 */
void CBEFunction::WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    if (m_pCorbaEnv)
    {
        if (bComma)
        {
            pFile->Print(",\n");
            pFile->PrintIndent("");
        }
        WriteCallParameter(pFile, m_pCorbaEnv, pContext);
    }
}

/**	\brief searches for an attribute and returns reference to it
 *	\param nAttrType the type of the searched attribute
 *	\return reference to the attribute or 0 if not found
 */
CBEAttribute *CBEFunction::FindAttribute(int nAttrType)
{
  VectorElement *pIter = GetFirstAttribute();
  CBEAttribute *pAttr;
  while ((pAttr = GetNextAttribute(pIter)) != 0)
    {
      if (pAttr->GetType() == nAttrType)
	return pAttr;
    }
  return 0;
}

/**	\brief writes the inline prefix just before a function definition
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBEFunction::WriteInlinePrefix(CBEFile * pFile, CBEContext * pContext)
{
  String sInline = pContext->GetNameFactory()->GetInlinePrefix(pContext);
  pFile->Print("%s ", (const char *) sInline);
}

/**	\brief counts the number of string parameters
 *	\param nDirection the direction to count
 *  \param nMustAttrs the attributes which have to be set for the parameters
 *  \param nMustNotAttrs the attributes which must not be set for the parameters
 *	\return the number of string parameters
 *
 * A string is every parameter, which's function IsString returns true.
 */
int CBEFunction::GetStringParameterCount(int nDirection, int nMustAttrs, int nMustNotAttrs)
{
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParam;
    int nCount = 0;
    while ((pParam = GetNextParameter(pIter)) != 0)
    {
        // if explicitely ATTR_IN (means only in) and dir is not in or
        // if explicitely ATTR_OUT (means only out) and dir is not out
        // then skip
        if (!pParam->IsDirection(nDirection))
            continue;
        // test for attributes that it must have (if it hasn't, skip count)
        bool bCount = true;
        if (!(pParam->FindAttribute(nMustAttrs)))
            bCount = false;
        // test for attributes that it must NOT have (skip count)
        if (pParam->FindAttribute(nMustNotAttrs) != 0)
            bCount = false;
        // count if string
        if (pParam->IsString() && bCount)
            nCount++;
    }
    if (m_pReturnVar)
    {
        if (m_pReturnVar->IsDirection(nDirection))
        {
            bool bCount = true;
            if (!m_pReturnVar->FindAttribute(nMustAttrs))
                bCount = false;
            if (m_pReturnVar->FindAttribute(nMustNotAttrs))
                bCount = false;
            if (m_pReturnVar->IsString() && bCount)
                nCount++;
        }
    }
    return nCount;
}

/**	\brief counts number of variable sized parameters
 *	\param nDirection the direction to count
 *	\return the number of variable sized parameters
 */
int CBEFunction::GetVariableSizedParameterCount(int nDirection)
{
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParam;
    int nCount = 0;
    while ((pParam = GetNextParameter(pIter)) != 0)
    {
         // if explicitely ATTR_IN (means only in) and dir is not in or
         // if explicitely ATTR_OUT (means only out) and dir is not out
         // then skip
         if (!pParam->IsDirection(nDirection))
             continue;
         if (pParam->IsVariableSized())
             nCount++;
    }
    if (m_pReturnVar)
    {
        if (m_pReturnVar->IsDirection(nDirection) && m_pReturnVar->IsVariableSized())
            nCount++;
    }
    return nCount;
}

/**	\brief calculates the total size of the parameters
 *	\param nDirection the direction to count
 *  \param pContext the context of this calculation
 *	\return the size of the parameters
 *
 * The size of the function's parameters is the sum of the parameter's sizes. The size of a parameter
 * is calculcated by CBETypedDeclarator::GetSize().
 *
 * \todo If member is indirect, we should add size of size-attr instead of size of type
 */
int CBEFunction::GetSize(int nDirection, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParam;
    int nSize = 0;
    while ((pParam = GetNextParameter(pIter)) != 0)
    {
        // if direction is in and no ATTR_IN  or
        // if direction is out and no ATTR_OUT
        // then skip
        if (!pParam->IsDirection(nDirection))
            continue;
        // a function cannot have bitfield parameters, so we don't need to test for those
        // if size is negative (param is variable sized), then add parameters base type
        // BTW: do not count message buffer parameter
        if (m_pMsgBuffer)
        {
            if (m_pMsgBuffer->GetAlias())
            {
                String sMsgBuf = m_pMsgBuffer->GetAlias()->GetName();
                if (pParam->FindDeclarator(sMsgBuf))
                    continue;
            }
        }
        if (pParam->IsVariableSized())
            nSize += pParam->GetType()->GetSize();   // FIXME: get size of size-attr
        else
        {
            // GetSize checks for referenced out and struct
            // but not for simple pointered vars, which
            // should be dereferenced for transmition
            int nParamSize = pParam->GetSize();
            if (nParamSize < 0)
            {
                // pointer (without size attributes!)
                nSize += pParam->GetType()->GetSize();
            }
            else
                nSize += nParamSize;
        }
    }
    // add return var's size
    nSize += GetReturnSize(nDirection, pContext);
    return nSize;
}

/** \brief returns the fixed size of the return variable
 *  \param nDirection the direction to count the return variable for
 *  \param pContext the context of the counting
 *  \return the fixed size of the return variable in bytes
 *
 * We put this into an extra function, because with some functions the return
 * variable is NOT a parameter to transferred. Which means that some functions
 * will return 0 even though they have an return value.
 */
int CBEFunction::GetReturnSize(int nDirection, CBEContext *pContext)
{
    if (m_pReturnVar)
    {
        if (m_pReturnVar->IsDirection(nDirection))
        {
            if (m_pReturnVar->IsVariableSized())
                return m_pReturnVar->GetType()->GetSize();
            else
            {
                int nParamSize = m_pReturnVar->GetSize();
                if (nParamSize < 0)
                    return m_pReturnVar->GetType()->GetSize();
                else
                    return nParamSize;
            }
        }
    }
    return 0;
}

/** \brief calculates the maximum size of a function's message buffer for a given direction
 *  \param nDirection the direction to count
 *  \param pContext the context of the counting
 *  \return the size in bytes
 *
 * The maximum size also count variable sized parameters, which have a max attribute or boundary.
 *
 * \todo: issue warning if no max size available
 */
int CBEFunction::GetMaxSize(int nDirection, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParam;
    int nSize = 0;
    while ((pParam = GetNextParameter(pIter)) != 0)
    {
        // if direction is in and no ATTR_IN  or
        // if direction is out and no ATTR_OUT
        // then skip
        if (!pParam->IsDirection(nDirection))
            continue;
        // a function cannot have bitfield parameters, so we don't need to test for those
        // if size is negative (param is variable sized), then add parameters base type
        // BTW: do not count message buffer parameter
        if (m_pMsgBuffer)
        {
            if (m_pMsgBuffer->GetAlias())
            {
                String sMsgBuf = m_pMsgBuffer->GetAlias()->GetName();
                if (pParam->FindDeclarator(sMsgBuf))
                    continue;
            }
        }

        // GetMaxSize already checks for variable sized params and
        // tries to find their MAX values. If there is no way to determine
        // them, it returns a negative value. (should issue a warning)
        int nParamSize = pParam->GetMaxSize(pContext);
        if (nParamSize < 0)
        {
            if (pContext->IsWarningSet(PROGRAM_WARNING_NO_MAXSIZE))
            {
                VectorElement *pI = pParam->GetFirstDeclarator();
                CBEDeclarator *pD = pParam->GetNextDeclarator(pI);
                CCompiler::Warning("%s in %s has no maximum size", (const char*)pD->GetName(), (const char*)GetName());
            }
            nSize += pContext->GetSizes()->GetMaxSizeOfType(pParam->GetType()->GetFEType());
        }
        else
            nSize += nParamSize;
    }
    // add return var's size
    nSize += GetMaxReturnSize(nDirection, pContext);
    return nSize;
}

/** \brief returns the maximum size of the return variable
 *  \param nDirection the direction to count the return variable for
 *  \param pContext the context of the counting
 *  \return the maximum size of the return variable in bytes
 *
 * We put this into an extra function, because with some functions the return
 * variable is NOT a parameter to transferred. Which means that some functions
 * will return 0 even though they have an return value.
 */
int CBEFunction::GetMaxReturnSize(int nDirection, CBEContext *pContext)
{
    if (m_pReturnVar)
    {
        if (m_pReturnVar->IsDirection(nDirection))
        {
            int nParamSize = m_pReturnVar->GetMaxSize(pContext);
            if (nParamSize < 0) // cannot issue warning, since return type cannot have attributes defined
                return pContext->GetSizes()->GetMaxSizeOfType(m_pReturnVar->GetType()->GetFEType());
            else
                return nParamSize;
        }
    }
    return 0;
}

/** \brief count the size of all fixed sized parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this counting
 *  \return the size in bytes of all fixed sized parameters
 *
 * The difference between this function and the normal GetSize function is, that the
 * above function counts array, which have a fixed upper bound, but also a size_is or
 * length_is parameter, which makes them effectively variable sized parameters.
 */
int CBEFunction::GetFixedSize(int nDirection, CBEContext *pContext)
{
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParam;
    int nSize = 0;
    while ((pParam = GetNextParameter(pIter)) != 0)
    {
        // if direction is in and no ATTR_IN  or
        // if direction is out and no ATTR_OUT
        // then skip
        if (!pParam->IsDirection(nDirection))
            continue;
        // function cannot have bitfield params, so we dont test for them
        // if param is not variable sized, then add its size
        // BTW: do not count message buffer parameter
        if (m_pMsgBuffer)
        {
            if (m_pMsgBuffer->GetAlias())
            {
                String sMsgBuf = m_pMsgBuffer->GetAlias()->GetName();
                if (pParam->FindDeclarator(sMsgBuf))
                    continue;
            }
        }
        // only count fixed sized
        if (pParam->IsFixedSized())
        {
            // pParam->GetSize also tests for referenced OUT
            // and referenced structs and delivers correct size
            // BUT: it does not check for pointered vars, which
            // should be dereferenced
            int nParamSize = pParam->GetSize();
            if (nParamSize < 0)
            {
                // pointer (without size attributes!)
                if (pParam->GetType())
                    nSize += pParam->GetType()->GetSize();
                else
                {
                    VectorElement *pI = pParam->GetFirstDeclarator();
                    CBEDeclarator *pD = pParam->GetNextDeclarator(pI);
                    CCompiler::Error("%s in %s has no type\n", (const char*)pD->GetName(), (const char*)GetName());
                }
            }
            else
                nSize += nParamSize;
        }
    }
    // add return var's size
    nSize += GetFixedReturnSize(nDirection, pContext);
    return nSize;
}

/** \brief returns the size of the return variable
 *  \param nDirection the direction to count the return variable for
 *  \param pContext the context of the counting
 *  \return the size of the return variable in bytes
 *
 * We put this into an extra function, because with some functions the return
 * variable is NOT a parameter to transferred. Which means that some functions
 * will return 0 even though they have an return value.
 */
int CBEFunction::GetFixedReturnSize(int nDirection, CBEContext *pContext)
{
    if (m_pReturnVar)
    {
        if (m_pReturnVar->IsDirection(nDirection) && m_pReturnVar->IsFixedSized())
        {
            int nParamSize = m_pReturnVar->GetSize();
            if (nParamSize < 0)
                return m_pReturnVar->GetType()->GetSize();
            else
                return nParamSize;
        }
    }
    return 0;
}

/**	\brief adds a reference to the message buffer to the parameter list
 *	\param pFEInterface the responding front-end interface
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * We add a parameter of user defined type (the message buffer type has been declared before) with
 * the message buffer variable name and one asterisk. This way we use the message buffer by reference.
 *
 * If we have a derived server-loop the functions of the base interface should use the message
 * buffer of the derived interface as well. We therefore reset the message buffer parameter in
 * CBESrvLoopFunction::CreateBE!
 */
bool CBEFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    m_pMsgBuffer = pContext->GetClassFactory()->GetNewMessageBufferType();
    m_pMsgBuffer->SetParent(this);
    if (!m_pMsgBuffer->CreateBackEnd(pFEInterface, pContext))
    {
        delete m_pMsgBuffer;
        m_pMsgBuffer = 0;
        return false;
    }
    return true;
}

/** \brief adds a message buffer parameter for a specific function
 *  \param pFEOperation the reference front-end operation
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBEFunction::AddMessageBuffer(CFEOperation *pFEOperation, CBEContext * pContext)
{
    m_pMsgBuffer = pContext->GetClassFactory()->GetNewMessageBufferType();
    m_pMsgBuffer->SetParent(this);
    if (!m_pMsgBuffer->CreateBackEnd(pFEOperation, pContext))
    {
     	delete m_pMsgBuffer;
        m_pMsgBuffer = 0;
   	    return false;
    }
    return true;
}

/**	\brief marshals the return value
 *	\param pFile the file to write to
 *	\param nStartOffset the offset at which the return value should be marshalled
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *	\param pContext the context of the write operation
 *	\return the number of bytes used for the return value
 *
 * This function assumes that it is called before the marshalling of the other parameters.
 */
int CBEFunction::WriteMarshalReturn(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    if (!m_pReturnVar)
        return 0;
    if (GetReturnType()->IsVoid())
        return 0;
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    int nSize = pMarshaller->Marshal(pFile, m_pReturnVar, nStartOffset, bUseConstOffset, m_vParameters.GetSize() == 0, pContext);
    delete pMarshaller;
    return nSize;
}

/**	\brief unmarshals the return value
 *	\param pFile the file to write to
 *	\param nStartOffset the offset in the message buffer to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *	\param pContext the context of the write operation
 *	\return thr number of bytes unmarshalled
 */
int CBEFunction::WriteUnmarshalReturn(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    if (!m_pReturnVar)
        return 0;
    if (GetReturnType()->IsVoid())
        return 0;
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    int nSize = pMarshaller->Unmarshal(pFile, m_pReturnVar, nStartOffset, bUseConstOffset, m_vParameters.GetSize() == 0, pContext);
    delete pMarshaller;
    return nSize;
}

/**	\brief initializes and sets the return var to a new value
 *	\param bUnsigned if the new return type should be unsigned
 *	\param nSize the size of the type
 *	\param nFEType the front-end type number
 *	\param sName the new name of the return variable
 *	\param pContext the context of the create process
 *	\return true if successful
 *
 * The type and the name should not be initialized yet. This is all done by this function.
 */
bool CBEFunction::SetReturnVar(bool bUnsigned, int nSize, int nFEType, String sName, CBEContext * pContext)
{
    // delete old
    if (m_pReturnVar)
        delete m_pReturnVar;
    CBEType *pType = pContext->GetClassFactory()->GetNewType(nFEType);
    if (!pType)
        return false;
    m_pReturnVar = pContext->GetClassFactory()->GetNewTypedDeclarator();
    m_pReturnVar->SetParent(this);
    pType->SetParent(m_pReturnVar);
    if (!pType->CreateBackEnd(bUnsigned, nSize, nFEType, pContext))
    {
        delete pType;
        delete m_pReturnVar;
        m_pReturnVar = 0;
        return false;
    }
    if (!m_pReturnVar->CreateBackEnd(pType, sName, pContext))
    {
        delete pType;
        delete m_pReturnVar;
        m_pReturnVar = 0;
        return false;
    }
    delete pType;		// is cloned by typed decl.
    // add OUT attribute
    CBEAttribute *pAttr = pContext->GetClassFactory()->GetNewAttribute();
    m_pReturnVar->AddAttribute(pAttr);
    if (!pAttr->CreateBackEnd(ATTR_OUT, pContext))
    {
        m_pReturnVar->RemoveAttribute(pAttr);
        delete pAttr;
    }
    return true;
}

/**	\brief initializes and sets the return var to a new value
 *	\param pType the new type of the return variable
 *	\param sName the new name of the return variable
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * The type and the name should not be initialized yet. This is all done by this function.
 */
bool CBEFunction::SetReturnVar(CBEOpcodeType * pType, String sName, CBEContext * pContext)
{
    // delete old
    if (m_pReturnVar)
        delete m_pReturnVar;
    if (!pType)
        return false;
    m_pReturnVar = pContext->GetClassFactory()->GetNewTypedDeclarator();
    m_pReturnVar->SetParent(this);
    pType->SetParent(m_pReturnVar);
    if (!pType->CreateBackEnd(pContext))
    {
        delete pType;
        delete m_pReturnVar;
        m_pReturnVar = 0;
        return false;
    }
    if (!m_pReturnVar->CreateBackEnd(pType, sName, pContext))
    {
        delete pType;
        delete m_pReturnVar;
        m_pReturnVar = 0;
        return false;
    }
    delete pType;		// is cloned by typed decl
    // add OUT attribute
    CBEAttribute *pAttr = pContext->GetClassFactory()->GetNewAttribute();
    m_pReturnVar->AddAttribute(pAttr);
    if (!pAttr->CreateBackEnd(ATTR_OUT, pContext))
    {
        m_pReturnVar->RemoveAttribute(pAttr);
        delete pAttr;
    }
    return true;
}

/**	\brief initializes and sets the return var to a new value
 *	\param pFEType the front-end type to use as reference for the new type
 *	\param sName the name of the variable
 *	\param pContext the context of the code generation
 *	\return true if successful
 */
bool CBEFunction::SetReturnVar(CFETypeSpec * pFEType, String sName, CBEContext * pContext)
{
    // delete old
    if (m_pReturnVar)
        delete m_pReturnVar;
    if (!pFEType)
        return false;
    CBEType *pType = pContext->GetClassFactory()->GetNewType(pFEType->GetType());
    m_pReturnVar = pContext->GetClassFactory()->GetNewTypedDeclarator();
    m_pReturnVar->SetParent(this);
    pType->SetParent(m_pReturnVar);
    if (!pType->CreateBackEnd(pFEType, pContext))
    {
        delete pType;
        delete m_pReturnVar;
        m_pReturnVar = 0;
        return false;
    }
    if (!m_pReturnVar->CreateBackEnd(pType, sName, pContext))
    {
        delete pType;
        delete m_pReturnVar;
        m_pReturnVar = 0;
        return false;
    }
    delete pType;		// cloned by typed declarator
    // if string, say so
    if (pFEType->GetType() == TYPE_CHAR_ASTERISK)
    {
        CBEAttribute *pNewAttr = pContext->GetClassFactory()->GetNewAttribute();
        m_pReturnVar->AddAttribute(pNewAttr);
        if (!pNewAttr->CreateBackEnd(ATTR_STRING, pContext))
        {
            m_pReturnVar->RemoveAttribute(pNewAttr);
            delete pNewAttr;
        }
    }
    // add OUT attribute
    CBEAttribute *pAttr = pContext->GetClassFactory()->GetNewAttribute();
    m_pReturnVar->AddAttribute(pAttr);
    if (!pAttr->CreateBackEnd(ATTR_OUT, pContext))
    {
        m_pReturnVar->RemoveAttribute(pAttr);
        delete pAttr;
    }
    return true;
}

/**	\brief searches for a parameter with the given name
 *	\param sName the name of the parameter
 *  \param bCall true if an call parameter is searched
 *	\return a reference to the parameter or 0 if not found
 */
CBETypedDeclarator *CBEFunction::FindParameter(String sName, bool bCall)
{
    if (bCall)
    {
        VectorElement *pIter = GetFirstCallParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = GetNextCallParameter(pIter)) != 0)
        {
            if (pParameter->FindDeclarator(sName))
                return pParameter;
        }
    }
    else
    {
        VectorElement *pIter = GetFirstParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = GetNextParameter(pIter)) != 0)
        {
            if (pParameter->FindDeclarator(sName))
                return pParameter;
        }
    }
    return 0;
}

/** \brief simply tests if this parameter should be marshalled
 *  \param pParameter the parameter to test
 *  \param pContext the context of this marshalling
 *  \return true if this parameter should be marshalled
 *
 * By default we marshal all parameters, so simply return true.
 */
bool CBEFunction::DoMarshalParameter(CBETypedDeclarator *pParameter, CBEContext *pContext)
{
    return true;
}

/** \brief simply tests if this parameter should be unmarshalled
 *  \param pParameter the parameter to test
 *  \param pContext the context of the unmarshalling
 *  \return true if this parameter should be unmarshalled
 *
 * By default we unmarshal all parameters, so simply return true.
 */
bool CBEFunction::DoUnmarshalParameter(CBETypedDeclarator *pParameter, CBEContext *pContext)
{
    return true;
}

/** \brief checks whether a given parameter needs an additional reference pointer
 *  \param pDeclarator the decl to check
 *  \param pContext the context of the operation
 *  \param bCall true if the parameter is a call parameter
 *  \return true if we need a reference
 *
 * An additional reference might be suitable if the parameter is declared without it, but
 * might be used in multiple places with an reference.
 *
 * By default a parameter needs no additional reference.
 */
bool CBEFunction::HasAdditionalReference(CBEDeclarator *pDeclarator, CBEContext *pContext, bool bCall)
{
    return false;
}

/** \brief checks if this function has variable sized parameters
 *  \return true if it has
 *
 * Parameters are variable sized if they are variable sized arrays or strings (meaning
 * are associated with a string attribute).
 */
bool CBEFunction::HasVariableSizedParameters(int nDirection)
{
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = GetNextParameter(pIter)) != 0)
    {
        if (!pParameter->IsDirection(nDirection))
             continue;
        if (pParameter->IsVariableSized() || pParameter->IsString())
            return true;
    }
    if (m_pReturnVar)
    {
        if (m_pReturnVar->IsDirection(nDirection))
        {
            if (m_pReturnVar->IsVariableSized() || m_pReturnVar->IsString())
                return true;
        }
    }
    return false;
}

/** \brief sorts the parameters of this function according to the given mode
 *  \param nMode the sorting mode
 *  \param pContext the context of the sorting
 *  \return success status (zero if successful)
 *
 * We do something like bubble-sort. We start from the beginning and check whether the
 * parameter after the current is smaller. If it is they are exchanged. We do this as long
 * as there is something to exchange.
 */
int CBEFunction::SortParameters(int nMode, CBEContext *pContext)
{
    bool bDidSomething;
    do
    {
        // reset did something
        bDidSomething = false;
        // move one bubble through the stack
        VectorElement *pIter = GetFirstSortedParameter();
        CBETypedDeclarator *pParameter;
        while ((pParameter = GetNextSortedParameter(pIter)) != 0)
        {
            // get successor
            VectorElement *pIterS = pIter;
            CBETypedDeclarator *pSuccessor = GetNextSortedParameter(pIterS);
            if (pSuccessor)
            {
                bool bExchange = DoSortParameters(pParameter, pSuccessor, pContext);
                if (bExchange)
                {
                    // iterators stay the same, so the next GetNextParameter will return
                    // the same parameter. Thus we can push this bubble through the whole
                    // parameter list
                    if (!m_vSortedParameters.Exchange(pParameter, pSuccessor))
                        return 1;
                    bDidSomething = true;
                }
            }
        }
    }
    while (bDidSomething);
    return 0;
}

/** \brief decides whether parameters are exchanged during sorting
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *  \param pContext the context of the sorting
 *  \return true if parameter should be exchanged
 *
 * - check if either is variable sized
 * - if pParameter is variable sized and successor fixed it will pass
 * (if both var sized: they might always be exchanged -> infinite loop)
 * - if pParameter is fixes sized and pSuccessor is variable, it won't pass
 * - if both are fixed sized, the size determines, whether it passes or not
 * because parameters of a function cannot contain bitfields we don't need
 * to test for GetSize() == 0.
 */
bool CBEFunction::DoSortParameters(CBETypedDeclarator *pPrecessor, CBETypedDeclarator *pSuccessor, CBEContext *pContext)
{
    return (pPrecessor->IsVariableSized() && !(pSuccessor->IsVariableSized()))
        || (((!pPrecessor->IsVariableSized()) && (!pSuccessor->IsVariableSized()))
        && (pPrecessor->GetSize() > pSuccessor->GetSize()));
}

/** \brief retrieves a pointer to the interface, which owns this function
 *  \return a reference to the interface
 */
CBEClass* CBEFunction::GetClass()
{
    CBEObject *pCurrent = (CBEObject*)GetParent();
    while (pCurrent)
    {
        if (pCurrent->IsKindOf(RUNTIME_CLASS(CBEClass)))
            return (CBEClass*)pCurrent;
        pCurrent = (CBEObject*)(pCurrent->GetParent());
    }
    return 0;
}

/** \brief counts the parameters of a specific front-end type
 *  \param nFEType the type to count
 *  \param nDirection the direction to count
 *  \return the number of parameters, which have this type
 */
int CBEFunction::GetParameterCount(int nFEType, int nDirection)
{
    if (nDirection == 0)
    {
        int nCountIn = GetParameterCount(nFEType, DIRECTION_IN);
        int nCountOut = GetParameterCount(nFEType, DIRECTION_OUT);
        return MAX(nCountIn, nCountOut);
    }

    int nCount = 0;

    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = GetNextParameter(pIter)) != 0)
    {
        if (!pParameter->IsDirection(nDirection))
            continue;
        if (pParameter->GetType()->IsOfType(nFEType))
            nCount++;
    }

    return nCount;
}

/** \brief counts parameters by their attributes
 *  \param nMustAttrs the attribute that must be set for the parameter
 *  \param nMustNotAttrs the attribute that must _not_ be set for the parameter
 *  \param nDirection the direction to count
 *  \return the number of parameters with or without the specified attributes
 */
int CBEFunction::GetParameterCount(int nMustAttrs, int nMustNotAttrs, int nDirection)
{
    if (nDirection == 0)
    {
        int nCountIn = GetParameterCount(nMustAttrs, nMustNotAttrs, DIRECTION_IN);
        int nCountOut = GetParameterCount(nMustAttrs, nMustNotAttrs, DIRECTION_OUT);
        return MAX(nCountIn, nCountOut);
    }

    int nCount = 0;
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = GetNextParameter(pIter)) != 0)
    {
        if (!pParameter->IsDirection(nDirection))
            continue;
        // test for attributes that it must have (if it hasn't, skip count)
        if (!(pParameter->FindAttribute(nMustAttrs)))
            continue;
        // test for attributes that it must NOT have (skip count)
        if (pParameter->FindAttribute(nMustNotAttrs))
            continue;
        // count 
        nCount++;
    }
    return nCount;    
}

/** \brief sets the m_bCastMsgBufferOnCall member
 *  \param bCastMsgBufferOnCall the new value
 *
 * If the message buffer variable should be casted to internal
 * message buffer type when calling this function,then this is true.
 */
void CBEFunction::SetMsgBufferCastOnCall(bool bCastMsgBufferOnCall)
{
    m_bCastMsgBufferOnCall = bCastMsgBufferOnCall;
}

/** \brief adds this function to the header files
 *  \param pHeader the header file
 *  \param pContext the context of the creation
 *  \return true if successful
 *
 * This function should be overloaded, because the functions should be added to the
 * files depending on their instance and attributes.
 */
bool CBEFunction::AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext)
{
    if (IsTargetFile(pHeader))
		pHeader->AddFunction(this);
    return false;
}

/** \brief checks if this function belings to the component side
 *  \return true if true
 */
bool CBEFunction::IsComponentSide()
{
    return m_bComponentSide;
}

/** \brief sets the communication side
 *  \param bComponentSide if true its the component's side, if false the client's
 */
void CBEFunction::SetComponentSide(bool bComponentSide)
{
    m_bComponentSide = bComponentSide;
}

/** \brief adds this function to the implementation file
 *  \param pImpl the implementation file
 *  \param pContext the context of the operation
 *
 * This is usually only used for global functions.
 */
bool CBEFunction::AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext)
{
    if (IsTargetFile(pImpl))
		pImpl->AddFunction(this);
    return false;
}

/** \brief tests if this function has array parameters
 *  \param nDirection the direction to test
 *  \return true if it has
 */
bool CBEFunction::HasArrayParameters(int nDirection)
{
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = GetNextParameter(pIter)) != 0)
    {
         if (!pParameter->IsDirection(nDirection))
             continue;
        VectorElement *pTestI = pParameter->GetFirstDeclarator();
        CBEDeclarator *pDecl = pParameter->GetNextDeclarator(pTestI);
        if (pDecl->IsArray())
        {
            return true;
        }
    }
    if (m_pReturnVar)
    {
        if (m_pReturnVar->IsDirection(nDirection))
        {
            VectorElement *pTestI = m_pReturnVar->GetFirstDeclarator();
            CBEDeclarator *pDecl = m_pReturnVar->GetNextDeclarator(pTestI);
            if (pDecl->IsArray())
            {
                return true;
            }
        }
    }
    return false;
}

/** \brief tests if this function should be written
 *  \param pFile the target file this function should be added to
 *  \param pContext the context of the write operations
 *  \return true if successful
 *
 * This is a "pure virtual" function, which means it HAS to be overloaded.
 */
bool CBEFunction::DoWriteFunction(CBEFile *pFile, CBEContext *pContext)
{
	ASSERTC(false);
	return false;
}

/** \brief tries to find a parameter with a specific type
 *  \param sTypeName the name of the type to look for
 *  \return a reference to the found parameter
 *
 * We test the regular parameter and if it exists the message buffer.
 */
CBETypedDeclarator* CBEFunction::FindParameterType(String sTypeName)
{
    VectorElement *pIter = GetFirstParameter();
    CBETypedDeclarator *pParameter;
    while ((pParameter = GetNextParameter(pIter)) != 0)
    {
        CBEType *pType = pParameter->GetType();
        if (pType->IsKindOf(RUNTIME_CLASS(CBEUserDefinedType)))
        {
            if (((CBEUserDefinedType*)pType)->GetName() == sTypeName)
                return pParameter;
        }
        if (pType->HasTag(sTypeName))
            return pParameter;
    }
    return 0;
}

/** \brief get the direction in which this function sends
 *  \return DIRECTION_IN if sending to server, DIRECTION_OUT otherwise
 *
 * The "sending" direction is the one where data is marshalled into the
 * message buffer. This is for a call function the direction DIRECTION_IN,
 * but for a reply function the direction DIRECTION_OUT.
 *
 * Default is DIRECTION_IN.
 *
 * Use the function IsComponentSide() to determine if this function is at the
 * sever's side or not.
 */
int CBEFunction::GetSendDirection()
{
    return DIRECTION_IN;
}

/** \brief get the direction from which this function receives
 *  \return DIRECTION_OUT if receiving from server, DIRECTION_IN otherwise
 *
 * The "receiving" direction is the one where data is unmarshalled from the
 * message buffer. This is for a call function the direction DIRECTION_OUT,
 * but for a reply-and-wait function the direction DIRECTION_IN.
 *
 * Default is DIRECTION_OUT.
 *
 * Use the function IsComponentSide() to determine if this function is at the
 * sever's side or not.
 */
int CBEFunction::GetReceiveDirection()
{
    return DIRECTION_OUT;
}

/** \brief performs basic initializations
 *  \param pContext the context of the create call
 *  \return true if successful
 *
 * We need to create the CORBA object and environment variables.
 */
bool CBEFunction::CreateBackEnd(CBEContext *pContext)
{
    if (m_pCorbaObject)
        delete m_pCorbaObject;
    if (m_pCorbaEnv)
        delete m_pCorbaEnv;
    // init CORBA Object
    String sTypeName("CORBA_Object");
    String sName = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    m_pCorbaObject = pContext->GetClassFactory()->GetNewTypedDeclarator();
    m_pCorbaObject->SetParent(this);
    if (!m_pCorbaObject->CreateBackEnd(sTypeName, sName, 1, pContext))
    {
        delete m_pCorbaObject;
        m_pCorbaObject = 0;
        VERBOSE("%s failed, because CORBA Object could not be created\n", __PRETTY_FUNCTION__);
        return false;
    }
    // init CORBA Environment
    sTypeName = "CORBA_Environment";
    sName = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);
    m_pCorbaEnv = pContext->GetClassFactory()->GetNewTypedDeclarator();
    m_pCorbaEnv->SetParent(this);
    if (!m_pCorbaEnv->CreateBackEnd(sTypeName, sName, 1, pContext))
    {
        delete m_pCorbaEnv;
        m_pCorbaEnv = 0;
        VERBOSE("%s failed, because CORBA Environment could not be created\n", __PRETTY_FUNCTION__);
        return false;
    }
    // done
    return true;
}

/** \brief access the message buffer member
 *  \return a reference to the message buffer
 */
CBEMsgBufferType* CBEFunction::GetMessageBuffer()
{
    return m_pMsgBuffer;
}

/** \brief sets the second declarator of the typed decl to the name and stars
 *  \param pTypedDecl the parameter to alter
 *  \param sNewDeclName the new name of the second declarator
 *  \param nStars the new number of stars of the second declarator
 *  \param pContext the context of the change
 */
void CBEFunction::SetCallVariable(CBETypedDeclarator *pTypedDecl, String sNewDeclName, int nStars, CBEContext *pContext)
{
    // check if there is already a second declarator
    VectorElement *pIter = pTypedDecl->GetFirstDeclarator();
    CBEDeclarator *pSecond = pTypedDecl->GetNextDeclarator(pIter);
    pSecond = pTypedDecl->GetNextDeclarator(pIter);
    if (pSecond)
    {
        pSecond->CreateBackEnd(sNewDeclName, nStars, pContext);
    }
    else
    {
        pSecond = pContext->GetClassFactory()->GetNewDeclarator();
        pTypedDecl->AddDeclarator(pSecond);
        if (!pSecond->CreateBackEnd(sNewDeclName, nStars, pContext))
        {
            pTypedDecl->RemoveDeclarator(pSecond);
            delete pSecond;
        }
    }
}

/** \brief create a call varaiable for the original parameter
 *  \param sOriginalName the name of the original parameter
 *  \param nStars the number of indirections of the variable used in the call
 *  \param sCallName the name of the variable used in the call
 *  \param pContext the context of the matching
 *
 * When calling a function the variables which are used to match the parameters may
 * have a different number of stars. This has to be matched with the number of stars
 * used inside the function (the parameter's stars).
 *
 * Therefore we create a "shadow" parameters used for function calls, which has
 * a own name, which usually corresponds to the parameter's name, and has a different
 * number of stars. When writing the "shadow" parameter the difference between these
 * numbers of stars is used to determine whether the call varaible has to be referenced
 * or dereferenced.
 *
 * Instead of replacing the old declarator with a new one, we simply create a second
 * declarator. Thus is valid since a parameter should have one declarator only. So the
 * second becomes out calling variable name.
 */
void CBEFunction::SetCallVariable(String sOriginalName, int nStars, String sCallName, CBEContext *pContext)
{
    // check if Corba variables are meant
    if (m_pCorbaObject)
    {
        if (m_pCorbaObject->FindDeclarator(sOriginalName))
        {
            SetCallVariable(m_pCorbaObject, sCallName, nStars, pContext);
            // can stop here
            return;
        }
    }
    if (m_pCorbaEnv)
    {
        if (m_pCorbaEnv->FindDeclarator(sOriginalName))
        {
            SetCallVariable(m_pCorbaEnv, sCallName, nStars, pContext);
            // can stop here
            return;
        }
    }

    // clone existing parameters if not yet done
    if (m_vCallParameters.GetSize() == 0)
        m_vCallParameters.Add(&m_vParameters);
    // search for original name
    VectorElement *pIter = GetFirstCallParameter();
    CBETypedDeclarator *pCallParam;
    while ((pCallParam = GetNextCallParameter(pIter)) != 0)
    {
        if (pCallParam->FindDeclarator(sOriginalName))
            break;
    }
    // if we didn't find anything, then pCallParam is NULL
    if (!pCallParam)
        return;
    // new name and new stars
    SetCallVariable(pCallParam, sCallName, nStars, pContext);
}

/** \brief retrieves a pointer to the first call parameter
 *  \return a pointer to the first call parameter
 */
VectorElement* CBEFunction::GetFirstCallParameter()
{
    return m_vCallParameters.GetFirst();
}

/** \brief retrieves a reference to the next call parameter
 *  \param pIter the pointer to the next call parameter
 *  \return the reference to the next call parameter
 */
CBETypedDeclarator* CBEFunction::GetNextCallParameter(VectorElement *&pIter)
{
    if (!pIter)
        return 0;
    CBETypedDeclarator *pRet = (CBETypedDeclarator*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextCallParameter(pIter);
    return pRet;
}

/** \brief allow access to m_pReturnVar
 *  \return reference to m_pReturnVar
 */
CBETypedDeclarator* CBEFunction::GetReturnVariable()
{
    return m_pReturnVar;
}

/** \brief tries to find the parameter of the given declarator
 *  \param pDeclarator the declarator with the name of the parameter
 *  \param bCall true if this is used for a call to this function
 *  \return a reference to the parameter
 */
CBETypedDeclarator* CBEFunction::GetParameter(CBEDeclarator *pDeclarator, bool bCall)
{
    ASSERT(pDeclarator);
    CBETypedDeclarator *pParameter = FindParameter(pDeclarator->GetName(), bCall);
    // declarators should be the same, e.g.
    // the function 'f(int x, struct_t b)' with struct_t = { int x, y; },
    // will return true for a.x, because it thinks it is x
    if (pParameter)
    {
        if (pParameter->FindDeclarator(pDeclarator->GetName()) != pDeclarator)
            pParameter = 0;
    }
    if (pDeclarator->GetParent()->IsKindOf(RUNTIME_CLASS(CBEAttribute)) && !pParameter)
        pParameter = (CBETypedDeclarator *) pDeclarator->GetParent()->GetParent();
    return pParameter;
}

/** \brief access the corba-environment member
 *  \return a reference to the environment member
 */
CBETypedDeclarator* CBEFunction::GetEnvironment()
{
    return m_pCorbaEnv;
}

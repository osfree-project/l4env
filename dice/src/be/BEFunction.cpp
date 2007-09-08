/**
 * \file    dice/src/be/BEFunction.cpp
 * \brief   contains the implementation of the class CBEFunction
 *
 * \date    01/11/2002
 * \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "BEFunction.h"
#include "BEAttribute.h"
#include "BEType.h"
#include "BETypedDeclarator.h"
#include "BEDeclarator.h"
#include "BEException.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEContext.h"
#include "BEMarshaller.h"
#include "BEClass.h"
#include "BETarget.h"
#include "BEUserDefinedType.h"
#include "BECommunication.h"
#include "BEOpcodeType.h"
#include "BEReplyCodeType.h"
#include "BEMsgBuffer.h"
#include "BEStructType.h"
#include "BEUnionType.h"
#include "BEUnionCase.h"
#include "BESizes.h"
#include "Trace.h"
#include "Attribute-Type.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FETypeSpec.h"
#include "Compiler.h"
#include "Error.h"
#include "Messages.h"
#include <cassert>

CBEFunction::CBEFunction(FUNCTION_TYPE nFunctionType)
: m_Attributes(0, this),
  m_Exceptions(0, this),
  m_Parameters(0, this),
  m_CallParameters(0, this),
  m_LocalVariables(0, this)
{
    m_pClass = 0;
    m_pTarget = 0;
    m_pMsgBuffer = 0;
    m_pCorbaObject = 0;
    m_pCorbaEnv = 0;
    m_nParameterIndent = 0;
    m_bComponentSide = false;
    m_bCastMsgBufferOnCall = false;
    m_nFunctionType = nFunctionType;

    m_pComm = 0;
    m_pMarshaller = 0;
    m_pTrace = 0;
    m_bTraceOn = false;
}

CBEFunction::CBEFunction(CBEFunction* src)
	: m_Attributes(src->m_Attributes),
	m_Exceptions(src->m_Exceptions),
	m_Parameters(src->m_Parameters),
	m_CallParameters(src->m_CallParameters),
	m_LocalVariables(src->m_LocalVariables)
{
    m_pClass = src->m_pClass;
    m_pTarget = src->m_pTarget;
    m_nParameterIndent = src->m_nParameterIndent;
    m_bComponentSide = src->m_bComponentSide;
    m_bCastMsgBufferOnCall = src->m_bCastMsgBufferOnCall;
    m_nFunctionType = src->m_nFunctionType;
    m_bTraceOn = src->m_bTraceOn;

	m_Attributes.Adopt(this);
	m_Exceptions.Adopt(this);
	m_Parameters.Adopt(this);
	m_CallParameters.Adopt(this);
	m_LocalVariables.Adopt(this);

	CLONE_MEM(CBEMsgBuffer, m_pMsgBuffer);
	CLONE_MEM(CBETypedDeclarator, m_pCorbaObject);
	CLONE_MEM(CBETypedDeclarator, m_pCorbaEnv);
	CLONE_MEM(CBECommunication, m_pComm);
	CLONE_MEM(CBEMarshaller, m_pMarshaller);
	if (src->m_pTrace)
	{
		CBEClassFactory *pCF = CCompiler::GetClassFactory();
		m_pTrace = pCF->GetNewTrace();
	}
	else
		m_pTrace = 0;
}

/** \brief destructor of target class
 *
 * Do _NOT_ delete the sorted parameters, because they are only references to
 * the "normal" parameters. Thus the sorted parameters are already deleted.
 * The interface is also deleted somewhere else.
 */
CBEFunction::~CBEFunction()
{
	if (m_pMsgBuffer)
		delete m_pMsgBuffer;
	if (m_pCorbaObject)
		delete m_pCorbaObject;
	if (m_pCorbaEnv)
		delete m_pCorbaEnv;
	if (m_pComm)
		delete m_pComm;
	if (m_pMarshaller)
		delete m_pMarshaller;
	if (m_pTrace)
		delete m_pTrace;
}

/** \brief retrieves the return type of the function
 *  \return a reference to the return type
 *
 * Do not manipulate the returned reference directly!
 */
CBEType *CBEFunction::GetReturnType()
{
    CBETypedDeclarator *pReturn = GetReturnVariable();
    if (!pReturn)
        return 0;
    return pReturn->GetType();
}

/** \brief adds parameters before all other parameters
 *
 * The CORBA C mapping specifies a CORBA_object to appear as first parameter.
 */
void
CBEFunction::AddBeforeParameters()
{
    if (m_pCorbaObject)
        m_Parameters.Add(m_pCorbaObject);
}

/** \brief adds parameters after all other parameters
 *
 * The CORBA C mapping specifies a pointer to a CORBA_Environment as last
 * parameter
 */
void
CBEFunction::AddAfterParameters()
{
    if (m_pCorbaEnv)
        m_Parameters.Add(m_pCorbaEnv);
}

/** \brief writes the content of the function to the target header file
 *  \param pFile the header file to write to
 *
 * A function in a header file is usually only a function declaration, except
 * the PROGRAM_GENERATE_INLINE option is set.
 *
 * This implementation first test if the target file is open and then checks
 * the options of the compiler. Based on the last check it decides which
 * internal Write function to call.
 */
void
CBEFunction::Write(CBEHeaderFile& pFile)
{
    if (!pFile.is_open())
	return;

    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s(%s) in %s called\n", __func__,
	pFile.GetFileName().c_str(), GetName().c_str());

    // in header file write access specifier
    WriteAccessSpecifier(pFile);

    // write inline preix
    if (DoWriteFunctionInline(pFile))
    {
	string sInline = CCompiler::GetNameFactory()->GetInlinePrefix();
	pFile << "\t" << sInline << std::endl;

        WriteFunctionDefinition(pFile);
    }
    else
        WriteFunctionDeclaration(pFile);

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s returns\n", __func__);
}

/** \brief writes the content of the function to the target implementation file
 *  \param pFile the file to write to
 *
 * Only write function definition if not inlining.
 */
void CBEFunction::Write(CBEImplementationFile& pFile)
{
    if (!pFile.is_open())
        return;

    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s(%s) in %s called\n", __func__,
	pFile.GetFileName().c_str(), GetName().c_str());

    if (!DoWriteFunctionInline(pFile))
	WriteFunctionDefinition(pFile);

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s returns\n", __func__);
}

/** \brief test if this function is written inline
 *  \param pFile the file to write to
 */
bool
CBEFunction::DoWriteFunctionInline(CBEFile& /*pFile*/)
{
    return CCompiler::IsOptionSet(PROGRAM_GENERATE_INLINE);
}

/** \brief writes the declaration of a function to the target file
 *  \param pFile the target file to write to
 *
 * A function declaration looks like this:
 *
 * <code> \<return type\> \<name\>(\<parameter list\>); </code>
 *
 * The \<name\> of the function is created by the name factory
 */
void
CBEFunction::WriteFunctionDeclaration(CBEFile& pFile)
{
    if (!pFile.is_open())
        return;

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEFunction::%s(%s) in %s called\n",
	__func__, pFile.GetFileName().c_str(), GetName().c_str());

    // CPP TODOs:
    // TODO: component functions at server side could be pure virtual to be
    // overloadable
    // TODO: interface functions and component functions are public,
    // everything else should be protected

    m_nParameterIndent = pFile.GetIndent();
    pFile << "\t";
    // <return type>
    WriteReturnType(pFile);
    // in the header file we add function attributes
    WriteFunctionAttributes(pFile);
    pFile << "\n";
    // <name> (
    pFile << "\t" << m_sName << " (";
    m_nParameterIndent += m_sName.length() + 2;

    // <parameter list>
    if (!WriteParameterList(pFile))
	pFile << "void";

    // ); newline
    pFile << ");\n";

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEFunction::%s returns\n", __func__);
}

/** \brief writes the definition of the function to the target file
 *  \param pFile the target file to write to
 *
 * A function definition looks like this:
 *
 * <code> \<return type\> \<name\>(\<parameter list\>)<br>
 * {<br>
 * \<function body\><br>
 * }<br>
 * </code>
 */
void
CBEFunction::WriteFunctionDefinition(CBEFile& pFile)
{
    if (!pFile.is_open())
        return;

    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s(%s) in %s called\n", __func__,
	pFile.GetFileName().c_str(), GetName().c_str());

    m_nParameterIndent = pFile.GetIndent();
    pFile << "\t";
    // return type
    WriteReturnType(pFile);
    pFile << "\n";
    // <name>(
    pFile << "\t";
    if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
	pFile.IsOfFileType(FILETYPE_IMPLEMENTATION))
    {
	// get class and write <class>::
	CBEClass *pClass = GetSpecificParent<CBEClass>();
	if (pClass)
	{
	    pClass->WriteClassName(pFile);
	    pFile << "::";
	}
    }
    pFile << m_sName << " (";
    m_nParameterIndent += m_sName.length() + 2;

    // <parameter list>
    if (!WriteParameterList(pFile))
	pFile << "void";

    // ) newline { newline
    pFile << ")\n";
    pFile << "\t{\n";
    ++pFile;

    // writes the body
    WriteBody(pFile);

    // } newline
    --pFile << "\t}\n\n";

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEFunction::%s returns\n",
	__func__);
}

/** \brief writes the return type of a function
 *  \param pFile the file to write to
 *
 * Because the return type differs for every kind of function, we use an
 * overloadable function here.
 */
void CBEFunction::WriteReturnType(CBEFile& pFile)
{
    CBEType *pRetType = GetReturnType();
    if (!pRetType)
    {
	pFile << "void";
        return;
    }
    pRetType->Write(pFile);
}

/** \brief writes the parameter list to the target file
 *  \param pFile the file to write to
 *
 * The parameter list contains the parameters of the function,speerated by
 * commas.
 */
bool CBEFunction::WriteParameterList(CBEFile& pFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called for %s\n", __func__,
	GetName().c_str());
    int nDiffIndent = m_nParameterIndent - pFile.GetIndent();
    pFile+=nDiffIndent;
    // remember if we wrote something
    bool bComma = false;
    // write own parameters
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
	if (DoWriteParameter(*iter))
	{
	    if (bComma)
		pFile << ",\n\t";
	    WriteParameter(pFile, *iter);
	    bComma = true;
	}
    }
    // after parameter list decrement indent
    pFile-=nDiffIndent;

    return bComma;
}

/** \brief write a single parameter to the target file
 *  \param pFile the file to write to
 *  \param pParameter the parameter to write
 *  \param bUseConst true if param should be const
 *
 * This implementation gets the first declarator of the typed declarator
 * and writes its names plus type.
 */
void
CBEFunction::WriteParameter(CBEFile& pFile,
    CBETypedDeclarator * pParameter,
    bool bUseConst)
{
    CBEDeclarator *pDeclarator = pParameter->m_Declarators.First();
    assert(pDeclarator);
    pParameter->WriteType(pFile, bUseConst);
    pFile << " ";
    pDeclarator->WriteDeclaration(pFile);
}

/** \brief writes the body of the function to the target file
 *  \param pFile the file to write to
 */
void CBEFunction::WriteBody(CBEFile& pFile)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s(%s) in %s called\n", __func__,
	pFile.GetFileName().c_str(), GetName().c_str());

    // variable declaration and initialization
    WriteVariableDeclaration(pFile);
    WriteVariableInitialization(pFile);
    // prepare message
    WriteMarshalling(pFile);
    // invoke message transfer
    WriteInvocation(pFile);
    // unmarshal response
    WriteUnmarshalling(pFile);
    // clean up and return
    WriteCleanup(pFile);
    WriteReturn(pFile);

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEFunction::%s returns\n",
	__func__);
}

/** \brief writes the declaration of the variables
 *  \param pFile the file to write to
 *
 * Write declaration of local variables
 */
void
CBEFunction::WriteVariableDeclaration(CBEFile& pFile)
{
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_LocalVariables.begin();
	 iter != m_LocalVariables.end();
	 iter++)
    {
        (*iter)->WriteInitDeclaration(pFile, string());
    }

    if (m_pTrace)
	m_pTrace->VariableDeclaration(pFile, this);
}

/** \brief writes the preparation of the message transfer
 *  \param pFile the file to write to
 *
 * This function creates a marshaller and lets it marshal this function.
 */
void
CBEFunction::WriteMarshalling(CBEFile& pFile)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEFunction::%s(%s) called\n",
	__func__, GetName().c_str());

    bool bLocalTrace = false;
    if (!m_bTraceOn && m_pTrace)
    {
	m_pTrace->BeforeMarshalling(pFile, this);
	m_bTraceOn = bLocalTrace = true;
    }

    assert(m_pMarshaller);
    m_pMarshaller->MarshalFunction(pFile, GetSendDirection());

    if (bLocalTrace)
    {
	m_pTrace->AfterMarshalling(pFile, this);
	m_bTraceOn = false;
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s(%s) finished\n", __func__, GetName().c_str());
}

/** \brief writes the clean up of the function
 *  \param pFile the file to write to
 *
 * \todo check if this belongs to CBEOperationFunction
 */
void CBEFunction::WriteCleanup(CBEFile& /*pFile*/)
{}

/** \brief writes the return statement of the function
 *  \param pFile the file to write to
 *
 * This function writes the return statement using the return variable.
 * Set this member respectively or overload this function if you wish to
 * obtain different results.
 *
 * If the return type is void, an empty return statement is written.
 * We do this to allow usage of WriteReturn conditionally.
 */
void CBEFunction::WriteReturn(CBEFile& pFile)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEFunction::%s(%s) called\n",
	__func__, GetName().c_str());

    CBETypedDeclarator *pReturn = GetReturnVariable();
    if (!pReturn || GetReturnType()->IsVoid())
    {
	pFile << "\treturn;\n";
        CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEFunction::%s(%s) finished\n", __func__, GetName().c_str());
        return;
    }

    pFile << "\treturn ";
    // get return var name, which is first declarator
    CBEDeclarator *pRetVar = pReturn->m_Declarators.First();
    pRetVar->WriteDeclaration(pFile);
    pFile << ";\n";

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s(%s) finished\n", __func__, GetName().c_str());
}

/** \brief writes the unmarshlling of the message buffer
 *  \param pFile the file to write to
 *
 * This function creates a marshaller and lets it unmarshal this function
 */
void
CBEFunction::WriteUnmarshalling(CBEFile& pFile)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEFunction::%s(%s) called\n",
	__func__, GetName().c_str());

    bool bLocalTrace = false;
    if (!m_bTraceOn && m_pTrace)
    {
	m_pTrace->BeforeUnmarshalling(pFile, this);
	m_bTraceOn = bLocalTrace = true;
    }

    assert(m_pMarshaller);
    m_pMarshaller->MarshalFunction(pFile, GetReceiveDirection());

    if (bLocalTrace)
    {
	m_pTrace->AfterUnmarshalling(pFile, this);
	m_bTraceOn = false;
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s(%s) finished\n", __func__, GetName().c_str());
}

/** \brief writes a function call to this function
 *  \param pFile the file to write to
 *  \param sReturnVar the name of the variable, which will be assigned the \
 *         return value
 *  \param bCallFromSameClass true if the call is made from the same class
 *
 * We always assume that a function call uses the same parameter names as the
 * function declaration.  This makes it simple to write a function call
 * without actually knowing the variable names. The return variable's name is
 * given to us. If it is 0 the caller wants no return value.
 */
void
CBEFunction::WriteCall(CBEFile& pFile,
	string sReturnVar,
	bool bCallFromSameClass)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEFunction::%s(%s) called\n",
	__func__, GetName().c_str());

    pFile << "\t";
    m_nParameterIndent = 0;
    CBEType *pRetType = GetReturnType();
    if (pRetType)
    {
        if ((!sReturnVar.empty()) && (!pRetType->IsVoid()))
        {
	    pFile << sReturnVar << " = ";
            m_nParameterIndent += sReturnVar.length() + 3;
        }
    }

    pFile << GetName() << " (";
    m_nParameterIndent += GetName().length() + 2;
    pFile+=m_nParameterIndent;
    WriteCallParameterList(pFile, bCallFromSameClass);
    pFile-=m_nParameterIndent;
    pFile << ");\n";

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s(%s) finished\n", __func__, GetName().c_str());
}

/** \brief writes the parameter list for a function call
 *  \param pFile the file to write to
 *  \param bCallFromSameClass true if call is made from same class
 */
void
CBEFunction::WriteCallParameterList(CBEFile& pFile,
    bool bCallFromSameClass)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called for %s\n", __func__,
	GetName().c_str());
    bool bComma = false;
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_CallParameters.begin();
	 iter != m_CallParameters.end();
	 iter++)
    {
	// find original param for call param
	CBETypedDeclarator *pOrig =
	    m_Parameters.Find((*iter)->m_Declarators.First()->GetName());
	if (DoWriteParameter(pOrig))
	{
	    if (bComma)
		pFile << ",\n\t";
	    WriteCallParameter(pFile, *iter, bCallFromSameClass);
	    bComma = true;
	}
    }
}

/** \brief writes a single parameter for the function call
 *  \param pFile the file to write to
 *  \param pParameter the parameter to be written
 *  \param bCallFromSameClass true if called from same class
 */
void
CBEFunction::WriteCallParameter(CBEFile& pFile,
	CBETypedDeclarator * pParameter,
	bool /*bCallFromSameClass*/)
{
    CBEDeclarator *pOriginalDecl = pParameter->m_Declarators.First();
    CBEDeclarator *pCallDecl = pParameter->GetCallDeclarator();
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBEFunction::%s: orig @ %p, call q %p\n",
	__func__, pOriginalDecl, pCallDecl);
    if (!pCallDecl)
        pCallDecl = pOriginalDecl;
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBEFunction::%s: write parameter %s\n", __func__,
	pOriginalDecl->GetName().c_str());
    WriteCallParameterName(pFile, pOriginalDecl, pCallDecl);
}

/** \brief writes the name of a parameter
 *  \param pFile the file to write to
 *  \param pInternalDecl the internal parameter
 *  \param pExternalDecl the calling parameter
 *
 * Because the common Write function of CBEDeclarator writes a parameter
 * definition we need something else to write the parameter for a function
 * call. This is simply the name of the parameter.
 *
 * To know wheter  we have to reference or dereference the parameter, we
 * compare the number of stars between the original (internal) parameter and
 * the call (external) parameter. If:
 * # the stars of external > stars of internal we need dereferencing:
 *   '*' x (stars external - stars internal)
 * # the stars of external < stars of internal we need referencing:
 *   '&' x -(stars external - stars internal)
 * # the stars of external == stars of internal we need nothing.
 */
void
CBEFunction::WriteCallParameterName(CBEFile& pFile,
	CBEDeclarator * pInternalDecl,
	CBEDeclarator * pExternalDecl)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEFunction::%s called\n", __func__);
    // check stars
    int nDiffStars = pExternalDecl->GetStars() - pInternalDecl->GetStars();
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEFunction::%s nDiffStars = %d = %d - %d (1)\n", __func__, nDiffStars,
	pExternalDecl->GetStars(), pInternalDecl->GetStars());
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEFunction::%s nDiffStars = %d (2)\n", __func__, nDiffStars);

    int nCount;
    for (nCount = nDiffStars; nCount < 0; nCount++)
    {
	pFile << "&";
        if (nCount+1 < 0)
	    pFile << "(";
    }
    for (nCount = nDiffStars; nCount > 0; nCount--)
	pFile << "*";
    if (nDiffStars > 0)
	pFile << "(";
    pFile << pExternalDecl->GetName();
    if (nDiffStars > 0)
	pFile << ")";
    for (nCount = nDiffStars+1; nCount < 0; nCount++)
	pFile << ")";

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBEFunction::%s returns\n", __func__);
}

/** \brief counts the number of string parameters
 *  \param nDirection the direction to count
 *  \param nMustAttrs the attributes which have to be set for the parameters
 *  \param nMustNotAttrs the attributes which must not be set for the parameters
 *  \return the number of string parameters
 *
 * A string is every parameter, which's function IsString returns true.
 */
int
CBEFunction::GetStringParameterCount(
    DIRECTION_TYPE nDirection,
    ATTR_TYPE nMustAttrs,
    ATTR_TYPE nMustNotAttrs)
{
    int nCount = 0;
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
        // if explicitely ATTR_IN (means only in) and dir is not in or
        // if explicitely ATTR_OUT (means only out) and dir is not out
        // then skip
        if (!(*iter)->IsDirection(nDirection))
            continue;
        // test for attributes that it must have (if it hasn't, skip count)
        bool bCount = true;
        if (!(*iter)->m_Attributes.Find(nMustAttrs))
            bCount = false;
        // test for attributes that it must NOT have (skip count)
        if ((*iter)->m_Attributes.Find(nMustNotAttrs) != 0)
            bCount = false;
        // count if string
        if ((*iter)->IsString() && bCount)
            nCount++;
    }
    CBETypedDeclarator *pRet = GetReturnVariable();
    if (pRet)
    {
        if (pRet->IsDirection(nDirection))
        {
            bool bCount = true;
            if (!pRet->m_Attributes.Find(nMustAttrs))
                bCount = false;
            if (pRet->m_Attributes.Find(nMustNotAttrs))
                bCount = false;
            if (pRet->IsString() && bCount)
                nCount++;
        }
    }
    return nCount;
}

/** \brief counts number of variable sized parameters
 *  \param nDirection the direction to count
 *  \return the number of variable sized parameters
 */
int CBEFunction::GetVariableSizedParameterCount(DIRECTION_TYPE nDirection)
{
    int nCount = 0;
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
         // if explicitely ATTR_IN (means only in) and dir is not in or
         // if explicitely ATTR_OUT (means only out) and dir is not out
         // then skip
         if (!(*iter)->IsDirection(nDirection))
             continue;
         if ((*iter)->IsVariableSized())
             nCount++;
    }
    CBETypedDeclarator *pRet = GetReturnVariable();
    if (pRet &&
        pRet->IsDirection(nDirection) &&
        pRet->IsVariableSized())
        nCount++;
    return nCount;
}

/** \brief calculates the total size of the parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * The size of the function's parameters is the sum of the parameter's sizes.
 * The size of a parameter is calculcated by CBETypedDeclarator::GetSize().
 *
 * \todo If member is indirect, we should add size of size-attr instead of
 * size of type
 */
int CBEFunction::GetSize(DIRECTION_TYPE nDirection)
{
    CBETypedDeclarator *pMsgBuf = GetMessageBuffer();
    string sMsgBuf;
    if (pMsgBuf && pMsgBuf->m_Declarators.First())
        sMsgBuf = pMsgBuf->m_Declarators.First()->GetName();
    int nSize = 0;
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
        // if direction is in and no ATTR_IN  or
        // if direction is out and no ATTR_OUT
        // then skip
        if (!(*iter)->IsDirection(nDirection))
            continue;
        // if attribute IGNORE skip
        if ((*iter)->m_Attributes.Find(ATTR_IGNORE))
            continue;
	// a function cannot have bitfield parameters, so we don't need to
	// test for those if size is negative (param is variable sized), then
	// add parameters base type
	// BTW: do not count message buffer parameter
        if ((*iter)->m_Declarators.Find(sMsgBuf))
            continue;

        if ((*iter)->IsVariableSized())
        {
            CBEAttribute *pAttr = (*iter)->m_Attributes.Find(ATTR_SIZE_IS);
            if (!pAttr)
                pAttr = (*iter)->m_Attributes.Find(ATTR_LENGTH_IS);
            if (!pAttr)
                pAttr = (*iter)->m_Attributes.Find(ATTR_MAX_IS);
            if (pAttr && pAttr->IsOfType(ATTR_CLASS_INT))
            {
                // check alignment
                int nAttrSize = pAttr->GetIntValue();
                nSize += nAttrSize + GetParameterAlignment(nSize, nAttrSize);
            }
            else
            {
                CBEType *pType = (*iter)->GetType();
                if ((pAttr = (*iter)->m_Attributes.Find(ATTR_TRANSMIT_AS)) != 0)
                    pType = pAttr->GetAttrType();
                // check alignment
                int nTypeSize = pType->GetSize();
                nSize += nTypeSize + GetParameterAlignment(nSize, nTypeSize);
            }
        }
        else
        {
            // GetSize checks for referenced out and struct
            // but not for simple pointered vars, which
            // should be dereferenced for transmition
            int nParamSize = (*iter)->GetSize();
            if (nParamSize < 0)
            {
                // pointer (without size attributes!)
                CBEType *pType = (*iter)->GetType();
                CBEAttribute *pAttr;
                if ((pAttr = (*iter)->m_Attributes.Find(ATTR_TRANSMIT_AS)) != 0)
                    pType = pAttr->GetAttrType();
                // check alignment
                int nTypeSize = pType->GetSize();
                nSize += nTypeSize + GetParameterAlignment(nSize , nTypeSize);
            }
            else
            {
                // check alignment
                nSize += nParamSize + GetParameterAlignment(nSize, nParamSize);
            }
        }
    }
    // add return var's size
    int nTypeSize = GetReturnSize(nDirection);
    nSize += nTypeSize + GetParameterAlignment(nSize, nTypeSize);

    return nSize;
}

/** \brief returns the fixed size of the return variable
 *  \param nDirection the direction to count the return variable for
 *  \return the fixed size of the return variable in bytes
 *
 * We put this into an extra function, because with some functions the return
 * variable is NOT a parameter to transferred. Which means that some functions
 * will return 0 even though they have an return value.
 */
int CBEFunction::GetReturnSize(DIRECTION_TYPE nDirection)
{
    CBETypedDeclarator *pReturn = GetReturnVariable();
    if (pReturn && pReturn->IsDirection(nDirection))
    {
        if (pReturn->IsVariableSized())
            return pReturn->GetType()->GetSize();

	int nParamSize = pReturn->GetSize();
	if (nParamSize < 0)
	    return pReturn->GetType()->GetSize();

	return nParamSize;
    }
    return 0;
}

/** \brief calculates the maximum size of a function's message buffer for a \
 *         given direction
 *  \param nDirection the direction to count
 *  \return return size of function
 *
 * The maximum size also count variable sized parameters, which have a max
 * attribute or boundary.
 *
 * \todo: issue warning if no max size available
 */
int CBEFunction::GetMaxSize(DIRECTION_TYPE nDirection)
{
    // get msg buffer's name
    string sMsgBuf;
    CBETypedDeclarator *pMsgBuf = GetMessageBuffer();
    if (pMsgBuf && pMsgBuf->m_Declarators.First())
        sMsgBuf = pMsgBuf->m_Declarators.First()->GetName();
    int nSize = 0;
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
        // if direction is in and no ATTR_IN  or
        // if direction is out and no ATTR_OUT
        // then skip
        if (!(*iter)->IsDirection(nDirection))
            continue;
	// a function cannot have bitfield parameters, so we don't need to
	// test for those if size is negative (param is variable sized), then
	// add parameters base type
        // BTW: do not count message buffer parameter
        if ((*iter)->m_Declarators.Find(sMsgBuf))
            continue;

	CBESizes *pSizes = CCompiler::GetSizes();
        // GetMaxSize already checks for variable sized params and
        // tries to find their MAX values. If there is no way to determine
        // them, it returns false.
        int nParamSize = 0;
        if (!(*iter)->GetMaxSize(nParamSize))
        {
            CBEType *pType = (*iter)->GetType();
            int nTypeSize = pSizes->GetMaxSizeOfType(pType->GetFEType());
            if (CCompiler::IsWarningSet(PROGRAM_WARNING_NO_MAXSIZE))
            {
                CBEDeclarator *pD = (*iter)->m_Declarators.First();
                CMessages::Warning("%s in %s has no maximum size (guessing %d)",
                    pD->GetName().c_str(), GetName().c_str(),
		    nTypeSize * -nParamSize);
            }
	    nParamSize = nTypeSize;
        }
	// check alignment
	nSize += nParamSize + GetParameterAlignment(nSize, nParamSize);
    }
    // add return var's size
    int nTypeSize = GetMaxReturnSize(nDirection);
    nSize += nTypeSize + GetParameterAlignment(nSize, nTypeSize);

    return nSize;
}

/** \brief returns the maximum size of the return variable
 *  \param nDirection the direction to count the return variable for
 *  \return the maximum size of the return variable in bytes
 *
 * We put this into an extra function, because with some functions the return
 * variable is NOT a parameter to transferred. Which means that some functions
 * will return 0 even though they have an return value.
 */
int CBEFunction::GetMaxReturnSize(DIRECTION_TYPE nDirection)
{
    CBETypedDeclarator *pReturn = GetReturnVariable();
    if (pReturn && pReturn->IsDirection(nDirection))
    {
        int nParamSize = 0;
        pReturn->GetMaxSize(nParamSize);
        if (nParamSize < 0) // cannot issue warning, since return type cannot have attributes defined
            return CCompiler::GetSizes()->GetMaxSizeOfType(pReturn->GetType()->GetFEType());
        else
            return nParamSize;
    }
    return 0;
}

/** \brief count the size of all fixed sized parameters
 *  \param nDirection the direction to count
 *  \return the size in bytes of all fixed sized parameters
 *
 * The difference between this function and the normal GetSize function is,
 * that the above function counts array, which have a fixed upper bound, but
 * also a size_is or length_is parameter, which makes them effectively
 * variable sized parameters.
 */
int CBEFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
    CBETypedDeclarator *pMsgBuf = GetMessageBuffer();
    string sMsgBuf;
    if (pMsgBuf && pMsgBuf->m_Declarators.First())
        sMsgBuf = pMsgBuf->m_Declarators.First()->GetName();
    int nSize = 0;
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
        // if direction is in and no ATTR_IN  or
        // if direction is out and no ATTR_OUT
        // then skip
        if (!(*iter)->IsDirection(nDirection))
            continue;
        // skip parameters to ignore
        if ((*iter)->m_Attributes.Find(ATTR_IGNORE))
            continue;

        // BTW: do not count message buffer parameter
        if ((*iter)->m_Declarators.Find(sMsgBuf))
            continue;
        // function cannot have bitfield params, so we dont test for them
        // if param is not variable sized, then add its size

        // only count fixed sized or with attribute max_is
        // var-size can be from size_is attribute
	int nParamSize = 0;
        if ((*iter)->IsFixedSized())
        {
            // pParam->GetSize also tests for referenced OUT
            // and referenced structs and delivers correct size
            // BUT: it does not check for pointered vars, which
            // should be dereferenced

            nParamSize = (*iter)->GetSize();
            if (nParamSize < 0)
		nParamSize = (*iter)->GetTransmitType()->GetSize();
        }
        else
        {
            // for the L4 backend and [ref, size_is(), max_is()] is NOT fixed,
            // since it is transmitted using indirect strings, not the fixed
            // data array. Therefore, CL4BETypedDeclarator has to overload
            // GetMaxSize to avoid this case.

            // an array with size_is attribute is still fixed in size
            (*iter)->GetMaxSize(nParamSize);
        }
	// check alignment
	nSize += nParamSize + GetParameterAlignment(nSize, nParamSize);
    }
    // add return var's size
    // check alignment
    int nTypeSize = GetFixedReturnSize(nDirection);
    nSize += nTypeSize + GetParameterAlignment(nSize, nTypeSize);

    return nSize;
}

/** \brief returns the size of the return variable
 *  \param nDirection the direction to count the return variable for
 *  \return the size of the return variable in bytes
 *
 * We put this into an extra function, because with some functions the return
 * variable is NOT a parameter to transferred. Which means that some functions
 * will return 0 even though they have an return value.
 */
int CBEFunction::GetFixedReturnSize(DIRECTION_TYPE nDirection)
{
    CBETypedDeclarator *pReturn = GetReturnVariable();
    if (pReturn &&
        pReturn->IsDirection(nDirection) &&
        pReturn->IsFixedSized())
    {
        int nParamSize = pReturn->GetSize();
        if (nParamSize < 0)
            return pReturn->GetType()->GetSize();
        else
            return nParamSize;
    }
    return 0;
}

/** \brief adds a reference to the message buffer to the parameter list
 *  \return true if successful
 *
 * We create a new message buffer for an interface. This implementation
 * is usually called by CBEWaitAnyFunction, CBESrvLoopFunction, ???
 */
void
CBEFunction::AddMessageBuffer()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);

    if (m_pMsgBuffer)
	delete m_pMsgBuffer;
    m_pMsgBuffer = CCompiler::GetClassFactory()->GetNewMessageBuffer();
    m_pMsgBuffer->SetParent(this);
    // get class' message buffer type
    CBEClass *pClass = GetSpecificParent<CBEClass>();
    assert(pClass);
    CBEMsgBuffer *pClassMsgBuf = pClass->GetMessageBuffer();
    assert(pClassMsgBuf);
    string sTypeName = pClassMsgBuf->m_Declarators.First()->GetName();
    // get local variable name
    string sName =
	CCompiler::GetNameFactory()->GetMessageBufferVariable();
    // add as user defined type
    m_pMsgBuffer->CreateBackEnd(sTypeName, sName, 1);
    // initialisation
    // NO platform-specific members added
    // NO sorting
    // NO post-create
    // (all done in msgbuffers used to create class' message buffer)
    MsgBufferInitialization(m_pMsgBuffer);

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

/** \brief adds a message buffer parameter for a specific function
 *  \param pFEOperation the reference front-end operation
 *  \return true if successful
 *
 * This method does the following steps
 * -# create the message buffer
 * -# call CBEMsgBuffer::CreateBackEnd
 * -# call CBEMsgBuffer::AddPlatformSpecificMembers(this)
 * -# call CBEFunction::MsgBufferInitialization(m_pMsgBuffer)
 * -# call CBEMsgBuffer::Sort(this)
 * -# call CBEMsgBuffer::PostCreate(this)
 *
 * - AddPlatformSpecificMembers adds opcode, exception, etc.
 * - MsgBufferInitialization adds return code, sets pointer of msgbuf, etc.
 * - PostCreate can create additional structs or add alignment, padding
 */
void
CBEFunction::AddMessageBuffer(CFEOperation *pFEOperation)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);

    string exc = string(__func__);
    if (m_pMsgBuffer)
	delete m_pMsgBuffer;
    m_pMsgBuffer = CCompiler::GetClassFactory()->GetNewMessageBuffer();
    m_pMsgBuffer->SetParent(this);
    m_pMsgBuffer->CreateBackEnd(pFEOperation);

    // add platform specific members
    m_pMsgBuffer->AddPlatformSpecificMembers(this);
    // function specific initialization
    MsgBufferInitialization(m_pMsgBuffer);

    // sort message buffer
    if (!m_pMsgBuffer->Sort(this))
    {
	exc += " failed, because Sort failed.";
	throw new error::create_error(exc);
    }

    // post create stuff
    m_pMsgBuffer->PostCreate(this, pFEOperation);

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns\n", __func__);
}

/** \brief manipulates the message buffer
 *  \param pMsgBuffer the message buffer to manipulate
 *  \return true on success
 */
void
CBEFunction::MsgBufferInitialization(CBEMsgBuffer*)
{ }

/** \brief marshals the return value
 *  \param pFile the file to write to
 *  \param bMarshal true if marshaling, false if unmarshaling
 *  \return the number of bytes used for the return value
 *
 * This function assumes that it is called before the marshalling of the other
 * parameters.
 */
int
CBEFunction::WriteMarshalReturn(CBEFile& pFile,
    bool bMarshal)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__, GetName().c_str());

    CBETypedDeclarator *pReturn = GetReturnVariable();
    if (!pReturn)
        return 0;
    CBEType *pType = GetReturnType();
    if (pType->IsVoid())
        return 0;
    CBEMarshaller *pMarshaller = GetMarshaller();
    pMarshaller->MarshalParameter(pFile, this, pReturn, bMarshal);

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s returns\n", __func__, GetName().c_str());
    return pType->GetSize();
}

/** \brief marshals the opcode
 *  \param pFile the file to marshal to
 *  \param bMarshal true if marshalling, false if unmarshalling
 *  \return the number of bates used for the opcode
 *
 * This function assumes that there exists a local variable for the
 * unmarshalling.
 */
int
CBEFunction::WriteMarshalOpcode(CBEFile& pFile,
    bool bMarshal)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__, GetName().c_str());

    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sOpcode = pNF->GetOpcodeVariable();
    CBETypedDeclarator *pOpcode = m_LocalVariables.Find(sOpcode);
    if (!pOpcode)
	return 0;
    CBEMarshaller *pMarshaller = GetMarshaller();
    pMarshaller->MarshalParameter(pFile, this, pOpcode, bMarshal);
    int nSize = pOpcode->GetType()->GetSize();

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s returns %d\n", __func__, GetName().c_str(), nSize);
    return nSize;
}

/** \brief marshals the exception
 *  \param pFile the file to write to
 *  \param bMarshal true if marshalling, false if unmarshalling
 *  \param bReturn true if we should write a return statement
 *
 * The exception is represented by the first two members of the
 * CORBA_Environment, major and repos_id, which are (together) of type
 * CORBA_exceptio_type, which is an int. If the major value is
 * CORBA_NO_EXCEPTION, then there was no exception and the normal parameters
 * follow. If it is CORBA_SYSTEM_EXCEPTION, then no more parameters will
 * follow.  If it is CORBA_USER_EXCEPTION there might be one exception
 * parameter. This is only the case when we allow typed exceptions, which we
 * skip for now.
 */
void
CBEFunction::WriteMarshalException(CBEFile& pFile,
    bool bMarshal,
    bool bReturn)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__,
	GetName().c_str());

    CBETypedDeclarator *pExceptionVar = GetExceptionVariable();
    if (!pExceptionVar)
	return;
    CBEMarshaller *pMarshaller = GetMarshaller();
    pMarshaller->MarshalParameter(pFile, this, pExceptionVar, bMarshal);

    if (bReturn)
    {
	CBEDeclarator *pEnv = GetEnvironment()->m_Declarators.First();
	string sName;
	if (pEnv->GetStars() == 0)
	    sName = "&";
	sName += pEnv->GetName();
	pFile << "\tif (DICE_HAS_EXCEPTION(" << sName << "))\n";
	++pFile;
	WriteReturn(pFile);
	--pFile;
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s finished\n", __func__,
	GetName().c_str());
}

/** \brief initializes and sets the return var to a new value
 *  \param bUnsigned if the new return type should be unsigned
 *  \param nSize the size of the type
 *  \param nFEType the front-end type number
 *  \param sName the new name of the return variable
 *  \return true if successful
 *
 * The type and the name should not be initialized yet. This is all done by
 * this function.
 */
bool
CBEFunction::SetReturnVar(bool bUnsigned, int nSize, int nFEType,
    string sName)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
    // recycle old var
    CBETypedDeclarator *pReturn = GetReturnVariable();

    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEType *pType = pCF->GetNewType(nFEType);
    // set type
    if (!pReturn)
    {
	pReturn = pCF->GetNewTypedDeclarator();
	AddLocalVariable(pReturn);
	pType->SetParent(pReturn);
	pType->CreateBackEnd(bUnsigned, nSize, nFEType);
	pReturn->CreateBackEnd(pType, sName);
	delete pType; // cloned in CBETypedDeclarator::CreateBackEnd
    }
    else
    {
	pType->SetParent(pReturn);
	pType->CreateBackEnd(bUnsigned, nSize, nFEType);
	pReturn->ReplaceType(pType);

	// set name
	CBEDeclarator *pDecl = pReturn->m_Declarators.First();
	pDecl->CreateBackEnd(sName, pDecl->GetStars());
    }

    SetReturnVarAttributes(pReturn);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns\n", __func__);
    return true;
}

/** \brief initializes and sets the return var to a new value
 *  \param pType the new type of the return variable
 *  \param sName the new name of the return variable
 *  \return true if successful
 *
 * The type and the name should not be initialized yet. This is all done by
 * this function.
 */
bool
CBEFunction::SetReturnVar(CBEType * pType,
	string sName)
{
    if (!pType)
        return false;
    // delete old
    CBETypedDeclarator *pReturn = GetReturnVariable();
    m_LocalVariables.Remove(pReturn);

    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    pReturn = pCF->GetNewTypedDeclarator();

    pType->SetParent(pReturn);
    AddLocalVariable(pReturn);
    if (dynamic_cast<CBEOpcodeType*>(pType))
	((CBEOpcodeType*)pType)->CreateBackEnd();
    if (dynamic_cast<CBEReplyCodeType*>(pType))
	((CBEReplyCodeType*)pType)->CreateBackEnd();
    pReturn->CreateBackEnd(pType, sName);
    delete pType;        // is cloned by typed decl

    SetReturnVarAttributes(pReturn);
    return true;
}

/** \brief initializes and sets the return var to a new value
 *  \param pFEType the front-end type to use as reference for the new type
 *  \param sName the name of the variable
 *  \return true if successful
 */
bool
CBEFunction::SetReturnVar(CFETypeSpec * pFEType,
	string sName)
{
    if (!pFEType)
        return false;
    // delete old
    CBETypedDeclarator *pReturn = GetReturnVariable();
    m_LocalVariables.Remove(pReturn);

    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEType *pType = pCF->GetNewType(pFEType->GetType());
    pReturn = pCF->GetNewTypedDeclarator();

    pType->SetParent(pReturn);
    AddLocalVariable(pReturn);
    pType->CreateBackEnd(pFEType);
    pReturn->CreateBackEnd(pType, sName);
    delete pType;        // cloned by typed declarator

    SetReturnVarAttributes(pReturn);
    return true;
}

/** \brief set some attributes of the return variable
 *  \param pReturn the return variable
 */
void
CBEFunction::SetReturnVarAttributes(CBETypedDeclarator *pReturn)
{
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    // FIXME should the attributes really be ignored if creation fails?
    CBEAttribute *pAttr = pCF->GetNewAttribute();
    // add OUT attribute
    pReturn->m_Attributes.Add(pAttr);
    pAttr->CreateBackEnd(ATTR_OUT);
    // apply type-specific attributes to return type
    if ((pAttr = m_Attributes.Find(ATTR_STRING)) != 0)
    {
	m_Attributes.Remove(pAttr);
	pReturn->m_Attributes.Add(pAttr);
    }
    if ((pAttr = m_Attributes.Find(ATTR_SIZE_IS)) != 0)
    {
	m_Attributes.Remove(pAttr);
	pReturn->m_Attributes.Add(pAttr);
    }
    if ((pAttr = m_Attributes.Find(ATTR_LENGTH_IS)) != 0)
    {
	m_Attributes.Remove(pAttr);
	pReturn->m_Attributes.Add(pAttr);
    }
    // test return type for string
    CBEType *pType = pReturn->GetType();
    if (pType->IsOfType(TYPE_CHAR_ASTERISK) &&
	!pReturn->m_Attributes.Find(ATTR_STRING) &&
	!pReturn->m_Attributes.Find(ATTR_SIZE_IS) &&
	!pReturn->m_Attributes.Find(ATTR_LENGTH_IS))
    {
	pAttr = pCF->GetNewAttribute();
	pReturn->m_Attributes.Add(pAttr);
	pAttr->CreateBackEnd(ATTR_STRING);
    }
}

/** \brief searches for a parameter with the given name
 *  \param sName the name of the parameter
 *  \param bCall true if an call parameter is searched
 *  \return a reference to the parameter or 0 if not found
 */
CBETypedDeclarator*
CBEFunction::FindParameter(string sName, bool bCall)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s(%s, %s) called\n", __func__,
	sName.c_str(), bCall ? "true" : "false");

    CBETypedDeclarator *pParameter;
    if (bCall)
	pParameter = m_CallParameters.Find(sName);
    else
	pParameter = m_Parameters.Find(sName);
    if (pParameter)
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEFunction::%s found, returns\n", __func__);
	return pParameter;
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s returns NULL\n", __func__);
    return 0;
}

/** \brief searches for a parameter with the given name
 *  \param stack the declarator stack of the member of this parameter
 *  \param bCall true if an call parameter is searched
 *  \return a reference to the parameter or 0 if not found
 */
CBETypedDeclarator*
CBEFunction::FindParameter(CDeclStack* pStack,
    bool bCall)
{
    if (!pStack || pStack->empty())
	return 0;

    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s(stack, %s) called in %s\n", __func__,
	bCall ? "true" : "false", GetName().c_str());

    // find parameter
    CBETypedDeclarator *pParameter =
	FindParameter(pStack->front().pDeclarator->GetName(), bCall);
    if (!pParameter)
    {
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEFunction::%s no param, returns 0\n", __func__);
	return 0;
    }

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEFunction::%s calling FindParameterMember(%s,,stack)\n",
	__func__, pParameter->m_Declarators.First()->GetName().c_str());

    return FindParameterMember(pParameter, pStack->begin() + 1, pStack);
}

/** \brief searches for member in a constructed type
 *  \param pParameter the parameter to test
 *  \param iter the iterator pointing to the next element in the stack
 *  \param stack the stack with the declarators
 *  \return the member of the constructed parameter
 */
CBETypedDeclarator*
CBEFunction::FindParameterMember(CBETypedDeclarator *pParameter,
    CDeclStack::iterator iter,
    CDeclStack* pStack)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s(%p (%s)) called\n", __func__,
	pParameter,
	pParameter ? pParameter->m_Declarators.First()->GetName().c_str() : "");
    // if no parameter, return
    if (!pParameter)
	return 0;
    // if at end of stack, pParameter is what we are looking for
    assert(pStack);
    if (iter == pStack->end())
	return pParameter;

    CBEType *pType = pParameter->GetType();
    // check for user defined
    CBEUserDefinedType *pUser = dynamic_cast<CBEUserDefinedType*>(pType);
    if (pUser)
	pType = pUser->GetRealType();
    // check if struct
    CBEStructType *pStruct = dynamic_cast<CBEStructType*>(pType);
    if (pStruct)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: type of parameter is struct\n",
	    __func__);
	CBETypedDeclarator *pMember =
	    pStruct->m_Members.Find(iter->pDeclarator->GetName());
	return FindParameterMember(pMember, iter + 1, pStack);
    }
    // check if union
    CBEUnionType *pUnion = dynamic_cast<CBEUnionType*>(pType);
    if (pUnion)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: type of parameter is union\n",
	    __func__);
	CBETypedDeclarator *pMember =
	    pUnion->m_UnionCases.Find(iter->pDeclarator->GetName());
	return FindParameterMember(pMember, iter + 1, pStack);
    }
    // no constructed type, and stack not empty, something is wrong
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: no constructed type: return 0\n",
	__func__);
    return 0;
}

/** \brief simply tests if this parameter should be marshalled
 *  \param pParameter the parameter to test
 *  \param bMarshal true if marshaling, false if unmarshaling
 *  \return true if this parameter should be marshalled
 *
 * By default we marshal all parameters, so simply return true.
 */
bool
CBEFunction::DoMarshalParameter(CBETypedDeclarator *pParameter,
	bool /* bMarshal */)
{
    if (pParameter == m_pCorbaObject)
	return false;
    if (pParameter == m_pCorbaEnv)
	return false;
    return true;
}

/** \brief checks if this function has variable sized parameters
 *  \return true if it has
 *
 * Parameters are variable sized if they are variable sized arrays or strings
 * (meaning are associated with a string attribute).
 */
bool
CBEFunction::HasVariableSizedParameters(DIRECTION_TYPE nDirection)
{
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
        if (!(*iter)->IsDirection(nDirection))
             continue;
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	    "%s checking %s (var: %s, str: %s)\n", __func__,
	    (*iter)->m_Declarators.First()->GetName().c_str(),
	    ((*iter)->IsVariableSized()) ? "yes" : "no",
	    ((*iter)->IsString()) ? "yes" : "no");
        if ((*iter)->IsVariableSized() || (*iter)->IsString())
            return true;
    }
    CBETypedDeclarator *pRet = GetReturnVariable();
    if (pRet && pRet->IsDirection(nDirection))
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	    "%s checking %s (var: %s, str: %s)\n", __func__,
	    pRet->m_Declarators.First()->GetName().c_str(),
	    (pRet->IsVariableSized()) ? "yes" : "no",
	    (pRet->IsString()) ? "yes" : "no");
        if (pRet->IsVariableSized() || pRet->IsString())
            return true;
    }
    return false;
}

/** \brief counts the parameters of a specific front-end type
 *  \param nFEType the type to count
 *  \param nDirection the direction to count
 *  \return the number of parameters, which have this type
 */
int CBEFunction::GetParameterCount(int nFEType, DIRECTION_TYPE nDirection)
{
    if (nDirection == DIRECTION_INOUT)
    {
        int nCountIn = GetParameterCount(nFEType, DIRECTION_IN);
        int nCountOut = GetParameterCount(nFEType, DIRECTION_OUT);
        return std::max(nCountIn, nCountOut);
    }

    int nCount = 0;

    CBEType *pType;
    CBEAttribute *pAttr;
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
        if (!(*iter)->IsDirection(nDirection))
            continue;
        pType = (*iter)->GetType();
        if ((pAttr = (*iter)->m_Attributes.Find(ATTR_TRANSMIT_AS)) != 0)
            pType = pAttr->GetAttrType();
        if (!pType->IsOfType(nFEType))
	    continue;

	// to catch array parameters, we have to get the maximum size and
	// divide by the size of the type
	int nParamSize = 0;
	CBESizes *pSizes = CCompiler::GetSizes();
	int nTypeSize = pSizes->GetSizeOfType(nFEType);
	if ((*iter)->GetMaxSize(nParamSize))
	    nParamSize /= nTypeSize;
	else
	    nParamSize = 1; // it's variable size, so count it at least once
	if (nParamSize > 0)
	    nCount += nParamSize;
	else
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
int
CBEFunction::GetParameterCount(ATTR_TYPE nMustAttrs,
    ATTR_TYPE nMustNotAttrs,
    DIRECTION_TYPE nDirection)
{
    if (nDirection == DIRECTION_INOUT)
    {
        int nCountIn = GetParameterCount(nMustAttrs, nMustNotAttrs,
	    DIRECTION_IN);
        int nCountOut = GetParameterCount(nMustAttrs, nMustNotAttrs,
	    DIRECTION_OUT);
        return std::max(nCountIn, nCountOut);
    }

    int nCount = 0;
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
        if (!(*iter)->IsDirection(nDirection))
            continue;
        // test for attributes that it must have (if it hasn't, skip count)
        if (!(*iter)->m_Attributes.Find(nMustAttrs))
            continue;
        // test for attributes that it must NOT have (skip count)
        if ((*iter)->m_Attributes.Find(nMustNotAttrs))
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
 *
 * This function should be overloaded, because the functions should be added to the
 * files depending on their instance and attributes.
 */
void CBEFunction::AddToHeader(CBEHeaderFile* pHeader)
{
    if (IsTargetFile(pHeader))
        pHeader->m_Functions.Add(this);
}

/** \brief adds this function to the implementation file
 *  \param pImpl the implementation file
 *
 * This is usually only used for global functions.
 */
void CBEFunction::AddToImpl(CBEImplementationFile* pImpl)
{
    if (IsTargetFile(pImpl))
        pImpl->m_Functions.Add(this);
}

/** \brief tests if this function has array parameters
 *  \param nDirection the direction to test
 *  \return true if it has
 */
bool CBEFunction::HasArrayParameters(DIRECTION_TYPE nDirection)
{
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
         if (!(*iter)->IsDirection(nDirection))
             continue;
	CBEDeclarator *pDecl = (*iter)->m_Declarators.First();
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s checking %s (array: %s)\n",
	    __func__, pDecl->GetName().c_str(), (pDecl->IsArray()) ? "yes" : "no");
        if (pDecl->IsArray())
            return true;
    }
    CBETypedDeclarator *pRet = GetReturnVariable();
    if (pRet && pRet->IsDirection(nDirection))
    {
	CBEDeclarator *pDecl = pRet->m_Declarators.First();
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s checking %s (array: %s)\n",
	    __func__, pDecl->GetName().c_str(), (pDecl->IsArray()) ? "yes" : "no");
        if (pDecl->IsArray())
            return true;
    }
    return false;
}

/** \brief tries to find a parameter with a specific type
 *  \param sTypeName the name of the type to look for
 *  \return a reference to the found parameter
 *
 * We test the regular parameter and if it exists the message buffer.
 */
CBETypedDeclarator* CBEFunction::FindParameterType(string sTypeName)
{
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
        CBEType *pType = (*iter)->GetType();
        if (dynamic_cast<CBEUserDefinedType*>(pType))
        {
            if (((CBEUserDefinedType*)pType)->GetName() == sTypeName)
                return *iter;
        }
        if (pType->HasTag(sTypeName))
            return *iter;
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
DIRECTION_TYPE CBEFunction::GetSendDirection()
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
DIRECTION_TYPE CBEFunction::GetReceiveDirection()
{
    return DIRECTION_OUT;
}

/** \brief performs basic initializations
 *  \param pFEObject the front-end object to use as reference
 *  \return true if successful
 *
 * We need to create the CORBA object and environment variables.
 * we also create the communication and marshalling class. We do
 * NOT create the message buffer here, because derived class first
 * call this function, then do some modifications, and after the
 * modifications they want to add the message buffer. Thus we split
 * the message buffer creation into the AddMessageBuffer method that
 * is called by derived CreateBackEnd methods.
 *
 * An easier way to go is to split CreateBackEnd similar to write
 * by calling sub-method, which can be overloaded. But currently
 * I cannot think of a reasonable split structure.
 */
void
CBEFunction::CreateBackEnd(CFEBase *pFEObject, bool bComponentSide)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);

    // call CBEObject's CreateBackEnd method
	SetComponentSide(bComponentSide);
    CBEObject::CreateBackEnd(pFEObject);

    // init CORBA Object
    CreateObject();
    // init CORBA Environment
    CreateEnvironment();
}

/** \brief create the marshaller class
 *
 * We have to use a seperate method, since the marshaller might require the
 * message buffer to be fully initialized. Since the message buffer is out of
 * line created, we have to add the marshaller also out of line.
 */
void
CBEFunction::CreateMarshaller()
{
    // create marshaller class
    if (m_pMarshaller)
	delete m_pMarshaller;
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    m_pMarshaller = pCF->GetNewMarshaller();
    assert(m_pMarshaller);
    m_pMarshaller->SetParent(this);
    m_pMarshaller->AddLocalVariable(this);
}

/** \brief create the communication calss
 *
 * We have to use a seperate method, since the communication class might
 * require the message buffer to be fully initialized. Since the message
 * buffer is out of line created, we have to add the communication class also
 * out of line.
 */
void
CBEFunction::CreateCommunication()
{
    // set the communication class (does not require initialization)
    if (m_pComm)
        delete m_pComm;
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    m_pComm = pCF->GetNewCommunication();
    assert(m_pComm);
    m_pComm->AddLocalVariable(this);
}

/** \brief create tracing class
 */
void
CBEFunction::CreateTrace()
{
	if (m_pTrace)
		delete m_pTrace;
	CBEClassFactory *pCF = CCompiler::GetClassFactory();
	m_pTrace = pCF->GetNewTrace();
	if (m_pTrace)
		m_pTrace->AddLocalVariable(this);
}

/** \brief creates the CORBA_Object variable (and member)
 *  \return true if successful
 */
void
CBEFunction::CreateObject()
{
    // trace remove old object
    if (m_pCorbaObject)
    {
        delete m_pCorbaObject;
        m_pCorbaObject = 0;
    }

    CBENameFactory *pNF = CCompiler::GetNameFactory();
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    string exc = string(__func__);

    string sTypeName("CORBA_Object");
    string sName = pNF->GetCorbaObjectVariable();
    m_pCorbaObject = pCF->GetNewTypedDeclarator();
    m_pCorbaObject->SetParent(this);
    m_pCorbaObject->CreateBackEnd(sTypeName, sName, 0);
    // CORBA_Object is always in
    CBEAttribute *pAttr = pCF->GetNewAttribute();
    pAttr->CreateBackEnd(ATTR_IN);
    m_pCorbaObject->m_Attributes.Add(pAttr);
}

/** \brief creates the CORBA_Environment variable (and parameter)
 *  \return true if successful
 */
void
CBEFunction::CreateEnvironment()
{
	// clean up
	if (m_pCorbaEnv)
	{
		delete m_pCorbaEnv;
		m_pCorbaEnv = 0;
	}

	CBENameFactory *pNF = CCompiler::GetNameFactory();
	CBEClassFactory *pCF = CCompiler::GetClassFactory();
	// if function is at server side, this is a CORBA_Server_Environment
	string sTypeName;
	if (IsComponentSide())
		sTypeName = "CORBA_Server_Environment";
	else
		sTypeName = "CORBA_Environment";
	string sName = pNF->GetCorbaEnvironmentVariable();
	m_pCorbaEnv = pCF->GetNewTypedDeclarator();
	m_pCorbaEnv->SetParent(this);
	m_pCorbaEnv->CreateBackEnd(sTypeName, sName, 1);
}

/** \brief sets the corba-environment member
 *  \param pEnv the new member
 */
void CBEFunction::SetEnvironment(CBETypedDeclarator* pEnv)
{
	if (m_pCorbaEnv)
		delete m_pCorbaEnv;
	m_pCorbaEnv = pEnv;
}

/** \brief sets the second declarator of the typed decl to the name and stars
 *  \param pTypedDecl the parameter to alter
 *  \param sNewDeclName the new name of the second declarator
 *  \param nStars the new number of stars of the second declarator
 *
 * This function is only allowed to be called by the other SetCallVariable
 * method. No other class, not even a derived one may call it. This way we
 * ensure that only parameters from the call parameter list are used.
 */
void
CBEFunction::SetCallVariable(CBETypedDeclarator *pTypedDecl,
	string sNewDeclName,
	int nStars)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEFunction::%s(decl: %s, %s, %d) called\n", __func__,
	pTypedDecl->m_Declarators.First()->GetName().c_str(),
	sNewDeclName.c_str(), nStars);
    // check if there is already a second declarator
    CBEDeclarator *pSecond = pTypedDecl->GetCallDeclarator();
    if (pSecond)
        pSecond->CreateBackEnd(sNewDeclName, nStars);
    else
    {
        pSecond = CCompiler::GetClassFactory()->GetNewDeclarator();
        pTypedDecl->m_Declarators.Add(pSecond);
	pSecond->CreateBackEnd(sNewDeclName, nStars);
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEFunction::%s(decl) returns\n", __func__);
}

/** \brief create a call varaiable for the original parameter
 *  \param sOriginalName the name of the original parameter
 *  \param nStars the number of indirections of the variable used in the call
 *  \param sCallName the name of the variable used in the call
 *
 * When calling a function the variables which are used to match the parameters
 * may have a different number of stars. This has to be matched with the number
 * of stars used inside the function (the parameter's stars).
 *
 * Therefore we create a "shadow" parameters used for function calls, which has
 * a own name, which usually corresponds to the parameter's name, and has a
 * different number of stars. When writing the "shadow" parameter the
 * difference between these numbers of stars is used to determine whether the
 * call varaible has to be referenced or dereferenced.
 *
 * Instead of replacing the old declarator with a new one, we simply create a
 * second declarator. Thus is valid since a parameter should have one
 * declarator only. So the second becomes out calling variable name.
 *
 * We do not have t check for CORBA Object and Environment explicetly, because
 * they are part of the parameter list.
 */
void
CBEFunction::SetCallVariable(string sOriginalName,
	int nStars,
	string sCallName)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEFunction::%s(%s, %d, %s) called\n", __func__,
	sOriginalName.c_str(), nStars, sCallName.c_str());
    // clone existing parameters if not yet done
    if (m_CallParameters.empty())
    {
        vector<CBETypedDeclarator*>::iterator iter;
        for (iter = m_Parameters.begin(); iter != m_Parameters.end(); iter++)
        {
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEFunction::%s cloning %s\n", __func__,
		(*iter)->m_Declarators.First()->GetName().c_str());
            CBETypedDeclarator *pParam =
		(CBETypedDeclarator*)((*iter)->Clone());
            m_CallParameters.Add(pParam);
        }
    }
    // search for original name
    CBETypedDeclarator *pCallParam = m_CallParameters.Find(sOriginalName);
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEFunction::%s call param for %s at %p\n", __func__,
	sOriginalName.c_str(), pCallParam);
    // if we didn't find anything, then pCallParam is 0
    if (!pCallParam)
        return;
    // new name and new stars
    SetCallVariable(pCallParam, sCallName, nStars);

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEFunction::%s returns\n", __func__);
}

/** \brief remove the call declarator set for a parameter
 *  \param sCallName the name of the call declarator to remove
 */
void
CBEFunction::RemoveCallVariable(string sCallName)
{
    CBETypedDeclarator *pCallParam = m_CallParameters.Find(sCallName);
    if (!pCallParam)
	return;
    pCallParam->RemoveCallDeclarator();
}

/** \brief tries to find the parameter of the given declarator
 *  \param pDeclarator the declarator with the name of the parameter
 *  \param bCall true if this is used for a call to this function
 *  \return a reference to the parameter
 */
CBETypedDeclarator*
CBEFunction::GetParameter(CBEDeclarator *pDeclarator, bool bCall)
{
    assert(pDeclarator);
    CBETypedDeclarator *pParameter =
	FindParameter(pDeclarator->GetName(), bCall);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s FindParameter(%s) returned %p\n",
	__func__, pDeclarator->GetName().c_str(), pParameter);
    // declarators should be the same, e.g.
    // the function 'f(int x, struct_t b)' with struct_t = { int x, y; },
    // will return true for a.x, because it thinks it is x
    if (pParameter)
    {
        if (pParameter->m_Declarators.Find(pDeclarator->GetName()) != pDeclarator)
            pParameter = 0;
    }
    return pParameter;
}

/** \brief writes the attributes for the function
 *  \param pFile the file to write to
 */
void CBEFunction::WriteFunctionAttributes(CBEFile& /*pFile*/)
{}

/** \brief access to opcode constant names
 *  \return a string conatining the opcode name
 */
string CBEFunction::GetOpcodeConstName()
{
    return m_sOpcodeConstName;
}

/** \brief tries to find a parameter with a specific attribute
 *  \param nAttributeType the attribute type to look for
 *  \return the first parameter with the given attribute
 */
CBETypedDeclarator*
CBEFunction::FindParameterAttribute(ATTR_TYPE nAttributeType)
{
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
        if ((*iter)->m_Attributes.Find(nAttributeType))
            return *iter;
    }
    return 0;
}

/** \brief tries to find a parameter with a specific IS attribute
 *  \param nAttributeType the attribute type to look for
 *  \param sAttributeParameter the name of the attributes parameter to look for
 *  \return the first parameter with the given attribute
 */
CBETypedDeclarator*
CBEFunction::FindParameterIsAttribute(ATTR_TYPE nAttributeType,
    string sAttributeParameter)
{
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_Parameters.begin();
	 iter != m_Parameters.end();
	 iter++)
    {
        CBEAttribute *pAttr = (*iter)->m_Attributes.Find(nAttributeType);
        if (pAttr && pAttr->m_Parameters.Find(sAttributeParameter))
            return *iter;
    }
    return 0;
}

/** \brief constructs the string to initialize the exception variable
 *  \return the init string
 */
string CBEFunction::GetExceptionWordInitString()
{
	string sInitString;
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
	{
		// ((dice_CORBA_exception_type){ _corba: { .major = env.major, .repos_id = env.repos_id }})._raw
		sInitString =
			string("((dice_CORBA_exception_type){ _corba: { .major = ");
		// add variable name of envrionment
		CBEDeclarator *pDecl = m_pCorbaEnv->m_Declarators.First();
		sInitString += "DICE_EXCEPTION_MAJOR(";
		if (pDecl->GetStars() == 0)
			sInitString += "&";
		sInitString += pDecl->GetName();
		sInitString += "), .repos_id = DICE_EXCEPTION_MINOR(";
		if (pDecl->GetStars() == 0)
			sInitString += "&";
		sInitString += pDecl->GetName();
		sInitString += ") }})._raw";
	}
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
	{
		CBEDeclarator *pDecl = m_pCorbaEnv->m_Declarators.First();
		sInitString = pDecl->GetName();
		if (pDecl->GetStars() == 0)
			sInitString += ".";
		else
			sInitString += "->";
		sInitString += "_exception._raw";
	}
	return sInitString;
}

/** \brief writes the initialization of the exception word variable
 *  \param pFile the file to write to
 */
void CBEFunction::WriteExceptionWordInitialization(CBEFile& pFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__,
	GetName().c_str());

    CBETypedDeclarator *pExceptionVar = GetExceptionVariable();
    if (!pExceptionVar)
        return;

    // get name of exception word
    CBEDeclarator *pDecl = pExceptionVar->m_Declarators.First();
    string sInitString = GetExceptionWordInitString();
    // get name of exception word
    pFile << "\t" << pDecl->GetName() << " = " << sInitString << ";\n";

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s returns\n", __func__,
	GetName().c_str());
}

/** \brief writes the check of the exception members of the environment
 *  \param pFile the file to write to
 */
void CBEFunction::WriteExceptionCheck(CBEFile& /*pFile*/)
{}

/** \brief returns the bytes to use for padding a parameter to its size
 *  \param nCurrentOffset the current position in the message buffer
 *  \param nParamSize the size of the parameter in bytes
 *  \return the number of bytes to align this parameter in the msgbuf
 */
int CBEFunction::GetParameterAlignment(int nCurrentOffset, int nParamSize)
{
    if (!CCompiler::IsOptionSet(PROGRAM_ALIGN_TO_TYPE))
        return 0;
    if (nParamSize == 0)
        return 0;
    int nMWordSize = CCompiler::GetSizes()->GetSizeOfType(TYPE_MWORD);
    /* always align to word size if type is bigger */
    if (nParamSize > nMWordSize)
        nParamSize = nMWordSize;
    int nAlignment = 0;
    if (nCurrentOffset % nParamSize)
        nAlignment = nParamSize - nCurrentOffset % nParamSize;
    nAlignment = (nAlignment < nMWordSize) ? nAlignment : nMWordSize;
    return nAlignment;
}

/** \brief adds a local variable to the function
 *  \param pVariable the variable to add
 */
void CBEFunction::AddLocalVariable(CBETypedDeclarator *pVariable)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s (%p) called\n", __func__,
	pVariable);
    if (!pVariable)
        return;
    if (pVariable->m_Declarators.First() &&
	m_LocalVariables.Find(pVariable->m_Declarators.First()->GetName()))
	return;
    m_LocalVariables.Add(pVariable);
}

/** \brief retrieve reference to exception variable
 *  \return reference to exception variable if exists
 *
 * Searches the local variables for the exception variable.
 */
CBETypedDeclarator*
CBEFunction::GetExceptionVariable()
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sName = pNF->GetExceptionWordVariable();
    return m_LocalVariables.Find(sName);
}

/** \brief retrieve reference to return variable
 *  \return reference to return variable if exists
 *
 * Searches the local variables for the return variable.
 * Since it may have different names, we use the OUT attribute
 * that has been set by the SetReturnVar methods. Simply
 * search local variables for the OUT attribute.
 */
CBETypedDeclarator*
CBEFunction::GetReturnVariable()
{
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = m_LocalVariables.begin();
	 iter != m_LocalVariables.end();
	 iter++)
    {
        if ((*iter)->m_Attributes.Find(ATTR_OUT))
            return *iter;
    }
    return 0;
}

/** \brief creates and adds a local variable
 *  \param sUserType the user defined type
 *  \param sName the name of the variable
 *  \param nStars the number of pointers of the variable
 *  \param sInit the default init string
 *  \return true if successful
 *
 * This method creates a user defined type and variable and
 * adds the created variable to the local variable vector.
 */
void
CBEFunction::AddLocalVariable(string sUserType,
    string sName,
    int nStars,
    string sInit)
{
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBETypedDeclarator *pVariable = pCF->GetNewTypedDeclarator();
    AddLocalVariable(pVariable);
    pVariable->CreateBackEnd(sUserType, sName, nStars);
    if (!sInit.empty())
        pVariable->SetDefaultInitString(sInit);
}

/** \brief creates and adds a local variable
 *  \param nFEType the user defined type
 *  \param bUnsigned true if unsigned type
 *  \param nSize the size of the type in bytes
 *  \param sName the name of the variable
 *  \param nStars the number of pointers of the variable
 *  \param sInit the default init string
 *  \return true if successful
 *
 * This method creates a user defined type and variable and
 * adds the created variable to the local variable vector.
 */
void
CBEFunction::AddLocalVariable(int nFEType,
    bool bUnsigned,
    int nSize,
    string sName,
    int nStars,
    string sInit)
{
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEType *pType = pCF->GetNewType(nFEType);
    CBETypedDeclarator *pVariable = pCF->GetNewTypedDeclarator();
    pType->SetParent(pVariable);
    AddLocalVariable(pVariable);
    pType->CreateBackEnd(bUnsigned, nSize, nFEType);
    pVariable->CreateBackEnd(pType, sName);
    delete pType; // cloned by typed decl.

    if (nStars > 0)
        pVariable->m_Declarators.First()->IncStars(nStars);
    if (!sInit.empty())
        pVariable->SetDefaultInitString(sInit);
}

/** \brief sets the name of the function according to the front-end operation
 *  \param pFEOperation the front-end operation to use as reference
 *  \param nFunctionType the type of function to get the name for
 */
void
CBEFunction::SetFunctionName(CFEOperation *pFEOperation,
    FUNCTION_TYPE nFunctionType)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    m_sName = pNF->GetFunctionName(pFEOperation, nFunctionType, IsComponentSide());
    m_sOriginalName = pFEOperation->GetName();
}

/** \brief sets the name of the function according to the front-end interface
 *  \param pFEInterface the front-end interface to use as reference
 *  \param nFunctionType the type of the function to set the name for
 */
void
CBEFunction::SetFunctionName(CFEInterface *pFEInterface,
    FUNCTION_TYPE nFunctionType)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    m_sName = pNF->GetFunctionName(pFEInterface, nFunctionType, IsComponentSide());
    m_sOriginalName = string();
}

/** \brief sets the name of the function (and the original name if given)
 *  \param sName the function name
 *  \param sOriginalName the original name
 */
void
CBEFunction::SetFunctionName(string sName,
    string sOriginalName)
{
    m_sName = sName;
    m_sOriginalName = sOriginalName;
}

/** \brief helper that creates an opcode variable
 *  \return a reference to the newly created opcode variable
 *
 * If error, throws exception.
 */
CBETypedDeclarator*
CBEFunction::CreateOpcodeVariable()
{
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEOpcodeType *pOpcodeType = pCF->GetNewOpcodeType();
    CBETypedDeclarator *pOpcode = pCF->GetNewTypedDeclarator();
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sOpcode = pNF->GetOpcodeVariable();
    pOpcode->SetParent(this);
    pOpcodeType->SetParent(pOpcode);
    pOpcodeType->CreateBackEnd();
    pOpcode->CreateBackEnd(pOpcodeType, sOpcode);
    delete pOpcodeType; // cloned in CBETypedDeclarator::CreateBackEnd

    return pOpcode;
}

/** \brief add the exception variable to the local variables
 */
void
CBEFunction::AddExceptionVariable()
{
    if (m_Attributes.Find(ATTR_NOEXCEPTIONS))
        return;

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);

    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    // create type
    CBEType *pType = pCF->GetNewType(TYPE_MWORD);
    pType->CreateBackEnd(true, 0, TYPE_MWORD);

    // create the exception word variable
    string sName = pNF->GetExceptionWordVariable();
    CBETypedDeclarator *pExceptionVar = pCF->GetNewTypedDeclarator();
    pExceptionVar->CreateBackEnd(pType, sName);
    delete pType; // cloned by typed decl.

    // add directional attribute, so the test if this should be marshalled
    // will work
    CBEAttribute *pAttr = pCF->GetNewAttribute();
    pAttr->SetParent(pExceptionVar);
    pAttr->CreateBackEnd(ATTR_OUT);
    pExceptionVar->m_Attributes.Add(pAttr);

    AddLocalVariable(pExceptionVar);
//     pExceptionVar->SetDefaultInitString(GetExceptionWordInitString());

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

/** \brief check if parameter should be written
 *  \param pParam the parameter to check
 *  \return true if so
 *
 * This implementation does not really skip parameters
 */
bool
CBEFunction::DoWriteParameter(CBETypedDeclarator* /*pParam*/)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s() called\n", __func__);
    return true;
}

/** \brief write the C++ access specifier before each function declaration
 *  \param pFile the file to write to
 *
 * per default, all functions are public.
 */
void CBEFunction::WriteAccessSpecifier(CBEHeaderFile& pFile)
{
    if (!CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
	return;

    --pFile << "\tpublic:" << std::endl;
    ++pFile;
}


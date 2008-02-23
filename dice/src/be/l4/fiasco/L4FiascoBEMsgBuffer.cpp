/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEMsgBuffer.cpp
 *  \brief   contains the implementation of the class CL4FiascoBEMsgBuffer
 *
 *  \date    08/24/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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

#include "L4FiascoBEMsgBuffer.h"
#include "be/l4/L4BESizes.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BESrvLoopFunction.h"
#include "be/BEMarshalFunction.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BEReplyFunction.h"
#include "be/BECallFunction.h"
#include "be/BEClass.h"
#include "be/BEStructType.h"
#include "be/BEUserDefinedType.h"
#include "be/BEAttribute.h"
#include "be/BEDeclarator.h"
#include "fe/FEOperation.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"
#include "Error.h"
#include <cassert>

CL4FiascoBEMsgBuffer::CL4FiascoBEMsgBuffer()
 : CL4BEMsgBuffer(),
	m_bIsUtcb(false)
{ }

CL4FiascoBEMsgBuffer::CL4FiascoBEMsgBuffer(CL4FiascoBEMsgBuffer* src)
 : CL4BEMsgBuffer(src),
	m_bIsUtcb(src->m_bIsUtcb)
{ }

/** destroys the object */
CL4FiascoBEMsgBuffer::~CL4FiascoBEMsgBuffer()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CL4FiascoBEMsgBuffer* CL4FiascoBEMsgBuffer::Clone()
{
	return new CL4FiascoBEMsgBuffer(this);
}

/** \brief add platform specific members to specific struct
 *  \param pFunction the function of the message buffer
 *  \param pStruct the struct to add the members to
 *  \return true if successful
 *
 *  In this implementation we should add the L4 specific receive
 *  window for flexpages, the size and the send dope.
 */
void CL4FiascoBEMsgBuffer::AddPlatformSpecificMembers(CBEFunction *pFunction, CBEStructType *pStruct)
{
	CCompiler::VerboseI("CL4FiascoBEMsgBuffer::%s(%s,, %d) called\n",
		__func__, pFunction->GetName().c_str(), (int)GetStructType(pStruct));

	CL4BEMsgBuffer::AddPlatformSpecificMembers(pFunction, pStruct);

	// create receive flexpage
	CBETypedDeclarator *pFlexpage = GetFlexpageVariable();
	pStruct->m_Members.Add(pFlexpage);
	// create size dope
	CBETypedDeclarator *pSizeDope = GetSizeDopeVariable();
	pStruct->m_Members.Add(pSizeDope);
	// create send dope
	CBETypedDeclarator *pSendDope = GetSendDopeVariable();
	pStruct->m_Members.Add(pSendDope);

	CCompiler::VerboseD("CL4FiascoBEMsgBuffer::%s: returns true\n", __func__);
}

/** \brief return the offset where the payload starts
 *  \return the offset in bytes
 */
int CL4FiascoBEMsgBuffer::GetPayloadOffset()
{
	CCompiler::Verbose("CL4FiascoBEMsgBuffer::GetPayloadOffset called\n");

	if (IsUtcb())
		return 0;

	CBESizes *pSizes = CCompiler::GetSizes();
	int nPage = pSizes->GetSizeOfType(TYPE_RCV_FLEXPAGE);
	int nDope = pSizes->GetSizeOfType(TYPE_MSGDOPE_SEND);
	return nPage + 2*nDope;
}

/** \brief adds a generic structure to the message buffer
 *  \param pFunction the function which owns the message buffer
 *  \param pFEOperation the front-end operation to use as reference
 *  \return true if successful
 *
 * For non interface functions we test if the members for both directions are
 * sufficient for short IPC (2/3 word sized members at the begin). If not, we
 * add an extra struct to the union containing base elements and 2/3 word
 * sized members. Always add the struct if doing UTCB IPC.
 */
void CL4FiascoBEMsgBuffer::AddGenericStruct(CBEFunction *pFunction, CFEOperation *pFEOperation)
{
	// test if this an interface function
	if (dynamic_cast<CBEInterfaceFunction*>(pFunction))
		return;

	if (HasWordMembers(pFunction, pFunction->GetSendDirection()) &&
		HasWordMembers(pFunction, pFunction->GetReceiveDirection()) &&
		!m_bIsUtcb)
		return;

	CL4BEMsgBuffer::AddGenericStruct(pFunction, pFEOperation);
}

/** \brief the post-create (and post-sort) step during creation
 *  \param pFunction the function owning this message buffer
 *  \param pFEOperation the front-end reference operation
 *  \return true if successful
 *
 * If this is a utcb IPC message buffer we remove the size and send dope and
 * the receive flexpage.
 *
 * We also have to do the check if this is a UTCB IPC here, using a function
 * that dynamically checks on each invocation will cause an recursive loop
 * because it uses function which then again use IsUtcb and so on.
 *
 * For the fiasco back-end we migrate slowly to fully using UTCBs:
 * # first only direct items are supported in the UTCB, thus we check here if
 *   the message buffer is smaller than the desired UTCB size and if it has no
 *   refstrings nor flexpages.
 * # second, when typed items are supported in the UTCB, we use the same
 *   conversion functions as the V4 backend, which are already present in the
 *   L4 generic message buffer
 *
 * For the first step we get the size of the maximum size of the message
 * buffer and compare it to the size of the UTCB area. Then we check for
 * members with ATTR_REF and with type FLEXPAGE.
 *
 * Then, because at the client side we can handle only either long IPC
 * messages OR UTCB IPC messages for BOTH directions, but not long for one and
 * UTCB for the other (yet), we have to make exceptions to the UTCB IPC
 * message buffer if the function is a (un)marshal or reply function. Then we try
 * to find the corresponding call function and check it's message buffer.
 */
void CL4FiascoBEMsgBuffer::PostCreate(CBEFunction *pFunction, CFEOperation *pFEOperation)
{
	CCompiler::Verbose("CL4FiascoBEMsgBuffer::PostCreate(%s, %s) called\n",
		pFunction ? pFunction->GetName().c_str() : "(none)",
		pFEOperation ? pFEOperation->GetName().c_str() : "(none fe)");

	CL4BEMsgBuffer::PostCreate(pFunction, pFEOperation);

	// detecting if this message buffer fits into a UTCB for IPC
	int nMaxSize = 0;
	GetMaxSize(nMaxSize);
	nMaxSize -= GetPayloadOffset();

	CBESizes *pSizes = CCompiler::GetSizes();
	int nUtcbSize = pSizes->GetSizeOfType(TYPE_UTCB);
	if (0 < nMaxSize && nMaxSize < nUtcbSize &&
		// if using generic as struct type, all types are counted
		GetCountAll(TYPE_FLEXPAGE, CMsgStructType::Generic) == 0)
	{
		CCompiler::Verbose("CL4FiascoBEMsgBuffer::PostCreate: size fits and no flexpages, testing ref\n");
		bool bRefFound = false;
		CBEStructType *pStruct = GetStruct(CMsgStructType::In);
		if (pStruct && pStruct->FindMemberAttribute(ATTR_REF))
			bRefFound = true;
		pStruct = GetStruct(CMsgStructType::Out);
		if (pStruct && pStruct->FindMemberAttribute(ATTR_REF))
			bRefFound = true;
		pStruct = GetStruct(CMsgStructType::Exc);
		if (pStruct && pStruct->FindMemberAttribute(ATTR_REF))
			bRefFound = true;
		m_bIsUtcb = !bRefFound;
		CCompiler::Verbose("CL4FiascoBEMsgBuffer::PostCreate: m_bIsUtcb is now %s\n", m_bIsUtcb ? "true" : "false");
	}

	if (m_bIsUtcb &&
		(dynamic_cast<CBEUnmarshalFunction*>(pFunction) ||
		 dynamic_cast<CBEMarshalFunction*>(pFunction) ||
		 dynamic_cast<CBEReplyFunction*>(pFunction)) &&
		!pFunction->m_Attributes.Find(ATTR_OUT) &&
		!pFunction->m_Attributes.Find(ATTR_IN))
	{
		CBEClass *pClass = GetSpecificParent<CBEClass>();
		assert(pClass);
		CBEFunction *pFunc = pClass->FindFunctionFor(pFunction, FUNCTION_CALL);
		assert(pFunc);
		CBEMsgBuffer *pMsgBuffer = pFunc->GetMessageBuffer();
		if (pMsgBuffer && !pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_UTCB_IPC,
				CMsgStructType::Generic))
			m_bIsUtcb = false;
	}


	if (m_bIsUtcb)
	{
		CCompiler::Verbose("CL4FiascoBEMsgBuffer::PostCreate: removing superfluous members\n");
		CBENameFactory *pNF = CBENameFactory::Instance();
		CMsgStructType nType(CMsgStructType::Generic);
		for (; CMsgStructType::Max != nType; ++nType)
		{
			CCompiler::Verbose("CL4FiascoBEMsgBuffer::PostCreate: checking struct for dir %d\n", (int)nType);
			CBEStructType *pStruct = GetStruct(pFunction, nType);
			if (!pStruct)
				continue;
			// remove receive flexpage
			string sName = pNF->GetMessageBufferMember(TYPE_RCV_FLEXPAGE);
			CBETypedDeclarator *pMember = FindMember(sName, pFunction, nType);
			pStruct->m_Members.Remove(pMember);
			// remove size dope
			sName = pNF->GetMessageBufferMember(TYPE_MSGDOPE_SIZE);
			pMember = FindMember(sName, pFunction, nType);
			pStruct->m_Members.Remove(pMember);
			// remove send dope
			sName = pNF->GetMessageBufferMember(TYPE_MSGDOPE_SEND);
			pMember = FindMember(sName, pFunction, nType);
			pStruct->m_Members.Remove(pMember);
		}

		// if we do not have a generic struct already, add it here.
		if (!GetStruct(pFunction, CMsgStructType::Generic))
			AddGenericStruct(pFunction, pFEOperation);
	}
}

/** \brief writes the initialisation of a refstring member with the init func
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pClass the class to write for
 *  \param nIndex the index of the refstring in an array
 *  \param nType the type of the message buffer struct
 *  \return true if we wrote something, false if not
 */
bool CL4FiascoBEMsgBuffer::WriteRefstringInitFunction(CBEFile& pFile, CBEFunction *pFunction,
	CBEClass *pClass, int nIndex, CMsgStructType nType)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	// check if this is server side, and if so, check for
	// init-rcvstring attribute of class
	if (pFunction->IsComponentSide() &&
		(CCompiler::IsOptionSet(PROGRAM_INIT_RCVSTRING) ||
		 (pClass &&
		  (pClass->m_Attributes.Find(ATTR_INIT_RCVSTRING) ||
		   pClass->m_Attributes.Find(ATTR_INIT_RCVSTRING_CLIENT) ||
		   pClass->m_Attributes.Find(ATTR_INIT_RCVSTRING_SERVER)))))
	{
		string sFunction;
		if (!CCompiler::IsOptionSet(PROGRAM_INIT_RCVSTRING))
		{
			CBEAttribute *pAttr = 0;
			if ((pAttr = pClass->m_Attributes.Find(ATTR_INIT_RCVSTRING))
				!= 0)
				sFunction = pAttr->GetString();
			if (((pAttr = pClass->m_Attributes.Find(
							ATTR_INIT_RCVSTRING_CLIENT)) != 0) &&
				pFile.IsOfFileType(FILETYPE_CLIENT))
				sFunction = pAttr->GetString();
			if (((pAttr = pClass->m_Attributes.Find(
							ATTR_INIT_RCVSTRING_SERVER)) != 0) &&
				pFile.IsOfFileType(FILETYPE_COMPONENT))
				sFunction = pAttr->GetString();
		}
		if (sFunction.empty())
			sFunction = pNF->GetString(CL4BENameFactory::STR_INIT_RCVSTRING_FUNC);
		else
			sFunction = pNF->GetString(CL4BENameFactory::STR_INIT_RCVSTRING_FUNC,
				(void*)&sFunction);

		CBETypedDeclarator *pEnvVar = pFunction->GetEnvironment();
		CBEDeclarator *pEnv = pEnvVar->m_Declarators.First();
		// call the init function for the indirect string
		pFile << "\t" << sFunction << " ( " << nIndex << ", &(";
		WriteMemberAccess(pFile, pFunction, nType, TYPE_REFSTRING, nIndex);
		pFile << ".rcv_str), &(";
		WriteMemberAccess(pFile, pFunction, nType, TYPE_REFSTRING, nIndex);
		pFile << ".rcv_size), " << pEnv->GetName() << ");\n";

		// OK, next!
		return true;
	}
	return false;
}

/** \brief writes the refstring member init for given parameter
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pMember the member to write
 *  \param nIndex the index of the refstring to initialize
 *  \param nType the type of the message buffer struct
 */
void CL4FiascoBEMsgBuffer::WriteRefstringInitParameter(CBEFile& pFile, CBEFunction *pFunction,
    CBETypedDeclarator *pMember, int nIndex, CMsgStructType nType)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sWord = pNF->GetTypeName(TYPE_MWORD, true);
	// get parameter
	CBETypedDeclarator *pParameter = pFunction->FindParameter(
		pMember->m_Declarators.First()->GetName());

	if (pParameter && (
			(pFile.IsOfFileType(FILETYPE_CLIENT) &&
			 pParameter->m_Attributes.Find(ATTR_PREALLOC_CLIENT)) ||
			(pFile.IsOfFileType(FILETYPE_COMPONENT) &&
			 pParameter->m_Attributes.Find(ATTR_PREALLOC_SERVER))) )
	{
		pFile << "\t";
		WriteAccess(pFile, pFunction, nType, pMember);
		pFile << ".rcv_str = (" << sWord << ")(";
		CBEDeclarator *pDecl = pParameter->m_Declarators.First();
		int nStars = pDecl->GetStars();
		CBEType *pType = pParameter->GetType();
		if (!pType->IsPointerType())
			nStars--;
		for (; nStars > 0; nStars--)
			pFile << "*";
		pFile << pDecl->GetName() << ");\n";

		pFile << "\t";
		WriteAccess(pFile, pFunction, nType, pMember);
		pFile << ".rcv_size = ";
		if ((pParameter->m_Attributes.Find(ATTR_SIZE_IS)) ||
			(pParameter->m_Attributes.Find(ATTR_LENGTH_IS)))
		{
			if (pType->GetSize() > 1)
				pFile << "(";
			pParameter->WriteGetSize(pFile, 0, pFunction);
			if (pType->GetSize() > 1)
			{
				pFile << ")*sizeof";
				pType->WriteCast(pFile, false);
			}
		}
		else if (pParameter->m_Attributes.Find(ATTR_STRING) &&
			pParameter->m_Attributes.Find(ATTR_MAX_IS) &&
			pParameter->m_Attributes.Find(ATTR_OUT))
			// WriteGetSize would write strlen, wich is not quite wha we want
			// in this situation
			pParameter->WriteGetMaxSize(pFile, 0, pFunction);
		else
			// write max-is
			pParameter->WriteGetSize(pFile, 0, pFunction);
		pFile << ";\n";

	}
	else
	{
		bool bUseArray = pMember->m_Declarators.First()->IsArray() &&
			pMember->m_Declarators.First()->GetArrayDimensionCount() > 0;
		pFile << "\t";
		WriteAccess(pFile, pFunction, nType, pMember);
		if (bUseArray)
			pFile << "[" << nIndex << "]";
		pFile << ".rcv_str = (" << sWord << ")";
		CBEContext::WriteMalloc(pFile, pFunction);
		pFile << "(";
		WriteMaxRefstringSize(pFile, pFunction, pMember, pParameter, nIndex);
		pFile << ");\n";

		pFile << "\t";
		WriteAccess(pFile, pFunction, nType, pMember);
		if (bUseArray)
			pFile << "[" << nIndex << "]";
		pFile << ".rcv_size = ";
		WriteMaxRefstringSize(pFile, pFunction, pMember, pParameter, nIndex);
		pFile << ";\n";
	}
}

/** \brief initializes the dopes to a value pair for short IPC
 *  \param pFile the file to write to
 *  \param pFunction the function for which to write the initialization
 *  \param nType the member to initialize
 *  \param nStructType the type of the message buffer struct
 *
 * \todo This is not X.2 conform (have map,grant items as well).
 */
void CL4FiascoBEMsgBuffer::WriteDopeShortInitialization(CBEFile& pFile, CBEFunction *pFunction,
	int nType, CMsgStructType nStructType)
{
	if ((nType != TYPE_MSGDOPE_SIZE) &&
		(nType != TYPE_MSGDOPE_SEND))
	{
		return;
	}
	CCompiler::Verbose("CL4FiascoBEMsgBuffer::WriteDopeShortInitialization called\n");

	// get name of member
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetMessageBufferMember(nType);
	// get member
	CBETypedDeclarator *pMember = FindMember(sName, pFunction, nStructType);
	if (!pMember)
		return;

	pFile << "\t";
	WriteAccess(pFile, pFunction, nStructType, pMember);
	// get short IPC values
	CL4BESizes *pSizes = static_cast<CL4BESizes*>(CCompiler::GetSizes());
	int nMinSize = pSizes->GetMaxShortIPCSize();
	int nWordSize = pSizes->GetSizeOfType(TYPE_MWORD);
	// set short IPC dope
	pFile << " = L4_IPC_DOPE(" << nMinSize / nWordSize << ", 0);\n";
}

/** \brief test if the message buffer has a certain property
 *  \param nProperty the property to test for
 *  \param nType the type of the message buffer struct
 *  \return true if property is available for direction
 *
 * This implementation checks the short IPC property.
 */
bool CL4FiascoBEMsgBuffer::HasProperty(int nProperty, CMsgStructType nType)
{
	if (nProperty == MSGBUF_PROP_UTCB_IPC)
		return IsUtcb();
	return CL4BEMsgBuffer::HasProperty(nProperty, nType);
}

/** \brief return if message buffer fits into UTCB
 *  \return true if it fits
 *
 * We use a cached value here, because otherwise the IsUtcb call would be
 * recursive and cause an endless loop. Thus, do the detection once in
 * PostCreate and then store it.
 */
bool CL4FiascoBEMsgBuffer::IsUtcb()
{
	CCompiler::Verbose("CL4FiascoBEMsgBuffer::IsUtcb() called for msgbuf @ %p\n",
		this);

	CBEType *pBaseType = CBETypedef::GetType();
	CBEUserDefinedType *pUserType = dynamic_cast<CBEUserDefinedType*>(pBaseType);
	if (pUserType)
	{
		CL4FiascoBEMsgBuffer *pMsgBuf = dynamic_cast<CL4FiascoBEMsgBuffer*>(
			FindTypedef(pUserType->GetName()));
		if (pMsgBuf)
			return pMsgBuf->IsUtcb();
	}

	CCompiler::Verbose("CL4FiascoBEMsgBuffer::IsUtcb: returns %s\n",
		m_bIsUtcb ? "true" : "false");
	return m_bIsUtcb;
}

/** \brief writes the initialization of the receive flexpage
 *  \param pFile the file to write to
 *  \param nType the type of the message buffer struct
 *
 * Only initialize receive window member with environment's receive flexpage,
 * if we really do receive a flexpage. This is a performance optimization: it
 * saves an indirect access via the environment parameter. For security
 * reasons, a mandatory, empty receive window is required.
 */
void CL4FiascoBEMsgBuffer::WriteRcvFlexpageInitialization(CBEFile& pFile, CMsgStructType nType)
{
	CCompiler::Verbose("CL4FiascoBEMsgBuffer::WriteRcvFlexpageInitialization(%s, %d) called\n",
		pFile.GetFileName().c_str(), (int)nType);
	if (IsUtcb())
		return;

	// get receive flexpage member
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sFlexName = pNF->GetMessageBufferMember(TYPE_RCV_FLEXPAGE);

	CBETypedDeclarator *pFlexpage = FindMember(sFlexName, nType);
	assert (pFlexpage);

	// get environment
	CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
	assert(pFunction);
	CBETypedDeclarator *pEnv = pFunction->GetEnvironment();
	assert (pEnv);
	string sEnv = pEnv->m_Declarators.First()->GetName();
	if (pEnv->m_Declarators.First()->GetStars() > 0)
		sEnv += "->";
	else
		sEnv += ".";

	// message buffer's receive window
	pFile << "\t";
	WriteAccess(pFile, pFunction, nType, pFlexpage);
	if (CMsgStructType::Generic != nType &&
		GetCount(pFunction, TYPE_FLEXPAGE, nType) == 0)
		pFile << ".raw = 0;\n";
	else
		pFile << " = " << sEnv << "rcv_fpage;\n";
}

/** \brief try to get the position of a member counting word sizes
 *  \param sName the name of the member
 *  \param nType the type of the message buffer struct
 *  \return the position (index) of the member, -1 if not found
 *
 * The position obtained here will be used as index into the word size member
 * array (_word). Because flexpage and send/size dope are not part of this
 * array we return -1 to indicate an error.
 */
int CL4FiascoBEMsgBuffer::GetMemberPosition(std::string sName, CMsgStructType nType)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	if (sName == pNF->GetMessageBufferMember(TYPE_RCV_FLEXPAGE))
		return -1;
	// test send dope
	if (sName == pNF->GetMessageBufferMember(TYPE_MSGDOPE_SEND))
		return -1;
	// test size dope
	if (sName == pNF->GetMessageBufferMember(TYPE_MSGDOPE_SIZE))
		return -1;

	// not found
	return CL4BEMsgBuffer::GetMemberPosition(sName, nType);
}

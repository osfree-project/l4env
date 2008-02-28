/**
 * \file   dice/src/be/l4/v4/L4V4BEMsgBuffer.cpp
 *  \brief  contains the implementation of the class CL4V4BEMsgBuffer
 *
 *  \date   06/15/2004
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#include "L4V4BEMsgBuffer.h"
#include "L4V4BENameFactory.h"
#include "be/BEInterfaceFunction.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEStructType.h"
#include "be/BEAttribute.h"
#include "be/BESizes.h"
#include "be/BEClass.h"
#include "be/BEContext.h"
#include "be/BEUserDefinedType.h"
#include "be/BEMsgBufferType.h"
#include "be/BEClassFactory.h"
#include "fe/FEOperation.h"
#include "Compiler.h"
#include "Error.h"
#include "Attribute-Type.h"
#include "TypeSpec-Type.h"
#include <cassert>

CL4V4BEMsgBuffer::CL4V4BEMsgBuffer()
: CL4BEMsgBuffer()
{ }

CL4V4BEMsgBuffer::CL4V4BEMsgBuffer(CL4V4BEMsgBuffer* src)
: CL4BEMsgBuffer(src)
{ }

/** destroys objects of this class */
CL4V4BEMsgBuffer::~CL4V4BEMsgBuffer()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CL4V4BEMsgBuffer* CL4V4BEMsgBuffer::Clone()
{
	return new CL4V4BEMsgBuffer(this);
}

/** \brief add platform specific members to specific struct
 *  \param pFunction the function of the message buffer
 *  \param pStruct the struct to add the members to
 *  \return true if successful
 *
 * We have to skip the V2 specific message buffer members. Instead we have to
 * add a message tag member ourselves, because we use the L4_MsgLoad and
 * L4_MsgStore functions to load and store from our local message buffer into
 * the message registers. This requires the message tag to be located at pos
 * 0.
 */
void CL4V4BEMsgBuffer::AddPlatformSpecificMembers(CBEFunction *pFunction, CBEStructType *pStruct)
{
	CL4BEMsgBuffer::AddPlatformSpecificMembers(pFunction, pStruct);

	// add message tag member
	AddMsgTagMember(pFunction, pStruct);
}

/** \brief sorts one struct in the message buffer \param pStruct the struct to
 * sort \return true if sorting was successful
 *
 * This implementation first moves the message tag up front and then calls the
 * base class' Sort method.  This order is important, because the Sort method
 * calls GetStartOfPayload, which in turn tries to skip the message tag.  Thus
 * it has to be at the begin of the message buffer before calling base class'
 * Sort.
 */
void CL4V4BEMsgBuffer::Sort(CBEStructType *pStruct)
{
	CCompiler::Verbose("CL4V4BEMsgBuffer::%s called for struct\n", __func__);

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
	pStruct->m_Members.Move(sName, 0);

	// sort payload
	CL4BEMsgBuffer::Sort(pStruct);

	// move message tag up front again
	pStruct->m_Members.Move(sName, 0);

	CCompiler::Verbose("CL4V4BEMsgBuffer::%s returns\n", __func__);
}

/** \brief checks if two members of a struct should be exchanged
 *  \param pFirst the first member
 *  \param pSecond the second member - first's successor
 *  \return true if exchange
 *
 * This methods overrides the L4 default behaviour to move flexpages up front.
 * Instead, they stay at the rear.
 */
bool CL4V4BEMsgBuffer::DoExchangeMembers(CBETypedDeclarator *pFirst, CBETypedDeclarator *pSecond)
{
	assert(pFirst);
	assert(pSecond);
	CCompiler::Verbose("CL4V4BEMsgBuffer::%s called with (%s, %s)\n", __func__,
		pFirst->m_Declarators.First()->GetName().c_str(),
		pSecond->m_Declarators.First()->GetName().c_str());

	int nFirstType = pFirst->GetType()->GetFEType();
	int nSecondType = pSecond->GetType()->GetFEType();

	// test flexpages
	if (TYPE_FLEXPAGE == nFirstType &&
		TYPE_FLEXPAGE != nSecondType)
		return true;
	if (TYPE_FLEXPAGE != nFirstType &&
		TYPE_FLEXPAGE == nSecondType)
		return false;
	if (TYPE_FLEXPAGE == nFirstType &&
		TYPE_FLEXPAGE == nSecondType)
		return false;

	return CL4BEMsgBuffer::DoExchangeMembers(pFirst, pSecond);
}

/** \brief writes the initialization of specific members
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param nType the type of the members to initialize
 *  \param nStructType the type of the message buffer struct
 */
void CL4V4BEMsgBuffer::WriteInitialization(CBEFile& pFile, CBEFunction *pFunction, int nType,
	CMsgStructType nStructType)
{
	if (nType != TYPE_MSGDOPE_SEND &&
		nType != TYPE_MSGDOPE_SIZE)
	{
		CL4BEMsgBuffer::WriteInitialization(pFile, pFunction, nType, nStructType);
		return;
	}

	/* The L4 V4 Message does not have a size dope, so we return here */
	if (nType == TYPE_MSGDOPE_SIZE)
		return;

	CCompiler::Verbose("CL4V4BEMsgBuffer::%s for func %s and dir %d\n",
		__func__, pFunction ? pFunction->GetName().c_str() : "(no func)", (int)nStructType);

	// if direction is 0 we have to get maximum of IN and OUT
	int nWords = 0;
	int nStrings = 0;
	if (CMsgStructType::Generic == nStructType)
	{
		int nWordsIn = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction, CMsgStructType::In, false);
		if (nWordsIn < 0)
			nWordsIn = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction, CMsgStructType::In, true);

		int nWordsOut = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction, CMsgStructType::Out, false);
		if (nWordsOut < 0)
			nWordsOut = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction, CMsgStructType::Out, true);

		CCompiler::Verbose("CL4V4BEMsgBuffer::%s words out: %d\n", __func__,
			nWordsOut);

		CCompiler::Verbose("CL4V4BEMsgBuffer::%s words in: %d words out: %d\n",
			__func__, nWordsIn, nWordsOut);

		if (nWordsIn >= 0 && nWordsOut >= 0)
			nWords = std::max(nWordsIn, nWordsOut);
		else
			nWords = std::min(nWordsIn, nWordsOut);

		CCompiler::Verbose("CL4V4BEMsgBuffer::%s words generic: %d\n", __func__,
			nWords);
	}
	else
	{
		nWords = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction, nStructType, false);

		CCompiler::Verbose("CL4V4BEMsgBuffer::%s words dir %d: %d\n", __func__,
			(int)nStructType, nWords);

	}
	nStrings = GetCount(pFunction, TYPE_REFSTRING, CMsgStructType::Generic);
	CBESizes *pSizes = CCompiler::GetSizes();
	nWords = pSizes->WordsFromBytes(pSizes->WordRoundUp(nWords));
	// check minimum number of words
	int nMinWords = GetWordMemberCountFunction();
	if (nWords >= 0)
		nWords = std::max(nWords, nMinWords);

	CCompiler::Verbose("CL4V4BEMsgBuffer::%s words (min): %d\n", __func__, nWords);

	// get name of member
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);

	pFile << "\t";
	WriteAccessToStruct(pFile, pFunction, nStructType);
	pFile << "." << sName << ".X.u = ";
	if (nWords >= 0)
		pFile << nWords;
	else // variable sized elements
	{
		int nRefSize = pSizes->WordsFromBytes(pSizes->GetSizeOfType(TYPE_REFSTRING));
		// sizeof(<msgbufvar>.<structname>)/sizeof(long)-3
		pFile << "sizeof(";
		WriteAccessToStruct(pFile, pFunction, nStructType);
		pFile << ")/sizeof(long)-";
		pFile << 3 + nStrings*nRefSize;
	}
	pFile << ";\n";
	pFile << "\t";
	WriteAccessToStruct(pFile, pFunction, nStructType);
	pFile << "." << sName << ".X.t = " << nStrings << ";\n";
}

/** \brief writes the initialization of the receive flexpage
 *  \param pFile the file to write to
 *  \param nType the type of the message buffer struct
 */
void CL4V4BEMsgBuffer::WriteRcvFlexpageInitialization(CBEFile& pFile, CMsgStructType /*nType*/)
{
	// get environment
	CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
	CBETypedDeclarator *pEnv = pFunction->GetEnvironment();
	assert (pEnv);
	string sEnv = pEnv->m_Declarators.First()->GetName();
	if (pEnv->m_Declarators.First()->GetStars() > 0)
		sEnv += "->";
	else
		sEnv += ".";

	// assign environment's flexpage to message buffer's flexpage
	pFile << "\tL4_Accept( L4_MapGrantItems( " << sEnv << "rcv_fpage ) );\n";
}

/** \brief adds platform specific opcode member
 *  \param pFunction the function to add the members for
 *  \param pStruct the struct to add to
 *  \return true if successful
 *
 * The opcode member is transmitted in the message tag. Do not create an
 * opcode member.
 *
 * However, we do create an exception member, because we might transmit
 * complex exceptions.
 */
void CL4V4BEMsgBuffer::AddOpcodeMember(CBEFunction* /*pFunction*/, CBEStructType* /*pStruct*/)
{ }

/** \brief adds platform specific msgtag member
 *  \param pFunction the function to add the members for
 *  \param pStruct the struct to add to
 *  \return true if successful
 */
void CL4V4BEMsgBuffer::AddMsgTagMember(CBEFunction* /*pFunction*/, CBEStructType *pStruct)
{
	CBETypedDeclarator *pMsgTag = GetMsgTagVariable();
	pStruct->m_Members.Add(pMsgTag);
}

/** \brief return the offset where the payload starts
 *  \return the offset in bytes
 *
 * On V4 we do only have the message tag that is not payload, so skip offset
 * is msgtag size.
 */
int CL4V4BEMsgBuffer::GetPayloadOffset()
{
	CCompiler::Verbose("CL4V4BEMsgBuffer::%s called\n", __func__);
	CBESizes *pSizes = CCompiler::GetSizes();
	return pSizes->GetSizeOfType(TYPE_MSGTAG);
}

/** \brief creates an opcode variable for the message buffer
 *  \return a reference to the newly created opcode member
 */
CBETypedDeclarator* CL4V4BEMsgBuffer::GetMsgTagVariable()
{
	// get name
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
	string sType = pNF->GetTypeName(TYPE_MSGTAG, false);
	// create var
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBETypedDeclarator *pMsgTag = pCF->GetNewTypedDeclarator();
	pMsgTag->SetParent(this);
	pMsgTag->CreateBackEnd(sType, sName, 0);
	// add directional attribute, so the test if this should be marshalled
	// will work
	CBEAttribute *pAttr = pCF->GetNewAttribute();
	pAttr->SetParent(pMsgTag);
	pAttr->CreateBackEnd(ATTR_OUT);
	pMsgTag->m_Attributes.Add(pAttr);
	// now IN
	pAttr = pCF->GetNewAttribute();
	pAttr->SetParent(pMsgTag);
	pAttr->CreateBackEnd(ATTR_IN);
	pMsgTag->m_Attributes.Add(pAttr);

	return pMsgTag;
}

/** \brief writes the initialisation of a refstring member with the init func
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pClass the class to write for
 *  \param nIndex the index of the refstring in an array
 *  \param nType the type of the message buffer struct
 *  \return true if we wrote something, false if not
 */
bool CL4V4BEMsgBuffer::WriteRefstringInitFunction(CBEFile& pFile, CBEFunction *pFunction, CBEClass *pClass,
	int nIndex, CMsgStructType nType)
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

		// store the string length in the temporary string length variable
		string sVarName = pNF->GetString(CL4V4BENameFactory::STR_INIT_RCVSTR_VARIABLE, 0);
		pFile << "\t" << sVarName << " = ";
		WriteMemberAccess(pFile, pFunction, nType, TYPE_REFSTRING,
			nIndex);
		pFile << ".X.string_length;\n";
		// call the init function for the indirect string
		string sMWord = pNF->GetTypeName(TYPE_MWORD, false);
		CBETypedDeclarator *pEnvVar = pFunction->GetEnvironment();
		CBEDeclarator *pEnv = pEnvVar->m_Declarators.First();
		pFile << "\t" << sFunction << " ( " << nIndex << ", (" << sMWord << "*)&(";
		WriteMemberAccess(pFile, pFunction, nType, TYPE_REFSTRING,
			nIndex);
		pFile << ".X.str.string_ptr), &" << sVarName << ", " << pEnv->GetName() << ");\n";
		// store returned size back in string dope
		pFile << "\t";
		WriteMemberAccess(pFile, pFunction, nType, TYPE_REFSTRING, nIndex);
		pFile << ".X.string_length = " << sVarName << ";\n";
		// load into respective buffer register
		pFile << "\tL4_LoadBRs (" << (nIndex*2+1) << ", 2, (" << sMWord << "*)&(";
		WriteMemberAccess(pFile, pFunction, nType, TYPE_REFSTRING, nIndex);
		pFile << "));\n";

		// OK, next!
		return true;
	}
	return false;
}


/** \brief write the initialization of refstring
 *  \param pFile the file to write to
 *  \param nType the type of the message buffer struct
 *  \return true if we actually wrote for
 *
 * Here, we call the base class to write the refstring initialization, but
 * have to set the "we want to receive refstrings" bit afterwards.
 */
bool CL4V4BEMsgBuffer::WriteRefstringInitialization(CBEFile& pFile, CMsgStructType nType)
{
	bool bRefstring = CL4BEMsgBuffer::WriteRefstringInitialization(pFile, nType);
	if (bRefstring)
	{
		CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
		if (!pFunction)
			return false;
		pFile << "\t{\n";
		++pFile << "\tL4_Acceptor_t a = L4_Accepted();\n";
		pFile << "\ta.raw |= L4_StringItemsAcceptor.raw;\n";
		pFile << "\tL4_Accept (a);\n";
		--pFile << "\t}\n";
	}
	return bRefstring;
}

/** \brief writes the refstring member init for given parameter
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pMember the member to write
 *  \param nIndex the index of the refstring to initialize
 *  \param nType the type of the message buffer struct
 */
void CL4V4BEMsgBuffer::WriteRefstringInitParameter(CBEFile& pFile, CBEFunction *pFunction,
	CBETypedDeclarator *pMember, int nIndex, CMsgStructType nType)
{
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
		pFile << " = L4_StringItem (";
		// size
		CBEType *pType = pParameter->GetType();
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
		else
			// write max-is
			pParameter->WriteGetSize(pFile, 0, pFunction);
		pFile << ", (void *)(";
		// address
		CBEDeclarator *pDecl = pParameter->m_Declarators.First();
		int nStars = pDecl->GetStars();
		if (!pType->IsPointerType())
			nStars--;
		for (; nStars > 0; nStars--)
			pFile << "*";
		pFile << pDecl->GetName() << ") );\n";
	}
	else
	{
		bool bUseArray = pMember->m_Declarators.First()->IsArray() &&
			pMember->m_Declarators.First()->GetArrayDimensionCount() > 0;
		pFile << "\t";
		WriteAccess(pFile, pFunction, nType, pMember);
		if (bUseArray)
			pFile << "[" << nIndex << "]";
		pFile << ".X.str.string_ptr = ";
		CBEContext::WriteMalloc(pFile, pFunction);
		pFile << "(";
		WriteMaxRefstringSize(pFile, pFunction, pMember, pParameter, nIndex);
		pFile << ");\n";

		pFile << "\t";
		WriteAccess(pFile, pFunction, nType, pMember);
		if (bUseArray)
			pFile << "[" << nIndex << "]";
		pFile << ".X.string_length = ";
		WriteMaxRefstringSize(pFile, pFunction, pMember, pParameter, nIndex);
		pFile << ";\n";

		// load into respective buffer register
		CBENameFactory *pNF = CBENameFactory::Instance();
		string sMWord = pNF->GetTypeName(TYPE_MWORD, false);
		pFile << "\tL4_LoadBRs (" << (nIndex*2+1) << ", 2, (" << sMWord << "*)&(";
		WriteAccess(pFile, pFunction, nType, pMember);
		if (bUseArray)
			pFile << "[" << nIndex << "]";
		pFile << "));\n";
	}
}

/** \brief the post-create (and post-sort) step during creation
 *  \param pFunction the function owning this message buffer
 *  \param pFEOperation the front-end reference operation
 *  \return true if successful
 *
 * For V4 we have to be aware of the maximum size of the message buffer and
 * have to change variable sized members into indirect parts if the message
 * buffer is bigger than possible.
 *
 * We call the base class after the replacement, because the base class adds
 * the generic struct (that should contain the new refstrings) and will call
 * the Pad method.
 */
void CL4V4BEMsgBuffer::PostCreate(CBEFunction *pFunction, CFEOperation *pFEOperation)
{
	CCompiler::Verbose("CL4V4BEMsgBuffer::%s(function %s) called\n", __func__,
		pFunction->GetName().c_str());

	int nMaxSize = CCompiler::GetSizes()->GetMaxSizeOfType(TYPE_MESSAGE), nSize;
	CBEStructType *pStruct = GetStruct(CMsgStructType::In);
	if (pStruct)
	{
		CheckConvertStruct(pStruct, pFunction);
		GetMaxSize(nSize, pFunction, CMsgStructType::In);
		CCompiler::Verbose("CL4V4BEMsgBuffer::%s: Check size: %d <= %d?\n", __func__,
			nSize, nMaxSize);
		assert(nSize <= nMaxSize);
	}
	pStruct = GetStruct(CMsgStructType::Out);
	if (pStruct)
	{
		CheckConvertStruct(pStruct, pFunction);
		GetMaxSize(nSize, pFunction, CMsgStructType::Out);
		CCompiler::Verbose("CL4V4BEMsgBuffer::%s: Check size: %d <= %d?\n", __func__,
			nSize, nMaxSize);
		assert(nSize <= nMaxSize);
	}
	pStruct = GetStruct(CMsgStructType::Exc);
	if (pStruct)
	{
		CheckConvertStruct(pStruct, pFunction);
		GetMaxSize(nSize, pFunction, CMsgStructType::Exc);
		CCompiler::Verbose("CL4V4BEMsgBuffer::%s: Check size: %d <= %d?\n", __func__,
			nSize, nMaxSize);
		assert(nSize <= nMaxSize);
	}

	// call base class
	CL4BEMsgBuffer::PostCreate(pFunction, pFEOperation);

	// resort structs
	pStruct = GetStruct(CMsgStructType::In);
	if (pStruct)
		Sort(pStruct);
	pStruct = GetStruct(CMsgStructType::Out);
	if (pStruct)
		Sort(pStruct);
	pStruct = GetStruct(CMsgStructType::Exc);
	if (pStruct)
		Sort(pStruct);

	CCompiler::Verbose("CL4V4BEMsgBuffer::%s(function %s) finished\n", __func__,
		pFunction->GetName().c_str());
}

/** \brief pads refstring to specified position
 *  \param pStruct the struct to pad the refstrings in
 *  \param nPosition the position to align refstring to
 *  \return false if an error occured
 *
 * Receive restrings should be padded such, that an incoming message does not
 * overwrites them, that is: behind the message buffer (UTCB).
 */
void CL4V4BEMsgBuffer::PadRefstringToPosition(CBEStructType *pStruct, int nPosition)
{
	CCompiler::Verbose("CL4V4BEMsgBuffer::%s() called\n", __func__);
	assert(pStruct);
	// if struct for send direction, call base class.
	if (CMsgStructType::In == GetStructType(pStruct))
	{
		CL4BEMsgBuffer::PadRefstringToPosition(pStruct, nPosition);
		return;
	}

	// if struct for receive or generic direction, get maximum word value and
	// use that to pad
	CBESizes *pSizes = CCompiler::GetSizes();
	int nMaxSize = pSizes->GetMaxSizeOfType(TYPE_MESSAGE);
	CL4BEMsgBuffer::PadRefstringToPosition(pStruct, nMaxSize);
}

/** \brief the post-create (and post-sort) step during creation
 *  \param pClass the class owning this message buffer
 *  \param pFEInterface the front-end reference interface
 *  \return true if successful
 *
 * For V4 we have to be aware of the maximum size of the message buffer and
 * have to change variable sized members into indirect parts if the message
 * buffer is bigger than possible.
 *
 * We call the base class after the replacement, because the base class adds
 * the generic struct (that should contain the new refstrings) and will call
 * the Pad method.
 */
void CL4V4BEMsgBuffer::PostCreate(CBEClass *pClass, CFEInterface *pFEInterface)
{
	assert(pClass);

	CCompiler::Verbose("CL4V4BEMsgBuffer::%s(class %s) called\n", __func__,
		pClass->GetName().c_str());

	CBEMsgBufferType *pMsgType = GetType(0);
	// iterate the structures and test each
	string sClassName = pClass->GetName();
	vector<CFunctionGroup*>::iterator i;
	for (i = pClass->m_FunctionGroups.begin();
		i != pClass->m_FunctionGroups.end();
		i++)
	{
		string sFuncName = (*i)->GetName();
		CBEStructType *pStruct = pMsgType->GetStruct(sFuncName, sClassName, CMsgStructType::In);
		if (pStruct)
			CheckConvertStruct(pStruct, 0);
		pStruct = pMsgType->GetStruct(sFuncName, sClassName, CMsgStructType::Out);
		if (pStruct)
			CheckConvertStruct(pStruct, 0);
		pStruct = pMsgType->GetStruct(sFuncName, sClassName, CMsgStructType::Exc);
		if (pStruct)
			CheckConvertStruct(pStruct, 0);
	}
	// call base class
	CL4BEMsgBuffer::PostCreate(pClass, pFEInterface);

	// resort structs
	for (i = pClass->m_FunctionGroups.begin();
		i != pClass->m_FunctionGroups.end();
		i++)
	{
		string sFuncName = (*i)->GetName();
		CBEStructType *pStruct = pMsgType->GetStruct(sFuncName, sClassName, CMsgStructType::In);
		if (pStruct)
			Sort(pStruct);
		pStruct = pMsgType->GetStruct(sFuncName, sClassName, CMsgStructType::Out);
		if (pStruct)
			Sort(pStruct);
		pStruct = pMsgType->GetStruct(sFuncName, sClassName, CMsgStructType::Exc);
		if (pStruct)
			Sort(pStruct);
	}

	CCompiler::Verbose("CL4V4BEMsgBuffer::%s(class %s) finished\n", __func__,
		pClass->GetName().c_str());

}

/** \brief calculate the number of word sized members required for generic server message buffer
 *  \return the number of word sized members
 *
 * For V4 we return the maximum possible size of words.  This is, because we
 * use the generic part for unmarshalling purposes only.  Also, we allocate
 * the buffer registers in the strings in the generic part and use them when
 * unmarshalling.  Therefore, we have to "protect" the string elements by
 * moving them behind the region that can be overwritten by the kernel.
 */
int CL4V4BEMsgBuffer::GetWordMemberCountClass()
{
	CCompiler::Verbose("CL4BEMsgBuffer::%s() called\n", __func__);
	CBESizes *pSizes = CCompiler::GetSizes();
	int nMaxSize = pSizes->GetMaxSizeOfType(TYPE_MESSAGE);
	return pSizes->WordsFromBytes(nMaxSize);
}

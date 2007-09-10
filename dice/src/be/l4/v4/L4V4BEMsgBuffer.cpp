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
#include "be/l4/TypeSpec-L4Types.h"
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
CObject* CL4V4BEMsgBuffer::Clone()
{
	return new CL4V4BEMsgBuffer(this);
}

/** \brief add platform specific members to specific struct
 *  \param pFunction the function of the message buffer
 *  \param pStruct the struct to add the members to
 *  \param nType the type of the message buffer struct
 *  \return true if successful
 *
 * We have to skip the V2 specific message buffer members. Instead we have to
 * add a message tag member ourselves, because we use the L4_MsgLoad and
 * L4_MsgStore functions to load and store from our local message buffer into
 * the message registers. This requires the message tag to be located at pos
 * 0.
 */
void CL4V4BEMsgBuffer::AddPlatformSpecificMembers(CBEFunction *pFunction, CBEStructType *pStruct,
    CMsgStructType nType)
{
	CL4BEMsgBuffer::AddPlatformSpecificMembers(pFunction, pStruct, nType);

	// add message tag member
	AddMsgTagMember(pFunction, pStruct, nType);
}

/** \brief sorts one struct in the message buffer
 *  \param pStruct the struct to sort
 *  \return true if sorting was successful
 *
 * Do sort the message tag up front.
 */
void CL4V4BEMsgBuffer::Sort(CBEStructType *pStruct)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"%s called for struct\n", __func__);

	// sort payload
	CL4BEMsgBuffer::Sort(pStruct);

	CBENameFactory *pNF = CBENameFactory::Instance();
	// find msgtag
	string sName = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
	pStruct->m_Members.Move(sName, 0);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns\n", __func__);
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
		CL4BEMsgBuffer::WriteInitialization(pFile, pFunction, nType,
			nStructType);
		return;
	}

	/* The L4 Message does not have a size dope, so we return here */
	if (nType == TYPE_MSGDOPE_SIZE)
		return;

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s for func %s and dir %d\n",
		__func__, pFunction ? pFunction->GetName().c_str() : "(no func)", (int)nStructType);

	// if direction is 0 we have to get maximum of IN and OUT
	int nWords = 0;
	int nStrings = 0;
	CBESizes *pSizes = CCompiler::GetSizes();
	int nRefSize = pSizes->GetSizeOfType(TYPE_REFSTRING) /
		pSizes->GetSizeOfType(TYPE_MWORD);
	if (CMsgStructType::Generic == nStructType)
	{
		int nWordsIn = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction,
			CMsgStructType::In, false);

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s words in: %d\n", __func__,
			nWordsIn);

		if (nWordsIn < 0)
			nWordsIn = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction,
				CMsgStructType::In, true);

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s words in: %d\n", __func__,
			nWordsIn);

		int nWordsOut = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction,
			CMsgStructType::Out, false);

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s words out: %d\n", __func__,
			nWordsOut);

		if (nWordsOut < 0)
			nWordsOut = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction,
				CMsgStructType::Out, true);

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s words out: %d\n", __func__,
			nWordsOut);

		// for generic struct we also have to subtract the strings from the
		// word count, because the strings have been counted when counting
		// TYPE_MWORD as well.
		int nStringsIn = CBEMsgBuffer::GetMemberSize(TYPE_REFSTRING,
			pFunction, CMsgStructType::In, false);

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s strings in: %d\n", __func__,
			nStringsIn);

		int nStringsOut = CBEMsgBuffer::GetMemberSize(TYPE_REFSTRING,
			pFunction, CMsgStructType::Out, false);

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s strings out: %d\n", __func__,
			nStringsOut);

		nStrings = std::max(nStringsIn, nStringsOut);

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s strings: %d\n", __func__,
			nStrings);
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s words in: %d words out: %d\n",
			__func__, nWordsIn, nWordsOut);

		if (nWordsIn >= 0 && nWordsOut >= 0)
			nWords = std::max(nWordsIn, nWordsOut);
		else
			nWords = std::min(nWordsIn, nWordsOut);

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s words: %d\n", __func__, nWords);
	}
	else
	{
		nWords = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction,
			nStructType, false);

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s words: %d\n", __func__, nWords);

		nStrings = CBEMsgBuffer::GetMemberSize(TYPE_REFSTRING, pFunction,
			nStructType, false);

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s strings: %d\n", __func__,
			nStrings);
	}
	// check minimum number of words
	int nMinWords = GetWordMemberCountFunction();
	if (nWords >= 0)
		nWords = std::max(nWords, nMinWords);

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s words (min): %d\n", __func__,
		nWords);

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
 *  \param nType the type of the message buffer struct
 *  \return true if successful
 *
 * The opcode member is transmitted in the message tag. Do not create an
 * opcode member.
 *
 * However, we do create an exception member, because we might transmit
 * complex exceptions.
 */
void CL4V4BEMsgBuffer::AddOpcodeMember(CBEFunction* /*pFunction*/, CBEStructType* /*pStruct*/,
    CMsgStructType /*nType*/)
{ }

/** \brief adds platform specific msgtag member
 *  \param pFunction the function to add the members for
 *  \param pStruct the struct to add to
 *  \param nType the type of the message buffer struct
 *  \return true if successful
 */
void CL4V4BEMsgBuffer::AddMsgTagMember(CBEFunction* /*pFunction*/, CBEStructType *pStruct,
    CMsgStructType /*nType*/)
{
	CBETypedDeclarator *pMsgTag = GetMsgTagVariable();
	if (!pMsgTag)
		throw error::create_error("message tag could not be created");
	// check if there already is a member with that name
	if (pStruct->m_Members.Find(pMsgTag->m_Declarators.First()->GetName()))
		delete pMsgTag;
	else
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
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
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
bool
CL4V4BEMsgBuffer::WriteRefstringInitFunction(CBEFile& pFile,
    CBEFunction *pFunction,
    CBEClass *pClass,
    int nIndex,
    CMsgStructType nType)
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
		pFile << " = L4_StringItem /* " << __func__ << " */ (";
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
	CBEStructType *pStruct = GetStruct(CMsgStructType::In);
	assert(pStruct);
	CheckConvertStruct(pStruct);

	pStruct = GetStruct(CMsgStructType::Out);
	assert(pStruct);
	CheckConvertStruct(pStruct);

	int nMaxSize = CCompiler::GetSizes()->GetMaxSizeOfType(TYPE_MESSAGE);
	int nSize = 0;
	GetMaxSize(nSize);
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "Check size: %d <= %d?\n",
		nSize, nMaxSize);
	assert(nSize <= nMaxSize);

	// call base class
	CL4BEMsgBuffer::PostCreate(pFunction, pFEOperation);

	// resort structs
	Sort(GetStruct(CMsgStructType::In));
	Sort(GetStruct(CMsgStructType::Out));
}

/** \brief pads refstring to specified position
 *  \param pStruct the struct to pad the refstrings in
 *  \param nPosition the position to align refstring to
 *  \return false if an error occured
 *
 * Receive restrings should be padded such, that an incoming message does not
 * overwrites them, that is: behind the message buffer (UTCB).
 */
bool CL4V4BEMsgBuffer::PadRefstringToPosition(CBEStructType *pStruct, int nPosition)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4V4BEMsgBuffer::%s() called\n", __func__);
	assert(pStruct);
	// if struct for send direction, call base class.
	if (GetStructType(pStruct) == CMsgStructType::In)
		return CL4BEMsgBuffer::PadRefstringToPosition(pStruct, nPosition);

	// if struct for receive or generic direction, get maximum word value and
	// use that to pad
	CBESizes *pSizes = CCompiler::GetSizes();
	int nMaxSize = pSizes->GetMaxSizeOfType(TYPE_MESSAGE);
	return CL4BEMsgBuffer::PadRefstringToPosition(pStruct, nMaxSize);
}

/** \brief check whether a member of the specified struct has to be converted
 *  \param pStruct the struct to check for convertable members
 */
void CL4V4BEMsgBuffer::CheckConvertStruct(CBEStructType *pStruct)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CL4V4BEMsgBuffer::%s(%p) struct %s called\n",
		__func__, pStruct, pStruct->GetTag().c_str());

	CBEMsgBufferType *pMsgType = pStruct->GetSpecificParent<CBEMsgBufferType>();
	assert(pMsgType);

	int nMaxSize = CCompiler::GetSizes()->GetMaxSizeOfType(TYPE_MESSAGE);
	int nSize = pStruct->GetMaxSize();
	bool bConverted = true;

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"\n\nCL4V4BEMsgBuffer::%s checking (nSize %d, nMaxSize %d)\n", __func__,
		nSize, nMaxSize);

	while ((nSize > nMaxSize) && bConverted)
	{
		// iterate members and try to find variable sized member for IN struct
		CBETypedDeclarator *pMember;
		bConverted = false;

		if ((pMember = CheckConvertMember(pStruct,
					pMsgType->GetStartOfPayload(pStruct))))
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
				"CL4V4BEMsgBuffer::%s Convert member %s\n", __func__,
				pMember->m_Declarators.First()->GetName().c_str());
			ConvertMember(pMember);
			bConverted = true;
		}

		nSize = pStruct->GetMaxSize();

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
			"\n\nCL4V4BEMsgBuffer::%s checking in (nSize %d, nMaxSize %d)\n", __func__,
			nSize, nMaxSize);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CL4V4BEMsgBuffer::%s finished (nSize %d, nMaxSize %d) struct %s\n", __func__,
		nSize, nMaxSize, pStruct->GetTag().c_str());
}

/** \brief check if members of the struct can be converted.
 *  \param pStruct struct to check
 *  \param iter the iterator to start the search at
 *  \return the member to convert
 */
CBETypedDeclarator*
CL4V4BEMsgBuffer::CheckConvertMember(CBEStructType *pStruct,
    vector<CBETypedDeclarator*>::iterator iter)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CL4V4BEMsgBuffer::%s struct %s called\n",
	__func__, pStruct->GetTag().c_str());

    int nMaxSize = CCompiler::GetSizes()->GetMaxSizeOfType(TYPE_MESSAGE);
    CBETypedDeclarator *pBiggestMember = 0;
    for (; iter != pStruct->m_Members.end(); iter++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	    "CL4V4BEMsgBuffer::%s checking %s\n", __func__,
	    (*iter)->m_Declarators.First()->GetName().c_str());
	// skip refstrings. We can't make them "better"
	if ((*iter)->GetType()->IsOfType(TYPE_REFSTRING))
	    continue;
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	    "CL4V4BEMsgBuffer::%s size: %d, max: %d\n", __func__,
	    (*iter)->GetSize(), nMaxSize);
	// if the member itself is bigger than the message size then it should
	// be converted
	if ((*iter)->GetSize() > nMaxSize)
	{
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CL4V4BEMsgBuffer::%s size of member bigger, then max, return\n", __func__);
	    return *iter;
	}
	// if member is variable sized then it should be converted
	if ((*iter)->GetSize() < 0)
	{
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CL4V4BEMsgBuffer::%s member variable sized, return\n", __func__);
	    return *iter;
	}
	// try to find biggest member.
	if (pBiggestMember && (pBiggestMember->GetSize() < (*iter)->GetSize()))
	    pBiggestMember = *iter;
	if (!pBiggestMember)
	    pBiggestMember = *iter;
    }
    if (pBiggestMember && (pBiggestMember->GetSize() >
	    CCompiler::GetSizes()->GetSizeOfType(TYPE_REFSTRING)))
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	    "CL4V4BEMsgBuffer::%s biggest member is %s\n", __func__,
	    pBiggestMember->m_Declarators.First()->GetName().c_str());
	return pBiggestMember;
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CL4V4BEMsgBuffer::%s nothing found\n",
	__func__);
    return 0;
}

/** \brief convert the member into an indirect part
 *  \param pMember the member to convert
 *
 * We make a refstring parameter out of the member
 */
void
CL4V4BEMsgBuffer::ConvertMember(CBETypedDeclarator* pMember)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CL4V4BEMsgBuffer::%s(%s) called\n", __func__,
	pMember->m_Declarators.First()->GetName().c_str());

    CBEClassFactory *pCF = CBEClassFactory::Instance();
    CBEType *pType = pCF->GetNewType(TYPE_REFSTRING);
    pType->CreateBackEnd(true, 0, TYPE_REFSTRING);
    pMember->ReplaceType(pType);
    // set the pointer of the declarator to zero
    CBEDeclarator *pDecl = pMember->m_Declarators.First();
    pDecl->SetStars(0);
    // check for array dimensions and remove if necessary
    if (pDecl->IsArray())
    {
	// replace array dimensions with max-is attribute
	if (!pMember->m_Attributes.Find(ATTR_MAX_IS))
	{
	    CBEAttribute *pAttr = pCF->GetNewAttribute();
	    pMember->m_Attributes.Add(pAttr);
	    int nSize = 0;
	    pMember->GetMaxSize(nSize);
	    pAttr->CreateBackEndInt(ATTR_MAX_IS, nSize);
	}
	// remove array dimensions
	while (!pDecl->m_Bounds.empty())
	    pDecl->RemoveArrayBound(*(pDecl->m_Bounds.begin()));
    }
    // add the ref attribute to catch all tests
    CBEAttribute *pAttr = pCF->GetNewAttribute();
    pMember->m_Attributes.Add(pAttr);
    pAttr->CreateBackEnd(ATTR_REF);
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CL4V4BEMsgBuffer::%s added [ref] to member %s (%p)\n", __func__,
	pMember->m_Declarators.First()->GetName().c_str(), pMember);
    // add C language property to avoid const qualifier in struct
    pMember->AddLanguageProperty(string("noconst"), string());

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CL4V4BEMsgBuffer::%s %s's type is now %d\n", __func__,
	pMember->m_Declarators.First()->GetName().c_str(),
	pMember->GetType()->GetFEType());

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CL4V4BEMsgBuffer::%s returns\n", __func__);
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

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CL4V4BEMsgBuffer::%s(class %s) called\n", __func__,
		pClass->GetName().c_str());

	CBEUserDefinedType *pUsrType = dynamic_cast<CBEUserDefinedType*>(GetType());
	CBEMsgBufferType *pMsgType = 0;
	if (pUsrType)
		pMsgType = dynamic_cast<CBEMsgBufferType*>(pUsrType->GetRealType());
	else
		pMsgType = dynamic_cast<CBEMsgBufferType*>(GetType());
	assert(pMsgType);
	// iterate the structures and test each
	string sClassName = pClass->GetName();
	vector<CFunctionGroup*>::iterator i;
	for (i = pClass->m_FunctionGroups.begin();
		i != pClass->m_FunctionGroups.end();
		i++)
	{
		string sFuncName = (*i)->GetName();
		if (!(*i)->GetOperation()->m_Attributes.Find(ATTR_OUT))
		{
			CBEStructType *pStruct = pMsgType->GetStruct(sFuncName, sClassName,
				CMsgStructType::In);
			CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
				"CL4V4BEMsgBuffer::%s struct for (%s, %s, IN) at %p\n", __func__,
				sFuncName.c_str(), sClassName.c_str(), pStruct);
			assert(pStruct);
			CheckConvertStruct(pStruct);
		}
		if (!(*i)->GetOperation()->m_Attributes.Find(ATTR_IN))
		{
			CBEStructType *pStruct = pMsgType->GetStruct(sFuncName, sClassName,
				CMsgStructType::Out);
			CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
				"CL4V4BEMsgBuffer::%s struct for (%s, %s, OUT) at %p\n", __func__,
				sFuncName.c_str(), sClassName.c_str(), pStruct);
			assert(pStruct);
			CheckConvertStruct(pStruct);
		}
	}
	// call base class
	CL4BEMsgBuffer::PostCreate(pClass, pFEInterface);

	// resort structs
	for (i = pClass->m_FunctionGroups.begin();
		 i != pClass->m_FunctionGroups.end();
		 i++)
	{
		string sFuncName = (*i)->GetName();
		if (!(*i)->GetOperation()->m_Attributes.Find(ATTR_OUT))
		{
			CBEStructType *pStruct = pMsgType->GetStruct(sFuncName, sClassName,
				CMsgStructType::In);
			CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
				"CL4V4BEMsgBuffer::%s struct for (%s, %s, IN) at %p\n", __func__,
				sFuncName.c_str(), sClassName.c_str(), pStruct);
			assert(pStruct);
			Sort(pStruct);
		}
		if (!(*i)->GetOperation()->m_Attributes.Find(ATTR_IN))
		{
			CBEStructType *pStruct = pMsgType->GetStruct(sFuncName, sClassName,
				CMsgStructType::Out);
			CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
				"CL4V4BEMsgBuffer::%s struct for (%s, %s, OUT) at %p\n", __func__,
				sFuncName.c_str(), sClassName.c_str(), pStruct);
			assert(pStruct);
			Sort(pStruct);
		}
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CL4V4BEMsgBuffer::%s finished\n",
		__func__);
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
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CL4BEMsgBuffer::%s() called\n", __func__);
	CBESizes *pSizes = CCompiler::GetSizes();
	int nMaxSize = pSizes->GetMaxSizeOfType(TYPE_MESSAGE);
	return pSizes->WordsFromBytes(nMaxSize);
}

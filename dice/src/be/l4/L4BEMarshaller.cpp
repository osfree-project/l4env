/**
 *    \file    dice/src/be/l4/L4BEMarshaller.cpp
 *    \brief   contains the implementation of the class CL4BEMarshaller
 *
 *    \date    01/26/2004
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "L4BEMarshaller.h"
#include "L4BESizes.h"
#include "L4BENameFactory.h"
#include "L4BEIPC.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEStructType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEMsgBuffer.h"
#include "be/BEMsgBufferType.h"
#include "be/BEFunction.h"
#include "be/BECallFunction.h"
#include "be/BESndFunction.h"
#include "be/BEReplyFunction.h"
#include "be/BEWaitFunction.h"
#include "be/BEDeclarator.h"
#include "be/BEExpression.h"
#include "be/BEClassFactory.h"
#include "TypeSpec-L4Types.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include "Messages.h"
#include <cassert>
#include <sstream>
using std::ostringstream;

CL4BEMarshaller::CL4BEMarshaller()
: CBEMarshaller()
{ }

/** destroys the object */
CL4BEMarshaller::~CL4BEMarshaller()
{ }

/** \brief write the marshaling code for a whole function
 *  \param pFile the file to write to
 *  \param pFunction the function to write the marshaling code for
 *  \param nDirection the direction to marshal
 *
 * This method initializes some internal counters, then calls the base class'
 * implementation.
 */
void
CL4BEMarshaller::MarshalFunction(CBEFile& pFile,
	CBEFunction *pFunction,
	DIRECTION_TYPE nDirection)
{
	m_nSkipSize = 0;
	CBEMarshaller::MarshalFunction(pFile, pFunction, nDirection);
}

/** \brief tests if this parameter should be marshalled
 *  \param pFunction the function the parameter belongs to
 *  \param pParameter the parameter (!) to be tested
 *  \param nDirection the direction of the marshalling
 *  \return true if this parameter should be skipped
 *
 * Additionally to the base class' tests this method skips the number of
 * parameters used in the IPC binding. The exception member is called
 * explicetly for marshalling/unmarshalling, thus test here for exception and
 * return always true.
 *
 * To watch:
 * - the special members, such as opcode and exception, since they are not
 * parameters and therefore this method is not called for them (their size
 * should have been added in MarshalFunction method).
 * - The ASM bindings might require a somewhat different strategy, because
 * they might assume parameters are all marshalled.
 * - We have to be aware that members reaching beyond that boundary have to be
 * marshalled, so the part beyond the boundary is in the message buffer.
 */
bool
CL4BEMarshaller::DoSkipParameter(CBEFunction *pFunction,
	CBETypedDeclarator *pParameter,
	DIRECTION_TYPE nDirection)
{
	// first test base class
	if (CBEMarshaller::DoSkipParameter(pFunction, pParameter, nDirection))
	{
		// because this parameter is skipped, we have to add its size to the
		// m_nSkipSize member. Otherwise we ignore some members of struct.
		m_nSkipSize += pParameter->GetSize();
		return true;
	}

	// skip members used in IPC binding only if we do marshaling, unmarshaling
	// in same function (call, send, reply, wait) Otherwise we have to store
	// the values in the message struct
	if ((dynamic_cast<CBECallFunction*>(pFunction) == 0) &&
		(dynamic_cast<CBESndFunction*>(pFunction) == 0) &&
		(dynamic_cast<CBEReplyFunction*>(pFunction) == 0) &&
		(dynamic_cast<CBEWaitFunction*>(pFunction) == 0))
		return false;

	// get supposed size of members to skip
	CL4BESizes *pSizes = static_cast<CL4BESizes*>(CCompiler::GetSizes());
	int nSize = pSizes->GetMaxShortIPCSize();
	int nWordSize = pSizes->GetSizeOfType(TYPE_MWORD);
	CBEType *pType = pParameter->GetType();
	// first: try to get the size of the parameter
	int nParamSize = pParameter->GetSize();
	// second: if that didn't work try to get the maximum size of the param
	if (pType->IsPointerType() ||
		nParamSize == -1)
		pParameter->GetMaxSize(nParamSize);
	// and third: if that didn't work either, get the max size of the type
	if (nParamSize == -1)
		nParamSize = pSizes->GetMaxSizeOfType(pType->GetFEType());
	m_nSkipSize += nParamSize;

	// do NOT skip exception variable
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "param %p, exc %p\n", pParameter,
		pFunction->GetExceptionVariable());
	if (pParameter == pFunction->GetExceptionVariable())
		return false;

	// Check if this parameter should be marshalled
	// - it should be of word size
	// - it should not be a constructed type
	if ((nParamSize == nWordSize) &&
		(m_nSkipSize <= nSize))
		return true;

	return false;
}

/** \brief marshals platform specific members of the message buffer
 *  \param pMember the member to test for marshalling
 *  \return true if the member has been marshaled
 */
bool
CL4BEMarshaller::MarshalSpecialMember(CBEFile& pFile, CBETypedDeclarator *pMember)
{
	assert(pMember);
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CL4BEMarshaller::%s(%s) called\n",
		__func__, pMember->m_Declarators.First()->GetName().c_str());
	if (CBEMarshaller::MarshalSpecialMember(pFile, pMember))
		return true;

	if (MarshalRcvFpage(pMember))
		return true;
	if (MarshalSendDope(pMember))
		return true;
	if (MarshalSizeDope(pMember))
		return true;
	if (MarshalZeroFlexpage(pFile, pMember))
		return true;

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CL4BEMarshaller::%s(%s) returns false\n",
		__func__, pMember->m_Declarators.First()->GetName().c_str());
	return false;
}

/** \brief marshals the receive flexpage (if necessary)
 *  \param pMember the member to test for marshalling
 *  \return true if the member has been marshaled
 *
 * The receive flexpage is never marshalled. Therefore we simply test if the
 * current member is the flexpage and if so, return true to indicate to skip
 * this member
 */
bool
CL4BEMarshaller::MarshalRcvFpage(CBETypedDeclarator *pMember)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sFlexName =
		pNF->GetMessageBufferMember(TYPE_RCV_FLEXPAGE);

	// is this the flexpage member
	if (!pMember->m_Declarators.Find(sFlexName))
		return false;

	return true;
}

/** \brief marshals the send dope
 *  \param pMember the member to test for marshalling
 *  \return true if the member has been marshaled
 *
 * The send dope is (like the receive flexpage) never marshalled. Therefore,
 * we look for it and if found indicate to skip it.
 */
bool
CL4BEMarshaller::MarshalSendDope(CBETypedDeclarator *pMember)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sSendName =
		pNF->GetMessageBufferMember(TYPE_MSGDOPE_SEND);

	// is this the send dope
	if (!pMember->m_Declarators.Find(sSendName))
		return false;

	return true;
}

/** \brief marshals the size dope
 *  \param pMember the member to test for marshalling
 *  \return true if the member has been marshaled
 *
 * The size dope is (like send dope and receive flexpage) never marshalled.
 * Therefore, we test for the size dope and return true to indicate that this
 * member should be skipped.
 */
bool
CL4BEMarshaller::MarshalSizeDope(CBETypedDeclarator *pMember)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sSizeName =
		pNF->GetMessageBufferMember(TYPE_MSGDOPE_SIZE);

	// is this the size dope
	if (!pMember->m_Declarators.Find(sSizeName))
		return false;

	return true;
}

/** \brief writes a single member fitting to a word sized location
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param nType the type of the struct
 *  \param nPosition the position of the requested parameter
 *  \param bReference true if a reference to the parameter is required
 *  \param bLValue true if parameter is l-Value
 *  \return true if member has been marshalled
 *
 * The function has to be specified explicetly, because this method might also
 * be called for server message buffers.
 *
 * If bReference is true a reference is required, such as when to use a
 * pointer to the member when calling a function.
 *
 * This implementation does only write the proper access to the member at the
 * desired position in the message buffer.  It does not influence where this
 * member is marshalled.
 */
bool
CL4BEMarshaller::MarshalWordMember(CBEFile& pFile,
	CBEFunction *pFunction,
	CMsgStructType nType,
	int nPosition,
	bool bReference,
	bool bLValue)
{
	m_pFunction = pFunction;
	PositionMarshaller *pPosMarshaller = new PositionMarshaller(this);
	bool bRet = pPosMarshaller->Marshal(pFile, pFunction, nType,
		nPosition, bReference, bLValue);
	delete pPosMarshaller;

	return bRet;
}

/** \brief marshals a single parameter
 *  \param pFile the file to marshal to
 *  \param pFunction the function the parameter belongs to
 *  \param pParameter the parameter to marshal
 *  \param bMarshal true if marshaling, false if unamrshaling
 *  \param nPosition the position to marshal the parameter in the generic \
 *         struct
 */
void
CL4BEMarshaller::MarshalParameter(CBEFile& pFile,
	CBEFunction *pFunction,
	CBETypedDeclarator *pParameter,
	bool bMarshal,
	int nPosition)
{
	m_pFunction = pFunction;
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"%s called for func %s and param %s (%s at pos %d)\n",
		__func__, pFunction ? pFunction->GetName().c_str() : "(none)",
		pParameter ? pParameter->m_Declarators.First()->GetName().c_str() : "(none)",
		bMarshal ? "marshalling" : "unmarshalling", nPosition);

	CMsgStructType nDirection(CMsgStructType::Generic);
	if (bMarshal)
		nDirection = pFunction->GetSendDirection();
	else
		nDirection = pFunction->GetReceiveDirection();

	CDeclStack stack;
	stack.push_back(pParameter->m_Declarators.First());

	if (bMarshal)
	{
		pFile << "\t";
		MarshalWordMember(pFile, pFunction, nDirection, nPosition, false, true);
		pFile << " = ";
		WriteParameter(pFile, pParameter, &stack, false);
		pFile << ";\n";
	}
	else
	{
		pFile << "\t";
		WriteParameter(pFile, pParameter, &stack, false);
		pFile << " = ";
		MarshalWordMember(pFile, pFunction, nDirection, nPosition, false, false);
		pFile << ";\n";
	}
}

/** \brief internal method to marshal a parameter
 *  \param pParameter the parameter to marshal
 *  \param stack the declarator stack
 *
 * This method decides which strategy should be used to marshal the given
 * parameter. It also checks if there is a special treatment necessary.
 */
void
CL4BEMarshaller::MarshalParameterIntern(CBEFile& pFile, CBETypedDeclarator *pParameter,
	CDeclStack* pStack)
{
	if (MarshalRefstring(pFile, pParameter, pStack))
		return;

	CBEMarshaller::MarshalParameterIntern(pFile, pParameter, pStack);
}

/** \brief marshal an indirect string parameter
 *  \param pParameter the member to test for refstring marshalling
 *  \param stack the declarator stack so far
 *  \return true if we marshalled a refstring
 *
 * A refstring parameter can be identified by its ATTR_REF attribute. A
 * pointer to this parameter has then to be assigned to the snd_str element of
 * the indirect part member. The size has to be assigned to the snd_size
 * element (for sending). For receiving the address of the receive buffer can
 * be used to set the address of the receive buffer.
 *
 * We have to use the rcv_str member to set incoming strings, because snd_str
 * is not se properly.
 */
bool
CL4BEMarshaller::MarshalRefstring(CBEFile& pFile,
	CBETypedDeclarator *pParameter,
	CDeclStack* pStack)
{
	assert(pParameter);

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CL4BEMarshaller::%s called for %s with%s [ref]\n", __func__,
		pParameter->m_Declarators.First()->GetName().c_str(),
		pParameter->m_Attributes.Find(ATTR_REF) ? "" : "out");
	if (!pParameter->m_Attributes.Find(ATTR_REF))
		return false;

	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer(m_pFunction);
	CBETypedDeclarator *pMember = FindMarshalMember(pStack);
	if (!pMember)
	{
		CMessages::Warning("CL4BEMarshaller::%s: couldn't find member for param %s\n",
			__func__, pParameter->m_Declarators.First()->GetName().c_str());
	}
	assert(pMember);
	CBEType *pType = pParameter->GetType();
	// try to find respective member and assign
	if (m_bMarshal)
	{
		pFile << "\t";
		WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pMember,
			pStack);
		// write access to snd_str part of indirect string
		pFile << ".snd_str = (l4_umword_t)";
		// if type of member and parameter are different, cast to member type
		WriteParameter(pFile, pParameter, pStack, true);
		pFile << ";\n";
		//
		// set size
		pFile << "\t";
		WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pMember,
			pStack);
		// write access to snd_str part of indirect string
		pFile << ".snd_size = ";
		pParameter->WriteGetSize(pFile, pStack, m_pFunction);
		if (!pParameter->m_Attributes.Find(ATTR_LENGTH_IS) &&
			!pParameter->m_Attributes.Find(ATTR_SIZE_IS) &&
			pParameter->m_Attributes.Find(ATTR_STRING))
			pFile << "+1"; // tranmist terminating zero
		pFile << ";\n";

		if (pParameter->m_Attributes.Find(ATTR_MAX_IS))
		{
			// if parameter has max-is attribute, make sure snd_size adheres to
			// that
			pFile << "\tif (";
			WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pMember,
				pStack);
			// write access to snd_str part of indirect string
			pFile << ".snd_size > ";
			pParameter->WriteGetMaxSize(pFile, pStack, m_pFunction);
			pFile << ")\n";
			++pFile << "\t";
			WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pMember,
				pStack);
			// write access to snd_str part of indirect string
			pFile << ".snd_size = ";
			pParameter->WriteGetMaxSize(pFile, pStack, m_pFunction);
			pFile << ";\n";
			--pFile;
		}
	}
	else if (m_pFunction->IsComponentSide() || // on server side OR
		!pParameter->m_Attributes.Find(ATTR_PREALLOC_CLIENT)) // no prealloc
	{
		// do not unmarshal refstring if preallocated at the client side,
		// because preallocated refstrings are already assigned to rcv_str.
		//
		// if parameter is [out] and has *one* reference then we have to
		// dereference the parameter, because the reference is simply for
		// [out].
		bool bDeref = pParameter->m_Attributes.Find(ATTR_OUT) &&
			pParameter->m_Declarators.First()->GetStars() == 1 &&
			!pParameter->GetType()->IsPointerType();
		pFile << "\t";
		WriteParameter(pFile, pParameter, pStack, !bDeref);
		pFile << " = ";
		if (bDeref)
			pFile << "*";
		// cast to type of parameter
		pType->WriteCast(pFile, true);
		// access message buffer
		WriteMember(pFile, m_pFunction->GetReceiveDirection(), pMsgBuffer, pMember, pStack);
		// append receive member
		pFile << ".rcv_str;\n";

		// We do unmarshal the size parameter, because the actually
		// transmitted size might be smaller than the size of the receive
		// buffer we provided.
		// But we only do this, if the size parameter is a parameter of the
		// function.

		// now: if we dereference the parameter and we malloced the receive
		// string, then we should free it now.
		if (bDeref)
		{
			pFile << "\t";
			CBEContext::WriteFree(pFile, m_pFunction);
			pFile << " ( (void*) ";
			WriteMember(pFile, m_pFunction->GetReceiveDirection(), pMsgBuffer, pMember, pStack);
			pFile << ".rcv_str);\n";
		}
	}

	return true;
}

/** \brief internal marshalling function for arrays
 *  \param pParameter the parameter to marshal
 *  \param pType the type to marshal with
 *  \param pStack the currently active declarator stack
 *
 * If we marshalled an array of flexpages, ensure that the last + 1 flexpage
 * is a zero flexpage.
 */
void CL4BEMarshaller::MarshalArrayIntern(CBEFile& pFile,
	CBETypedDeclarator *pParameter,
	CBEType *pType,
	CDeclStack* pStack)
{
	// first marshal array
	CBEMarshaller::MarshalArrayIntern(pFile, pParameter, pType, pStack);

	// now check for flexpage and marshalling
	if (!pType->IsOfType(TYPE_FLEXPAGE) ||
		!m_bMarshal)
		return;
	// check for size-is or length-is which indicates variable length
	CBEAttribute *pAttr = pParameter->m_Attributes.Find(ATTR_SIZE_IS);
	if (!pAttr)
		pAttr = pParameter->m_Attributes.Find(ATTR_LENGTH_IS);
	if (!pAttr)
		return;

	if (pAttr->IsOfType(ATTR_CLASS_IS))
	{
		pFile << "\tif (";
		pParameter->WriteGetSize(pFile, pStack, m_pFunction);
		pFile << " < ";
		if (pParameter->m_Attributes.Find(ATTR_MAX_IS))
			pParameter->WriteGetMaxSize(pFile, pStack, m_pFunction);
		else
		{
			CBEDeclarator *pDeclarator = pParameter->m_Declarators.First();
			int nMax = 0;
			if (pDeclarator->GetArrayDimensionCount() > 0)
				nMax = pDeclarator->m_Bounds.First()->GetIntValue();
			pFile << nMax;
		}
		pFile << ")\n";
		pFile << "\t{\n";

		CBEMsgBuffer *pMsgBuffer = GetMessageBuffer(m_pFunction);
		// zero send base
		++pFile << "\t";
		WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pParameter, pStack);
		pFile << "[";
		pParameter->WriteGetSize(pFile, pStack, m_pFunction);
		pFile << "].snd_base = 0;\n";
		// zero fpage member
		pFile << "\t";
		WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pParameter, pStack);
		pFile << "[";
		pParameter->WriteGetSize(pFile, pStack, m_pFunction);
		pFile << "].fpage.raw = 0;\n";

		--pFile << "\t}\n";
	}

}

/** \brief test if zero flexpage and marshal if so
 *  \param pMember the parameter to marshal
 *  \return true if zero flexpage marshalled
 */
bool
CL4BEMarshaller::MarshalZeroFlexpage(CBEFile& pFile,
	CBETypedDeclarator *pMember)
{
	assert(pMember);

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetString(CL4BENameFactory::STR_ZERO_FPAGE);
	if (!pMember->m_Declarators.Find(sName))
		return false;

	// get message buffer
	CBEMsgBuffer *pMsgBuffer = pMember->GetSpecificParent<CBEMsgBuffer>();
	assert(pMsgBuffer);

	if (m_bMarshal)
	{
		// zero send base
		pFile << "\t";
		WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pMember, 0);
		pFile << ".snd_base = 0;\n";
		// zero fpage member
		pFile << "\t";
		WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pMember, 0);
		pFile << ".fpage.raw = 0;\n";
	}

	return true;
}

/** \brief writes the access to a specific member in the message buffer
 *  \param nDirection the direction of the parameter
 *  \param pMsgBuffer the message buffer containing the members
 *  \param pMember the member to access
 *  \param stack set if a stack is to be used
 *
 * For derived interfaces the offset into the message buffer where indirect
 * strings may start can vary greatly. For marshalling we cannot directly use
 * the indirect part member, but have to use an offset into the word buffer
 * (size dope's word value) and cast that location to a stringdope. (This only
 * applies when marshalling at server side.)
 *
 * To allow multiple indirect strings, we first have to know at which position
 * in the indirect string list the current member is.
 */
void
CL4BEMarshaller::WriteMember(CBEFile& pFile,
	DIRECTION_TYPE nDirection,
	CBEMsgBuffer *pMsgBuffer,
	CBETypedDeclarator *pMember,
	CDeclStack* pStack)
{
	if (pMember->m_Attributes.Find(ATTR_REF) &&
		m_pFunction->IsComponentSide() &&
		!dynamic_cast<CBESndFunction*>(m_pFunction) &&
		!dynamic_cast<CBEReplyFunction*>(m_pFunction))
	{
		WriteRefstringCastMember(pFile, nDirection, pMsgBuffer, pMember);
		return;
	}

	CBEMarshaller::WriteMember(pFile, nDirection, pMsgBuffer, pMember, pStack);
}

/** \brief writes the access to a refstring member in the message buffer
 *  \param nDirection the direction of the parameter
 *  \param pMsgBuffer the message buffer containing the members
 *  \param pMember the member to access
 *
 * For derived interfaces the offset into the message buffer where indirect
 * strings may start can vary greatly. For marshalling we cannot directly use
 * the indirect part member, but have to use an offset into the word buffer
 * (size dope's word value) and cast that location to a stringdope. (This only
 * applies when marshalling at server side.)
 *
 * To allow multiple indirect strings, we first have to know at which position
 * in the indirect string list the current member is.
 */
void
CL4BEMarshaller::WriteRefstringCastMember(CBEFile& pFile,
	DIRECTION_TYPE nDirection,
	CBEMsgBuffer *pMsgBuffer,
	CBETypedDeclarator *pMember)
{
	assert(pMember);
	assert(pMsgBuffer);

	// get index in refstring field
	CMsgStructType nType(nDirection);
	CBEStructType *pStruct = GetStruct(m_pFunction, nType);
	assert(pStruct);
	// iterate members of struct, when member of struct matches pMember, then
	// stop counting, otherwise: if member of struct is of type refstring
	// increment counter
	int nIndex = -1;
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = pStruct->m_Members.begin();
		iter != pStruct->m_Members.end();
		iter++)
	{
		/* increment first, so nIndex is at zero if we have only one member */
		if ((*iter)->m_Attributes.Find(ATTR_REF))
			nIndex++;
		if ((*iter) == pMember)
			break;
	}
	assert (nIndex >= 0);
	ostringstream os;
	// of course we have to add the size of a stringdope in words
	CBESizes *pSizes = CCompiler::GetSizes();
	nIndex *= pSizes->GetSizeOfType(TYPE_REFSTRING);
	nIndex = pSizes->WordsFromBytes(nIndex);
	os << nIndex;

	// access to the strings is done using the generic struct, the word member
	// using the size-dope's word count plus the index from above as index
	// into the word member and casting the result to a string dope.
	//
	// *(l4_strdope_t*)(&(<msgbuf>->_word[<msgbuf>->_word._size_dope.md.dwords
	//     + nIndex]))

	// get name of word sized member
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sMember = pNF->GetWordMemberVariable();
	pFile << "(*";
	// write type cast for restring
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEType *pType = pCF->GetNewType(TYPE_REFSTRING);
	pType->CreateBackEnd(true, 0, TYPE_REFSTRING);
	pType->WriteCast(pFile, true);
	delete pType;

	pFile << "(&(";
	pMsgBuffer->WriteAccessToStruct(pFile, m_pFunction, CMsgStructType::Generic);
	pFile << "." << sMember << "[";
	pMsgBuffer->WriteMemberAccess(pFile, m_pFunction, nType, TYPE_MSGDOPE_SIZE, 0);
	pFile << ".md.dwords";
	if (nIndex > 0)
		pFile << " + " << os.str();
	pFile << "])))";
}


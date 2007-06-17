/**
 *  \file    dice/src/be/l4/L4BEMsgBuffer.cpp
 *  \brief   contains the implementation of the class CL4BEMsgBuffer
 *
 *  \date    02/02/2005
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "L4BEMsgBuffer.h"
#include "L4BEMsgBufferType.h"
#include "L4BESizes.h"
#include "L4BENameFactory.h"
#include "be/BEMsgBuffer.h"
#include "be/BEFunction.h" // required for DIRECTION_IN/_OUT
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEMsgBufferType.h"
#include "be/BEInterfaceFunction.h"
#include "be/BESrvLoopFunction.h"
#include "be/BEStructType.h"
#include "be/BEAttribute.h"
#include "be/BEExpression.h"
#include "be/BEDeclarator.h"
#include "be/BEUnionCase.h"
#include "be/BEClassFactory.h"
#include "Compiler.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "TypeSpec-L4Types.h"
#include <cassert>

CL4BEMsgBuffer::CL4BEMsgBuffer()
 : CBEMsgBuffer()
{}

CL4BEMsgBuffer::CL4BEMsgBuffer(CL4BEMsgBuffer & src)
 : CBEMsgBuffer(src)
{}

/** destroys the object */
CL4BEMsgBuffer::~CL4BEMsgBuffer()
{}

/** \brief creates a copy of the object
 *  \return a reference to the newly created instance
 */
CObject* CL4BEMsgBuffer::Clone()
{ 
    return new CL4BEMsgBuffer(*this); 
}

/** \brief creates a member variable for the receive flexpage
 *  \return a reference to the variable or NULL if none could be created
 */
CBETypedDeclarator* 
CL4BEMsgBuffer::GetFlexpageVariable()
{
    string exc = string(__func__);
    
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEType *pType = pCF->GetNewType(TYPE_RCV_FLEXPAGE);
    string sName = CCompiler::GetNameFactory()->
	GetMessageBufferMember(TYPE_RCV_FLEXPAGE);
    CBETypedDeclarator *pFpage = pCF->GetNewTypedDeclarator();
    pType->CreateBackEnd(false, 0, TYPE_RCV_FLEXPAGE);
    pFpage->CreateBackEnd(pType, sName);
    delete pType; // cloned in CBETypedDeclarator::CreateBackEnd
    
    CBEAttribute *pAttr = pCF->GetNewAttribute();
    // add directional attribute so later checks when marshaling work
    pAttr->SetParent(pFpage);
    pAttr->CreateBackEnd(ATTR_IN);
    pFpage->m_Attributes.Add(pAttr);
    // add directional attribute so later checks when marshaling work
    pAttr = pCF->GetNewAttribute();
    pAttr->SetParent(pFpage);
    pAttr->CreateBackEnd(ATTR_OUT);
    pFpage->m_Attributes.Add(pAttr);

    return pFpage;
}

/** \brief creates a member variable for the size dope
 *  \return a reference to the created variable or NULL if failed
 */
CBETypedDeclarator* 
CL4BEMsgBuffer::GetSizeDopeVariable()
{
    string exc = string(__func__);
    
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEType *pType = pCF->GetNewType(TYPE_MSGDOPE_SIZE);
    string sName = CCompiler::GetNameFactory()->
	GetMessageBufferMember(TYPE_MSGDOPE_SIZE);
    CBETypedDeclarator *pSize = pCF->GetNewTypedDeclarator();
    pType->CreateBackEnd(false, 0, TYPE_MSGDOPE_SIZE);
    pSize->CreateBackEnd(pType, sName);
    delete pType; // cloned in CBETypedDeclarator::CreateBackEnd
    
    // add directional attribute so later checks when marshaling work
    CBEAttribute *pAttr = pCF->GetNewAttribute();
    pAttr->SetParent(pSize);
    pAttr->CreateBackEnd(ATTR_IN);
    pSize->m_Attributes.Add(pAttr);
    // add directional attribute so later checks when marshaling work
    pAttr = pCF->GetNewAttribute();
    pAttr->SetParent(pSize);
    pAttr->CreateBackEnd(ATTR_OUT);
    pSize->m_Attributes.Add(pAttr);

    return pSize;
}

/** \brief creates a member variable for the send dope
 *  \return a reference to the send dope variable
 */
CBETypedDeclarator* 
CL4BEMsgBuffer::GetSendDopeVariable()
{
    string exc = string(__func__);
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEType *pType = pCF->GetNewType(TYPE_MSGDOPE_SEND);
    string sName = CCompiler::GetNameFactory()->
	GetMessageBufferMember(TYPE_MSGDOPE_SEND);
    CBETypedDeclarator *pSend = pCF->GetNewTypedDeclarator();
    pType->CreateBackEnd(false, 0, TYPE_MSGDOPE_SEND);
    pSend->CreateBackEnd(pType, sName);
    delete pType; // cloned in CBETypedDeclarator::CreateBackEnd
    
    // add directional attribute so later checks when marshaling work
    CBEAttribute *pAttr = pCF->GetNewAttribute();
    pAttr->SetParent(pSend);
    pAttr->CreateBackEnd(ATTR_IN);
    pSend->m_Attributes.Add(pAttr);
    // add directional attribute so later checks when marshaling work
    pAttr = pCF->GetNewAttribute();
    pAttr->SetParent(pSend);
    pAttr->CreateBackEnd(ATTR_OUT);
    pSend->m_Attributes.Add(pAttr);

    return pSend;
}

/** \brief sorts one struct in the message buffer
 *  \param pStruct the struct to sort
 *  \return true if sorting was successful
 *
 * This implementation first moves the special message buffer members to the
 * front and then calls the base class' Sort method.  This order is important,
 * because the Sort method calls GetStartOfPayload, which in turn tries to
 * skip the special message buffer members.  Thus they have to be at the begin
 * of the message buffer before calling base class' Sort.
 *
 * The same procedure has to be done after calling the base class' Sort,
 * because after the payload sort opcode and exception are moved to the start
 * of the message buffer.  Thus we have to move the special members past them
 * again.
 *
 * This implementation takes also care of flexpages and indirect strings.
 */
bool
CL4BEMsgBuffer::Sort(CBEStructType *pStruct)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    // doing this in reverse order, so we can insert them at
    // position 0
    // get send dope
    string sName = pNF->GetMessageBufferMember(TYPE_MSGDOPE_SEND);
    pStruct->m_Members.Move(sName, 0);
    // get size dope
    sName = pNF->GetMessageBufferMember(TYPE_MSGDOPE_SIZE);
    pStruct->m_Members.Move(sName, 0);
    // get flexpage
    sName = pNF->GetMessageBufferMember(TYPE_RCV_FLEXPAGE);
    pStruct->m_Members.Move(sName, 0);

    // the following call, calls SortPayload which uses DoExchangeMembers to
    // sort the members.
    if (!CBEMsgBuffer::Sort(pStruct))
	return false;

    // move all flexpages to begin (to override opcode and exception
    // placement) -- therefore we do not start with the payload, but we
    // iterate all members of the struct
    vector<CBETypedDeclarator*>::iterator iter = pStruct->m_Members.begin();
    while (iter != pStruct->m_Members.end())
    {
	vector<CBETypedDeclarator*>::iterator iterS = iter + 1;
	if (iterS == pStruct->m_Members.end())
	    break; // finished
	
	int nFirstType = (*iter)->GetType()->GetFEType();
	int nSecondType = (*iterS)->GetType()->GetFEType();

	if ((nFirstType == TYPE_FLEXPAGE) &&
	    (nSecondType != TYPE_FLEXPAGE))
	{
	    iter++;
	    continue;
	}
	
	if ((nFirstType != TYPE_FLEXPAGE) &&
	    (nSecondType == TYPE_FLEXPAGE))
	{
	    pStruct->m_Members.Move((*iterS)->m_Declarators.First()->GetName(),
		    (*iter)->m_Declarators.First()->GetName());
	    iter = pStruct->m_Members.begin();
	    continue;
	}

	// if both are flexpages, ensure that zero flexpage comes last
	if ((nFirstType == TYPE_FLEXPAGE) &&
	    (nSecondType == TYPE_FLEXPAGE))
	{
	    sName = pNF->GetString(CL4BENameFactory::STR_ZERO_FPAGE);
	    if ((*iter)->m_Declarators.Find(sName))
	    {
		pStruct->m_Members.Move((*iterS)->m_Declarators.First()->GetName(),
		    (*iter)->m_Declarators.First()->GetName());
		iter = pStruct->m_Members.begin();
		continue;
	    }
	}

	iter++;
    }

    // and now move the L4 message header back up front
    // get send dope
    sName = pNF->GetMessageBufferMember(TYPE_MSGDOPE_SEND);
    pStruct->m_Members.Move(sName, 0);
    // get size dope
    sName = pNF->GetMessageBufferMember(TYPE_MSGDOPE_SIZE);
    pStruct->m_Members.Move(sName, 0);
    // get flexpage
    sName = pNF->GetMessageBufferMember(TYPE_RCV_FLEXPAGE);
    pStruct->m_Members.Move(sName, 0);
    
    return true;
}

/** \brief checks if two members of a struct should be exchanged
 *  \param pFirst the first member
 *  \param pSecond the second member - first's successor
 *  \return true if exchange
 *
 * This method check for refstrings in the message buffer. They should always
 * stay at the end.
 */
bool
CL4BEMsgBuffer::DoExchangeMembers(CBETypedDeclarator *pFirst,
	CBETypedDeclarator *pSecond)
{
    assert(pFirst);
    assert(pSecond);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called with (%s, %s)\n", __func__,
	pFirst->m_Declarators.First()->GetName().c_str(),
	pSecond->m_Declarators.First()->GetName().c_str());

    int nFirstType = pFirst->GetType()->GetFEType();
    int nSecondType = pSecond->GetType()->GetFEType();

    // test refstrings
    if ((nFirstType == TYPE_REFSTRING) &&
	(nSecondType != TYPE_REFSTRING))
	return true;
    if ((nFirstType != TYPE_REFSTRING) &&
	(nSecondType == TYPE_REFSTRING))
	return false;

    return CBEMsgBuffer::DoExchangeMembers(pFirst, pSecond);
}

/** \brief adds the members of to the generic struct
 *  \param pStruct the struct to add the members to
 *  \return true if successful
 */
bool
CL4BEMsgBuffer::AddGenericStructMembersClass(CBEStructType *pStruct)
{
    // count word members
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);

    int nWords = GetWordMemberCountClass();
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s GetWordMemberCount returned %d\n",
	__func__, nWords);
    // we use at least one element in the _word array, that is the opcode.
    // Exception checks regularely reset it to 0. Thus we need at least one
    // word array dimension in class' word struct.
    nWords = std::max(nWords, 1);
    // create word member and add to struct
    CBETypedDeclarator *pMember = GetWordMemberVariable(nWords);
    if (!pMember)
	return false;
    pStruct->m_Members.Add(pMember);
    // count refstring members
    int nStrings = CBEMsgBuffer::GetMemberSize(TYPE_REFSTRING);
    // create restring member and add to struct
    if (nStrings > 0)
    {
	pMember = GetRefstringMemberVariable(nStrings);
	if (!pMember)
	    return false;
	pStruct->m_Members.Add(pMember);
    }

    // return status
    return true;
}

/** \brief creates an array member of word sized elements for the message buffer
 *  \param nNumber the number of elements
 *  \return a reference to the newly created opcode member
 */
CBETypedDeclarator*
CL4BEMsgBuffer::GetRefstringMemberVariable(int nNumber)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sName = pNF->GetMessageBufferMember(TYPE_REFSTRING);
    return GetMemberVariable(TYPE_REFSTRING, false, sName, nNumber);
}

/** \brief write the ref string initialisation
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pClass the containing class
 *  \param nIndex the index in an array of indirect strings
 *  \param nType the type of the message buffer struct
 *  \return true if we handled the function
 *
 * This implementation is empty.
 */
bool
CL4BEMsgBuffer::WriteRefstringInitFunction(CBEFile& /*pFile*/,
    CBEFunction* /*pFunction*/,
    CBEClass* /*pClass*/,
    int /*nIndex*/,
    CMsgStructType /*nType*/)
{
    return false;
}

/** \brief write the refstring initialization for a specific parameter
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pMember the member that is the refstring
 *  \param nIndex the index in an array of refstrings
 *  \param nType the type if the message buffer struct
 *
 * This implementation is empty.
 */
void 
CL4BEMsgBuffer::WriteRefstringInitParameter(CBEFile& /*pFile*/,
    CBEFunction* /*pFunction*/,
    CBETypedDeclarator* /*pMember*/,
    int /*nIndex*/,
    CMsgStructType /*nType*/)
{
}

/** \brief write the receive flexpage initialization
 *  \param pFile the file to write to
 *  \param nType the type of the message buffer struct
 *
 * This implementation is empty
 */
void
CL4BEMsgBuffer::WriteRcvFlexpageInitialization(CBEFile& /*pFile*/,
    CMsgStructType /*nType*/)
{
}

/** \brief writes the initialization of specific members
 *  \param pFile the file to write to
 *  \param pFunction the function to use as reference for message buffer
 *  \param nType the type of the members to initialize
 *  \param nStructType the type of the message buffer struct
 *
 * We do rely on the sizeof feature and the variable sized array features of
 * the taret compiler here. Variable sized arrays are ideally declared in the
 * message buffer using the declarators denoting the array size in the array
 * declaration. Using sizeof on the message buffer correctly calculates the
 * size of the total message buffer.
 *
 * Example: (param: [in, string] char * str)
 *
 * {
 *   long _str_len = strlen (str) + 1;
 *   struct {
 *     l4_strdope_t size;
 *     ...
 *     long _str_len;
 *     char str[_str_len];
 *   } msg;
 *   msg._str_len = _str_len;
 *   memcpy (msg.str, str, _str_len);
 *   msg.size = L4_IPC_DOPE(sizeof(msg)/sizeof(long)-3, 0);
 *   ...
 * }
 *
 * We can also simple subtract the number of indirect strings, because they
 * have to be aligned in the [in] and [out] buffers. All we have to do is to
 * count the indirect strings.
 *
 * Example:
 *
 * union {
 *   struct {
 *     ...
 *     long param1;
 *     long param2;
 *     l4_strdope_t str_param1;
 *   } in;
 *   struct {
 *     ...
 *     long param1;
 *     long __pad_to_string;
 *     l4_strdope_t str_param2;
 *   } out;
 * } msg;
 *
 */
void
CL4BEMsgBuffer::WriteInitialization(CBEFile& pFile,
    CBEFunction *pFunction,
    int nType,
    CMsgStructType nStructType)
{
    if ((nType != TYPE_MSGDOPE_SIZE) &&
	(nType != TYPE_MSGDOPE_SEND) &&
	(nType != TYPE_REFSTRING) &&
	(nType != TYPE_RCV_FLEXPAGE))
    {
	CBEMsgBuffer::WriteInitialization(pFile, pFunction, nType, nStructType);
	return;
    }

    if (nType == TYPE_REFSTRING)
    {
	WriteRefstringInitialization(pFile, nStructType);
	return;
    }
    if (nType == TYPE_RCV_FLEXPAGE)
    {
	WriteRcvFlexpageInitialization(pFile, nStructType);
	return;
    }
   
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s for func %s and dir %d\n", 
	__func__, pFunction ? pFunction->GetName().c_str() : "(no func)", 
	(int)nStructType);

    // count the indirect strings
    // if direction is 0 we have to get maximum of IN and OUT
    int nStrings = 0;
    CBESizes *pSizes = CCompiler::GetSizes();
    int nWordSize = pSizes->GetSizeOfType(TYPE_MWORD);
    int nRefSize = pSizes->GetSizeOfType(TYPE_REFSTRING) / nWordSize;
    if (CMsgStructType::Generic == nStructType)
    {
	// for generic struct we also have to subtract the strings from the
	// word count, because the strings have been counted when counting
	// TYPE_MWORD as well.
	int nStringsIn = CBEMsgBuffer::GetMemberSize(TYPE_REFSTRING, pFunction,
	    CMsgStructType::In, false);

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s strings in: %d\n", 
	    __func__, nStringsIn);

	int nStringsOut = CBEMsgBuffer::GetMemberSize(TYPE_REFSTRING,
	    pFunction, CMsgStructType::Out, false);

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s strings out: %d\n", 
	    __func__, nStringsOut);

	nStrings = std::max(nStringsIn, nStringsOut);

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s strings: %d\n", __func__,
	    nStrings);
    }
    else
    {
	nStrings = CBEMsgBuffer::GetMemberSize(TYPE_REFSTRING, pFunction,
	    nStructType, false);

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s strings: %d\n", __func__,
	    nStrings);
    }

    // get name of member
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sName = pNF->GetMessageBufferMember(nType);
    // get member
    CBETypedDeclarator *pMember = FindMember(sName, pFunction, nStructType);
    // check if we have member of that type
    if (!pMember && (CMsgStructType::Generic == nStructType))
    {
	nStructType = CMsgStructType::In;
	// maybe we do not have a generic struct
	pMember = FindMember(sName, nStructType);
    }
    if (!pMember)
	return;

    pFile << "\t";
    WriteAccess(pFile, pFunction, nStructType, pMember);
    pFile << " = L4_IPC_DOPE( sizeof(";
    if (CMsgStructType::Generic == nStructType ||
	nType == TYPE_MSGDOPE_SIZE)
    {
	// sizeof(<msgbufvar>)/sizeof(long)-3
	WriteAccessToVariable(pFile, pFunction, false);
    }
    else
    {
	// sizeof(<msgbufvar>.<structname>)/sizeof(long)-3
	WriteAccessToStruct(pFile, pFunction, nStructType);
    }
    pFile << ")/sizeof(long)-" << 3 + nStrings*nRefSize;
    if (CMsgStructType::Generic != nStructType && 
	nType == TYPE_MSGDOPE_SEND)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	    "In func %s try to find padding members:\n", pFunction->GetName().c_str());
	// if we have any padding members in the send part, subtract their size.
	// do NOT subtract byte padding, because the byte padding is always
	// smaller than word size and we have to transmit full words. Thus
	// only subtract word sized padding.
	CBEStructType *pStruct = GetStruct(pFunction, nStructType);
	sName = pNF->GetPaddingMember(TYPE_MWORD, TYPE_REFSTRING);
	pMember = pStruct->m_Members.Find(sName);

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "  member %s found at %p\n",
	    sName.c_str(), pMember);

	if (pMember)
	    pFile << "-" << pMember->GetSize()/nWordSize;
    }
    pFile << ", " << nStrings << ");\n";
}

/** \brief writes the initialization of the refstring members
 *  \param pFile the file to write to
 *  \param nType the type of the message buffer struct
 *  \return true if we actually did something
 *
 * initialize refstring members: check if they are OUT parameters for this
 * function (have to find them as parameter and check the OUT attribute). If
 * so, set the rcv_size and rcv_str members. This depends on attributes of the
 * parameter (prealloc -> pointer is parameter, size is size attribute). If no
 * attribute given, allocate using CBEContext::WriteMalloc().
 */
bool
CL4BEMsgBuffer::WriteRefstringInitialization(CBEFile& pFile,
    CMsgStructType nType)
{
    CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
    if (!pFunction)
	return false;
    CBEClass *pClass = GetSpecificParent<CBEClass>();
    CBEStructType *pStruct = GetStruct(nType);
    assert(pStruct);

    int nIndex = -1;
    bool bRefstring = false;
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = pStruct->m_Members.begin();
	 iter != pStruct->m_Members.end();
	 iter++)
    {
	if (!(*iter)->GetType()->IsOfType(TYPE_REFSTRING))
	    continue;

	bRefstring = true;

	// check if declarator is array (as can be for generic part of message
	// buffer...
	CBEDeclarator *pMemberDecl = (*iter)->m_Declarators.First();
	vector<CBEExpression*>::iterator arrIter =
	    pMemberDecl->m_Bounds.begin();
	do // using 'do { } while' so we pass here even without array bounds
	{
	    // for each element in the array
	    int nArrayCount = 1;
	    if (arrIter != pMemberDecl->m_Bounds.end())
		nArrayCount = (*arrIter)->GetIntValue();
	    while (nArrayCount--)
	    {

		// increment here, so we catch 'continue' in the middle of the
		// loop.  Because we increment at begin of loop, we have to
		// init index with -1, so first iteration makes it zero.
		nIndex++;

		if (WriteRefstringInitFunction(pFile, pFunction, pClass, 
			nIndex, nType))
		    continue;

		// member is refstring, initialize:
		WriteRefstringInitParameter(pFile, pFunction, (*iter), nIndex,
		    nType);
	    } // array iteration
	    // array bound iteration
	    if (arrIter != pMemberDecl->m_Bounds.end())
		arrIter++;
	} while (arrIter != pMemberDecl->m_Bounds.end());
    }

    return bRefstring;
}

/** \brief writes the maximu size of a specific refstring
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pMember the member to write
 *  \param pParameter the parameter to use as reference
 *  \param nIndex the index of the refstring to initialize
 *
 * The maxmimum size can either be a max-is attribute or the maximum size of
 * the type of the member.  The max-is attribute can only be determined if we
 * have a parameter or the member has one, but it might be difficult to find
 * if there is no parameter, which means we have to initialize for a generic
 * refstring in the server's message buffer.
 *
 * For the latter case we have to iterate the max values of all refstrings for
 * this specific position and determine their maximum.
 */
void
CL4BEMsgBuffer::WriteMaxRefstringSize(CBEFile& pFile,
    CBEFunction *pFunction,
    CBETypedDeclarator *pMember,
    CBETypedDeclarator *pParameter,
    int nIndex)
{
    // if parameter has a max_is attribute use that
    CBEAttribute *pMaxAttr = 0;
    if (pParameter)
	pMaxAttr = pParameter->m_Attributes.Find(ATTR_MAX_IS);
    if (!pMaxAttr)
	pMaxAttr = pMember->m_Attributes.Find(ATTR_MAX_IS);
    if (pMaxAttr)
    {
	pMaxAttr->Write(pFile);
	return;
    }

    // ok, seems to be a generic struct, get the maximum of the max-is
    // attributes of the refstrings of the other message buffer structs
    if (!pParameter)
    {
	string sMaxStr = string();

	bool bInterfaceFunc = (dynamic_cast<CBEInterfaceFunction*>(pFunction));
	// get type
	CBEMsgBufferType *pMsgType;
	if (bInterfaceFunc)
	{
	    CBEClass *pClass = GetSpecificParent<CBEClass>();
	    assert(pClass);
	    pMsgType = dynamic_cast<CBEMsgBufferType*>
		(pClass->GetMessageBuffer()->GetType());
	}
	else
	    pMsgType = dynamic_cast<CBEMsgBufferType*>(GetType());
	assert(pMsgType);
	// iterate structs
	CBEStructType *pStruct = 0;
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sNameIn = pNF->GetMessageBufferStructName(CMsgStructType::In,
	    string(), string());
	string sNameOut = pNF->GetMessageBufferStructName(CMsgStructType::Out,
	    string(), string());
	int nLenIn = sNameIn.length();
	int nLenOut = sNameOut.length();
	vector<CBEUnionCase*>::iterator iter;
	for (iter = pMsgType->m_UnionCases.begin();
	     iter != pMsgType->m_UnionCases.end();
	     iter++)
	{
	    // check direction
	    string sUnion = (*iter)->m_Declarators.First()->GetName();
	    if (sUnion.substr(sUnion.length() - nLenIn) == sNameIn ||
		sUnion.substr(sUnion.length() - nLenOut) == sNameOut)
	    {		
		pStruct = static_cast<CBEStructType*>((*iter)->GetType());

		int nCurr = nIndex;
		
		vector<CBETypedDeclarator*>::iterator iM;
		for (iM = pMsgType->GetStartOfPayload(pStruct);
		     iM != pStruct->m_Members.end();
		     iM++)
		{
		    // get refstring member at index
		    if (!(*iM)->GetType()->IsOfType(TYPE_REFSTRING))
			continue;
		    if (nCurr-- > 0)
			continue;
		    
		    // check max-is attribute of member
		    CBEAttribute *pA = (*iM)->m_Attributes.Find(ATTR_MAX_IS);
		    if (!pA)
			break;

		    if (sMaxStr.empty())
			pA->WriteToStr(sMaxStr);
		    else
		    {
			string s = "_dice_max(" + sMaxStr + ", ";
			pA->WriteToStr(s);
			sMaxStr = s + ")";
		    }
		}
	    }
	}

	// if max-is string is not empty print and return
	if (!sMaxStr.empty())
	{
	    pFile << sMaxStr;
	    return;
	}
    }

    // the last resort: use the type's max size
    CBESizes *pSizes = CCompiler::GetSizes();
    int nMaxStrSize = pSizes->GetMaxSizeOfType(TYPE_CHAR);
    pFile << nMaxStrSize;
}

/** \brief tests if the struct for a direction has enough word sized members \
 *         for short IPC
 *  \param pFunction the function of the message buffer
 *  \param nType the type of the message buffer struct
 *  \return true if enough word sized members exist
 */
bool
CL4BEMsgBuffer::HasWordMembers(CBEFunction *pFunction, 
    CMsgStructType nType)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"%s called for func %s and direction %d\n", 
	__func__, pFunction->GetName().c_str(), (int)nType);

    int nWordSize = CCompiler::GetSizes()->GetSizeOfType(TYPE_MWORD);
    int nShortMembers = GetWordMemberCountFunction();

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: word size %d, short size %d\n",
	__func__, nWordSize, nShortMembers);

    // get structure
    CBEStructType *pStruct = GetStruct(pFunction, nType);
    assert(pStruct);
    // test word sized members
    CBEMsgBufferType *pType = pStruct->GetSpecificParent<CBEMsgBufferType>();
    assert(pType);
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter = pType->GetStartOfPayload(pStruct);
	 iter != pStruct->m_Members.end() && (nShortMembers-- > 0);
	 iter++)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	    "%s: testing member %s %d for word size (short members %d)\n",
	    __func__, (*iter)->m_Declarators.First()->GetName().c_str(),
	    (*iter)->GetSize(), nShortMembers);

	if ((*iter)->GetSize() != nWordSize)
	{
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
		"%s: not word size, return false\n", __func__);

	    return false;
	}
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: short members left: %d\n",
	__func__, nShortMembers);
    if (nShortMembers > 0)
	return false;

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
    return true;
}

/** \brief calculate the number of word sized members required for short IPC
 *  \return the number of word sized members
 *
 * This implementation has to return the number of required word sized members
 * in the generic message buffer. Since this is for functions, not class, the
 * maximum we need is the number of words for the IPC bindings.
 */
int
CL4BEMsgBuffer::GetWordMemberCountFunction()
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);

    CL4BESizes *pSizes = static_cast<CL4BESizes*>(CCompiler::GetSizes());
    int nShort = pSizes->GetMaxShortIPCSize();

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"%s: sizes says, max short size is %d -> make words from that\n",
	__func__, nShort);
    return pSizes->WordsFromBytes(nShort);
}

/** \brief calculate the number of word sized members required for short IPC
 *  \return the number of word sized members
 */
int
CL4BEMsgBuffer::GetWordMemberCountClass()
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
    // get value from base class
    int nSize = CBEMsgBuffer::GetWordMemberCountClass();
    CL4BESizes *pSizes = static_cast<CL4BESizes*>(CCompiler::GetSizes());
    int nShort = pSizes->GetMaxShortIPCSize();
    nShort = pSizes->WordsFromBytes(nShort);
    // subtract refstrings if any
    int nStrings = CBEMsgBuffer::GetMemberSize(TYPE_REFSTRING);
    nStrings *= pSizes->GetSizeOfType(TYPE_REFSTRING);
    // check minimum
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "%s return max(%d, %d)\n", __func__,
	nSize - nStrings, nShort);
    return std::max(nSize - nStrings, nShort);
}

/** \brief determine the size of a specific member
 *  \param nType the type requested to determine the size
 *  \param pMember the member to get the size for
 *  \param bMax true if the maximum size should be determined
 *  \return the size of the member
 *
 * This version of the member size determination filters out refstrings,
 * because sometimes (especially with transmit_as attribute _and_ ref
 * attribute) even though a parameter is transmitted as refstring and has
 * therefore the member type refstring, its size determination uses the
 * transmit-as type.
 *
 * (Maybe we should remove automatically added transmit-as types when creating
 * a refstring member).
 */
int
CL4BEMsgBuffer::GetMemberSize(int nType,
    CBETypedDeclarator *pMember,
    bool bMax)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s (%d, %s, %s) called\n", __func__,
	nType, pMember->m_Declarators.First()->GetName().c_str(),
	bMax ? "true" : "false");
    // filter out refstring members
    CBESizes *pSizes = CCompiler::GetSizes();
    bool bMemberIsRef = pMember->m_Attributes.Find(ATTR_REF) ||
	pMember->GetType()->IsOfType(TYPE_REFSTRING);
    if (bMemberIsRef &&	nType == TYPE_REFSTRING &&
	pMember->m_Attributes.Find(ATTR_TRANSMIT_AS))
	return pSizes->GetSizeOfType(TYPE_REFSTRING);
    if (bMemberIsRef && nType != TYPE_REFSTRING)
	return 0;

    return CBEMsgBuffer::GetMemberSize(nType, pMember, bMax);
}

/** \brief pads elements of the message buffer
 *  \return false if an error occured
 *
 * This implementation has to pad indirect strings to NOT reside in the first
 * two dwords and aligns all refstrings to the same position.
 */
bool
CL4BEMsgBuffer::Pad()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
    // get minimal size for short IPC
    CL4BESizes *pSizes = static_cast<CL4BESizes*>(CCompiler::GetSizes());
    int nMinSize = pSizes->GetMaxShortIPCSize();
    // get maximum position of any refstring in message buffer
    int nMax = GetMaxPosOfRefstringInMsgBuffer();
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	"%s: min size %d, max pos of refstring %d\n", __func__, nMinSize, nMax);
    if (nMinSize > nMax)
	nMax = nMinSize;
    // align refstrings in all message buffers to the max of the two
    return PadRefstringToPosition (nMax);
}

/** \brief pad refstrings to specified position
 *  \param nPosition the position to align refstring to
 *  \return false if something went wrong
 *
 * Refstrings should start after the short IPC words.
 */
bool
CL4BEMsgBuffer::PadRefstringToPosition(int nPosition)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s(%d) called\n", __func__, nPosition);

    CBEMsgBufferType *pType = dynamic_cast<CBEMsgBufferType*>(GetType());
    assert(pType);
    vector<CBEUnionCase*>::iterator iter;
    for (iter = pType->m_UnionCases.begin();
	 iter != pType->m_UnionCases.end();
	 iter++)
    {
	CBEStructType *pStruct = dynamic_cast<CBEStructType*>((*iter)->GetType());
	if (!pStruct)
	    continue;
	if (!PadRefstringToPosition(pStruct, nPosition))
	    return false;
    }
    return true;
}

/** \brief pad refstrings to specified position
 *  \param pStruct the struct to pad the refstrings in
 *  \param nPosition the position to align refstring to
 *  \return false if something went wrong
 *
 * Refstrings should start after the specified position.
 */
bool
CL4BEMsgBuffer::PadRefstringToPosition(CBEStructType *pStruct, 
    int nPosition)
{
    assert(pStruct);
    // get start of payload
    CBEMsgBufferType *pType = dynamic_cast<CBEMsgBufferType*>(GetType());
    assert(pType);
    vector<CBETypedDeclarator*>::iterator iter;
    // iterate
    int nSize = 0;
    for (iter = pType->GetStartOfPayload(pStruct);
	 iter != pStruct->m_Members.end();
	 iter++)
    {
	if ((*iter)->GetType()->IsOfType(TYPE_REFSTRING))
	    break;
	int nMemberSize = (*iter)->GetSize();
	if (nMemberSize < 0)
	    (*iter)->GetMaxSize(nMemberSize);
	nSize += nMemberSize;
	if (nSize > nPosition)
	    return true;
    }
    
    // if no member now, than all members are smaller than position
    if (iter == pStruct->m_Members.end())
	return true;

    assert(*iter);
    // otherwise, the member is a refstring and the previous members are less
    // than the minimal position. Pad them with words until minimal size
    // is reached.
    if (nSize < nPosition)
    {
	int nPadding = nPosition - nSize;
    	int nWordSize = CCompiler::GetSizes()->GetSizeOfType(TYPE_MWORD);
	// store member before which to insert padding into extra variable,
	// because if first padding succeeds, the iterator is no longer valid
	CBETypedDeclarator *pMember = *iter;
	if (nPadding % nWordSize)
	    InsertPadMember(TYPE_BYTE, nPadding % nWordSize,
		pMember, pStruct);
	if (nPadding / nWordSize)
	    InsertPadMember(TYPE_MWORD, nPadding / nWordSize,
		pMember, pStruct);
    }
    
    return true;
}

/** \brief insert a padding member
 *  \param nFEType the type of the member to insert (usually byte or word)
 *  \param nSize the size to insert
 *  \param pMember the member to insert the padding before
 *  \param pStruct the struct where to insert the padding

 *  \return true if successful
 *
 * The implementation first checks if there exists a member with the same
 * name (of the padding variable), which indicates that there has been some
 * padding before (at client side, for instance). If there is such member,
 * then this member's size is increased by the given size (nSize). If no such
 * member existed before, then a new member is created and inserted before
 * pMember.
 */
bool
CL4BEMsgBuffer::InsertPadMember(int nFEType,
    int nSize,
    CBETypedDeclarator *pMember,
    CBEStructType *pStruct)
{
    CBETypedDeclarator *pPadMember = 0; 
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBENameFactory *pNF = CCompiler::GetNameFactory();

    // get name
    string sName = pNF->GetPaddingMember(nFEType, 
	TYPE_REFSTRING);
    // try to find padding member
    if ((pPadMember = pStruct->m_Members.Find(sName)) == 0)
    {
	// create type
	CBEType *pType = pCF->GetNewType(nFEType);
	pType->CreateBackEnd(true, 1, nFEType);
	// create padding member
	pPadMember = pCF->GetNewTypedDeclarator();
	pPadMember->CreateBackEnd(pType, sName);
	delete pType; /* cloned in CBETypedDeclarator::CreateBackEnd */
	// add array with size difference
	if (nSize > 1)
	{
	    CBEExpression *pBound = pCF->GetNewExpression();
	    pBound->CreateBackEnd(nSize);
	    pPadMember->m_Declarators.First()->AddArrayBound(pBound);
	}
	// add member to struct
	pStruct->m_Members.Add(pPadMember);
	// move member just before refstring member
	pStruct->m_Members.Move(sName, pMember->m_Declarators.First()->GetName());
    }
    else
    {
	CBEDeclarator *pDecl = pPadMember->m_Declarators.First();
	CBEExpression *pBound = 0;
	if (pDecl->IsArray())
	{
	    pBound = pDecl->m_Bounds.First();
	    nSize += pBound->GetIntValue();
	}
	else
	{
	    pBound = pCF->GetNewExpression();
	    pDecl->AddArrayBound(pBound);
	    nSize++;
	}
	pBound->CreateBackEnd(nSize);
    }
    return true;
}

/** \brief determine maximum position of a refstring in the message buffer
 *  \return the maximum position or 0 if no refstring member
 *
 *  Iterates all message buffer structs and determines the position of the
 *  first refstring in the payload. This requires that the structs have been
 *  sorted before!
 */
int
CL4BEMsgBuffer::GetMaxPosOfRefstringInMsgBuffer()
{
    int nMax = 0;
    // determine maximum distance to refstring parameters
    CBEMsgBufferType *pType = dynamic_cast<CBEMsgBufferType*>(GetType());
    assert(pType);
    vector<CBEUnionCase*>::iterator iterS;
    for (iterS = pType->m_UnionCases.begin();
	 iterS != pType->m_UnionCases.end();
	 iterS++)
    {
	CBEStructType* pStruct = dynamic_cast<CBEStructType*>((*iterS)->GetType());
	if (!pStruct)
	    continue;

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: Checking next struct\n", 
	    __func__);
	// iterate over struct members
	vector<CBETypedDeclarator*>::iterator iter;
	int nSize = 0;
	for (iter = pType->GetStartOfPayload(pStruct);
	     iter != pStruct->m_Members.end();
	     iter++)
	{
	    if ((*iter)->GetType()->IsOfType(TYPE_REFSTRING))
		break;
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: size of member %s is %d\n",
		__func__, (*iter)->m_Declarators.First()->GetName().c_str(),
		(*iter)->GetSize());
	    int nMemberSize = (*iter)->GetSize();
	    if (nMemberSize < 0)
		(*iter)->GetMaxSize(nMemberSize);
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: adding size (max?) %d\n",
		__func__, nMemberSize);
	    nSize += nMemberSize;
	}
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: Size of this struct is %d\n",
	    __func__, nSize);
	// check if new max
	if (nSize > nMax)
	    nMax = nSize;
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s: returns %d\n", __func__, nMax);
    return nMax;
}

/** \brief test if the message buffer has a certain property
 *  \param nProperty the property to test for
 *  \param nType the type of the message buffer struct
 *  \return true if property is available for direction
 *
 * This implementation checks the short IPC property.
 */
bool
CL4BEMsgBuffer::HasProperty(int nProperty,
	CMsgStructType nType)
{
    if (nProperty == MSGBUF_PROP_SHORT_IPC)
    {
	CBEFunction *pFunction = GetSpecificParent<CBEFunction>();

    	// if direction is 0 we have to get maximum of IN and OUT
	int nWords = 0;
	int nStrings = 0;
	CL4BESizes *pSizes = static_cast<CL4BESizes*>(CCompiler::GetSizes());
	int nWordSize = pSizes->GetSizeOfType(TYPE_MWORD);
	if (CMsgStructType::Generic == nType)
	{
	    int nWordsIn = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction,
		CMsgStructType::In, false);
	    int nWordsOut = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction,
		CMsgStructType::Out, false);
	    
	    int nStringsIn = CBEMsgBuffer::GetMemberSize(TYPE_REFSTRING,
		pFunction, CMsgStructType::In, false);
	    int nStringsOut = CBEMsgBuffer::GetMemberSize(TYPE_REFSTRING,
		pFunction, CMsgStructType::Out, false);
	    nStrings = std::max(nStringsIn, nStringsOut);
	    if (nWordsIn > 0 && nWordsOut > 0)
		nWords = std::max(nWordsIn, nWordsOut);
	    else
		nWords = std::min(nWordsIn, nWordsOut);
	}
	else
	{
	    nWords = CBEMsgBuffer::GetMemberSize(TYPE_MWORD, pFunction,
		nType, false);
	    nStrings = CBEMsgBuffer::GetMemberSize(TYPE_REFSTRING, pFunction,
		nType, false);
	}
	// check minimum number of words
	int nMinWords = GetWordMemberCountFunction();
	if (nWords >= 0)
	    nWords = std::max(nWords, nMinWords);
    
       	// get short IPC values
	int nShortSize = pSizes->GetMaxShortIPCSize() / nWordSize;
	return (nStrings == 0) && (nWords <= nShortSize) && (nWords > 0);
    }

    // send everything else to base class
    return CBEMsgBuffer::HasProperty(nProperty, nType);
}

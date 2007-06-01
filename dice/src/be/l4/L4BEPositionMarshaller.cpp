/**
 *    \file    dice/src/be/l4/L4BEPositionMarshaller.cpp
 *    \brief   contains the implementation of the class \
 *             CL4BEMarshaller::PositionMarshaller
 *
 *    \date    02/07/2005
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
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEInterfaceFunction.h"
#include "be/BEClass.h"
#include "be/BEMsgBuffer.h"
#include "be/BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEStructType.h"
#include "be/BEDeclarator.h"
#include "be/BERoot.h"
#include "be/BESizes.h"
#include "be/BEAttribute.h"
#include "be/BEUserDefinedType.h"
#include "Compiler.h"
#include "Messages.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include <cassert>

CL4BEMarshaller::PositionMarshaller::PositionMarshaller(
    CL4BEMarshaller *pParent)
{
    m_pParent = pParent;
    m_nPosSize = 0;
    m_bReference = false;
    m_pFile = (CBEFile*)0;
}

/** deletes the instance of this class */
CL4BEMarshaller::PositionMarshaller::~PositionMarshaller()
{
}

/** \brief marshal a specific member of a function to a specific position
 *  \param pFile the file to write to
 *  \param pFunction the function to get the parameter from
 *  \param nType the type of the struct
 *  \param nPosition the word position of the member in the message buffer
 *  \param bReference true if the written parameter should be a reference
 *  \param bLValue true if param is l-Value
 *  \return true if successful (anything marshaled)
 *
 * If we call this function, we get the return value true if a parameter
 * has been written to the given position. If the return value is false,
 * no parameter could be written to this position. The calling function
 * should try to write at the next position.
 *
 * Using the message buffer struct of first machine word sized parameters and
 * then smaller ones, there should be machine word sized parameters to start
 * marshaling with. If there are not enough machine word sized parameters a
 * special struct should be added to the union, containing only word sized
 * members. So if we cannot find a word sized parameter at the position, we
 * try the special struct with the word sized members.
 */
bool
CL4BEMarshaller::PositionMarshaller::Marshal(CBEFile *pFile, 
	CBEFunction *pFunction, 
	CMsgStructType nType, 
	int nPosition,
	bool bReference,
	bool bLValue)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"PositionMarshaller::%s(%s, %s, %d, %d, %s, %s) called\n", __func__,
	pFile->GetFileName().c_str(), pFunction ? pFunction->GetName().c_str() : "", 
	(int)nType, nPosition, bReference ? "true" : "false",
	bLValue ? "true" : "false");

    m_nPosSize = CCompiler::GetSizes()->GetSizeOfType(TYPE_MWORD);
    m_bReference = bReference;
    m_pFile = pFile;
    
    // get the message buffer type
    CBEMsgBufferType *pMsgType = GetMessageBufferType(pFunction);

    // get the respective struct from the function
    string sFuncName = pFunction->GetOriginalName();
    string sClassName = pFunction->GetSpecificParent<CBEClass>()->GetName();
    // if name is empty, get generic struct
    if (sFuncName.empty())
    {
	nType = CMsgStructType::Generic;
	sClassName = string();
    }
    CBEStructType *pStruct = pMsgType->GetStruct(sFuncName, sClassName, 
	nType);
    if (!pStruct)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	    "PositionMarshaller::%s failed, because no struct could be found for func %s\n",
	    __func__, pFunction->GetName().c_str());
	return false;
    }
    assert(pStruct);

    CBETypedDeclarator *pMember = 
	GetMemberAt(pMsgType, pStruct, nPosition);
    if (!pMember)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	    "PositionMarshaller::%s: could not find a member at pos %d in struct\n",
	    __func__, nPosition);
	// struct too small, nothing to put there
	return false;
    }

    // do not get message buffer as parent of type, but instead get it from
    // function to obtain the correct declarator of the function's scope
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);
    // Now we test if member if of word size. If not, get the struct with the
    // word sized members. We also have to test if its type is simple, since
    // constructed types cannot be put directly into positions.
    int nMemberSize = GetMemberSize(pMember);
    if (nMemberSize != m_nPosSize)
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
	    "PositionMarshaller::%s: member of different size\n", __func__);
	if (bReference)
	    *pFile << "&";
	pMsgBuffer->WriteGenericMemberAccess(pFile, nPosition);
    }
    else
    {
	// find respective parameter
	string sName = pMember->m_Declarators.First()->GetName();
	CBETypedDeclarator *pParameter = pFunction->FindParameter(sName);
	if (!pParameter)
	{
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
		"PositionMarshaller::%s: parameter %s not found, try special\n", __func__,
		sName.c_str());
	    // only call this method if we are sure the member is special
	    // e.g. return, opcode, etc.
	    WriteSpecialMember(pFile, pFunction, pMember, nType, 
		bReference, bLValue);
	}
	else
	{
	    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, 
		"PositionMarshaller::%s: parameter found, write access to member\n", __func__);
	    // if there is a transmit_as attribute, replace the type
	    CBEAttribute *pAttr = pParameter->m_Attributes.Find(ATTR_TRANSMIT_AS);
	    // if user defined type, then the alias might have a transmit as
	    // attribute as well (the parameter's transmit-as overrides the
	    // one of the alias)
	    CBEType *pParamType = pParameter->GetType();
	    if (!pAttr)
	    {
		CBERoot *pRoot = pParameter->GetSpecificParent<CBERoot>();
		assert(pRoot);
		
		while (!pAttr && pParamType->IsOfType(TYPE_USER_DEFINED))
		{
		    string sTypeName = 
			static_cast<CBEUserDefinedType*>(pParamType)->GetName();
		    CBETypedef *pTypedef = pRoot->FindTypedef(sTypeName);
		    if (pTypedef)
		    {
			pAttr = pTypedef->m_Attributes.Find(ATTR_TRANSMIT_AS);
			pParamType = pTypedef->GetType();
		    }
		}
	    }
	    CBEType *pTransmitType = 0;
	    if (pAttr)
	    {
		// get type from attribute
		pTransmitType = static_cast<CBEType*>(pAttr->GetAttrType());
	    }
	    
	    /* if member is constructed type, cast to word size 
	     * test for flexpage explicetly, because in C its a constructed
	     * type but internally handled as simple type
	     */
	    if ((pMember->GetType()->IsConstructedType() ||
		    pMember->GetType()->IsOfType(TYPE_FLEXPAGE)) &&
		!pTransmitType)
	    {
		CBEClassFactory *pCF = CCompiler::GetClassFactory();
		pTransmitType = pCF->GetNewType(TYPE_MWORD);
		pTransmitType->CreateBackEnd(true, m_nPosSize, TYPE_MWORD);
		/* set lval so that the cast below is actually performed */
		bLValue = true;
	    }

	    // get member type (possibly a user defined type as well)
	    CBEType *pMemType = pMember->GetType();
	    while (pMemType->IsOfType(TYPE_USER_DEFINED))
		pMemType =
		    static_cast<CBEUserDefinedType*>(pMemType)->GetRealType();
	    // if member and parameter have different types, rely on member
	    // exception is if transmit type exists, then cast parameter to
	    // transmit type (is not always == member type)
	    if (!pMemType->IsOfType(pParamType->GetFEType()) &&
		!pTransmitType)
	    {
		if (bReference)
    		    *pFile << "&";
		pMsgBuffer->WriteGenericMemberAccess(pFile, nPosition);
	    }
	    else
	    {
		// if there is an attribute we have a possibly different type.
		// Just to be on the safe side, cast the types
		//
		// We do so by letting WriteParameter handle the cast. This is
		// necessary, so all the special cases, such as constructed
		// types and pointer types are cought. This might look
		// nasty for simple types, such as float transmitted as long
		// and generate wrong type casts. Therefore, we have to check
		// for simple types here.
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		    "PositionMarshaller::%s: bReference=%s, pTransmitType=%p, C++=%s, bLValue=%s\n",
		    __func__, bReference ? "true" : "false", pTransmitType, 
		    CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) ? "true" : "false",
		    bLValue ? "true" : "false");
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		    "PositionMarshaller::%s: !%s && %p && !(%s && %s)\n", __func__,
		    bReference ? "true" : "false", pTransmitType, 
		    CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) ? "true" : "false",
		    bLValue ? "true" : "false");
		if (!bReference && pTransmitType &&
		    !(CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
			bLValue))
		{
		    /* test for that flexpage again */
		    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
			"PositionMarshaller::%s: %s && %s && !%s\n", __func__,
			pMemType->IsSimpleType() ? "true" : "false",
			pParamType->IsSimpleType() ? "true" : "false",
			pMemType->IsOfType(TYPE_FLEXPAGE) ? "true" : "false");
		    if (pMemType->IsSimpleType() &&
			pParamType->IsSimpleType() &&
			!pMemType->IsOfType(TYPE_FLEXPAGE))
			pMemType->WriteCast(pFile, false);
		    else
		    {
			*pFile << "*";
			bReference = true;
		    }
		}
		WriteParameter(pFile, pParameter, bReference, bLValue);
	    }
	}
    }

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "PositionMarshaller::%s returns true\n", __func__);
    return true;
}

/** \brief retrieve the type of the message buffer 
 *  \param pFunction the function to get the type for
 *  \return a reference to the message buffer type
 *
 * This implementation returns the type with the structs, not some alias (as
 * in user defined type).
 */
CBEMsgBufferType*
CL4BEMarshaller::PositionMarshaller::GetMessageBufferType(
    CBEFunction *pFunction)
{
    CBEMsgBuffer *pMsgBuffer;
    if (dynamic_cast<CBEInterfaceFunction*>(pFunction))
    {
	CBEClass *pClass = pFunction->GetSpecificParent<CBEClass>();
	assert(pClass);
	pMsgBuffer = pClass->GetMessageBuffer();
    }
    else
	pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);
    CBEMsgBufferType *pType = 
	dynamic_cast<CBEMsgBufferType*>(pMsgBuffer->GetType());
    assert (pType);
    return pType;
}

/** \brief retrieve the member at a given position
 *  \param pType the message buffer type
 *  \param pStruct the struct to search
 *  \param nPosition the position to get the member from
 *  \return a reference to the member (if there is any)
 */
CBETypedDeclarator*
CL4BEMarshaller::PositionMarshaller::GetMemberAt(CBEMsgBufferType *pType,
    CBEStructType *pStruct,
    int nPosition)
{
    int nCurSize = 0, nMemberSize;
    // set position size if not set yet
    if (m_nPosSize == 0)
	m_nPosSize = CCompiler::GetSizes()->GetSizeOfType(TYPE_MWORD);
    // try to find the member for the position
    vector<CBETypedDeclarator*>::iterator iter;
    for (iter =	pType->GetStartOfPayload(pStruct);
	 iter != pStruct->m_Members.end();
	 iter++)
    {
	// direction fits, since this is the struct of our desired direction
	nMemberSize = GetMemberSize(*iter);
	// if we cross the border of the parameter position we want to write,
	// stop
	if (nPosition*m_nPosSize < (nCurSize + nMemberSize))
	    break;
	nCurSize += nMemberSize;
    }
    if (iter == pStruct->m_Members.end())
	return (CBETypedDeclarator*)0;
    return *iter;
}

/** \brief get the size of a member
 *  \param pMember the member to get the size from
 *  \return the size in bytes
 *
 * This implementation wraps the GetSize method with some side checking
 * relevant here.
 */
int
CL4BEMarshaller::PositionMarshaller::GetMemberSize(CBETypedDeclarator *pMember)
{
    int nMemberSize = pMember->GetSize();
    // if size is negative (a pointer) test for size attributes, which
    // would indicate an array. If not, dereference it.
    if (nMemberSize < 0)
    {
	if (pMember->m_Attributes.Find(ATTR_SIZE_IS) ||
	    pMember->m_Attributes.Find(ATTR_LENGTH_IS) ||
	    pMember->m_Attributes.Find(ATTR_MAX_IS))
	    return nMemberSize;
	nMemberSize = pMember->GetType()->GetSize();
    }
    return nMemberSize;
}

/** \brief tests for and writes special members, such as opcode, exception
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pMember the special member to write
 *  \param nType the type of the struct
 *  \param bReference true if member should be referenced
 *  \param bLValue true if the parameter is an l-Value
 *
 * This method first tests for special members, such as the opcode. Such
 * special parameters might be treated differently (e.g. use the opcode
 * constant directly). 
 * Otherwise (no known special parameter) the member of the struct has to be 
 * used, because this method is only called if we are sure that the given
 * parameter is special.
 */
void
CL4BEMarshaller::PositionMarshaller::WriteSpecialMember(CBEFile *pFile,
    CBEFunction *pFunction,
    CBETypedDeclarator *pMember,
    CMsgStructType nType,
    bool bReference,
    bool bLValue)
{
    // do not get message buffer as parent of type, but instead get it from
    // function to obtain the correct declarator of the function's scope
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);
    
    // test for opcode (only if reference false, otherwise use struct member)
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    CBEDeclarator *pDecl = pMember->m_Declarators.First();
    if ((pDecl->GetName() == pNF->GetOpcodeVariable()) &&
	!bReference)
    {
	*pFile <<  pFunction->GetOpcodeConstName();
	return;
    }

    // if constructed type, then do pointer cast
    // (we have to test for flexpage type explicetly, because it is of size
    // word, in C declared as constructed type, but internally handled as
    // simple type)
    // But do not dereference if bReference is set
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"PositionMarshaller::%s(%s, %s, %s, %d, %s, %s)\n", __func__,
	pFile->GetFileName().c_str(), pFunction->GetName().c_str(),
	pDecl->GetName().c_str(), (int)nType,
	bReference ? "true" : "false",
	bLValue ? "true" : "false");
    if ((pMember->GetType()->IsConstructedType() ||
	    pMember->GetType()->IsOfType(TYPE_FLEXPAGE)) &&
	!bReference)
    {
	*pFile << "*";
	bReference = true;
    }

    // cast to mword if the parameters type is not exactly mword
    if (!pMember->GetType()->IsOfType(TYPE_MWORD) &&
	!bLValue)
    {
	string sName = pNF->GetTypeName(TYPE_MWORD, true);
	*pFile << "(" << sName;
	if (bReference)
	    *pFile << "*";
	*pFile << ")";
    }
    if (bReference)
	*pFile << "&";
    // test for return variable
    CBETypedDeclarator *pReturn = pFunction->GetReturnVariable();
    if (pReturn && pReturn->m_Declarators.Find(pDecl->GetName()))
    {
	pReturn = pFunction->m_LocalVariables.Find(pDecl->GetName());
	*pFile << pReturn->m_Declarators.First()->GetName();	
	return;
    }

    // no special member we know of: access in struct
    assert(m_pParent);
    m_pParent->WriteMember(nType, pMsgBuffer, pMember, NULL);
}

/** \brief writes the access to a parameter
 *  \param pFile the file to write the access to
 *  \param pParameter the parameter to access
 *  \param bReference true if this should be a reference
 *  \param bLValue true if the parameter to marshal is an l-value
 *
 * This method is only invoked if the member is of word size.  If we have to
 * write a referenced access we have to find out if it is a simple type, so we
 * can access it easily, or whether we have to cast it to a simple type to
 * access it.
 *
 * This method is a special case for CBEMarshaller::WriteParameter for word
 * sized members!
 *
 * In case of a reference we might have to cast if the parameter has the same
 * size but is for instance different in sign.
 */
void
CL4BEMarshaller::PositionMarshaller::WriteParameter(CBEFile *pFile,
    CBETypedDeclarator *pParameter,
    bool bReference,
    bool bLValue)
{
    CBEDeclarator *pDecl = pParameter->m_Declarators.First();
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"PositionMarshaller::%s(, %s, ref:%s, lval:%s) called\n", __func__,
	pDecl->GetName().c_str(), bReference ? "true" : "false",
	bLValue ? "true" : "false");
    // get declarator
    int nStars = pDecl->GetStars();
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"PositionMarshaller::%s stars(1)=%d\n", __func__, nStars);
    if (bReference)
	nStars--;
    // get function and check for additional references
    CBEFunction *pFunction = pParameter->GetSpecificParent<CBEFunction>();
    if (pFunction && pFunction->HasAdditionalReference(pDecl, false))
	nStars++;
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"PositionMarshaller::%s stars(2)=%d\n", __func__, nStars);
    // check type equality
    // (is sufficient for reference, that is, pointers. Simple types are
    // casted automatically. Compiler only complaints about mismatching
    // pointers.
    // Do not cast an l-Value in C++
    if (bReference &&
	!(CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP) &&
	    bLValue))
    {
	CBEType *pType = pParameter->GetType();
	int nFEType = pType->GetFEType();
	bool bUnsigned = pType->IsUnsigned();
	if (!(bUnsigned && (nFEType == TYPE_MWORD)))
	{
	    CBEClassFactory *pCF = CCompiler::GetClassFactory();
	    CBEType *pMType = pCF->GetNewType(TYPE_MWORD);
	    pMType->CreateBackEnd(true, 0, TYPE_MWORD);
	    pMType->WriteCast(pFile, true);
	    delete pMType;
	}
    }
    // create reference if necessary
    // create reference after writing type
    if (nStars < 0)
    {
	if (nStars < -1)
	    CMessages::Error("Cannot create more than one reference to" \
		" parameter %s (%s: %d).\n", pDecl->GetName().c_str(),
		__FILE__, __LINE__);
	
	*pFile << "&";
    }
    // dereference if necessary
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"PositionMarshaller::%s stars(3)=%d\n", __func__, nStars);
    for (int nIndx = 0; nIndx < nStars; nIndx++)
	*pFile << "*";
    // print name
    *pFile << pDecl->GetName();
    
    // FIXME test array
    // FIXME test complex type?

    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"PositionMarshaller::%s returns\n", __func__);
}


/**
 * \file    dice/src/be/BEStructType.cpp
 * \brief   contains the implementation of the class CBEStructType
 *
 * \date    01/15/2002
 * \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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

#include "BEStructType.h"
#include "BEUserDefinedType.h"
#include "BEContext.h"
#include "BEFile.h"
#include "BETypedef.h"
#include "BEDeclarator.h"
#include "BERoot.h"
#include "BEAttribute.h"
#include "BEStructType.h"
#include "BEClassFactory.h"
#include "BESizes.h"
#include "BEUnionType.h"
#include "Compiler.h"
#include "Error.h"
#include "fe/FEStructType.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"
#include "fe/FEFile.h"
#include "fe/FEArrayType.h"
#include "fe/FESimpleType.h"
#include "fe/FEIsAttribute.h"
#include "fe/FEDeclarator.h"
#include <stdexcept>
#include <cassert>
#include <sstream>

CBEStructType::CBEStructType()
: m_sTag(),
	m_Members(0, this)
{
	m_bForwardDeclaration = false;
}

CBEStructType::CBEStructType(CBEStructType* src)
: CBEType(src),
	m_Members(src->m_Members)
{
	m_sTag = src->m_sTag;
	m_bForwardDeclaration = src->m_bForwardDeclaration;
	m_Members.Adopt(this);
}

/** \brief destructor of this instance */
CBEStructType::~CBEStructType()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CObject* CBEStructType::Clone()
{
	return new CBEStructType(this);
}

/** \brief prepares this instance for the code generation
 *  \param pFEType the corresponding front-end attribute
 *  \return true if the code generation was successful
 *
 * This implementation calls the base class' implementatio first to set
 * default values and then adds the members of the struct to this class.
 */
void
CBEStructType::CreateBackEnd(CFETypeSpec * pFEType)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEStructType::%s(fe) called\n", __func__);

	// sets m_sName to "struct"
	CBEType::CreateBackEnd(pFEType);
	// if sequence create own members
	if (pFEType->GetType() == TYPE_ARRAY)
	{
		CreateBackEndSequence((CFEArrayType*)pFEType);
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s(fe) returns\n", __func__);
		return;
	}
	// get forward declaration
	m_bForwardDeclaration =
		((CFEConstructedType*) pFEType)->IsForwardDeclaration();
	CBEClassFactory *pCF = CCompiler::GetClassFactory();
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	// iterate over members
	CFEStructType *pFEStruct = (CFEStructType *) pFEType;
	vector<CFETypedDeclarator*>::iterator iterM;
	for (iterM = pFEStruct->m_Members.begin();
		iterM != pFEStruct->m_Members.end();
		iterM++)
	{
		CBETypedDeclarator *pMember = pCF->GetNewTypedDeclarator();
		m_Members.Add(pMember);
		pMember->CreateBackEnd(*iterM);
	}
	// set tag
	string sTag = pFEStruct->GetTag();
	if (!sTag.empty())
	{
		// we start with the parent interface and walk all the way up to the
		// root
		CFEConstructedType *pFETaggedDecl = 0;
		CFEInterface *pFEInterface = pFEType->GetSpecificParent<CFEInterface>();
		if (pFEInterface)
		{
			pFETaggedDecl = pFEInterface->m_TaggedDeclarators.Find(sTag);
			if (!pFETaggedDecl)
			{
				CFELibrary *pParentLib =
					pFEInterface->GetSpecificParent<CFELibrary>();
				while (pParentLib && !pFETaggedDecl)
				{
					pFETaggedDecl = pParentLib->FindTaggedDecl(sTag);
					pParentLib = pParentLib->GetSpecificParent<CFELibrary>();
				}
			}
		}
		if (!pFETaggedDecl)
		{
			CFEFile *pFERoot = pFEType->GetRoot();
			// we definetly have a root
			assert(pFERoot);
			// we definetly have this decl in there
			pFETaggedDecl = pFERoot->FindTaggedDecl(sTag);
		}
		// now we can assign a global tag name
		if (pFETaggedDecl)
			m_sTag = pNF->GetTypeName(pFETaggedDecl, sTag);
		else
		{
			// if this is a complete type, than this should
			// be made a full name as well, since it is defined in
			// an idl file
			if (!m_bForwardDeclaration)
				m_sTag = pNF->GetTypeName(pFEType, sTag);
			else
				m_sTag = sTag;
		}
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEStructType::%s(fe) returns\n", __func__);
}

/** \brief initialize instance of object
 *  \param sTag the tag of the struct
 *  \param pRefObj a reference object
 *  \return true if successful
 *
 * The members are added later using AddMember.
 */
void
CBEStructType::CreateBackEnd(string sTag,
	CFEBase *pRefObj)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEStructType::%s(%s, %p) called\n", __func__,
		sTag.c_str(), pRefObj);
	// skip CBEType::CreateBackEnd to avoid asserts
	CBEObject::CreateBackEnd(pRefObj);
	m_nFEType = TYPE_STRUCT;

	CBENameFactory *pNF = CCompiler::GetNameFactory();
	m_sName = pNF->GetTypeName(TYPE_STRUCT, false);
	m_sTag = sTag;

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEStructType::%s(%s,) called\n", __func__, sTag.c_str());
}

/** \brief prepares this instance for the code generation
 *  \param pFEType the corresponding front-end type
 *  \return true if the code generation was successful
 */
void
CBEStructType::CreateBackEndSequence(CFEArrayType * pFEType)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEStructType::%s(fe-array) called\n", __func__);
	// if sequence create own members
	if (pFEType->GetType() != TYPE_ARRAY)
	{
		string exc = string(__func__);
		exc += " failed, because array type is no array type";
		throw new error::create_error(exc);
	}
	// CLM states that (1.11)
	// that 'sequence <type, size>' will be mapped to
	// struct {
	// unsigned long _maximum;
	// unsigned long _length;
	// type* _buffer;
	// }
	// we extend this to add the attributes max_is and length_is to buffer, so
	// the marshaller can perform range checks.

	// the member vector
	vector<CFETypedDeclarator*> *pMembers = new vector<CFETypedDeclarator*>();

	// create the _maximum member
	CBESizes *pSizes = CCompiler::GetSizes();
	int nLongSize = pSizes->GetSizeOfType(TYPE_LONG, 4);
	CFETypeSpec* pFEMType = new CFESimpleType(TYPE_INTEGER, true,
		true, nLongSize, false);
	CFEDeclarator *pFEDeclarator = new CFEDeclarator(DECL_IDENTIFIER,
		string("_maximum"));
	vector<CFEDeclarator*> *pFEDeclarators = new vector<CFEDeclarator*>();
	pFEDeclarators->push_back(pFEDeclarator);
	CFETypedDeclarator *pFEMember = new CFETypedDeclarator(TYPEDECL_FIELD,
		pFEMType,
		pFEDeclarators);
	pFEMType->SetParent(pFEMember);
	pFEDeclarator->SetParent(pFEMember);
	pMembers->push_back(pFEMember);
	pFEDeclarators->clear();

	// create _length member
	pFEMType = new CFESimpleType(TYPE_INTEGER, true,
		true, nLongSize, false);
	pFEDeclarator = new CFEDeclarator(DECL_IDENTIFIER, string("_length"));
	pFEDeclarators->push_back(pFEDeclarator);
	pFEMember = new CFETypedDeclarator(TYPEDECL_FIELD,
		pFEMType,
		pFEDeclarators);
	pFEMType->SetParent(pFEMember);
	pFEDeclarator->SetParent(pFEMember);
	pMembers->push_back(pFEMember);
	pFEDeclarators->clear();

	// add attributes
	// attribute [max_is(_maximum)]
	vector<CFEAttribute*> *pAttributes = new vector<CFEAttribute*>();
	pFEDeclarator = new CFEDeclarator(DECL_IDENTIFIER, string("_maximum"));
	vector<CFEDeclarator*> *pAttrParams = new vector<CFEDeclarator*>();
	pAttrParams->push_back(pFEDeclarator);
	CFEAttribute *pFEAttribute = new CFEIsAttribute(ATTR_MAX_IS,
		pAttrParams);
	delete pAttrParams;
	pFEDeclarator->SetParent(pFEAttribute);
	pAttributes->push_back(pFEAttribute);
	// attribute [length_is(_length)]
	pFEDeclarator = new CFEDeclarator(DECL_IDENTIFIER, string("_length"));
	pAttrParams = new vector<CFEDeclarator*>();
	pAttrParams->push_back(pFEDeclarator);
	pFEAttribute = new CFEIsAttribute(ATTR_LENGTH_IS,
		pAttrParams);
	delete pAttrParams;
	pFEDeclarator->SetParent(pFEAttribute);
	pAttributes->push_back(pFEAttribute);
	// create the *_buffer member
	pFEMType = pFEType->GetBaseType();
	pFEDeclarator = new CFEDeclarator(DECL_IDENTIFIER, string("_buffer"), 1);
	pFEDeclarators->push_back(pFEDeclarator);
	pFEMember = new CFETypedDeclarator(TYPEDECL_FIELD,
		pFEMType, pFEDeclarators, pAttributes);
	pFEMType->SetParent(pFEMember);
	pFEDeclarator->SetParent(pFEMember);
	pMembers->push_back(pFEMember);
	pFEDeclarators->clear();

	// create struct
	CFEStructType *pFEStruct = new CFEStructType(string(), pMembers);
	pFEStruct->SetParent(pFEType->GetParent());
	delete pMembers;
	delete pAttributes;

	// recusively call CreateBackEnd to initialize struct
	CreateBackEnd(pFEStruct);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEStructType::%s(fe-array) returns\n", __func__);
}

/** \brief writes the structure into the target file
 *  \param pFile the target file
 *
 * A struct looks like this:
 * <code>
 * struct \<tag\>
 * {
 *   \<member list\>
 * }
 * </code>
 */
void CBEStructType::Write(CBEFile& pFile)
{
	if (!pFile.is_open())
		return;

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called.\n", __func__);
	// open struct
	pFile << m_sName;
	if (!m_sTag.empty())
		pFile << " " << m_sTag;
	if (GetMemberCount() > 0)
	{
		pFile << "\n";
		pFile << "\t{\n";
		++pFile;
		// print members
		vector<CBETypedDeclarator*>::iterator iter;
		for (iter = m_Members.begin();
			iter != m_Members.end();
			iter++)
		{
			// this might happend (if we add return types to a struct)
			if ((*iter)->GetType()->IsVoid() &&
				((*iter)->GetSize() == 0))
				continue;
			pFile << "\t";
			(*iter)->WriteDeclaration(pFile);
			pFile << ";\n";
		}
		// close struct
		--pFile << "\t}";
	}
	else if (!m_bForwardDeclaration)
	{
		pFile << " { }";
	}
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returned.\n", __func__);
}

/** \brief calculates the size of the written string
 *  \return the length of the written string
 *
 * This function is used to see how long the type of a parameter (or return type) is.
 * Thus no members are necessary, but we have to consider the tag if it exists.
 */
int CBEStructType::GetStringLength()
{
	int nSize = m_sName.length();
	if (!m_sTag.empty())
		nSize += m_sTag.length();
	return nSize;
}

/** \brief calculate the size of a struct type
 *  \return the size in bytes of the struct
 *
 * The struct's size is the sum of the member's size.
 *
 * If the declarator's GetSize returns 0, the member is a bitfield. We have to
 * add the bits of the declarators. If a bitfield declarator is followed by a
 * non-bitfield declarator, the size has to be aligned to the next byte.
 *
 * \todo If member is indirect, we should add size of pointer instead of \
 *       size of type
 *
 * Do not use cached size of struct, because it might change
 */
int CBEStructType::GetSize()
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEStructType::%s called\n", __func__);

	int nSize = 0;
	// if this is a tagged struct without members, we have to find the
	// original struct
	if (m_bForwardDeclaration)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s forward decl of %s (@ %p)\n", __func__,
			GetTag().c_str(), this);

		// search for tag
		CBERoot *pRoot = GetSpecificParent<CBERoot>();
		assert(pRoot);
		CBEStructType *pTaggedType =
			(CBEStructType*)pRoot->FindTaggedType(TYPE_STRUCT, GetTag());
		// if found, get size from it
		if ((pTaggedType) && (pTaggedType != this))
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBEStructType::%s get size from tagged type\n", __func__);
			nSize = pTaggedType->GetSize();
			CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
				"CBEStructType::%s returns %d\n", __func__, nSize);
			return nSize;
		}
		// if no tagged struct found, this is a user defined type
		// get the size from there
		if (!pTaggedType)
		{
			CBETypedef *pTypedef = pRoot->FindTypedef(m_sTag);
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBEStructType::%s found typedef @ %p\n", __func__, pTypedef);
			if (pTypedef && pTypedef->GetTransmitType() == this)
			{
				CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
					"CBEStructType::%s typedef's type %p is this %p\n", __func__,
					pTypedef->GetTransmitType(), this);
				pTypedef = pRoot->FindTypedef(m_sTag, pTypedef);
				CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
					"CBEStructType::%s next typedef @ %p\n", __func__, pTypedef);
			}
			if (!pTypedef)
			{
				CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
					"CBEStructType::%s returns 0\n", __func__);
				return 0;
			}
			CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
				"CBEStructType::%s get size from typedef type\n", __func__);
			/* since the typedef is a CBETypedDeclarator, it would evaluate
			 * the size of it's base type and sum it for all it's declarators.
			 * We only want it for the declarator we are using. That's why we
			 * use a specific GetSize function instead of the generic one. */
			return pTypedef->GetSize(m_sTag);
		}
	}

	int nBitSize = 0;
	int __loop = 1;
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Members.begin();
		iter != m_Members.end();
		iter++)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s at member %d, type at %p (%d), first decl at %p\n",
			__func__, __loop++, (*iter)->GetType(),
			(*iter)->GetType() ? (*iter)->GetType()->GetFEType() : 0,
			(*iter)->m_Declarators.First());
		CBEDeclarator *pDecl = (*iter)->m_Declarators.First();
		// do not assert pDecl, but check later to allow anonym unions/structs
		// as members of the struct
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s testing member %s\n",
			__func__, pDecl ? pDecl->GetName().c_str() : "(anonym)");

		// special case handling:
		// if the member is a pointer to ourself, then we
		// handle this as a variable sized entry.
		// example:
		// struct list {
		//     struct list *prev, *next;
		//     ...
		// };
		// To catch this we test if the type of the
		// member is a tagged struct type with the same
		// tag as we have.
		//
		// another special case:
		// typedef struct A A_t;
		// struct A {
		//   A_t *next;
		// };
		//
		// To catch this, we hav to check if the type is
		// a user defined type. If so, we get the original
		// type, which should be the struct, and then check
		// if it has the same tag.
		CBEType *pMemberType = (*iter)->GetType();

		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s member %s is of type %d\n",
			__func__, pDecl ? pDecl->GetName().c_str() : "(anonym)",
			pMemberType->GetFEType());

		while (dynamic_cast<CBEUserDefinedType*>(pMemberType) &&
			static_cast<CBEUserDefinedType*>(pMemberType)->GetRealType())
			pMemberType = ((CBEUserDefinedType*)pMemberType)->GetRealType();
		assert(pMemberType);

		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s member %s has real type %d\n",
			__func__, pDecl ? pDecl->GetName().c_str() : "(anonym)",
			pMemberType->GetFEType());

		if ((dynamic_cast<CBEStructType*>(pMemberType)) &&
			pMemberType->HasTag(m_sTag) &&
			!m_sTag.empty())
		{
			// FIXME: get size from
			// CCompiler::GetSizes()->GetSizeOfType(TYPE_MWORD);
			CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
				"CBEStructType::%s self reference (%s): return -1\n",
				__func__, m_sTag.c_str());
			return -1;
		}

		int nElemSize = 0;
		if (pDecl)
			nElemSize = (*iter)->GetSize();
		else
			nElemSize = pMemberType->GetSize();
		if ((*iter)->IsString())
		{
			// a string is also variable sized member
			CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
				"CBEStructType::%s string member: return -1\n", __func__);
			return -1;
		}
		else if ((*iter)->IsVariableSized())
		{
			// if one of the members is variable sized,
			// the whole struct is variable sized
			CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
				"CBEStructType::%s var-sized member: return -1\n",
				__func__);
			return -1;
		}
		else if (nElemSize == 0)
		{
			nBitSize += (*iter)->GetBitfieldSize();
		}
		else
		{
			CBESizes *pSizes = CCompiler::GetSizes();
			// check for alignment:
			// if current size (nSize) is 4 bytes or above then sum is aligned
			// to dword size
			//
			// if current size (nSize) is 2 bytes then sum is aligned to word
			// size
			if (nElemSize >= 4)
				nSize = pSizes->WordRoundUp(nSize);
			if ((nElemSize == 2) && ((nSize % 2) > 0))
				nSize++; // word align
			nSize += nElemSize;
			// if bitfields before, align them and add them
			if (nBitSize > 0)
			{
				nSize += nBitSize / 8;
				if ((nBitSize % 8) > 0)
					nSize++;
				nBitSize = 0;
			}
		}

		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s size of member is %d, new total: %d\n",
			__func__, nElemSize, nSize);
	}
	// some bitfields left? -> align them and add them
	if (nBitSize > 0)
	{
		nSize += nBitSize / 8;
		if ((nBitSize % 8) > 0)
			nSize++;
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEStructType::%s returns %d\n", __func__, nSize);
	return nSize;
}

/** \brief returns the maximum size of the struct
 *  \return the size in bytes
 *
 * First it calls the GetSize method. If that returns a negative value, it is
 * variable sized and a maximum size calculation is feasable. The algorithm is
 * basically the same as for GetSize, but when a member returns -1 (variable
 * sized) then this implementation tries to determine it's maximum size.
 *
 * Do not use cached max size, because size might change.
 */
int CBEStructType::GetMaxSize()
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEStructType::%s called\n", __func__);

	int nMaxSize = GetSize();
	if (nMaxSize > 0)
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s normal size is %d, return\n", __func__,
			nMaxSize);
		return nMaxSize;
	}

	// if this is a tagged struct without members, we have to find the
	// original struct
	if (m_bForwardDeclaration)
	{
		// search for tag
		CBERoot *pRoot = GetSpecificParent<CBERoot>();
		assert(pRoot);
		CBEStructType *pTaggedType =
			(CBEStructType*)pRoot->FindTaggedType(TYPE_STRUCT, GetTag());
		// if found, marshal this instead
		if ((pTaggedType) && (pTaggedType != this))
		{
			nMaxSize = pTaggedType->GetMaxSize();
			CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
				"CBEStructType::%s size of forward decl struct is %d\n",
				__func__, nMaxSize);
			return nMaxSize;
		}
		// if no tagged struct found, this is a user defined type
		// we asked the sizes class in CreateBackEnd, maybe it knows my size
		// -> nMaxSize should be > 0
	}

	CBESizes *pSizes = CCompiler::GetSizes();

	int nBitSize = 0;
	// reset nMaxSize, which could have been negative before
	nMaxSize = 0;

	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Members.begin();
		iter != m_Members.end();
		iter++)
	{
		CBEDeclarator *pDecl = (*iter)->m_Declarators.First();
		// do not assert pDecl, but check later to allow anonym unions/structs
		// as members of the struct
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s testing member %s\n",
			__func__, pDecl ? pDecl->GetName().c_str() : "(anonym)");
		// special case handling:
		// if the member is a pointer to ourself, then we
		// handle this as a variable sized entry.
		// example:
		// struct list {
		//     struct list *prev, *next;
		//     ...
		// };
		// To catch this we test if the type of the
		// member is a tagged struct type with the same
		// tag as we have.
		//
		// another special case:
		// typedef struct A A_t;
		// struct A {
		//   A_t *next;
		// };
		//
		// To catch this, we hav to check if the type is
		// a user defined type. If so, we get the original
		// type, which should be the struct, and then check
		// if it has the same tag.
		CBEType *pMemberType = (*iter)->GetType();

		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s member %s is of type %d\n",
			__func__, pDecl ? pDecl->GetName().c_str() : "(anonym)",
			pMemberType->GetFEType());

		while (dynamic_cast<CBEUserDefinedType*>(pMemberType) &&
			static_cast<CBEUserDefinedType*>(pMemberType)->GetRealType())
			pMemberType = ((CBEUserDefinedType*)pMemberType)->GetRealType();
		assert(pMemberType);

		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s member %s has real type %d\n",
			__func__, pDecl ? pDecl->GetName().c_str() : "(anonym)",
			pMemberType->GetFEType());

		if ((dynamic_cast<CBEStructType*>(pMemberType)) &&
			pMemberType->HasTag(m_sTag))
		{
			nMaxSize = pSizes->GetSizeOfType(TYPE_VOID_ASTERISK);
			CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
				"CBEStructType::%s pointer to self, return %d\n",
				__func__, nMaxSize);
			return nMaxSize;
		}

		int nSize = 0;
		if (!(*iter)->GetMaxSize(nSize))
		{
			nMaxSize = -1;
			CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
				"CBEStructType::%s can't determine max of member, return -1\n",
				__func__);
			return -1;
		}
		else if (nSize == 0)
		{
			nBitSize += (*iter)->GetBitfieldSize();
		}
		else
		{
			// check for alignment:
			// if current size (nSize) is 4 bytes or above then sum is aligned
			// to dword size
			//
			// if current size (nSize) is 2 bytes then sum is aligned to word
			// size
			int nWordSize = pSizes->GetSizeOfType(TYPE_MWORD);
			if (nSize >= nWordSize)
				nMaxSize = pSizes->WordRoundUp(nMaxSize);
			nMaxSize += nSize;
			// if bitfields before, align them and add them
			if (nBitSize > 0)
			{
				nMaxSize += nBitSize / 8;
				if ((nBitSize % 8) > 0)
					nMaxSize++;
				nBitSize = 0;
			}
		}

		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEStructType::%s: member %s has size %d (bits %d) - total: %d\n",
			__func__, pDecl ? pDecl->GetName().c_str() : "(anonym)",
			nSize, nBitSize, nMaxSize);
	}
	// some bitfields left? -> align them and add them
	if (nBitSize > 0)
	{
		nMaxSize += nBitSize / 8;
		if ((nBitSize % 8) > 0)
			nMaxSize++;
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEStructType::%s return max size %d\n",
		__func__, nMaxSize);
	return nMaxSize;
}

/** \brief writes code to initialize a variable of this type with a zero value
 *  \param pFile the file to write to
 *
 * A struct is usually initialized by writing a init value for all its members
 * in a comma seperated list, embraced by braces. E.g. { (CORBA_long)0,
 * (CORBA_float)0 }
 */
void CBEStructType::WriteZeroInit(CBEFile& pFile)
{
	pFile << "{ ";
	++pFile;
	bool bComma = false;
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Members.begin();
		iter != m_Members.end();
		iter++)
	{
		// get type
		CBEType *pType = (*iter)->GetType();
		// get declarator
		vector<CBEDeclarator*>::iterator iterD;
		for (iterD = (*iter)->m_Declarators.begin();
			iterD != (*iter)->m_Declarators.end();
			iterD++)
		{
			if (bComma)
				pFile << ", \n\t";
			// be C99 compliant:
			pFile << (*iterD)->GetName() << " : ";
			if ((*iterD)->IsArray())
				WriteZeroInitArray(pFile, pType, *iterD,
					(*iterD)->m_Bounds.begin());
			else if (pType)
				pType->WriteZeroInit(pFile);

			bComma = true;
		}
	}
	--pFile << " }";
}

/** \brief checks if this is a constructed type
 *  \return true, because this is a constructed type
 */
bool CBEStructType::IsConstructedType()
{
	return true;
}

/** \brief counts the members of the struct
 *  \return the number of members
 */
int CBEStructType::GetMemberCount()
{
	return m_Members.size();
}

/** \brief tests if this type has the given tag
 *  \param sTag the tag to check
 *  \return true if the same
 */
bool CBEStructType::HasTag(string sTag)
{
	return (m_sTag == sTag);
}

/** \brief writes a cast of this type
 *  \param pFile the file to write to
 *  \param bPointer true if the cast should produce a pointer
 *
 * A struct cast is '(struct tag)'.
 */
void CBEStructType::WriteCast(CBEFile& pFile, bool bPointer)
{
	pFile << "(";
	if (m_sTag.empty())
	{
		// no tag -> we need a typedef to save us
		// the alias can be used for the cast
		CBETypedef *pTypedef = GetTypedef();
		assert(pTypedef);
		// get first declarator (without stars)
		vector<CBEDeclarator*>::iterator iterD;
		for (iterD = pTypedef->m_Declarators.begin();
			iterD != pTypedef->m_Declarators.end();
			iterD++)
		{
			if ((*iterD)->GetStars() <= (bPointer?1:0))
				break;
		}
		assert(iterD != pTypedef->m_Declarators.end());
		pFile << (*iterD)->GetName();
		if (bPointer && ((*iterD)->GetStars() == 0))
			pFile << "*";
	}
	else
	{
		pFile << m_sName << " " << m_sTag;
		if (bPointer)
			pFile << "*";
	}
	pFile << ")";
}

/** \brief allows to access tag member
 *  \return a copy of the tag
 */
string CBEStructType::GetTag()
{
	return m_sTag;
}

/** \brief write the declaration of this type
 *  \param pFile the file to write to
 *
 * Only write a 'struct \<tag\>'.
 */
void CBEStructType::WriteDeclaration(CBEFile& pFile)
{
	pFile << m_sName;
	if (!m_sTag.empty())
		pFile << " " << m_sTag;
}

/** \brief if struct is variable size, it has to write the size
 *  \param pFile the file to write to
 *  \param pStack contains the declarator stack for constructed typed variables
 *  \param pUsingFunc the function to use as reference for members
 *
 * This is usually the sum of the member's sizes. Because this is only called
 * when the struct is variable sized, we have to first add all the fixed sized
 * members, use this number plus the variable sized members.
 *
 * This might also get called for a [ref, prealloc] parameter, so first check
 * if we really are variable sized. If not and we have a tag:
 * sizeof(struct tag).
 */
void CBEStructType::WriteGetSize(CBEFile& pFile,
	CDeclStack* pStack,
	CBEFunction *pUsingFunc)
{
	/* check for variable sized members */
	bool bVarSized = false;
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Members.begin();
		iter != m_Members.end() && !bVarSized;
		iter++)
	{
		if ((*iter)->IsVariableSized() ||
			(*iter)->IsString())
			bVarSized = true;
	}
	if (!bVarSized && !m_sTag.empty())
	{
		pFile << "sizeof(struct " << m_sTag << ")";
		return;
	}

	int nFixedSize = GetFixedSize();
	bool bFirst = true;
	if (nFixedSize > 0)
	{
		pFile << nFixedSize;
		bFirst = false;
	}
	for (iter = m_Members.begin();
		iter != m_Members.end();
		iter++)
	{
		if (!(*iter)->IsVariableSized() &&
			!(*iter)->IsString())
			continue;
		if (!bFirst)
			pFile << "+";
		bFirst = false;
		WriteGetMemberSize(pFile, *iter, pStack, pUsingFunc);
	}
}

/** \brief calculates the size of all fixed sized members
 *  \return the sum of the fixed-sized member's sizes in bytes
 */
int CBEStructType::GetFixedSize()
{
	int nSize = 0;

	// if this is a tagged struct without members,
	// we have to find the original struct
	if (m_bForwardDeclaration)
	{
		// search for tag
		CBERoot *pRoot = GetSpecificParent<CBERoot>();
		assert(pRoot);
		CBEStructType *pTaggedType =
			(CBEStructType*)pRoot->FindTaggedType(TYPE_STRUCT, GetTag());
		// if found, marshal this instead
		if ((pTaggedType) && (pTaggedType != this))
		{
			nSize = pTaggedType->GetSize();
			return nSize;
		}
	}

	int nBitSize = 0;
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Members.begin();
		iter != m_Members.end();
		iter++)
	{
		int nMemberSize = (*iter)->GetSize();
		if ((*iter)->IsString() ||
			(*iter)->IsVariableSized())
		{
			// its of variable size
			// if bitfields before, align them and add them
			if (nBitSize > 0)
			{
				nSize += nBitSize / 8;
				if ((nBitSize % 8) > 0)
					nSize++;
				nBitSize = 0;
			}
		}
		else if (nMemberSize == 0)
		{
			nBitSize += (*iter)->GetBitfieldSize();
		}
		else
		{
			CBESizes *pSizes = CCompiler::GetSizes();
			// check for alignment:
			// if current size (nSize) is 4 bytes or above then sum is aligned
			// to dword size
			//
			// if current size (nSize) is 2 bytes then sum is aligned to word
			// size
			if (nMemberSize >= 4)
				nSize = pSizes->WordRoundUp(nSize); // dword align
			if ((nMemberSize == 2) && ((nSize % 2) > 0))
				nSize++; // word align
			nSize += nMemberSize;
			// if bitfields before, align them and add them
			if (nBitSize > 0)
			{
				nSize += nBitSize / 8;
				if ((nBitSize % 8) > 0)
					nSize++;
				nBitSize = 0;
			}
		}
	}
	// some bitfields left? -> align them and add them
	if (nBitSize > 0)
	{
		nSize += nBitSize / 8;
		if ((nBitSize % 8) > 0)
			nSize++;
	}
	return nSize;
}

/** \brief writes the whole size string for a member
 *  \param pFile the file to write to
 *  \param pMember the member to write for
 *  \param pStack contains the declarator stack
 *  \param pUsingFunc the function that should be used as reference for
 *         pMember
 *
 * This code is taken from
 * CBEMsgBufferType::WriteInitializationVarSizedParameters if something is not
 * working, check if something changed there as well.
 */
void CBEStructType::WriteGetMemberSize(CBEFile& pFile, CBETypedDeclarator *pMember, CDeclStack* pStack,
	CBEFunction *pUsingFunc)
{
	bool bFirst = true;
	vector<CBEDeclarator*>::iterator iterD;
	for (iterD = pMember->m_Declarators.begin();
		iterD != pMember->m_Declarators.end();
		iterD++)
	{
		if (!bFirst)
			pFile << "+";

		bFirst = false;
		// add the current decl to the stack
		pStack->push_back(*iterD);
		// add the member of the struct to the stack
		pMember->WriteGetSize(pFile, pStack, pUsingFunc);
		if (pMember->IsString())
		{
			// add terminating zero
			pFile << "+1";
			bool bHasSizeAttr =
				pMember->m_Attributes.Find(ATTR_SIZE_IS) ||
				pMember->m_Attributes.Find(ATTR_LENGTH_IS) ||
				pMember->m_Attributes.Find(ATTR_MAX_IS);
			CBESizes *pSizes = CCompiler::GetSizes();
			if (!bHasSizeAttr)
				pFile << "+" << pSizes->GetSizeOfType(TYPE_INTEGER);
		} else if (pMember->GetType()->GetSize() > 1)  // && !pMember->IsString()
		{
			pFile << "*sizeof";
			pMember->GetType()->WriteCast(pFile, false);
		}
		// remove the decl from the stack
		pStack->pop_back();
	}
}

/** \brief test if this is a simple type
 *  \return false
 */
bool CBEStructType::IsSimpleType()
{
	return false;
}

/** \brief tries to find a member with a declarator stack
 *  \param pStack contains the list of members to search for
 *  \param iCurr the iterator pointing to the currently searched element
 *  \return the member found or 0 if not found
 *
 * Gets the first element on the stack and tries to find
 */
CBETypedDeclarator*
CBEStructType::FindMember(CDeclStack* pStack,
	CDeclStack::iterator iCurr)
{
	// if at end, return
	if (iCurr == pStack->end())
		return 0;

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEStructType::%s(stack) called\n", __func__);

	DUMP_STACK(i, pStack, "Find");
	// try to find member for current declarator
	string sName = iCurr->pDeclarator->GetName();
	CBETypedDeclarator *pMember = m_Members.Find(sName);
	if (!pMember)
		return pMember;

	// no more elements in stack, we are finished
	if (++iCurr == pStack->end())
		return pMember;

	// check member types
	CBEType *pType = pMember->GetType();
	while (dynamic_cast<CBEUserDefinedType*>(pType))
		pType = dynamic_cast<CBEUserDefinedType*>(pType);

	CBEStructType *pStruct = dynamic_cast<CBEStructType*>(pType);
	if (pStruct)
		return pStruct->FindMember(pStack, iCurr);

	CBEUnionType *pUnion = dynamic_cast<CBEUnionType*>(pType);
	if (pUnion)
		return pUnion->FindMember(pStack, iCurr);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEStructType::%s no member in struct or union found. return 0.\n",
		__func__);
	return 0;
}

/** \brief tries to find a member with a specific attribute
 *  \param nAttributeType the attribute type to look for
 *  \return the first member with the given attribute
 */
CBETypedDeclarator* CBEStructType::FindMemberAttribute(ATTR_TYPE nAttributeType)
{
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Members.begin();
		iter != m_Members.end();
		iter++)
	{
		if ((*iter)->m_Attributes.Find(nAttributeType))
			return *iter;
	}
	return 0;
}

/** \brief tries to find a member with a specific IS attribute
 *  \param nAttributeType the attribute type to look for
 *  \param sAttributeParameter the name of the attributes parameter to look for
 *  \return the first member with the given attribute
 */
CBETypedDeclarator*
CBEStructType::FindMemberIsAttribute(ATTR_TYPE nAttributeType,
	string sAttributeParameter)
{
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Members.begin();
		iter != m_Members.end();
		iter++)
	{
		CBEAttribute *pAttr = (*iter)->m_Attributes.Find(nAttributeType);
		if (pAttr && pAttr->m_Parameters.Find(sAttributeParameter))
			return *iter;
	}
	return 0;
}


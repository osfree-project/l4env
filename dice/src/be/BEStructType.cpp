/**
 *	\file	dice/src/be/BEStructType.cpp
 *	\brief	contains the implementation of the class CBEStructType
 *
 *	\date	01/15/2002
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

#include "be/BEStructType.h"
#include "be/BEContext.h"
#include "be/BETypedef.h"
#include "be/BEDeclarator.h"
#include "be/BERoot.h"
#include "be/BEAttribute.h"

#include "fe/FETaggedStructType.h"
#include "fe/FEInterface.h"
#include "fe/FELibrary.h"
#include "fe/FEFile.h"
#include "fe/FEArrayType.h"
#include "fe/FESimpleType.h"
#include "fe/FEIsAttribute.h"
#include "fe/FEDeclarator.h"

IMPLEMENT_DYNAMIC(CBEStructType);

CBEStructType::CBEStructType():m_vMembers(RUNTIME_CLASS(CBETypedDeclarator))
{
    IMPLEMENT_DYNAMIC_BASE(CBEStructType, CBEType);
}

CBEStructType::CBEStructType(CBEStructType & src):CBEType(src),
m_vMembers(RUNTIME_CLASS
	   (CBETypedDeclarator))
{
    m_sTag = src.m_sTag;
    m_vMembers.Add(&src.m_vMembers);
    m_vMembers.SetParentOfElements(this);
    IMPLEMENT_DYNAMIC_BASE(CBEStructType, CBEType);
}

/**	\brief destructor of this instance */
CBEStructType::~CBEStructType()
{
    m_vMembers.DeleteAll();
}

/**	\brief prepares this instance for the code generation
 *	\param pFEType the corresponding front-end attribute
 *	\param pContext the context of the code generation
 *	\return true if the code generation was successful
 *
 * This implementation calls the base class' implementatio first to set default values and then adds the
 * members of the struct to this class.
 */
bool CBEStructType::CreateBackEnd(CFETypeSpec * pFEType, CBEContext * pContext)
{
    // sets m_sName to "struct"
    if (!CBEType::CreateBackEnd(pFEType, pContext))
        return false;
    // if sequence create own members
	if (pFEType->GetType() == TYPE_ARRAY)
	    return CreateBackEndSequence((CFEArrayType*)pFEType, pContext);
    // iterate over members
    CFEStructType *pFEStruct = (CFEStructType *) pFEType;
    VectorElement *pIter = pFEStruct->GetFirstMember();
    CFETypedDeclarator *pFEMember;
    while ((pFEMember = pFEStruct->GetNextMember(pIter)) != 0)
    {
        CBETypedDeclarator *pMember = pContext->GetClassFactory()->GetNewTypedDeclarator();
        AddMember(pMember);
        if (!pMember->CreateBackEnd(pFEMember, pContext))
        {
            RemoveMember(pMember);
            delete pMember;
            return false;
        }
    }
    // set tag
    if (pFEType->IsKindOf(RUNTIME_CLASS(CFETaggedStructType)))
    {
        // see if we can find the original struct
        String sTag = ((CFETaggedStructType*)pFEType)->GetTag();
		// we start with the parent interface and walk all the way up to the root
        CFEConstructedType *pFETaggedDecl = 0;
		CFEInterface *pFEInterface = pFEType->GetParentInterface();
		if (pFEInterface)
		{
		    pFETaggedDecl = pFEInterface->FindTaggedDecl(sTag);
			if (!pFETaggedDecl)
			{
			    CFELibrary *pParentLib = pFEInterface->GetParentLibrary();
				while (pParentLib && !pFETaggedDecl)
				{
				    pFETaggedDecl = pParentLib->FindTaggedDecl(sTag);
					pParentLib = pParentLib->GetParentLibrary();
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
            m_sTag = pContext->GetNameFactory()->GetTypeName(pFETaggedDecl, sTag, pContext);
        else
		{
		    // if this is a complete type, than this should
			// be made a full name as well, since it is defined in
			// an idl file
			if (GetMemberCount() > 0)
			    m_sTag = pContext->GetNameFactory()->GetTypeName(pFEType, sTag, pContext);
			else
                m_sTag = sTag;
			// still no original struct found, than this might be a user
			// defined struct
			// get the size from there
			CBESizes *pSizes = pContext->GetSizes();
			m_nSize = pSizes->GetSizeOfEnvType(sTag);
		}
    }

    return true;
}
/**	\brief prepares this instance for the code generation
 *	\param pFEType the corresponding front-end type
 *	\param pContext the context of the code generation
 *	\return true if the code generation was successful
 */
bool CBEStructType::CreateBackEndSequence(CFEArrayType * pFEType, CBEContext * pContext)
{
    // if sequence create own members
	if (pFEType->GetType() != TYPE_ARRAY)
	    return false;
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
	Vector *pMembers = new Vector(RUNTIME_CLASS(CFETypedDeclarator));

	// create the _maximum member
	CFETypeSpec* pFEMType = new CFESimpleType(TYPE_INTEGER, true,
	                                    true, 4/*value for LONG*/, false);
	CFEDeclarator *pFEDeclarator = new CFEDeclarator(DECL_IDENTIFIER, String("_maximum"));
	CFETypedDeclarator *pFEMember = new CFETypedDeclarator(TYPEDECL_FIELD,
	                                    pFEMType,
										new Vector(RUNTIME_CLASS(CFEDeclarator),
										    1, pFEDeclarator));
	pFEMType->SetParent(pFEMember);
	pFEDeclarator->SetParent(pFEMember);
	pMembers->Add(pFEMember);

	// create _length member
	pFEMType = new CFESimpleType(TYPE_INTEGER, true,
	                                    true, 4/*value for LONG*/, false);
	pFEDeclarator = new CFEDeclarator(DECL_IDENTIFIER, String("_length"));
	pFEMember = new CFETypedDeclarator(TYPEDECL_FIELD,
	                                    pFEMType,
										new Vector(RUNTIME_CLASS(CFEDeclarator),
										    1, pFEDeclarator));
	pFEMType->SetParent(pFEMember);
	pFEDeclarator->SetParent(pFEMember);
	pMembers->Add(pFEMember);

	// add attributes
	// attribute [max_is(_maximum)]
	Vector *pAttributes = new Vector(RUNTIME_CLASS(CFEAttribute));
	pFEDeclarator = new CFEDeclarator(DECL_IDENTIFIER, String("_maximum"));
	CFEAttribute *pFEAttribute = new CFEIsAttribute(ATTR_MAX_IS,
	                                    new Vector(RUNTIME_CLASS(CFEDeclarator),
										    1, pFEDeclarator));
	pFEDeclarator->SetParent(pFEAttribute);
	pAttributes->Add(pFEAttribute);
    // attribute [length_is(_length)]
	pFEDeclarator = new CFEDeclarator(DECL_IDENTIFIER, String("_length"));
	pFEAttribute = new CFEIsAttribute(ATTR_LENGTH_IS,
	                                    new Vector(RUNTIME_CLASS(CFEDeclarator),
										    1, pFEDeclarator));
	pFEDeclarator->SetParent(pFEAttribute);
	pAttributes->Add(pFEAttribute);
	// create the *_buffer member
	pFEMType = pFEType->GetBaseType();
	pFEDeclarator = new CFEDeclarator(DECL_IDENTIFIER, String("_buffer"), 1);
	pFEMember = new CFETypedDeclarator(TYPEDECL_FIELD,
	                                    pFEMType,
										new Vector(RUNTIME_CLASS(CFEDeclarator),
										    1, pFEDeclarator), pAttributes);
	pFEMType->SetParent(pFEMember);
	pFEDeclarator->SetParent(pFEMember);
	pAttributes->SetParentOfElements(pFEMember);
	pMembers->Add(pFEMember);

	// create struct
	CFEStructType *pFEStruct = new CFEStructType(pMembers);
	pMembers->SetParentOfElements(pFEStruct);
	pFEStruct->SetParent(pFEType->GetParent());

	// recusively call CreateBackEnd to initialize struct
    return CreateBackEnd(pFEStruct, pContext);
}

/**	\brief adds a new member
 *	\param pMember the new member to add
 */
void CBEStructType::AddMember(CBETypedDeclarator * pMember)
{
    if (!pMember)
        return;
    m_vMembers.Add(pMember);
    pMember->SetParent(this);
}

/**	\brief removes a member from the members vector
 *	\param pMember the member to remove
 */
void CBEStructType::RemoveMember(CBETypedDeclarator * pMember)
{
    if (!pMember)
        return;
    m_vMembers.Remove(pMember);
}

/**	\brief retrieves a pointer to the first member
 *	\return a pointer to the first member
 */
VectorElement *CBEStructType::GetFirstMember()
{
    return m_vMembers.GetFirst();
}

/**	\brief retrieves reference to next member
 *	\param pIter the pointer to the next member
 *	\return a reference to the member pIter points to
 */
CBETypedDeclarator *CBEStructType::GetNextMember(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    CBETypedDeclarator *pRet = (CBETypedDeclarator *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextMember(pIter);
    return pRet;
}

/**	\brief writes the structure into the target file
 *	\param pFile the target file
 *	\param pContext the context of the write operation
 *
 * A struct looks like this:
 * <code>
 * struct &lt;tag&gt;
 * {
 *   &lt;member list&gt;
 * }
 * </code>
 */
void CBEStructType::Write(CBEFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    // open struct
    pFile->Print("%s", (const char *) m_sName);	// should be set to "struct"
    if (!m_sTag.IsEmpty())
        pFile->Print(" %s", (const char *) m_sTag);
    if (GetMemberCount() > 0)
    {
        pFile->Print("\n");
        pFile->PrintIndent("{\n");
        pFile->IncIndent();
        // print members
        VectorElement *pIter = GetFirstMember();
        CBETypedDeclarator *pMember;
        while ((pMember = GetNextMember(pIter)) != 0)
        {
            pFile->PrintIndent("");
            pMember->WriteDeclaration(pFile, pContext);
            pFile->Print(";\n");
        }
        // close struct
        pFile->DecIndent();
        pFile->PrintIndent("}");
    }
}

/**	\brief calculates the size of the written string
 *	\return the length of the written string
 *
 * This function is used to see how long the type of a parameter (or return type) is.
 * Thus no members are necessary, but we have to consider the tag if it exists.
 */
int CBEStructType::GetStringLength()
{
    int nSize = m_sName.GetLength();
    if (!m_sTag.IsEmpty())
        nSize += m_sTag.GetLength();
    return nSize;
}

/**	\brief generates an exact copy of this class
 *	\return a reference to the new object
 */
CObject *CBEStructType::Clone()
{
    return new CBEStructType(*this);
}

/**	\brief calculate the size of a struct type
 *	\return the size in bytes of the struct
 *
 * The struct's size is the sum of the member's size.
 * We test m_nSize on zero. If it is greater than zero, the size
 * has already been set. If it isn't we calculate it.
 *
 * If the declarator's GetSize returns 0, the member is a bitfield. We have to add
 * the bits of the declarators. If a bitfield declarator is followed by a non-bitfield
 * declarator, the size has to be aligned to the next byte.
 *
 * \todo If member is indirect, we should add size of pointer instead of size of type
 */
int CBEStructType::GetSize()
{
    if (m_nSize != 0)
        return m_nSize;

    // if this is a tagged struct without members, we have to find the original struct
    if ((GetMemberCount() == 0) && (GetFEType() == TYPE_TAGGED_STRUCT))
    {
        // search for tag
        CBERoot *pRoot = GetRoot();
        assert(pRoot);
        CBEStructType *pTaggedType = (CBEStructType*)pRoot->FindTaggedType(TYPE_TAGGED_STRUCT, GetTag());
        // if found, marshal this instead
        if ((pTaggedType) && (pTaggedType != this))
        {
            m_nSize = pTaggedType->GetSize();
            return m_nSize;
        }
		// if no tagged struct found, this is a user defined type
		// we asked the sizes class in CreateBackEnd, maybe it knows my size
		// -> m_nSize should be > 0
    }

    VectorElement *pIter = GetFirstMember();
    CBETypedDeclarator *pMember;
    int nBitSize = 0;
    while ((pMember = GetNextMember(pIter)) != 0)
    {
        int nSize = pMember->GetSize();
        if (pMember->IsString())
        {
            // a string is also variable sized member
			m_nSize = -1;
            return -1;
        }
        else if (pMember->IsVariableSized())
        {
            // if one of the members is variable sized,
            // the whole struct is variable sized
			m_nSize = -1;
            return -1;
//        	// its of variable size
//            m_nSize += pMember->GetType()->GetSize();	// we add the base type size
//            // if bitfields before, align them and add them
//            if (nBitSize > 0)
//            {
//                m_nSize += nBitSize / 8;
//                if ((nBitSize % 8) > 0)
//                    m_nSize++;
//                nBitSize = 0;
//            }
        }
        else if (nSize == 0)
        {
            nBitSize += pMember->GetBitfieldSize();
        }
        else
        {
			// check for alignment:
			// if current size (nSize) is 4 bytes or above then sum is aligned to dword size
			// if current size (nSize) is 2 bytes then sum is aligned to word size
			if ((nSize >= 4) && ((m_nSize % 4) > 0))
				m_nSize += 4 - (m_nSize % 4); // dword align
			if ((nSize == 2) && ((m_nSize % 2) > 0))
				m_nSize++; // word align
            m_nSize += nSize;
            // if bitfields before, align them and add them
            if (nBitSize > 0)
            {
                m_nSize += nBitSize / 8;
                if ((nBitSize % 8) > 0)
                    m_nSize++;
                nBitSize = 0;
            }
        }
    }
    // some bitfields left? -> align them and add them
    if (nBitSize > 0)
    {
        m_nSize += nBitSize / 8;
        if ((nBitSize % 8) > 0)
            m_nSize++;
    }
    return m_nSize;
}

/**	\brief writes code to initialize a variable of this type with a zero value
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * A struct is usually initialized by writing a init value for all its members in a comma seperated list,
 * embraced by braces. E.g. { (CORBA_long)0, (CORBA_float)0 }
 */
void CBEStructType::WriteZeroInit(CBEFile * pFile, CBEContext * pContext)
{
    pFile->Print("{ ");
	pFile->IncIndent();
    VectorElement *pIter = GetFirstMember();
    CBETypedDeclarator *pMember;
    while ((pMember = GetNextMember(pIter)) != 0)
    {
        // get type
		CBEType *pType = pMember->GetType();
        // get declarator
		VectorElement *pIDecl = pMember->GetFirstDeclarator();
		CBEDeclarator *pDecl;
		while ((pDecl = pMember->GetNextDeclarator(pIDecl)) != 0)
		{
		    // be C99 compliant:
            pFile->Print("%s : ", (const char*)pDecl->GetName());
			if (pDecl->IsArray())
				WriteZeroInitArray(pFile, pType, pDecl, pDecl->GetFirstArrayBound(), pContext);
			else if (pType)
				pType->WriteZeroInit(pFile, pContext);
			if (pIDecl)
			{
			    pFile->Print(",\n");
				pFile->PrintIndent("");
			}
		}

		if (pIter)
		{
		    pFile->Print(",\n");
			pFile->PrintIndent("");
		}
    }
	pFile->DecIndent();
    pFile->Print(" }");
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
    return m_vMembers.GetSize();
}

/** \brief tests if this type has the given tag
 *  \param sTag the tag to check
 *  \return true if the same
 */
bool CBEStructType::HasTag(String sTag)
{
    return (m_sTag == sTag);
}

/** \brief writes a cast of this type
 *  \param pFile the file to write to
 *  \param bPointer true if the cast should produce a pointer
 *  \param pContext the context of the write operation
 *
 * A struct cast is '(struct tag)'.
 */
void CBEStructType::WriteCast(CBEFile * pFile, bool bPointer, CBEContext * pContext)
{
    pFile->Print("(");
	if (m_sTag.IsEmpty())
	{
		// no tag -> we need a typedef to save us
		// the alias can be used for the cast
		CBETypedef *pTypedef = GetTypedef();
		assert(pTypedef);
		// get first declarator (without stars)
		VectorElement *pIter = pTypedef->GetFirstDeclarator();
		CBEDeclarator *pDecl;
		while ((pDecl = pTypedef->GetNextDeclarator(pIter)) != 0)
		{
			if (pDecl->GetStars() <= (bPointer?1:0))
			    break;
		}
		assert(pDecl);
		pFile->Print("%s", (const char*)pDecl->GetName());
		if (bPointer && (pDecl->GetStars() == 0))
			pFile->Print("*");
	}
	else
	{
		pFile->Print("%s %s", (const char*)m_sName, (const char*)m_sTag);
    	if (bPointer)
    		pFile->Print("*");
	}
	pFile->Print(")");
}

/** \brief allows to access tag member
 *  \return a copy of the tag
 */
String CBEStructType::GetTag()
{
    return m_sTag;
}

/** \brief write the declaration of this type
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * Only write a 'struct &lt;tag&gt;'.
 */
void CBEStructType::WriteDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    pFile->Print("%s", (const char *) m_sName);	// should be set to "struct"
    if (!m_sTag.IsEmpty())
        pFile->Print(" %s", (const char *) m_sTag);
}

/** \brief if struct is variable size, it has to write the size
 *  \param pFile the file to write to
 *  \param pStack contains the declarator stack for constructed typed variables
 *  \param pContext the context of teh write operation
 *
 * This is usually the sum of the member's sizes. Because this is only called
 * when the struct is variable sized, we have to first add all the fixed sized
 * members, use this number plus the variable sized members.
 */
void CBEStructType::WriteGetSize(CBEFile * pFile, CDeclaratorStack *pStack, CBEContext * pContext)
{
    int nFixedSize = GetFixedSize();
    bool bFirst = true;
    if (nFixedSize > 0)
    {
        pFile->Print("%d", nFixedSize);
        bFirst = false;
    }
    VectorElement *pIter = GetFirstMember();
    CBETypedDeclarator *pMember;
    while ((pMember = GetNextMember(pIter)) != 0)
    {
        if (!pMember->IsVariableSized() &&
            !pMember->IsString())
            continue;
        if (!bFirst)
            pFile->Print("+");
        bFirst = false;
        WriteGetMemberSize(pFile, pMember, pStack, pContext);
    }
}

/** \brief calculates the size of all fixed sized members
 *  \return the sum of the fixed-sized member's sizes in bytes
 */
int CBEStructType::GetFixedSize()
{
    int nSize = 0;

    // if this is a tagged struct without members, we have to find the original struct
    if ((GetMemberCount() == 0) && (GetFEType() == TYPE_TAGGED_STRUCT))
    {
        // search for tag
        CBERoot *pRoot = GetRoot();
        assert(pRoot);
        CBEStructType *pTaggedType = (CBEStructType*)pRoot->FindTaggedType(TYPE_TAGGED_STRUCT, GetTag());
        // if found, marshal this instead
        if ((pTaggedType) && (pTaggedType != this))
        {
            nSize = pTaggedType->GetSize();
            return nSize;
        }
    }

    VectorElement *pIter = GetFirstMember();
    CBETypedDeclarator *pMember;
    int nBitSize = 0;
    while ((pMember = GetNextMember(pIter)) != 0)
    {
        int nMemberSize = pMember->GetSize();
        if (pMember->IsString() ||
            pMember->IsVariableSized())
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
            nBitSize += pMember->GetBitfieldSize();
        }
        else
        {
			// check for alignment:
			// if current size (nSize) is 4 bytes or above then sum is aligned to dword size
			// if current size (nSize) is 2 bytes then sum is aligned to word size
			if ((nMemberSize >= 4) && ((nSize % 4) > 0))
				nSize += 4 - (nSize % 4); // dword align
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
 *  \param pContext the context of the write operation
 *
 * This code is taken from CBEMsgBufferType::WriteInitializationVarSizedParameters
 * if something is not working, check if something changed there as well.
 */
void CBEStructType::WriteGetMemberSize(CBEFile *pFile, CBETypedDeclarator *pMember, CDeclaratorStack *pStack, CBEContext *pContext)
{
    bool bFirst = true;
    VectorElement *pIter = pMember->GetFirstDeclarator();
    CBEDeclarator *pDecl;
    while ((pDecl = pMember->GetNextDeclarator(pIter)) != 0)
    {
        if (!bFirst)
        {
            pFile->Print("+");
            bFirst = false;
        }
        // add the current decl to the stack
        pStack->Push(pDecl);
        // add the member of the struct to the stack
        pMember->WriteGetSize(pFile, pStack, pContext);
        if ((pMember->GetType()->GetSize() > 1) && !(pMember->IsString()))
        {
            pFile->Print("*sizeof");
            pMember->GetType()->WriteCast(pFile, false, pContext);
        }
        else if (pMember->IsString())
        {
            // add terminating zero
            pFile->Print("+1");
            bool bHasSizeAttr = pMember->HasSizeAttr(ATTR_SIZE_IS) ||
                    pMember->HasSizeAttr(ATTR_LENGTH_IS) ||
                    pMember->HasSizeAttr(ATTR_MAX_IS);
            if (!bHasSizeAttr)
                pFile->Print("+%d", pContext->GetSizes()->GetSizeOfType(TYPE_INTEGER));
        }
        // remove the decl from the stack
        pStack->Pop();
    }
}

/** \brief test if this is a simple type
 *  \return false
 */
bool CBEStructType::IsSimpleType()
{
    return false;
}

/** \brief tries to find a member with the name sName
 *  \param sName the name of the member to search
 *  \return the member found or 0 if not found
 */
CBETypedDeclarator* CBEStructType::FindMember(String sName)
{
    VectorElement *pIter = GetFirstMember();
	CBETypedDeclarator *pMember;
	while ((pMember = GetNextMember(pIter)) != 0)
	{
	    if (pMember->FindDeclarator(sName))
		    return pMember;
	}
	return 0;
}

/** \brief tries to find a member with a specific attribute
 *  \param nAttributeType the attribute type to look for
 *  \return the first member with the given attribute
 */
CBETypedDeclarator* CBEStructType::FindMemberAttribute(int nAttributeType)
{
    VectorElement *pIter = GetFirstMember();
	CBETypedDeclarator *pMember;
	while ((pMember = GetNextMember(pIter)) != 0)
	{
	    if (pMember->FindAttribute(nAttributeType))
		    return pMember;
	}
	return 0;
}

/** \brief tries to find a member with a specific IS attribute
 *  \param nAttributeType the attribute type to look for
 *  \param sAttributeParameter the name of the attributes parameter to look for
 *  \return the first member with the given attribute
 */
CBETypedDeclarator* CBEStructType::FindMemberIsAttribute(int nAttributeType, String sAttributeParameter)
{
    VectorElement *pIter = GetFirstMember();
	CBETypedDeclarator *pMember;
	while ((pMember = GetNextMember(pIter)) != 0)
	{
	    CBEAttribute *pAttr = pMember->FindAttribute(nAttributeType);
		if (pAttr && pAttr->FindIsParameter(sAttributeParameter))
		    return pMember;
	}
	return 0;
}

/**
 *	\file	dice/src/be/BEType.cpp
 *	\brief	contains the implementation of the class CBEType
 *
 *	\date	01/15/2002
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

#include "be/BEType.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BETypedef.h"

#include "fe/FETypeSpec.h"
#include "fe/FESimpleType.h"
#include "fe/FEUserDefinedType.h"

IMPLEMENT_DYNAMIC(CBEType);

CBEType::CBEType()
{
    m_bUnsigned = false;
    m_nSize = 0;
    m_nFEType = TYPE_NONE;
    IMPLEMENT_DYNAMIC_BASE(CBEType, CBEObject);
}

CBEType::CBEType(CBEType & src):CBEObject(src)
{
    m_bUnsigned = src.m_bUnsigned;
    m_nSize = src.m_nSize;
    m_sName = src.m_sName;
    m_nFEType = src.m_nFEType;
    IMPLEMENT_DYNAMIC_BASE(CBEType, CBEObject);
}

/**	\brief destructor of this instance */
CBEType::~CBEType()
{

}

/**	\brief creates the back-end structure for a type class
 *	\param pFEType the respective front-end type class
 *	\param pContext the context of the code generation
 *	\return true if code generation was successful
 *
 * This implementation sets basic values, common for all types. Since this class is also used for simple
 * types, these values have to be set here.
 */
bool CBEType::CreateBackEnd(CFETypeSpec * pFEType, CBEContext * pContext)
{
    VERBOSE("CEBType::CreateBackEnd(front-end type)\n");
    if (!pFEType)
    {
        VERBOSE("CBEType::CreateBE failed because FE type is 0\n");
        return false;
    }

    if (pFEType->IsKindOf(RUNTIME_CLASS(CFESimpleType)))
    {
        m_bUnsigned = ((CFESimpleType *) pFEType)->IsUnsigned();
        m_nSize = pContext->GetSizes()->GetSizeOfType(pFEType->GetType(), ((CFESimpleType*)pFEType)->GetSize());
    }

    m_sName = pContext->GetNameFactory()->GetTypeName(pFEType->GetType(), m_bUnsigned, pContext, m_nSize);
    if (m_sName.IsEmpty())
    {
        // user defined type overloads this function -> m_sName should always be set
        VERBOSE("CBEType::CreateBE failed because no type name could be assigned\n");
        return false;
    }

    m_nFEType = pFEType->GetType();
    return true;
}

/**	\brief create back-end structure for type class without front-end type
 *	\param bUnsigned true if unsigned type
 *	\param nSize the size of the type in bytes
 *	\param nFEType the type's identifier
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This implementation is used to generate a back-end type without having a front-end type. This can be used to
 * create additional types.
 */
bool CBEType::CreateBackEnd(bool bUnsigned, int nSize, int nFEType, CBEContext * pContext)
{
    m_bUnsigned = bUnsigned;
    m_nSize = nSize;
    m_sName = pContext->GetNameFactory()->GetTypeName(nFEType, bUnsigned, pContext, m_nSize);
    m_nFEType = nFEType;
    return true;
}

/**	\brief write the type to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * If this function is called, it is expected to write this current type. Constructed types
 * have their own write function. We use the member name which has been set to a globally correct
 * type name by the CreateBE function.
 *
 */
void CBEType::Write(CBEFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    pFile->Print("%s", (const char *) m_sName);
}

/**	\brief calculates the size of the written string
 *	\return the length of the written string
 */
int CBEType::GetStringLength()
{
    int nSize = 0;
    //if (m_bUnsigned)
    //      nSize += 9;
    nSize += m_sName.GetLength();
    return nSize;
}

/**	\brief checks if this is a void type
 *	\return true if void
 *
 * A "void type" is a type which does not use any memory (0 bytes) (e.g. void)
 */
bool CBEType::IsVoid()
{
    return ((m_nFEType == TYPE_VOID) || (m_nFEType == TYPE_NONE));
}

/**	\brief returns the size of this type
 *	\return the member m_nSize
 */
int CBEType::GetSize()
{
    return m_nSize;
}

/**	\brief checks the type of the type
 *	\param nFEType the type to compare to
 *	\return true if the same
 */
bool CBEType::IsOfType(int nFEType)
{
    return (m_nFEType == nFEType);
}

/**	\brief generates an exact copy of this class
 *	\return a reference to the new object
 */
CObject *CBEType::Clone()
{
    return new CBEType(*this);
}

/**	\brief write code to initialize a variable of this type with a zero value
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This operation simply casts a zero (0) to the appropriate type.
 */
void CBEType::WriteZeroInit(CBEFile * pFile, CBEContext * pContext)
{
    WriteCast(pFile, false, pContext);
    pFile->Print("0");
}

/**	\brief allows access to the m_nFEType member
 *	\return the value of m_nFEType
 */
int CBEType::GetFEType()
{
    return m_nFEType;
}

/**	\brief allows access to the m_bUnsigned member
 *	\return the value of m_bUnsigned
 */
bool CBEType::IsUnsigned()
{
    return m_bUnsigned;
}

/** \brief tests if this type is a constructed type
 *  \return true if it is
 */
bool CBEType::IsConstructedType()
{
    return false;
}

/** \brief tests if a tagged type has the given tag
 *  \param sTag the tag to test
 *  \return true if the same
 */
bool CBEType::HasTag(String sTag)
{
    return false;
}

/** \brief writes a cast string for this type
 *  \param pFile the file to write to
 *  \param bPointer true if the cast should produce a pointer
 *  \param pContext the context of the write operation
 *
 * The cast is usually the name of the type.
 * E.g., int. Since we usually require correct bracing for the cast, we
 * will print (int).
 */
void CBEType::WriteCast(CBEFile *pFile, bool bPointer, CBEContext *pContext)
{
    if (IsPointerType() && !bPointer)
    {
        // this is a pointer-type, but to cast pointer-less,
        // we need the pointer's base type
        int nBaseType = TYPE_NONE;
        int nBaseSize = 0;
        switch (m_nFEType)
        {
        case TYPE_CHAR_ASTERISK:
            nBaseType = TYPE_CHAR;
            nBaseSize = 1;
            break;
        default:
            break;
        }    
        String sName = pContext->GetNameFactory()->GetTypeName(nBaseType, false, pContext, nBaseSize);
        pFile->Print("(%s", (const char*)sName);
    }
    else
        pFile->Print("(%s", (const char*)m_sName);
    // if type is pointer itself, we need no extra asterisk
    if (bPointer && !IsPointerType())
        pFile->Print("*");
	pFile->Print(")");
}

/** \brief searches for a parent, which is a typedef
 *  \return a reference to the parent typedef or 0 if none found
 */
CBETypedef* CBEType::GetTypedef()
{
    CObject *pParent = GetParent();
    while (pParent)
    {
         if (pParent->IsKindOf(RUNTIME_CLASS(CBETypedef)))
			 return (CBETypedef*)pParent;
         pParent = pParent->GetParent();
    }
    return 0;
}

/** \brief checks if this is a type, which is a pointer
 *  \return true if this is a pointer type
 *
 * A pointer type is for instance TYPE_CHAR_ASETRISK (char*)
 */
bool CBEType::IsPointerType()
{
    return (m_nFEType == TYPE_CHAR_ASTERISK);
}

/** \brief writes the type for declarations
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This function is used to write a short declaration of the type, which
 * means especially for constructed types to write only a type name, not the
 * whole definition.
 *
 * For simple types this is the same as Write, so we call this one here.
 */
void CBEType::WriteDeclaration(CBEFile *pFile, CBEContext *pContext)
{
    Write(pFile, pContext);
}

/** \brief returns true if a zero init is written
 *  \return true by default
 *
 * This is used to determine if we have to write introducing or extroducing code
 */
bool CBEType::DoWriteZeroInit()
{
    return true;
}

/** \brief if this is a variable sized type, this writes it's size
 *  \param pFile the file to write to
 *  \param pStack the declarator stack that contains the variable sized declarators
 *  \param pContext the context of the write operation
 *
 * This function commonly issues an assert. Only types, which really can
 * be variable sized overload this function to print the correct size.
 */
void CBEType::WriteGetSize(CBEFile *pFile, CDeclaratorStack *pStack, CBEContext *pContext)
{
    ASSERT(false);
}

/** \brief test if this is a simple type
 *  \return true if it is
 *
 * All basic types are simple
 */
bool CBEType::IsSimpleType()
{
    return true;
}

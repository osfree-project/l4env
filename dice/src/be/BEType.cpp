/**
 *    \file    dice/src/be/BEType.cpp
 *    \brief   contains the implementation of the class CBEType
 *
 *    \date    01/15/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/BEType.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BETypedef.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BEExpression.h"
#include "be/BEHeaderFile.h"

#include "TypeSpec-Type.h"
#include "fe/FESimpleType.h"
#include "fe/FEUserDefinedType.h"

CBEType::CBEType()
{
    m_bUnsigned = false;
    m_nSize = 0;
    m_nFEType = TYPE_NONE;
}

CBEType::CBEType(CBEType & src)
: CBEObject(src)
{
    m_bUnsigned = src.m_bUnsigned;
    m_nSize = src.m_nSize;
    m_nFEType = src.m_nFEType;
    m_sName = src.m_sName;
}

/**    \brief destructor of this instance */
CBEType::~CBEType()
{

}

/**    \brief creates the back-end structure for a type class
 *    \param pFEType the respective front-end type class
 *    \param pContext the context of the code generation
 *    \return true if code generation was successful
 *
 * This implementation sets basic values, common for all types. Since this class is also used for simple
 * types, these values have to be set here.
 */
bool CBEType::CreateBackEnd(CFETypeSpec * pFEType, CBEContext * pContext)
{
    VERBOSE("CEBType::CreateBackEnd(front-end type)\n");
    assert(pFEType);

    // call CBEObject's CreateBackEnd method
    if (!CBEObject::CreateBackEnd(pFEType))
        return false;

    // set target file name
    SetTargetFileName(pFEType, pContext);

    if (dynamic_cast<CFESimpleType*>(pFEType))
    {
        m_bUnsigned = ((CFESimpleType *) pFEType)->IsUnsigned();
        m_nSize = pContext->GetSizes()->GetSizeOfType(pFEType->GetType(), ((CFESimpleType*)pFEType)->GetSize());
    }

    m_sName = pContext->GetNameFactory()->GetTypeName(pFEType->GetType(), m_bUnsigned, pContext, m_nSize);
    if (m_sName.empty())
    {
        // user defined type overloads this function -> m_sName.c_str() should always be set
        VERBOSE("CBEType::CreateBE failed because no type name could be assigned for (%d)\n", pFEType->GetType());
        return false;
    }

    m_nFEType = pFEType->GetType();
    return true;
}

/**    \brief create back-end structure for type class without front-end type
 *    \param bUnsigned true if unsigned type
 *    \param nSize the size of the type in bytes
 *    \param nFEType the type's identifier
 *    \param pContext the context of the code generation
 *    \return true if successful
 *
 * This implementation is used to generate a back-end type without having a front-end type. This can be used to
 * create additional types.
 */
bool CBEType::CreateBackEnd(bool bUnsigned, int nSize, int nFEType, CBEContext * pContext)
{
    m_bUnsigned = bUnsigned;
    m_nSize = pContext->GetSizes()->GetSizeOfType(nFEType, nSize);
    m_sName = pContext->GetNameFactory()->GetTypeName(nFEType, bUnsigned, pContext, m_nSize);
    m_nFEType = nFEType;
    return true;
}

/**    \brief write the type to the target file
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
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
    *pFile << m_sName;
}

/**    \brief calculates the size of the written string
 *    \return the length of the written string
 */
int CBEType::GetStringLength()
{
    int nSize = 0;
    //if (m_bUnsigned)
    //      nSize += 9;
    nSize += m_sName.length();
    return nSize;
}

/**    \brief checks if this is a void type
 *    \return true if void
 *
 * A "void type" is a type which does not use any memory (0 bytes) (e.g. void)
 */
bool CBEType::IsVoid()
{
    return ((m_nFEType == TYPE_VOID) || (m_nFEType == TYPE_NONE));
}

/**    \brief returns the size of this type
 *    \return the member m_nSize
 */
int CBEType::GetSize()
{
    return m_nSize;
}

/**    \brief checks the type of the type
 *    \param nFEType the type to compare to
 *    \return true if the same
 */
bool CBEType::IsOfType(int nFEType)
{
    return (m_nFEType == nFEType);
}

/**    \brief generates an exact copy of this class
 *    \return a reference to the new object
 */
CObject *CBEType::Clone()
{
    return new CBEType(*this);
}

/**    \brief write code to initialize a variable of this type with a zero value
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This operation simply casts a zero (0) to the appropriate type.
 * We only need this cast if the type is not an integer type
 * (TYPE_INTEGER, TYPE_BYTE, TYPE_CHAR). Type TYPE_FLOAT and TYPE_DOUBLE
 * are initialized with 0.0 instead of 0.
 */
void CBEType::WriteZeroInit(CBEFile * pFile, CBEContext * pContext)
{
    switch (m_nFEType)
    {
    case TYPE_INTEGER:
    case TYPE_LONG:
    case TYPE_BYTE:
        pFile->Print("0");
        break;
    case TYPE_FLOAT:
    case TYPE_DOUBLE:
        pFile->Print("0.0");
        break;
    default:
        WriteCast(pFile, false, pContext);
        pFile->Print("0");
        break;
    }
}

/**    \brief allows access to the m_nFEType member
 *    \return the value of m_nFEType
 */
int CBEType::GetFEType()
{
    return m_nFEType;
}

/**    \brief allows access to the m_bUnsigned member
 *    \return the value of m_bUnsigned
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
bool CBEType::HasTag(string sTag)
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
        string sName = pContext->GetNameFactory()->GetTypeName(nBaseType, false, pContext, nBaseSize);
        pFile->Print("(%s", sName.c_str());
    }
    else
        pFile->Print("(%s", m_sName.c_str());
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
         if (dynamic_cast<CBETypedef*>(pParent))
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
    return (m_nFEType == TYPE_CHAR_ASTERISK) ||
           (m_nFEType == TYPE_VOID_ASTERISK);
}

/** \brief checks if this is a type, which has array dimensions
 *  \return false, since only user defined type may have array dimensions
 */
bool CBEType::IsArrayType()
{
    return false;
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
void CBEType::WriteGetSize(CBEFile *pFile,
    vector<CDeclaratorStackLocation*> *pStack,
    CBEContext *pContext)
{
    assert(false);
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

/** \brief write zero inits for array types
 *  \param pFile the file to write to
 *  \param pType the base type of the array type
 *  \param pAlias the decl with the array dimensions
 *  \param iterB the iterator pointing to the next level if there are multiple array dimensions
 *  \param pContext the context of this write operation
 */
void CBEType::WriteZeroInitArray(CBEFile *pFile, CBEType *pType, CBEDeclarator *pAlias, vector<CBEExpression*>::iterator iterB, CBEContext *pContext)
{
    CBEExpression *pBound = pAlias->GetNextArrayBound(iterB);
    if (pBound == 0)
        return;
    int nBound = pBound->GetIntValue();
    if (nBound == 0)
        return;
    pFile->Print("{ ");
    for (int i=0; i<nBound; i++)
    {
        // if there is another level, we have to step down into it,
        // if there is no other level, we have to init the elements
        vector<CBEExpression*>::iterator iTemp = iterB;
        if (pAlias->GetNextArrayBound(iTemp))
        {
            pFile->Print("\n");
            pFile->IncIndent(2);
            pFile->PrintIndent("");
            WriteZeroInitArray(pFile, pType, pAlias, iterB, pContext);
            pFile->DecIndent(2);
        }
        else
            pType->WriteZeroInit(pFile, pContext);
        if (i < nBound-1)
        {
            pFile->Print(", ");
            vector<CBEExpression*>::iterator iTemp = iterB;
            if (pAlias->GetNextArrayBound(iTemp))
                pFile->Print("\n");
        }
    }
    pFile->Print(" }");
}

/** \brief tries to get the maximum array dimension count of this type
 *  \return the number of array dimensions (0 for this implementation)
 */
int CBEType::GetArrayDimensionCount()
{
    return 0;
}

/** \brief add tagged types to the header file
 *  \param pHeader the header file to add this type to
 *  \param pContext the context of this adding
 *  \return if the adding succeeded
 */
bool CBEType::AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext)
{
    VERBOSE("CBEType::AddToFile(header: %s) for type %d called\n",
        pHeader->GetFileName().c_str(), GetFEType());
    if (IsTargetFile(pHeader))
        pHeader->AddTaggedType(this);
    return true;
}

/** \brief writes the type for an indirect declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return the levels of indirect removed for type
 */
void CBEType::WriteIndirect(CBEFile* pFile, CBEContext* pContext)
{
    Write(pFile, pContext);
}

/** \brief returns the number of indirections, in case this is a pointer type
 *  \return the indirection level
 */
int CBEType::GetIndirectionCount()
{
    return 0;
}

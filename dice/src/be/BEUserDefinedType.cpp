/**
 *  \file    dice/src/be/BEUserDefinedType.cpp
 *  \brief    contains the implementation of the class CBEUserDefinedType
 *
 *  \date    02/13/2002
 *  \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/BEUserDefinedType.h"
#include "be/BEContext.h"
#include "BEFile.h"
#include "be/BERoot.h"
#include "be/BETypedef.h"
#include "be/BEDeclarator.h"
#include "be/BEExpression.h"
#include "be/BESizes.h"
#include "Compiler.h"
#include "Error.h"
#include "TypeSpec-Type.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FEFile.h"
#include <cassert>

CBEUserDefinedType::CBEUserDefinedType()
{ }

CBEUserDefinedType::CBEUserDefinedType(CBEUserDefinedType & src)
 : CBEType(src)
{
    m_sOriginalName = src.m_sOriginalName;
}

/** \brief destructor of this instance */
CBEUserDefinedType::~CBEUserDefinedType()
{ }

/** \brief generates an exact copy of this class
 *  \return a reference to the new object
 */
CObject* CBEUserDefinedType::Clone()
{
    return new CBEUserDefinedType(*this);
}

/** \brief creates a user defined type
 *  \param sName the name of the type
 */
void
CBEUserDefinedType::CreateBackEnd(string sName)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s(%s) called\n", __func__, sName.c_str());

    string exc = string (__func__);
    if (sName.empty())
    {
	exc += " failed because user defined name is empty";
	throw new error::create_error(exc);
    }
    m_sName = sName;
    m_sOriginalName = sName;
    // set size
    m_nSize = GetSizeOfTypedef(sName);
    // determine max size later
    m_nMaxSize = 0;
    // set type
    m_nFEType = TYPE_USER_DEFINED;

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s(%s) set names, size %d\n", __func__,
	sName.c_str(), m_nSize);

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s(%s) (this=%p) returns\n", __func__,
	sName.c_str(), this);
}

/** \brief creates a user defined type
 *  \param pFEType the front-end type
 *
 * This overloads the CBEType::CreateBE function to implement user-defined
 * type specific behaviour.
 */
void
CBEUserDefinedType::CreateBackEnd(CFETypeSpec * pFEType)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s(fe) called\n", __func__);

    // call CBEObject's CreateBackEnd method
    CBEObject::CreateBackEnd(pFEType);

    string exc = string (__func__);
    if (!pFEType)
    {
	exc += " failed, because FE Type is 0";
        throw new error::create_error(exc);
    }

    CFEUserDefinedType *pUserType = dynamic_cast<CFEUserDefinedType*>(pFEType);
    if (!pUserType)
    {
	exc += " failed because FE Type is not 'user defined'";
	throw new error::create_error(exc);
    }

    CBENameFactory *pNF = CCompiler::GetNameFactory();
    if (!pUserType->GetName().empty())
    {
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	    "CBEUserDefinedType::%s get orig type for %s\n", __func__,
	    pUserType->GetName().c_str());
        // find original type
        CFEFile *pFERoot = pFEType->GetRoot();
        assert(pFERoot);
        string sName, sUserName = pUserType->GetName();
        CFETypedDeclarator *pFETypedef =
	    pFERoot->FindUserDefinedType(sUserName);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	    "CBEUserDefinedType::%s typedef found, use as ref\n",
	    __func__);
        if (pFETypedef)
            sName = pNF->GetTypeName((CFEBase*)pFETypedef, sUserName);
        else
        {
            CFEInterface *pFEInterface = pFERoot->FindInterface(sUserName);
            if (pFEInterface)
                sName = pNF->GetTypeName((CFEBase*)pFEInterface, sUserName);
        }
        if (sName.empty())
            sName = sUserName;
        CreateBackEnd(sName);
        // reset original name
        m_sOriginalName = sUserName;
    }
    m_nFEType = TYPE_USER_DEFINED;

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s(fe) (this=%p) returns\n", __func__, this);
}

/** \brief finds the size of a user defined type
 *  \param sTypeName the name of the type, we are looking for
 *  \return the size of the type in bytes
 *
 * This implementation finds the root of the tree and then searches downward
 * for the type.
 */
int CBEUserDefinedType::GetSizeOfTypedef(string sTypeName)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s(%s) called\n", __func__, sTypeName.c_str());

    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    CBETypedef *pTypedef = pRoot->FindTypedef(sTypeName);
    if (!pTypedef)
    {
	// try to find size at CBESizes
	CBESizes *pSizes = CCompiler::GetSizes();
	int nSize = pSizes->GetSizeOfType(sTypeName);
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEUserDefinedType::%s returns %d\n", __func__, nSize);
        return nSize;
    }
    /* check for recursion: if type of typedef is ourselves, then try to get
     * size of real type instead
     */
    if (pTypedef->GetType() == this)
    {
	CBEType *pReal = GetRealType();
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEUserDefinedType::%s(%s) calling GetSize of real type.\n",
	    __func__, sTypeName.c_str());
	return pReal ? pReal->GetSize() : 0;
    }
    /* since the typedef is a CBETypedDeclarator, it would evaluate the size
       of it's base type and sum it for all it's declarators. We only want it
       for the declarator we are using. That's why we use a specific
       GetSize function instead of the generic one. */
    int nSize = pTypedef->GetSize(sTypeName);

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s(%s) returns %d\n", __func__, sTypeName.c_str(),
	nSize);
    return nSize;
}

/** \brief finds the maximum size of a user defined type
 *  \param sTypeName the name of the type, we are looking for
 *  \return the size of the type in bytes
 *
 * This implementation finds the root of the tree and then searches downward
 * for the type.
 */
int CBEUserDefinedType::GetMaxSizeOfTypedef(string sTypeName)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s(%s) called\n", __func__, sTypeName.c_str());

    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    CBETypedef *pTypedef = pRoot->FindTypedef(sTypeName);
    if (!pTypedef)
    {
	// try to find size at CBESizes
	CBESizes *pSizes = CCompiler::GetSizes();
	int nSize = pSizes->GetSizeOfType(sTypeName);
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEUserDefinedType::%s returns %d\n", __func__, nSize);
        return nSize;
    }
    /* check for recursion: if type of typedef is ourselves, then try to get
     * size of real type instead
     */
    if (pTypedef->GetType() == this)
    {
	CBEType *pReal = GetRealType();
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	    "CBEUserDefinedType::%s(%s) calling GetSize of real type.\n",
	    __func__, sTypeName.c_str());
	return pReal ? pReal->GetMaxSize() : 0;
    }
    /* since the typedef is a CBETypedDeclarator, it would evaluate the size
       of it's base type and sum it for all it's declarators. We only want it
       for the declarator we are using. That's why we use a specific
       GetSize function instead of the generic one. */
    int nSize;
    if (!pTypedef->GetMaxSize(nSize, sTypeName))
	nSize = 0;

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s(%s) returns %d\n", __func__, sTypeName.c_str(),
	nSize);
    return nSize;
}

/** \brief get the user defined name
 *  \return a reference to the name
 */
string CBEUserDefinedType::GetName()
{
    return m_sName;
}

/** \brief calculate the size of thise type
 *  \return the size in bytes.
 *
 * The size of the user defined type can be calculated by searching for the
 * typedef with this name and calling its type GetSize() function.
 */
int CBEUserDefinedType::GetSize()
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s called for %s (m_nSize is %d)\n",
	__func__, m_sName.c_str(), m_nSize);

    if (m_nSize == 0)
        m_nSize = GetSizeOfTypedef(m_sName);
    /* if it is still zero, use original name */
    if (m_nSize == 0)
        m_nSize = GetSizeOfTypedef(m_sOriginalName);

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s returns %d\n", __func__, m_nSize);
    return m_nSize;
}

/** \brief try to get the maximum size of the type
 *  \return maximum size in bytes
 *
 * Do not store max size in "normal" size member.
 */
int CBEUserDefinedType::GetMaxSize()
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s called for %s (m_nMaxSize %d)\n",
	__func__, m_sName.c_str(), m_nMaxSize);

    if (m_nMaxSize == 0)
	m_nMaxSize = GetMaxSizeOfTypedef(m_sName);
    if (m_nMaxSize == 0)
	m_nMaxSize = GetMaxSizeOfTypedef(m_sOriginalName);

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s returns %d\n", __func__, m_nMaxSize);
    return m_nMaxSize;
}

/** \brief writes cod eto initialize a variable of this type with a zero value
 *  \param pFile the file to write to
 *
 * To initialize a user defined type with zero values, means to find the
 * typedef and use its type to write this initialization.
 */
void CBEUserDefinedType::WriteZeroInit(CBEFile& pFile)
{
    CBEType *pType = GetRealType();
    if (pType)
    {
        // test for array types, something like: typedef int five_ints[5];
        CBEDeclarator *pAlias = GetRealName();
        if (pAlias && pAlias->IsArray())
        {
            WriteZeroInitArray(pFile, pType, pAlias,
		pAlias->m_Bounds.begin());
            return;
        }
        // no array
        pType->WriteZeroInit(pFile);
        return;
    }
    CBEType::WriteZeroInit(pFile);
}

/** \brief checks if this is a constructed type
 *  \return true if it is
 *
 * This call is redirected to the "real" type
 */
bool CBEUserDefinedType::IsConstructedType()
{
    CBEType *pType = GetRealType();
    if (pType)
        return pType->IsConstructedType();
    return false;
}

/** \brief determines if this function writes zero init code
 *  \return the delegate result
 */
bool CBEUserDefinedType::DoWriteZeroInit()
{
    CBEType *pType = GetRealType();
    if (pType)
        return pType->DoWriteZeroInit();;
    return CBEType::DoWriteZeroInit();
}

/** \brief calls the WriteGetSize function of the original type
 *  \param pFile the file to write to
 *  \param pStack contains the declarator stack of constructed typed var-sized
 *         parameters
 *  \param pUsingFunc the function to use as reference for members
 */
void CBEUserDefinedType::WriteGetSize(CBEFile& pFile,
    CDeclStack* pStack,
    CBEFunction *pUsingFunc)
{
    CBEType *pType = GetRealType();
    CBEDeclarator *pAlias = GetRealName();
    // if the type is simple, but the alias is variable sized
    // then we need to the get max-size of the type
    // e.g. typedef small *small_var_array;
    if (pType && pType->IsSimpleType() &&
        pAlias && (pAlias->GetStars() > 0))
    {
        int nMaxSize = CCompiler::GetSizes()->GetMaxSizeOfType(
	    pType->GetFEType());
	pFile << nMaxSize;
    }
    else
    {
        if (pType)
            pType->WriteGetSize(pFile, pStack, pUsingFunc);
    }
}

/** \brief test if this is a simple type
 *  \return false
 */
bool CBEUserDefinedType::IsSimpleType()
{
    CBEType *pType = GetRealType();
    if (pType)
        return pType->IsSimpleType();
    return false;
}

/** \brief checks if this type has array dimensions
 *  \return true if it has
 */
bool CBEUserDefinedType::IsArrayType()
{
    CBEDeclarator *pAlias = GetRealName();
    // if the type is simple, but the alias is variable sized
    // then we have at least one array dimensions
    if (pAlias && (pAlias->GetStars() > 0))
        return true;
    if (pAlias && (pAlias->GetArrayDimensionCount() > 0))
        return true;
    // check base type
    CBEType *pType = GetRealType();
    if (pType)
        return pType->IsArrayType();
    return false;
}

/** \brief calculates the number of array dimensions for this type
 *  \return the number of found array dimensions
 */
int CBEUserDefinedType::GetArrayDimensionCount()
{
    CBEType *pType = GetRealType();
    CBEDeclarator *pAlias = GetRealName();
    // if the type is simple, but the alias is variable sized
    // then we have at least one array dimensions
    if (pType && pAlias)
        return pType->GetArrayDimensionCount() +
	    pAlias->GetArrayDimensionCount();
    return 0;
}

/** \brief test if this type is a pointer type
 *  \return true if it is
 *
 * The CORBA_Object type is a pointer type
 */
bool CBEUserDefinedType::IsPointerType()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEUserDefinedType::%s called\n", __func__);

    CBEDeclarator *pAlias = GetRealName();
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEUserDefinedType::%s check if alias %s has stars (%d)\n",
	__func__, pAlias ? pAlias->GetName().c_str() : "(no alias)",
	pAlias ? pAlias->GetStars() : 0);
    // if the type is simple, but the alias is variable sized
    // then we have at least one array dimensions
    if (pAlias && (pAlias->GetStars() > 0))
        return true;
    // check the aliased type
    CBEType *pType = GetRealType();
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEUserDefinedType::%s check if real type is pointer (%s)\n",
	__func__, pType ? (pType->IsPointerType() ? "yes" : "no") : "(no type)");
    if (pType)
        return pType->IsPointerType();
    // no alias; fallback to base type
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
	"CBEUserDefinedType::%s call base class\n", __func__);
    return CBEType::IsPointerType();
}

/** \brief if this is the alias of another type, get this type
 *  \return the type of the typedef
 */
CBEType* CBEUserDefinedType::GetRealType()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEUserDefinedType::%s called\n", __func__);

    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    CBETypedef *pTypedef = pRoot->FindTypedef(m_sName);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s found typedef for %s at %p\n",
	__func__, m_sName.c_str(), pTypedef);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
	"CBEUserDefinedType::%s type of typedef is %p, this %p\n",
	__func__, pTypedef ? pTypedef->GetType() : 0, this);
    if (pTypedef &&
	pTypedef->GetType() != this)
        return pTypedef->GetType();
    return 0;
}

/** \brief if this is the alias of another type, get the alias of that type
 *  \return the alias of the typedef
 */
CBEDeclarator* CBEUserDefinedType::GetRealName()
{
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    CBETypedef *pTypedef = pRoot->FindTypedef(m_sName);
    if (pTypedef)
        return pTypedef->m_Declarators.First();
    return 0;
}

/** \brief writes the type without indirections
 *  \param pFile the file to write to
 */
void CBEUserDefinedType::WriteIndirect(CBEFile& pFile)
{
    if (IsPointerType())
    {
        CBEType *pType = GetRealType();
        if (pType)
        {
            pType->WriteIndirect(pFile);
            return;
        }
    }
    CBEType::WriteIndirect(pFile);
}

/** \brief returns the number of indirections
 *  \return the number of stars in the typedef alias
 */
int CBEUserDefinedType::GetIndirectionCount()
{
    if (IsPointerType())
    {
        int nIndirect = 0;
        CBEType *pType = GetRealType();
        if (pType)
        {
            CBEDeclarator *pAlias = GetRealName();
            if (pAlias)
                nIndirect = pAlias->GetStars();
            return nIndirect + pType->GetIndirectionCount();
        }
    }
    return CBEType::GetIndirectionCount();
}

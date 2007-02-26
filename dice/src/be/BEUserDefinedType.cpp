/**
 *	\file	dice/src/be/BEUserDefinedType.cpp
 *	\brief	contains the implementation of the class CBEUserDefinedType
 *
 *	\date	02/13/2002
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

#include "be/BEUserDefinedType.h"
#include "be/BEContext.h"
#include "be/BERoot.h"
#include "be/BETypedef.h"
#include "be/BEDeclarator.h"
#include "be/BEExpression.h"

#include "TypeSpec-Type.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FEFile.h"

IMPLEMENT_DYNAMIC(CBEUserDefinedType);

CBEUserDefinedType::CBEUserDefinedType()
{
    IMPLEMENT_DYNAMIC_BASE(CBEUserDefinedType, CBEType);
}

CBEUserDefinedType::CBEUserDefinedType(CBEUserDefinedType & src):CBEType(src)
{
    IMPLEMENT_DYNAMIC_BASE(CBEUserDefinedType, CBEType);
}

/**	\brief destructor of this instance */
CBEUserDefinedType::~CBEUserDefinedType()
{

}

/**	\brief creates a user defined type
 *	\param sName the name of the type
 *	\param pContext the context of the creation
 *	\return true if successful
 */
bool CBEUserDefinedType::CreateBackEnd(String sName, CBEContext * pContext)
{
    if (sName.IsEmpty())
	{
        VERBOSE("%s failed because user defined name is empty\n", __PRETTY_FUNCTION__);
        return false;
	}
    m_sName = sName;
    // set size
    m_nSize = GetSizeOfType(sName);
    // check if size could be found; if not test environment types
    if (m_nSize == 0)
    {
        CBESizes *pSizes = pContext->GetSizes();
        m_nSize = pSizes->GetSizeOfEnvType(sName);
    }
    // set type
    m_nFEType = TYPE_USER_DEFINED;
    return true;
}

/**	\brief creates a user defined type
 *	\param pFEType the front-end type
 *	\param pContext the context of the creation
 *	\return true if successful
 *
 * This overloads the CBEType::CreateBE function to implement user-defined type specific behaviour.
 */
bool CBEUserDefinedType::CreateBackEnd(CFETypeSpec * pFEType, CBEContext * pContext)
{
    if (!pFEType)
    {
        VERBOSE("%s failed because FE Type is 0\n", __PRETTY_FUNCTION__);
        return false;
    }

    if (!pFEType->IsKindOf(RUNTIME_CLASS(CFEUserDefinedType)))
    {
        VERBOSE("%s failed because FE Type is not 'user defined'\n", __PRETTY_FUNCTION__);
        return false;
    }

    CFEUserDefinedType *pUserType = (CFEUserDefinedType *) pFEType;
    if (pUserType->GetName())
    {
        // find original type
        CFEFile *pFERoot = pFEType->GetRoot();
        assert(pFERoot);
        String sName, sUserName = pUserType->GetName();
        CFETypedDeclarator *pFETypedef = pFERoot->FindUserDefinedType(sUserName);
        if (pFETypedef)
            sName = pContext->GetNameFactory()->GetTypeName((CFEBase*)pFETypedef, sUserName, pContext);
        else
		{
		    CFEInterface *pFEInterface = pFERoot->FindInterface(sUserName);
			if (pFEInterface)
			    sName = pContext->GetNameFactory()->GetTypeName((CFEBase*)pFEInterface, sUserName, pContext);
		}
		if (sName.IsEmpty())
            sName = sUserName;
        if (!CreateBackEnd(sName, pContext))
        {
			VERBOSE("%s failed because no name set\n", __PRETTY_FUNCTION__);
            return false;
        }
    }

    m_nFEType = TYPE_USER_DEFINED;
    return true;
}

/**	\brief finds the size of a user defined type
 *	\param sTypeName the name of the type, we are looking for
 *	\return the size of the type in bytes
 *
 * This implementation finds the root of the tree and then searches downward
 * for the type.
 */
int CBEUserDefinedType::GetSizeOfType(String sTypeName)
{
    CBERoot *pRoot = GetRoot();
    assert(pRoot);
    CBETypedef *pTypedef = pRoot->FindTypedef(sTypeName);
    if (!pTypedef)
        return 0;
    return pTypedef->GetSize();
}

/**	\brief generates an exact copy of this class
 *	\return a reference to the new object
 */
CObject *CBEUserDefinedType::Clone()
{
    return new CBEUserDefinedType(*this);
}

/**	\brief get the user defined name
 *	\return a reference to the name
 */
String CBEUserDefinedType::GetName()
{
    return m_sName;
}

/**	\brief calculate the size of thise type
 *	\return the size in bytes.
 *
 * The size of the user defined type can be calculated by searching for the typedef with this name
 * and calling its type GetSize() function.
 */
int CBEUserDefinedType::GetSize()
{
    if (m_nSize == 0)
		m_nSize = GetSizeOfType(m_sName);
    return m_nSize;
}


/**	\brief writes cod eto initialize a variable of this type with a zero value
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * To initialize a user defined type with zero values, means to find the typedef and use its type
 * to write this initialization.
 */
void CBEUserDefinedType::WriteZeroInit(CBEFile * pFile, CBEContext * pContext)
{
    CBEType *pType = GetRealType();
	if (pType)
	{
		// test for array types, something like: typedef int five_ints[5];
		CBEDeclarator *pAlias = GetRealName();
		if (pAlias && pAlias->IsArray())
		{
			WriteZeroInitArray(pFile, pType, pAlias, pAlias->GetFirstArrayBound(), pContext);
			return;
		}
		// no array
		pType->WriteZeroInit(pFile, pContext);
		return;
	}
    CBEType::WriteZeroInit(pFile, pContext);
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
 *  \param pStack contains the declarator stack of constructed typed var-sized parameters
 *  \param pContext the context of the write operation
 */
void CBEUserDefinedType::WriteGetSize(CBEFile *pFile, CDeclaratorStack *pStack, CBEContext *pContext)
{
	CBEType *pType = GetRealType();
	CBEDeclarator *pAlias = GetRealName();
	// if the type is simple, but the alias is variable sized
	// then we need to the get max-size of the type
	// e.g. typedef small *small_var_array;
	if (pType && pType->IsSimpleType() &&
	    pAlias && (pAlias->GetStars() > 0))
	{
		int nMaxSize = pContext->GetSizes()->GetMaxSizeOfType(pType->GetFEType());
		pFile->Print("%d", nMaxSize);
	}
	else
	{
		if (pType)
			pType->WriteGetSize(pFile, pStack, pContext);
	}
}

/** \brief test if this is a simple type
 *  \return false
 */
bool CBEUserDefinedType::IsSimpleType()
{
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
		return pType->GetArrayDimensionCount() + pAlias->GetArrayDimensionCount();
	return 0;
}

/** \brief test if this type is a pointer type
 *  \return true if it is
 *
 * The CORBA_Object type is a pointer type
 */
bool CBEUserDefinedType::IsPointerType()
{
	CBEDeclarator *pAlias = GetRealName();
	// if the type is simple, but the alias is variable sized
	// then we have at least one array dimensions
	if (pAlias && (pAlias->GetStars() > 0))
		return true;
	// check the aliased type
	CBEType *pType = GetRealType();
	if (pType)
		return pType->IsPointerType();
	// no alias; fallback to base type
    return CBEType::IsPointerType();
}

/** \brief if this is the alias of another type, get this type
 *  \return the type of the typedef
 */
CBEType* CBEUserDefinedType::GetRealType()
{
    CBERoot *pRoot = GetRoot();
	assert(pRoot);
	CBETypedef *pTypedef = pRoot->FindTypedef(m_sName);
	if (pTypedef)
	    return pTypedef->GetType();
    return 0;
}

/** \brief if this is the alias of another type, get the alias of that type
 *  \return the alias of the typedef
 */
CBEDeclarator* CBEUserDefinedType::GetRealName()
{
    CBERoot *pRoot = GetRoot();
	assert(pRoot);
	CBETypedef *pTypedef = pRoot->FindTypedef(m_sName);
	if (pTypedef)
	    return pTypedef->GetAlias();
    return 0;
}

/** \brief writes the type without indirections
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEUserDefinedType::WriteIndirect(CBEFile* pFile,  CBEContext* pContext)
{
    if (IsPointerType())
	{
		CBEType *pType = GetRealType();
		if (pType)
		{
			pType->WriteIndirect(pFile, pContext);
			return;
		}
	}
	CBEType::WriteIndirect(pFile, pContext);
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

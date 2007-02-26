/**
 *	\file	dice/src/be/BEUserDefinedType.cpp
 *	\brief	contains the implementation of the class CBEUserDefinedType
 *
 *	\date	02/13/2002
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

#include "be/BEUserDefinedType.h"
#include "be/BEContext.h"
#include "be/BERoot.h"
#include "be/BETypedef.h"

#include "fe/FETypeSpec.h"
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
        return false;
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
    VERBOSE("CBEUserDefinedType::CreateBackEnd(front-end type)\n");
    if (!pFEType)
    {
        VERBOSE("CBEUserDefinedType::CreateBE failed because FE type is 0\n");
        return false;
    }

    if (!pFEType->IsKindOf(RUNTIME_CLASS(CFEUserDefinedType)))
    {
        VERBOSE("CBEUserDefinedType::CreateBE failed because FE type is not user defined\n");
        return false;
    }

    CFEUserDefinedType *pUserType = (CFEUserDefinedType *) pFEType;
    if (pUserType->GetName())
    {
        // find original type
        CFEFile *pFERoot = pFEType->GetRoot();
        ASSERT(pFERoot);
        CFETypedDeclarator *pFETypedef = pFERoot->FindUserDefinedType(pUserType->GetName());
        String sName;
        if (pFETypedef)
            sName = pContext->GetNameFactory()->GetTypeName((CFEBase*)pFETypedef, pUserType->GetName(), pContext);
        else
            sName = pUserType->GetName();
        if (!CreateBackEnd(sName, pContext))
        {
            VERBOSE("CBEUserDefinedType::CreateBE failed because no name set\n");
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
    ASSERT(pRoot);
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
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    CBETypedef *pTypedef = pRoot->FindTypedef(m_sName);
    if (pTypedef)
    {
        if (pTypedef->GetType())
        {
            pTypedef->GetType()->WriteZeroInit(pFile, pContext);
            return;
        }
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
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    CBETypedef *pTypedef = pRoot->FindTypedef(m_sName);
    if (pTypedef)
    {
        if (pTypedef->GetType())
            return pTypedef->GetType()->IsConstructedType();
    }
    return false;
}

/** \brief determines if this function writes zero init code
 *  \return the delegate result
 */
bool CBEUserDefinedType::DoWriteZeroInit()
{
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    CBETypedef *pTypedef = pRoot->FindTypedef(m_sName);
    if (pTypedef)
    {
        if (pTypedef->GetType())
        {
            return pTypedef->GetType()->DoWriteZeroInit();;
        }
    }
    return CBEType::DoWriteZeroInit();
}

/** \brief calls the WriteGetSize function of the original type
 *  \param pFile the file to write to
 *  \param pStack contains the declarator stack of constructed typed var-sized parameters
 *  \param pContext the context of the write operation
 */
void CBEUserDefinedType::WriteGetSize(CBEFile *pFile, CDeclaratorStack *pStack, CBEContext *pContext)
{
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    CBETypedef *pTypedef = pRoot->FindTypedef(m_sName);
    if (pTypedef)
    {
        CBEType *pType = pTypedef->GetType();
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

/**
 *	\file	dice/src/fe/FEInterface.cpp
 *	\brief	contains the implementation of the class CFEInterface
 *
 *	\date	01/31/2001
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

#include "fe/FEInterface.h"
#include "fe/FEIdentifier.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEOperation.h"
#include "fe/FEFile.h"
#include "fe/FESimpleType.h"
#include "fe/FEVersionAttribute.h"
#include "fe/FEIntAttribute.h"
#include "fe/FEConstructedType.h"
#include "fe/FETaggedStructType.h"
#include "fe/FETaggedUnionType.h"
#include "fe/FETaggedEnumType.h"

#include "CString.h"

/////////////////////////////////////////////////////////////////////
// Interface stuff
IMPLEMENT_DYNAMIC(CFEInterface)

CFEInterface::CFEInterface(Vector * pIAttributes, String sIName, Vector * pIBaseNames, Vector * pComponents)
: m_vAttributes(RUNTIME_CLASS(CFEAttribute)),
  m_vConstants(RUNTIME_CLASS(CFEConstDeclarator)),
  m_vTypedefs(RUNTIME_CLASS(CFETypedDeclarator)),
  m_vOperations(RUNTIME_CLASS(CFEOperation)),
  m_vTaggedDeclarators(RUNTIME_CLASS(CFEConstructedType))
{
    IMPLEMENT_DYNAMIC_BASE(CFEInterface, CFEFileComponent);

    if (pComponents)
    {
		VectorElement *pIter;
		for (pIter = pComponents->GetFirst(); pIter; pIter = pIter->GetNext())
		{
			if (pIter->GetElement())
			{
				if (pIter->GetElement()->IsKindOf(RUNTIME_CLASS(CFEConstDeclarator)))
				    AddConstant((CFEConstDeclarator*)pIter->GetElement());
				else if (pIter->GetElement()->IsKindOf(RUNTIME_CLASS(CFETypedDeclarator)))
					AddTypedef((CFETypedDeclarator*) pIter->GetElement());
				else if (pIter->GetElement()->IsKindOf(RUNTIME_CLASS(CFEOperation)))
					m_vOperations.Add(pIter->GetElement());
				else if (pIter->GetElement()->IsKindOf(RUNTIME_CLASS(CFEConstructedType)))
					AddTaggedDecl((CFEConstructedType*)pIter->GetElement());
				else
				{
					TRACE("Unknown Interface component: %s\n", pIter->GetElement()->GetClassName());
					ASSERT(false);
				}
			}
		}
	}
    m_pBaseInterfaces = 0;
    m_pDerivedInterfaces = 0;
    m_sInterfaceName = sIName;
    m_pBaseInterfaceNames = pIBaseNames;
    if (pIAttributes)
		m_vAttributes.Add(pIAttributes);
}

CFEInterface::CFEInterface(CFEInterface & src)
: CFEFileComponent(src),
  m_vAttributes(RUNTIME_CLASS(CFEAttribute)),
  m_vConstants(RUNTIME_CLASS(CFEConstDeclarator)),
  m_vTypedefs(RUNTIME_CLASS(CFETypedDeclarator)),
  m_vOperations(RUNTIME_CLASS(CFEOperation)),
  m_vTaggedDeclarators(RUNTIME_CLASS(CFEConstructedType))
{
    IMPLEMENT_DYNAMIC_BASE(CFEInterface, CFEFileComponent);

    m_sInterfaceName = src.m_sInterfaceName;
    if (src.m_pBaseInterfaces)
    {
        m_pBaseInterfaces = src.m_pBaseInterfaces->Clone();
        //m_pBaseInterfaces->SetParentOfElements(this);
    }
    else
        m_pBaseInterfaces = 0;
    if (src.m_pDerivedInterfaces)
        m_pDerivedInterfaces = src.m_pDerivedInterfaces->Clone();
    else
        m_pDerivedInterfaces = 0;
    if (src.m_pBaseInterfaceNames)
    {
        m_pBaseInterfaceNames = src.m_pBaseInterfaceNames->Clone();
        m_pBaseInterfaceNames->SetParentOfElements(this);
    }
    else
        m_pBaseInterfaceNames = 0;
	m_vAttributes.Add(&src.m_vAttributes);
	m_vConstants.Add(&src.m_vConstants);
	m_vTypedefs.Add(&src.m_vTypedefs);
	m_vOperations.Add(&src.m_vOperations);
	m_vTaggedDeclarators.Add(&src.m_vTaggedDeclarators);
}

/** destructs the interface and all its members */
CFEInterface::~CFEInterface()
{
    if (m_pBaseInterfaceNames)
        delete m_pBaseInterfaceNames;
    if (m_pBaseInterfaces)
        delete m_pBaseInterfaces;
}

/**
 *	\brief returns the iterator for the first operation
 *	\return the iterator, which points to the first operation
 */
VectorElement *CFEInterface::GetFirstOperation()
{
	return m_vOperations.GetFirst();
}

/**
 *	\brief returns the next operations
 *	\param iter the iterator, which points to the next operation
 *	\return a pointer to the next operation
 */
CFEOperation *CFEInterface::GetNextOperation(VectorElement * &iter)
{
	if (!iter)
		return 0;
	CFEOperation *pRet = (CFEOperation*)iter->GetElement();
	iter = iter->GetNext();
	if (!pRet)
		return GetNextOperation(iter);
	return pRet;
}

/**
 *	\brief returns the name of the interface
 *	\return the interface's name
 *
 * This function redirects the request to the interface's header, which contains all
 * names and attributes of the interface.
 */
String CFEInterface::GetName()
{
    // if we got an identifier get it's name
    return m_sInterfaceName;
}

/**
 *	\brief retrieves the first type definition
 *	\return an iterator, which points to the first type definition
 */
VectorElement *CFEInterface::GetFirstTypeDef()
{
	return m_vTypedefs.GetFirst();
}

/**
 *	\brief retrieves the next type definition
 *	\param iter the pointer to the next type definition
 *	\return a pointer to the object, containing the type definition
 */
CFETypedDeclarator *CFEInterface::GetNextTypeDef(VectorElement * &iter)
{
	if (!iter)
		return 0;
	CFETypedDeclarator *pRet = (CFETypedDeclarator*)iter->GetElement();
	iter = iter->GetNext();
	if (!pRet)
		return GetNextTypeDef(iter);
	return pRet;
}

/**
 *	\brief retrieves a iterator pointing at the first const declarator
 *	\return the iterator pointing at the first const daclarator
 */
VectorElement *CFEInterface::GetFirstConstant()
{
	return m_vConstants.GetFirst();
}

/**
 *	\brief retrieves the next constant declarator
 *	\param iter the iterator pointing at the next const declarator
 *	\return a pointer to the object containing the next constant declarator
 */
CFEConstDeclarator *CFEInterface::GetNextConstant(VectorElement * &iter)
{
	if (!iter)
		return 0;
	CFEConstDeclarator *pRet = (CFEConstDeclarator*)iter->GetElement();
	iter = iter->GetNext();
	if (!pRet)
		return GetNextConstant(iter);
	return pRet;
}

/**
 *	\brief adds a reference to a base interface
 *	\param pBaseInterface the reference to the base interface
 *
 * Creates a new array for the references if none exists or adds the given reference
 * to the existing array (m_pBaseInterfaces).
 */
void CFEInterface::AddBaseInterface(CFEInterface * pBaseInterface)
{
    if (!m_pBaseInterfaces)
        m_pBaseInterfaces = new Vector(RUNTIME_CLASS(CFEInterface));
    if (pBaseInterface)
    {
        m_pBaseInterfaces->Add(pBaseInterface);
        pBaseInterface->AddDerivedInterface(this);
    }
}

/**
 *	\brief retrieves a iterator to the first base interface
 *	\return the iterator pointing at the first base interface
 */
VectorElement *CFEInterface::GetFirstBaseInterface()
{
    if (!m_pBaseInterfaces)
        return 0;
    return m_pBaseInterfaces->GetFirst();
}

/**
 *	\brief retrieves next base interface
 *	\param iter the iterator pointing at the next base interface
 *	\return a pointer to the object containing the next base interface, 0 if none available
 */
CFEInterface *CFEInterface::GetNextBaseInterface(VectorElement * &iter)
{
    if (!m_pBaseInterfaces)
        return 0;
    if (!iter)
        return 0;
    CFEInterface *pRet = (CFEInterface *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/**
 *	\brief adds a reference to a derived interface
 *	\param pDerivedInterface the reference to the derived interface
 *
 * Creates a new array for the references if none exists or adds the given reference
 * to the existing array (m_pDerivedInterfaces).
 */
void CFEInterface::AddDerivedInterface(CFEInterface * pDerivedInterface)
{
    if (!m_pDerivedInterfaces)
        m_pDerivedInterfaces = new Vector(RUNTIME_CLASS(CFEInterface));
    if (pDerivedInterface)
        m_pDerivedInterfaces->Add(pDerivedInterface);
}

/**
 *	\brief retrieves a iterator to the first derived interface
 *	\return the iterator pointing at the first derived interface
 */
VectorElement *CFEInterface::GetFirstDerivedInterface()
{
    if (!m_pDerivedInterfaces)
        return 0;
    return m_pDerivedInterfaces->GetFirst();
}

/**
 *	\brief retrieves next derived interface
 *	\param iter the iterator pointing at the next derived interface
 *	\return a pointer to the object containing the next derived interface, 0 if none available
 */
CFEInterface *CFEInterface::GetNextDerivedInterface(VectorElement * &iter)
{
    if (!m_pDerivedInterfaces)
        return 0;
    if (!iter)
        return 0;
    CFEInterface *pRet = (CFEInterface *) (iter->GetElement());
    iter = iter->GetNext();
    if (!pRet)
		return GetNextDerivedInterface(iter);
    return pRet;
}

/**
 *	\brief tries to locate a constant declarator by its name
 *	\param sName the name of the constant declarator
 *	\return a reference to the object containing the declarator, 0 if none was found
 */
CFEConstDeclarator *CFEInterface::FindConstant(String sName)
{
    CFEConstDeclarator *pConst;
    VectorElement *pIterIComponent = GetFirstConstant();
    while ((pConst = GetNextConstant(pIterIComponent)) != 0)
    {
        if (pConst->GetName() == sName)
            return pConst;
    }
    return 0;
}

/**
 *	\brief calculates the number of operations in this interface
 *	\param bCountBase true if the base interfaces should be counted too
 *	\return the number of operations (functions) in this interface
 *
 * This function is used for the enumeration of the operation identifiers, used
 * to identify, which function is called.
 */
int CFEInterface::GetOperationCount(bool bCountBase)
{
    int count = 0;
    if (bCountBase)
    {
        VectorElement *pIterI = GetFirstBaseInterface();
        CFEInterface *pInterface;
        while ((pInterface = GetNextBaseInterface(pIterI)) != 0)
        {
            count += pInterface->GetOperationCount();
        }
    }
    // now count functions
    VectorElement *pIterO = GetFirstOperation();
    while (GetNextOperation(pIterO))
    {
        count++;
    }
    return count;
}

/**
 *	\brief tries to locate a user defined type by its name
 *	\param sName the name of the type
 *	\return a pointer to the object containing the searched type, 0 if not found
 */
CFETypedDeclarator *CFEInterface::FindUserDefinedType(String sName)
{
    VectorElement *pIterT = GetFirstTypeDef();
    CFETypedDeclarator *pUserType;
    while ((pUserType = GetNextTypeDef(pIterT)) != 0)
    {
        if (pUserType->FindDeclarator(sName))
            return pUserType;
    }
    return 0;
}

/**
 *	\brief tries to find a base interface
 *	\param sName the name of the base interface
 *	\return a reference to the searched interface, 0 if not found
 */
CFEInterface *CFEInterface::FindBaseInterface(String sName)
{
    if (sName.IsEmpty())
        return 0;
    VectorElement *pIter = GetFirstBaseInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextBaseInterface(pIter)) != 0)
    {
        if (pInterface->GetName() == sName)
            return pInterface;
    }
    return 0;
}

/** \brief test all values, if they are consistent
 *  \return true, if the interface's syntax and gramar are o.k.
 *
 * This function is used by the parser to check the interface's consistency, after
 * it has been created.
 *
 * It is also used later by the compiler to run a global consistency check.
 *
 * First it checks whether the base interfaces are really defined. Then it checks
 * it's typedefs and constants and finally the operations.
 *
 * If the interface has a version attribute specified and the no-check-version is NOT
 * specified we have to add a function which retrieves the version from the server.
 * <code> void check_version([out] int *major, [out] int *minor); </code>
 * Because we have right here no clue about any compiler options, we only test for
 * the attribute and later (when writing the target files) we test the compiler option.
 * Usually operation don't have a version attribute. But to distinguish the check-version
 * operation from the other operations we copy the version attribute to the new operation.
 *
 */
bool CFEInterface::CheckConsistency()
{
    // set base interfaces
    CFEFile *pRoot = GetRoot();
    ASSERT(pRoot);
    VectorElement *pIter = GetFirstBaseInterfaceName();
    CFEIdentifier *pBaseName = 0;
    while ((pBaseName = GetNextBaseInterfaceName(pIter)) != 0)
    {
        CFEInterface *pBase = pRoot->FindInterface(pBaseName->GetName());
        if (pBase)
        {
            // check if interface is already referenced
            if (!FindBaseInterface(pBaseName->GetName()))
                AddBaseInterface(pBase);
        }
        else
        {
            CCompiler::GccError(this, 0, "Base interface %s not declared.", (const char *) (pBaseName->GetName()));	// 0 character appended by sprintf
            return false;
        }
    }
    // test for version attribute
    CFEAttribute *pVersionAttr = FindAttribute(ATTR_VERSION);
    if (pVersionAttr)
    {
        int nMajor, nMinor;
        ((CFEVersionAttribute *) pVersionAttr)->GetVersion(nMajor, nMinor);
        if ((nMajor) || (nMinor))
        {
            // version(0.0) is a reserved version number for internal use
            // we add a function which returns bool and has NO parameters
            // the check is hard coded on equal major and minor numbers
            // create return type
            CFETypeSpec *pType = new CFESimpleType(TYPE_BOOLEAN);
            // create operation
            CFEVersionAttribute *pOpAttr = (CFEVersionAttribute *) (pVersionAttr->Clone());
            CFEOperation *pOperation = new CFEOperation(pType, String("check_version"), 0, new Vector(RUNTIME_CLASS(CFEAttribute), 1, pOpAttr));
            pType->SetParent(pOperation);
            pOpAttr->SetParent(pOperation);
            // add operation
            AddOperation(pOperation);
        }
    }
    // check typedefs
    pIter = GetFirstTypeDef();
    CFETypedDeclarator *pTypedef;
    while ((pTypedef = GetNextTypeDef(pIter)) != 0)
    {
        if (!(pTypedef->CheckConsistency()))
            return false;
    }
    // check constants
    pIter = GetFirstConstant();
    CFEConstDeclarator *pConst;
    while ((pConst = GetNextConstant(pIter)) != 0)
    {
        if (!(pConst->CheckConsistency()))
            return false;
    }
    // check operations
    pIter = GetFirstOperation();
    CFEOperation *pOp;
    while ((pOp = GetNextOperation(pIter)) != 0)
    {
        if (!(pOp->CheckConsistency()))
            return false;
    }
    // we ran straight through, so we are clean
    return true;
}

/**
 *	\brief reutrns a pointer to the first base interface name
 *	\return an iterator, which points to the first base interface name
 */
VectorElement *CFEInterface::GetFirstBaseInterfaceName()
{
    if (!m_pBaseInterfaceNames)
        return 0;
    return m_pBaseInterfaceNames->GetFirst();
}

/**
 *	\brief return the next base interface name
 *	\param iter the iterator pointing to the next name
 *	\return the next base interface name
 */
CFEIdentifier *CFEInterface::GetNextBaseInterfaceName(VectorElement * &iter)
{
    if (!m_pBaseInterfaceNames)
        return 0;
    if (!iter)
        return 0;
    CFEIdentifier *pRet = (CFEIdentifier *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/**	creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEInterface::Clone()
{
    return new CFEInterface(*this);
}

/** retrieves a pointer to the first attribute
 *	\return a pointer to the first attribute
 */
VectorElement *CFEInterface::GetFirstAttribute()
{
	return m_vAttributes.GetFirst();
}

/** \brief retrieves a reference to the next attribute
 *  \param iter a pointer to the next attribute
 *  \return a reference to the next attribute
 */
CFEAttribute *CFEInterface::GetNextAttribute(VectorElement * &iter)
{
    if (!iter)
        return 0;
    CFEAttribute *pRet = (CFEAttribute *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/** tries to find an specific attribute
 *	\param nType the type of the attribute to find
 *	\return a reference to the attribute, or 0 if not found
 */
CFEAttribute *CFEInterface::FindAttribute(ATTR_TYPE nType)
{
    VectorElement *pIter = GetFirstAttribute();
    CFEAttribute *pAttr;
    while ((pAttr = GetNextAttribute(pIter)) != 0)
    {
        if (pAttr->GetAttrType() == nType)
            return pAttr;
    }
    return 0;
}

/** adds an operation to the operation vector
 *	\param pOperation the new operation to add
 */
void CFEInterface::AddOperation(CFEOperation * pOperation)
{
	if (!pOperation)
		return;
	m_vOperations.Add(pOperation);
    pOperation->SetParent(this);
}

/** for debugging purposes only */
void CFEInterface::Dump()
{
    printf("Dump: CFEInterface (%s) parent %s at 0x%x\n",
           (const char *) GetName(), (const char *) GetParent()->GetClassName(),
           (unsigned int) GetParent());
    printf("Dump: CFEInterface (%s): typedefs\n", (const char *) GetName());
    VectorElement *pIter = GetFirstTypeDef();
    CFEBase *pElement;
    while ((pElement = GetNextTypeDef(pIter)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEInterface (%s): constants\n", (const char *) GetName());
    pIter = GetFirstConstant();
    while ((pElement = GetNextConstant(pIter)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEInterface (%s): operations\n", (const char *) GetName());
    pIter = GetFirstOperation();
    while ((pElement = GetNextOperation(pIter)) != 0)
    {
        pElement->Dump();
    }
}

/** serializes this object to/from a file
 *	\param pFile the file to serialize to/from
 */
void CFEInterface::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<interface>\n");
        pFile->IncIndent();
        pFile->PrintIndent("<name>%s</name>\n", (const char *) GetName());
        // write base interfaces' names
        VectorElement *pIter = GetFirstBaseInterfaceName();
        CFEBase *pElement;
        while ((pElement = GetNextBaseInterfaceName(pIter)) != 0)
        {
            pFile->PrintIndent("<baseinterface>%s</baseinterface>\n",
                               (const char*)((CFEIdentifier *)pElement)->GetName());
        }
        // write attributes
        pIter = GetFirstAttribute();
        while ((pElement = GetNextAttribute(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write constants
        pIter = GetFirstConstant();
        while ((pElement = GetNextConstant(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write typedefs
        pIter = GetFirstTypeDef();
        while ((pElement = GetNextTypeDef(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write operations
        pIter = GetFirstOperation();
        while ((pElement = GetNextOperation(pIter)) != 0)
        {
            pElement->Serialize(pFile);
        }
        pFile->DecIndent();
        pFile->PrintIndent("</interface>\n");
    }
}

/** \brief adds a new type definition to the interface
 *  \param pFETypedef the type defintion to add
 */
void CFEInterface::AddTypedef(CFETypedDeclarator *pFETypedef)
{
	m_vTypedefs.Add(pFETypedef);
	pFETypedef->SetParent(this);
}

/** \brief adds a new constant to the interface
 *  \param pFEConstant the constant to add
 */
void CFEInterface::AddConstant(CFEConstDeclarator *pFEConstant)
{
	m_vConstants.Add(pFEConstant);
	pFEConstant->SetParent(this);
}

/** \brief adds a constructed type to the interface
 *  \param pFETaggedDecl the new tagged declarator
 */
void CFEInterface::AddTaggedDecl(CFEConstructedType *pFETaggedDecl)
{
	m_vTaggedDeclarators.Add(pFETaggedDecl);
	pFETaggedDecl->SetParent(this);
}

/** \brief get a pointer to the first tagged type declaration
 *  \return a pointer to the first tagged type decl
 */
VectorElement* CFEInterface::GetFirstTaggedDecl()
{
    return m_vTaggedDeclarators.GetFirst();
}

/** \brief get a reference to the next tagged type declaration
 *  \param pIter the pointer to the next tagged type decl
 *  \return a reference to the next tagged type decl or 0
 */
CFEConstructedType* CFEInterface::GetNextTaggedDecl(VectorElement* &pIter)
{
    if (!pIter)
        return 0;
    CFEConstructedType *pRet = (CFEConstructedType*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextTaggedDecl(pIter);
    return pRet;
}

/** \brief tests if this is a foward declaration
 *  \return true if it is
 *
 * A forward declaration contains no elements
 */
bool CFEInterface::IsForward()
{
    return (m_vAttributes.GetSize() == 0) &&
           (m_vConstants.GetSize() == 0) &&
           (m_vOperations.GetSize() == 0) &&
           (m_vTaggedDeclarators.GetSize() == 0) &&
           (m_vTypedefs.GetSize() == 0);
}

/** \brief adds attributes
 *  \param pSrcAttributes the source of the attributes
 */
void CFEInterface::AddAttributes(Vector *pSrcAttributes)
{
    m_vAttributes.Add(pSrcAttributes);
}

/** \brief adds base interface names
 *  \param pSrcNames the names to add
 */
void CFEInterface::AddBaseInterfaceNames(Vector *pSrcNames)
{
    if (!m_pBaseInterfaceNames)
        m_pBaseInterfaceNames = new Vector(RUNTIME_CLASS(CFEIdentifier));
    m_pBaseInterfaceNames->Add(pSrcNames);
}

/** \brief search for a tagged decl
 *  \param sName the tag (name) of the tagged decl to search for
 *  \return a reference to the found tagged decl or NULL if none found
 */
CFEConstructedType* CFEInterface::FindTaggedDecl(String sName)
{
    // own tagged decls
    VectorElement *pIter = GetFirstTaggedDecl();
    CFEConstructedType* pTaggedDecl;
    while ((pTaggedDecl = GetNextTaggedDecl(pIter)) != 0)
    {
        if (pTaggedDecl->IsKindOf(RUNTIME_CLASS(CFETaggedStructType)))
            if (((CFETaggedStructType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (pTaggedDecl->IsKindOf(RUNTIME_CLASS(CFETaggedUnionType)))
            if (((CFETaggedUnionType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (pTaggedDecl->IsKindOf(RUNTIME_CLASS(CFETaggedEnumType)))
            if (((CFETaggedEnumType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
    }
    // nothing found
    return 0;
}

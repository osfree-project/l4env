/**
 *    \file    dice/src/fe/FEInterface.cpp
 *    \brief   contains the implementation of the class CFEInterface
 *
 *    \date    01/31/2001
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
#include "fe/FEAttributeDeclarator.h"
#include "FELibrary.h"
#include "Compiler.h"
#include <string>
#include "File.h"
#include <typeinfo>
using namespace std;

/////////////////////////////////////////////////////////////////////
// Interface stuff
CFEInterface::CFEInterface(vector<CFEAttribute*> * pIAttributes,
    string sIName,
    vector<CFEIdentifier*> *pIBaseNames,
    vector<CFEInterfaceComponent*> *pComponents)
{
    if (pComponents)
    {
        vector<CFEInterfaceComponent*>::iterator iter;
        for (iter = pComponents->begin(); iter != pComponents->end(); iter++)
        {
            if (!*iter)
                continue;
            // parent is set in Add* functions
            if (dynamic_cast<CFEConstDeclarator*>(*iter))
                AddConstant((CFEConstDeclarator*)*iter);
            else if (dynamic_cast<CFETypedDeclarator*>(*iter))
                AddTypedef((CFETypedDeclarator*) *iter);
            else if (dynamic_cast<CFEOperation*>(*iter))
                AddOperation((CFEOperation*)*iter);
            else if (dynamic_cast<CFEConstructedType*>(*iter))
                AddTaggedDecl((CFEConstructedType*)*iter);
            else if (dynamic_cast<CFEAttributeDeclarator*>(*iter))
                AddAttributeDeclarator((CFEAttributeDeclarator*)*iter);
            else
            {
                TRACE("Unknown Interface component: %s\n", typeid(*(*iter)).name());
                assert(false);
            }
        }
    }

    m_vBaseInterfaces.clear();
    m_vDerivedInterfaces.clear();
    m_sInterfaceName = sIName;

    if (pIBaseNames)
        m_vBaseInterfaceNames.swap(*pIBaseNames);
    vector<CFEIdentifier*>::iterator iterB;
    for (iterB = m_vBaseInterfaceNames.begin();
         iterB != m_vBaseInterfaceNames.end(); iterB++)
    {
        (*iterB)->SetParent(this);
    }

    if (pIAttributes)
        m_vAttributes.swap(*pIAttributes);
    vector<CFEAttribute*>::iterator iterA;
    for (iterA = m_vAttributes.begin(); iterA != m_vAttributes.end(); iterA++)
    {
        (*iterA)->SetParent(this);
    }
}

CFEInterface::CFEInterface(CFEInterface & src)
: CFEFileComponent(src)
{
    m_sInterfaceName = src.m_sInterfaceName;

    COPY_VECTOR_WOP(CFEInterface, m_vBaseInterfaces, iterBI);
    COPY_VECTOR_WOP(CFEInterface, m_vDerivedInterfaces, iterDI);

    COPY_VECTOR(CFEIdentifier, m_vBaseInterfaceNames, iterBIN);
    COPY_VECTOR(CFEAttributeDeclarator, m_vAttributeDeclarators, iterAD);
    COPY_VECTOR(CFEAttribute, m_vAttributes, iterA);
    COPY_VECTOR(CFEConstDeclarator, m_vConstants, iterC);
    COPY_VECTOR(CFETypedDeclarator, m_vTypedefs, iterT);
    COPY_VECTOR(CFEOperation, m_vOperations, iterO);
    COPY_VECTOR(CFEConstructedType, m_vTaggedDeclarators, iterTD);
}

/** destructs the interface and all its members */
CFEInterface::~CFEInterface()
{
    DEL_VECTOR(m_vBaseInterfaceNames);
    DEL_VECTOR(m_vAttributeDeclarators);
    DEL_VECTOR(m_vAttributes);
    DEL_VECTOR(m_vConstants);
    DEL_VECTOR(m_vTypedefs);
    DEL_VECTOR(m_vOperations);
    DEL_VECTOR(m_vTaggedDeclarators);
}

/**
 *    \brief returns the iterator for the first operation
 *    \return the iterator, which points to the first operation
 */
vector<CFEOperation*>::iterator CFEInterface::GetFirstOperation()
{
    return m_vOperations.begin();
}

/**
 *    \brief returns the next operations
 *    \param iter the iterator, which points to the next operation
 *    \return a pointer to the next operation
 */
CFEOperation *CFEInterface::GetNextOperation(vector<CFEOperation*>::iterator &iter)
{
    if (iter == m_vOperations.end())
        return 0;
    return *iter++;
}

/**
 *    \brief returns the name of the interface
 *    \return the interface's name
 *
 * This function redirects the request to the interface's header, which contains all
 * names and attributes of the interface.
 */
string CFEInterface::GetName()
{
    // if we got an identifier get it's name
    return m_sInterfaceName;
}

/**
 *    \brief retrieves the first type definition
 *    \return an iterator, which points to the first type definition
 */
vector<CFETypedDeclarator*>::iterator CFEInterface::GetFirstTypeDef()
{
    return m_vTypedefs.begin();
}

/**
 *    \brief retrieves the next type definition
 *    \param iter the pointer to the next type definition
 *    \return a pointer to the object, containing the type definition
 */
CFETypedDeclarator *CFEInterface::GetNextTypeDef(vector<CFETypedDeclarator*>::iterator &iter)
{
    if (iter == m_vTypedefs.end())
        return 0;
    return *iter++;
}

/**
 *    \brief retrieves a iterator pointing at the first const declarator
 *    \return the iterator pointing at the first const daclarator
 */
vector<CFEConstDeclarator*>::iterator CFEInterface::GetFirstConstant()
{
    return m_vConstants.begin();
}

/**
 *    \brief retrieves the next constant declarator
 *    \param iter the iterator pointing at the next const declarator
 *    \return a pointer to the object containing the next constant declarator
 */
CFEConstDeclarator *CFEInterface::GetNextConstant(vector<CFEConstDeclarator*>::iterator &iter)
{
    if (iter == m_vConstants.end())
        return 0;
    return *iter++;
}

/**
 *    \brief adds a reference to a base interface
 *    \param pBaseInterface the reference to the base interface
 *
 * Creates a new array for the references if none exists or adds the given reference
 * to the existing array (m_pBaseInterfaces).
 */
void CFEInterface::AddBaseInterface(CFEInterface * pBaseInterface)
{
    if (!pBaseInterface)
        return;
    m_vBaseInterfaces.push_back(pBaseInterface);
    pBaseInterface->AddDerivedInterface(this);
}

/**
 *    \brief retrieves a iterator to the first base interface
 *    \return the iterator pointing at the first base interface
 */
vector<CFEInterface*>::iterator CFEInterface::GetFirstBaseInterface()
{
    return m_vBaseInterfaces.begin();
}

/**
 *    \brief retrieves next base interface
 *    \param iter the iterator pointing at the next base interface
 *    \return a pointer to the object containing the next base interface, 0 if none available
 */
CFEInterface *CFEInterface::GetNextBaseInterface(vector<CFEInterface*>::iterator &iter)
{
    if (iter == m_vBaseInterfaces.end())
        return 0;
    return *iter++;
}

/**
 *    \brief adds a reference to a derived interface
 *    \param pDerivedInterface the reference to the derived interface
 *
 * Creates a new array for the references if none exists or adds the given reference
 * to the existing array (m_pDerivedInterfaces).
 */
void CFEInterface::AddDerivedInterface(CFEInterface * pDerivedInterface)
{
    if (!pDerivedInterface)
        return;
    m_vDerivedInterfaces.push_back(pDerivedInterface);
}

/**
 *    \brief retrieves a iterator to the first derived interface
 *    \return the iterator pointing at the first derived interface
 */
vector<CFEInterface*>::iterator CFEInterface::GetFirstDerivedInterface()
{
    return m_vDerivedInterfaces.begin();
}

/**
 *    \brief retrieves next derived interface
 *    \param iter the iterator pointing at the next derived interface
 *    \return a pointer to the object containing the next derived interface, 0 if none available
 */
CFEInterface *CFEInterface::GetNextDerivedInterface(vector<CFEInterface*>::iterator &iter)
{
    if (iter == m_vDerivedInterfaces.end())
        return 0;
    return *iter++;
}

/**
 *    \brief tries to locate a constant declarator by its name
 *    \param sName the name of the constant declarator
 *    \return a reference to the object containing the declarator, 0 if none was found
 */
CFEConstDeclarator *CFEInterface::FindConstant(string sName)
{
    CFEConstDeclarator *pConst;
    vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
    while ((pConst = GetNextConstant(iterC)) != 0)
    {
        if (pConst->GetName() == sName)
            return pConst;
    }
    return 0;
}

/**
 *    \brief calculates the number of operations in this interface
 *    \param bCountBase true if the base interfaces should be counted too
 *    \return the number of operations (functions) in this interface
 *
 * This function is used for the enumeration of the operation identifiers, used
 * to identify, which function is called.
 */
int CFEInterface::GetOperationCount(bool bCountBase)
{
    int count = 0;
    if (bCountBase)
    {
        vector<CFEInterface*>::iterator iterI = GetFirstBaseInterface();
        CFEInterface *pInterface;
        while ((pInterface = GetNextBaseInterface(iterI)) != 0)
        {
            count += pInterface->GetOperationCount();
        }
    }
    // now count functions
    vector<CFEOperation*>::iterator iterO = GetFirstOperation();
    while (GetNextOperation(iterO))
    {
        count++;
    }
    return count;
}

/**
 *    \brief tries to locate a user defined type by its name
 *    \param sName the name of the type
 *    \return a pointer to the object containing the searched type, 0 if not found
 */
CFETypedDeclarator *CFEInterface::FindUserDefinedType(string sName)
{
    vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypeDef();
    CFETypedDeclarator *pUserType;
    while ((pUserType = GetNextTypeDef(iterT)) != 0)
    {
        if (pUserType->FindDeclarator(sName))
            return pUserType;
    }
    return 0;
}

/**
 *    \brief tries to find a base interface
 *    \param sName the name of the base interface
 *    \return a reference to the searched interface, 0 if not found
 */
CFEInterface *CFEInterface::FindBaseInterface(string sName)
{
    if (sName.empty())
        return 0;
    vector<CFEInterface*>::iterator iter = GetFirstBaseInterface();
    CFEInterface *pInterface;
    while ((pInterface = GetNextBaseInterface(iter)) != 0)
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
    CFEFile *pRoot = dynamic_cast<CFEFile*>(GetRoot());
    assert(pRoot);
    vector<CFEIdentifier*>::iterator iterBIN = GetFirstBaseInterfaceName();
    CFEIdentifier *pBaseName = 0;
    while ((pBaseName = GetNextBaseInterfaceName(iterBIN)) != 0)
    {
        CFEInterface *pBase = NULL;
        if (pBaseName->GetName().find("::") != string::npos)
            pBase = pRoot->FindInterface(pBaseName->GetName());
        else
        {
            CFELibrary *pFELibrary = GetSpecificParent<CFELibrary>();
            // should be in same library
            if (pFELibrary)
                pBase = pFELibrary->FindInterface(pBaseName->GetName());
            else // no library
                pBase = pRoot->FindInterface(pBaseName->GetName());
        }

        if (pBase)
        {
            // check if interface is already referenced
            if (!FindBaseInterface(pBaseName->GetName()))
                AddBaseInterface(pBase);
        }
        else
        {
            CCompiler::GccError(this, 0, "Base interface %s not declared.",
                pBaseName->GetName().c_str());    // 0 character appended by sprintf
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
            vector<CFEAttribute*> *pVecAttr = new vector<CFEAttribute*>();
            pVecAttr->push_back(pOpAttr);
            CFEOperation *pOperation = new CFEOperation(pType,
                string("check_version"), 0, pVecAttr);
            delete pVecAttr;
            pType->SetParent(pOperation);
            pOpAttr->SetParent(pOperation);
            // add operation
            AddOperation(pOperation);
        }
    }
    // check typedefs
    vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypeDef();
    CFETypedDeclarator *pTypedef;
    while ((pTypedef = GetNextTypeDef(iterT)) != 0)
    {
        if (!(pTypedef->CheckConsistency()))
            return false;
    }
    // check constants
    vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
    CFEConstDeclarator *pConst;
    while ((pConst = GetNextConstant(iterC)) != 0)
    {
        if (!(pConst->CheckConsistency()))
            return false;
    }
    /////////////////////////////////////////////////////////////
    // check if function name is used twice
    // check done here and not in parser, because we do not have
    // a simpol table in the parser (yet)
    vector<CFEOperation*>::iterator iterO = GetFirstOperation();
    CFEOperation *pOp;
    while ((pOp = GetNextOperation(iterO)) != 0)
    {
        vector<CFEOperation*>::iterator iterO2 = iterO;
        CFEOperation *pOp2;
        while ((pOp2 = GetNextOperation(iterO2)) != 0)
        {
            if ((pOp != pOp2) &&
                (pOp->GetName() == pOp2->GetName()))
            {
                CCompiler::GccWarning(pOp2, 0,
                    "Function name \"%s\" used before (here: %d)\n",
                    pOp2->GetName().c_str(), pOp->GetSourceLine());
                return false;
            }
        }
    }
    // check operations
    iterO = GetFirstOperation();
    while ((pOp = GetNextOperation(iterO)) != 0)
    {
        if (!(pOp->CheckConsistency()))
            return false;
    }
    // we ran straight through, so we are clean
    return true;
}

/**
 *    \brief reutrns a pointer to the first base interface name
 *    \return an iterator, which points to the first base interface name
 */
vector<CFEIdentifier*>::iterator CFEInterface::GetFirstBaseInterfaceName()
{
    return m_vBaseInterfaceNames.begin();
}

/**
 *    \brief return the next base interface name
 *    \param iter the iterator pointing to the next name
 *    \return the next base interface name
 */
CFEIdentifier *CFEInterface::GetNextBaseInterfaceName(vector<CFEIdentifier*>::iterator &iter)
{
    if (iter == m_vBaseInterfaceNames.end())
        return 0;
    return *iter++;
}

/**    creates a copy of this object
 *    \return a copy of this object
 */
CObject *CFEInterface::Clone()
{
    return new CFEInterface(*this);
}

/** retrieves a pointer to the first attribute
 *    \return a pointer to the first attribute
 */
vector<CFEAttribute*>::iterator CFEInterface::GetFirstAttribute()
{
    return m_vAttributes.begin();
}

/** \brief retrieves a reference to the next attribute
 *  \param iter a pointer to the next attribute
 *  \return a reference to the next attribute
 */
CFEAttribute *CFEInterface::GetNextAttribute(vector<CFEAttribute*>::iterator &iter)
{
    if (iter == m_vAttributes.end())
        return 0;
    return *iter++;
}

/** tries to find an specific attribute
 *    \param nType the type of the attribute to find
 *    \return a reference to the attribute, or 0 if not found
 */
CFEAttribute *CFEInterface::FindAttribute(ATTR_TYPE nType)
{
    vector<CFEAttribute*>::iterator iterA = GetFirstAttribute();
    CFEAttribute *pAttr;
    while ((pAttr = GetNextAttribute(iterA)) != 0)
    {
        if (pAttr->GetAttrType() == nType)
            return pAttr;
    }
    return 0;
}

/** adds an operation to the operation vector
 *    \param pOperation the new operation to add
 */
void CFEInterface::AddOperation(CFEOperation * pOperation)
{
    if (!pOperation)
        return;
    m_vOperations.push_back(pOperation);
    pOperation->SetParent(this);
}

/** for debugging purposes only */
void CFEInterface::Dump()
{
    printf("Dump: CFEInterface (%s) parent %s at 0x%x\n",
           GetName().c_str(), typeid(*GetParent()).name(),
           (unsigned int) GetParent());
    printf("Dump: CFEInterface (%s): typedefs\n", GetName().c_str());
    vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypeDef();
    CFEBase *pElement;
    while ((pElement = GetNextTypeDef(iterT)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEInterface (%s): constants\n", GetName().c_str());
    vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
    while ((pElement = GetNextConstant(iterC)) != 0)
    {
        pElement->Dump();
    }
    printf("Dump: CFEInterface (%s): operations\n", GetName().c_str());
    vector<CFEOperation*>::iterator iterO = GetFirstOperation();
    while ((pElement = GetNextOperation(iterO)) != 0)
    {
        pElement->Dump();
    }
}

/** serializes this object to/from a file
 *    \param pFile the file to serialize to/from
 */
void CFEInterface::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<interface>\n");
        pFile->IncIndent();
        pFile->PrintIndent("<name>%s</name>\n", GetName().c_str());
        // write base interfaces' names
        vector<CFEIdentifier*>::iterator iterBIN = GetFirstBaseInterfaceName();
        CFEBase *pElement;
        while ((pElement = GetNextBaseInterfaceName(iterBIN)) != 0)
        {
            pFile->PrintIndent("<baseinterface>%s</baseinterface>\n",
                               ((CFEIdentifier *)pElement)->GetName().c_str());
        }
        // write attributes
        vector<CFEAttribute*>::iterator iterA = GetFirstAttribute();
        while ((pElement = GetNextAttribute(iterA)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write constants
        vector<CFEConstDeclarator*>::iterator iterC = GetFirstConstant();
        while ((pElement = GetNextConstant(iterC)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write typedefs
        vector<CFETypedDeclarator*>::iterator iterT = GetFirstTypeDef();
        while ((pElement = GetNextTypeDef(iterT)) != 0)
        {
            pElement->Serialize(pFile);
        }
        // write operations
        vector<CFEOperation*>::iterator iterO = GetFirstOperation();
        while ((pElement = GetNextOperation(iterO)) != 0)
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
    if (!pFETypedef)
        return;
    m_vTypedefs.push_back(pFETypedef);
    pFETypedef->SetParent(this);
}

/** \brief adds a new constant to the interface
 *  \param pFEConstant the constant to add
 */
void CFEInterface::AddConstant(CFEConstDeclarator *pFEConstant)
{
    if (!pFEConstant)
        return;
    m_vConstants.push_back(pFEConstant);
    pFEConstant->SetParent(this);
}

/** \brief adds a constructed type to the interface
 *  \param pFETaggedDecl the new tagged declarator
 */
void CFEInterface::AddTaggedDecl(CFEConstructedType *pFETaggedDecl)
{
    if (!pFETaggedDecl)
        return;
    m_vTaggedDeclarators.push_back(pFETaggedDecl);
    pFETaggedDecl->SetParent(this);
}

/** \brief get a pointer to the first tagged type declaration
 *  \return a pointer to the first tagged type decl
 */
vector<CFEConstructedType*>::iterator CFEInterface::GetFirstTaggedDecl()
{
    return m_vTaggedDeclarators.begin();
}

/** \brief get a reference to the next tagged type declaration
 *  \param iter the pointer to the next tagged type decl
 *  \return a reference to the next tagged type decl or 0
 */
CFEConstructedType* CFEInterface::GetNextTaggedDecl(vector<CFEConstructedType*>::iterator &iter)
{
    if (iter == m_vTaggedDeclarators.end())
        return 0;
    return *iter++;
}

/** \brief tests if this is a foward declaration
 *  \return true if it is
 *
 * A forward declaration contains no elements
 */
bool CFEInterface::IsForward()
{
    return (m_vAttributes.size() == 0) &&
           (m_vConstants.size() == 0) &&
           (m_vOperations.size() == 0) &&
           (m_vTaggedDeclarators.size() == 0) &&
           (m_vTypedefs.size() == 0);
}

/** \brief adds attributes
 *  \param pSrcAttributes the source of the attributes
 */
void CFEInterface::AddAttributes(vector<CFEAttribute*> *pSrcAttributes)
{
    if (!pSrcAttributes)
        return;
    vector<CFEAttribute*>::iterator iter = pSrcAttributes->begin();
    for (; iter != pSrcAttributes->end(); iter++)
    {
        CFEAttribute *pNew = (CFEAttribute*)((*iter)->Clone());
        m_vAttributes.push_back(pNew);
        pNew->SetParent(this);
    }
}

/** \brief adds base interface names
 *  \param pSrcNames the names to add
 */
void CFEInterface::AddBaseInterfaceNames(vector<CFEIdentifier*> *pSrcNames)
{
    if (!pSrcNames)
        return;
    vector<CFEIdentifier*>::iterator iter = pSrcNames->begin();
    for (; iter != pSrcNames->end(); iter++)
    {
        CFEIdentifier *pNew = (CFEIdentifier*)((*iter)->Clone());
        m_vBaseInterfaceNames.push_back(pNew);
        pNew->SetParent(this);
    }
}

/** \brief search for a tagged decl
 *  \param sName the tag (name) of the tagged decl to search for
 *  \return a reference to the found tagged decl or NULL if none found
 */
CFEConstructedType* CFEInterface::FindTaggedDecl(string sName)
{
    // own tagged decls
    vector<CFEConstructedType*>::iterator iterTD = GetFirstTaggedDecl();
    CFEConstructedType* pTaggedDecl;
    while ((pTaggedDecl = GetNextTaggedDecl(iterTD)) != 0)
    {
        if (dynamic_cast<CFETaggedStructType*>(pTaggedDecl))
            if (((CFETaggedStructType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (dynamic_cast<CFETaggedUnionType*>(pTaggedDecl))
            if (((CFETaggedUnionType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
        if (dynamic_cast<CFETaggedEnumType*>(pTaggedDecl))
            if (((CFETaggedEnumType*)pTaggedDecl)->GetTag() == sName)
                return pTaggedDecl;
    }
    // nothing found
    return 0;
}

/** \brief add an attribute declarator
 *  \param pAttrDecl the declarator to add
 */
void CFEInterface::AddAttributeDeclarator(CFEAttributeDeclarator* pAttrDecl)
{
    if (!pAttrDecl)
        return;
    m_vAttributeDeclarators.push_back(pAttrDecl);
    pAttrDecl->SetParent(this);
}

/** \brief return a reference to the next attribute declarator
 *  \param iter the pointer to the next attribute declarator
 *  \return a reference to the next attribute declarator
 */
CFEAttributeDeclarator* CFEInterface::GetNextAttributeDeclarator(vector<CFEAttributeDeclarator*>::iterator &iter)
{
    if (iter == m_vAttributeDeclarators.end())
        return 0;
    return *iter++;
}

/** \brief retrun a pointer to the first attribute declarator
 *  \return a pointer to the first attribute declarator
 */
vector<CFEAttributeDeclarator*>::iterator CFEInterface::GetFirstAttributeDeclarator()
{
    return m_vAttributeDeclarators.begin();
}

/** \brief return a reference to the attribute declarator with the given name
 *  \param sName the name of the declarator to search for
 *  \return a reference to the attribute declarator found
 */
CFEAttributeDeclarator* CFEInterface::FindAttributeDeclarator(string sName)
{
    vector<CFEAttributeDeclarator*>::iterator iterAD = GetFirstAttributeDeclarator();
    CFEAttributeDeclarator *pAttrDecl;
    while ((pAttrDecl = GetNextAttributeDeclarator(iterAD)) != 0)
    {
        if (pAttrDecl->FindDeclarator(sName))
            return pAttrDecl;
    }
    return 0;
}

/**
 *    \file    dice/src/be/BENameSpace.cpp
 *    \brief   contains the implementation of the class CBENameSpace
 *
 *    \date    Tue Jun 25 2002
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

#include "be/BENameSpace.h"
#include "be/BEContext.h"
#include "be/BEClass.h"
#include "be/BENameSpace.h"
#include "be/BEConstant.h"
#include "be/BEType.h"
#include "be/BETypedef.h"
#include "be/BEAttribute.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEEnumType.h"
#include "be/BERoot.h"
#include "be/BEStructType.h"
#include "be/BEUnionType.h"

#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FEConstructedType.h"
#include "fe/FEIdentifier.h"

CBENameSpace::CBENameSpace()
{
}

/** destructs the class class */
CBENameSpace::~CBENameSpace()
{
    DEL_VECTOR(m_vClasses);
    DEL_VECTOR(m_vNestedNamespaces);
    DEL_VECTOR(m_vConstants);
    DEL_VECTOR(m_vTypedefs);
    DEL_VECTOR(m_vTypeDeclarations);
    DEL_VECTOR(m_vAttributes);
}


/** \brief adds an Class to the internal collextion
 *  \param pClass the Class to add
 */
void CBENameSpace::AddClass(CBEClass *pClass)
{
    if (!pClass)
        return;
    m_vClasses.push_back(pClass);
    pClass->SetParent(this);
}

/** \brief removes an Class from the collection
 *  \param pClass the Class to remove
 */
void CBENameSpace::RemoveClass(CBEClass *pClass)
{
    if (!pClass)
        return;
    vector<CBEClass*>::iterator iter;
    for (iter = m_vClasses.begin(); iter != m_vClasses.end(); iter++)
    {
        if (*iter == pClass)
        {
            m_vClasses.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first Class
 *  \return a pointer to the first Class
 */
vector<CBEClass*>::iterator CBENameSpace::GetFirstClass()
{
    return m_vClasses.begin();
}

/** \brief retrieves the next Class
 *  \param iter the pointer to the next Class
 *  \return a reference to the Class or 0 if end of collection
 */
CBEClass* CBENameSpace::GetNextClass(vector<CBEClass*>::iterator &iter)
{
    if (iter == m_vClasses.end())
        return 0;
    return *iter++;
}

/** \brief searches for a specific Class
 *  \param sClassName the name of the Class
 *  \return a reference to the Class or 0 if not found
 */
CBEClass* CBENameSpace::FindClass(string sClassName)
{
    // first search own Classes
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if (pClass->GetName() == sClassName)
            return pClass;
    }
    // then check nested libs
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNestedNameSpace;
    while ((pNestedNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if ((pClass = pNestedNameSpace->FindClass(sClassName)) != 0)
            return pClass;
    }
    return 0 ;
}

/** \brief adds a nested NameSpace to the internal collection
 *  \param pNameSpace the NameSpace to add
 */
void CBENameSpace::AddNameSpace(CBENameSpace* pNameSpace)
{
    if (!pNameSpace)
        return;
    m_vNestedNamespaces.push_back(pNameSpace);
    pNameSpace->SetParent(this);
}

/** \brief removes a nested NameSpace from the internal collection
 *  \param pNameSpace the NameSpace to remove
 */
void CBENameSpace::RemoveNameSpace(CBENameSpace *pNameSpace)
{
    if (!pNameSpace)
        return;
    vector<CBENameSpace*>::iterator iter;
    for (iter = m_vNestedNamespaces.begin(); iter != m_vNestedNamespaces.end(); iter++)
    {
        if (*iter == pNameSpace)
        {
            m_vNestedNamespaces.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first NameSpace
 *  \return a pointer to the first NameSpace
 */
vector<CBENameSpace*>::iterator CBENameSpace::GetFirstNameSpace()
{
    return m_vNestedNamespaces.begin();
}

/** \brief retrieves a reference to the next nested NameSpace
 *  \param iter the pointer to the next NameSpace
 *  \return a reference to the next NameSpace or 0 if no more libraries
 */
CBENameSpace* CBENameSpace::GetNextNameSpace(vector<CBENameSpace*>::iterator &iter)
{
    if (iter == m_vNestedNamespaces.end())
        return 0;
    return *iter++;
}

/** \brief tries to find a NameSpace
 *  \param sNameSpaceName the name of the NameSpace
 *  \return a reference to the NameSpace
 */
CBENameSpace* CBENameSpace::FindNameSpace(string sNameSpaceName)
{
    // then check nested libs
    vector<CBENameSpace*>::iterator iter = GetFirstNameSpace();
    CBENameSpace *pNestedNameSpace, *pFoundNameSpace;
    while ((pNestedNameSpace = GetNextNameSpace(iter)) != 0)
    {
        if (pNestedNameSpace->GetName() == sNameSpaceName)
            return pNestedNameSpace;
        if ((pFoundNameSpace = pNestedNameSpace->FindNameSpace(sNameSpaceName)) != 0)
            return pFoundNameSpace;
    }
    return 0;
}

/** \brief retrieves the name of the NameSpace
 *  \return the name of the lib
 */
string CBENameSpace::GetName()
{
    return m_sName;
}

/** \brief creates the back-end NameSpace
 *  \param pFELibrary the front-end library to use as reference
 *  \param pContext the context of the creation
 *  \return true if successful
 *
 * This function should be callable multiple types, because the library can be
 * defined mulitple times including different interfaces and such.
 */
bool CBENameSpace::CreateBackEnd(CFELibrary *pFELibrary, CBEContext *pContext)
{
    assert(pFELibrary);
    // set target file name
    SetTargetFileName(pFELibrary, pContext);
    // call base class to set source file information
    if (!CBEObject::CreateBackEnd(pFELibrary))
        return false;
    // set own name
    m_sName = pFELibrary->GetName();
    // search for attributes
    vector<CFEAttribute*>::iterator iterA = pFELibrary->GetFirstAttribute();
    CFEAttribute *pFEAttribute;
    while ((pFEAttribute = pFELibrary->GetNextAttribute(iterA)) != 0)
    {
        if (!CreateBackEnd(pFEAttribute, pContext))
            return false;
    }
    // search for constants
    vector<CFEConstDeclarator*>::iterator iterC = pFELibrary->GetFirstConstant();
    CFEConstDeclarator *pFEConstant;
    while ((pFEConstant = pFELibrary->GetNextConstant(iterC)) != 0)
    {
        if (!CreateBackEnd(pFEConstant, pContext))
            return false;
    }
    // search for types
    vector<CFETypedDeclarator*>::iterator iterT = pFELibrary->GetFirstTypedef();
    CFETypedDeclarator *pFETypedef;
    while ((pFETypedef = pFELibrary->GetNextTypedef(iterT)) != 0)
    {
        if (!CreateBackEnd(pFETypedef, pContext))
            return false;
    }
    // search for type declarations
    vector<CFEConstructedType*>::iterator iterCT = pFELibrary->GetFirstTaggedDecl();
    CFEConstructedType *pFETaggedType;
    while ((pFETaggedType = pFELibrary->GetNextTaggedDecl(iterCT)) != 0)
    {
        if (!CreateBackEnd(pFETaggedType, pContext))
            return false;
    }
    // search for libraries, which contains base interfaces of our own interfaces
    // we therefore iterate over the interfaces and try to find their base
    // interfaces in any nested library
    // we have to remember those libraries, so we won't initialize them twice
    vector<CFELibrary*> vLibraries;
    vector<CFEInterface*>::iterator iterI = pFELibrary->GetFirstInterface();
    CFEInterface *pFEInterface;
    CFELibrary *pNestedLib;
    while ((pFEInterface = pFELibrary->GetNextInterface(iterI)) != 0)
    {
        // get base interface names
        vector<CFEIdentifier*>::iterator iterBIN = pFEInterface->GetFirstBaseInterfaceName();
        CFEIdentifier *pFEBaseName;
        while ((pFEBaseName = pFEInterface->GetNextBaseInterfaceName(iterBIN)) != 0)
        {
            // search for interface
            CFEInterface *pFEBaseInterface = pFELibrary->FindInterface(pFEBaseName->GetName());
            if (pFEBaseInterface)
            {
                pNestedLib = pFEBaseInterface->GetSpecificParent<CFELibrary>();
                if (pNestedLib && (pNestedLib != pFELibrary))
                {
                    // init nested library
                    CBENameSpace *pNameSpace = FindNameSpace(pNestedLib->GetName());
                    if (!pNameSpace)
                    {
                        pNameSpace = pContext->GetClassFactory()->GetNewNameSpace();
                        AddNameSpace(pNameSpace);
                        if (!pNameSpace->CreateBackEnd(pNestedLib, pContext))
                        {
                            RemoveNameSpace(pNameSpace);
                            VERBOSE("CBENameSpace::CreateBE failed because NameSpace %s could not be created\n",
                                    pNestedLib->GetName().c_str());
                            return false;
                        }
                    }
                    else
                    {
                        // call create function again to add the interface of the redefined nested lib
                        if (!pNameSpace->CreateBackEnd(pNestedLib, pContext))
                        {
                            VERBOSE("CBENameSpace::CreateBE failed because NameSpace %s could not be re-created in scope of %s\n",
                                    pNestedLib->GetName().c_str(), m_sName.c_str());
                            return false;
                        }
                    }
                    vLibraries.push_back(pNestedLib);
                }
            }
        }
    }
    // search for interfaces
    // there might be some classes, which have base-classes defined in the libs,
    // which come afterwards.
    iterI = pFELibrary->GetFirstInterface();
    while ((pFEInterface = pFELibrary->GetNextInterface(iterI)) != 0)
    {
        if (!CreateBackEnd(pFEInterface, pContext))
            return false;
    }
    // search for libraries
    vector<CFELibrary*>::iterator iterL = pFELibrary->GetFirstLibrary();
    while ((pNestedLib = pFELibrary->GetNextLibrary(iterL)) != 0)
    {
        // if we initialized this lib already, skip it
        bool bSkip = false;
        vector<CFELibrary*>::iterator iter = vLibraries.begin();
        while ((iter != vLibraries.end()) && !bSkip)
        {
            if (*iter && (*iter == pNestedLib))
                bSkip = true;
            iter++;
        }
        if (bSkip)
            continue;
        CBENameSpace *pNameSpace = FindNameSpace(pNestedLib->GetName());
        if (!pNameSpace)
        {
            pNameSpace = pContext->GetClassFactory()->GetNewNameSpace();
            AddNameSpace(pNameSpace);
            if (!pNameSpace->CreateBackEnd(pNestedLib, pContext))
            {
                RemoveNameSpace(pNameSpace);
                VERBOSE("CBENameSpace::CreateBE failed because NameSpace %s could not be created\n",
                        pNestedLib->GetName().c_str());
                return false;
            }
        }
        else
        {
            // call create function again to add the interface of the redefined nested lib
            if (!pNameSpace->CreateBackEnd(pNestedLib, pContext))
            {
                VERBOSE("CBENameSpace::CreateBE failed because NameSpace %s could not be re-created in scope of %s\n",
                        pNestedLib->GetName().c_str(), m_sName.c_str());
                return false;
            }
        }
    }

    return true;
}

/** \brief internal function to create a backend of this NameSpace
 *  \param pFEInterface the respective front-end interface
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBENameSpace::CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext)
{
    CBEClass *pClass = NULL;
    // check if class already exists
    CBERoot *pRoot = GetSpecificParent<CBERoot>();
    assert(pRoot);
    pClass = pRoot->FindClass(pFEInterface->GetName());
    if (!pClass)
    {
        pClass = pContext->GetClassFactory()->GetNewClass();
        AddClass(pClass);
        // recreate class to add additional members
        if (!pClass->CreateBackEnd(pFEInterface, pContext))
        {
            RemoveClass(pClass);
            delete pClass;
            VERBOSE("CBENameSpace::CreateBE failed because class %s could not be created\n",
                pFEInterface->GetName().c_str());
            return false;
        }
    }
    // otherwise: it was the base class of some class and has been created by it directly
    return true;
}

/** \brief internal function to create constants
 *  \param pFEConstant the respective front-end constant
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBENameSpace::CreateBackEnd(CFEConstDeclarator *pFEConstant, CBEContext *pContext)
{
    CBEConstant *pConstant = pContext->GetClassFactory()->GetNewConstant();
    AddConstant(pConstant);
    if (!pConstant->CreateBackEnd(pFEConstant, pContext))
    {
        RemoveConstant(pConstant);
        delete pConstant;
        VERBOSE("CBENameSpace::CreateBE failed because constant %s could not be created\n",
                pFEConstant->GetName().c_str());
        return false;
    }
    return true;
}

/** \brief internal function to create typedef
 *  \param pFETypedef the respective front-end typedefinition
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBENameSpace::CreateBackEnd(CFETypedDeclarator *pFETypedef, CBEContext *pContext)
{
    CBETypedef *pTypedef = pContext->GetClassFactory()->GetNewTypedef();
    AddTypedef(pTypedef);
    pTypedef->SetParent(this);
    if (!pTypedef->CreateBackEnd(pFETypedef, pContext))
    {
        RemoveTypedef(pTypedef);
        delete pTypedef;
        VERBOSE("CBENameSpace::CreateBE failed because typedef could not be created\n");
        return false;
    }
    return true;
}

/** \brief internale function to add an attribute
 *  \param pFEAttribute the respective front.end attribute
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBENameSpace::CreateBackEnd(CFEAttribute *pFEAttribute, CBEContext *pContext)
{
    CBEAttribute *pAttribute = pContext->GetClassFactory()->GetNewAttribute();
    AddAttribute(pAttribute);
    if (!pAttribute->CreateBackEnd(pFEAttribute, pContext))
    {
        RemoveAttribute(pAttribute);
        delete pAttribute;
        VERBOSE("%s failed because attribute could not be created\n",
            __PRETTY_FUNCTION__);
        return false;
    }
    return true;
}

/** \brief adds a constant
 *  \param pConstant the const to add
 */
void CBENameSpace::AddConstant(CBEConstant *pConstant)
{
    if (!pConstant)
        return;
    m_vConstants.push_back(pConstant);
    pConstant->SetParent(this);
}

/** \brief removes a constant
 *  \param pConstant the const to remove
 */
void CBENameSpace::RemoveConstant(CBEConstant *pConstant)
{
    if (!pConstant)
        return;
    vector<CBEConstant*>::iterator iter;
    for (iter = m_vConstants.begin(); iter != m_vConstants.end(); iter++)
    {
        if (*iter == pConstant)
        {
            m_vConstants.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first constant
 *  \return a pointer to the first constant
 */
vector<CBEConstant*>::iterator CBENameSpace::GetFirstConstant()
{
    return m_vConstants.begin();
}

/** \brief retrieves a reference to the next constant
 *  \param iter the pointer to the next const
 *  \return a reference to the next const
 */
CBEConstant* CBENameSpace::GetNextConstant(vector<CBEConstant*>::iterator &iter)
{
    if (iter == m_vConstants.end())
        return 0;
    return *iter++;
}

/** \brief searches for a constants
 *  \param sConstantName the name of the constants
 *  \return a reference to the constant if found, zero otherwise
 */
CBEConstant* CBENameSpace::FindConstant(string sConstantName)
{
    if (sConstantName.empty())
        return 0;
    // simply scan the namespace for a match
    vector<CBEConstant*>::iterator iterCo = GetFirstConstant();
    CBEConstant *pConstant;
    while ((pConstant = GetNextConstant(iterCo)) != 0)
    {
        if (pConstant->GetName() == sConstantName)
            return pConstant;
    }
    // check classes
    vector<CBEClass*>::iterator iterCl = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterCl)) != 0)
    {
        if ((pConstant = pClass->FindConstant(sConstantName)) != 0)
            return pConstant;
    }
    // check namespaces
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if ((pConstant = pNameSpace->FindConstant(sConstantName)) != 0)
            return pConstant;
    }
    // nothing found
    return 0;
}


/** \brief adds a typedef
 *  \param pTypedef the typedef to add
 */
void CBENameSpace::AddTypedef(CBETypedef *pTypedef)
{
    if (!pTypedef)
        return;
    m_vTypedefs.push_back(pTypedef);
    pTypedef->SetParent(this);
}

/** \brief removes a typedef
 *  \param pTypedef the typedef to remove
 */
void CBENameSpace::RemoveTypedef(CBETypedef *pTypedef)
{
    if (!pTypedef)
        return;
    vector<CBETypedef*>::iterator iter;
    for (iter = m_vTypedefs.begin(); iter != m_vTypedefs.end(); iter++)
    {
        if (*iter == pTypedef)
        {
            m_vTypedefs.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first typedef
 *  \return a pointer to the first typedef
 */
vector<CBETypedef*>::iterator CBENameSpace::GetFirstTypedef()
{
    return m_vTypedefs.begin();
}

/** \brief retrieves a reference to the next typedef
 *  \param iter the pointer to the next typedef
 *  \return a reference to the typedef
 */
CBETypedef* CBENameSpace::GetNextTypedef(vector<CBETypedef*>::iterator &iter)
{
    if (iter == m_vTypedefs.end())
        return 0;
    return *iter++;
}

/** \brief adds an attribute
 *  \param pAttribute the attribute to add
 */
void CBENameSpace::AddAttribute(CBEAttribute *pAttribute)
{
    if (!pAttribute)
        return;
    m_vAttributes.push_back(pAttribute);
    pAttribute->SetParent(this);
}

/** \brief remove an attribute
 *  \param pAttribute the attribute to remove
 */
void CBENameSpace::RemoveAttribute(CBEAttribute *pAttribute)
{
    if (!pAttribute)
        return;
    vector<CBEAttribute*>::iterator iter;
    for (iter = m_vAttributes.begin(); iter != m_vAttributes.end(); iter++)
    {
        if (*iter == pAttribute)
        {
            m_vAttributes.erase(iter);
            return;
        }
    }
}

/** \brief retrieves the pointer to the first attribute
 *  \return a pointer to the first attribute
 */
vector<CBEAttribute*>::iterator CBENameSpace::GetFirstAttribute()
{
    return m_vAttributes.begin();
}

/** \brief retrieve a reference to the next attribute
 *  \param iter the pointer to the next attribute
 *  \return a reference to the attribute
 */
CBEAttribute* CBENameSpace::GetNextAttribute(vector<CBEAttribute*>::iterator &iter)
{
    if (iter == m_vAttributes.end())
        return 0;
    return *iter++;
}

/** \brief adds this NameSpace to the target file
 *  \param pHeader the header file to add to
 *  \param pContext the context of the operation
 *  \return true if successful
 *
 * This implementation adds its types, constants, interfaces and
 * nested libs.
 */
bool CBENameSpace::AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext)
{
    VERBOSE("CBENameSpace::AddToFile(header: %s) for namespace %s called\n",
        pHeader->GetFileName().c_str(), GetName().c_str());
    // add this namespace to the file
    if (IsTargetFile(pHeader))
        pHeader->AddNameSpace(this);
    return true;
}

/** \brief adds this namespace to the file
 *  \param pImpl the implementation file to add this namespace to
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBENameSpace::AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext)
{
    VERBOSE("CBENameSpace::AddToFile(impl: %s) for namespace %s called\n",
        pImpl->GetFileName().c_str(), GetName().c_str());
    // if compiler options for interface or function target file are set
    // iterate over interfaces and add them
    if (pContext->IsOptionSet(PROGRAM_FILE_FUNCTION) ||
        pContext->IsOptionSet(PROGRAM_FILE_INTERFACE))
    {
        vector<CBEClass*>::iterator iter = GetFirstClass();
        CBEClass *pClass;
        while ((pClass = GetNextClass(iter)) != 0)
        {
            pClass->AddToFile(pImpl, pContext);
        }
    }
    // add this namespace to the file
    // (needed for types, etc.)
    if (IsTargetFile(pImpl))
        pImpl->AddNameSpace(this);
    return true;
}

/** \brief adds the opcodes for this namespace to the file
 *  \param pFile the file to add the opcodes to
 *  \param pContext the context of the whole create process
 *  \return true if successful
 *
 * This implements the opcodes of the included classes and nested namespaces.
 */
bool CBENameSpace::AddOpcodesToFile(CBEHeaderFile *pFile, CBEContext *pContext)
{
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if (!pClass->AddOpcodesToFile(pFile, pContext))
            return false;
    }

    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if (!pNameSpace->AddOpcodesToFile(pFile, pContext))
            return false;
    }

    return true;
}

/** \brief writes the name-space to the header file
 *  \param pFile the header file to write to
 *  \param pContext the context of the operation
 *
 * For the C implementation the name-space simply calls the embedded
 * constants, types, classes, and namespaces.
 *
 * Before writing, we create an ordered list of contained element,
 * depending on their appearance in the source file. Then we iterate
 * over the generated list and write each element.
 */
void CBENameSpace::Write(CBEHeaderFile *pFile, CBEContext *pContext)
{
    // create ordered list
    CreateOrderedElementList();

    // for C++ write namespace opening
    if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
    {
	*pFile << "\tnamespace " << GetName() << "\n";
	*pFile << "\t{\n";
	pFile->IncIndent();
    }

    // write target file
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    int nLastType = 0, nCurrType = 0;
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        nCurrType = 0;
        if (dynamic_cast<CBEClass*>(*iter))
            nCurrType = 1;
        else if (dynamic_cast<CBENameSpace*>(*iter))
            nCurrType = 2;
        else if (dynamic_cast<CBEConstant*>(*iter))
            nCurrType = 3;
        else if (dynamic_cast<CBETypedef*>(*iter))
            nCurrType = 4;
        else if (dynamic_cast<CBEType*>(*iter))
            nCurrType = 5;
        if (nCurrType != nLastType)
        {
            *pFile << "\n";
            nLastType = nCurrType;
        }
        // add pre-processor directive to denote source line
        if (pContext->IsOptionSet(PROGRAM_GENERATE_LINE_DIRECTIVE) &&
            (nCurrType >= 1) && (nCurrType <= 5))
        {
            *pFile << "# " << (*iter)->GetSourceLine() <<  " \"" <<
                (*iter)->GetSourceFileName() << "\"\n";
        }
        switch (nCurrType)
        {
        case 1:
            WriteClass((CBEClass*)(*iter), pFile, pContext);
            break;
        case 2:
            WriteNameSpace((CBENameSpace*)(*iter), pFile, pContext);
            break;
        case 3:
            WriteConstant((CBEConstant*)(*iter), pFile, pContext);
            break;
        case 4:
            WriteTypedef((CBETypedef*)(*iter), pFile, pContext);
            break;
        case 5:
            WriteTaggedType((CBEType*)(*iter), pFile, pContext);
            break;
        default:
            break;
        }
    }

    // close namespace for C++
    if (pContext->IsBackEndSet(PROGRAM_BE_CPP))
    {
	pFile->DecIndent();
	*pFile << "\t}\n";
    }
}

/** \brief writes the name-space to the header file
 *  \param pFile the header file to write to
 *  \param pContext the context of the operation
 *
 * For the C implementation the name-space simply calls the embedded
 * classes and namespaces.
 */
void CBENameSpace::Write(CBEImplementationFile *pFile, CBEContext *pContext)
{
    // create ordered list
    CreateOrderedElementList();

    /** only write classes and interfaces if this is not
     * -fFfunction or -fFinterface (the classes have been added to the file
     * seperately, and are thus written by CBEFile::WriteClasses...
     */
    if (pContext->IsOptionSet(PROGRAM_FILE_FUNCTION) ||
        pContext->IsOptionSet(PROGRAM_FILE_INTERFACE))
        return;

    // write target file
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    int nLastType = 0, nCurrType = 0;
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        nCurrType = 0;
        if (dynamic_cast<CBEClass*>(*iter))
            nCurrType = 1;
        else if (dynamic_cast<CBENameSpace*>(*iter))
            nCurrType = 2;
        if ((nCurrType != nLastType) &&
            (nCurrType > 0))
        {
            *pFile << "\n";
            nLastType = nCurrType;
        }
        // add pre-processor directive to denote source line
        if (pContext->IsOptionSet(PROGRAM_GENERATE_LINE_DIRECTIVE) &&
            (nCurrType >= 1) && (nCurrType <= 2))
        {
            *pFile << "# " << (*iter)->GetSourceLine() <<  " \"" <<
                (*iter)->GetSourceFileName() << "\"\n";
        }
        switch (nCurrType)
        {
        case 1:
            WriteClass((CBEClass*)(*iter), pFile, pContext);
            break;
        case 2:
            WriteNameSpace((CBENameSpace*)(*iter), pFile, pContext);
            break;
        default:
            break;
        }
    }
}

/** \brief write a constant
 *  \param pClass the constant to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteConstant(CBEConstant *pConstant, CBEHeaderFile *pFile, CBEContext *pContext)
{
    assert(pConstant);
    pConstant->Write(pFile, pContext);
}

/** \brief write a type definition
 *  \param pTypedef the typedef to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteTypedef(CBETypedef *pTypedef, CBEHeaderFile *pFile, CBEContext *pContext)
{
    assert(pTypedef);
    pTypedef->WriteDeclaration(pFile, pContext);
}

/** \brief write a class
 *  \param pClass the class to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteClass(CBEClass *pClass, CBEImplementationFile *pFile, CBEContext *pContext)
{
    assert(pClass);
    pClass->Write(pFile, pContext);
}

/** \brief write a class
 *  \param pClass the class to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteClass(CBEClass *pClass, CBEHeaderFile *pFile, CBEContext *pContext)
{
    assert(pClass);
    pClass->Write(pFile, pContext);
}

/** \brief write a nested namespace
 *  \param pNameSpace the namespace to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteNameSpace(CBENameSpace *pNameSpace, CBEImplementationFile *pFile, CBEContext *pContext)
{
    assert(pNameSpace);
    pNameSpace->Write(pFile, pContext);
}

/** \brief write a nested namespace
 *  \param pNameSpace the namespace to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteNameSpace(CBENameSpace *pNameSpace, CBEHeaderFile *pFile, CBEContext *pContext)
{
    assert(pNameSpace);
    pNameSpace->Write(pFile, pContext);
}

/** \brief tries to find a function
 *  \param sFunctionName the name of the funtion to find
 *  \return a reference to the found function (or 0 if not found)
 *
 * Because the namespace does not have functions itself, it searches its classes and
 * netsed namespaces.
 */
CBEFunction* CBENameSpace::FindFunction(string sFunctionName)
{
    CBEFunction *pFunction;

    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if ((pFunction = pClass->FindFunction(sFunctionName)) != 0)
            return pFunction;
    }

    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if ((pFunction = pNameSpace->FindFunction(sFunctionName)) != 0)
            return  pFunction;
    }

    return 0;
}

/** \brief tries to find a type definition
 *  \param sTypeName the name of the searched typedef
 *  \return a reference to the found type definition
 */
CBETypedef* CBENameSpace::FindTypedef(string sTypeName)
{
    vector<CBETypedef*>::iterator iterT = GetFirstTypedef();
    CBETypedef *pTypedef;
    while ((pTypedef = GetNextTypedef(iterT)) != 0)
    {
        if (pTypedef->FindDeclarator(sTypeName) != 0)
            return pTypedef;
    }

    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    CBETypedDeclarator *pTypedDecl;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if ((pTypedDecl = pClass->FindTypedef(sTypeName)) != 0)
            if (dynamic_cast<CBETypedef*>(pTypedDecl))
                return (CBETypedef*)pTypedDecl;
    }

    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if ((pTypedef = pNameSpace->FindTypedef(sTypeName)) != 0)
            return pTypedef;
    }

    return 0;
}

/** \brief test if the given file is a target file for the namespace
 *  \param pFile the file to test
 *  \return true if name-space should be added to file
 *
 * A file is target file of a name-space if at least one of its classes or
 * nested name-spaces belongs to this class.
 */
bool CBENameSpace::IsTargetFile(CBEHeaderFile * pFile)
{
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if (pClass->IsTargetFile(pFile))
            return true;
    }
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if (pNameSpace->IsTargetFile(pFile))
            return true;
    }
    return false;
}

/** \brief test if the given file is a target file for the namespace
 *  \param pFile the file to test
 *  \return true if name-space should be added to file
 *
 * A file is target file of a name-space if at least one of its classes or
 * nested name-spaces belongs to this class.
 */
bool CBENameSpace::IsTargetFile(CBEImplementationFile * pFile)
{
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if (pClass->IsTargetFile(pFile))
            return true;
    }
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if (pNameSpace->IsTargetFile(pFile))
            return true;
    }
    return false;
}

/** \brief tries to find a type declaration of a tagged type
 *  \param nType the type (struct/enum/union) of the searched type
 *  \param sTag the tag of the type
 *  \return a reference to the found type or 0
 */
CBEType* CBENameSpace::FindTaggedType(int nType, string sTag)
{
    // search own types
    vector<CBEType*>::iterator iterT = GetFirstTaggedType();
    CBEType *pTypeDecl;
    while ((pTypeDecl = GetNextTaggedType(iterT)) != 0)
    {
        int nFEType = pTypeDecl->GetFEType();
        if (nType != nFEType)
            continue;
        if (nFEType == TYPE_TAGGED_STRUCT)
        {
            if ((CBEStructType*)(pTypeDecl)->HasTag(sTag))
                return pTypeDecl;
        }
        if (nFEType == TYPE_TAGGED_UNION)
        {
            if ((CBEUnionType*)(pTypeDecl)->HasTag(sTag))
                return pTypeDecl;
        }
        if (nFEType == TYPE_TAGGED_ENUM)
        {
            if ((CBEEnumType*)(pTypeDecl)->HasTag(sTag))
                return pTypeDecl;
        }
    }
    // search classes
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    CBEType *pType;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if ((pType = pClass->FindTaggedType(nType, sTag)) != 0)
            return pType;
    }
    // search nested namespaces
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if ((pType = pNameSpace->FindTaggedType(nType, sTag)) != 0)
            return pType;
    }
    return 0;
}

/** \brief adds a tagged type declaration
 *  \param pType the type to add
 */
void CBENameSpace::AddTaggedType(CBEType *pType)
{
    if (!pType)
        return;
    m_vTypeDeclarations.push_back(pType);
    pType->SetParent(this);
}

/** \brief removes the tagged type declaration
 *  \param pType the type to remove
 */
void CBENameSpace::RemoveTaggedType(CBEType *pType)
{
    if (!pType)
        return;
    vector<CBEType*>::iterator iter;
    for (iter = m_vTypeDeclarations.begin(); iter != m_vTypeDeclarations.end(); iter++)
    {
        if (*iter == pType)
        {
            m_vTypeDeclarations.erase(iter);
            return;
        }
    }
}

/** \brief tries to find the first type declaration
 *  \return a pointer to the first tagged type declaration
 */
vector<CBEType*>::iterator CBENameSpace::GetFirstTaggedType()
{
    return m_vTypeDeclarations.begin();
}

/** \brief retrieves a reference to the next tagged type declaration
 *  \param iter the pointer to the next tagged type declaration
 *  \return a reference to the next tagged type or 0 if no more
 */
CBEType* CBENameSpace::GetNextTaggedType(vector<CBEType*>::iterator &iter)
{
    if (iter == m_vTypeDeclarations.end())
        return 0;
    return *iter++;
}

/** \brief tries to create the back-end presentation of a type declaration
 *  \param pFEType the respective front-end type
 *  \param pContext the context of the creat process
 *  \return true if successful
 */
bool CBENameSpace::CreateBackEnd(CFEConstructedType *pFEType, CBEContext *pContext)
{
    CBEType *pType = pContext->GetClassFactory()->GetNewType(pFEType->GetType());
    AddTaggedType(pType);
    pType->SetParent(this);
    if (!pType->CreateBackEnd(pFEType, pContext))
    {
        RemoveTaggedType(pType);
        delete pType;
        VERBOSE("%s failed because tagged type could not be created\n",
            __PRETTY_FUNCTION__);
        return false;
    }
    return true;
}

/** \brief write a tagged type declaration
 *  \param pType the type to write
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBENameSpace::WriteTaggedType(CBEType *pType, CBEHeaderFile *pFile, CBEContext *pContext)
{
    assert(pType);
    string sTag;
    if (dynamic_cast<CBEStructType*>(pType))
        sTag = ((CBEStructType*)pType)->GetTag();
    if (dynamic_cast<CBEUnionType*>(pType))
        sTag = ((CBEUnionType*)pType)->GetTag();
    sTag = pContext->GetNameFactory()->GetTypeDefine(sTag, pContext);
    pFile->Print("#ifndef %s\n", sTag.c_str());
    pFile->Print("#define %s\n", sTag.c_str());
    pType->Write(pFile, pContext);
    pFile->Print(";\n");
    pFile->Print("#endif /* !%s */\n", sTag.c_str());
    pFile->Print("\n");
}

/** \brief search for a function with a specific type
 *  \param sTypeName the name of the type
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if a parameter of that type is found
 *
 * search own classe, namespaces and function for a function, which has
 * a parameter of that type
 */
bool CBENameSpace::HasFunctionWithUserType(string sTypeName, CBEFile *pFile, CBEContext *pContext)
{
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if (pNameSpace->HasFunctionWithUserType(sTypeName, pFile, pContext))
            return true;
    }
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if (pClass->HasFunctionWithUserType(sTypeName, pFile, pContext))
            return true;
    }
    return false;
}

/** \brief creates a list of ordered elements
 *
 * This method iterates each member vector and inserts their
 * elements into the ordered element list using bubble sort.
 * Sort criteria is the source line number.
 */
void CBENameSpace::CreateOrderedElementList(void)
{
    // clear vector
    m_vOrderedElements.erase(m_vOrderedElements.begin(), m_vOrderedElements.end());
    // namespaces
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNS;
    while ((pNS = GetNextNameSpace(iterN)) != NULL)
    {
        InsertOrderedElement(pNS);
    }
    // classes
    vector<CBEClass*>::iterator iterCl = GetFirstClass();
    CBEClass *pCl;
    while ((pCl = GetNextClass(iterCl)) != NULL)
    {
        InsertOrderedElement(pCl);
    }
    // typedef
    vector<CBETypedef*>::iterator iterT = GetFirstTypedef();
    CBETypedef *pT;
    while ((pT = GetNextTypedef(iterT)) != NULL)
    {
        InsertOrderedElement(pT);
    }
    // tagged types
    vector<CBEType*>::iterator iterTa = GetFirstTaggedType();
    CBEType* pTa;
    while ((pTa = GetNextTaggedType(iterTa)) != NULL)
    {
        InsertOrderedElement(pTa);
    }
    // consts
    vector<CBEConstant*>::iterator iterC = GetFirstConstant();
    CBEConstant *pC;
    while ((pC = GetNextConstant(iterC)) != NULL)
    {
        InsertOrderedElement(pC);
    }
}

/** \brief insert one element into the ordered list
 *  \param pObj the new element
 *
 * This is the insert implementation
 */
void CBENameSpace::InsertOrderedElement(CObject *pObj)
{
    // get source line number
    int nLine = pObj->GetSourceLine();
    // search for element with larger number
    vector<CObject*>::iterator iter = m_vOrderedElements.begin();
    for (; iter != m_vOrderedElements.end(); iter++)
    {
        if ((*iter)->GetSourceLine() > nLine)
        {
//             TRACE("Insert element from %s:%d before element from %s:%d\n",
//                 pObj->GetSourceFileName().c_str(), pObj->GetSourceLine(),
//                 (*iter)->GetSourceFileName().c_str(),
//                 (*iter)->GetSourceLine());
            // insert before that element
            m_vOrderedElements.insert(iter, pObj);
            return;
        }
    }
    // new object is bigger that all existing
//     TRACE("Insert element from %s:%d at end\n",
//         pObj->GetSourceFileName().c_str(), pObj->GetSourceLine());
    m_vOrderedElements.push_back(pObj);
}

/**
 *    \file    dice/src/be/BERoot.cpp
 *    \brief   contains the implementation of the class CBERoot
 *
 *    \date    01/10/2002
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

#include "be/BERoot.h"
#include "be/BEContext.h"
#include "be/BEClient.h"
#include "be/BEComponent.h"
#include "be/BETestsuite.h"
#include "be/BEClass.h"
#include "be/BEType.h"
#include "be/BEEnumType.h"
#include "be/BETypedef.h"
#include "be/BEFunction.h"
#include "be/BEConstant.h"
#include "be/BENameSpace.h"
#include "be/BEDeclarator.h"
#include "be/BETestMainFunction.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEConstructedType.h"

CBERoot::CBERoot()
{
    m_pClient = 0;
    m_pComponent = 0;
    m_pTestsuite = 0;
}

CBERoot::CBERoot(CBERoot & src)
: CBEObject(src)
{
    m_pClient = src.m_pClient;
    m_pComponent = src.m_pComponent;
    m_pTestsuite = src.m_pTestsuite;

    COPY_VECTOR(CBEConstant, m_vConstants, iterC);
    COPY_VECTOR(CBEClass, m_vClasses, iterCl);
    COPY_VECTOR(CBENameSpace, m_vNamespaces, iterN);
    COPY_VECTOR(CBETypedef, m_vTypedefs, iterT);
    COPY_VECTOR(CBEType, m_vTypeDeclarations, iterTy);
    COPY_VECTOR(CBEFunction, m_vGlobalFunctions, iterF);
}

/**    \brief destructor
 */
CBERoot::~CBERoot()
{
    if (m_pClient)
        delete m_pClient;
    if (m_pComponent)
        delete m_pComponent;
    if (m_pTestsuite)
        delete m_pTestsuite;

    DEL_VECTOR(m_vConstants);
    DEL_VECTOR(m_vClasses);
    DEL_VECTOR(m_vNamespaces);
    DEL_VECTOR(m_vTypedefs);
    DEL_VECTOR(m_vGlobalFunctions);
}

/**    \brief creates the back-end structure
 *    \param pFEFile a reference to the corresponding starting point
 *    \param pContext the context of the generated back-end
 *    \return true if generation was successful
 *
 * This implementation creates the corresponding client, component and testsuite parts. If these parts already exists the old
 * versions are deleted and replaced by the new ones.
 */
bool CBERoot::CreateBE(CFEFile * pFEFile, CBEContext * pContext)
{
    VERBOSE("CBERoot::CreateBE(file: %s) called\n",
        pFEFile->GetFileName().c_str());
    // clean up
    if (m_pClient)
    {
        delete m_pClient;
        m_pClient = 0;
    }
    if (m_pComponent)
    {
        delete m_pComponent;
        m_pComponent = 0;
    }
    if (m_pTestsuite)
    {
        delete m_pTestsuite;
        m_pTestsuite = 0;
    }
    // create the "normal" namespace-class-function hierarchy now, because
    // client, component and testsuite depend on its existence
    if (!CreateBackEnd(pFEFile, pContext))
    {
        return false;
    }
    // create new client
    if (pContext->IsOptionSet(PROGRAM_GENERATE_CLIENT))
    {
        m_pClient = pContext->GetClassFactory()->GetNewClient();
        m_pClient->SetParent(this);
        if (!m_pClient->CreateBackEnd(pFEFile, pContext))
        {
            delete m_pClient;
            m_pClient = 0;
            return false;
        }
    }
    // create new component
    if (pContext->IsOptionSet(PROGRAM_GENERATE_COMPONENT))
    {
        m_pComponent = pContext->GetClassFactory()->GetNewComponent();
        m_pComponent->SetParent(this);
        if (!m_pComponent->CreateBackEnd(pFEFile, pContext))
        {
            delete m_pComponent;
            m_pComponent = 0;
            return false;
        }
    }
    // create testsuite if option set
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
        m_pTestsuite = pContext->GetClassFactory()->GetNewTestsuite();
        m_pTestsuite->SetParent(this);
        if (!m_pTestsuite->CreateBackEnd(pFEFile, pContext))
        {
            delete m_pTestsuite;
            m_pTestsuite = 0;
            return false;
        }
    }
    return true;
}

/**    \brief generates the output files and code
 *    \param pContext the context of the code generation
 */
void CBERoot::Write(CBEContext * pContext)
{
    if (m_pClient)
        m_pClient->Write(pContext);
    if (m_pComponent)
        m_pComponent->Write(pContext);
    if (m_pTestsuite)
        m_pTestsuite->Write(pContext);
}

/**    \brief tries to find the typedef to the given type-name
 *    \param sTypeName the name of the type to find
 *    \return a reference to the found typedef or 0
 *
 * Since we have all the elements in the containes types, constants,
 * classes and namespaces, we will search for the typedef first in our
 * own typedefs and then in the classes and namespaces.
 */
CBETypedef *CBERoot::FindTypedef(string sTypeName)
{
    DTRACE("CBERoot::FindTypedef(%s) called\n", sTypeName.c_str());
    vector<CBETypedef*>::iterator iterT = GetFirstTypedef();
    CBETypedef *pTypedef;
    while ((pTypedef = GetNextTypedef(iterT)) != 0)
    {
        DTRACE("CBERoot::FindTypedef check top level typedef %s\n",
            pTypedef->GetDeclarator()->GetName().c_str());
        if (pTypedef->FindDeclarator(sTypeName))
            return pTypedef;
    }

    vector<CBEClass*>::iterator iterCl = GetFirstClass();
    CBEClass *pClass;
    CBETypedDeclarator *pTypedDecl;
    while ((pClass = GetNextClass(iterCl)) != 0)
    {
        DTRACE("CBERoot::FindTypedef checking class %s\n",
            pClass->GetName().c_str());
        if ((pTypedDecl = pClass->FindTypedef(sTypeName)) != 0)
            if (dynamic_cast<CBETypedef*>(pTypedDecl))
                return (CBETypedef*)pTypedDecl;
    }

    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        DTRACE("CBERoot::FindTypedef checking namespace %s\n",
            pNameSpace->GetName().c_str());
        if ((pTypedef = pNameSpace->FindTypedef(sTypeName)) != 0)
            return pTypedef;
    }

    return 0;
}

/**    \brief tries to find the function with the given name
 *    \param sFunctionName the name to search for
 *    \return a reference to the function or NUL if not found
 *
 * To find a function, we search our classes and namespaces
 */
CBEFunction *CBERoot::FindFunction(string sFunctionName)
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
            return pFunction;
    }

    return 0;
}

/** \brief searches for an class
 *  \param sClassName the name of the class to look for
 *  \return a reference to the found class (or 0)
 *
 * First we search out top-level classes. If we can't find anything we
 * ask the namespaces.
 */
CBEClass* CBERoot::FindClass(string sClassName)
{
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if (pClass->GetName() == sClassName)
            return pClass;
    }
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if ((pClass = pNameSpace->FindClass(sClassName)) != 0)
            return pClass;
    }
    return 0;
}

/** \brief adds a constant to the collection
 *  \param pConstant the constant to add
 */
void CBERoot::AddConstant(CBEConstant *pConstant)
{
    if (!pConstant)
        return;
    m_vConstants.push_back(pConstant);
    pConstant->SetParent(this);
}

/** \brief removes a constant from the collection
 *  \param pConstant the constant to remove
 */
void CBERoot::RemoveConstant(CBEConstant *pConstant)
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
vector<CBEConstant*>::iterator CBERoot::GetFirstConstant()
{
    return m_vConstants.begin();
}

/** \brief retrieves a reference to the next constant
 *  \param iter the pointer to the next constant
 *  \return a reference to the next constant or 0 if no mor constants
 */
CBEConstant* CBERoot::GetNextConstant(vector<CBEConstant*>::iterator &iter)
{
    if (iter == m_vConstants.end())
        return 0;
    return *iter++;
}

/** \brief tries to find a constant by its name
 *  \param sConstantName the name to look for
 *  \return a reference to the constant with this name or 0 if not found
 *
 * First we search our own constants, and because all constants are in there, this should
 * be sufficient.
 */
CBEConstant* CBERoot::FindConstant(string sConstantName)
{
    vector<CBEConstant*>::iterator iterC = GetFirstConstant();
    CBEConstant *pConstant;
    while ((pConstant = GetNextConstant(iterC)) != 0)
    {
        if (pConstant->GetName() == sConstantName)
            return pConstant;
    }
    // search interfaces
    vector<CBEClass*>::iterator iterCl = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterCl)) != 0)
    {
        if ((pConstant = pClass->FindConstant(sConstantName)) != 0)
            return pConstant;
    }
    // search libraries
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

/** \brief adds a typedef to the back-end
 *  \param pTypedef the typedef to add
 */
void CBERoot::AddTypedef(CBETypedef *pTypedef)
{
    if (!pTypedef)
        return;
    m_vTypedefs.push_back(pTypedef);
    pTypedef->SetParent(this);
}

/** \brief removes a typedef from the back-end
 *  \param pTypedef the typedef to remove
 */
void CBERoot::RemoveTypedef(CBETypedef *pTypedef)
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
 * \return a pointer to the first typedef
 */
vector<CBETypedef*>::iterator CBERoot::GetFirstTypedef()
{
    return m_vTypedefs.begin();
}

/** \brief retrieves a reference to the next typedef
 *  \param iter the pointer to the next typedef
 *  \return a reference to the next typedef or 0 if none found
 */
CBETypedef* CBERoot::GetNextTypedef(vector<CBETypedef*>::iterator &iter)
{
    if (iter == m_vTypedefs.end())
        return 0;
    return *iter++;
}

/** \brief adds an Class to the back-end
 *  \param pClass the Class to add
 */
void CBERoot::AddClass(CBEClass *pClass)
{
    if (!pClass)
        return;
    m_vClasses.push_back(pClass);
    pClass->SetParent(this);
}

/** \brief removes an Class from the back-end
 *  \param pClass the Class to remove
 */
void CBERoot::RemoveClass(CBEClass *pClass)
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
vector<CBEClass*>::iterator CBERoot::GetFirstClass()
{
    return m_vClasses.begin();
}

/** \brief retrieves a reference to the next Class
 *  \param iter the pointer to the next Class
 *  \return a reference to the next Class
 */
CBEClass* CBERoot::GetNextClass(vector<CBEClass*>::iterator &iter)
{
    if (iter == m_vClasses.end())
        return 0;
    return *iter++;
}

/** \brief adds a namespace to the back-end
 *  \param pNameSpace the namespace to add
 */
void CBERoot::AddNameSpace(CBENameSpace *pNameSpace)
{
    if (!pNameSpace)
        return;
    m_vNamespaces.push_back(pNameSpace);
    pNameSpace->SetParent(this);
}

/** \brief remove a namespace from the back-end
 *  \param pNameSpace the namespace to remove
 */
void CBERoot::RemoveNameSpace(CBENameSpace *pNameSpace)
{
    if (!pNameSpace)
        return;
    vector<CBENameSpace*>::iterator iter;
    for (iter = m_vNamespaces.begin(); iter != m_vNamespaces.end(); iter++)
    {
        if (*iter == pNameSpace)
        {
            m_vNamespaces.erase(iter);
            return;
        }
    }
}

/** \brief retrieves a pointer to the first namespace
 *  \return the pointer to the first namespace
 */
vector<CBENameSpace*>::iterator CBERoot::GetFirstNameSpace()
{
    return m_vNamespaces.begin();
}

/** \brief returns a reference to the next namespace
 *  \param iter the pointer to the next namespace
 *  \return a reference to the next namespace
 */
CBENameSpace* CBERoot::GetNextNameSpace(vector<CBENameSpace*>::iterator &iter)
{
    if (iter == m_vNamespaces.end())
        return 0;
    return *iter++;
}

/** \brief searches for a namespace with the given name
 *  \param sNameSpaceName the name of the namespace
 *  \return a reference to the namespace or 0 if not found
 */
CBENameSpace* CBERoot::FindNameSpace(string sNameSpaceName)
{
    vector<CBENameSpace*>::iterator iter = GetFirstNameSpace();
    CBENameSpace *pNameSpace, *pFoundNameSpace;
    while ((pNameSpace = GetNextNameSpace(iter)) != 0)
    {
        // check the namespace itself
        if (pNameSpace->GetName() == sNameSpaceName)
            return pNameSpace;
        // check nested namespaces
        if ((pFoundNameSpace = pNameSpace->FindNameSpace(sNameSpaceName)) != 0)
            return pFoundNameSpace;
    }
    return 0;
}

/** \brief creates the constants of a file
 *  \param pFEFile the front-end file to search for constants
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBERoot::CreateBackEnd(CFEFile *pFEFile, CBEContext *pContext)
{
    VERBOSE("%s for %s called\n", __PRETTY_FUNCTION__,
        pFEFile->GetFileName().c_str());
    // first search included files-> may contain base interfaces we need later
    vector<CFEFile*>::iterator iterF = pFEFile->GetFirstChildFile();
    CFEFile *pFEIncludedFile;
    while ((pFEIncludedFile = pFEFile->GetNextChildFile(iterF)) != 0)
    {
        if (!CreateBackEnd(pFEIncludedFile, pContext))
            return false;
    }
    // next search top level consts
    vector<CFEConstDeclarator*>::iterator iterC = pFEFile->GetFirstConstant();
    CFEConstDeclarator *pFEConst;
    while ((pFEConst = pFEFile->GetNextConstant(iterC)) != 0)
    {
        if (!CreateBackEnd(pFEConst, pContext))
            return false;
    }
    // next search top level typedefs
    vector<CFETypedDeclarator*>::iterator iterT = pFEFile->GetFirstTypedef();
    CFETypedDeclarator *pFETypedef;
    while ((pFETypedef = pFEFile->GetNextTypedef(iterT)) != 0)
    {
        if (!CreateBackEnd(pFETypedef, pContext))
            return false;
    }
    // next search top level type declarations
    vector<CFEConstructedType*>::iterator iterTD = pFEFile->GetFirstTaggedDecl();
    CFEConstructedType *pFETaggedType;
    while ((pFETaggedType = pFEFile->GetNextTaggedDecl(iterTD)) != 0)
    {
        if (!CreateBackEnd(pFETaggedType, pContext))
            return false;
    }
    // next search top level interfaces
    vector<CFEInterface*>::iterator iterI = pFEFile->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFEFile->GetNextInterface(iterI)) != 0)
    {
        if (!CreateBackEnd(pFEInterface, pContext))
            return false;
    }
    // next search libraries
    vector<CFELibrary*>::iterator iterL = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(iterL)) != 0)
    {
        if (!CreateBackEnd(pFELibrary, pContext))
            return false;
    }
    // if testsuite we need main function
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
        // this creates only one main function -> for the top IDL file
        if (pFEFile->IsIDLFile() && !(pFEFile->GetParent()))
        {
	    CBEClassFactory *pCF = pContext->GetClassFactory();
            CBETestMainFunction *pMain = pCF->GetNewTestMainFunction();
            AddGlobalFunction(pMain);
            if (!pMain->CreateBackEnd(pFEFile, pContext))
            {
                RemoveGlobalFunction(pMain);
                delete pMain;
                VERBOSE("%s failed because main could not be created\n",
                    __PRETTY_FUNCTION__);
                return false;
            }
	    // set line number of main
	    pMain->SetSourceLine(pFEFile->GetSourceLineEnd());
        }
    }
    return true;
}

/** \brief creates the constants for a specific library
 *  \param pFELibrary the front-end library to search for constants
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBERoot::CreateBackEnd(CFELibrary *pFELibrary, CBEContext *pContext)
{
    VERBOSE("%s for %s called\n", __PRETTY_FUNCTION__,
        pFELibrary->GetName().c_str());
    // first check if NameSpace is already there
    CBENameSpace *pNameSpace = FindNameSpace(pFELibrary->GetName());
    if (!pNameSpace)
    {
        // create NameSpace itself
        pNameSpace = pContext->GetClassFactory()->GetNewNameSpace();
        AddNameSpace(pNameSpace);
        if (!pNameSpace->CreateBackEnd(pFELibrary, pContext))
        {
            RemoveNameSpace(pNameSpace);
            delete pNameSpace;
            VERBOSE("%s failed because namespace %s could not be created\n",
                     __PRETTY_FUNCTION__, pFELibrary->GetName().c_str());
            return false;
        }
    }
    else
    {
        // call create function again to create the new Classs and such
        if (!pNameSpace->CreateBackEnd(pFELibrary, pContext))
        {
            RemoveNameSpace(pNameSpace);
            VERBOSE("%s failed because namespace %s could not be re-created\n",
                    __PRETTY_FUNCTION__, pFELibrary->GetName().c_str());
            return false;
        }
    }
    return true;
}

/** \brief creates the back end for an interface
 *  \param  pFEInterface the interface to search for classes
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBERoot::CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext)
{
    VERBOSE("%s for %s called\n", __PRETTY_FUNCTION__,
        pFEInterface->GetName().c_str());
    CBEClass *pClass = pContext->GetClassFactory()->GetNewClass();
    AddClass(pClass);
    if (!pClass->CreateBackEnd(pFEInterface, pContext))
    {
        RemoveClass(pClass);
        VERBOSE("%s failed because class could not be created\n",
            __PRETTY_FUNCTION__);
        delete pClass;
        return false;
    }
    return true;
}

/** \brief creates a back-end const for the front-end const
 *  \param pFEConstant the constant to use as reference
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CBERoot::CreateBackEnd(CFEConstDeclarator *pFEConstant, CBEContext *pContext)
{
    VERBOSE("%s for %s called\n", __PRETTY_FUNCTION__,
        pFEConstant->GetName().c_str());
    CBEConstant *pConstant = pContext->GetClassFactory()->GetNewConstant();
    AddConstant(pConstant);
    if (!pConstant->CreateBackEnd(pFEConstant, pContext))
    {
        RemoveConstant(pConstant);
        VERBOSE("CBERoot::CreateBEConstants failed because BE constant couldn't be created\n");
        delete pConstant;
        return false;
    }

    return true;
}

/** \brief creates then back-end representation of an type definition
 *  \param pFETypedef the front-end type definition
 *  \param pContext the context of the type definition
 *  \return true if successful
 */
bool CBERoot::CreateBackEnd(CFETypedDeclarator *pFETypedef, CBEContext *pContext)
{
    VERBOSE("%s called\n", __PRETTY_FUNCTION__);
    CBETypedef *pTypedef = pContext->GetClassFactory()->GetNewTypedef();
    AddTypedef(pTypedef);
    if (!pTypedef->CreateBackEnd(pFETypedef, pContext))
    {
        RemoveTypedef(pTypedef);
        VERBOSE("%s failed because type could not be created\n",
            __PRETTY_FUNCTION__);
        delete pTypedef;
        return false;
    }

    return true;
}

/** \brief adds the members of the root to the header file
 *  \param pHeader the header file
 *  \param pContext the context of this operation
 *  \return true if successful
 *
 * The root adds to the header files everything it own. It iterates over its members
 * and calls their respective AddToFile functions.
 */
bool CBERoot::AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext)
{
    VERBOSE("CBERoot::AddToFile(header: %s) called\n",
        pHeader->GetFileName().c_str());
    // constants
    vector<CBEConstant*>::iterator iterC = GetFirstConstant();
    CBEConstant *pConstant;
    while ((pConstant = GetNextConstant(iterC)) != 0)
    {
        if (!pConstant->AddToFile(pHeader, pContext))
            return false;
    }
    // types
    vector<CBETypedef*>::iterator iterT = GetFirstTypedef();
    CBETypedef *pTypedef;
    while ((pTypedef = GetNextTypedef(iterT)) != 0)
    {
        if (!pTypedef->AddToFile(pHeader, pContext))
            return false;
    }
    // tagged declarations
    vector<CBEType*>::iterator iterTa = GetFirstTaggedType();
    CBEType *pTaggedType;
    while ((pTaggedType = GetNextTaggedType(iterTa)) != 0)
    {
        if (!pTaggedType->AddToFile(pHeader, pContext))
            return false;
    }
    // Classs
    vector<CBEClass*>::iterator iterCl = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterCl)) != 0)
    {
        if (!pClass->AddToFile(pHeader, pContext))
            return false;
    }
    // libraries
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if (!pNameSpace->AddToFile(pHeader, pContext))
            return false;
    }
    return true;
}

/** \brief adds the members of the root to the implementation file
 *  \param pImpl the implementation file
 *  \param pContext the context of the operation
 *  \return true if successful
 *
 * The root adds to the implementation file only the members of the Classs
 * and libraries.
 */
bool CBERoot::AddToFile(CBEImplementationFile *pImpl, CBEContext *pContext)
{
    VERBOSE("CBERoot::AddToFile(impl: %s) called\n",
        pImpl->GetFileName().c_str());
    // Classs
    vector<CBEClass*>::iterator iterC = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(iterC)) != 0)
    {
        if (!pClass->AddToFile(pImpl, pContext))
            return false;
    }
    // name-spaces
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if (!pNameSpace->AddToFile(pImpl, pContext))
            return false;
    }
    // global functions
    vector<CBEFunction*>::iterator iterF = GetFirstGlobalFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextGlobalFunction(iterF)) != 0)
    {
        if (!pFunction->AddToFile(pImpl, pContext))
            return false;
    }
    return true;
}

/** \brief adds the opcodes of a file to the header files
 *  \param pHeader the header file to add the opcodes to
 *  \param pFEFile the respective front-end file to use as reference
 *  \param pContext the context of this operation
 *  \return true if successful
 *
 * Root adds opcodes by calling its classes and namespaces. Because it should only
 * add the opcodes of the current IDL file, it is used as reference to find these
 * classes and namespaces
 */
bool CBERoot::AddOpcodesToFile(CBEHeaderFile *pHeader, CFEFile *pFEFile, CBEContext *pContext)
{
    VERBOSE("CBERoot::AddOpcodesToFile(header: %s, file: %s) called\n",
        pHeader->GetFileName().c_str(), pFEFile->GetFileName().c_str());
    assert(pHeader);
    assert(pFEFile);
    // if FILE_ALL the included files have to be regarded as well
    // and because they may contain base interfaces, they come first
    if (pContext->IsOptionSet(PROGRAM_FILE_ALL))
    {
        vector<CFEFile*>::iterator iterF = pFEFile->GetFirstChildFile();
        CFEFile *pIncFile;
        while ((pIncFile = pFEFile->GetNextChildFile(iterF)) != 0)
        {
            if (!AddOpcodesToFile(pHeader, pIncFile, pContext))
                return false;
        }
    }
    // classes
    vector<CFEInterface*>::iterator iterI = pFEFile->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFEFile->GetNextInterface(iterI)) != 0)
    {
        CBEClass *pClass = FindClass(pFEInterface->GetName());
        if (!pClass)
        {
            VERBOSE("CBERoot::AddOpcodesToFile failed because class %s could not be found\n",
                    pFEInterface->GetName().c_str());
            return false;
        }
        if (!pClass->AddOpcodesToFile(pHeader, pContext))
            return false;
    }
    // namespaces
    vector<CFELibrary*>::iterator iterL = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(iterL)) != 0)
    {
        CBENameSpace *pNameSpace = FindNameSpace(pFELibrary->GetName());
        if (!pNameSpace)
        {
            VERBOSE("CBERoot::AddOpcodesToFile failed because namespace %s could not be found\n",
                    pFELibrary->GetName().c_str());
            return false;
        }
        if (!pNameSpace->AddOpcodesToFile(pHeader, pContext))
            return false;
    }
    return true;
}

/** \brief adds a global function
 *  \param pFunction the function to add
 */
void CBERoot::AddGlobalFunction(CBEFunction *pFunction)
{
    if (!pFunction)
        return;
    m_vGlobalFunctions.push_back(pFunction);
    pFunction->SetParent(this);
}

/** \brief removes a global function
 *  \param pFunction the function to remove
 */
void CBERoot::RemoveGlobalFunction(CBEFunction *pFunction)
{
    if (!pFunction)
        return;
    vector<CBEFunction*>::iterator iter;
    for (iter = m_vGlobalFunctions.begin(); iter != m_vGlobalFunctions.end(); iter++)
    {
        if (*iter == pFunction)
        {
            m_vGlobalFunctions.erase(iter);
            return;
        }
    }
}

/** \brief returns pointer to first global function
 *  \return a pointer to the first global function
 */
vector<CBEFunction*>::iterator CBERoot::GetFirstGlobalFunction()
{
    return m_vGlobalFunctions.begin();
}

/** \brief returns reference to next global function
 *  \param iter the pointer to the next global function
 *  \return a reference to the next global function
 */
CBEFunction* CBERoot::GetNextGlobalFunction(vector<CBEFunction*>::iterator &iter)
{
    if (iter == m_vGlobalFunctions.end())
        return 0;
    return *iter++;
}

/** \brief tries to find a global function
 *  \param sFuncName the name of the function
 *  \return a reference to the found function or 0
 */
CBEFunction* CBERoot::FindGlobalFunction(string sFuncName)
{
    vector<CBEFunction*>::iterator iter = GetFirstGlobalFunction();
    CBEFunction *pFunc;
    while ((pFunc = GetNextGlobalFunction(iter)) != 0)
    {
        if (pFunc->GetName() == sFuncName)
            return pFunc;
    }
    return 0;
}

/** \brief prints the generated target files to the given output
 *  \param output the output stream to write to
 *  \param nCurCol the current column where to start to print (indent)
 *  \param nMaxCol the maximum number of columns
 */
void CBERoot::PrintTargetFiles(FILE *output, int &nCurCol, int nMaxCol)
{
    if (m_pClient)
    {
        m_pClient->PrintTargetFiles(output, nCurCol, nMaxCol);
    }
    if (m_pComponent)
    {
        m_pComponent->PrintTargetFiles(output, nCurCol, nMaxCol);
    }
    if (m_pTestsuite)
    {
        m_pTestsuite->PrintTargetFiles(output, nCurCol, nMaxCol);
    }
}

/** \brief searches for a type with the given tag
 *  \param nType the type (struct/union/enum) of the searched type
 *  \param sTag the tag of the type
 *  \return a reference to the found type
 */
CBEType* CBERoot::FindTaggedType(int nType, string sTag)
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
    // search namespaces
    vector<CBENameSpace*>::iterator iterN = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(iterN)) != 0)
    {
        if ((pType = pNameSpace->FindTaggedType(nType, sTag)) != 0)
            return pType;
    }
    return 0;
}

/** \brief adds a tagged type declaration to the root
 *  \param pType the type to add
 */
void CBERoot::AddTaggedType(CBEType *pType)
{
    if (!pType)
        return;
    m_vTypeDeclarations.push_back(pType);
}

/** \brief removes a tagged type from the root's collection
 *  \param pType the type to remove
 */
void CBERoot::RemoveTaggedType(CBEType *pType)
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

/** \brief retrieves a reference to the first tagged type
 *  \return a pointer to the first tagged type declaration
 */
vector<CBEType*>::iterator CBERoot::GetFirstTaggedType()
{
    return m_vTypeDeclarations.begin();
}

/** \brief retrieve a reference to the next type declaration
 *  \param iter the pointer to the next tagged type declaration
 *  \return a reference to the next tagged type declaration
 */
CBEType* CBERoot::GetNextTaggedType(vector<CBEType*>::iterator &iter)
{
    if (iter == m_vTypeDeclarations.end())
        return 0;
    return *iter++;
}

/** \brief creates and stores a new tagged type declaration
 *  \param pFEType the respective front-end type
 *  \param pContext the context of the create process
 *  \return true if successful
 */
bool CBERoot::CreateBackEnd(CFEConstructedType *pFEType, CBEContext *pContext)
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

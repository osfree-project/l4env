/**
 *	\file	dice/src/be/BERoot.cpp
 *	\brief	contains the implementation of the class CBERoot
 *
 *	\date	01/10/2002
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

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEConstructedType.h"

IMPLEMENT_DYNAMIC(CBERoot);

CBERoot::CBERoot()
: m_vConstants(RUNTIME_CLASS(CBEConstant)),
  m_vTypedefs(RUNTIME_CLASS(CBETypedef)),
  m_vTypeDeclarations(RUNTIME_CLASS(CBEType)),
  m_vClasses(RUNTIME_CLASS(CBEClass)),
  m_vNamespaces(RUNTIME_CLASS(CBENameSpace)),
  m_vGlobalFunctions(RUNTIME_CLASS(CBEFunction))
{
    m_pClient = 0;
    m_pComponent = 0;
    m_pTestsuite = 0;
    IMPLEMENT_DYNAMIC_BASE(CBERoot, CBEObject);
}

CBERoot::CBERoot(CBERoot & src)
: CBEObject(src),
  m_vConstants(RUNTIME_CLASS(CBEConstant)),
  m_vTypedefs(RUNTIME_CLASS(CBETypedef)),
  m_vTypeDeclarations(RUNTIME_CLASS(CBEType)),
  m_vClasses(RUNTIME_CLASS(CBEClass)),
  m_vNamespaces(RUNTIME_CLASS(CBENameSpace)),
  m_vGlobalFunctions(RUNTIME_CLASS(CBEFunction))
{
    m_pClient = src.m_pClient;
    m_pComponent = src.m_pComponent;
    m_pTestsuite = src.m_pTestsuite;
    m_vConstants.Add(&src.m_vConstants);
    m_vClasses.Add(&src.m_vClasses);
    m_vNamespaces.Add(&src.m_vNamespaces);
    m_vTypedefs.Add(&src.m_vTypedefs);
    m_vTypeDeclarations.Add(&src.m_vTypeDeclarations);
    m_vGlobalFunctions.Add(&src.m_vGlobalFunctions);
    IMPLEMENT_DYNAMIC_BASE(CBERoot, CBEObject);
}

/**	\brief destructor
 */
CBERoot::~CBERoot()
{
    if (m_pClient)
        delete m_pClient;
    if (m_pComponent)
        delete m_pComponent;
    if (m_pTestsuite)
        delete m_pTestsuite;
    m_vConstants.DeleteAll();
    m_vClasses.DeleteAll();
    m_vNamespaces.DeleteAll();
    m_vTypedefs.DeleteAll();
    m_vGlobalFunctions.DeleteAll();
}

/**	\brief creates the back-end structure
 *	\param pFEFile a reference to the corresponding starting point
 *	\param pContext the context of the generated back-end
 *	\return true if generation was successful
 *
 * This implementation creates the corresponding client, component and testsuite parts. If these parts already exists the old
 * versions are deleted and replaced by the new ones.
 */
bool CBERoot::CreateBE(CFEFile * pFEFile, CBEContext * pContext)
{
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

/**	\brief optimizes the existing back-end
 *	\param nLevel the level of optimization
 *  \param pContext the context of the optimization
 *	\return success or failure code
 *
 * Currently you can only optimize on a function level. Thus this implementation initiates
 * the optimization process by calling the Optimize function of its members.
 */
int CBERoot::Optimize(int nLevel, CBEContext *pContext)
{
    int nRet = 0;
    if (m_pClient)
	if ((nRet = m_pClient->Optimize(nLevel, pContext)) != 0)
	    return nRet;
    if (m_pComponent)
	if ((nRet = m_pComponent->Optimize(nLevel, pContext)) != 0)
	    return nRet;
    if (m_pTestsuite)
	if ((nRet = m_pTestsuite->Optimize(nLevel, pContext)) != 0)
	    return nRet;
    return 0;
}

/**	\brief generates the output files and code
 *	\param pContext the context of the code generation
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

/**	\brief tries to find the typedef to the given type-name
 *	\param sTypeName the name of the type to find
 *	\return a reference to the found typedef or 0
 *
 * Since we have all the elements in the containes types, constants,
 * classes and namespaces, we will search for the typedef first in our
 * own typedefs and then in the classes and namespaces.
 */
CBETypedef *CBERoot::FindTypedef(String sTypeName)
{
    VectorElement *pIter = GetFirstTypedef();
    CBETypedef *pTypedef;
    while ((pTypedef = GetNextTypedef(pIter)) != 0)
    {
        if (pTypedef->FindDeclarator(sTypeName))
            return pTypedef;
    }

    pIter = GetFirstClass();
    CBEClass *pClass;
    CBETypedDeclarator *pTypedDecl;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if ((pTypedDecl = pClass->FindTypedef(sTypeName)) != 0)
            if (pTypedDecl->IsKindOf(RUNTIME_CLASS(CBETypedef)))
                return (CBETypedef*)pTypedDecl;
    }

    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if ((pTypedef = pNameSpace->FindTypedef(sTypeName)) != 0)
            return pTypedef;
    }

    return 0;
}

/**	\brief tries to find the function with the given name
 *	\param sFunctionName the name to search for
 *	\return a reference to the function or NUL if not found
 *
 * To find a function, we search our classes and namespaces
 */
CBEFunction *CBERoot::FindFunction(String sFunctionName)
{
    CBEFunction *pFunction;
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if ((pFunction = pClass->FindFunction(sFunctionName)) != 0)
            return pFunction;
    }

    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
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
CBEClass* CBERoot::FindClass(String sClassName)
{
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if (pClass->GetName() == sClassName)
            return pClass;
    }
    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
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
    m_vConstants.Add(pConstant);
    pConstant->SetParent(this);
}

/** \brief removes a constant from the collection
 *  \param pConstant the constant to remove
 */
void CBERoot::RemoveConstant(CBEConstant *pConstant)
{
    m_vConstants.Remove(pConstant);
}

/** \brief retrieves a pointer to the first constant
 *  \return a pointer to the first constant
 */
VectorElement* CBERoot::GetFirstConstant()
{
    return m_vConstants.GetFirst();
}

/** \brief retrieves a reference to the next constant
 *  \param pIter the pointer to the next constant
 *  \return a reference to the next constant or 0 if no mor constants
 */
CBEConstant* CBERoot::GetNextConstant(VectorElement* &pIter)
{
    if (!pIter)
        return 0;
    CBEConstant *pRet = (CBEConstant*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextConstant(pIter);
    return pRet;
}

/** \brief tries to find a constant by its name
 *  \param sConstantName the name to look for
 *  \return a reference to the constant with this name or 0 if not found
 *
 * First we search our own constants, and because all constants are in there, this should
 * be sufficient.
 */
CBEConstant* CBERoot::FindConstant(String sConstantName)
{
    VectorElement *pIter = GetFirstConstant();
    CBEConstant *pConstant;
    while ((pConstant = GetNextConstant(pIter)) != 0)
    {
        if (pConstant->GetName() == sConstantName)
            return pConstant;
    }
    return 0;
}

/** \brief adds a typedef to the back-end
 *  \param pTypedef the typedef to add
 */
void CBERoot::AddTypedef(CBETypedef *pTypedef)
{
    m_vTypedefs.Add(pTypedef);
    pTypedef->SetParent(this);
}

/** \brief removes a typedef from the back-end
 *  \param pTypedef the typedef to remove
 */
void CBERoot::RemoveTypedef(CBETypedef *pTypedef)
{
    m_vTypedefs.Remove(pTypedef);
}

/** \brief retrieves a pointer to the first typedef
 * \return a pointer to the first typedef
 */
VectorElement* CBERoot::GetFirstTypedef()
{
    return m_vTypedefs.GetFirst();
}

/** \brief retrieves a reference to the next typedef
 *  \param pIter the pointer to the next typedef
 *  \return a reference to the next typedef or 0 if none found
 */
CBETypedef* CBERoot::GetNextTypedef(VectorElement* &pIter)
{
    if (!pIter)
        return 0;
    CBETypedef *pTypedef = (CBETypedef*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pTypedef)
        return GetNextTypedef(pIter);
    return pTypedef;
}

/** \brief adds an Class to the back-end
 *  \param pClass the Class to add
 */
void CBERoot::AddClass(CBEClass *pClass)
{
    m_vClasses.Add(pClass);
    pClass->SetParent(this);
}

/** \brief removes an Class from the back-end
 *  \param pClass the Class to remove
 */
void CBERoot::RemoveClass(CBEClass *pClass)
{
    m_vClasses.Remove(pClass);
}

/** \brief retrieves a pointer to the first Class
 *  \return a pointer to the first Class
 */
VectorElement* CBERoot::GetFirstClass()
{
    return m_vClasses.GetFirst();
}

/** \brief retrieves a reference to the next Class
 *  \param pIter the pointer to the next Class
 *  \return a reference to the next Class
 */
CBEClass* CBERoot::GetNextClass(VectorElement* &pIter)
{
    if (!pIter)
        return 0;
    CBEClass *pRet = (CBEClass*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextClass(pIter);
    return pRet;
}

/** \brief adds a namespace to the back-end
 *  \param pNameSpace the namespace to add
 */
void CBERoot::AddNameSpace(CBENameSpace *pNameSpace)
{
    m_vNamespaces.Add(pNameSpace);
    pNameSpace->SetParent(this);
}

/** \brief remove a namespace from the back-end
 *  \param pNameSpace the namespace to remove
 */
void CBERoot::RemoveNameSpace(CBENameSpace *pNameSpace)
{
    m_vNamespaces.Remove(pNameSpace);
}

/** \brief retrieves a pointer to the first namespace
 *  \return the pointer to the first namespace
 */
VectorElement* CBERoot::GetFirstNameSpace()
{
    return m_vNamespaces.GetFirst();
}

/** \brief returns a reference to the next namespace
 *  \param pIter the pointer to the next namespace
 *  \return a reference to the next namespace
 */
CBENameSpace* CBERoot::GetNextNameSpace(VectorElement*&pIter)
{
    if (!pIter)
        return 0;
    CBENameSpace *pRet = (CBENameSpace*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextNameSpace(pIter);
    return pRet;
}

/** \brief searches for a namespace with the given name
 *  \param sNameSpaceName the name of the namespace
 *  \return a reference to the namespace or 0 if not found
 */
CBENameSpace* CBERoot::FindNameSpace(String sNameSpaceName)
{
    VectorElement *pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace, *pFoundNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
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
    // first search included files-> may contain base interfaces we need later
    VectorElement *pIter = pFEFile->GetFirstIncludeFile();
    CFEFile *pFEIncludedFile;
    while ((pFEIncludedFile = pFEFile->GetNextIncludeFile(pIter)) != 0)
    {
        if (!CreateBackEnd(pFEIncludedFile, pContext))
            return false;
    }
    // next search top level consts
    pIter = pFEFile->GetFirstConstant();
    CFEConstDeclarator *pFEConst;
    while ((pFEConst = pFEFile->GetNextConstant(pIter)) != 0)
    {
        if (!CreateBackEnd(pFEConst, pContext))
            return false;
    }
    // next search top level typedefs
    pIter = pFEFile->GetFirstTypedef();
    CFETypedDeclarator *pFETypedef;
    while ((pFETypedef = pFEFile->GetNextTypedef(pIter)) != 0)
    {
        if (!CreateBackEnd(pFETypedef, pContext))
            return false;
    }
    // next search top level type declarations
    pIter = pFEFile->GetFirstTaggedDecl();
    CFEConstructedType *pFETaggedType;
    while ((pFETaggedType = pFEFile->GetNextTaggedDecl(pIter)) != 0)
    {
        if (!CreateBackEnd(pFETaggedType, pContext))
            return false;
    }
    // next search top level interfaces
    pIter = pFEFile->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFEFile->GetNextInterface(pIter)) != 0)
    {
        if (!CreateBackEnd(pFEInterface, pContext))
            return false;
    }
    // next search libraries
    pIter = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(pIter)) != 0)
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
            CBETestMainFunction *pMain = pContext->GetClassFactory()->GetNewTestMainFunction();
            AddGlobalFunction(pMain);
            if (!pMain->CreateBackEnd(pFEFile, pContext))
            {
                RemoveGlobalFunction(pMain);
                delete pMain;
                VERBOSE("CBERoot::CreateBackEnd failed because main function could not be created\n");
                return false;
            }
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
            VERBOSE("CBERoot::CreateBackEnd failed because namespace %s could not be created\n",
                     (const char*)pFELibrary->GetName());
            return false;
        }
    }
    else
    {
        // call create function again to create the new Classs and such
        if (!pNameSpace->CreateBackEnd(pFELibrary, pContext))
        {
            RemoveNameSpace(pNameSpace);
            VERBOSE("CBERoot::CreateBackEnd failed because namespace could not be re-created\n",
                    (const char*)pFELibrary->GetName());
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
    CBEClass *pClass = pContext->GetClassFactory()->GetNewClass();
    AddClass(pClass);
    if (!pClass->CreateBackEnd(pFEInterface, pContext))
    {
        RemoveClass(pClass);
        VERBOSE("CBERoot::CreateBackEnd failed because class could not be created\n");
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
    CBETypedef *pTypedef = pContext->GetClassFactory()->GetNewTypedef();
    AddTypedef(pTypedef);
    if (!pTypedef->CreateBackEnd(pFETypedef, pContext))
    {
        RemoveTypedef(pTypedef);
        VERBOSE("CBERoot::CreateBackEnd failed because type could not be created\n");
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
    // constants
    VectorElement *pIter = GetFirstConstant();
    CBEConstant *pConstant;
    while ((pConstant = GetNextConstant(pIter)) != 0)
    {
        if (!pConstant->AddToFile(pHeader, pContext))
            return false;
    }
    // types
    pIter = GetFirstTypedef();
    CBETypedef *pTypedef;
    while ((pTypedef = GetNextTypedef(pIter)) != 0)
    {
        if (!pTypedef->AddToFile(pHeader, pContext))
            return false;
    }
    // Classs
    pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if (!pClass->AddToFile(pHeader, pContext))
            return false;
    }
    // libraries
    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
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
    // Classs
    VectorElement *pIter = GetFirstClass();
    CBEClass *pClass;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if (!pClass->AddToFile(pImpl, pContext))
            return false;
    }
    // name-spaces
    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
    {
        if (!pNameSpace->AddToFile(pImpl, pContext))
            return false;
    }
    // global functions
    pIter = GetFirstGlobalFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextGlobalFunction(pIter)) != 0)
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
    // if FILE_ALL the included files have to be regarded as well
    // and because they may contain base interfaces, they come first
    if (pContext->IsOptionSet(PROGRAM_FILE_ALL))
    {
        VectorElement *pIter = pFEFile->GetFirstIncludeFile();
        CFEFile *pIncFile;
        while ((pIncFile = pFEFile->GetNextIncludeFile(pIter)) != 0)
        {
            if (!AddOpcodesToFile(pHeader, pIncFile, pContext))
                return false;
        }
    }
    // get root
    CBERoot *pRoot = GetRoot();
    ASSERT(pRoot);
    // classes
    VectorElement *pIter = pFEFile->GetFirstInterface();
    CFEInterface *pFEInterface;
    while ((pFEInterface = pFEFile->GetNextInterface(pIter)) != 0)
    {
        CBEClass *pClass = pRoot->FindClass(pFEInterface->GetName());
        if (!pClass)
        {
            VERBOSE("CBERoot::AddOpcodesToFile failed because class %s could not be found\n",
                    (const char*)pFEInterface->GetName());
            return false;
        }
        if (!pClass->AddOpcodesToFile(pHeader, pContext))
            return false;
    }
    // namespaces
    pIter = pFEFile->GetFirstLibrary();
    CFELibrary *pFELibrary;
    while ((pFELibrary = pFEFile->GetNextLibrary(pIter)) != 0)
    {
        CBENameSpace *pNameSpace = pRoot->FindNameSpace(pFELibrary->GetName());
        if (!pNameSpace)
        {
            VERBOSE("CBERoot::AddOpcodesToFile failed because namespace %s could not be found\n",
                    (const char*)pFELibrary->GetName());
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
    m_vGlobalFunctions.Add(pFunction);
    pFunction->SetParent(this);
}

/** \brief removes a global function
 *  \param pFunction the function to remove
 */
void CBERoot::RemoveGlobalFunction(CBEFunction *pFunction)
{
    m_vGlobalFunctions.Remove(pFunction);
}

/** \brief returns pointer to first global function
 *  \return a pointer to the first global function
 */
VectorElement* CBERoot::GetFirstGlobalFunction()
{
    return m_vGlobalFunctions.GetFirst();
}

/** \brief returns reference to next global function
 *  \param pIter the pointer to the next global function
 *  \return a reference to the next global function
 */
CBEFunction* CBERoot::GetNextGlobalFunction(VectorElement*&pIter)
{
    if (!pIter)
        return 0;
    CBEFunction *pRet = (CBEFunction*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextGlobalFunction(pIter);
    return pRet;
}

/** \brief tries to find a global function
 *  \param sFuncName the name of the function
 *  \return a reference to the found function or 0
 */
CBEFunction* CBERoot::FindGlobalFunction(String sFuncName)
{
    VectorElement *pIter = GetFirstGlobalFunction();
    CBEFunction *pFunc;
    while ((pFunc = GetNextGlobalFunction(pIter)) != 0)
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
CBEType* CBERoot::FindTaggedType(int nType, String sTag)
{
    // search own types
    VectorElement *pIter = GetFirstTaggedType();
    CBEType *pTypeDecl;
    while ((pTypeDecl = GetNextTaggedType(pIter)) != 0)
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
    pIter = GetFirstClass();
    CBEClass *pClass;
    CBEType *pType;
    while ((pClass = GetNextClass(pIter)) != 0)
    {
        if ((pType = pClass->FindTaggedType(nType, sTag)) != 0)
            return pType;
    }
    // search namespaces
    pIter = GetFirstNameSpace();
    CBENameSpace *pNameSpace;
    while ((pNameSpace = GetNextNameSpace(pIter)) != 0)
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
    m_vTypeDeclarations.Add(pType);
}

/** \brief removes a tagged type from the root's collection
 *  \param pType the type to remove
 */
void CBERoot::RemoveTaggedType(CBEType *pType)
{
    m_vTypeDeclarations.Remove(pType);
}

/** \brief retrieves a reference to the first tagged type
 *  \return a pointer to the first tagged type declaration
 */
VectorElement* CBERoot::GetFirstTaggedType()
{
    return m_vTypeDeclarations.GetFirst();
}

/** \brief retrieve a reference to the next type declaration
 *  \param pIter the pointer to the next tagged type declaration
 *  \return a reference to the next tagged type declaration
 */
CBEType* CBERoot::GetNextTaggedType(VectorElement* &pIter)
{
    if (!pIter)
        return 0;
    CBEType *pRet = (CBEType*)pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextTaggedType(pIter);
    return pRet;
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
        VERBOSE("CBERoot::CreateBackEnd failed because tagged type could not be created\n");
        return false;
    }
    return true;
}

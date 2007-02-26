/**
 *  \file    dice/src/be/BERoot.cpp
 *  \brief   contains the implementation of the class CBERoot
 *
 *  \date    01/10/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#include "be/BEClass.h"
#include "be/BEType.h"
#include "be/BETypedef.h"
#include "be/BEFunction.h"
#include "be/BEConstant.h"
#include "be/BENameSpace.h"
#include "be/BEDeclarator.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "Compiler.h"
#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEConstDeclarator.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEConstructedType.h"
#include <cassert>
#include <iostream>

CBERoot::CBERoot()
: m_Constants(0, this),
  m_Namespaces(0, this),
  m_Classes(0, this),
  m_Typedefs(0, this),
  m_TypeDeclarations(0, this),
  m_GlobalFunctions(0, this)
{
    m_pClient = 0;
    m_pComponent = 0;
}

CBERoot::CBERoot(CBERoot & src)
: CBEObject(src),
  m_Constants(src.m_Constants),
  m_Namespaces(src.m_Namespaces),
  m_Classes(src.m_Classes),
  m_Typedefs(src.m_Typedefs),
  m_TypeDeclarations(src.m_TypeDeclarations),
  m_GlobalFunctions(src.m_GlobalFunctions)
{
    m_pClient = src.m_pClient;
    m_pComponent = src.m_pComponent;

    m_Constants.Adopt(this);
    m_Namespaces.Adopt(this);
    m_Classes.Adopt(this);
    m_Typedefs.Adopt(this);
    m_TypeDeclarations.Adopt(this);
    m_GlobalFunctions.Adopt(this);
}

/** \brief destructor
 */
CBERoot::~CBERoot()
{
    if (m_pClient)
        delete m_pClient;
    if (m_pComponent)
        delete m_pComponent;
}

/** \brief creates the back-end structure
 *  \param pFEFile a reference to the corresponding starting point
 *  \throw CBECreateException if error
 *
 * This implementation creates the corresponding client and component
 * parts. If these parts already exists the old versions are deleted
 * and replaced by the new ones.
 */
void CBERoot::CreateBE(CFEFile * pFEFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s(file: %s) called\n", __func__,
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
    // create the "normal" namespace-class-function hierarchy now, because
    // client and component depend on its existence
    try
    {
	CreateBackEnd(pFEFile);
    }
    catch (CBECreateException *e)
    {
	throw;
    }
    // create new client
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    if (CCompiler::IsOptionSet(PROGRAM_GENERATE_CLIENT))
    {
        m_pClient = pCF->GetNewClient();
        m_pClient->SetParent(this);
	try
	{
	    m_pClient->CreateBackEnd(pFEFile);
	}
	catch (CBECreateException *e)
	{
            delete m_pClient;
            m_pClient = 0;
	    throw;
        }
    }
    // create new component
    if (CCompiler::IsOptionSet(PROGRAM_GENERATE_COMPONENT))
    {
        m_pComponent = pCF->GetNewComponent();
        m_pComponent->SetParent(this);
	try
	{
	    m_pComponent->CreateBackEnd(pFEFile);
	}
	catch (CBECreateException *e)
        {
            delete m_pComponent;
            m_pComponent = 0;
	    throw;
        }
    }
}

/** \brief generates the output files and code
 */
void CBERoot::Write()
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s write client\n", __func__);
    if (m_pClient)
        m_pClient->Write();
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s write component\n", __func__);
    if (m_pComponent)
        m_pComponent->Write();
}

/** \brief tries to find the typedef to the given type-name
 *  \param sTypeName the name of the type to find
 *  \return a reference to the found typedef or 0
 *
 * Since we have all the elements in the containes types, constants,
 * classes and namespaces, we will search for the typedef first in our
 * own typedefs and then in the classes and namespaces.
 */
CBETypedef *CBERoot::FindTypedef(string sTypeName)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s(%s) called\n", __func__,
	sTypeName.c_str());
    CBETypedef *pTypedef = m_Typedefs.Find(sTypeName);
    if (pTypedef)
	return pTypedef;

    vector<CBEClass*>::iterator iterCl;
    for (iterCl = m_Classes.begin();
	 iterCl != m_Classes.end();
	 iterCl++)
    {
        if ((pTypedef = (*iterCl)->FindTypedef(sTypeName)) != 0)
	    return pTypedef;
    }

    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_Namespaces.begin();
	 iterN != m_Namespaces.end();
	 iterN++)
    {
        if ((pTypedef = (*iterN)->FindTypedef(sTypeName)) != 0)
            return pTypedef;
    }

    return 0;
}

/** \brief tries to find the function with the given name
 *  \param sFunctionName the name to search for
 *  \param nFunctionType the type of the function to find
 *  \return a reference to the function or NUL if not found
 *
 * To find a function, we search our classes and namespaces
 */
CBEFunction *CBERoot::FindFunction(string sFunctionName, 
    FUNCTION_TYPE nFunctionType)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s (%s) called\n", __func__,
        sFunctionName.c_str());
    CBEFunction *pFunction;
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
        if ((pFunction = (*iterC)->FindFunction(sFunctionName, 
		    nFunctionType)) != 0)
            return pFunction;
    }

    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_Namespaces.begin();
	 iterN != m_Namespaces.end();
	 iterN++)
    {
        if ((pFunction = (*iterN)->FindFunction(sFunctionName, 
		    nFunctionType)) != 0)
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
    CBEClass *pClass = m_Classes.Find(sClassName);
    if (pClass)
	return pClass;
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_Namespaces.begin();
	 iterN != m_Namespaces.end();
	 iterN++)
    {
        if ((pClass = (*iterN)->FindClass(sClassName)) != 0)
            return pClass;
    }
    return 0;
}

/** \brief tries to find a constant by its name
 *  \param sConstantName the name to look for
 *  \return a reference to the constant with this name or 0 if not found
 *
 * First we search our own constants, and because all constants are in there,
 * this should be sufficient.
 */
CBEConstant* CBERoot::FindConstant(string sConstantName)
{
    CBEConstant *pConstant = m_Constants.Find(sConstantName);
    if (pConstant)
	return pConstant;
    // search interfaces
    vector<CBEClass*>::iterator iterCl;
    for (iterCl = m_Classes.begin();
	 iterCl != m_Classes.end();
	 iterCl++)
    {
        if ((pConstant = (*iterCl)->m_Constants.Find(sConstantName)) != 0)
            return pConstant;
    }
    // search libraries
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_Namespaces.begin();
	 iterN != m_Namespaces.end();
	 iterN++)
    {
        if ((pConstant = (*iterN)->FindConstant(sConstantName)) != 0)
            return pConstant;
    }
    // nothing found
    return 0;
}

/** \brief searches for a namespace with the given name
 *  \param sNameSpaceName the name of the namespace
 *  \return a reference to the namespace or 0 if not found
 */
CBENameSpace* CBERoot::FindNameSpace(string sNameSpaceName)
{
    vector<CBENameSpace*>::iterator iter;
    for (iter = m_Namespaces.begin();
	 iter != m_Namespaces.end();
	 iter++)
    {
        // check the namespace itself
        if ((*iter)->GetName() == sNameSpaceName)
            return *iter;
        // check nested namespaces
	CBENameSpace *pFoundNameSpace;
        if ((pFoundNameSpace = (*iter)->FindNameSpace(sNameSpaceName)) != 0)
            return pFoundNameSpace;
    }
    return 0;
}

/** \brief creates the constants of a file
 *  \param pFEFile the front-end file to search for constants
 *
 * This method calls itself CreateBackEnd methods, which may throw exceptions.
 * These exceptions are simply propagated to the calling method.
 */
void CBERoot::CreateBackEnd(CFEFile *pFEFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__,
        pFEFile->GetFileName().c_str());
    // first search included files-> may contain base interfaces we need later
    // FIXME: for_each
    vector<CFEFile*>::iterator iterF;
    for (iterF = pFEFile->m_ChildFiles.begin();
	 iterF != pFEFile->m_ChildFiles.end();
	 iterF++)
    {
        CreateBackEnd(*iterF);
    }
    // next search top level consts
    vector<CFEConstDeclarator*>::iterator iterC;
    for (iterC = pFEFile->m_Constants.begin();
	 iterC != pFEFile->m_Constants.end();
	 iterC++)
    {
        CreateBackEnd(*iterC);
    }
    // next search top level typedefs
    vector<CFETypedDeclarator*>::iterator iterT;
    for (iterT = pFEFile->m_Typedefs.begin();
	 iterT != pFEFile->m_Typedefs.end();
	 iterT++)
    {
        CreateBackEnd(*iterT);
    }
    // next search top level type declarations
    vector<CFEConstructedType*>::iterator iterTD;
    for (iterTD = pFEFile->m_TaggedDeclarators.begin();
	 iterTD != pFEFile->m_TaggedDeclarators.end();
	 iterTD++)
    {
        CreateBackEnd(*iterTD);
    }
    // next search top level interfaces
    vector<CFEInterface*>::iterator iterI;
    for (iterI = pFEFile->m_Interfaces.begin();
	 iterI != pFEFile->m_Interfaces.end();
	 iterI++)
    {
        CreateBackEnd(*iterI);
    }
    // next search libraries
    vector<CFELibrary*>::iterator iterL;
    for (iterL = pFEFile->m_Libraries.begin();
	 iterL != pFEFile->m_Libraries.end();
	 iterL++)
    {
        CreateBackEnd(*iterL);
    }
}

/** \brief creates the constants for a specific library
 *  \param pFELibrary the front-end library to search for constants
 */
void CBERoot::CreateBackEnd(CFELibrary *pFELibrary)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__,
        pFELibrary->GetName().c_str());
    // first check if NameSpace is already there
    CBENameSpace *pNameSpace = FindNameSpace(pFELibrary->GetName());
    if (!pNameSpace)
    {
        // create NameSpace itself
        pNameSpace = CCompiler::GetClassFactory()->GetNewNameSpace();
        m_Namespaces.Add(pNameSpace);
	try
	{
	    pNameSpace->CreateBackEnd(pFELibrary);
	}
	catch (CBECreateException *e)
        {
	    m_Namespaces.Remove(pNameSpace);
            delete pNameSpace;
            throw;
        }
    }
    else
    {
        // call create function again to create the new Classs and such
	pNameSpace->CreateBackEnd(pFELibrary);
    }
}

/** \brief creates the back end for an interface
 *  \param  pFEInterface the interface to search for classes
 */
void CBERoot::CreateBackEnd(CFEInterface *pFEInterface)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__,
        pFEInterface->GetName().c_str());
    CBEClass *pClass = CCompiler::GetClassFactory()->GetNewClass();
    m_Classes.Add(pClass);
    try
    {
	pClass->CreateBackEnd(pFEInterface);
    }
    catch (CBECreateException *e)
    {
	m_Classes.Remove(pClass);
        delete pClass;
	e->Print();
	delete e;

	string fail = string (__func__);
    	fail += " failed because class ";
	fail += pFEInterface->GetName();
	fail += " could not be created";
	throw new CBECreateException(fail);
    }
}

/** \brief creates a back-end const for the front-end const
 *  \param pFEConstant the constant to use as reference
 */
void
CBERoot::CreateBackEnd(CFEConstDeclarator *pFEConstant)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__,
        pFEConstant->GetName().c_str());
    CBEConstant *pConstant = CCompiler::GetClassFactory()->GetNewConstant();
    m_Constants.Add(pConstant);
    try
    {
	pConstant->CreateBackEnd(pFEConstant);
    }
    catch (CBECreateException *e)
    {
	m_Constants.Remove(pConstant);
        delete pConstant;
	e->Print();
	delete e;

	string fail = string (__func__);
    	fail += " failed because BE constant could not be created";
	throw new CBECreateException(fail);
    }
}

/** \brief creates then back-end representation of an type definition
 *  \param pFETypedef the front-end type definition
 */
void
CBERoot::CreateBackEnd(CFETypedDeclarator *pFETypedef)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
    CBETypedef *pTypedef = CCompiler::GetClassFactory()->GetNewTypedef();
    m_Typedefs.Add(pTypedef);
    try
    {
	pTypedef->CreateBackEnd(pFETypedef);
    }
    catch (CBECreateException *e)
    {
	m_Typedefs.Remove(pTypedef);
        delete pTypedef;
	e->Print();
	delete e;

	string fail = string (__func__);
    	fail += " failed because BE type could not be created";
	throw new CBECreateException(fail);
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBERoot::AddTypedef for %s with type at %p called\n",
	pTypedef->m_Declarators.First()->GetName().c_str(),
	pTypedef->GetType());
	
}

/** \brief creates and stores a new tagged type declaration
 *  \param pFEType the respective front-end type
 */
void
CBERoot::CreateBackEnd(CFEConstructedType *pFEType)
{
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEType *pType = pCF->GetNewType(pFEType->GetType());
    m_TypeDeclarations.Add(pType);
    pType->SetParent(this);
    try
    {
	pType->CreateBackEnd(pFEType);
    }
    catch (CBECreateException *e)
    {
	m_TypeDeclarations.Remove(pType);
        delete pType;
	e->Print();
	delete e;

	string fail = string (__func__);
    	fail += " failed because BE tagged type could not be created";
	throw new CBECreateException(fail);
    }
}

/** \brief adds the members of the root to the header file
 *  \param pHeader the header file
 *  \return true if successful
 *
 * The root adds to the header files everything it own. It iterates over its
 * members and calls their respective AddToFile functions.
 */
bool CBERoot::AddToFile(CBEHeaderFile *pHeader)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s (%s) called\n", __func__,
        pHeader->GetFileName().c_str());
    // constants
    vector<CBEConstant*>::iterator iterC;
    for (iterC = m_Constants.begin();
	 iterC != m_Constants.end();
	 iterC++)
    {
        if (!(*iterC)->AddToFile(pHeader))
            return false;
    }
    // types
    vector<CBETypedef*>::iterator iterT;
    for (iterT = m_Typedefs.begin();
	 iterT != m_Typedefs.end();
	 iterT++)
    {
        if (!(*iterT)->AddToFile(pHeader))
            return false;
    }
    // tagged declarations
    vector<CBEType*>::iterator iterTa;
    for (iterTa = m_TypeDeclarations.begin();
	 iterTa != m_TypeDeclarations.end();
	 iterTa++)
    {
        if (!(*iterTa)->AddToFile(pHeader))
            return false;
    }
    // Classs
    vector<CBEClass*>::iterator iterCl;
    for (iterCl = m_Classes.begin();
	 iterCl != m_Classes.end();
	 iterCl++)
    {
        if (!(*iterCl)->AddToFile(pHeader))
            return false;
    }
    // libraries
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_Namespaces.begin();
	 iterN != m_Namespaces.end();
	 iterN++)
    {
        if (!(*iterN)->AddToFile(pHeader))
            return false;
    }
    return true;
}

/** \brief adds the members of the root to the implementation file
 *  \param pImpl the implementation file
 *  \return true if successful
 *
 * The root adds to the implementation file only the members of the Classs
 * and libraries.
 */
bool CBERoot::AddToFile(CBEImplementationFile *pImpl)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s(%s) called\n", __func__,
        pImpl->GetFileName().c_str());
    // Classs
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
        if (!(*iterC)->AddToFile(pImpl))
            return false;
    }
    // name-spaces
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_Namespaces.begin();
	 iterN != m_Namespaces.end();
	 iterN++)
    {
        if (!(*iterN)->AddToFile(pImpl))
            return false;
    }
    // global functions
    vector<CBEFunction*>::iterator iterF;
    for (iterF = m_GlobalFunctions.begin();
	 iterF != m_GlobalFunctions.end();
	 iterF++)
    {
        if (!(*iterF)->AddToFile(pImpl))
            return false;
    }
    return true;
}

/** \brief adds the opcodes of a file to the header files
 *  \param pHeader the header file to add the opcodes to
 *  \param pFEFile the respective front-end file to use as reference
 *  \return true if successful
 *
 * Root adds opcodes by calling its classes and namespaces. Because it should
 * only add the opcodes of the current IDL file, it is used as reference to
 * find these classes and namespaces
 */
bool CBERoot::AddOpcodesToFile(CBEHeaderFile *pHeader, CFEFile *pFEFile)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"%s(header: %s, file: %s) called\n", __func__,
        pHeader->GetFileName().c_str(), pFEFile->GetFileName().c_str());
    assert(pHeader);
    assert(pFEFile);
    // if FILE_ALL the included files have to be regarded as well
    // and because they may contain base interfaces, they come first
    if (CCompiler::IsFileOptionSet(PROGRAM_FILE_ALL))
    {
	vector<CFEFile*>::iterator iterF;
	for (iterF = pFEFile->m_ChildFiles.begin();
	    iterF != pFEFile->m_ChildFiles.end();
	    iterF++)
	{
	    if (!AddOpcodesToFile(pHeader, *iterF))
		return false;
        }
    }
    // classes
    vector<CFEInterface*>::iterator iterI;
    for (iterI = pFEFile->m_Interfaces.begin();
	 iterI != pFEFile->m_Interfaces.end();
	 iterI++)
    {
        CBEClass *pClass = FindClass((*iterI)->GetName());
        if (!pClass)
        {
            CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"%s failed because class %s could not be found\n",
                __func__, (*iterI)->GetName().c_str());
            return false;
        }
        if (!pClass->AddOpcodesToFile(pHeader))
            return false;
    }
    // namespaces
    vector<CFELibrary*>::iterator iterL;
    for (iterL = pFEFile->m_Libraries.begin();
	 iterL != pFEFile->m_Libraries.end();
	 iterL++)
    {
        CBENameSpace *pNameSpace = FindNameSpace((*iterL)->GetName());
        if (!pNameSpace)
        {
            CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"%s failed because namespace %s could not be found\n",
                __func__, (*iterL)->GetName().c_str());
            return false;
        }
        if (!pNameSpace->AddOpcodesToFile(pHeader))
            return false;
    }
    return true;
}

/** \brief prints the generated target files to the given output
 *  \param output the output stream to write to
 *  \param nCurCol the current column where to start to print (indent)
 *  \param nMaxCol the maximum number of columns
 */
void CBERoot::PrintTargetFiles(ostream& output, int &nCurCol, int nMaxCol)
{
    if (m_pClient)
    {
        m_pClient->PrintTargetFiles(output, nCurCol, nMaxCol);
    }
    if (m_pComponent)
    {
        m_pComponent->PrintTargetFiles(output, nCurCol, nMaxCol);
    }
}

/** \brief searches for a type with the given tag
 *  \param nType the type (struct/union/enum) of the searched type
 *  \param sTag the tag of the type
 *  \return a reference to the found type
 */
CBEType* CBERoot::FindTaggedType(unsigned int nType, string sTag)
{
    // search own types
    vector<CBEType*>::iterator iterT;
    for (iterT = m_TypeDeclarations.begin();
	 iterT != m_TypeDeclarations.end();
	 iterT++)
    {
        unsigned int nFEType = (*iterT)->GetFEType();
        if (nType != nFEType)
            continue;
        if (nFEType == TYPE_STRUCT ||
	    nFEType == TYPE_UNION ||
	    nFEType == TYPE_ENUM)
        {
            if ((*iterT)->HasTag(sTag))
                return *iterT;
        }
    }
    // search classes
    CBEType *pType;
    vector<CBEClass*>::iterator iterC;
    for (iterC = m_Classes.begin();
	 iterC != m_Classes.end();
	 iterC++)
    {
        if ((pType = (*iterC)->FindTaggedType(nType, sTag)) != 0)
            return pType;
    }
    // search namespaces
    vector<CBENameSpace*>::iterator iterN;
    for (iterN = m_Namespaces.begin();
	 iterN != m_Namespaces.end();
	 iterN++)
    {
        if ((pType = (*iterN)->FindTaggedType(nType, sTag)) != 0)
            return pType;
    }
    return 0;
}


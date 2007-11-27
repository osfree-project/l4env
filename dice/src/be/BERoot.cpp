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

#include "BERoot.h"
#include "BEContext.h"
#include "BEClient.h"
#include "BEComponent.h"
#include "BEClass.h"
#include "BEType.h"
#include "BETypedef.h"
#include "BEFunction.h"
#include "BEConstant.h"
#include "BEEnumType.h"
#include "BENameSpace.h"
#include "BEDeclarator.h"
#include "BEImplementationFile.h"
#include "BEHeaderFile.h"
#include "BENameFactory.h"
#include "BEClassFactory.h"
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
	m_TypeDeclarations(0, this)
{
	m_pClient = 0;
	m_pComponent = 0;
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
 *  \throw error::create_error if error
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
	CreateBackEnd(pFEFile);
	// create new client
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_CLIENT))
	{
		m_pClient = pCF->GetNewClient();
		m_pClient->SetParent(this);
		m_pClient->CreateBackEnd(pFEFile);
	}
	// create new component
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_COMPONENT))
	{
		m_pComponent = pCF->GetNewComponent();
		m_pComponent->SetParent(this);
		m_pComponent->CreateBackEnd(pFEFile);
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
 *  \param pPrev the previous found typedef with the same name
 *  \return a reference to the found typedef or 0
 */
CBETypedef *CBERoot::FindTypedef(std::string sTypeName, CBETypedef *pPrev)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBERoot::%s(%s, %p) called\n", __func__,
		sTypeName.c_str(), pPrev);
	return m_Typedefs.Find(sTypeName, pPrev);
}

/** \brief searches for an class
 *  \param sClassName the name of the class to look for
 *  \return a reference to the found class (or 0)
 *
 * First we search out top-level classes. If we can't find anything we
 * ask the namespaces.
 */
CBEClass* CBERoot::FindClass(std::string sClassName)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBERoot::FindClass(%s) called\n", sClassName.c_str());
	return m_Classes.Find(sClassName);
}

/** \brief tries to find a constant by its name
 *  \param sConstantName the name to look for
 *  \return a reference to the constant with this name or 0 if not found
 *
 * First we search our own constants, and because all constants are in there,
 * this should be sufficient.
 */
CBEConstant* CBERoot::FindConstant(std::string sConstantName)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBERoot::FindConstant(%s) called\n", sConstantName.c_str());
	return m_Constants.Find(sConstantName);
}

/** \brief searches for a namespace with the given name
 *  \param sNameSpaceName the name of the namespace
 *  \return a reference to the namespace or 0 if not found
 */
CBENameSpace* CBERoot::FindNameSpace(std::string sNameSpaceName)
{
	return m_Namespaces.Find(sNameSpaceName);
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
	for_each(pFEFile->m_ChildFiles.begin(), pFEFile->m_ChildFiles.end(),
		DoCall<CBERoot, CFEFile>(this, &CBERoot::CreateBackEnd));
	// next search top level consts
	for_each(pFEFile->m_Constants.begin(), pFEFile->m_Constants.end(),
		DoCall<CBERoot, CFEConstDeclarator>(this, &CBERoot::CreateBackEnd));
	// next search top level typedefs
	for_each(pFEFile->m_Typedefs.begin(), pFEFile->m_Typedefs.end(),
		DoCall<CBERoot, CFETypedDeclarator>(this, &CBERoot::CreateBackEnd));
	// next search top level type declarations
	for_each(pFEFile->m_TaggedDeclarators.begin(), pFEFile->m_TaggedDeclarators.end(),
		DoCall<CBERoot, CFEConstructedType>(this, &CBERoot::CreateBackEnd));
	// next search top level interfaces
	for_each(pFEFile->m_Interfaces.begin(), pFEFile->m_Interfaces.end(),
		DoCall<CBERoot, CFEInterface>(this, &CBERoot::CreateBackEnd));
	// next search libraries
	for_each(pFEFile->m_Libraries.begin(), pFEFile->m_Libraries.end(),
		DoCall<CBERoot, CFELibrary>(this, &CBERoot::CreateBackEnd));
}

/** \brief creates the constants for a specific library
 *  \param pFELibrary the front-end library to search for constants
 */
void CBERoot::CreateBackEnd(CFELibrary *pFELibrary)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__,
		pFELibrary->GetName().c_str());
	// first check if NameSpace is already there
	CBENameSpace *pNameSpace = SearchNamespace(pFELibrary->GetName());
	if (!pNameSpace)
	{
		// create NameSpace itself
		pNameSpace = CBEClassFactory::Instance()->GetNewNameSpace();
		m_Namespaces.Add(pNameSpace);
		pNameSpace->CreateBackEnd(pFELibrary);
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
	CBEClass *pClass = CBEClassFactory::Instance()->GetNewClass();
	m_Classes.Add(pClass);
	pClass->CreateBackEnd(pFEInterface);
}

/** \brief creates a back-end const for the front-end const
 *  \param pFEConstant the constant to use as reference
 */
void
CBERoot::CreateBackEnd(CFEConstDeclarator *pFEConstant)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__,
		pFEConstant->GetName().c_str());
	CBEConstant *pConstant = CBEClassFactory::Instance()->GetNewConstant();
	m_Constants.Add(pConstant);
	pConstant->CreateBackEnd(pFEConstant);
}

/** \brief creates then back-end representation of an type definition
 *  \param pFETypedef the front-end type definition
 */
void
CBERoot::CreateBackEnd(CFETypedDeclarator *pFETypedef)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
	CBETypedef *pTypedef = CBEClassFactory::Instance()->GetNewTypedef();
	m_Typedefs.Add(pTypedef);
	pTypedef->CreateBackEnd(pFETypedef);
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
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEType *pType = pCF->GetNewType(pFEType->GetType());
	m_TypeDeclarations.Add(pType);
	pType->SetParent(this);
	pType->CreateBackEnd(pFEType);
}

/** \brief adds the members of the root to the header file
 *  \param pHeader the header file
 *  \return true if successful
 *
 * The root adds to the header files everything it own. It iterates over its
 * members and calls their respective AddToHeader functions.
 */
void CBERoot::AddToHeader(CBEHeaderFile* pHeader)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s (%s) called\n", __func__,
		pHeader->GetFileName().c_str());
	// constants
	for_each(m_Constants.begin(), m_Constants.end(),
		std::bind2nd(std::mem_fun(&CBEConstant::AddToHeader), pHeader));
	// types
	for_each(m_Typedefs.begin(), m_Typedefs.end(),
		std::bind2nd(std::mem_fun(&CBETypedef::AddToHeader), pHeader));
	// tagged declarations
	for_each(m_TypeDeclarations.begin(), m_TypeDeclarations.end(),
		std::bind2nd(std::mem_fun(&CBEType::AddToHeader), pHeader));
	// Classs
	for_each(m_Classes.begin(), m_Classes.end(),
		std::bind2nd(std::mem_fun(&CBEClass::AddToHeader), pHeader));
	// libraries
	for_each(m_Namespaces.begin(), m_Namespaces.end(),
		std::bind2nd(std::mem_fun(&CBENameSpace::AddToHeader), pHeader));
}

/** \brief adds the members of the root to the implementation file
 *  \param pImpl the implementation file
 *  \return true if successful
 *
 * The root adds to the implementation file only the members of the Classs
 * and libraries.
 */
void CBERoot::AddToImpl(CBEImplementationFile* pImpl)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s(%s) called\n", __func__,
		pImpl->GetFileName().c_str());
	// Classs
	for_each(m_Classes.begin(), m_Classes.end(),
		std::bind2nd(std::mem_fun(&CBEClass::AddToImpl), pImpl));
	// name-spaces
	for_each(m_Namespaces.begin(), m_Namespaces.end(),
		std::bind2nd(std::mem_fun(&CBENameSpace::AddToImpl), pImpl));
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
void CBERoot::AddOpcodesToFile(CBEHeaderFile* pHeader, CFEFile *pFEFile)
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
			AddOpcodesToFile(pHeader, *iterF);
		}
	}
	// classes
	vector<CFEInterface*>::iterator iterI;
	for (iterI = pFEFile->m_Interfaces.begin();
		iterI != pFEFile->m_Interfaces.end();
		iterI++)
	{
		CBEClass *pClass = SearchClass((*iterI)->GetName());
		assert(pClass);
		pClass->AddOpcodesToFile(pHeader);
	}
	// namespaces
	vector<CFELibrary*>::iterator iterL;
	for (iterL = pFEFile->m_Libraries.begin();
		iterL != pFEFile->m_Libraries.end();
		iterL++)
	{
		CBENameSpace *pNameSpace = SearchNamespace((*iterL)->GetName());
		assert(pNameSpace);
		pNameSpace->AddOpcodesToFile(pHeader);
	}
}

/** \brief prints the generated target files to the given output
 *  \param output the output stream to write to
 *  \param nCurCol the current column where to start to print (indent)
 *  \param nMaxCol the maximum number of columns
 */
void CBERoot::PrintTargetFiles(ostream& output, int &nCurCol, int nMaxCol)
{
	if (m_pClient)
		m_pClient->PrintTargetFiles(output, nCurCol, nMaxCol);
	if (m_pComponent)
		m_pComponent->PrintTargetFiles(output, nCurCol, nMaxCol);
}

/** \brief top down search for namespace
 *  \param sNamespace the name of the namespace
 *  \return reference to found namespace
 */
CBENameSpace* CBERoot::SearchNamespace(std::string sNamespace)
{
	CBENameSpace *pRet = m_Namespaces.Find(sNamespace);
	if (pRet)
		return pRet;

	vector<CBENameSpace*>::iterator iter;
	for (iter = m_Namespaces.begin(); iter != m_Namespaces.end(); iter++)
	{
		if ((pRet = (*iter)->SearchNamespace(sNamespace)))
			return pRet;
	}
	return 0;
}

/** \brief top down search for class
 *  \param sClass the name of the class
 *  \return a reference to the class or 0 if not found
 *
 * If the name contains a scope, we have to get the first part of the scope,
 * check if we know the interface and then continue the search in that
 * interface.
 */
CBEClass* CBERoot::SearchClass(std::string sClass)
{
	// if there is a scope at the begin of the name, remove it (we are root)
	// and return with the result of the search in our classes only. No search
	// in the namespaces is allowed.
	if (sClass.find("::") == 0)
	{
		sClass = sClass.substr(2);
		return m_Classes.Find(sClass);
	}
	// seperate any namespaces from fully qualified name
	string::size_type pos = sClass.find("::");
	if (pos != string::npos)
	{
		string sNamespace = sClass.substr(0, pos);
		sClass = sClass.substr(pos+2);
		CBENameSpace *pNameSpace = m_Namespaces.Find(sNamespace);
		if (!pNameSpace)
			return 0;
		pNameSpace->SearchClass(sClass);
	}

	CBEClass *pRet = m_Classes.Find(sClass);
	if (pRet)
		return pRet;

	vector<CBENameSpace*>::iterator iter;
	for (iter = m_Namespaces.begin(); iter != m_Namespaces.end(); iter++)
	{
		if ((pRet = (*iter)->SearchClass(sClass)))
			return pRet;
	}
	return 0;
}

/** \brief searches for a type with the given tag
 *  \param nType the type (struct/union/enum) of the searched type
 *  \param sTag the tag of the type
 *  \return a reference to the found type
 */
CBEType* CBERoot::FindTaggedType(unsigned int nType, std::string sTag)
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
		if (TYPE_STRUCT == nFEType ||
			TYPE_UNION  == nFEType ||
			TYPE_ENUM   == nFEType)
		{
			if ((*iterT)->HasTag(sTag))
				return *iterT;
		}
	}
	return 0;
}

/** \brief tries to find an enumeration with the given enumerator
 *  \param sName the name of the enumerator
 *  \return the type containing the enumerator
 */
CBEEnumType* CBERoot::FindEnum(std::string sName)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBERoot::FindEnum(%s) called\n", sName.c_str());

	// search own types
	CBEEnumType* pEnum;
	vector<CBEType*>::iterator iterT;
	for (iterT = m_TypeDeclarations.begin();
		iterT != m_TypeDeclarations.end();
		iterT++)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
			"CBERoot::FindEnum try to find in own types\n");

		pEnum = dynamic_cast<CBEEnumType*>(*iterT);
		if (pEnum && pEnum->m_Members.Find(sName))
			return pEnum;
	}
	// search typedef
	vector<CBETypedef*>::iterator iterTD;
	for (iterTD = m_Typedefs.begin();
		iterTD != m_Typedefs.end();
		iterTD++)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
			"CBERoot::FindEnum try to find in own typedefs\n");

		pEnum = dynamic_cast<CBEEnumType*>((*iterTD)->GetType());
		if (pEnum && pEnum->m_Members.Find(sName))
			return pEnum;
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBERoot::FindEnum returns NULL\n");
	return 0;
}

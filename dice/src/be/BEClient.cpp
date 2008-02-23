/**
 *  \file    dice/src/be/BEClient.cpp
 *  \brief   contains the implementation of the class CBEClient
 *
 *  \date    01/11/2002
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

#include "BEClient.h"
#include "BEFile.h"
#include "BEContext.h"
#include "BESndFunction.h"
#include "BEWaitFunction.h"
#include "BECallFunction.h"
#include "BEUnmarshalFunction.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEExpression.h"
#include "BEConstant.h"
#include "BERoot.h"
#include "BEClass.h"
#include "BENameSpace.h"
#include "BENameFactory.h"
#include "BEClassFactory.h"
#include "Compiler.h"
#include "Error.h"
#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FEUnaryExpression.h"
#include "fe/FEIntAttribute.h"
#include <algorithm>
#include <cassert>

CBEClient::CBEClient()
{ }

/** \brief destructor
 */
CBEClient::~CBEClient()
{ }

/** \brief writes the clients output
 */
void CBEClient::Write()
{
    CCompiler::Verbose("CBEClient::%s called\n", __func__);
    WriteHeaderFiles();
    WriteImplementationFiles();
    CCompiler::Verbose("CBEClient::%s done.\n", __func__);
}

/** \brief creates the back-end files for a function
 *  \param pFEOperation the front-end function to use as reference
 *  \return true if successful
 */
void CBEClient::CreateBackEndFunction(CFEOperation *pFEOperation)
{
	CCompiler::Verbose("CBEClient::%s for %s called\n", __func__,
		pFEOperation->GetName().c_str());

	// find appropriate header file
	CBEHeaderFile* pHeader = FindHeaderFile(pFEOperation, FILETYPE_CLIENTHEADER);
	assert(pHeader);
	// create the file
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEImplementationFile* pImpl = pCF->GetNewImplementationFile();
	m_ImplementationFiles.Add(pImpl);
	pImpl->SetHeaderFile(pHeader);
	pImpl->CreateBackEnd(pFEOperation, FILETYPE_CLIENTIMPLEMENTATION);
	// add the functions to the file
	// search the functions
	// if attribute == IN, we need send
	// if attribute == OUT, we need wait, recv, unmarshal
	// if attribute == empty, we need call if test, we need test
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sFuncName;
	CBEFunction *pFunction;
	if (pFEOperation->m_Attributes.Find(ATTR_IN))
	{
		sFuncName = pNF->GetFunctionName(pFEOperation, FUNCTION_SEND, false);
		pFunction = FindFunction(sFuncName, FUNCTION_SEND);
		assert(pFunction);
		pFunction->AddToImpl(pImpl);
	}
	else if (pFEOperation->m_Attributes.Find(ATTR_OUT))
	{
		// wait function
		sFuncName = pNF->GetFunctionName(pFEOperation, FUNCTION_WAIT, false);
		pFunction = FindFunction(sFuncName, FUNCTION_WAIT);
		assert(pFunction);
		pFunction->AddToImpl(pImpl);
		// receive function
		sFuncName = pNF->GetFunctionName(pFEOperation, FUNCTION_RECV, false);
		pFunction = FindFunction(sFuncName, FUNCTION_RECV);
		assert(pFunction);
		pFunction->AddToImpl(pImpl);
		// unmarshal function
		if (CCompiler::IsOptionSet(PROGRAM_GENERATE_MESSAGE))
		{
			sFuncName = pNF->GetFunctionName(pFEOperation, FUNCTION_UNMARSHAL, false);
			pFunction = FindFunction(sFuncName, FUNCTION_UNMARSHAL);
			assert(pFunction);
			pFunction->AddToImpl(pImpl);
		}
	}
	else
	{
		sFuncName = pNF->GetFunctionName(pFEOperation, FUNCTION_CALL, false);
		pFunction = FindFunction(sFuncName, FUNCTION_CALL);
		assert(pFunction);
		pFunction->AddToImpl(pImpl);
	}

	CCompiler::Verbose("CBEClient::%s returns\n", __func__);
}

/** \brief creates the header files of the client
 *  \param pFEFile the front end file to use as reference
 *  \return true if successful
 *
 * We could call the base class to create the header file as usual. But we
 * need a reference to it, which we would have to search for. Thus we simply
 * create it here, as the base class would do and use its reference.
 */
void CBEClient::CreateBackEndHeader(CFEFile * pFEFile)
{
	string exc = string(__func__);
	CCompiler::Verbose("CBEClient::CreateBackEndHeader(file: %s) called\n",
		pFEFile->GetFileName().c_str());
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	assert(pRoot);
	// the header files are created on a per IDL file basis, no matter
	// which option is set
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEHeaderFile* pHeader = pCF->GetNewHeaderFile();
	m_HeaderFiles.Add(pHeader);
	pHeader->CreateBackEnd(pFEFile, FILETYPE_CLIENTHEADER);
	pRoot->AddToHeader(pHeader);
	// create opcode files per IDL file
	if (!CCompiler::IsOptionSet(PROGRAM_NO_OPCODES))
	{
		CBEHeaderFile* pOpcodes = pCF->GetNewHeaderFile();
		m_HeaderFiles.Add(pOpcodes);
		pOpcodes->CreateBackEnd(pFEFile, FILETYPE_OPCODE);
		pRoot->AddOpcodesToFile(pOpcodes, pFEFile);
		// include opcode file to included files
		// do not use include file name, since the opcode file is
		// assumed to be in the same directory
		pHeader->AddIncludedFileName(pOpcodes->GetFileName(), true, false,
			pFEFile);
	}
	CCompiler::Verbose("CBEClient::CreateBackEndHeader(file: %s) return true\n",
		pFEFile->GetFileName().c_str());
}

/** \brief create the back-end implementation files
 *  \param pFEFile the respective front-end file
 *  \return true if successful
 */
void CBEClient::CreateBackEndImplementation(CFEFile * pFEFile)
{
	CCompiler::Verbose("CBEClient::CreateBackEndImplementation(file: %s) called\n",
		pFEFile->GetFileName().c_str());
	// depending on options call respective functions
	if (CCompiler::IsFileOptionSet(PROGRAM_FILE_ALL) ||
		CCompiler::IsFileOptionSet(PROGRAM_FILE_IDLFILE))
		CreateBackEndFile(pFEFile);
	else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_MODULE))
		CreateBackEndModule(pFEFile);
	else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_INTERFACE))
		CreateBackEndInterface(pFEFile);
	else if (CCompiler::IsFileOptionSet(PROGRAM_FILE_FUNCTION))
		CreateBackEndFunction(pFEFile);
}

/** \brief internal function to create a file-pair per front-end file
 *  \param pFEFile the respective front-end file
 *  \return true if successful
 *
 * the client generates one implementation file per IDL file or for all IDL
 * files (depending on the options).
 */
void CBEClient::CreateBackEndFile(CFEFile *pFEFile)
{
	if (!pFEFile->IsIDLFile())
		return;

	CCompiler::Verbose("CBEClient::CreateBackEndFile(file: %s) called\n",
		pFEFile->GetFileName().c_str());

	string exc = string(__func__);
	// find appropriate header file
	CBEHeaderFile* pHeader = FindHeaderFile(pFEFile, FILETYPE_CLIENTHEADER);
	assert(pHeader);
	// create file
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEImplementationFile* pImpl = pCF->GetNewImplementationFile();
	m_ImplementationFiles.Add(pImpl);
	pImpl->SetHeaderFile(pHeader);
	pImpl->CreateBackEnd(pFEFile, FILETYPE_CLIENTIMPLEMENTATION);
	// add interfaces and functions
	// throws exception if failing
	CreateBackEndFile(pFEFile, pImpl);
}

/** \brief internal functions, which adds all members of a file to a file
 *  \param pFEFile the front-end file to search
 *  \param pImpl the implementation file to add the members to
 *  \return true if successful
 */
void CBEClient::CreateBackEndFile(CFEFile * pFEFile, CBEImplementationFile* pImpl)
{
	CCompiler::Verbose("CBEClient::CreateBackEndFile(file: %s, impl: %s) called\n",
		pFEFile->GetFileName().c_str(), pImpl->GetFileName().c_str());

	// iterate over interfaces and add them
	vector<CFEInterface*>::iterator iterI;
	for (iterI = pFEFile->m_Interfaces.begin();
		iterI != pFEFile->m_Interfaces.end();
		iterI++)
	{
		CBEClass *pClass = FindClass((*iterI)->GetName());
		assert(pClass);
		// if class has been added already, then skip it
		if (pImpl->FindClass(pClass->GetName()) != pClass)
			pClass->AddToImpl(pImpl);
	}
	// iterate over libraries and add them
	vector<CFELibrary*>::iterator iterL;
	for (iterL = pFEFile->m_Libraries.begin();
		iterL != pFEFile->m_Libraries.end();
		iterL++)
	{
		CBENameSpace *pNameSpace = FindNameSpace((*iterL)->GetName());
		assert(pNameSpace);
		// if this namespace is already added, skip it
		if (pImpl->FindNameSpace(pNameSpace->GetName()) != pNameSpace)
			pNameSpace->AddToImpl(pImpl);
	}
	// if FILE_ALL: iterate over included files and call this function using
	// them
	if (CCompiler::IsFileOptionSet(PROGRAM_FILE_ALL))
	{
		vector<CFEFile*>::iterator iterF;
		for (iterF = pFEFile->m_ChildFiles.begin();
			iterF != pFEFile->m_ChildFiles.end();
			iterF++)
		{
			CreateBackEndFile(*iterF, pImpl);
		}
	}
}

/** \brief creates the back-end files for the FILE_MODULE option
 *  \param pFEFile the respective front-end file
 *  \return true if successful
 *
 * Because a file may also contain interfaces, we have to create a file for
 * them as well (if there are any). We assume that interfaces without a module
 * belong to a "default" module (or namespace). The name of the file for these
 * interfaces is derived from the IDL file directly. We do not add typedefs and
 * constants, because we do not add them to implementation files.
 */
void CBEClient::CreateBackEndModule(CFEFile *pFEFile)
{
	if (!pFEFile->IsIDLFile())
		return; // do not abort creation

	CCompiler::Verbose("CBEClient::%s(file: %s) called\n",
		__func__, pFEFile->GetFileName().c_str());

	// find appropriate header file
	CBEHeaderFile* pHeader = FindHeaderFile(pFEFile, FILETYPE_CLIENTHEADER);
	assert(pHeader);
	// check if we have interfaces
	CFEInterface *pFEInterface = pFEFile->m_Interfaces.First();
	if (pFEInterface)
	{
		// we do have interfaces
		// create file
		CBEClassFactory *pCF = CBEClassFactory::Instance();
		CBEImplementationFile* pImpl = pCF->GetNewImplementationFile();
		m_ImplementationFiles.Add(pImpl);
		pImpl->SetHeaderFile(pHeader);
		pImpl->CreateBackEnd(pFEFile, FILETYPE_CLIENTIMPLEMENTATION);
		// add interfaces to this file
		vector<CFEInterface*>::iterator iterI;
		for (iterI = pFEFile->m_Interfaces.begin();
			iterI != pFEFile->m_Interfaces.end();
			iterI++)
		{
			// find interface
			CBEClass *pClass = FindClass((*iterI)->GetName());
			assert(pClass);
			// add interface to file
			pClass->AddToImpl(pImpl);
		}
	}
	// iterate over libraries and create files for them
	for_each(pFEFile->m_Libraries.begin(), pFEFile->m_Libraries.end(),
		DoCall<CBEClient, CFELibrary>(this, &CBEClient::CreateBackEndModule));
	// success
}

/** \brief creates the file for the module
 *  \param pFELibrary the respective front-end library
 *  \return true if successful
 */
void CBEClient::CreateBackEndModule(CFELibrary *pFELibrary)
{
	CCompiler::Verbose("CBEClient::%s(lib: %s) called\n",
		__func__, pFELibrary->GetName().c_str());

	// find appropriate header file
	CBEHeaderFile* pHeader = FindHeaderFile(pFELibrary, FILETYPE_CLIENTHEADER);
	assert(pHeader);
	// search for the library
	CBENameSpace *pBENameSpace = FindNameSpace(pFELibrary->GetName());
	assert(pBENameSpace);
	// create the file
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEImplementationFile* pImpl = pCF->GetNewImplementationFile();
	m_ImplementationFiles.Add(pImpl);
	pImpl->SetHeaderFile(pHeader);
	pImpl->CreateBackEnd(pFELibrary, FILETYPE_CLIENTIMPLEMENTATION);
	// add it to the file
	pBENameSpace->AddToImpl(pImpl);
	// iterate over nested libs and call this function for them as well
	for_each(pFELibrary->m_Libraries.begin(), pFELibrary->m_Libraries.end(),
		DoCall<CBEClient, CFELibrary>(this, &CBEClient::CreateBackEndModule));
}

/** \brief creates the files for the FILE_INTERFACE option
 *  \param pFEFile the file to search for interfaces
 *  \return true if successful
 */
void CBEClient::CreateBackEndInterface(CFEFile *pFEFile)
{
	CCompiler::Verbose("CBEClient::CreateBackEndInterface(file: %s) called\n",
		pFEFile->GetFileName().c_str());
	// search for top-level interfaces
	for_each(pFEFile->m_Interfaces.begin(), pFEFile->m_Interfaces.end(),
		DoCall<CBEClient, CFEInterface>(this, &CBEClient::CreateBackEndInterface));
	// search for libraries
	for_each(pFEFile->m_Libraries.begin(), pFEFile->m_Libraries.end(),
		DoCall<CBEClient, CFELibrary>(this, &CBEClient::CreateBackEndInterface));
}

/** \brief creates the file for the FILE_INTERFACE option
 *  \param pFELibrary the module to search for interfaces
 *  \return true if successful
 */
void CBEClient::CreateBackEndInterface(CFELibrary *pFELibrary)
{
	CCompiler::Verbose("CBEClient::CreateBackEndInterface(lib: %s) called\n",
		pFELibrary->GetName().c_str());
	// search for interfaces
	for_each(pFELibrary->m_Interfaces.begin(), pFELibrary->m_Interfaces.end(),
		DoCall<CBEClient, CFEInterface>(this, &CBEClient::CreateBackEndInterface));
	// search for nested libs
	for_each(pFELibrary->m_Libraries.begin(), pFELibrary->m_Libraries.end(),
		DoCall<CBEClient, CFELibrary>(this, &CBEClient::CreateBackEndInterface));
}

/** \brief create the back-end file for an interface
 *  \param pFEInterface the front-end interface
 *  \return true if successful
 */
void CBEClient::CreateBackEndInterface(CFEInterface *pFEInterface)
{
	CCompiler::Verbose("CBEClient::CreateBackEndInterface(interface: %s) called\n",
		pFEInterface->GetName().c_str());

	// find appropriate header file
	CBEHeaderFile* pHeader = FindHeaderFile(pFEInterface, FILETYPE_CLIENTHEADER);
	assert(pHeader);
	// find the interface
	CBEClass *pBEClass = FindClass(pFEInterface->GetName());
	assert(pBEClass);
	// create the file
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEImplementationFile* pImpl = pCF->GetNewImplementationFile();
	m_ImplementationFiles.Add(pImpl);
	pImpl->SetHeaderFile(pHeader);
	pImpl->CreateBackEnd(pFEInterface, FILETYPE_CLIENTIMPLEMENTATION);
	// add the interface
	pBEClass->AddToImpl(pImpl);
}

/** \brief creates the files for the FILE_FUNCTION option
 *  \param pFEFile the file to search for functions
 *  \return true if successful
 */
void CBEClient::CreateBackEndFunction(CFEFile *pFEFile)
{
	CCompiler::Verbose("CBEClient::%s for %s called\n", __func__,
		pFEFile->GetFileName().c_str());
	// if there are any top level type definitions and  constants
	// iterate over interfaces
	for_each(pFEFile->m_Interfaces.begin(), pFEFile->m_Interfaces.end(),
		DoCall<CBEClient, CFEInterface>(this, &CBEClient::CreateBackEndFunction));
	// iterate over libraries
	for_each(pFEFile->m_Libraries.begin(), pFEFile->m_Libraries.end(),
		DoCall<CBEClient, CFELibrary>(this, &CBEClient::CreateBackEndFunction));
}

/** \brief creates the back-end files for the FILE_FUNCTION option
 *  \param pFELibrary the library to search for functions
 *  \return true if successful
 */
void CBEClient::CreateBackEndFunction(CFELibrary *pFELibrary)
{
	CCompiler::Verbose("CBEClient::%s for %s called\n", __func__,
		pFELibrary->GetName().c_str());
	// search for interface
	for_each(pFELibrary->m_Interfaces.begin(), pFELibrary->m_Interfaces.end(),
		DoCall<CBEClient, CFEInterface>(this, &CBEClient::CreateBackEndFunction));
	// search for nested libs
	for_each(pFELibrary->m_Libraries.begin(), pFELibrary->m_Libraries.end(),
		DoCall<CBEClient, CFELibrary>(this, &CBEClient::CreateBackEndFunction));
}

/** \brief creates the back-end file for the FILE_FUNCTION option
 *  \param pFEInterface the interface to search for the functions
 *  \return true if successful
 */
void CBEClient::CreateBackEndFunction(CFEInterface *pFEInterface)
{
	CCompiler::Verbose("CBEClient::%s for %s called\n", __func__,
		pFEInterface->GetName().c_str());
	// search the interface
	for_each(pFEInterface->m_Operations.begin(), pFEInterface->m_Operations.end(),
		DoCall<CBEClient, CFEOperation>(this, &CBEClient::CreateBackEndFunction));
}


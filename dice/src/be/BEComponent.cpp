/**
 *  \file    dice/src/be/BEComponent.cpp
 *  \brief   contains the implementation of the class CBEComponent
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

#include "BEComponent.h"
#include "BEFile.h"
#include "BEContext.h"
#include "BEWaitFunction.h"
#include "BESrvLoopFunction.h"
#include "BEUnmarshalFunction.h"
#include "BEComponentFunction.h"
#include "BEWaitAnyFunction.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BERoot.h"
#include "BENameSpace.h"
#include "BENameFactory.h"
#include "BEClassFactory.h"
#include "Compiler.h"
#include "Error.h"
#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include <cassert>

CBEComponent::CBEComponent()
{ }

/** \brief destructor
 */
CBEComponent::~CBEComponent()
{ }

/** \brief writes the output
 *
 * The component's write does initiate the write for each of it's files
 */
void CBEComponent::Write()
{
    CCompiler::Verbose("CBEComponent::%s called\n", __func__);
    WriteHeaderFiles();
    WriteImplementationFiles();
    CCompiler::Verbose("CBEComponent::%s done.\n", __func__);
}

/** \brief checks whether this interface needs a server loop
 *  \param pFEInterface the interface to check
 *  \return true if a server loop is needed
 *
 * A server loop is not needed if all functions of the interface and its base
 * interfaces are message passing interfaces.  So if at least one of the
 * functions (operations) is any RPC function we need a server loop.
 *
 * There is also no server loop needed if the interface is abstract.
 */
bool
CBEComponent::NeedServerLoop(CFEInterface * pFEInterface)
{
    // test astract attribute
    if (pFEInterface->m_Attributes.Find(ATTR_ABSTRACT))
        return false;
    // test functions
    vector<CFEOperation*>::iterator iterO;
    for (iterO = pFEInterface->m_Operations.begin();
	 iterO != pFEInterface->m_Operations.end();
	 iterO++)
    {
        if (!((*iterO)->m_Attributes.Find(ATTR_IN)) &&
            !((*iterO)->m_Attributes.Find(ATTR_OUT)))
        return true;
    }

    vector<CFEInterface*>::iterator iterI;
    for (iterI = pFEInterface->m_BaseInterfaces.begin();
	 iterI != pFEInterface->m_BaseInterfaces.end();
	 iterI++)
    {
        if (NeedServerLoop(*iterI))
            return true;
    }

    return false;
}

/** \brief creates the header files for the component
 *  \param pFEFile the front-end file to use as reference
 *  \return true if successful
 *
 * We could call the base class' implementation but we need a reference to the
 * header, we then would have to search for. Therefore we simply do what the
 * base class would do and use the local reference to the header file.
 */
void
CBEComponent::CreateBackEndHeader(CFEFile * pFEFile)
{
	// the header files are created on a per IDL file basis, no matter
	// which option is set
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEHeaderFile* pHeader = pCF->GetNewHeaderFile();
	m_HeaderFiles.Add(pHeader);
	pHeader->CreateBackEnd(pFEFile, FILETYPE_COMPONENTHEADER);
	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	pRoot->AddToHeader(pHeader);
	// add include of opcode file to header file
	if (!CCompiler::IsOptionSet(PROGRAM_NO_OPCODES))
	{
		if (!CCompiler::IsOptionSet(PROGRAM_GENERATE_CLIENT))
		{
			// create opcode header file outselves, because client, which
			// normally does, is not created
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
		else
		{
			// get name
			// do not use include file name, since we assume, that opcode
			// file is in same directory
			CBENameFactory *pNF = CBENameFactory::Instance();
			string sOpcode = pNF->GetFileName(pFEFile, FILETYPE_OPCODE);
			pHeader->AddIncludedFileName(sOpcode, true, false, pFEFile);
		}
	}
}

/** \brief create the back-end implementation file
 *  \param pFEFile the front-end file to use as reference
 *  \return true if successful
 */
void
CBEComponent::CreateBackEndImplementation(CFEFile * pFEFile)
{
	string exc = string(__func__);
	// find appropriate header file
	CBEHeaderFile* pHeader = FindHeaderFile(pFEFile, FILETYPE_COMPONENTHEADER);
	if (!pHeader)
	{
		exc += " failed, because could not find header file for " +
			pFEFile->GetFileName();
		throw new error::create_error(exc);
	}
	// the implementation files are created on a per IDL file basis, no matter
	// which option is set
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEImplementationFile* pImpl = pCF->GetNewImplementationFile();
	m_ImplementationFiles.Add(pImpl);
	pImpl->SetHeaderFile(pHeader);
	pImpl->CreateBackEnd(pFEFile, FILETYPE_COMPONENTIMPLEMENTATION);

	CBERoot *pRoot = GetSpecificParent<CBERoot>();
	assert(pRoot);
	pRoot->AddToImpl(pImpl);
	// if create component function, we use seperate file for this
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_TEMPLATE))
	{
		pImpl = pCF->GetNewImplementationFile();
		m_ImplementationFiles.Add(pImpl);
		pImpl->SetHeaderFile(pHeader);
		pImpl->CreateBackEnd(pFEFile, FILETYPE_TEMPLATE);
		pRoot->AddToImpl(pImpl);
	}
}


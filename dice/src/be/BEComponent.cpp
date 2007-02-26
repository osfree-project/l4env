/**
 *	\file	dice/src/be/BEComponent.cpp
 *	\brief	contains the implementation of the class CBEComponent
 *
 *	\date	01/11/2002
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

#include "be/BEComponent.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BESndFunction.h"
#include "be/BERcvFunction.h"
#include "be/BEWaitFunction.h"
#include "be/BEReplyRcvFunction.h"
#include "be/BEReplyWaitFunction.h"
#include "be/BESrvLoopFunction.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BEComponentFunction.h"
#include "be/BEWaitAnyFunction.h"
#include "be/BEReplyAnyWaitAnyFunction.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BERoot.h"

#include "be/BENameSpace.h"

#include "fe/FEFile.h"
#include "fe/FELibrary.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

IMPLEMENT_DYNAMIC(CBEComponent);

CBEComponent::CBEComponent()
{
    IMPLEMENT_DYNAMIC_BASE(CBEComponent, CBETarget);
}

CBEComponent::CBEComponent(CBEComponent & src):CBETarget(src)
{
    IMPLEMENT_DYNAMIC_BASE(CBEComponent, CBETarget);
}

/**	\brief destructor
 */
CBEComponent::~CBEComponent()
{

}

/**	\brief writes the output
 *	\param pContext the context of the write operation
 *
 * The component's write does initiate the write for each of it's files
 */
void CBEComponent::Write(CBEContext * pContext)
{
    WriteHeaderFiles(pContext);
    WriteImplementationFiles(pContext);
}

/**	\brief sets the current file-type in the context
 *	\param pContext the context to manipulate
 *	\param nHeaderOrImplementation a flag to indicate whether we need a header or implementation file
 */
void CBEComponent::SetFileType(CBEContext * pContext, int nHeaderOrImplementation)
{
    switch (nHeaderOrImplementation)
    {
    case FILETYPE_HEADER:
        pContext->SetFileType(FILETYPE_COMPONENTHEADER);
        break;
    case FILETYPE_IMPLEMENTATION:
        pContext->SetFileType(FILETYPE_COMPONENTIMPLEMENTATION);
        break;
    default:
        CBETarget::SetFileType(pContext, nHeaderOrImplementation);
        break;
    }
}

/**	\brief checks whether this interface needs a server loop
 *	\param pFEInterface the interface to check
 *	\param pContext the context of th code generation
 *	\return true if a server loop is needed
 *
 * A server loop is not needed if all functions of the interface and its base interfaces are message passing interfaces.
 * So if at least one of the functions (operations) is any RPC function we need a server loop.
 */
bool CBEComponent::NeedServerLoop(CFEInterface * pFEInterface, CBEContext * pContext)
{
    VectorElement *pIter = pFEInterface->GetFirstOperation();
    CFEOperation *pOperation;
    while ((pOperation = pFEInterface->GetNextOperation(pIter)) != 0)
    {
        if (!(pOperation->FindAttribute(ATTR_IN)) &&
            !(pOperation->FindAttribute(ATTR_OUT)))
        return true;
    }

    pIter = pFEInterface->GetFirstBaseInterface();
    CFEInterface *pBase;
    while ((pBase = pFEInterface->GetNextBaseInterface(pIter)) != 0)
    {
        if (NeedServerLoop(pBase, pContext))
            return true;
    }

    return false;
}

/** \brief creates the header files for the component
 *  \param pFEFile the front-end file to use as reference
 *  \param pContext the context of this operation
 *  \return true if successful
 *
 * We could call the base class' implementation but we need a reference to the header, we
 * then would have to search for. Therefore we simply do what the base class would do and use
 * the local reference to the header file.
 */
bool CBEComponent::CreateBackEndHeader(CFEFile * pFEFile, CBEContext * pContext)
{
    // the header files are created on a per IDL file basis, no matter
    // which option is set
    CBEHeaderFile *pHeader = pContext->GetClassFactory()->GetNewHeaderFile();
    AddFile(pHeader);
    pContext->SetFileType(FILETYPE_COMPONENTHEADER);
    if (!pHeader->CreateBackEnd(pFEFile, pContext))
    {
        RemoveFile(pHeader);
        delete pHeader;
        VERBOSE("CBEComponent::CreateBackEndHeader failed because header file could not be created\n");
        return false;
    }
    GetRoot()->AddToFile(pHeader, pContext);
    // add include of opcode file to header file
    if (!pContext->IsOptionSet(PROGRAM_NO_OPCODES))
    {
        // get name
        // do not use include file name, since we assume, that opcode
        // file is in same directory
        pContext->SetFileType(FILETYPE_OPCODE);
        String sOpcode = pContext->GetNameFactory()->GetFileName(pFEFile, pContext);
        pHeader->AddIncludedFileName(sOpcode, true);
    }
    return true;
}

/** \brief create the back-end implementation file
 *  \param pFEFile the front-end file to use as reference
 *  \param pContext the context of this operation
 *  \return true if successful
 */
bool CBEComponent::CreateBackEndImplementation(CFEFile * pFEFile, CBEContext * pContext)
{
    // find appropriate header file
    CBEHeaderFile *pHeader = FindHeaderFile(pFEFile, pContext);
    if (!pHeader)
        return false;
    // the implementation files are created on a per IDL file basis, no matter
    // which option is set
    CBEImplementationFile *pImpl = pContext->GetClassFactory()->GetNewImplementationFile();
    AddFile(pImpl);
    pImpl->SetHeaderFile(pHeader);
    pContext->SetFileType(FILETYPE_COMPONENTIMPLEMENTATION);
    if (!pImpl->CreateBackEnd(pFEFile, pContext))
    {
        RemoveFile(pImpl);
        delete pImpl;
        VERBOSE("CBEComponent::CreateBackEndHeader failed because header file could not be created\n");
        return false;
    }
    GetRoot()->AddToFile(pImpl, pContext);
    // if create component function, we use seperate file for this
    if (pContext->IsOptionSet(PROGRAM_GENERATE_SKELETON))
    {
		pImpl = pContext->GetClassFactory()->GetNewImplementationFile();
		AddFile(pImpl);
		pImpl->SetHeaderFile(pHeader);
		pContext->SetFileType(FILETYPE_SKELETON);
		if (!pImpl->CreateBackEnd(pFEFile, pContext))
		{                	                	
			RemoveFile(pImpl);
			delete pImpl;       	
			VERBOSE("CBEComponent::CreateBackEndImplementation failed because implementation file could not be created\n");
			return false;               	
		}
		GetRoot()->AddToFile(pImpl, pContext);
	}
	return true;
}

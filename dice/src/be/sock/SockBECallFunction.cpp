/**
 *  \file    dice/src/be/sock/SockBECallFunction.cpp
 *  \brief   contains the implementation of the class CSockBECallFunction
 *
 *  \date    11/28/2002
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

#include "SockBECallFunction.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMarshaller.h"
#include "be/BEMsgBuffer.h"
#include "be/BENameFactory.h"
#include "be/BEDeclarator.h"
#include "BESocket.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"
#include <cassert>

CSockBECallFunction::CSockBECallFunction()
{
}

CSockBECallFunction::CSockBECallFunction(CSockBECallFunction & src)
: CBECallFunction(src)
{
}

/** \brief destructor of target class */
CSockBECallFunction::~CSockBECallFunction()
{

}

/** \brief invoke the call to the server
 *  \param pFile the file to write to
 *
 * Now this is a bit hairy:
 * -# need to open socke (socket call)
 * -# set parametes
 * -# send to socket
 * -# receive from socket
 * -# clode socket
 */
void
CSockBECallFunction::WriteInvocation(CBEFile& pFile)
{
    CBECommunication *pComm = GetCommunication();
    assert(pComm);
    // create socket
    pComm->WriteInitialization(pFile, this);
    // call
    pComm->WriteCall(pFile, this);
    // close socket
    pComm->WriteCleanup(pFile, this);
}

/** \brief initializes the variables
 *  \param pFile the file to write to
 */
void 
CSockBECallFunction::WriteVariableInitialization(CBEFile& pFile)
{
    CBECallFunction::WriteVariableInitialization(pFile);
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sOffset = pNF->GetOffsetVariable();
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    string sPtrName, sSizeName;
    if (pMsgBuffer->m_Declarators.First()->GetStars() == 0)
	sPtrName = "&";
    else
	sSizeName = "*";
    sPtrName += pMsgBuffer->m_Declarators.First()->GetName();
    sSizeName += pMsgBuffer->m_Declarators.First()->GetName(); 

    pFile << "\tbzero(" << sPtrName << ", sizeof(" << sSizeName << "));\n";
}

/** \brief initialize the instance of this class
 *  \param pFEOperation the front-end operation to use as reference
 *  \return true if successful
 */
void
CSockBECallFunction::CreateBackEnd(CFEOperation *pFEOperation)
{
    CBECallFunction::CreateBackEnd(pFEOperation);

    string exc = string(__func__);
    // add local variables
    string sCurr = string("dice_ret_size");
    AddLocalVariable(TYPE_INTEGER, false, 4, sCurr, 0);
    // reuse pType (its been cloned)
    sCurr = string("sd");
    AddLocalVariable(TYPE_INTEGER, false, 4, sCurr, 0);

    // needed for receive
    string sInit = "sizeof(*" + 
	CCompiler::GetNameFactory()->GetCorbaObjectVariable() + ")";
    sCurr = string("dice_fromlen");
    AddLocalVariable(string("socklen_t"), sCurr, 0, sInit);
}


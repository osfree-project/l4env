/**
 *  \file    dice/src/be/l4/L4BESrvLoopFunction.cpp
 *  \brief   contains the implementation of the class CL4BESrvLoopFunction
 *
 *  \date    02/10/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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

#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BESrvLoopFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEOperationFunction.h"
#include "be/BEClass.h"
#include "be/BEMsgBuffer.h"
#include "be/BEType.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEWaitAnyFunction.h"

#include "TypeSpec-L4Types.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <cassert>

CL4BESrvLoopFunction::CL4BESrvLoopFunction()
{
}

CL4BESrvLoopFunction::CL4BESrvLoopFunction(CL4BESrvLoopFunction & src)
:CBESrvLoopFunction(src)
{
}

/** \brief destructor of target class */
CL4BESrvLoopFunction::~CL4BESrvLoopFunction()
{
}

/** \brief writes the varaible initialization
 *  \param pFile the file to write to
 */
void
CL4BESrvLoopFunction::WriteVariableInitialization(CBEFile& pFile)
{
    // call base class - initializes opcode
    CBESrvLoopFunction::WriteVariableInitialization(pFile);

    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    // zero msg buffer
    if (CCompiler::IsOptionSet(PROGRAM_ZERO_MSGBUF))
	pMsgBuffer->WriteSetZero(pFile);
    // set the size dope here, so we do not need to set it anywhere else
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SIZE, CMsgStructType::Generic);
    // init receive flexpage
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_RCV_FLEXPAGE, CMsgStructType::Generic);
    // init indirect strings (using generic struct)
    pMsgBuffer->WriteInitialization(pFile, this, TYPE_REFSTRING, CMsgStructType::Generic);

    // set CORBA_Object depending on [dedicated_partner]. Here, the
    // environment is initialized, so it is save to use the environment's
    // partner member (independent of server parameter)
    CBEClass *pClass = GetSpecificParent<CBEClass>();
    assert(pClass);
    if (pClass->m_Attributes.Find(ATTR_DEDICATED_PARTNER))
    {
       string sObj = GetObject()->m_Declarators.First()->GetName();
       CL4BENameFactory *pNF =
           static_cast<CL4BENameFactory*>(CCompiler::GetNameFactory());
       string sPartner = pNF->GetPartnerVariable();
       pFile << "\t_" << sObj << " = " << sPartner << ";\n";
    }
}

/** \brief writes the assignment of the default environment to the env var
 *  \param pFile the file to write to
 */
void
CL4BESrvLoopFunction::WriteDefaultEnvAssignment(CBEFile& pFile)
{
    CBETypedDeclarator *pEnv = GetEnvironment();
    CBEDeclarator *pDecl = pEnv->m_Declarators.First();

    if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
    {
	// *corba-env = dice_default_env;
	pFile << "\t*" << pDecl->GetName() << " = ";
	pEnv->GetType()->WriteCast(pFile, false);
	pFile << "dice_default_server_environment" << ";\n";
    }
    else if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
    {
	pFile << "\tDICE_EXCEPTION_MAJOR(" << pDecl->GetName() <<
	    ") = CORBA_NO_EXCEPTION;\n";
	pFile << "\tDICE_EXCEPTION_MINOR(" << pDecl->GetName() <<
	    ") = CORBA_DICE_EXCEPTION_NONE;\n";
	pFile << "\t" << pDecl->GetName() << "->_p.param = 0;\n";
	pFile << "\t" << pDecl->GetName() <<
	    "->timeout = L4_IPC_SEND_TIMEOUT_0;\n";
	pFile << "\t" << pDecl->GetName() << "->rcv_fpage.fp.grant = 1;\n";
	pFile << "\t" << pDecl->GetName() << "->rcv_fpage.fp.write = 1;\n";
	pFile << "\t" << pDecl->GetName() <<
	    "->rcv_fpage.fp.size = L4_WHOLE_ADDRESS_SPACE;\n";
	pFile << "\t" << pDecl->GetName() << "->rcv_fpage.fp.zero = 0;\n";
	pFile << "\t" << pDecl->GetName() << "->rcv_fpage.fp.page = 0;\n";
	pFile << "\t" << pDecl->GetName() << "->malloc = malloc_warning;\n";
	pFile << "\t" << pDecl->GetName() << "->free = free_warning;\n";
	pFile << "\t" << pDecl->GetName() << "->partner = L4_INVALID_ID;\n";
	pFile << "\t" << pDecl->GetName() << "->user_data = 0;\n";
	pFile << "\tfor (int i=0; i<DICE_PTRS_MAX; i++)\n";
	++pFile << "\t" << pDecl->GetName() << "->ptrs[i] = 0;\n";
	--pFile << "\t" << pDecl->GetName() << "->ptrs_cur = 0;\n";
    }
}

/** \brief create this instance of a server loop function
 *  \param pFEInterface the interface to use as reference
 *  \return true if create was successful
 *
 * We have to check whether the server loop might receive flexpages. If it
 * does, we have to use a server function parameter (the Corba Environment) to
 * set the receive flexpage.  To find out if we have receive flexpages, we use
 * the message buffer, but we then have to set the context option.
 */
void
CL4BESrvLoopFunction::CreateBackEnd(CFEInterface * pFEInterface)
{
    CBESrvLoopFunction::CreateBackEnd(pFEInterface);
    // test for flexpages
    CBETypedDeclarator *pEnv = GetEnvironment();
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    CMsgStructType nType = GetReceiveDirection();
    if ((pMsgBuffer->GetCount(TYPE_FLEXPAGE, nType) > 0) &&
	pEnv)
    {
        CBEDeclarator *pDecl = pEnv->m_Declarators.First();
        if (pDecl->GetStars() == 0)
            pDecl->IncStars(1);
        // set the call variables
        if (m_pWaitAnyFunction)
            m_pWaitAnyFunction->SetCallVariable(pDecl->GetName(),
                pDecl->GetStars(), pDecl->GetName());
        if (m_pReplyAnyWaitAnyFunction)
            m_pReplyAnyWaitAnyFunction->SetCallVariable(pDecl->GetName(),
                pDecl->GetStars(), pDecl->GetName());
    }
}

/** \brief writes the dispatcher invocation
 *  \param pFile the file to write to
 */
void CL4BESrvLoopFunction::WriteDispatchInvocation(CBEFile& pFile)
{
    CBESrvLoopFunction::WriteDispatchInvocation(pFile);

    // set CORBA_Object depending on [dedicated_partner]. Here, the
    // environment is initialized, so it is save to use the environment's
    // partner member (independent of server parameter)
    CBEClass *pClass = GetSpecificParent<CBEClass>();
    assert(pClass);
    if (pClass->m_Attributes.Find(ATTR_DEDICATED_PARTNER))
    {
	string sObj = GetObject()->m_Declarators.First()->GetName();
	CL4BENameFactory *pNF =
	    static_cast<CL4BENameFactory*>(CCompiler::GetNameFactory());
	string sPartner = pNF->GetPartnerVariable();
	pFile << "\tif (!l4_is_invalid_id(" << sPartner << "))\n";
	++pFile << "\t_" << sObj << " = " << sPartner << ";\n";
	--pFile;
    }
}


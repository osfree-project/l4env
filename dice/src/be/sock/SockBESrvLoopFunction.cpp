/**
 *  \file    dice/src/be/sock/SockBESrvLoopFunction.cpp
 *  \brief   contains the implementation of the class CSockBESrvLoopCallFunction
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

#include "SockBESrvLoopFunction.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "BESocket.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <cassert>

CSockBESrvLoopFunction::CSockBESrvLoopFunction()
{ }

/** \brief destructor of target class */
CSockBESrvLoopFunction::~CSockBESrvLoopFunction()
{ }

/** \brief creates the server loop function for the given interface
 *  \param pFEInterface the respective front-end interface
 *  \return true if successful
 */
void CSockBESrvLoopFunction::CreateBackEnd(CFEInterface * pFEInterface, bool bComponentSide)
{
	CBESrvLoopFunction::CreateBackEnd(pFEInterface, bComponentSide);
	CreateCommunication();
}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * Init the CORBA_Object and CORBA_Environment variables:
 * we have to open a socket (set the socket-descriptor in
 * environment), set the accepted addresses to any address,
 * and bind to socket.
 */
void CSockBESrvLoopFunction::WriteVariableInitialization(CBEFile& pFile)
{
	WriteObjectInitialization(pFile);
	// if server-parameter is given, we init CORBA_Env with it
	WriteEnvironmentInitialization(pFile);

	CBECommunication *pComm = GetCommunication();
	assert(pComm);
	pComm->WriteInitialization(pFile, this);

	string sObj = CCompiler::GetNameFactory()->GetCorbaObjectVariable();
	string sEnv =
		CCompiler::GetNameFactory()->GetCorbaEnvironmentVariable();

	// init socket address
	pFile << "\tbzero(" << sObj << ", sizeof(struct sockaddr));\n";
	pFile << "\t" << sObj << "->sin_family = AF_INET;\n";
	pFile << "\t" << sObj << "->sin_port = " << sEnv << "->srv_port;\n";
	pFile << "\t" << sObj << "->sin_addr.s_addr = INADDR_ANY;\n";

	pComm->WriteBind(pFile, this);
}

/** \brief writes the assignment of the default environment to the env var
 *  \param pFile the file to write to
 */
void CSockBESrvLoopFunction::WriteDefaultEnvAssignment(CBEFile& pFile)
{
	string sName = GetEnvironment()->m_Declarators.First()->GetName();

	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
	{
		// *corba-env = dice_default_env;
		pFile << "\t*" << sName << " = ";
		GetEnvironment()->GetType()->WriteCast(pFile, false);
		pFile << "dice_default_server_environment;\n";
	}
	else if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
	{
		pFile << "\tDICE_EXCEPTION_MAJOR(" << sName <<
			") = CORBA_NO_EXCEPTION;\n";
		pFile << "\tDICE_EXCEPTION_MINOR(" << sName <<
			") = CORBA_DICE_EXCEPTION_NONE;\n";
		pFile << "\t" << sName << "->param = 0;\n";
		pFile << "\t" << sName << "->srv_port = 9999;\n";
		pFile << "\t" << sName << "->cur_socket = -1;\n";
		pFile << "\t" << sName << "->user_data = 0;\n";
		pFile << "\t" << sName << "->malloc = ::malloc;\n";
		pFile << "\t" << sName << "->free = ::free;\n";
		pFile << "\tfor (int i=0; i<DICE_PTRS_MAX; i++)\n";
		++pFile << "\t" << sName << "->ptrs[i] = 0;\n";
		--pFile << "\t" << sName << "->ptrs_cur = 0;\n";
	}
}

/** \brief write the clean up code
 *  \param pFile the file to write to
 *
 * This should never be reached, but just in case:
 * we close the socket here. Use the socket descriptor
 * in the environment variable.
 */
void CSockBESrvLoopFunction::WriteCleanup(CBEFile& pFile)
{
	CBECommunication *pComm = GetCommunication();
	assert(pComm);
	pComm->WriteCleanup(pFile, this);
}


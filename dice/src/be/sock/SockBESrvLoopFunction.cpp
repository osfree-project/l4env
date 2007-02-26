/**
 *	\file	dice/src/be/sock/SockBESrvLoopFunction.cpp
 *	\brief	contains the implementation of the class CSockBESrvLoopCallFunction
 *
 *	\date	11/28/2002
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

#include "be/sock/SockBESrvLoopFunction.h"
#include "be/BEFile.h"
#include "be/BEContext.h"

#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CSockBESrvLoopFunction);

CSockBESrvLoopFunction::CSockBESrvLoopFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CSockBESrvLoopFunction, CBESrvLoopFunction);
}

CSockBESrvLoopFunction::CSockBESrvLoopFunction(CSockBESrvLoopFunction & src)
: CBESrvLoopFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CSockBESrvLoopFunction, CBESrvLoopFunction);
}

/**	\brief destructor of target class */
CSockBESrvLoopFunction::~CSockBESrvLoopFunction()
{

}

/**	\brief writes the variable initializations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Init the CORBA_object and CORBA_environment variables:
 * we have to open a socket (set the socket-descriptor in
 * environment), set the accepted addresses to any address,
 * and bind to socket.
 */
void CSockBESrvLoopFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // if server-parameter is given, we init CORBA_Env with it
    WriteEnvironmentInitialization(pFile, pContext);
    
    String sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    String sEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);

    bool bHasServerParameter = DoUseParameterAsEnv(pContext);

    if (bHasServerParameter)
    {
        pFile->PrintIndent("%s->cur_socket = socket(PF_INET, SOCK_DGRAM, 0);\n", (const char*)sEnv);
        pFile->PrintIndent("if (%s->cur_socket < 0)\n", (const char*)sEnv);
    }
    else
    {
        pFile->PrintIndent("%s.cur_socket = socket(PF_INET, SOCK_DGRAM, 0);\n", (const char*)sEnv);
        pFile->PrintIndent("if (%s.cur_socket < 0)\n", (const char*)sEnv);
    }
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("perror(\"socket--server\");\n");
    WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    pFile->PrintIndent("bzero(&%s, sizeof(%s));\n", (const char*)sObj, (const char*)sObj);
    pFile->PrintIndent("%s.sin_family = AF_INET;\n", (const char*)sObj);
    if (bHasServerParameter)
        pFile->PrintIndent("%s.sin_port = %s->srv_port;\n", (const char*)sObj, (const char*)sEnv);
    else
        pFile->PrintIndent("%s.sin_port = DICE_DEFAULT_PORT;\n", (const char*)sObj);
    pFile->PrintIndent("%s.sin_addr.s_addr = INADDR_ANY;\n", (const char*)sObj);
    if (bHasServerParameter)
        pFile->PrintIndent("if (bind(%s->cur_socket, (struct sockaddr*)&%s, sizeof(%s)) < 0)\n",
            (const char*)sEnv, (const char*)sObj, (const char*)sObj);
    else
        pFile->PrintIndent("if (bind(%s.cur_socket, (struct sockaddr*)&%s, sizeof(%s)) < 0)\n",
            (const char*)sEnv, (const char*)sObj, (const char*)sObj);
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("perror(\"bind\");\n");
    WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief write the clean up code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This should never be reached, but just in case:
 * we close the socket here. Use the socket descriptor
 * in the environment variable.
 */
void CSockBESrvLoopFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
    String sEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);
    if (DoUseParameterAsEnv(pContext))
        pFile->PrintIndent("close(%s->cur_socket);\n", (const char*)sEnv);
    else
        pFile->PrintIndent("close(%s.cur_socket);\n", (const char*)sEnv);
}

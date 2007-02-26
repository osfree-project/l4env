/**
 *    \file    dice/src/be/sock/SockBESrvLoopFunction.cpp
 *    \brief   contains the implementation of the class CSockBESrvLoopCallFunction
 *
 *    \date    11/28/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/sock/SockBESrvLoopFunction.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/sock/BESocket.h"
#include "Attribute-Type.h"

CSockBESrvLoopFunction::CSockBESrvLoopFunction()
{
}

CSockBESrvLoopFunction::CSockBESrvLoopFunction(CSockBESrvLoopFunction & src)
: CBESrvLoopFunction(src)
{
}

/**    \brief destructor of target class */
CSockBESrvLoopFunction::~CSockBESrvLoopFunction()
{

}

/** \brief write the declaration of the CORBA_Object variable
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CSockBESrvLoopFunction::WriteCorbaObjectDeclaration(CBEFile *pFile, CBEContext *pContext)
{
    if (m_pCorbaObject)
    {
        vector<CBEDeclarator*>::iterator iterCO = m_pCorbaObject->GetFirstDeclarator();
        CBEDeclarator *pName = *iterCO;
        pFile->PrintIndent("CORBA_Object_base _%s;\n", pName->GetName().c_str());
        pFile->PrintIndent("");
        m_pCorbaObject->WriteDeclaration(pFile, pContext);
        pFile->Print(" = (CORBA_Object)&_%s; // is client id\n", pName->GetName().c_str());
    }
}

/**    \brief writes the variable initializations of this function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * Init the CORBA_Object and CORBA_Environment variables:
 * we have to open a socket (set the socket-descriptor in
 * environment), set the accepted addresses to any address,
 * and bind to socket.
 */
void CSockBESrvLoopFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // if server-parameter is given, we init CORBA_Env with it
    WriteEnvironmentInitialization(pFile, pContext);

    m_pComm->WriteInitialization(pFile, this, pContext);

    string sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    string sEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);

    // init socket address
    pFile->PrintIndent("bzero(%s, sizeof(struct sockaddr));\n", sObj.c_str(), sObj.c_str());
    pFile->PrintIndent("%s->sin_family = AF_INET;\n", sObj.c_str());
    if (DoUseParameterAsEnv(pContext))
        pFile->PrintIndent("%s->sin_port = %s->srv_port;\n", sObj.c_str(), sEnv.c_str());
    else
        pFile->PrintIndent("%s->sin_port = DICE_DEFAULT_PORT;\n", sObj.c_str());
    pFile->PrintIndent("%s->sin_addr.s_addr = INADDR_ANY;\n", sObj.c_str());

    m_pComm->WriteBind(pFile, this, pContext);
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
    m_pComm->WriteCleanup(pFile, this, pContext);
}

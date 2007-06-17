/**
 *  \file    dice/src/be/l4/L4BEDispatchFunction.cpp
 *  \brief   contains the implementation of the class CL4BEDispatchFunction
 *
 *  \date    10/10/2003
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
#include "L4BEDispatchFunction.h"
#include "L4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"
#include <cassert>

CL4BEDispatchFunction::CL4BEDispatchFunction()
{ }

CL4BEDispatchFunction::CL4BEDispatchFunction(CL4BEDispatchFunction & src)
: CBEDispatchFunction(src)
{ }

/** \brief destructor of target class */
CL4BEDispatchFunction::~CL4BEDispatchFunction()
{ }

/** \brief writes the default case if there is no default function
 *  \param pFile the file to write to
 *
 * If we have an error function, this function has been called on IPC errors.
 * After it has been called the IPC error exception is set in the environment.
 * If this is the case, we do not have to send a reply, because this was no
 * real error.
 */
void
CL4BEDispatchFunction::WriteDefaultCaseWithoutDefaultFunc(CBEFile& pFile)
{
    CBEDeclarator *pDecl = GetEnvironment()->m_Declarators.First();
    string sEnv;
    if (pDecl->GetStars() == 0)
	sEnv = "&";
    sEnv += pDecl->GetName();

    pFile << "\tif (DICE_IS_EXCEPTION(" << sEnv << 
	", CORBA_SYSTEM_EXCEPTION) &&\n";
    ++pFile << "\t(DICE_EXCEPTION_MINOR(" << sEnv << 
	") == CORBA_DICE_INTERNAL_IPC_ERROR))\n";
    --pFile << "\t{\n";
    // clear exception
    ++pFile << "\tCORBA_server_exception_free(" << sEnv << ");\n";
    // wait for next ipc
    string sReply = CCompiler::GetNameFactory()->GetReplyCodeVariable();
    pFile << "\t" << sReply << " = DICE_NEVER_REPLY;\n";

    // finished
    --pFile << "\t}\n";
    // else: normal handling
    pFile << "\telse\n";
    pFile << "\t{\n";
    ++pFile;
    CBEDispatchFunction::WriteDefaultCaseWithoutDefaultFunc(pFile);
    --pFile;
    pFile << "\t}\n";
}


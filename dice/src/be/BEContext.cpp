/**
 * \file   dice/src/be/BEContext.cpp
 * \brief  contains the implementation of the class CBEContext
 *
 * \date   01/10/2002
 * \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/BEContext.h"
#include "BEFile.h"
#include "be/BEFunction.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "Compiler.h"
#include <string.h>

CBEContext::CBEContext()
{
}

/** CBEContext destructor */
CBEContext::~CBEContext()
{
}

/** \brief writes the actually used malloc function
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *
 * We use CORBA_alloc if it is explicetly forced, or if the function is used
 * at the client's side, or if used at the server's side only if the
 * -fserver-parameter option is set. Only then do we have an environment which
 * might contain a valid malloc member.
 *
 * Another option is to force the usage of env.malloc.
 */
void CBEContext::WriteMalloc(CBEFile* pFile, CBEFunction* pFunction)
{
    WriteMemory(pFile, pFunction, string("malloc"), string("CORBA_alloc"));
}

/** \brief writes the actual used free function
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CBEContext::WriteFree(CBEFile* pFile, CBEFunction* pFunction)
{
    WriteMemory(pFile, pFunction, string("free"), string("CORBA_free"));
}

/** \brief writes memory access function
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param sEnv the environment function
 *  \param sCorba the CORBA function
 */
void CBEContext::WriteMemory(CBEFile *pFile,
    CBEFunction *pFunction,
    string sEnv,
    string sCorba)
{
    bool bUseEnv = !CCompiler::IsOptionSet(PROGRAM_FORCE_CORBA_ALLOC) ||
	CCompiler::IsOptionSet(PROGRAM_FORCE_ENV_MALLOC);
    string sFuncName = pFunction->GetName();
    if (bUseEnv)
    {
        CBETypedDeclarator* pEnv = pFunction->GetEnvironment();
        CBEDeclarator *pDecl = (pEnv) ? pEnv->m_Declarators.First() : 0;
        if (pDecl)
        {
	    string sFree = "(" + pDecl->GetName();
            if (pDecl->GetStars())
		sFree += "->";
            else
		sFree += ".";
	    sFree += sEnv + ")";
	    *pFile << sFree;
            if (CCompiler::IsWarningSet(PROGRAM_WARNING_PREALLOC))
                CCompiler::Warning("CORBA_Environment.%s is used to set receive buffer in %s.",
		    sEnv.c_str(), sFuncName.c_str());
        }
        else
        {
            if (CCompiler::IsOptionSet(PROGRAM_FORCE_ENV_MALLOC))
                CCompiler::Warning("Using %s because function %s has no environment.",
		    sCorba.c_str(), sFuncName.c_str());
            if (CCompiler::IsWarningSet(PROGRAM_WARNING_PREALLOC))
                CCompiler::Warning("%s is used to set receive buffer in %s.",
		    sCorba.c_str(), sFuncName.c_str());
            *pFile << sCorba;
        }
    }
    else
    {
        if (CCompiler::IsWarningSet(PROGRAM_WARNING_PREALLOC))
            CCompiler::Warning("%s is used to set receive buffer in %s.",
		sCorba.c_str(), sFuncName.c_str());
        *pFile << sCorba;
    }
}


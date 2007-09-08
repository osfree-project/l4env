/**
 *  \file    dice/src/be/l4/v4/ia32/L4V4IA32IPC.cpp
 *  \brief   contains the implementation of the class CL4V4IA32IPC
 *
 *  \date    02/08/2004
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

#include "be/l4/v4/ia32/L4V4IA32IPC.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEFunction.h"
#include "be/BETypedDeclarator.h"
#include "be/l4/v4/L4V4BENameFactory.h"
#include "Compiler.h"

CL4V4IA32IPC::CL4V4IA32IPC()
 : CL4V4BEIPC()
{ }

/** destroys the IPC object */
CL4V4IA32IPC::~CL4V4IA32IPC()
{ }

/** \brief writes the IPC call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CL4V4IA32IPC::WriteCall(CBEFile& pFile, CBEFunction* pFunction)
{
    if (CCompiler::IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
    {
        CL4V4BEIPC::WriteCall(pFile, pFunction);
        return;
    }

    CL4BENameFactory *pNF = (CL4BENameFactory*)CCompiler::GetNameFactory();
    string sResult = pNF->GetResultName();
    string sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    string sMsgTag = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);

    CBEDeclarator *pObjName = pFunction->GetObject()->m_Declarators.First();;
    // Call(to, sndtimeout, rcvtimeout)
    //  = Ipc(to, to, timeouts(sndtimeout, rcvtimeout), -)
    //
    // Ipc():
    // IN:
    // to: EAX
    // timeouts: ECX
    // from-specifier: EDX
    // MR0: ESI
    // UTCB: EDI
    //
    // OUT:
    // EAX: from
    // EDI: UTCB
    // ESI: MR0
    // EBX: MR1
    // EBP: MR2
    pFile << "\tasm volatile (\n";

    // do not load to into EAX, because EAX is used
    // during mathematical calculations, such as
    // MR0 bit stuffing
    ++pFile << "\t\"mov %%edx, %%eax \\n\\t\" /* from-specifier == to */\n";
    pFile << "\t\"mov %%gs:[0], %%edi \\n\\t\"\n";
    pFile << "\t\"call Ipc \\n\\t\"\n";
    pFile << "\t\"mov %%ebx, 4(%%edi) \\n\\t\" /* save mr1 */\n";
    pFile << "\t\"mov %%ebp, 8(%%edi) \\n\\t\" /* save mr2 */\n";
    pFile << "\t: /* output */\n";
    pFile << "\t\"=S\" (" << sMsgTag << ") /* ESI */\n";
    pFile << "\t: /* input */\n";
    pFile << "\t\"d\" (" << pObjName->GetName() << "), /* EDX */\n";
    pFile << "\t\"c\" (" << sTimeout << "), /* ECX */\n";
    pFile << "\t\"S\" (" << sMsgTag << ") /* ESI */\n";
    pFile << "\t: /* clobber list */\n";
    pFile << "\t\"memory\"\n";

    --pFile << "\t);\n";
}


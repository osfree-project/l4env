/**
 *    \file    dice/src/be/l4/L4BEHeaderFile.cpp
 *    \brief   contains the implementation of the class CL4BEHeaderFile
 *
 *    \date    03/25/2002
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

#include "be/l4/L4BEHeaderFile.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEClient.h"
#include "TypeSpec-Type.h"

CL4BEHeaderFile::CL4BEHeaderFile()
{
}

CL4BEHeaderFile::CL4BEHeaderFile(CL4BEHeaderFile & src)
: CBEHeaderFile(src)
{
}

/** \brief destructor
 */
CL4BEHeaderFile::~CL4BEHeaderFile()
{

}

/** \brief write the function declaration for the init-recv string function
 *  \param pContext the context of the write operation
 *
 * This test for the global init-rcvstring option which was set with
 * -finit-rcvstring. There may also be init-rcvstring functions with
 * each interface, which have to be declared for each class.
 *
 * The init-rcvstring function is:
 * void name(int, l4_umword_t*, l4_umword_t*, CORBA_Environment);
 */
void CL4BEHeaderFile::WriteHelperFunctions(CBEContext * pContext)
{
    if (pContext->IsOptionSet(PROGRAM_INIT_RCVSTRING))
    {
        string sEnvType;
        if (IsOfFileType(FILETYPE_COMPONENT))
            sEnvType = "CORBA_Server_Environment";
        else
            sEnvType = "CORBA_Environment";
        /* if function-count is zero, then there is no extern "C" written
         * before calling this function, so write it now.
         */
        if (GetFunctionCount() == 0)
        {
            Print("#ifdef __cplusplus\n");
            Print("extern \"C\" {\n");
            Print("#endif\n\n");
        }
        string sFuncName = pContext->GetNameFactory()->GetString(STR_INIT_RCVSTRING_FUNC, pContext);
        string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
        PrintIndent("void %s(int, %s*, %s*, %s*);\n\n", sFuncName.c_str(),
                sMWord.c_str(), sMWord.c_str(), sEnvType.c_str());
        if (GetFunctionCount() == 0)
        {
            Print("#ifdef __cplusplus\n");
            Print("}\n");
            Print("#endif\n\n");
        }
    }
    CBEHeaderFile::WriteHelperFunctions(pContext);
}

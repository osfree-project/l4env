/**
 *    \file    dice/src/be/l4/L4BETestMainFunction.cpp
 *    \brief   contains the implementation of the class CL4BETestMainFunction
 *
 *    \date    04/04/2002
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

#include "be/l4/L4BETestMainFunction.h"
#include "be/BETestFunction.h"
#include "be/BEContext.h"
#include "be/BEImplementationFile.h"
#include "be/l4/L4BENameFactory.h"

CL4BETestMainFunction::CL4BETestMainFunction()
{
}

CL4BETestMainFunction::CL4BETestMainFunction(CL4BETestMainFunction & src):CBETestMainFunction
    (src)
{
}

/**    \brief destructor of target class */
CL4BETestMainFunction::~CL4BETestMainFunction()
{

}

/**    \brief do some L4 specific initialization for this server test
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation does some L4 environment initialization, such as initializing the log service.
 *
 */
void CL4BETestMainFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // print status
    pFile->Print("\n");
    pFile->PrintIndent("printf(\"test started\\n\");\n");
}

/**    \brief clean up the test application
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 */
void CL4BETestMainFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("printf(\"test stopped\\n\");\n");
    pFile->Print("\n");
    if (pContext->IsOptionSet(PROGRAM_TESTSUITE_SHUTDOWN_FIASCO))
    {
        *pFile << "\tl4_sleep(2000);\n";
        *pFile << "\tl4util_reboot();\n";
    }
}

/** \brief wrapper for writing the elements of this function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * We have to write the LOG_tag declaration here.
 */
void CL4BETestMainFunction::Write(CBEImplementationFile* pFile,  CBEContext* pContext)
{
    pFile->PrintIndent("char LOG_tag[9] = \"dice_tst\";\n\n");
    CBETestMainFunction::Write(pFile, pContext);
}

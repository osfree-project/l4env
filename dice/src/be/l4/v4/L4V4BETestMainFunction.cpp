/**
 *  \file     dice/src/be/l4/v4/L4V4BETestMainFunction.cpp
 *  \brief    contains the implementation of the class CL4V4BETestMainFunction
 *
 *  \date     Sat Jul 3 2004
 *  \author   Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004 by
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
#include "L4V4BETestMainFunction.h"
#include "be/BEFile.h"
#include "be/BEContext.h"

CL4V4BETestMainFunction::CL4V4BETestMainFunction()
 : CL4BETestMainFunction()
{
}

/** destroys objects of this class */
CL4V4BETestMainFunction::~CL4V4BETestMainFunction()
{
}

void CL4V4BETestMainFunction::WriteCleanup(CBEFile *pFile, CBEContext *pContext)
{
    *pFile << "\tprintf(\"test stopped\");\n";
    *pFile << "\n";
    if (pContext->IsOptionSet(PROGRAM_TESTSUITE_SHUTDOWN_FIASCO))
    {
        *pFile << "\tL4_Sleep(L4_TimePeriod(2000));\n";
        *pFile << "\tenter_kdebug(\"*#^test stopped\");\n";
    }
}

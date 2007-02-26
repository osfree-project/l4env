/**
 *	\file	dice/src/be/l4/L4BEHeaderFile.cpp
 *	\brief	contains the implementation of the class CL4BEHeaderFile
 *
 *	\date	03/25/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

IMPLEMENT_DYNAMIC(CL4BEHeaderFile);

CL4BEHeaderFile::CL4BEHeaderFile()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEHeaderFile, CBEHeaderFile);
}

CL4BEHeaderFile::CL4BEHeaderFile(CL4BEHeaderFile & src):CBEHeaderFile(src)
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEHeaderFile, CBEHeaderFile);
}

/**	\brief destructor
 */
CL4BEHeaderFile::~CL4BEHeaderFile()
{

}

/**	\brief write the include statements
 *	\param pContext the context of the write operation
 *
 * This implementation adds L4 specific includes
 */
void CL4BEHeaderFile::WriteIncludesBeforeTypes(CBEContext * pContext)
{
    // if testing include thread lib
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE))
    {
        Print("/* needed to create threads */\n");
        Print("#include <l4/thread/thread.h>\n");
        Print("/* needed to print error messages  */\n");
        Print("#include <l4/log/l4log.h>\n");
        Print("#include <l4/sys/kdebug.h>\n");
		if (pContext->IsOptionSet(PROGRAM_TESTSUITE_SHUTDOWN_FIASCO) &&
		    (GetTarget()->IsKindOf(RUNTIME_CLASS(CBEClient))))
		{
			Print("/* needed for l4_sleep */\n");
			Print("#include <l4/util/util.h>\n");
		}
		Print("/* needed for printf */\n");
        Print("#include <stdio.h>\n");
        Print("\n");
    }
	else if (pContext->IsOptionSet(PROGRAM_TRACE_SERVER) ||
	         pContext->IsOptionSet(PROGRAM_TRACE_CLIENT) ||
			 pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF))
	{
	    Print("#include <l4/log/l4log.h>\n");
	}
    // call base class (contains CORBA type defs)
    CBEHeaderFile::WriteIncludesBeforeTypes(pContext);
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
void CL4BEHeaderFile::WriteFunctions(CBEContext * pContext)
{
    if (pContext->IsOptionSet(PROGRAM_INIT_RCVSTRING))
    {
        String sFuncName = pContext->GetNameFactory()->GetString(STR_INIT_RCVSTRING_FUNC, pContext);
        String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
        PrintIndent("void %s(int, %s*, %s*, CORBA_Environment*);\n\n", (const char*)sFuncName,
                (const char*)sMWord, (const char*)sMWord);
    }
    CBEHeaderFile::WriteFunctions(pContext);
}

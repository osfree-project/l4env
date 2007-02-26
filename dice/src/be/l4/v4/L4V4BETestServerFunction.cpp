/**
 *  \file     dice/src/be/l4/v4/L4V4BETestServerFunction.cpp
 *  \brief    contains the implementation of the class CL4V4BETestServerFunction
 *
 *  \date     Sun Jul 4 2004
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
#include "L4V4BETestServerFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEImplementationFile.h"

CL4V4BETestServerFunction::CL4V4BETestServerFunction()
 : CL4BETestServerFunction()
{
}

/** destroys instance of this class */
CL4V4BETestServerFunction::~CL4V4BETestServerFunction()
{
}

/** \brief writes this function to the implementation file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * overloaded to declare global variables.
 */
void
CL4V4BETestServerFunction::Write(CBEImplementationFile * pFile,
    CBEContext * pContext)
{
    WriteGlobalVariableDeclaration(pFile, pContext);
    WriteWrapperFunctions(pFile, pContext);

    *pFile << "static\n";
    CBEInterfaceFunction::Write(pFile, pContext);
}

/** \brief declares global variables needed to test server
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The declared variables are taken from the pingpong example of Pistachiao.
 */
void
CL4V4BETestServerFunction::WriteGlobalVariableDeclaration(CBEFile * pFile,
    CBEContext * pContext)
{
    *pFile << "L4_ThreadId_t  master_tid, pager_tid, server_tid, client_tid;\n";
    *pFile << "\n";
    *pFile << "L4_Word_t server_stack[2048] __attribute__ ((aligned (16)));\n";
    *pFile << "L4_Word_t client_stack[2048] __attribute__ ((aligned (16)));\n";
    *pFile << "L4_Word_t pager_stack[2048] __attribute__ ((aligned (16)));\n";
    *pFile << "\n";
    *pFile << "\textern L4_Word_t _end, _start;\n";
    *pFile << "\n";
}

/**    \brief writes the test of a server
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * Slight modification of the order
 */
void
CL4V4BETestServerFunction::WriteBody(CBEFile * pFile, CBEContext * pContext)
{
    // variable declaration and initialization
    WriteVariableDeclaration(pFile, pContext);
    WriteVariableInitialization(pFile, pContext);
    // test server loop
    WriteStartServerLoop(pFile, pContext);
    WriteStartupNotification(pFile, pContext);
    WriteStopServerLoop(pFile, pContext);
    // finish
    WriteCleanup(pFile, pContext);
    WriteReturn(pFile, pContext);
}

/** \brief write some wrapper functions for the server startup and client startup
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 */
void
CL4V4BETestServerFunction::WriteWrapperFunctions(CBEFile * pFile,
    CBEContext * pContext)
{
    *pFile << "// wrapper functions used as startup functions of\n";
    *pFile << "// test threads\n";
    *pFile << "\n";

    *pFile << "// wraps server startup\n";
    *pFile << "static void __attribute__((noreturn))\n";
    *pFile << "_start_server_" << m_pFunction->GetName() << "(void)\n";
    *pFile << "{\n";
    pFile->IncIndent();
    *pFile << "\t// start server loop\n";
    *pFile << "\t" << m_pFunction->GetName() << "(NULL);\n";
    *pFile << "\n";
    *pFile << "\t// do not return\n";
    *pFile << "\tfor (;;) ;\n";
    pFile->DecIndent();
    *pFile << "}\n";
    *pFile << "\n";

    *pFile << "// wraps the client invocations\n";
    *pFile << "static void __attribute__((noreturn))\n";
    *pFile << "_start_client_" << m_pFunction->GetName() << "(void)\n";
    *pFile << "{\n";
    pFile->IncIndent();
    string sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    *pFile << "\t// aliasing for client test functions;\n";
    *pFile << "\tCORBA_Object_base _" << sObj << " = server_tid;\n";
    *pFile << "\tCORBA_Object " << sObj << " = &_" << sObj << ";\n";

    string sEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);
    *pFile << "\t// the corba client environment\n";
    *pFile << "\tCORBA_Environment " << sEnv;
    if (pContext->IsBackEndSet(PROGRAM_BE_C))
	*pFile << " = dice_default_environment";
    *pFile << ";\n";

    *pFile << "\t// call the client test functions\n";
    WriteTestFunctions(pFile, pContext);
    *pFile << "\n";
    *pFile << "\t// send master thread IPC that we are ready\n";
    *pFile << "\tL4_LoadMR ( 0, 0 );\n";
    *pFile << "\tL4_Send ( master_tid );\n";
    *pFile << "\n";
    *pFile << "\t// do not return\n";
    *pFile << "\tfor (;;) ;\n";
    pFile->DecIndent();
    *pFile << "}\n";
    *pFile << "\n";
}

/** \brief writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The declared variables are taken from the pingpong example of Pistachiao.
 */
void
CL4V4BETestServerFunction::WriteVariableDeclaration(CBEFile * pFile,
    CBEContext * pContext)
{
    *pFile << "\tL4_Word_t control;\n";
    *pFile << "\tL4_Fpage_t kip_area, utcb_area;\n";
    *pFile << "\tL4_Word_t utcb_size;\n";
    *pFile << "\tL4_KernelInterfacePage_t * kip;\n";
    *pFile << "\tL4_Word_t page_bits;\n";
    *pFile << "\tL4_Word_t pager_utcb;\n";
    *pFile << "\t// needed to touch memory\n";
    *pFile << "\tL4_Word_t *x, q;\n";
    *pFile << "\tL4_Msg_t msg;\n";

    *pFile << "\n";
}

/** \brief intialize the variables for this server test
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The initialization code is taken from the pingpong example of Pistachiao.
 */
void
CL4V4BETestServerFunction::WriteVariableInitialization(CBEFile * pFile,
    CBEContext * pContext)
{
    *pFile << "\t/* get address of kip */\n";
    *pFile << "\tkip = (L4_KernelInterfacePage_t *) L4_GetKernelInterface ();\n";
    *pFile << "\n";
    *pFile << "\t/* Find smallest supported page size. There's better at least one\n";
    *pFile << "\t * bit set. */\n";
    *pFile << "\tfor (page_bits = 0;\n";
    *pFile << "\t     !((1 << page_bits) & L4_PageSizeMask(kip));\n";
    *pFile << "\t     page_bits++);\n";
    *pFile << "\n";
    *pFile << "\t/* Size for one UTCB */\n";
    *pFile << "\tutcb_size = L4_UtcbSize (kip);\n";
    *pFile << "\n";
    *pFile << "#if defined(KIP_ADDRESS)\n";
    *pFile << "\t// Put kip in different location (e.g., to allow for small\n";
    *pFile << "\t// spaces).\n";
    *pFile << "\tkip_area = L4_FpageLog2 (KIP_ADDRESS, L4_KipAreaSizeLog2 (kip));\n";
    *pFile << "#else\n";
    *pFile << "\t// Put the kip at the same location in all AS to make sure we can\n";
    *pFile << "\t// reuse the syscall jump table.\n";
    *pFile << "\tkip_area = L4_FpageLog2 ((L4_Word_t) kip, L4_KipAreaSizeLog2 (kip));\n";
    *pFile << "#endif\n";
    *pFile << "\n";
    *pFile << "\t// Touch the memory to make sure we never get pagefaults\n";
    /* we should use l4util_touch_r() here */
    *pFile << "\tfor (x = (&_start); x < &_end; x++)\n";
    pFile->IncIndent();
    *pFile << "\tq = *(volatile L4_Word_t*) x;\n";
    pFile->DecIndent();
    *pFile << "\n";
    /* select thread IDs of server */
    *pFile << "\tmaster_tid = L4_Myself ();\n";
    *pFile << "\tserver_tid = L4_GlobalId (L4_ThreadNo (master_tid) + 2, 2);\n";
    *pFile << "\tclient_tid = L4_GlobalId (L4_ThreadNo (master_tid) + 3, 2);\n";
    *pFile << "\n";

    // start pager thread
    WriteStartPagerThread(pFile, pContext);
}

/**    \brief writes the startup code for the pager thread
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * The initialization code is take from the pingpong example of Pistachio.
 */
void
CL4V4BETestServerFunction::WriteStartPagerThread(CBEFile * pFile,
    CBEContext * pContext)
{
    *pFile << "\t// Create pager\n";
    *pFile << "\tpager_tid = L4_GlobalId (L4_ThreadNo (master_tid) + 1, 2);\n";
    *pFile << "\n";
    *pFile << "\t// VU: calculate UTCB address -- this has to be revised\n";
    *pFile << "\tpager_utcb = L4_MyLocalId().raw;\n";
    *pFile << "\tpager_utcb = (pager_utcb & ~(utcb_size - 1)) + utcb_size;\n";
    *pFile << "\tprintf(\"local id = %%x, pager UTCB = %%lx\\n\",\n";
    *pFile << "\t\tL4_MyLocalId().local.X.local_id, pager_utcb);\n";
    *pFile << "\n";
    *pFile << "\tL4_ThreadControl (pager_tid, L4_Myself (), L4_Myself (),\n";
    *pFile << "\t\tL4_Myself (), (void*)pager_utcb);\n";
    *pFile << "\tL4_Start_SpIp (pager_tid, (L4_Word_t) pager_stack + sizeof(pager_stack) - 32,\n";
    *pFile << "\t\tSTART_ADDR (pager));\n";
}

/** \brief writes the start code for one server loop
 *  \param pFile the file to write to
 *  \param pContext the context of the writes operation
 *
 * The initialization code is taken from the pingpong exmaple of Pistachio.
 */
void
CL4V4BETestServerFunction::WriteStartServerLoop(CBEFile * pFile,
    CBEContext * pContext)
{
    *pFile << "\n";
    *pFile << "\t// starting test server and client\n";
    // if test tasks: create two address spaces
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_TASK))
    {
        *pFile << "\t// testing in two address spaces\n";
        *pFile << "\tL4_ThreadControl (server_tid, server_tid, master_tid,\n";
        *pFile << "\t\tL4_nilthread, UTCB(0));\n";
        *pFile << "\tL4_ThreadControl (client_tid, client_tid, master_tid,\n";
        *pFile << "\t\tL4_nilthread, UTCB(1));\n";
        *pFile << "\tL4_SpaceControl (server_tid, 0, kip_area, utcb_area, L4_nilthread,\n";
        *pFile << "\t\t&control);\n";
        *pFile << "\tL4_SpaceControl (client_tid, 0, kip_area, utcb_area, L4_nilthread,\n";
        *pFile << "\t\t&control);\n";
        *pFile << "\tL4_ThreadControl (server_tid, server_tid, master_tid, pager_tid,\n";
        *pFile << "\t\tNOUTCB);\n";
        *pFile << "\tL4_ThreadControl (client_tid, client_tid, master_tid, pager_tid,\n";
        *pFile << "\t\tNOUTCB);\n";
    }
    // else: testing within one address space
    else
    {
        *pFile << "\tL4_ThreadControl (server_tid, server_tid, master_tid, L4_nilthread,\n";
        *pFile << "\t\tUTCB(0));\n";
        *pFile << "\tL4_SpaceControl (server_tid, 0, kip_area, utcb_area, L4_nilthread,\n";
        *pFile << "\t\t&control);\n";
        *pFile << "\tL4_ThreadControl (server_tid, server_tid, master_tid, pager_tid,\n";
        *pFile << "\t\tNOUTCB);\n";
        *pFile << "\tL4_ThreadControl (client_tid, server_tid, master_tid, pager_tid,\n";
        *pFile << "\t\tNOUTCB);\n";
    }
    *pFile << "\n";

}

/** \brief write startup notification IPC
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 */
void
CL4V4BETestServerFunction::WriteStartupNotification(CBEFile *pFile,
    CBEContext *pContext)
{
    *pFile << "\t// send startup IPC to pager\n";
    *pFile << "\t// msg has format: { SP, IP, ID }\n";
    *pFile << "\t// server\n";
    *pFile << "\tL4_MsgClear ( &msg );\n";
    *pFile << "\tL4_MsgAppendWord ( &msg, (L4_Word_t) server_stack + \n";
    *pFile << "\t                    sizeof (server_stack) - 32 );\n";
    *pFile << "\tL4_MsgAppendWord ( &msg, START_ADDR ( _start_server_" <<
        m_pFunction->GetName() << " ) );\n";
    *pFile << "\tL4_MsgAppendWord ( &msg, server_tid.raw );\n";
    *pFile << "\tL4_MsgLoad ( &msg );\n";
    *pFile << "\tL4_Send ( pager_tid ); // should use Lipc\n";

    *pFile << "\t// client\n";
    *pFile << "\tL4_MsgClear ( &msg );\n";
    *pFile << "\tL4_MsgAppendWord ( &msg, (L4_Word_t) client_stack + \n";
    *pFile << "\t                    sizeof (client_stack) - 32 );\n";
    *pFile << "\tL4_MsgAppendWord ( &msg, START_ADDR ( _start_client_" <<
        m_pFunction->GetName() << " ) );\n";
    *pFile << "\tL4_MsgAppendWord ( &msg, client_tid.raw );\n";
    *pFile << "\tL4_MsgLoad ( &msg );\n";
    *pFile << "\tL4_Send ( pager_tid ); // should use Lipc\n";
}

/** \brief writes the code to stop a server loop
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The cleanup code is taken from the pingpong example of Pistachio.
 */
void
CL4V4BETestServerFunction::WriteStopServerLoop(CBEFile * pFile,
    CBEContext * pContext)
{
    *pFile << "\t// wait for message from client to stop threads\n";
    *pFile << "\tL4_Receive (client_tid);\n";
    *pFile << "\n";
    *pFile << "\t// stop server and client\n";
    *pFile << "\tL4_ThreadControl (server_tid, L4_nilthread, L4_nilthread,\n";
    *pFile << "\t\tL4_nilthread, NOUTCB);\n";
    *pFile << "\tL4_ThreadControl (client_tid, L4_nilthread, L4_nilthread,\n";
    *pFile << "\t\tL4_nilthread, NOUTCB);\n";
    *pFile << "\n";

}

/**
 *	\file	dice/src/be/l4/L4BETestServerFunction.cpp
 *	\brief	contains the implementation of the class CL4BETestServerFunction
 *
 *	\date	04/04/2002
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

#include "be/l4/L4BETestServerFunction.h"
#include "be/BETestFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/l4/L4BENameFactory.h"

#include "TypeSpec-Type.h" // needed for TYPE_MWORD

IMPLEMENT_DYNAMIC(CL4BETestServerFunction);

CL4BETestServerFunction::CL4BETestServerFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BETestServerFunction, CBETestServerFunction);
}

CL4BETestServerFunction::CL4BETestServerFunction(CL4BETestServerFunction & src)
:CBETestServerFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CL4BETestServerFunction, CBETestServerFunction);
}

/**	\brief destructor of target class */
CL4BETestServerFunction::~CL4BETestServerFunction()
{

}

/**	\brief starts a server loop task
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * After the test server task/thread has been created the thread id of the server should
 * be in the CorbaObjectVariable() returned by Name-Factory.
 */
void CL4BETestServerFunction::WriteStartServerLoop(CBEFile * pFile, CBEContext * pContext)
{
    // print status
    pFile->PrintIndent("LOG(\"start server %s\");\n",
                       (const char *) m_pFunction->GetName());

    String sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_THREAD))
    {
        // start server as thread
        String sThread = pContext->GetNameFactory()->GetString(STR_THREAD_ID_VAR, pContext);
        pFile->PrintIndent("%s = l4thread_create(%s, 0, L4THREAD_CREATE_SYNC);\n",
                           (const char *) sThread, (const char *) m_pFunction->GetName());
        pFile->PrintIndent("*%s = l4thread_l4_id(%s);\n", (const char *) sObj, (const char *) sThread);
    }
    else if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_TASK))
    {
        // start server as task
        // start task with stack and function pointer as active! (the function returns the task-id)
        // sObj = l4_task_new(server_id, 0, /* id, prio */
        // _my_stack, m_pFunction->GetName(), _my_pager);
        String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
        pFile->PrintIndent("*%s = l4_task_new(_server_id, 0, (%s)_my_stack, (%s)%s, _my_pager);\n",
                           (const char*)sObj, (const char*)sMWord, (const char*)sMWord,
                           (const char*)m_pFunction->GetName());
        pFile->PrintIndent("if (l4_is_nil_id(*%s))\n", (const char*)sObj);
        pFile->PrintIndent("{\n");
        pFile->IncIndent();
        pFile->PrintIndent("LOG(\"Creation of server %s failed.\");\n", (const char*)m_pFunction->GetName());
        pFile->PrintIndent("return;\n");
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
        pFile->PrintIndent("else\n");
        pFile->PrintIndent("{\n");
        pFile->IncIndent();
        pFile->PrintIndent("LOG(\"Created server %s with %%x.%%x\", %s->id.task, %s->id.lthread);\n",
                           (const char*)m_pFunction->GetName(), (const char*)sObj, (const char*)sObj);
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
}

/**	\brief stops a server loop task
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * To delete a task in L4 it is transferred from active to inactive. Fiasco does this by
 * checking the pager argument. If it is NIL_ID the task is inactive (if the task existed
 * before, it is deleted).
 */
void CL4BETestServerFunction::WriteStopServerLoop(CBEFile * pFile, CBEContext * pContext)
{
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_THREAD))
    {
        String sThread = pContext->GetNameFactory()->GetString(STR_THREAD_ID_VAR, pContext);
        pFile->PrintIndent("l4thread_shutdown(%s);\n", (const char *) sThread);
    }
    else if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_TASK))
    {
        // deactivate server task
        String sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
        pFile->PrintIndent("l4_task_new(*%s, 0, 0, 0, L4_NIL_ID);\n", (const char*)sObj);
    }
    // print status
    pFile->PrintIndent("LOG(\"server %s stopped\");\n", (const char *) m_pFunction->GetName());
}

/**	\brief intialize the variables for this server test
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Because we always use the same parameter names when calling the test-functions (for all servers),
 * we have to initialize them correctly.
 */
void CL4BETestServerFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // if task, init pager and preempter variables
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_TASK))
    {
        pFile->PrintIndent("_my_pager = start_pager_thread();\n");
        // init server id (simply increment task number - we assume we are the last task created)
        pFile->PrintIndent("_server_id = l4_myself();\n");
        pFile->PrintIndent("_server_id.id.lthread = 0;\n");
        pFile->PrintIndent("while (rmgr_get_task(++(_server_id.id.task)) == -1) ;\n");
    }
}

/** \brief writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 *
 * We have to declare a l4thread_t variable, because the id returned from thread creation is not equal
 * to CORBA_Object. But because thread deletion uses it, we need it. We use one variable for all servers.
 *
 * Because we need to initialize the CORBA variables, we cannot call the base class' fucntion, which
 * simply declares them.
 *
 * \todo make name of "ignore" a name-factory string
 * \todo make "_my_id","_my_preempter","_my_pager" name-factory strings
 */
void CL4BETestServerFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_THREAD))
    {
        String sThread = pContext->GetNameFactory()->GetString(STR_THREAD_ID_VAR, pContext);
        pFile->PrintIndent("l4thread_t %s;\n", (const char *) sThread);
    }
    else if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_TASK))
    {
        pFile->PrintIndent("l4_threadid_t _my_pager, _server_id;\n");
    }

    String sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
	pFile->PrintIndent("CORBA_Object_base _%s;\n", (const char *) sObj);
    pFile->PrintIndent("CORBA_Object %s = &_%s;\n", (const char *) sObj, (const char *) sObj);

    String sEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);
    pFile->PrintIndent("CORBA_Environment %s = dice_default_environment;\n", (const char *) sEnv);
}

/** \brief writes the cleanupo code
 *  \param pFile the file to write to
 *  \param pContext the context of the operation
 */
void CL4BETestServerFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
}

/** \brief declares global variables needed to test server
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we generate code for the testsuite, we have to provide a stack for the new task (if we
 * create a task).
 *
 * \todo string _my_stack is hard-coded
 */
void CL4BETestServerFunction::WriteGlobalVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    if (pContext->IsOptionSet(PROGRAM_GENERATE_TESTSUITE_TASK))
    {
        pFile->Print("unsigned long _my_stack[2048];\n");
        pFile->Print("\n");
    }
}

/**	\brief writes the code to call a test-function
 *	\param pFile the file to write to
 *	\param pFunction the function to call
 *	\param pContext the context of the write operation
 */
void CL4BETestServerFunction::WriteTestFunction(CBEFile * pFile, CBETestFunction * pFunction, CBEContext * pContext)
{
    pFile->PrintIndent("LOG(\"*** %s start ***\");\n", (const char*)pFunction->GetName());
    CBETestServerFunction::WriteTestFunction(pFile, pFunction, pContext);
    pFile->PrintIndent("LOG(\"*** %s end ***\");\n", (const char*)pFunction->GetName());
}

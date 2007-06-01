#include <stdio.h>
#include "sensor.h"

/* Dice includes. Include path has to be set to l4/tool/dice/src in makefile 
 */
#include "be/BEContext.h"
#include "be/BETarget.h"
#include "be/BEFunction.h"
#include "be/BESrvLoopFunction.h"
#include "be/BEClass.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BENameFactory.h"
#include "be/BEFile.h"
#include "Compiler.h"

void Sensor::DefaultIncludes(CBEFile *pFile)
{
    if (!pFile->IsOfFileType(FILETYPE_COMPONENTIMPLEMENTATION))
	return;
    if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
	return;

    *pFile << "#include <l4/util/l4_macros.h>\n";
    *pFile << "#include <l4/util/util.h>\n";
    *pFile << "#include <stdio.h>\n";
}

void Sensor::VariableDeclaration(CBEFile *pFile, CBEFunction *pFunction)
{
    if (!dynamic_cast<CBESrvLoopFunction*>(pFunction))
	return;

    *pFile << "\tunsigned long _trace_loop[0x9][0x20];\n";
    *pFile << "\tunsigned long _trace_loop_diff[0x9][0x20];\n";
    *pFile << "\tunsigned long _trace_i = 0;\n";
}
    
void Sensor::InitServer(CBEFile *pFile, CBEFunction *pFunction)
{
    *pFile << "\t{\n";
    pFile->IncIndent();
    *pFile << "\tint _j, _k;\n";
    *pFile << "\tfor (_k=0; _k<0x9; _k++)\n";
    pFile->IncIndent();
    *pFile << "\tfor (_j=0; _j<0x20; _j++)\n";
    *pFile << "\t{\n";
    pFile->IncIndent();
    *pFile << "\t_trace_loop[_k][_j] = 0;\n";
    *pFile << "\t_trace_loop_diff[_k][_j] = 0;\n";
    pFile->DecIndent();
    *pFile << "\t}\n";
    pFile->DecIndent();
    pFile->DecIndent();
    *pFile << "\t}\n";
}

void Sensor::BeforeLoop(CBEFile *pFile, CBEFunction *pFunction)
{
    // have to init it here, so environment variable will be set

    // if no wait timeout is set, that is 
    // timeout == L4_IPC_TIMEOUT(0, 1, 0, 0, 15, 0)
    // then set our own timeout.
    *pFile << "\t{\n";
    pFile->IncIndent();

    *pFile << "\tl4_timeout_t _t = L4_IPC_TIMEOUT(0, 1, 0, 0, 15, 0);\n";
    string sEnv = pFunction->GetEnvironment()->m_Declarators.First()->GetName();
    *pFile << "\tif (" << sEnv << "->timeout.timeout == _t.timeout)\n";
    pFile->IncIndent();
    // 1000us : m=250 e=14
    // 2000us : m=125 e=13
    // 4000us : m=250 e=13
    // 10000us : m=156 e=12
    *pFile << "\t" << sEnv << "->timeout = L4_IPC_TIMEOUT(0, 1, 125, 13, 15, 0);\n";
    pFile->DecIndent();
    *pFile << "\telse\n";
    pFile->IncIndent();
    *pFile << "\t_trace_i = 1;\n";
    pFile->DecIndent();

    pFile->DecIndent();
    *pFile << "\t}\n";
}

void Sensor::BeforeDispatch(CBEFile *pFile, CBEFunction *pFunction)
{
    if (!pFunction->IsComponentSide())
	return;
    if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
	return;
    
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sOpcodeVar = pNF->GetOpcodeVariable();
    string sObj = pFunction->GetObject()->m_Declarators.First()->GetName();

    *pFile << "\t{\n"; // outer 1
    pFile->IncIndent();
    *pFile << "\tint op = " << sOpcodeVar << " & DICE_FID_MASK, i = " << sOpcodeVar << 
	" >> DICE_IID_BITS;\n";

    *pFile << "\tif (op > 0 && i > 0)\n";
    *pFile << "\t{\n";
    pFile->IncIndent();

    *pFile << "\tif (op < 0x20 && i < 0x9)\n";
    pFile->IncIndent();
    *pFile << "\t_trace_loop_diff[i][op]++;\n";
    pFile->DecIndent();
    *pFile << "\telse\n";
    pFile->IncIndent();
    *pFile << "\t_trace_loop_diff[0][0]++;\n";
    pFile->DecIndent();
    *pFile << "\tif (_trace_i) _trace_i++;\n";
    
    pFile->DecIndent();
    *pFile << "\t}\n";

    pFile->DecIndent();
    *pFile << "\t}\n"; // outer 1

    string sFunc;
    CCompiler::GetBackEndOption("trace-server-func", sFunc);
    CBEClass *pClass = pFunction->GetSpecificParent<CBEClass>();

    // whenever IPC timeut strikes, print stats
    *pFile << "\tif(" << sOpcodeVar << " == 0 || \n";
    *pFile << "\t   _trace_i >= 20)\n";
    *pFile << "\t{\n";
    pFile->IncIndent();

    *pFile << "\tint _j, _k, _diff = 0;\n";
    *pFile << "\tfor (_k=0; _k<0x9; _k++)\n";
    pFile->IncIndent();
    *pFile << "\tfor (_j=0; _j<0x20; _j++)\n";
    pFile->IncIndent();
    *pFile << "\tif (_trace_loop_diff[_k][_j])\n";
    *pFile << "\t{\n";
    pFile->IncIndent();

    *pFile << "\t_trace_loop[_k][_j] += _trace_loop_diff[_k][_j];\n";
    *pFile << "\t_diff += _trace_loop_diff[_k][_j];\n";
    *pFile << "\t_trace_loop_diff[_k][_j] = 0;\n";
    pFile->DecIndent();
    *pFile << "\t}\n";
    pFile->DecIndent();
    pFile->DecIndent();

    *pFile << "\tif (_diff)\n";
    pFile->IncIndent();
    *pFile << "\tfor (_k=0; _k<0x9; _k++)\n";
    pFile->IncIndent();
    *pFile << "\tfor (_j=0; _j<0x20; _j++)\n";
    pFile->IncIndent();
    *pFile << "\tif (_trace_loop[_k][_j])\n";
    pFile->IncIndent();
    *pFile << "\t" << sFunc << " (\"" << pClass->GetName() << 
	" %02x %06x in \"l4util_idfmt\" %ld\\n\", _k, _j, "
	<< "l4util_idstr(l4_myself()), _trace_loop[_k][_j]);\n";
    pFile->DecIndent();
    pFile->DecIndent();
    pFile->DecIndent();

//     pFile->DecIndent();
    *pFile << "\tif (_trace_i) _trace_i = 1; else _trace_i = 0;\n";
    pFile->DecIndent();
    *pFile << "\t}\n";
}


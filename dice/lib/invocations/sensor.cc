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

void Sensor::DefaultIncludes(CBEFile& pFile)
{
	if (!pFile.IsOfFileType(FILETYPE_COMPONENTIMPLEMENTATION))
		return;
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
		return;

	pFile << "#include <l4/util/l4_macros.h>\n";
	pFile << "#include <l4/util/util.h>\n";
	pFile << "#include <stdio.h>\n";
	pFile << "\tstatic unsigned long _trace_loop[0x9][0x20][0x20][0x20];\n";
	pFile << "\tstatic unsigned long _trace_loop_diff[0x9][0x20][0x20][0x20];\n";
}

void Sensor::VariableDeclaration(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!dynamic_cast<CBESrvLoopFunction*>(pFunction))
		return;

	pFile << "\tunsigned long _trace_i = 0, _trace_c = 0;\n";
}

void Sensor::InitServer(CBEFile& pFile, CBEFunction *pFunction)
{
	pFile << "\t{\n";
	++pFile << "\tint i, op, ta, th;\n";

	pFile << "\tfor (i=0; i<0x9; i++)\n";
	pFile << "\t{\n";

	++pFile << "\tfor (op=0; op<0x20; op++)\n";
	pFile << "\t{\n";

	++pFile << "\tfor (ta=0; ta<0x20; ta++)\n";
	pFile << "\t{\n";

	++pFile << "\tfor (th=0; th<0x20; th++)\n";
	pFile << "\t{\n";

	++pFile << "\t_trace_loop[i][op][ta][th] = 0;\n";
	pFile << "\t_trace_loop_diff[i][op][ta][th] = 0;\n";
	--pFile << "\t}\n";
	--pFile << "\t}\n";
	--pFile << "\t}\n";
	--pFile << "\t}\n";
	--pFile << "\t}\n";
}

void Sensor::BeforeLoop(CBEFile& pFile, CBEFunction *pFunction)
{
	// have to init it here, so environment variable will be set

	// if no wait timeout is set, that is
	// timeout == L4_IPC_TIMEOUT(0, 1, 0, 0, 15, 0)
	// then set our own timeout.
	pFile << "\t{\n";

	++pFile << "\tl4_timeout_t _t = L4_IPC_TIMEOUT(0, 1, 0, 0, 15, 0);\n";
	string sEnv = pFunction->GetEnvironment()->m_Declarators.First()->GetName();
	pFile << "\tif (" << sEnv << "->timeout.timeout == _t.timeout)\n";
	// 1000us : m=250 e=14
	// 2000us : m=125 e=13
	// 4000us : m=250 e=13
	// 10000us : m=156 e=12
	++pFile << "\t" << sEnv << "->timeout = L4_IPC_TIMEOUT(0, 1, 125, 13, 15, 0);\n";
	--pFile << "\telse\n";
	++pFile << "\t_trace_i = 1;\n";
	--(--pFile) << "\t}\n";
}

void Sensor::BeforeDispatch(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!pFunction->IsComponentSide())
		return;
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
		return;

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sOpcodeVar = pNF->GetOpcodeVariable();
	string sObj = pFunction->GetObject()->m_Declarators.First()->GetName();

	pFile << "\t{\n"; // outer 1
	++pFile << "\tint op = " << sOpcodeVar << " & DICE_FID_MASK, i = " << sOpcodeVar <<
		" >> DICE_IID_BITS;\n";

	pFile << "\tif (op > 0 && i > 0)\n";
	pFile << "\t{\n";

	++pFile << "\tif (op < 0x20 && i < 0x9)\n";
	++pFile << "\t_trace_loop_diff[i][op][" << sObj << "->id.task & 0x1f][" << sObj << "->id.lthread]++;\n";
	--pFile << "\telse\n";
	++pFile << "\t_trace_loop_diff[0][0][0][0]++;\n";
	--pFile << "\tif (_trace_i) _trace_i++;\n";

	--pFile << "\t}\n";
	--pFile << "\t}\n"; // outer 1

	string sFunc;
	CCompiler::GetBackEndOption("trace-server-func", sFunc);
	CBEClass *pClass = pFunction->GetSpecificParent<CBEClass>();

	// whenever IPC timeut strikes, print stats
	pFile << "\tif(" << sOpcodeVar << " == 0 || \n";
	pFile << "\t   _trace_i >= 20)\n";
	pFile << "\t{\n";

	++pFile << "\tint i, op, ta, th, _diff = 0;\n";
	pFile << "\tfor (i=0; i<0x9; i++)\n";
	++pFile << "\tfor (op=0; op<0x20; op++)\n";
	++pFile << "\tfor (ta=0; ta<0x20; ta++)\n";
	++pFile << "\tfor (th=0; th<0x20; th++)\n";
	++pFile << "\tif (_trace_loop_diff[i][op][ta][th])\n";
	pFile << "\t{\n";

	++pFile << "\t_trace_loop[i][op][ta][th] += _trace_loop_diff[i][op][ta][th];\n";
	pFile << "\t_diff += _trace_loop_diff[i][op][ta][th];\n";
	--pFile << "\t}\n";

	--(--(--(--pFile))) << "\tif (_diff)\n";
	pFile << "\t{\n";
	++pFile << "\tfor (i=0; i<0x9; i++)\n";
	++pFile << "\tfor (op=0; op<0x20; op++)\n";
	++pFile << "\tfor (ta=0; ta<0x20; ta++)\n";
	++pFile << "\tfor (th=0; th<0x20; th++)\n";
	pFile << "\t{\n";

	++pFile << "\tif (_trace_c == 10)\n";
	pFile << "\t{\n";
	++pFile << "\tif (_trace_loop[i][op][ta][th])\n";
	++pFile << "\t" << sFunc << " (\"" << pClass->GetName() <<
		" %02x %06x in \"l4util_idfmt\" from %x.%x sum %ld\\n\", i, op, "
		<< "l4util_idstr(l4_myself()), ta, th, _trace_loop[i][op][ta][th]);\n";
	--(--pFile) << "\t} else {\n";
	++pFile << "\tif (_trace_loop_diff[i][op][ta][th])\n";
	++pFile << "\t" << sFunc << " (\"" << pClass->GetName() <<
		" %02x %06x in \"l4util_idfmt\" from %x.%x diff %ld\\n\", i, op, "
		<< "l4util_idstr(l4_myself()), ta, th, _trace_loop_diff[i][op][ta][th]);\n";
	--pFile << "\t_trace_loop_diff[i][op][ta][th] = 0;\n";
	--pFile << "\t}\n";

	--pFile << "\t}\n";

	--(--(--pFile)) << "\tif (_trace_c == 10)\n";
	++pFile << "\t_trace_c = 0;\n";
	--pFile << "\telse\n";
	++pFile << "\t_trace_c++;\n";
	--(--pFile) << "\t}\n";

	pFile << "\tif (_trace_i) _trace_i = 1; else _trace_i = 0;\n";
	--pFile << "\t}\n";
}


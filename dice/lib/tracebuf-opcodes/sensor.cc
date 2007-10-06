#include <stdio.h>
#include "sensor.h"

/* Dice includes. Include path has to be set to l4/tool/dice/src in makefile
 */
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEClass.h"
#include "be/BENameFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "Compiler.h"

void Sensor::DefaultIncludes(CBEFile& pFile)
{
	if (!pFile.IsOfFileType(FILETYPE_COMPONENTIMPLEMENTATION))
		return;
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
		return;

	pFile << "#include <l4/util/l4_macros.h> /* l4util_idfmt, l4util_idstr */\n";
	pFile << "#include <l4/sys/ktrace.h> /* fiasco_tbuf_log */\n";
	pFile << "#include <stdio.h> /* snprintf */\n";
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
	CBEClass *pClass = pFunction->GetSpecificParent<CBEClass>();

	pFile << "\t{\n";
	++pFile << "\tunion {\n";
	++pFile << "\tchar msg[31];\n";
	pFile << "\tstruct {\n";
	++pFile << "\tunsigned char _pad1;\n";
	pFile << "\tunsigned long if;\n";
	pFile << "\tunsigned long op;\n";
	pFile << "\tl4_threadid_t tid;\n";
	pFile << "\tl4_threadid_t from;\n";
	pFile << "\tchar iname[14];\n";
	--pFile << "\t} entry;\n";
	--pFile << "\t} msg = { .entry: { ._pad1: 0, .if: " << sOpcodeVar << " >> DICE_IID_BITS, " <<
		".op: " << sOpcodeVar << " & DICE_FID_MASK, .tid: l4_myself(), .from: *(" << sObj <<
		"), .iname: \"" << pClass->GetName().substr(0,14) << "} };\n";
	pFile << "\tfiasco_tbuf_log(msg.msg);\n";

	--pFile << "\t}\n";
}

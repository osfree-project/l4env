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
	pFile << "#include <l4/sys/ktrace.h> /* fiasco_tbuf_log_binary */\n";
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
	string dot(".");
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		dot.clear();
	string del;
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		del = ":";
	else /* C */
		del = "=";

	pFile << "\t{\n";
	++pFile << "\tint i;\n";
	pFile << "\tunion __attribute__((packed)) {\n";
	++pFile << "\tunsigned char msg[31];\n";
	pFile << "\tstruct __attribute__((packed)) {\n";
	++pFile << "\tunsigned char _pad1;\n";
	pFile << "\tunsigned long if_id;\n";
	pFile << "\tunsigned long op;\n";
	pFile << "\tl4_threadid_t tid;\n";
	pFile << "\tl4_threadid_t from;\n";
	pFile << "\tchar iname[14];\n";
	--pFile << "\t} entry;\n";
	--pFile << "\t} msg;\n";
	pFile << "\tfor (i=0; i<31; i++)\n";
	++pFile << "\tmsg.msg[i] = 0;\n";
	--pFile << "\tmsg.entry._pad1" << del << "0;\n";
	pFile << "\tmsg.entry.if_id" << del << " (" << sOpcodeVar << " >> DICE_IID_BITS) & 0xfff;\n";
	pFile << "\tmsg.entry.op" << del << " " << sOpcodeVar << " & DICE_FID_MASK;\n";
	pFile << "\tmsg.entry.tid" << del << " l4_myself();\n";
	pFile << "\tmsg.entry.from" << del << " *(" << sObj <<	");\n";
	pFile << "\tstrncpy(msg.entry.iname, \"" << pClass->GetName().substr(0,14) << "\", 14);\n";
	pFile << "\tfiasco_tbuf_log_binary(msg.msg);\n";

	--pFile << "\t}\n";
}

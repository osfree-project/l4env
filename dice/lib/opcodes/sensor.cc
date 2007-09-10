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
    string sFunc;
    CCompiler::GetBackEndOption("trace-server-func", sFunc);
    CBEClass *pClass = pFunction->GetSpecificParent<CBEClass>();

    pFile << "\t{\n"; // outer 1
    ++pFile << "\tint op = " << sOpcodeVar << " & DICE_FID_MASK, i = " << sOpcodeVar << 
	" >> DICE_IID_BITS;\n";

    pFile << "\t" << sFunc << " (\"" << pClass->GetName() << 
	" %02x %06x in \"l4util_idfmt\" from \"l4util_idfmt\"\\n\", i, op, "
	<< "l4util_idstr(l4_myself()), l4util_idstr(*" << sObj << "));\n";

    --pFile << "\t}\n";
}


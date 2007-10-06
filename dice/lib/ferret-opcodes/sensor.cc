#include <stdio.h>
#include "sensor.h"

/* Dice includes. Include path has to be set to l4/tool/dice/src in makefile
 */
#include "be/BEFile.h"
#include "be/BEFunction.h"
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

	pFile << "#include <stdlib.h>\n";
	pFile << "#include <l4/ferret/client.h>\n";
	pFile << "#include <l4/ferret/clock.h>\n";
	pFile << "#include <l4/ferret/types.h>\n";
	pFile << "#include <l4/ferret/util.h>\n";
	pFile << "#include <l4/ferret/maj_min.h>\n";
	pFile << "#include <l4/ferret/sensors/list_producer.h>\n";
	pFile << "#include <l4/ferret/sensors/list_producer_wrap.h>\n";


	pFile << "static ferret_list_local_t *_trace_list = 0;\n";
}

void Sensor::BeforeLoop(CBEFile& pFile, CBEFunction* /*pFunction*/)
{
	pFile << "\tferret_create( FERRET_DICE_MAJOR, FERRET_DICE_MINOR, 0 /* instance */,\n";
	++pFile << "\tFERRET_LIST /* type */, 0 /* flags */,\n";
	pFile << "\t\"16:10000\" /* config */, _trace_list /* addr */, &malloc /* alloc */);\n";
	--pFile;
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

	pFile << "\tferret_list_post_1t1w(_trace_list, FERRET_DICE_MAJOR, FERRET_DICE_MINOR, 0,\n";
	++pFile << "\t*" << sObj << ", " << sOpcodeVar << ");\n";
}

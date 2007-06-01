#include <stdio.h>
#include "sensor.h"

/* Dice includes. Include path has to be set to l4/tool/dice/src in makefile 
 */
#include "be/BECallFunction.h"
#include "be/BESndFunction.h"
#include "be/BEMsgBuffer.h"
#include "be/MsgStructType.h"
#include "TypeSpec-Type.h"
#include "be/BEFile.h"

void Sensor::BeforeCall(CBEFile *pFile, CBEFunction *pFunction)
{
    if (!dynamic_cast<CBECallFunction*>(pFunction) &&
	!dynamic_cast<CBESndFunction*>(pFunction))
	return;

    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    int nSndWords = pMsgBuffer->GetMemberSize(TYPE_MWORD, pFunction, 
	CMsgStructType::In, true);
    int nSndStr = pMsgBuffer->GetMemberSize(TYPE_REFSTRING, pFunction,
	CMsgStructType::In, true);
    int nRcvWords = pMsgBuffer->GetMemberSize(TYPE_MWORD, pFunction,
	CMsgStructType::Out, true);
    int nRcvStr = pMsgBuffer->GetMemberSize(TYPE_REFSTRING, pFunction,
	CMsgStructType::Out, true);

    *pFile << "\t/* MsgSize " << pFunction->GetOpcodeConstName() << " " <<
	nSndWords << " " << nSndStr << " " << nRcvWords << " " << nRcvStr << " */\n";
}


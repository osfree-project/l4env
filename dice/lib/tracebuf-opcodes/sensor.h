#ifndef __DICE_TESTSUITE_LIBTRACE_SENSOR_H__
#define __DICE_TESTSUITE_LIBTRACE_SENSOR_H__

#include "be/Trace.h"

class Sensor : public CTrace
{
public:
    Sensor() {}
    ~Sensor() {}

    virtual void DefaultIncludes(CBEFile& pFile);
    virtual void BeforeDispatch(CBEFile& pFile, CBEFunction *pFunction);
    /* empty */
    virtual void VariableDeclaration(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void InitServer(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void AddLocalVariable(CBEFunction *pFunction) {}
    virtual void BeforeLoop(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void BeforeMarshalling(CBEFile& , CBEFunction*) {}
    virtual void AfterMarshalling(CBEFile& , CBEFunction*) {}
    virtual void BeforeUnmarshalling(CBEFile& , CBEFunction*) {}
    virtual void AfterUnmarshalling(CBEFile& , CBEFunction*) {}
    virtual void BeforeCall(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void AfterCall(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void AfterDispatch(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void BeforeReplyWait(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void AfterReplyWait(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void BeforeReplyOnly(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void AfterReplyOnly(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void BeforeComponent(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void AfterComponent(CBEFile& pFile, CBEFunction *pFunction) {}
    virtual void WaitCommError(CBEFile& , CBEFunction*) {}
};

#endif /* __DICE_TESTSUITE_LIBTRACE_SENSOR_H__ */

#ifndef XFUNCTIONGENERATOR_H
#define XFUNCTIONGENERATOR_H

#include "xBase.h"
#include "xFunctionGeneratorAsset.h"

class xFunctionGenerator : public xBase
{
public:
    xFunctionGenerator(const xFunctionGeneratorAsset* asset);

    void Update(F32 dt);
    void HandleEvent(xBase*, U32 toEvent, const F32* toParam, xBase*, U32);
    void Start();
    void Stop();
    void Reset();

#ifdef DEBUGRELEASE
    void AddTweaks(const char* base);
#endif

    bool IsStarted() const { return started; }
    bool IsEnabled() const { return (baseFlags & k_XBASE_IS_ENABLED) && started; }
    const xFunctionGeneratorAsset* GetAsset() const { return asset; }
    
private:
    const xFunctionGeneratorAsset* asset;
    bool started;
    bool currentState;
    bool override;

#ifdef DEBUGRELEASE
    const char* graphName;
#endif

    F32 currentCycleWidth;
    F32 currentPulseWidth;
    F32 currentTime;
    F32 lastUpTime;
    F32 lastDownTime;
};

void xFunctionGenerator_Init(xBase& data, xDynAsset& asset, size_t);

#endif

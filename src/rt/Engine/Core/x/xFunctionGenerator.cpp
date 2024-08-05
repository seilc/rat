#include "xFunctionGenerator.h"

#include "xDebugTweak.h"
#include "zScene.h"
#include "xGlobals.h"
#include "zEvent.h"
#include "xDebugGraph.h"

namespace {

void FunctionGeneratorEventWrapper(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    ((xFunctionGenerator*)to)->HandleEvent(from, toEvent, toParam, toParamWidget, toParamWidgetID);
}

#ifdef DEBUGRELEASE
void FunctionGeneratorExpandTweaks(const tweak_info& info)
{
    xFunctionGenerator* fg = (xFunctionGenerator*)info.context;

    char name[128];
    sprintf(name, "Widgets|Function Generator|%s", xSceneID2Name(xglobals->sceneCur, fg->GetAsset()->id));

    fg->AddTweaks(name);
}

void FunctionGeneratorStartTweak(const tweak_info& info)
{
    xFunctionGenerator* fg = (xFunctionGenerator*)info.context;

    if (fg->IsStarted()) {
        fg->Stop();
    }

    fg->Start();
}

void FunctionGeneratorStopTweak(const tweak_info& info)
{
    xFunctionGenerator* fg = (xFunctionGenerator*)info.context;

    fg->Stop();
}
#endif

}

xFunctionGenerator::xFunctionGenerator(const xFunctionGeneratorAsset* asset)
{
    this->asset = asset;

    xBaseInit(this, asset);

    eventFunc = FunctionGeneratorEventWrapper;
    link = (xLinkAsset*)((U8*)asset + sizeof(xFunctionGeneratorAsset));
    started = false;
    currentState = false;

#ifdef DEBUGRELEASE
    graphName = xSceneID2Name(xglobals->sceneCur, asset->id);

    char name[128];
    sprintf(name, "Widgets|Function Generator|%s", xSceneID2Name(xglobals->sceneCur, asset->id));

    static tweak_callback cb = tweak_callback::create_expand(FunctionGeneratorExpandTweaks, NULL);
    xTWEAKBRANCH(name, &cb, this, 0x2);
#endif
}

void xFunctionGenerator::Update(F32 dt)
{
    if (!IsEnabled()) return;

    currentTime += dt;

    if (!override) {
        if (asset->middleEnabled) {
            if (currentTime < asset->middleTime) {
                F32 t = currentTime / asset->middleTime;
                currentCycleWidth = asset->startCycleWidth + t * (asset->middleCycleWidth - asset->startCycleWidth);
                currentPulseWidth = asset->startPulseWidth + t * (asset->middlePulseWidth - asset->startPulseWidth);
            } else {
                currentCycleWidth = asset->middleCycleWidth;
                currentPulseWidth = asset->middlePulseWidth;
            }
        }
        if (asset->endEnabled) {
            if (currentTime < asset->endTime) {
                if (!asset->middleEnabled) {
                    F32 t = currentTime / asset->endTime;
                    currentCycleWidth = asset->startCycleWidth + t * (asset->endCycleWidth - asset->startCycleWidth);
                    currentPulseWidth = asset->startPulseWidth + t * (asset->endPulseWidth - asset->startPulseWidth);
                } else if (currentTime > asset->middleTime) {
                    F32 t = (currentTime - asset->middleTime) / (asset->endTime - asset->middleTime);
                    currentCycleWidth = asset->middleCycleWidth + t * (asset->endCycleWidth - asset->middleCycleWidth);
                    currentPulseWidth = asset->middlePulseWidth + t * (asset->endPulseWidth - asset->middlePulseWidth);
                }
            } else {
                currentCycleWidth = asset->endCycleWidth;
                currentPulseWidth = asset->endPulseWidth;
            }
        }
    }

    bool firedAny;
    do {
        firedAny = false;

        if (!currentState) {
            F32 currentOffWidth = currentCycleWidth - currentPulseWidth;
            xASSERT(122, currentOffWidth > 0.0f);

            if (currentTime - lastDownTime > currentOffWidth) {
                currentState = true;
                firedAny = true;
                zEntEvent(this, this, eEventTrue);
                if (!IsEnabled()) return;
                lastUpTime = lastDownTime + currentOffWidth;
            }
        }

        if (currentState) {
            if (currentTime - lastUpTime > currentPulseWidth) {
                currentState = false;
                firedAny = true;
                zEntEvent(this, this, eEventFalse);
                if (!IsEnabled()) return;
                lastDownTime = lastUpTime + currentPulseWidth;
            }
        }
    } while (firedAny);
    
#ifdef DEBUGRELEASE
    xDebugGraphAdd(graphName, currentState ? 1 : 0);
    xDebugGraphSetOriginValue(graphName, 0.5f);
    xDebugGraphSetScale(graphName, 1.0f);
#endif

    if (asset->endEnabled && currentTime >= asset->endTime) {
        zEntEvent(this, this, eEventExpired);
        started = false;
    }
}

void xFunctionGenerator::HandleEvent(xBase*, U32 toEvent, const F32* toParam, xBase*, U32)
{
    switch (toEvent) {
    case eEventEnable:
        xBaseEnable(this);
        break;
    case eEventDisable:
        xBaseDisable(this);
        break;
    case eEventReset:
    case eEventDebugReset:
        Reset();
        break;
    case eEventStart:
        Start();
        break;
    case eEventStop:
        Stop();
        break;
    case eEventOverrideFreqency:
        override = true;
        currentCycleWidth = toParam[0];
        currentPulseWidth = toParam[1];
        break;
    case eEventResetFrequency:
        override = false;
        break;
    }
}

void xFunctionGenerator::Start()
{
    currentCycleWidth = asset->startCycleWidth;
    currentPulseWidth = asset->startPulseWidth;
    currentTime = 0.0f;
    lastUpTime = 0.0f;
    lastDownTime = 0.0f;
    currentState = false;
    started = true;
    override = false;
}

void xFunctionGenerator::Stop()
{
    started = false;
}

void xFunctionGenerator::Reset()
{
    started = false;
}

#ifdef DEBUGRELEASE
void xFunctionGenerator::AddTweaks(const char* base)
{
    char buffer[128];
    xFunctionGeneratorAsset* ncasset = (xFunctionGeneratorAsset*)asset;

    sprintf(buffer, "%s|Start Cycle Width", base);
    xTWEAK(buffer, &ncasset->startCycleWidth, 0.00000001f, 1000000.0f, NULL, NULL, 0x2);

    sprintf(buffer, "%s|Middle Cycle Width", base);
    xTWEAK(buffer, &ncasset->middleCycleWidth, 0.00000001f, 1000000.0f, NULL, NULL, 0x2);

    sprintf(buffer, "%s|End Cycle Width", base);
    xTWEAK(buffer, &ncasset->endCycleWidth, 0.00000001f, 1000000.0f, NULL, NULL, 0x2);

    sprintf(buffer, "%s|Start Pulse Width", base);
    xTWEAK(buffer, &ncasset->startPulseWidth, 0.0f, 1000000.0f, NULL, NULL, 0x2);

    sprintf(buffer, "%s|Middle Pulse Width", base);
    xTWEAK(buffer, &ncasset->middlePulseWidth, 0.0f, 1000000.0f, NULL, NULL, 0x2);

    sprintf(buffer, "%s|End Pulse Width", base);
    xTWEAK(buffer, &ncasset->endPulseWidth, 0.0f, 1000000.0f, NULL, NULL, 0x2);

    sprintf(buffer, "%s|Middle Time", base);
    xTWEAK(buffer, &ncasset->middleTime, 0.0f, 1000000.0f, NULL, NULL, 0x2);

    sprintf(buffer, "%s|End Time", base);
    xTWEAK(buffer, &ncasset->endTime, 0.0f, 1000000.0f, NULL, NULL, 0x2);

    sprintf(buffer, "%s|Middle Enabled", base);
    xTWEAKBOOL(buffer, &ncasset->middleEnabled, NULL, NULL, 0x2);

    sprintf(buffer, "%s|End Enabled", base);
    xTWEAKBOOL(buffer, &ncasset->endEnabled, NULL, NULL, 0x2);

    sprintf(buffer, "%s|Override", base);
    xTWEAKBOOL(buffer, &override, NULL, NULL, 0x2);

    sprintf(buffer, "%s|Current Cycle Width", base);
    xTWEAK(buffer, &currentCycleWidth, 0.0f, 1000000000.0f, NULL, NULL, 0x2);

    sprintf(buffer, "%s|Current Pulse Width", base);
    xTWEAK(buffer, &currentPulseWidth, 0.0f, 1000000000.0f, NULL, NULL, 0x2);

    sprintf(buffer, "%s|Current Time", base);
    xTWEAK(buffer, &currentTime, 0.0f, 1000000000.0f, NULL, NULL, 0x3);

    sprintf(buffer, "%s|Current State", base);
    xTWEAKBOOL(buffer, &currentState, NULL, NULL, 0x3);

    sprintf(buffer, "%s|Current Started", base);
    xTWEAKBOOL(buffer, &started, NULL, NULL, 0x3);

    sprintf(buffer, "%s|Start", base);
    static tweak_callback cb1 = tweak_callback::create_change(FunctionGeneratorStartTweak);
    xTWEAKMESSAGE(buffer, "Go!", &cb1, this, 0x2);

    sprintf(buffer, "%s|Stop", base);
    static tweak_callback cb2 = tweak_callback::create_change(FunctionGeneratorStopTweak);
    xTWEAKMESSAGE(buffer, "Go!", &cb2, this, 0x2);
}
#endif

void xFunctionGenerator_Init(xBase& data, xDynAsset& asset, size_t)
{
    new (&data) xFunctionGenerator((const xFunctionGeneratorAsset*)&asset);
}

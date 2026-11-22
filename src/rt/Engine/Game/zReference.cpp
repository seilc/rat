#include "xCommon.h"
#include "zReference.h"

#include "containers.h"
#include "xDebugTweak.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xPad.h"
#include "xScene.h"
#include "zEvent.h"
#include "zScene.h"

namespace {

#if DEBUG || RELEASE
void ReferenceMemToTweak(const tweak_info& info, void* value)
{
    const char** valueStr = (const char**)value;
    zReference* context = (zReference*)info.context;
    if (context->current == NULL)
        *valueStr = "<null>";
    else
        *valueStr = xSceneID2Name(xglobals->sceneCur, context->current->id);
}
#endif

}

void zReferenceInit(xBase& data, xDynAsset& asset, size_t)
{
    zReferenceInit((zReference*)&data, (zReferenceAsset*)&asset);
}

void zReferenceInit(zReference* t, zReferenceAsset* a)
{
    xBaseInit(t, a);

    t->current = NULL;
    if (a->initial) {
        t->current = zSceneFindObject(a->initial);
        xASSERT_L(55, t->current != t);
    }

    t->eventFunc = zReferenceEventCB;

#if DEBUG || RELEASE
    char buffer[128];
    sprintf(buffer, "Widgets|Reference|%s|Value", xSceneID2Name(xglobals->sceneCur, a->id));
    static tweak_callback cb = tweak_callback::create_converter(ReferenceMemToTweak, NULL);
    xTWEAK_MESSAGE(buffer, "", &cb, t, 0x3);
#endif
}

void zReferenceEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID)
{
    zReference* ref = (zReference*)to;

    if (toEvent == eEventSetReference) {
        xASSERT_L(72, ref != toParamWidget);
        ref->current = toParamWidget;
    } else if (toEvent == eEventCopyReference) {
        xASSERT_L(75, toParamWidget->baseType == to->baseType);
        ref->current = ((zReference*)toParamWidget)->current;
    } else if (ref->current && ref->current->eventFunc) {
        zEntEvent(from, ref->current, toEvent, toParam, toParamWidget, toParamWidgetID);
    }
}

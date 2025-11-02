#include "xRenderState.h"

#include "xMath.h"
#include "xDebug.h"

#include <dolphin.h>

extern "C" {
extern void _rwDlRenderStateSetZCompLoc(RwBool zBeforeTex);
extern void RwGameCubeSetAlphaCompare(RwInt32 comp0, RwUInt8 ref0, RwInt32 op, RwInt32 comp1, RwUInt8 ref1);
}

#undef RwCameraBeginUpdate

RwReal old_near_plane = -HUGE;
RwReal old_far_plane = -HUGE;

void xRenderSceneEnter()
{
}

void xRenderSceneExit()
{
}

void xRenderStateSetZBias(RwReal bias)
{
    xASSERT(182, (old_near_plane == -HUGE) && (old_far_plane == -HUGE));

    RwCamera* cam = RwCameraGetCurrentCamera();
    xASSERT(184, cam != 0);
    if (!cam) return;

// Fakematch?
#ifdef DEBUGRELEASE
    static RwReal bias_scale = 0.005f;
#else
    static const RwReal bias_scale = 0.005f;
#endif
    bias *= bias_scale;

    old_near_plane = RwCameraGetNearClipPlane(cam);
    old_far_plane = RwCameraGetFarClipPlane(cam);

    RwCameraEndUpdate(cam);
    RwCameraSetNearClipPlane(cam, bias + old_near_plane);
    RwCameraSetFarClipPlane(cam, bias + old_far_plane);
    RwCameraBeginUpdate(cam);
}

void xRenderStateResetZBias()
{
    xASSERT(202, (old_near_plane != -HUGE) && (old_far_plane != -HUGE));

    RwCamera* cam = RwCameraGetCurrentCamera();
    xASSERT(204, cam != 0);
    if (!cam) return;

    RwCameraEndUpdate(cam);
    RwCameraSetNearClipPlane(cam, old_near_plane);
    RwCameraSetFarClipPlane(cam, old_far_plane);
    RwCameraBeginUpdate(cam);

    old_near_plane = old_far_plane = -HUGE;
}

static void* old_alpha_discard_value_gc;

void xRenderStateSetAlphaDiscard(RwInt32 value)
{
    RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &old_alpha_discard_value_gc);

    RwGameCubeSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, value);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_GEQUAL, value);

    if (value > 0) {
        _rwDlRenderStateSetZCompLoc(FALSE);
        GXSetZCompLoc(GX_FALSE);
    } else {
        _rwDlRenderStateSetZCompLoc(TRUE);
        GXSetZCompLoc(GX_TRUE);
    }
}

void xRenderStateResetAlphaDiscard()
{
    xRenderStateSetAlphaDiscard(0);
}

void xRenderFixUntexturedBegin(RpAtomic*)
{
}

void xRenderFixUntexturedEnd(RpAtomic*)
{
}

void xRenderFixIMBegin()
{
}

void xRenderFixIMEnd()
{
}

#ifdef DEBUGRELEASE
RwBool xRenderStateCheckBlendModes(RwBlendFunction desiredSrcBlend, RwBlendFunction desiredDestBlend)
{
    RwBlendFunction currentSrcBlend, currentDestBlend;
    xRenderStateBlendModesGet(&currentSrcBlend, &currentDestBlend);

    if (currentSrcBlend != desiredSrcBlend) {
        return TRUE;
    }

    if (currentDestBlend != desiredDestBlend) {
        xASSERTM(1466, 0, "Unexpected SRCBLEND.\n"
                            "Expected: 0x%08X Actual: 0x%08X", desiredDestBlend, currentDestBlend);
        return TRUE;
    }

    return FALSE;
}

RwBool xRenderStateCheckState(RwRenderState state, void* desiredValue)
{
    if (state == rwRENDERSTATESRCBLEND || state == rwRENDERSTATEDESTBLEND) {
        xFAILM(1478, "%s", "Please do not use RwRenderStateSet for SRCBLEND, DESTBLEND.\n"
                                     "Use xRenderStateCheckBlendModes.");
        return TRUE;
    }

    RwUInt32 curValue;
    RwRenderStateGetMacro(state, (void*)&curValue);

    if (curValue != (RwUInt32)desiredValue) {
        static RwBool shhhh = FALSE;
        if (shhhh) {
            iprintf("Unexpected Render State."
                    "State %d Expected: 0x%08X Actual: 0x%08X", state, desiredValue, curValue);
        } else {
            xASSERTM(1495, 0, "Unexpected Render State.\n"
                                "State %d Expected: 0x%08X Actual: 0x%08X", state, desiredValue, curValue);
        }
        return TRUE;
    }

    return FALSE;
}

void RwRenderStateSetWrapper(RwRenderState state, void* value)
{
    if (state == rwRENDERSTATESRCBLEND ||
        state == rwRENDERSTATEDESTBLEND ||
        state == rwRENDERSTATEZWRITEENABLE ||
        state == rwRENDERSTATEZTESTENABLE) {
        xFAILM(1606, "%s", "Please do not use RwRenderStateSet for SRCBLEND, DESTBLEND, ZWRITE and ZTEST.\n"
                                     "Use xRenderStateBlendModesSet, xRenderStateZModesSet or xRenderStateBlendAndZModesSet.");
        return;
    }

    RwBool ret = RwRenderStateSetMacro(state, value);
    if (!ret && state != rwRENDERSTATESRCBLEND) {
        RwInt32 asdf = 1234;
    }
}

RwBool RwRenderStateGetWrapper(RwRenderState state, void* value)
{
    if (state == rwRENDERSTATESRCBLEND || state == rwRENDERSTATEDESTBLEND) {
        xFAILM(1630, "%s", "Please do not use RwRenderStateGet for SRCBLEND and DESTBLEND.\n"
                                     "Use xRenderStateBlendModesGet.");
        return FALSE;
    }

    RwBool ret = RwRenderStateGetMacro(state, value);
    if (!ret) {
        RwInt32 asdf = 1234;
    }
    return ret;
}
#endif

void xRenderStateBlendModesSet(RwBlendFunction srcBlend, RwBlendFunction destBlend)
{
    RwBool ret;

    ret = RwRenderStateSetMacro(rwRENDERSTATESRCBLEND, (void*)srcBlend);
    if (!ret) {
        xFAILM(2160, "Unable to set rwRENDERSTATESRCBLEND with value %d", srcBlend);
    }

    ret = RwRenderStateSetMacro(rwRENDERSTATEDESTBLEND, (void*)destBlend);
    if (!ret) {
        xFAILM(2166, "Unable to set rwRENDERSTATEDESTBLEND with value %d", destBlend);
    }
}

void xRenderStateZModesSet(RwBool ZWrite, RwBool ZTest)
{
    RwBool ret;

    ret = RwRenderStateSetMacro(rwRENDERSTATEZWRITEENABLE, (void*)ZWrite);
    if (!ret) {
        xFAILM(2176, "Unable to set rwRENDERSTATEZWRITEENABLE with value %d", ZWrite);
    }

    ret = RwRenderStateSetMacro(rwRENDERSTATEZTESTENABLE, (void*)ZTest);
    if (!ret) {
        xFAILM(2182, "Unable to set rwRENDERSTATEZTESTENABLE with value %d", ZTest);
    }
}

void xRenderStateBlendAndZModesSet(RwBlendFunction srcBlend, RwBlendFunction destBlend, RwBool ZWrite, RwBool ZTest)
{
    RwBool ret;

    ret = RwRenderStateSetMacro(rwRENDERSTATESRCBLEND, (void*)srcBlend);
    if (!ret) {
        xFAILM(2193, "Unable to set rwRENDERSTATESRCBLEND with value %d", srcBlend);
    }

    ret = RwRenderStateSetMacro(rwRENDERSTATEDESTBLEND, (void*)destBlend);
    if (!ret) {
        xFAILM(2199, "Unable to set rwRENDERSTATEDESTBLEND with value %d", destBlend);
    }

    ret = RwRenderStateSetMacro(rwRENDERSTATEZWRITEENABLE, (void*)ZWrite);
    if (!ret) {
        xFAILM(2205, "Unable to set rwRENDERSTATEZWRITEENABLE with value %d", ZWrite);
    }

    ret = RwRenderStateSetMacro(rwRENDERSTATEZTESTENABLE, (void*)ZTest);
    if (!ret) {
        xFAILM(2211, "Unable to set rwRENDERSTATEZTESTENABLE with value %d", ZTest);
    }
}

void xRenderStateBlendModesGet(RwBlendFunction* srcBlend, RwBlendFunction* destBlend)
{
    RwBool ret;

    if (srcBlend) {
        ret = RwRenderStateGetMacro(rwRENDERSTATESRCBLEND, (void*)srcBlend);
        if (!ret) {
            xFAILM(2225, "Unable to get rwRENDERSTATESRCBLEND with value %d", srcBlend);
        }
    }

    if (destBlend) {
        ret = RwRenderStateGetMacro(rwRENDERSTATEDESTBLEND, (void*)destBlend);
        if (!ret) {
            xFAILM(2234, "Unable to get rwRENDERSTATEDESTBLEND with value %d", destBlend);
        }
    }
}

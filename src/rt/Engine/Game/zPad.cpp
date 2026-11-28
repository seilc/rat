#include "xCommon.h"
#include "zPad.h"

#include "containers.h"
#include "xFont.h"
#include "xTRC.h"
#include "zGameState.h"
#include "zGlobals.h"
#include "zScene.h"

#include "decomp.h"

static zPad pads[zPad::MAX_PADS];
static zPad* debugPad;
static zPad* dualControlPad;

void zPadsReleaseDebugAndDual()
{
    if (debugPad) {
        debugPad->ReleasePad();
        debugPad = NULL;
    }

    if (dualControlPad) {
        dualControlPad->ReleasePad();
        dualControlPad = NULL;
    }
}

void zPadsInit()
{
    for (S32 i = 0; i < zPad::MAX_PADS; i++) {
        pads[i].DeactivateTRC();
        pads[i].Init();
    }

    zPadsReleaseDebugAndDual();
}

zPad::zPad()
    : used(false)
    , enable(false)
    , triggered(false)
    , key1(0)
    , key2(0)
    , padPort(-1)
    , analog1(this, 1)
    , analog2(this, 2)
{
}

void zPad::Init()
{
    padPort = -1;
    used = false;
    enable = false;
    triggered = false;
    key1 = 0;
    key2 = 0;
}

bool zPad::IsPadAtPort(S32 port, S32& arrayIndex)
{
    for (S32 j = 0; j < MAX_PADS; j++) {
        if (pads[j].used && port == pads[j].padPort) {
            arrayIndex = j;
            return true;
        }
    }

    arrayIndex = -1;
    return false;
}

zPad* zPad::GetFirstUnused()
{
    for (S32 i = 0; i < MAX_PADS; i++) {
        if (!pads[i].used)
            return &pads[i];
    }

    xFAIL_MSG_L(101, "Ran out of virtual pads, this is bad.");
    return NULL;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("arrayIndex > -1 && arrayIndex < MAX_PADS")
#endif

zPad* zPad::CreatePadAtPort(S32 port, bool isAI)
{
    zPad* firstUnusedPad = GetFirstUnused();

    if (firstUnusedPad) {
        firstUnusedPad->enable = true;
        firstUnusedPad->used = true;
        firstUnusedPad->padPort = port;

        if (!isAI || (!isAI && zGameStateGet() != eGameState_FirstTime)) {
            if (!xTRCPadPortWasActive(port)) {
                xTRCPadSetStatePadPort(port, TRC_PadActive);
                xTRCPadReOrder();
            }

            xPadUpdate(port, 0.0f);
            xPadEnable(port);
        }
    }

    RefreshActivePad(port);

    return firstUnusedPad;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("false", "Cannot get pad at port %i because it's already in use")
#endif

zPad* zPad::GetUnusedPad(bool isAI)
{
    if (!globals.firstStartPressed) {
        return NULL;
    }

    S32 arrayIndex = -1;

    for (S32 i = 0; i < MAX_PORTS; i++) {
        if ((!isAI || xTRCPadIsPort(i, TRC_PadInserted)) && (isAI || xTRCPadIsPort(i, TRC_PadActive)))
            if (!IsPadAtPort(i, arrayIndex)) {
                return CreatePadAtPort(i, isAI);
            }
    }

    if (!isAI && !globals.enableDebugControls) {
        xFAIL_MSG_L(182, "No available ports to create a new pad !");
    }

    return NULL;
}

zPad* zPad::GetTriggeredPad(U32 key1, U32 key2, U32 key3)
{
    zPad* pad = GetFirstUnused();

    if (pad) {
        pad->used = true;
        pad->enable = false;
        pad->triggered = true;
        pad->key1 = key1;
        pad->key2 = key2;
        pad->key3 = key3;
    }

    return pad;
}

zPad* zPad::GetTriggeredPad(U32 key1, U32 key2)
{
    return GetTriggeredPad(key1, key2, 0);
}

zPad* zPad::GetDebugPad()
{
    if (!debugPad) {
        debugPad = GetTriggeredPad(0x100000, 0x1);
    }

    return debugPad;
}

bool zPad::KeyCheck()
{
    return (PressedDirect(key1) || PressedDirect(key2) || PressedDirect(key3)) && (!key1 || OnDirect(key1)) &&
           (!key2 || OnDirect(key2)) && (!key3 || OnDirect(key3));
}

bool zPad::Toggle(S32& newPort)
{
    if (!triggered || !globals.enableDebugControls) {
        return false;
    }

    S32 oldPort = padPort;

    for (S32 i = 0; i < MAX_PORTS; i++) {
        padPort = i;
        if (KeyCheck()) {
            newPort = padPort;
            padPort = oldPort;
            return true;
        }
    }

    padPort = oldPort;

    return false;
}

void zPad::RefreshActivePad(S32 port)
{
    for (S32 i = 0; i < MAX_PADS; i++) {
        if (port == pads[i].padPort) {
            if (mPad[port].state != ePad_Enabled) {
                if (xTRCPadIsPort(port, TRC_PadInserted)) {
                    zPad* p = &pads[i];
                    p->padPort = -1;
                    p->used = false;
                    p->enable = false;
                    p->triggered = false;
                    p->key1 = 0;
                    p->key2 = 0;
                }
            }
        }
    }
}

void zPad::DisablePad()
{
    enable = false;
    if (padPort >= 0)
        RefreshActivePad(padPort);
}

void zPad::EnablePad()
{
    enable = true;
    if (padPort >= 0)
        RefreshActivePad(padPort);
}

void zPad::CheckPadTriggers()
{
    for (S32 j = 0; j < MAX_PORTS; j++) {
        RefreshActivePad(j);
    }

    for (S32 i = 0; i < MAX_PADS; i++) {
        S32 newPort;
        if (pads[i].triggered && pads[i].Toggle(newPort)) {
            pads[newPort].Clear();

            if (pads[i].Active() && newPort == pads[i].padPort) {
                pads[i].DisablePad();
            } else {
                pads[i].DisablePad();
                pads[i].padPort = newPort;
                pads[i].EnablePad();
            }
        }
    }
}

bool zPad::Active()
{
    return Enabled();
}

void zPad::DeactivateTRC()
{
    if (padPort != -1)
        xTRCPadSetStatePadPort(padPort, TRC_PadInserted);
}

void zPad::Clear()
{
    if (padPort != -1)
        mPad[padPort].pressed = 0;
}

void zPad::ReleasePad()
{
    used = false;
    enable = false;
    padPort = -1;
}

bool zPad::Pressed(U32 button)
{
    return Active() && PressedDirect(button);
}

bool zPad::On(U32 button)
{
    return Active() && OnDirect(button);
}

bool zPad::Released(U32 button)
{
    return Active() && ReleasedDirect(button);
}

bool zPad::PressedDirect(U32 button)
{
    return padPort != -1 && (button & mPad[padPort].pressed);
}

bool zPad::OnDirect(U32 button)
{
    return padPort != -1 && (button & mPad[padPort].on);
}

bool zPad::ReleasedDirect(U32 button)
{
    return padPort != -1 && (button & mPad[padPort].released);
}

bool zPad::Enabled()
{
    if (padPort != -1) {
        for (S32 i = 0; i < MAX_PORTS; i++)
            if (padPort == xTRCPadGetPadPort(i) && !xTRCPadIsIdx(i, TRC_PadActive) && !xTRCPadIsIdx(i, TRC_PadInserted))
                return false;
    }

    return enable && used;
}

F32 zPadAnalogControl::GetX()
{
    if (!owner || owner->padPort == -1) {
        return 0.0f;
    }

    F32 v = 0.0f, y = 0.0f;

    if (owner->Active()) {
        if (analogIndex == 1) {
            v = -mPad[owner->padPort].analog1.x;
            y = -mPad[owner->padPort].analog1.y;
        } else {
            v = -mPad[owner->padPort].analog2.x;
            y = -mPad[owner->padPort].analog2.y;
        }
    }

    if (v > -32.0f && v < 32.0f && y > -32.0f && y < 32.0f) {
        return 0.0f;
    }

    if (v > ANALOG_MAX) {
        return 1.0f;
    }

    if (v < ANALOG_MIN) {
        return -1.0f;
    }

    return v / ANALOG_MAX;
}

F32 zPadAnalogControl::GetY()
{
    if (!owner || owner->padPort == -1) {
        return 0.0f;
    }

    F32 v = 0.0f, x = 0.0f;

    if (owner->Active()) {
        if (analogIndex == 1) {
            v = -mPad[owner->padPort].analog1.y;
            x = -mPad[owner->padPort].analog1.x;
        } else {
            v = -mPad[owner->padPort].analog2.y;
            x = -mPad[owner->padPort].analog2.x;
        }
    }

    if (v > -32.0f && v < 32.0f && x > -32.0f && x < 32.0f) {
        return 0.0f;
    }

    if (v > ANALOG_MAX) {
        return 1.0f;
    }

    if (v < ANALOG_MIN) {
        return -1.0f;
    }

    return v / ANALOG_MAX;
}

F32 zPadAnalogControl::GetMag(F32 x, F32 y)
{
    return MAX(xabs(x), xabs(y));
}

F32 zPadAnalogControl::GetMag()
{
    F32 x, y;
    Get(x, y);
    return GetMag(x, y);
}

F32 zPadAnalogControl::GetAngle()
{
    return GetAngle(GetX(), GetY());
}

F32 zPadAnalogControl::GetAngle(F32 x, F32 y)
{
    return xatan2(x, y);
}

void zPadAnalogControl::Get(F32& x, F32& y, F32& angle, F32& mag)
{
    Get(x, y);
    angle = GetAngle(x, y);
    mag = GetMag(x, y);
}

void zPadAnalogControl::Get(F32& x, F32& y)
{
    x = GetX();
    y = GetY();
}

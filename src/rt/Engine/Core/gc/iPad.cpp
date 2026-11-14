#include "xCommon.h"
#include "iPad.h"

#include "xPad.h"
#include "xTRC.h"
#include "xDebugTweak.h"
#include "zGlobals.h"
#include "zScene.h"
#include "iTime.h"
#include "iTRC.h"
#include "iSystem.h"
#include "iDebug.h"
#include "xutil.h"

#if GRC
#include <revolution/kpad.h>
#endif
#include <string.h>

#if GRC
#define iPadControlMotor(chan, cmd) WPADControlMotor(chan, cmd)
#else
#define iPadControlMotor(chan, cmd) PADControlMotor(chan, cmd)
#endif

F32 g_rumblingThreshold;

#if GRC
namespace {

KPADStatus kpads[PAD_MAX_CONTROLLERS][10] = {};
S32 kpad_reads;
const char* inputSources[4] = { "Nunchaku stick", "Nunchaku tilt", "Revolution tilt", "Revolution dpad" };
U32 sPlayerInputSource;
U32 sCameraInputSource = 2;
bool sPlayerFlipX = true;
bool sPlayerFlipY;
bool sCameraFlipX;
bool sCameraFlipY;
F32 sAccParamPlayRadius;
F32 sAccParamPlaySensitivity = 1.0f;
F32 sPlayerClampMin;
F32 sPlayerClampMax = 1.0f;
bool sPlayerClampRenormalize;
F32 sCameraClampMin = 0.3f;
F32 sCameraClampMax = 1.0f;
bool sCameraClampRenormalize;

}
#endif

static PADStatus sPadData[PAD_MAX_CONTROLLERS] = {};
static u32 sPadMotor[PAD_MAX_CONTROLLERS];
static S32 sPadCurrLevel[PAD_MAX_CONTROLLERS];
static S32 sPadDeltaLevel[PAD_MAX_CONTROLLERS];

S32 iPadInit()
{
#if DEBUG || RELEASE
    xTWEAK("Rumble Start Threshold", &g_rumblingThreshold, 0.0f, 1.0f, NULL, NULL, 0);
#endif

    S32 i;

#if GRC
    xTWEAK_SELECT("Revolution controller|Player input source", &sPlayerInputSource, inputSources, NULL, ARRAY_LENGTH(inputSources), NULL, NULL, 0);
    xTWEAK_SELECT("Revolution controller|Camera input source", &sCameraInputSource, inputSources, NULL, ARRAY_LENGTH(inputSources), NULL, NULL, 0);
    xTWEAK_BOOL("Revolution controller|Player input flip X", &sPlayerFlipX, NULL, NULL, 0);
    xTWEAK_BOOL("Revolution controller|Player input flip Y", &sPlayerFlipY, NULL, NULL, 0);
    xTWEAK_BOOL("Revolution controller|Camera input flip X", &sCameraFlipX, NULL, NULL, 0);
    xTWEAK_BOOL("Revolution controller|Camera input flip Y", &sCameraFlipY, NULL, NULL, 0);
    xTWEAK("Revolution controller|Acc param play radius", &sAccParamPlayRadius, 0.0f, 100.0f, NULL, NULL, 0);
    xTWEAK("Revolution controller|Acc param play sensitivity", &sAccParamPlaySensitivity, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Revolution controller|Player clamp min", &sPlayerClampMin, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Revolution controller|Player clamp max", &sPlayerClampMax, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Revolution controller|Camera clamp min", &sCameraClampMin, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK("Revolution controller|Camera clamp max", &sCameraClampMax, 0.0f, 1.0f, NULL, NULL, 0);
    xTWEAK_BOOL("Revolution controller|Player clamp renormalize", &sPlayerClampRenormalize, NULL, NULL, 0);
    xTWEAK_BOOL("Revolution controller|Camera clamp renormalize", &sCameraClampRenormalize, NULL, NULL, 0);

    KPADInit();
    KPADReset();
    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        u32 type;
        WPADProbe(i, &type);
        if (type == 2)
            WPADSetDataFormat(i, 2);
    }
#else
    PADInit();
#endif

    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        sPadCurrLevel[i] = 0;
        sPadDeltaLevel[i] = 0;
        sPadMotor[i] = PAD_MOTOR_STOP;
    }
    return TRUE;
}

xPad* iPadEnable(xPad* pad, S16 port)
{
    pad->port = port;
    pad->slot = 0;
    pad->flags |= 0x3;
    pad->flags |= 0x4;

    U32 new_on;
    iPadUpdate(pad, &new_on);
    pad->on = new_on;
    pad->pressed = 0;
    pad->released = 0;

    return pad;
}

static inline U32 iPadConvFromGCN(U32 buttons, U32 gcn_flag, U32 x_flag)
{
    if (buttons & gcn_flag)
        return x_flag;
    return 0;
}

static S8 iPadConvStick(F32 in)
{
    if (in > 40.0f)
        in = 40.0f;
    else if (in < -40.0f)
        in = -40.0f;

    F32 fin = 3.2f * in;
    if (fin > 127.0f)
        fin = 127.0f;
    else if (fin < -127.0f)
        fin = -127.0f;

    return (S8)fin;
}

void iPadWaitForRevolution()
{
#if GRC
    bool ready;

    do {
        ready = true;
        iVSync();
        iPadUpdateDataAll();
        for (S32 i = 0; i < PAD_MAX_CONTROLLERS; i++) {
            U32 dummy;
            iPadUpdate(&mPad[i], &dummy);

            u32 type;
            WPADProbe(i, &type);
            if ((type == 2 || type == 1) && mPad[i].state != ePad_Enabled)
                ready = false;
        }
    } while (!ready);
#endif
}

#if GRC
static U16 ConvertKPADButtons(U32 kbut)
{
    U16 pbut = 0;

    pbut |= (kbut & 0x1000) ? PAD_BUTTON_LEFT : 0;
    pbut |= (kbut & 0x800) ? PAD_BUTTON_RIGHT : 0;
    pbut |= (kbut & 0x400) ? PAD_BUTTON_DOWN : 0;
    pbut |= (kbut & 0x200) ? PAD_BUTTON_UP : 0;
    pbut |= (kbut & 0x100) ? PAD_BUTTON_START : 0;
    pbut |= (kbut & 0x4) ? PAD_BUTTON_A : 0;
    pbut |= (kbut & 0x8) ? PAD_BUTTON_X : 0;
    pbut |= (kbut & 0x2) ? PAD_BUTTON_B : 0;
    pbut |= (kbut & 0x1) ? PAD_BUTTON_Y : 0;
    pbut |= (kbut & 0x2000) ? PAD_TRIGGER_L : 0;
    pbut |= (kbut & 0x4000) ? PAD_TRIGGER_R : 0;
    pbut |= (kbut & 0x10) ? PAD_TRIGGER_Z : 0;

    return pbut;
}

static void DeadZone(F32& x, F32 flip, F32 clampMin, F32 clampMax, bool nrmlz)
{
    if (flip)
        x = -x;

    if (x > -clampMin && x < clampMin)
        x = 0.0f;
    else if (x < -clampMax)
        x = -1.0f;
    else if (x > clampMax)
        x = 1.0f;
    else if (x > 0.0f)
        x = nrmlz ? (x - clampMin) / (clampMax - clampMin) : x;
    else
        x = nrmlz ? (x + clampMin) / (clampMax - clampMin) : x;
}
#endif

void iPadUpdateDataAll()
{
#if GRC
    bool spew = gFrameCount > 100;

    for (S32 i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        kpad_reads = KPADRead(i, kpads[i], ARRAY_LENGTH(kpads[i]));

        memset(&sPadData[i], 0, sizeof(sPadData[i]));
        sPadData[i].err = PAD_ERR_NO_CONTROLLER;

        if (spew)
            xprintf("kpad %d type %d %04X\n", i, kpads[i][0].dev_type, (U16)kpads[i][0].hold);

        switch (kpads[i][0].dev_type) {
        case 1:
            break;
        case 2: {
            sPadData[i].button = ConvertKPADButtons(kpads[i][0].hold);
            xVec2 inputs[4] = {
                { kpads[i][0].ex_status.fs.stick.x, kpads[i][0].ex_status.fs.stick.y },
                { kpads[i][0].ex_status.fs.acc.x, kpads[i][0].ex_status.fs.acc.z },
                { kpads[i][0].acc.x, kpads[i][0].acc.z },
                {
                    (sPadData[i].button & PAD_BUTTON_LEFT) ? -1.0f : (sPadData[i].button & PAD_BUTTON_RIGHT) ? 1.0f : 0.0f,
                    (sPadData[i].button & PAD_BUTTON_UP) ? -1.0f : (sPadData[i].button & PAD_BUTTON_DOWN) ? 1.0f : 0.0f,
                },
            };

            F32 inputX = inputs[sPlayerInputSource].x;
            F32 inputY = inputs[sPlayerInputSource].y;
            DeadZone(inputX, sPlayerFlipX, sPlayerClampMin, sPlayerClampMax, sPlayerClampRenormalize);
            DeadZone(inputY, sPlayerFlipY, sPlayerClampMin, sPlayerClampMax, sPlayerClampRenormalize);
            sPadData[i].stickX = 127.0f * inputX;
            sPadData[i].stickY = 127.0f * inputY;

            inputX = inputs[sCameraInputSource].x;
            inputY = inputs[sCameraInputSource].y;
            DeadZone(inputX, sCameraFlipX, sCameraClampMin, sCameraClampMax, sCameraClampRenormalize);
            DeadZone(inputY, sCameraFlipY, sCameraClampMin, sCameraClampMax, sCameraClampRenormalize);
            sPadData[i].substickX = 127.0f * inputX;
            sPadData[i].substickY = 127.0f * inputY;

            sPadData[i].err = kpads[i][0].wpad_err;
            KPADSetAccParam(i, sAccParamPlayRadius, sAccParamPlaySensitivity);
            break;
        }
        case 3:
            sPadData[i].button = kpads[i][0].hold;
            sPadData[i].stickX = 127.0f * kpads[i][0].ex_status.gc.stick.x;
            sPadData[i].stickY = 127.0f * kpads[i][0].ex_status.gc.stick.y;
            sPadData[i].substickX = 127.0f * kpads[i][0].ex_status.gc.substick.x;
            sPadData[i].substickY = 127.0f * kpads[i][0].ex_status.gc.substick.y;
            sPadData[i].triggerLeft = 255.0f * kpads[i][0].ex_status.gc.ltrigger;
            sPadData[i].triggerRight = 255.0f * kpads[i][0].ex_status.gc.rtrigger;
            sPadData[i].err = kpads[i][0].wpad_err;
            break;
        }

        if (sPadData[i].err != PAD_ERR_NO_CONTROLLER)
            if (spew)
                xprintf("sp %d stick %4d,%4d substick %4d,%4d\n", i, sPadData[i].stickX, sPadData[i].stickY,
                        sPadData[i].substickX, sPadData[i].substickY);
    }
#else
    PADRead(sPadData);
    PADClamp(sPadData);
#endif
}

void iPadInHandReset(S32 port, U32* on)
{
    static iTime held_start[PAD_MAX_CONTROLLERS] = {};
    static bool held_through_reset[PAD_MAX_CONTROLLERS] = { true, true, true, true };

    bool held = (*on & 0xA0001) == 0xA0001;
    if (held && !held_through_reset[port]) {
        if (iTimeDiffSec(held_start[port], iTimeGet()) >= 0.5f)
            ResetButton::ResetSystem();
    } else {
        held_start[port] = iTimeGet();
        if (!held)
            held_through_reset[port] = false;
    }
}

S32 iPadUpdate(xPad* pad, U32* on)
{
    U32 read_on = 0;

    switch (sPadData[pad->port].err) {
    case PAD_ERR_NO_CONTROLLER:
        xTRCPadRemove(pad->port);
        mPad[pad->port].state = ePad_Disabled;
#if GRC
        if (gFrameCount > 100)
            xprintf("port %d no pad\n", pad->port);
#else
        PADReset(PAD_CHAN0_BIT >> pad->port);
#endif
        pad->pressed = 0;
        pad->released = 0;
        pad->analog1.x = 0;
        pad->analog1.y = 0;
        pad->analog2.x = 0;
        pad->analog2.y = 0;
        break;
    case PAD_ERR_NOT_READY:
    case PAD_ERR_TRANSFER:
#if GRC
    case WPAD_ERR_INVALID:
#endif
        return FALSE;
    default:
        iprintf("iPadUpdate pad %d err %d\n", pad->port, sPadData[pad->port].err);
        xFAIL_MSG_L(DEBUG ? 591 : 455, "damn, we missed a return code from GCN pad shiznit");
        break;
    case PAD_ERR_NONE:
        goto read;
    }
    return FALSE;

read:
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_LEFT, 0x80);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_RIGHT, 0x20);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_DOWN, 0x40);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_UP, 0x10);

    if (sPadData[pad->port].button & PAD_TRIGGER_Z) {
        read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_TRIGGER_R, 0x2000);
        read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_TRIGGER_L, 0x200);

        if (sPadData[pad->port].triggerLeft >= 0x18)
            read_on |= 0x200;
        if (sPadData[pad->port].triggerRight >= 0x18)
            read_on |= 0x2000;

        read_on |= 0x100000;
    } else {
        read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_TRIGGER_R, 0x1000);
        read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_TRIGGER_L, 0x100);

        if (sPadData[pad->port].triggerLeft >= 0x18)
            read_on |= 0x100;
        if (sPadData[pad->port].triggerRight >= 0x18)
            read_on |= 0x1000;
    }

    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_A, 0x10000);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_B, 0x80000);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_Y, 0x40000);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_X, 0x20000);

    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_START, 0x1);

    *on = read_on;

    pad->analog1.x = iPadConvStick(sPadData[pad->port].stickX);
    pad->analog1.y = -iPadConvStick(sPadData[pad->port].stickY);
    pad->analog2.x = iPadConvStick(sPadData[pad->port].substickX);
    pad->analog2.y = -iPadConvStick(sPadData[pad->port].substickY);

    mPad[pad->port].state = ePad_Enabled;

    xTRCPadInsert(pad->port);

#if DEBUG || RELEASE
    if (pad && xDebugModeGet() == 15)
        xprintf("pad %d state %d:  %05X (% 4d,% 4d) (% 4d,% 4d)\n", pad->port, pad->state,
                sPadData[pad->port].button, sPadData[pad->port].stickX, sPadData[pad->port].stickY,
                sPadData[pad->port].substickX, sPadData[pad->port].substickY);
#endif

    iPadUpdateRumble(pad);

    if (!globals.firstStartPressed) {
        if (xTRCPadIsPort(pad->port, TRC_PadInserted))
            iPadInHandReset(pad->port, on);
    } else {
        if (xTRCPadIsPort(pad->port, TRC_PadActive))
            iPadInHandReset(pad->port, on);
    }

    return TRUE;
}

void iPadStopRumble(xPad* pad)
{
    sPadCurrLevel[pad->port] = 0;
    sPadDeltaLevel[pad->port] = 0;
    sPadMotor[pad->port] = PAD_MOTOR_STOP;
    iPadControlMotor(pad->port, PAD_MOTOR_STOP);
}

void iPadStopRumble()
{
    for (S32 j = 0; j < PAD_MAX_CONTROLLERS; j++) {
        if (xTRCPadIsPort(j, TRC_PadActive)) {
            xPad* pad = &mPad[j];
            sPadCurrLevel[pad->port] = 0;
            sPadDeltaLevel[pad->port] = 0;
            sPadMotor[pad->port] = PAD_MOTOR_STOP;
            iPadControlMotor(pad->port, PAD_MOTOR_STOP);
        }
    }
}

void iPadStartRumble(xPad* pad, F32 intensity)
{
    if (pad->flags & 0x8) {
        if (intensity < g_rumblingThreshold) {
            sPadCurrLevel[pad->port] = 0;
            sPadDeltaLevel[pad->port] = 0;
            sPadMotor[pad->port] = PAD_MOTOR_STOP;
        } else {
            sPadCurrLevel[pad->port] = 0;
            sPadDeltaLevel[pad->port] = 3600.0 * (0.05 + intensity);
            sPadMotor[pad->port] = PAD_MOTOR_RUMBLE;
        }

        iPadControlMotor(pad->port, sPadMotor[pad->port]);
    }
}

void iPadUpdateRumble(xPad* pad)
{
    if (pad->flags & 0x8) {
        sPadCurrLevel[pad->port] += sPadDeltaLevel[pad->port];
        if (sPadCurrLevel[pad->port] > 3600) {
            sPadCurrLevel[pad->port] = 0;
            if (sPadMotor[pad->port] != PAD_MOTOR_RUMBLE) {
                sPadMotor[pad->port] = PAD_MOTOR_RUMBLE;
                iPadControlMotor(pad->port, sPadMotor[pad->port]);
            }
        }
        else if (sPadMotor[pad->port] != PAD_MOTOR_STOP) {
            sPadMotor[pad->port] = PAD_MOTOR_STOP;
            iPadControlMotor(pad->port, sPadMotor[pad->port]);
        }
    }
}

void iPadKill()
{
}

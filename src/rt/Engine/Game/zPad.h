#ifndef ZPAD_H
#define ZPAD_H

#include "xPad.h"

class zPad;

class zPadAnalogControl
{
public:
    static const S32 ANALOG_MIN = -110;
    static const S32 ANALOG_MAX = 110;

    zPadAnalogControl(zPad* owner, S32 analogIndex) : owner(owner), analogIndex(analogIndex) {}

    void Get(F32& x, F32& y, F32& angle); // UNUSED
    void Get(F32& x, F32& y, F32& angle, F32& mag);
    void Get(F32& x, F32& y);
    F32 GetX();
    F32 GetY();
    F32 GetMag(F32 x, F32 y);
    F32 GetMag();
    F32 GetAngle();
    F32 GetAngle(F32 x, F32 y);

private:
    zPad* owner;
    S32 analogIndex;
};

class zPad
{
    friend class zPadAnalogControl;

public:
    static const S32 MAX_PORTS = 4;
    static const S32 MAX_PADS = 6;

    zPad();

    void Init();
    static bool IsPadAtPort(S32 port, S32& arrayIndex);
    static zPad* GetFirstUnused();
    static zPad* FetchPad(S32 arrayIndex); // UNUSED
    static zPad* CreatePadAtPort(S32 port, bool isAI);
    static zPad* GetPadAtPort(S32 port, bool isAI); // UNUSED
    static zPad* GetUnusedPad(bool isAI);
    static zPad* GetTriggeredPad(U32 key1, U32 key2, U32 key3);
    static zPad* GetTriggeredPad(U32 key1, U32 key2);
    static zPad* GetTriggeredPad(U32 key); // UNUSED
    static zPad* GetDebugPad();
    static zPad* GetDualControlPad(); // UNUSED
    bool KeyCheck();
    bool Toggle(S32& newPort);
    static void RefreshActivePad(S32 port);
    void DisablePad();
    void EnablePad();
    static void CheckPadTriggers();
    bool Active();
    void DeactivateTRC();
    void Clear();
    bool IsValid(); // UNUSED
    void ReleasePad();
    bool Pressed(U32 button);
    bool On(U32 button);
    bool Released(U32 button);
    bool PressedDirect(U32 button);
    bool OnDirect(U32 button);
    bool ReleasedDirect(U32 button);
    bool Enabled();

    S32 GetPadPort()
    {
        return padPort;
    }

    xPad* GetXPad()
    {
        return &mPad[padPort];
    }

    zPadAnalogControl* GetAnalog1()
    {
        return &analog1;
    }

    zPadAnalogControl* GetAnalog2()
    {
        return &analog2;
    }

private:
    bool used;
    bool enable;
    bool triggered;
    U32 key1;
    U32 key2;
    U32 key3;
    S32 padPort;
    zPadAnalogControl analog1;
    zPadAnalogControl analog2;
};

void zPadsReleaseDebugAndDual();
void zPadsInit();

#endif

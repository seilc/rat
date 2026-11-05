#ifndef ZPAD_H
#define ZPAD_H

#include "xPad.h"

class zPad;

class zPadAnalogControl
{
public:
    static const S32 ANALOG_MIN;
    static const S32 ANALOG_MAX;

    void Get(F32& x, F32& y);
    F32 GetX();
    F32 GetY();

private:
    zPad* owner;
    S32 analogIndex;
};

class zPad
{
public:
    static const S32 MAX_PORTS;
    static const S32 MAX_PADS;

    static zPad* GetDebugPad();

    bool On(U32 button);
    bool Pressed(U32 button);

    S32 GetPadPort()
    {
        return padPort;
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
    U8 used;
    U8 enable;
    U8 triggered;
    U32 key1;
    U32 key2;
    U32 key3;
    S32 padPort;
    zPadAnalogControl analog1;
    zPadAnalogControl analog2;
};

#endif

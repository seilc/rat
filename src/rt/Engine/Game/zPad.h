#ifndef ZPAD_H
#define ZPAD_H

#include "xPad.h"

class zPad;

class zPadAnalogControl
{
public:
    void Get(F32& x, F32& y);

private:
    zPad* owner;
    S32 analogIndex;
};

class zPad
{
public:
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

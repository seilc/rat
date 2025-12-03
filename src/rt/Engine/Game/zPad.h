#ifndef ZPAD_H
#define ZPAD_H

#include "xPad.h"

class zPad;

class zPadAnalogControl
{
private:
    zPad* owner;
    S32 analogIndex;
};

class zPad
{
public:
    static zPad* GetDebugPad();

    bool Pressed(U32 button);

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

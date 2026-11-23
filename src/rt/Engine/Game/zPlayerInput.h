#ifndef ZPLAYERINPUT_H
#define ZPLAYERINPUT_H

#include "zPad.h"

class zPlayerInput
{
public:
    struct AnalogStick
    {
        F32 x;
        F32 y;
    };

    struct Buttons
    {
#if DEBUG
        bool jump;
        bool actionMove;
        bool contextMove;
        bool otherMove;
#else
        bool jump : 1;
        bool actionMove : 1;
        bool contextMove : 1;
        bool otherMove : 1;
#endif
    };

    zPlayerInput();
    ~zPlayerInput();
    void Clear();
    void Release();
    void SetPad(zPad* pad);
    void Update(F32 dt);
    void HumanPlayerUpdate(F32 dt);
    void GetAnalog1(F32& x, F32& y, F32& angle, F32& mag) const;
    void GetAnalog2(F32& x, F32& y, F32& angle, F32& mag) const; // UNUSED
    zPad* GetPad() const;

    AnalogStick stick1;
    AnalogStick stick2;
    Buttons on;
    Buttons pressed;
    Buttons released;

private:
    zPad* mPad;
    F32 mActionTimer;
};

extern xTWEAKABLE U32 BUTTON_JUMP;
extern xTWEAKABLE U32 BUTTON_ACTION_MOVE;
extern xTWEAKABLE U32 BUTTON_CONTEXT_MOVE;
extern xTWEAKABLE U32 BUTTON_OTHER_MOVE;

#endif

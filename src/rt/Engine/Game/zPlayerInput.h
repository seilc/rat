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

    void SetPad(zPad* pad);

    AnalogStick stick1;
    AnalogStick stick2;
    Buttons on;
    Buttons pressed;
    Buttons released;

private:
    zPad* mPad;
    F32 mActionTimer;
};

#endif

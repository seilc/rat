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
        bool jump;
        bool actionMove;
        bool contextMove;
        bool otherMove;
    };

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

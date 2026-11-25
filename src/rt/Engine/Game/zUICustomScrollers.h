#ifndef ZUICUSTOMSCROLLERS_H
#define ZUICUSTOMSCROLLERS_H

#include "zUICustom.h"
#include "xCounter.h"

class zUICustomNumberScroller : public zUICustom
{
public:
    zUICustomNumberScroller(zUI* ui);

private:
    char buffer[128];
    S32 current;
    F32 timeLeft;
};

class zUICustomDescriptionScroller : public zUICustom
{
public:
    zUICustomDescriptionScroller(zUI* ui);

private:
    xCounter* counter;
    const char* baseText;
    char buffer[384];
    F32 currentTime;
    bool running;
};

class zUICustomMapScroller : public zUICustom
{
public:
    zUICustomMapScroller(zUI* ui);

private:
    xCounter* counter;
    F32 currentX;
    F32 currentY;
    F32 destX;
    F32 destY;
    F32 currentSpeed;
    bool lastRunning;
};

#endif

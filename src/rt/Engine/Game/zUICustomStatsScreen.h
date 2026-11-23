#ifndef ZUICUSTOMSTATSSCREEN_H
#define ZUICUSTOMSTATSSCREEN_H

#include "zUICustom.h"

class zUIText;

class zUICustomParrBestScreen : public zUICustom
{
public:
    void SetCompletedGoalsFlags();

private:
    zUIText* scoreText[4];
    zUIText* scoreNum[4];
    zUIText* goalText[4];
    zUIText* goalNum[4];
    zUIText* statusText[4];
    char numBuff1[4][10];
    char numBuff2[4][10];
    F32 totalTime;
    bool doneUpdating;
};

extern zUICustomParrBestScreen* pUICustomParrBestScreen;

#endif

#ifndef ZUICUSTOMSTATSSCREEN_H
#define ZUICUSTOMSTATSSCREEN_H

#include "xSnd.h"
#include "zUICustom.h"

class zUIText;

class zUICustomStatsScreen : public zUICustom
{
public:
    zUICustomStatsScreen(zUI* ui);

private:
    zUIText* stats1[7];
    zUIText* stats2[7];
    zUIText* numbers1[7];
    zUIText* numbers2[7];
    char numBuff1[7][10];
    char numBuff2[7][10];
    xSndHandle statCycleSound;
    F32 totalTime;
    S32 lastLine;
    bool doneUpdating;
};

class zUICustomParrBestScreen : public zUICustom
{
public:
    zUICustomParrBestScreen(zUI* ui);

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

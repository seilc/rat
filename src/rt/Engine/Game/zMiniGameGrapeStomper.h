#ifndef ZMINIGAMEGRAPESTOMPER_H
#define ZMINIGAMEGRAPESTOMPER_H

#include "zMiniGameBase.h"
#include "iSnd.h"

struct ztextbox;
class zUIImage;
class ResultScreen;

struct zMiniGameAssetGrapeStomper : zMiniGameAsset
{
    F32 timeLimit;
    U32 controlType;
    U32 spitterGroupID;
};

class zMiniGameGrapeStomper : public zMiniGameBase
{
public:
    static void CreatePlayers(xDynAsset*);

    zMiniGameAssetGrapeStomper* GetAsset() const { return (zMiniGameAssetGrapeStomper*)asset; }

protected:
    F32 timer;
    F32 prepStageTimer;
    S32 prepStageTexIdx;
    F32 gameEndTimer;
    F32 resultScreenDelayTimer;
    S32 highscore;
    S32 winner;
    F32 instructPauseTimer;
    iSndGroupHandle prepScreenSounds[3];
    iSndGroupHandle resultSound;
    ztextbox* statsText[4];
    ztextbox* timerText;
    zUIImage* resultImage;
    ResultScreen* rScreen;
};

#endif

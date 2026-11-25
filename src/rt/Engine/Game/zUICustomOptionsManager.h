#ifndef ZUICUSTOMOPTIONSMANAGER_H
#define ZUICUSTOMOPTIONSMANAGER_H

#include "zUICustom.h"

class zUIBox;
class zUIText;

class zUICustomOptionsManager : public zUICustom
{
public:
    zUICustomOptionsManager(zUI* ui);

    F32 musicVolume;
    F32 sfxVolume;
    S32 soundMode;
    bool vibrationOnP1;
    bool vibrationOnP2;

protected:
    bool doupdate;
    zUIText* optionVibP1Text;
    zUIText* optionVibP2Text;
    zUIBox* optionVibP1Box;
    zUIBox* optionVibP2Box;
};

#endif

#ifndef ZUICUSTOMCONTROLOVERLAY_H
#define ZUICUSTOMCONTROLOVERLAY_H

#include "zUICustom.h"

class zUICustomControlOverlay : public zUICustom
{
public:
    zUICustomControlOverlay(zUI* ui);

private:
    U32 controlCount;
    U8 currPlayer;
};

#endif

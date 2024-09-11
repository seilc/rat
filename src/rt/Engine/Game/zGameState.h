#ifndef ZGAMESTATE_H
#define ZGAMESTATE_H

#include "types.h"

typedef enum _GameOstrich
{
    eGameOstrich_Loading,
    eGameOstrich_PlayingMovie,
    eGameOstrich_InScene,
    eGameOstrich_Total
} eGameOstrich;

eGameOstrich zGameGetOstrich();

#endif

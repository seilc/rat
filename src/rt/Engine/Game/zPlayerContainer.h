#ifndef ZPLAYERCONTAINER_H
#define ZPLAYERCONTAINER_H

#include "types.h"

struct zPlayer;

struct zPlayerContainer
{
private:
    zPlayer* playerArray[4];
    S32 numPlayers;
};

#endif

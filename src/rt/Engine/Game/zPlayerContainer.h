#ifndef ZPLAYERCONTAINER_H
#define ZPLAYERCONTAINER_H

#include "xDebug.h"
#include "xEnt.h"

struct zPlayer;

class zPlayerContainer
{
public:
    zPlayer* operator[](S32 index)
    {
        xASSERT(23, index >= 0 && index < numPlayers);
        return playerArray[index];
    }

    S32 size();
    bool IsPlayer(xEnt* ptr);

private:
    zPlayer* playerArray[4];
    S32 numPlayers;
};

#endif

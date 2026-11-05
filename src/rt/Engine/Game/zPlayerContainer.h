#ifndef ZPLAYERCONTAINER_H
#define ZPLAYERCONTAINER_H

#include "xDebug.h"
#include "xEnt.h"

struct zPlayer;

static const S32 MAX_PLAYERS_PER_SCENE = 4;

class zPlayerContainer
{
public:
    zPlayer* operator[](S32 index)
    {
        xASSERT_L(23, index >= 0 && index < numPlayers);
        return playerArray[index];
    }

    S32 size();
    bool IsPlayer(xEnt* ptr);

    void PushBack(zPlayer* player)
    {
        xASSERT_FMT_L(39, numPlayers < MAX_PLAYERS_PER_SCENE, "You can only have %i players in the scene and you are trying to add more", MAX_PLAYERS_PER_SCENE);
        playerArray[numPlayers] = player;
        numPlayers++;
    }

private:
    zPlayer* playerArray[MAX_PLAYERS_PER_SCENE];
    S32 numPlayers;
};

#endif

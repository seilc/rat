#include "zPlayerContainer.h"

#include "zPlayer.h"

bool zPlayerContainer::IsPlayer(xEnt* ptr)
{
    for (S32 i = 0; i < numPlayers; i++) {
        if (playerArray[i] == ptr) {
            return true;
        }
    }
    return false;
}

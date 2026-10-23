#include "zPlayerContainer.h"

bool zPlayerContainer::IsPlayer(xEnt* ptr)
{
    for (S32 i = 0; i < numPlayers; i++) {
        if ((xEnt*)playerArray[i] == ptr) {
            return true;
        }
    }
    return false;
}

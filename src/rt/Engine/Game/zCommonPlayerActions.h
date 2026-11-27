#ifndef ZCOMMONPLAYERACTIONS_H
#define ZCOMMONPLAYERACTIONS_H

#include "zCommonPlayer.h"

class zCommonPlayerAction : public zPlayerAction
{
public:
    zCommonPlayer* GetCommonPlayer() { return (zCommonPlayer*)player; }

    PLAYER_ACTION_DEST_CHECK(zCommonPlayerAction, LandCheck)
};

void zCommonPlayerActionsAirTransitions(zPlayerActionManager* manager, xAnimTable* table, const char* transitionString);

#endif

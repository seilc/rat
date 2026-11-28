#ifndef ZCOMMONPLAYERCONTEXTACTIONS_H
#define ZCOMMONPLAYERCONTEXTACTIONS_H

#include "zCommonPlayerActions.h"

class zPlayerContextAction : public zCommonPlayerAction
{
public:
    zContextObjectManager& GetContextObjectManager()
    {
        return GetCommonPlayer()->contextManager;
    }

    PLAYER_ACTION_CHECK_L(zPlayerContextAction, MountedCheck, 29)
};

#endif

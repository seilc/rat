#ifndef ZPLAYERACTION_H
#define ZPLAYERACTION_H

#include "zEnt.h"

class zPlayerActionManager;
struct zPlayer;

class zPlayerAction
{
protected:
    zPlayerActionManager* manager;
    zPlayer* player;

public:
    void* __vptr; // TEMP
};

class zPlayerActionManager
{
protected:
    zPlayerAction** actionList;
    zPlayerAction* currentAction;
    U32 listSize;
};

#endif

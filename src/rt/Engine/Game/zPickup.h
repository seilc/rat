#ifndef ZPICKUP_H
#define ZPICKUP_H

#include "xBase.h"

struct zPlayer;

class zPickup : public xBase
{
public:
    static void TryBeGrabbedAll(zPlayer* currentPlayer);
};

#endif

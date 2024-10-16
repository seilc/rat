#ifndef ZNPCGROUP_H
#define ZNPCGROUP_H

#include "xBase.h"
#include "xDynAsset.h"

namespace zNPC {

struct group_asset : xDynAsset
{
    S32 max_attackers;
};

class group : public xBase
{
protected:
    const group_asset* asset;
    S32 attacking_count;
};

}

#endif

#ifndef ZPLAYERCONSTRAINER_H
#define ZPLAYERCONSTRAINER_H

#include "types.h"

class zPlayerConstrainer
{
public:
    static void SetConstraints(F32 nearWall, F32 farWall);
    static void GetDefaults(F32& nearWall, F32& farWall);
};

#endif

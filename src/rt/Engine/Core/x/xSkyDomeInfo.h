#ifndef XSKYDOMEINFO_H
#define XSKYDOMEINFO_H

#include "xEnt.h"

struct SkyDomeInfo
{
    xEnt* ent;
    S32 sortorder;
    S32 lockXZ;
    S32 lockY;
    S32 zReadWrite;
};

#ifndef NON_MATCHING
static void __unused(SkyDomeInfo* a, SkyDomeInfo* b)
{
    *a = *b;
}
#endif

#endif

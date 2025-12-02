#include "xEnt.h"

#ifndef NON_MATCHING
static void __unused(xEnt* ent, xVec3& v)
{
    xEntDisable(ent);
    xVec3Inv(v);
}
#endif

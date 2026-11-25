#ifndef ZVOLUME_H
#define ZVOLUME_H

#include "xVolume.h"

#include <rwcore.h>

struct RpAtomic;
struct xVec3;

class zVolume : public xVolume
{
};

extern S32 gOccludeCount;

void zVolumeInit();
void zVolumeSetup();
U16 zVolumeGetNumVolumes(); // UNUSED
zVolume* zVolumeGetVolume(U16 n);
void zVolume_OccludePrecalc(xVec3* camPos);
S32 zVolume_OccludeTest(RpAtomic*, RwMatrix*); // UNUSED
void zVolumeEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif

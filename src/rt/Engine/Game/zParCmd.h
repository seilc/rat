#ifndef ZPARCMD_H
#define ZPARCMD_H

#include "types.h"

struct xParCmd;
struct xParGroup;

void zParCmdInit();
void zParCmdFindClipVolumes();
void xParCmdKillDistance_Update(xParCmd* c, xParGroup* ps, F32);
void xParCmdClipVolumes_Update(xParCmd* c, xParGroup* ps, F32);
void xParCmdPlayerCollision_Update(xParCmd*, xParGroup*, F32);
void xParCmdAnimalMagentism_Update(xParCmd* c, xParGroup* ps, F32 dt);
void xParCmdDamagePlayer_Update(xParCmd* c, xParGroup* ps, F32);
void xParCmdJet_Update(xParCmd* c, xParGroup* ps, F32 dt);
void xParCmdCustom_Grass_Update(xParCmd*, xParGroup* ps, F32 dt);
void xParCmdApplyCamMat_Update(xParCmd* c, xParGroup* ps, F32 dt);
void xParCmdCustom_Update(xParCmd* c, xParGroup* ps, F32 dt);

#endif

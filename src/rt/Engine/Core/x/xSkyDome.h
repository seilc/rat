#ifndef XSKYDOME_H
#define XSKYDOME_H

#include "xEnt.h"

void xSkyDome_EmptyRender(xEnt*);
void xSkyDome_Setup();
void xSkyDome_AddEntity(xEnt* ent, S32 sortorder, S32 lockY, S32 lockXZ, S32 ZReadWrite);
void xSkyDome_Render();
void xSkyDome_GetSettings(xEnt* ent, S32& sortorder, S32& lockY, S32& lockXZ);
void xSkyDome_BeginRenderSingle();
void xSkyDome_EndRenderSingle();
void xSkyDome_RenderSingle(xEnt* ent);

inline bool xSkyDome_EntityIsSkyDome(const xEnt* ent)
{
    return ent->render == xSkyDome_EmptyRender;
}

#endif

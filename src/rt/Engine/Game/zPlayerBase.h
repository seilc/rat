#ifndef ZPLAYERBASE_H
#define ZPLAYERBASE_H

#include "types.h"

struct xEnt;
struct xEntFrame;
struct xScene;
struct zEnt;
struct zPlayerCheckPoint;

void zPlayerUpdate(xEnt* ent, xScene* sc, F32 dt);
void zPlayerRender(zEnt* ent);
void zPlayerReset(zEnt* ent);
void zPlayerMove(xEnt* ent, xScene* s, F32 dt, xEntFrame* frame);
void zPlayerLoadHIP(U32 gameSceneID);
void zPlayerUnloadHIP();
void zPlayer_StoreCheckPoint(const zPlayerCheckPoint* checkPoints, U32 initialCameraID);
void zPlayer_LoadCheckPoint(); // UNUSED
void zPlayerSceneInit(U32 sceneID);
void zPlayerSceneExit();

#endif

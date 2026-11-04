#ifndef XSCENE_H
#define XSCENE_H

#include "xBase.h"
#include "xEnv.h"
#include "xMemMgr.h"

struct xCollis;
struct xEnt;
struct xGrid;
struct xModelInstance;
struct xQCData;
struct xRay3;
struct xScene;
struct xSerial;
struct xSphere;

typedef xBase*(*xSceneResolvIDCallback)(U32);
typedef char*(*xSceneBase2NameCallback)(xBase*);
typedef char*(*xSceneID2NameCallback)(U32);
typedef void(*xRayEntCallback)(xScene*, xRay3*, xQCData*, xEnt*, void*);
typedef xEnt*(*xSceneEntCallback)(xEnt*, xScene*, void*);
typedef void(*xRayCameraCallback)(xEnt*);

struct xScene
{
    U32 sceneID;
    U16 flags;
    U16 numTrigs;
    U16 numSpecialTriggers;
    U16 numStats;
    U16 numDyns;
    U16 numNpcs;
    U16 numActEnts;
    F32 gravity;
    F32 drag;
    F32 friction;
    U16 numEntsAllocd;
    U16 numTrigsAllocd;
    U16 numSpecialTriggersAllocd;
    U16 numStatsAllocd;
    U16 numDynsAllocd;
    U16 numNpcsAllocd;
    xEnt** trigs;
    xEnt** specialTriggers;
    xEnt** stats;
    xEnt** dyns;
    xEnt** npcs;
    xEnt** actEnts;
    xEnv* env;
    xMemPool mempool;
    xSceneResolvIDCallback resolvID;
    xSceneBase2NameCallback base2Name;
    xSceneID2NameCallback id2Name;
};

struct xNearFloorPoly
{
    xBox box;
    xVec3 center;
    xVec3 facingVec;
    F32 facingDot;
    F32 floorDot;
    xVec3 vert[3];
    F32 neardist;
    xVec3 nearpt;
    S32 nearvert;
    S32 nearedge;
    U32 oid;
    void* optr;
    xModelInstance* mptr;
};

void xSceneInit(xScene* sc, U16 numTrigs, U16 numSpecialTrigs, U16 numStats, U16 numDyns, U16 numNpcs);
void xSceneExit(xScene* sc);
void xSceneSave(xScene* ent, xSerial* s);
void xSceneLoad(xScene* ent, xSerial* s);
void xSceneToString(char*, xScene*);
void xSceneLoadTexDict(xScene* sc, const void* data, U32 datasize);
void xSceneSetEnv(xScene* sc, xEnv* env);
void xSceneSetup(xScene* sc);
void xSceneAddEnt(xScene* sc, xEnt* ent);
void xSceneUpdate(xScene* sc, F32 dt);
xBase* xSceneResolvID(xScene* sc, U32 id);
const char* xSceneBase2Name(xScene* sc, xBase* p);
const char* xSceneID2Name(xScene* sc, U32 id);
void xSceneRender(xScene* sc, bool alpha);
void xSceneForAllEnts(xScene* sc, xSceneEntCallback func, void* data);
void xSceneForAllTriggers(xScene* sc, xSceneEntCallback func, void* data);
void xSceneForAllStatics(xScene* sc, xSceneEntCallback func, void* data);
void xSceneForAllDynamics(xScene* sc, xSceneEntCallback func, void* data);
void xSceneForAllNPCs(xScene* sc, xSceneEntCallback func, void* data);
void xRayHitsGrid(xGrid* grid, xScene* sc, xRay3* r, xRayEntCallback rentcb, xQCData* qcr, void* data);
void xSceneDebugUpdate();
void xRayHitsTikiLandableEnt(xScene*, xRay3* r, xQCData* qcr, xEnt* ent, void* colldata);
void xRayHitsEnt(xScene*, xRay3* r, xQCData* qcr, xEnt* ent, void* colldata);
void xRayHitsTikiLandableScene(xScene* sc, xRay3* r, xCollis* coll);
void xRayHitsScene(xScene* sc, xRay3* r, xCollis* coll);
void xRayHitsCameraEnt(xScene*, xRay3* r, xCollis*, xQCData* qcr, xEnt* ent);
void xRayHitsCameraScene(xScene* sc, xRay3* r, xRayCameraCallback cameraFunc);
void xRayHitsEntFlags(xScene*, xRay3* r, xCollis* coll, xQCData* qcr, xEnt* ent, U8 collType);
void xRayHitsEntFlagsCB(xScene* sc, xRay3* r, xCollis* coll, xQCData* qcr, xEnt* ent);
void xRayHitsSceneFlags(xScene* sc, const xRay3* r, xCollis* coll, U8 collType, U8 chk);
void xSphereHitsScene(xScene* sc, const xSphere* sphere, xCollis* coll, U8 collType, U8 chk);
U32 xSceneNearestFloorPoly(xScene* sc, xNearFloorPoly* nfpoly, U8 collType, U8 chk);

#endif

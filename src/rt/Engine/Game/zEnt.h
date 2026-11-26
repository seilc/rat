#ifndef ZENT_H
#define ZENT_H

#include "xEnt.h"

struct xModelAssetParam;
struct zScene;

struct zEnt : xEnt
{
    xAnimTable* atbl;
};

void zEntInit(zEnt* ent, xEntAsset* asset, U32 type);
void zEntSetup(zEnt* ent);
void zEntSave(zEnt* ent, xSerial* s);
void zEntLoad(zEnt* ent, xSerial* s);
void zEntReset(zEnt* ent);
void zEntUpdate(zEnt* ent, zScene* scene, F32 elapsedSec);
void zEntParseModelInfo(xEnt* ent, U32 assetID);
xModelInstance* zEntRecurseModelInfo(void* info, xEnt* ent);
void zEntAnimEventCore(xModelInstance* model, xAnimTable* atbl, U32 animEvent, const F32* animParam, xEnt*);
void zEntAnimEvent(zEnt* ent, U32 animEvent, const F32* animParam);
xModelAssetParam* zEntGetModelParams(U32 assetID, U16* size);
xModelAssetParam* zParamGetNext(xModelAssetParam* param, U16* size);
F32 zParamGetFloat(xModelAssetParam* param, U32 size, const char* tok, F32 def);
void zEntGetShadowParams(xEnt* ent, xVec3* center, xVec3* radius, F32* strength, xEntShadow::radius_enum rtype);
void zEntGetShadowParams(xEnt* ent, xVec3* center, F32* radius, F32* strength, xEntShadow::radius_enum rtype);
void zEntEventAllOfType(U32 toEvent, U32 type);

void zEventDebugLog(xBase* from, U32 fromEvent, xBase* to, U32 toEvent, const F32*);

#endif

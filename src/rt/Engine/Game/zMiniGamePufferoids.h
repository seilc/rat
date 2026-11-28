#ifndef ZMINIGAMEPUFFEROIDS_H
#define ZMINIGAMEPUFFEROIDS_H

#include "zMiniGameBase.h"
#include "zMiniGamePickups.h"
#include "xMath3.h"

struct pointer_asset;
struct xGroup;
struct ztextbox;
class zUIImage;

class zMiniGamePufferoidsPuff
{
public:
    struct PuffInitData
    {
        pointer_asset* pointer;
        F32 speed;
    };

    void Init(PuffInitData* data);
    void Update(F32 dt);
    void Render();
    F32 GetRadius();
    void ProjectileHit(S32 owner, S32 value);
    void Kill();

    void GetSphere(xSphere& sph)
    {
        sph.center = mat.pos;
        sph.r = GetRadius();
    }

protected:
    S32 ownerIndex;
    F32 currentSize;
    F32 speed;
    F32 bounceTime;
    xMat4x3 mat;
    xModelInstance* models[5];
    S32 state;

public:
    zMiniGamePufferoidsPuff* next;
};

struct zMiniGameAssetPufferoidsStage : zMiniGameAsset
{
    U32 totalPuffs;
    U32 initialPuffs;
    U32 spawnMin;
    U32 spawnMax;
    F32 puffSpeed;
    U32 puffSpawnGroupID;
    U32 pickupSpawnGroupID;
    U32 nextStageID;
};

struct zMiniGameAssetPufferoids : zMiniGameAsset
{
    U32 playerLives;
    U32 firstStageID;
};

class zMiniGamePufferoids : public zMiniGameBase
{
public:
    static void CreatePlayers(xDynAsset*); // UNUSED

    zMiniGameAssetPufferoids* GetAsset() const { return (zMiniGameAssetPufferoids*)asset; }

    zMiniGamePufferoidsPuff* GetActivePuffs() { return activePuffs; }

protected:
    zMiniGamePickupController pickups;
    zMiniGameAssetPufferoidsStage* currStage;
    xGroup* puffSpawnGroup;
    U32 puffSpawned;
    zMiniGamePufferoidsPuff* freePuffs;
    zMiniGamePufferoidsPuff* activePuffs;
    F32 resetTimer;
    S32 winner;
    ztextbox* statsText[4];
    zUIImage* resultImage;
};

#endif

#ifndef ZMINIGAMEDEFENDTHEBASE_H
#define ZMINIGAMEDEFENDTHEBASE_H

#include "zMiniGameBase.h"
#include "xMath3.h"

struct pointer_asset;
struct xGroup;
struct ztextbox;
class zUIImage;

class zMiniGameDefendTheBaseEnemy : public zMiniGameObject
{
public:
    void Spawn(pointer_asset* pointer, F32 speed, S32 pIdx);
    virtual void Update(F32 dt);
    virtual void Render();
    virtual F32 GetRadius();
    virtual void Kill();

    xVec3& GetPosition() { return mat.pos; }

protected:
    S32 playerIdx;
    xMat4x3 mat;
    F32 speed;
    F32 damageBaseTimer;
};

struct zMiniGameAssetDefendTheBaseWave : zMiniGameAsset
{
    S32 enemyCount;
    F32 enemySpeed;
    S32 initialMin;
    S32 initialMax;
    F32 delayMin;
    F32 delayMax;
    U32 spawnGroupP1ID;
    U32 spawnGroupP2ID;
    U32 nextWaveID;
};

struct zMiniGameAssetDefendTheBase : zMiniGameAsset
{
    U32 playerModelGroupID;
    F32 coolDown;
    U32 p1BaseGroupID;
    U32 p2BaseGroupID;
    U32 firstWaveID;
};

class zMiniGameDefendTheBase : public zMiniGameBase
{
public:
    class Wave
    {
        struct PlayerData
        {
            S32 pointerCount;
            pointer_asset** spawnPointers;
            S32 enemiesActive;
            S32 enemiesToSpawn;
            F32 spawnTimer;
        };

        zMiniGameAssetDefendTheBaseWave* asset;
        zMiniGameDefendTheBaseEnemy* activeEnemyList;
        S32 playerCount;
        PlayerData plyrData[4];

    public:
        Wave* next;

        zMiniGameDefendTheBaseEnemy* GetActiveEnemyList() { return activeEnemyList; }
    };

    static void CreatePlayers(xDynAsset*);

    zMiniGameAssetDefendTheBase* GetAsset() const { return (zMiniGameAssetDefendTheBase*)asset; }

    zMiniGameDefendTheBaseEnemy* GetActiveEnemyList()
    {
        if (currentWave) {
            return currentWave->GetActiveEnemyList();
        }
        return NULL;
    }

    F32 GetPlayerCoolDown()
    {
        return GetAsset()->coolDown;
    }

protected:
    Wave* firstWave;
    Wave* currentWave;
    S32 waveNum;
    bool waitingForWaveDone;
    F32 waveDoneTimer;
    S32 winner;
    xGroup* baseGroups[4];
    ztextbox* statsText[4];
    ztextbox* waveText;
    ztextbox* waveDoneText;
    zUIImage* resultImage;
};

#endif

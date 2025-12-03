#ifndef ZNPCBASE_H
#define ZNPCBASE_H

#include "xBase.h"
#include "xMath3.h"
#include "xModelAsset.h"
#include "zNPCTypes.h"
#include "zNPCAsset.h"

class zICompLogic;
class zICompNPCEntity;
class zICompSteering;
class zICompPerception;
class zICompSound;
class zICompCombat;

struct xScene;

class zNPCBase : public xBase
{
protected:
    xVec3 lastPos;
    xVec3 lastOrientation;
    F32 updateCullDistance;
    eNPCType type;
    zNPCAssetRATS* npcAsset;
    U32 npcAssetSize;
    const xModelAssetInfo* modelAsset;
    zNPCBase* children;
    zNPCBase* parent;
    S32 numChildren;
    bool activated : 1;
    bool alive : 1;
    bool inUpdateDistance : 1;
    bool crossingUpdateDistance : 1;
    bool taggedForSpawn : 1;
    bool taggedForKill : 1;
    U32 pad : 26;

public:
    virtual bool Activate() = 0;
    virtual void Deactivate() = 0;
    virtual void Spawn();
    virtual bool Kill();
    virtual void Update(xScene* xscn, F32 dt) = 0;
    virtual void Render() = 0;
    virtual void UpdateNPCBaseData();
    virtual void SceneEnter();
    virtual void SceneReset();
    virtual void SceneExit();
    virtual bool SystemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);
    virtual void Load(eNPCType type, const xModelAssetInfo* modelAsset, zNPCAssetRATS* npcAsset, U32 npcAssetSize);
    virtual void Reset();

    zICompNPCEntity* GetEntityComponent();
    zICompSteering* GetSteeringComponent();

protected:
    zICompLogic* npcLogicComp;
    zICompNPCEntity* npcEntityComp;
    zICompSteering* npcSteeringComp;
    zICompPerception* npcPerceptionComp;
    zICompSound* npcSoundComp;
    zICompCombat* npcCombatComp;
};

#endif

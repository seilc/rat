#ifndef ZCOMMONPLAYER_H
#define ZCOMMONPLAYER_H

#include "zPlayer.h"

struct xEnv;
struct xHierarchyBoundInitData;
struct zSurfAssetIN;

enum zCommonPlayerUpgradeType
{
    ezCommonPlayerUpgradeType_Melee,
    ezCommonPlayerUpgradeType_AirMelee,
    ezCommonPlayerUpgradeType_Distance,
    ezCommonPlayerUpgradeType_Health,
    ezCommonPlayerUpgradeType_NukeCnt,
    ezCommonPlayerUpgradeType_Count
};

enum zCommonPlayerUpgradeLevel
{
    ezCommonPlayerUpgradeLevel_Normal,
    ezCommonPlayerUpgradeLevel_Level1,
    ezCommonPlayerUpgradeLevel_Level2,
    ezCommonPlayerUpgradeLevel_Count
};

struct zCommonPlayer : zPlayer
{
    virtual S32 EnvCollision(xSweptSphere* sphere, xEnv*);
    virtual void BreakGrab();
    virtual void BeforeAnimMatrices(xAnimPlay*, xQuat*, xVec3*, S32);
    virtual void LoadUpgrades();
    virtual U8 DoCriticalCollisionCheck() const;
    virtual xHierarchyBoundInitData* GetHierarchyBoundInitData() const;
    virtual F32 GetHierarchyMasterRadius() const;
    virtual void AbsControl(F32 x, F32 z, F32 dt);
    virtual void Rumble(U32 rumbleType);
    virtual void SetupRumbles();
    virtual void HandleFootstepCustom(zSurfAssetIN*);
    virtual void SavedFriend();
    virtual void SetUpgrade(zCommonPlayerUpgradeType forThisType, zCommonPlayerUpgradeLevel level);

    zCommonPlayerUpgradeLevel GetUpgradeLevel(zCommonPlayerUpgradeType forThisType);
};

#endif

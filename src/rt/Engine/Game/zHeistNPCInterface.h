#ifndef ZHEISTNPCINTERFACE_H
#define ZHEISTNPCINTERFACE_H

#include "xBase.h"
#include "xColor.h"
#include "xVec3.h"
#include "zHeistMovementPath.h"
#include "zHeistNPCInterfaceAsset.h"

struct xEnt;
struct xMovePoint;
class zHeistDistractionObject;
class zHeistGangMemberInterfaceManager;
class zHeistLookoutPoint;
class zHeistNPCDetectorEnemyInterfaceManager;
class zNPCBase;

struct zHeistNPCDetectorPrivateFlags
{
    U8 hasPatrolRoute : 1;
    U8 detectionEnabled : 1;
    U32 Pad : 30;
};

class zHeistNPCDetectorViewBlocker : public xBase
{
    struct PrivateFlags
    {
        U8 isViewed : 1;
        U8 isBlocking : 1;
        U32 Pad : 30;
    };

protected:
    static zHeistNPCDetectorEnemyInterfaceManager* detectorEnemyInterfaceManager;

    PrivateFlags privateFlags;
    xEnt* blockerObject;
    xVec3 safeAreaOrientation;
    zHeistNPCDetectorViewBlockerAsset* blockerAsset;
    S32 viewBlockerArrayIdx;
};

class zHeistNPCDetectorEnemyInterface : public xBase
{
public:
    zNPCBase* GetNPCBase();

protected:
    static zHeistNPCDetectorEnemyInterfaceManager* detectorEnemyInterfaceManager;

    zHeistNPCDetectorPrivateFlags privateFlags;
    zHeistNPCDetectorEnemyInterfaceAsset* npcAsset;
    xBase* objBase;
    xEnt* detectedObj;
    S32 interfaceArrayIdx;
    S32 currentPoint;
    S32 numPatrolPoints;
    xMovePoint** patrolRoute;
    S32 detectionState;
    xMovePoint* destinationPoint;
    F32 distractTimer;
    zHeistDistractionObject* distractor;
    F32 detectionFOV;
    F32 alertFOV;
    xColor safeAreaColor;
};

class zHeistNPCDetectorEnemyInterfaceManager
{
protected:
    S32 interfaceCount;
    S32 viewBlockerCount;
    zHeistNPCDetectorEnemyInterface** interfaceArray;
    zHeistNPCDetectorViewBlocker** viewBlockerArray;
};

struct zHeistGangMemberPrototypeNavData
{
private:
    S32 currentMovePointIdx;
    S32 direction;
};

class zHeistGangMemberInterface : public xBase
{
    struct PrivateFlags
    {
        U8 atStart : 1;
        U8 on : 1;
        U32 pad : 30;
    };

protected:
    static zHeistGangMemberInterfaceManager* gangMemberInterfaceManager;

    S32 interfaceArrayIdx;
    zHeistGangMemberInterfaceAsset* gangMemberAsset;
    xEnt* gangMember;
    S32 currPathIdx;
    zHeistGangMemberPrototypeNavData navData;
    S32 actionType;
    xBase* actionObject;
    zHeistMovementPath* lastPath;
    PrivateFlags privateFlags;
};

class zHeistGangMemberInterfaceManager
{
protected:
    S32 gangMemberInterfaceCount;
    zHeistGangMemberInterface** interfaceArray;
    zHeistMovementPathManager* heistMovementPathManager;
    S32 selectedGangMemberIdx;
};

class zHeistNPCInterfaceManager
{
public:
    void Init();
    void Exit();
    void Update(F32 dt);
    void RenderOverlays();
    void ResetSelection(zHeistLookoutPoint* lookoutPoint);
    void SelectNextGangMember(zHeistLookoutPoint* lookoutPoint);
    void SelectPrevGangMember(zHeistLookoutPoint* lookoutPoint);
    void SelectNextPath();
    void SelectPrevPath();
    void ExecuteCommand();
    void CheckSelected(zHeistLookoutPoint* lookoutPoint);

protected:
    zHeistNPCDetectorEnemyInterfaceManager* heistNPCDetectorEnemyInterfaceManager;
    zHeistGangMemberInterfaceManager* heistGangMemberInterfaceManager;
};

#endif

#ifndef ZICOMPNPCENTITY_H
#define ZICOMPNPCENTITY_H

#include "zIComponent.h"
#include "zNPCBase.h"
#include "xEnt.h"

struct xEntDrive;

class zICompNPCEntityParams : public zCompParameters
{
public:
    F32 shadowRadiusPar;
    F32 shadowRadiusRatio;
    F32 modelScale;
    F32 boundScale;
    S32 boundBone;
    F32 boundRadius;
    F32 shadowStrength;
    xVec3 boundOffset;
    bool hasCollision : 1;
    bool noAttackDuringCinematic : 1;
    bool castShadow : 1;
    bool complexShadow : 1;
    bool useBoundingBox : 1;
    bool useShadowRadiusRatio : 1;
    
    virtual void SetDefaults();
};

class zICompNPCEntity : public zIComponent<zNPCBase>, public xEnt
{
private:
    xEntDrive* npcDrive;
    F32 shadowRadius;
    eNPCType type;
    const zICompNPCEntityParams* parameters;
    xSpringyAngle turnSpring;
    xVec3 acc;
    xVec3 oldAcc;
    U8 old_chkby;
    U8 accUpdated;
};

#endif

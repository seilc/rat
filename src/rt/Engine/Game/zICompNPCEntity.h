#ifndef ZICOMPNPCENTITY_H
#define ZICOMPNPCENTITY_H

#include "zIComponent.h"
#include "zNPCBase.h"
#include "xEnt.h"

struct xEntDrive;

class zICompNPCEntityParams : public zCompParameters
{
public:
    zICompNPCEntityParams() { SetDefaults(); }

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

    virtual void SetDefaults()
    {
        shadowRadiusPar = 1.0f;
        shadowRadiusRatio = 1.25f;
        modelScale = 1.0f;
        boundScale = 1.0f;
        boundBone = -1;
        boundRadius = 0.0f;
        shadowStrength = 0.3f;

        boundOffset = xVec3::m_Null;

        hasCollision = true;
        noAttackDuringCinematic = true;
        castShadow = true;
        complexShadow = false;
        useBoundingBox = false;
        useShadowRadiusRatio = true;
    }
};

class zICompNPCEntity : public zIComponent<zNPCBase>, public xEnt
{
    xEntDrive* npcDrive;
    F32 shadowRadius;
    eNPCType type;
    const zICompNPCEntityParams* parameters;
    xSpringyAngle turnSpring;
    xVec3 acc;
    xVec3 oldAcc;
    U8 old_chkby;
    U8 accUpdated;

public:
    void Render();
    void SetAnimState(U32 new_stateID, F32 blend, xModelInstance* model);
    void SetAnimState(const char* new_state, F32 blend, xModelInstance* model);
    bool IsAnimationStopped(U32 AnimationID);
    bool IsAnimationTranstitionFinished(U32 AnimationID);
    xVec3* GetPosition() { return &frame->mat.pos; }
    xVec3* GetHeading() { return &frame->mat.at; }
    xEntFrame* GetFrame() { return frame; }
    xModelInstance* GetModel() { return model; }
    F32 GetModelScale() { return Params()->modelScale; }

private:
    const zICompNPCEntityParams* Params() { return parameters; }
};

#endif

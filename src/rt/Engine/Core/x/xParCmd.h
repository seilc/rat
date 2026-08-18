#ifndef XPARCMD_H
#define XPARCMD_H

#include "xMath3.h"

struct xParGroup;

struct xParCmdAsset
{
    U32 type;
    bool enabled;
    U8 mode;
    U8 pad[2];
};

struct xParCmdMove : xParCmdAsset
{
    xVec3 dir;
};

struct xParCmdMoveRandom : xParCmdAsset
{
    xVec3 dim;
};

struct xParCmdMoveRandomPar : xParCmdAsset
{
    xVec3 dim;
};

struct xParCmdScale3rdPolyReg : xParCmdAsset
{
    F32 constant;
    F32 a;
    F32 a2;
    F32 a3;
};

struct xParCmdAlpha3rdPolyReg : xParCmdAsset
{
    F32 constant;
    F32 a;
    F32 a2;
    F32 a3;
};

struct xParCmdClipVolumes : xParCmdAsset
{
    S32 unused;
};

struct xParCmdSmokeAlpha : xParCmdAsset
{
    S32 type;
};

struct xParCmdDamagePlayer : xParCmdAsset
{
    S32 damage;
    S32 granular;
};

struct xParCmdScale : xParCmdAsset
{
    S32 type;
};

struct xParCmdAnimalMagentism : xParCmdAsset
{
    F32 magnetism;
};

struct xParCmdRotPar : xParCmdAsset
{
    xVec3 min;
    xVec3 max;
};

struct xParCmdApplyWind : xParCmdAsset
{
    F32 strength;
};

struct xParCmdPlayerCollision : xParCmdAsset
{
    F32 min;
    F32 max;
};

struct xParCmdRandomVelocityPar : xParCmdAsset
{
    F32 x;
    F32 y;
    F32 z;
};

struct xParCmdAccelerate : xParCmdAsset
{
    xVec3 acc;
};

struct xParCmdVelocityApply : xParCmdAsset
{
};

struct xParCmdJet : xParCmdAsset
{
    xVec3 center;
    xVec3 acc;
    F32 gravity;
    F32 epsilon;
    F32 radiusSqr;
};

struct xParCmdKillOld : xParCmdAsset
{
    F32 ageMax;
};

struct xParCmdKillSlow : xParCmdAsset
{
    F32 speedLimitSqr;
    U32 kill_less_than;
};

struct xParCmdKillDistance : xParCmdAsset
{
    F32 dSqr;
    U32 kill_greater_than;
};

struct xParCmdAge : xParCmdAsset
{
    F32 age_rate;
};

struct xParCmdFollow : xParCmdAsset
{
    F32 gravity;
    F32 epsilon;
};

struct xParCmdOrbitLine : xParCmdAsset
{
    xVec3 p;
    xVec3 axis;
    F32 gravity;
    F32 epsilon;
    F32 maxRadiusSqr;
};

struct xParCmdOrbitPoint : xParCmdAsset
{
    xVec3 center;
    F32 gravity;
    F32 epsilon;
    F32 maxRadiusSqr;
};

struct xParCmdApplyCamMat : xParCmdAsset
{
    xVec3 apply;
};

struct xParCmdRotateAround : xParCmdAsset
{
    xVec3 pos;
    F32 unused1;
    F32 radius_growth;
    F32 yaw;
};

struct xParCmdTex : xParCmdAsset
{
    F32 x1;
    F32 y1;
    F32 x2;
    F32 y2;
    U8 birthMode;
    U8 rows;
    U8 cols;
    U8 unit_count;
    F32 unit_width;
    F32 unit_height;
};

struct xParCmdTexAnim : xParCmdAsset
{
    U8 anim_mode;
    U8 anim_wrap_mode;
    U8 pad_anim;
    U8 throttle_spd_less_than;
    F32 throttle_spd_sqr;
    F32 throttle_time;
    F32 throttle_time_elapsed;
};

struct xParCmdCustom : xParCmdAsset
{
    U32 user_id;
    F32 user_val[4];
};

struct xParCmdCollideFall : xParCmdAsset
{
    F32 y;
    F32 bounce;
};

struct xParCmdCollideFallSticky : xParCmdCollideFall
{
    F32 sticky;
};

struct xParCmdDampenData : xParCmdAsset
{
    F32 dampSpeed;
};

struct xParCmdAlphaInOutData : xParCmdAsset
{
    F32 custAlpha[4];
};

struct xParCmdSizeInOutData : xParCmdAsset
{
    F32 custSize[4];
};

struct xParCmdShaperData : xParCmdAsset
{
    F32 custAlpha[4];
    F32 custSize[4];
    F32 dampSpeed;
    F32 gravity;
};

union xParCmdAny
{
    xParCmdMove Move;
    xParCmdMoveRandom MoveRandom;
    xParCmdMoveRandomPar MoveRandomPar;
    xParCmdScale3rdPolyReg Scale3rdPolyReg;
    xParCmdAlpha3rdPolyReg Alpha3rdPolyReg;
    xParCmdClipVolumes ClipVolumes;
    xParCmdSmokeAlpha SmokeAlpha;
    xParCmdDamagePlayer DamagePlayer;
    xParCmdScale Scale;
    xParCmdAnimalMagentism AnimalMagentism;
    xParCmdRotPar RotPar;
    xParCmdApplyWind ApplyWind;
    xParCmdPlayerCollision PlayerCollision;
    xParCmdRandomVelocityPar RandomVelocityPar;
    xParCmdAccelerate Accelerate;
    xParCmdVelocityApply VelocityApply;
    xParCmdJet Jet;
    xParCmdKillOld KillOld;
    xParCmdKillSlow KillSlow;
    xParCmdKillDistance KillDistance;
    xParCmdAge Age;
    xParCmdFollow Follow;
    xParCmdOrbitLine OrbitLine;
    xParCmdOrbitPoint OrbitPoint;
    xParCmdApplyCamMat ApplyCamMat;
    xParCmdRotateAround RotateAround;
    xParCmdTex Tex;
    xParCmdTexAnim TexAnim;
    xParCmdCustom Custom;
    xParCmdCollideFall CollideFall;
    xParCmdCollideFallSticky CollideFallSticky;
    xParCmdDampenData DampenData;
    xParCmdAlphaInOutData AlphaInOutData;
    xParCmdSizeInOutData SizeInOutData;
    xParCmdShaperData ShaperData;
};

struct xParCmd
{
    U32 flag;
    xParCmdAsset* tasset;
};

typedef void(*xParCmdUpdateFunction)(xParCmd* c, xParGroup* ps, F32 dt);

U32 xParCmdGetSize(U32 parType);
xParCmdUpdateFunction xParCmdGetUpdateFunc(U32 parType);

#endif

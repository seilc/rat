#ifndef ZNPCPROJECTILEGRUBFX_H
#define ZNPCPROJECTILEGRUBFX_H

#include "xVec3.h"

struct xMat4x3;
struct xModelInstance;
class zNPCProjectile;

class zNPCProjectileGrubTrail
{
public:
    struct TrailConfig
    {
        F32 floor;
        F32 SMOKE_ALPHA_DELTA;
        F32 SMOKE_SIZE_DELTA;
        F32 SMOKE_START_SIZE;
        xVec3 SMOKE_VELOCITY;
        F32 SMOKE_EMIT_RATE;
        F32 SMOKE_ROTATION_DELTA_MIN;
        F32 SMOKE_ROTATION_DELTA_MAX;
        F32 SMOKE_COLOR;
        F32 VELOCITY_RANDOM;
        F32 START_ALPHA;
        F32 R_MULT;
        F32 G_MULT;
        F32 B_MULT;
        bool distance_emit;
    };

    static void SceneSetup();
    static void Emit(F32& emit_remaining, xVec3 position, const xVec3& velocity, F32 dt, F32 scale, TrailConfig* cfg);
};

class zNPCProjectileGrubSparks
{
public:
    struct SparkConfig
    {
        F32 lifeMin;
        F32 lifeMax;
        F32 sizeMin;
        F32 sizeMax;
        F32 emitRate;
        F32 rotateDeltaMin;
        F32 rotateDeltaMax;
        F32 velRandom;
        F32 posRandom;
        F32 swapTime;
        F32 endFadeIn;
        F32 startFadeOut;
        bool distance_emit;
    };

    static void Emit(F32& emit_remaining, xVec3 position, const xVec3& velocity, F32 dt, SparkConfig* cfg);
};

class zNPCProjectileGrubArc
{
public:
    struct ArcConfig
    {
        F32 lifeMin;
        F32 lifeMax;
        F32 sizeMin;
        F32 sizeMax;
        F32 emitRate;
        F32 swapTime;
        bool emitAll;
        bool attachToParent;
    };

    static void Emit(F32& emit_remaining, const ArcConfig* cfg, xModelInstance* model, zNPCProjectile* ownerGrub, F32 dt,
                     S32 sysID);
};

extern zNPCProjectileGrubSparks::SparkConfig burstSparkConfig;

void zNPCProjectileGrubFX_SceneSetup();
void zNPCProjectileGrubFX_SceneExit();
void zNPCProjectileGrubFX_SceneReset();
void zNPCProjectileGrubFX_Update(F32 dt);
void zNPCProjectileGrubFX_RenderModels();
void zNPCProjectileGrubHitExplosionEmit(const xMat4x3& mat);

#endif

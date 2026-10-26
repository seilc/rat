#ifndef ZFROZONEBLAST_H
#define ZFROZONEBLAST_H

#include "xMath3.h"
#include "xParticleBatch.h"

struct xLight;

// RESTORED: from RotU GC
class zFrozoneBlast
{
public:
    struct blastStream
    {
        xRay3 direction;
        F32 emitTimer;
        F32 initSize;
        S32 upgradeLevel;
        bool active;
        bool colliding;
        F32 velMultiplier;
        bool singleBlast;
        F32 singleTimer;
        F32 velocity;
        xLight* light;
        F32 lightIntensity;
        F32 lightTimer;
    };

    struct blastParticle
    {
        xVec3 loc;
        F32 lifetime;
        xVec3 dir;
        U8 flags;
        U8 r;
        U8 g;
        U8 b;
        F32 scaleX;
        F32 scaleY;
        F32 growthRateX;
        F32 growthRateY;
    };

    struct mistParticle
    {
        xVec3 loc;
        F32 lifetime;
        xVec3 dir;
        F32 scale;
    };

    static void SceneEnter();
    static void Reset();
    static void Update(F32 dt);
    static U32 ActivateStream(const xRay3* direction);
    static void UpdateStream(U32 id, const xRay3* newDirection, bool collision);
    static void StopStream(U32 id);
    static void SingleBlast(const xRay3* direction, F32 velocity, bool collision);
    static void SetPowerupLevel(S32 level);
    static void CollisionSplash(const xVec3* pos, const xVec3* normal, F32 dt);
    static void EmitBlastParticles(blastStream& stream, F32 dt);

    void addTweaks();
    void loadAssets();
    S32 createParticleSystem(S32 oldSystemId, U32, U32 srcBlend, U32 dstBlend, S32 stride,
                             xParticleBatchUpdateCallback update);
    static void updateParticle(blastParticle* p, F32 dt);
    static S32 updateBlastParticles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void*);
    void emitBlastParticles(blastStream& stream, F32 dt);
    void emitSingleBlastParticles(blastStream& stream, F32 dt);
    void updateLight(blastStream& stream, F32 dt);
    void _SceneEnter();
    void _Reset();
    void _Update(F32 dt);
    U32 _ActivateStream(const xRay3* direction);
    void _UpdateStream(U32 id, const xRay3* newDirection, bool collision);
    void _StopStream(U32 id);
    void _SingleBlast(const xRay3* direction, F32 velocity, bool collision);
    static S32 updateMistParticles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void*);
    void emitSplashMist(const xVec3* pos, const xVec3*, F32 dt);
    void emitSplashParticles(const xVec3* pos, const xVec3* normal, F32 dt);
    void _CollisionSplash(const xVec3* pos, const xVec3* normal, F32 dt);

private:
    S32 mSystemID;
    blastStream mStreams[3];
    S32 mMistSystemID;
    F32 mMistEmitTimer;
    S32 mGlintSystemID;
    F32 mGlintSplashTimer;
};
// END RESTORED

#endif

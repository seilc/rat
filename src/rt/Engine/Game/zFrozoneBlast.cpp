#include "zFrozoneBlast.h"

#include "iMath.h"
#include "xCam.h"
#include "xColor.h"
#include "xDebugTweak.h"
#include "xGlobals.h"
#include "xLight.h"
#include "xMemMgr.h"
#include "xParticleBatch.h"
#include "xPtankPool.h"
#include "xRand.h"
#include "xSpacePartitionTree.h"
#include "xstransvc.h"
#include "zGlobals.h"
#include "zParticleCustom.h"

#include "decomp.h"

// RESTORED: from RotU GC
namespace {

struct _blastConfig
{
    U32 alpha;
    F32 emit_rate;
    F32 growthRate;
    F32 randomEmitDir;
    F32 randomEmitLoc;
    F32 size;
    F32 velocity;
    F32 velocityRange;
    F32 blueTint;
    F32 blueTintDist;
};

xTWEAKABLE U32 texture = 0xDED04D47; // xStrHash("ice_stream_particle_01")
xTWEAKABLE U32 mistTexture = 0xDED04D47; // xStrHash("ice_stream_particle_01")
xTWEAKABLE U32 glintTexture = 0xDED04D47; // xStrHash("ice_stream_particle_01")
xTWEAKABLE F32 singleEmitTime = 0.03f;
xTWEAKABLE F32 singleVelocity = 20.0f;
xTWEAKABLE F32 singleSize = 0.3f;
xTWEAKABLE F32 singleRandomEmitLoc = 0.1f;
xTWEAKABLE F32 maxLightIntensity = 1.0f;
xTWEAKABLE F32 lightDecay = 0.8f;
xTWEAKABLE F32 lightSampleDelay = 0.5f;
xTWEAKABLE F32 lightRadius = 7.0f;
xTWEAKABLE F32 redMultiplier = 1.0f;
xTWEAKABLE F32 greenMultiplier = 1.0f;
xTWEAKABLE F32 angleStretch = 2.0f;
xTWEAKABLE F32 alphaMissFade = 0.2f;
xTWEAKABLE F32 alphaCollideFade = 0.0f;
xTWEAKABLE F32 mistEmitRate = 100.0f;
xTWEAKABLE U32 mistAlpha = 16;
xTWEAKABLE F32 mistLifetime = 1.0f;
xTWEAKABLE F32 mistGrowthRate = 3.0f;
xTWEAKABLE F32 mistMinSize = 0.5f;
xTWEAKABLE F32 mistVelocity = 1.0f;
xTWEAKABLE F32 mistFadeInTime = 0.3f;
xTWEAKABLE F32 glintVelocity = 7.0f;
xTWEAKABLE F32 glintEmitRate = 15.0f;
xTWEAKABLE F32 glintLength = 0.0f;
xTWEAKABLE F32 glintLengthRand = 0.1f;
xTWEAKABLE F32 glintLegnthMax = 0.1f;
xTWEAKABLE F32 glintLifetimeMin = 0.3f;
xTWEAKABLE F32 glintLifetimeRange = 0.1f;
xTWEAKABLE F32 glintWidth = 0.02f;
xTWEAKABLE F32 glintWidthRand = 0.0f;
xTWEAKABLE F32 glintGravity = 20.0f;
xTWEAKABLE F32 glintSplashDelay = 0.1f;
xTWEAKABLE F32 glintDirVary = PI / 2;

S32 sPowerupLevel;
zFrozoneBlast blastEffect;
_blastConfig blastConfig[3];

}

void zFrozoneBlast::SceneEnter()
{
    blastEffect._SceneEnter();
}

void zFrozoneBlast::Reset()
{
    blastEffect._Reset();
}

void zFrozoneBlast::Update(F32 dt)
{
    blastEffect._Update(dt);
}

U32 zFrozoneBlast::ActivateStream(const xRay3* direction)
{
    return blastEffect._ActivateStream(direction);
}

void zFrozoneBlast::UpdateStream(U32 id, const xRay3* newDirection, bool collision)
{
    blastEffect._UpdateStream(id, newDirection, collision);
}

void zFrozoneBlast::StopStream(U32 id)
{
    blastEffect._StopStream(id);
}

void zFrozoneBlast::SingleBlast(const xRay3* direction, F32 velocity, bool collision)
{
    blastEffect._SingleBlast(direction, velocity, collision);
}

void zFrozoneBlast::SetPowerupLevel(S32 level)
{
    xASSERT(__LINE__, level >= 0 && level < 3);
    sPowerupLevel = level;
}

void zFrozoneBlast::CollisionSplash(const xVec3* pos, const xVec3* normal, F32 dt)
{
    blastEffect._CollisionSplash(pos, normal, dt);
}

void zFrozoneBlast::EmitBlastParticles(blastStream& stream, F32 dt)
{
    blastEffect.emitBlastParticles(stream, dt);
}

void zFrozoneBlast::addTweaks()
{
}

void zFrozoneBlast::loadAssets()
{
    blastConfig[0].alpha = 32;
    blastConfig[0].blueTint = 0.0f;
    blastConfig[0].emit_rate = 2000.0f;
    blastConfig[0].growthRate = 10.0f;
    blastConfig[0].randomEmitDir = 2.0f;
    blastConfig[0].randomEmitLoc = 0.05f;
    blastConfig[0].size = 0.1f;
    blastConfig[0].blueTintDist = 1.0f;
    blastConfig[0].velocity = 50.0f;
    blastConfig[0].velocityRange = 0.5f;

    blastConfig[1].alpha = 24;
    blastConfig[1].blueTint = 192.0f;
    blastConfig[1].emit_rate = 2000.0f;
    blastConfig[1].growthRate = 10.0f;
    blastConfig[1].randomEmitDir = 2.5f;
    blastConfig[1].randomEmitLoc = 0.05f;
    blastConfig[1].size = 0.4f;
    blastConfig[1].blueTintDist = 5.0f;
    blastConfig[1].velocity = 50.0f;
    blastConfig[1].velocityRange = 0.5f;

    blastConfig[2].alpha = 24;
    blastConfig[2].blueTint = 255.0f;
    blastConfig[2].emit_rate = 2000.0f;
    blastConfig[2].growthRate = 10.0f;
    blastConfig[2].randomEmitDir = 4.0f;
    blastConfig[2].randomEmitLoc = 0.05f;
    blastConfig[2].size = 0.4f;
    blastConfig[2].blueTintDist = 5.0f;
    blastConfig[2].velocity = 50.0f;
    blastConfig[2].velocityRange = 0.5f;

    mSystemID = createParticleSystem(mSystemID, texture, 5, 6, sizeof(blastParticle), updateBlastParticles);
    xASSERT(__LINE__, mSystemID != -1);
    mMistSystemID = createParticleSystem(mMistSystemID, mistTexture, 5, 6, sizeof(mistParticle), updateMistParticles);
    xASSERT(__LINE__, mMistSystemID != -1);
    zParticleCustomCreateGlintSystem(mGlintSystemID, glintTexture, "Frozone Blast Splash");
}

S32 zFrozoneBlast::createParticleSystem(S32 oldSystemId, U32, U32 srcBlend, U32 dstBlend, S32 stride,
                                        S32 (*update)(U8*, S32, ptank_pool&, F32, void*))
{
    xParticleBatchSystem system;
    system.pool_type = PGT_POS_COLOR_SIZE;
    system.rs.texture = texture ? (RwTexture*)xSTFindAsset(texture, NULL) : NULL;
    system.rs.src_blend = srcBlend;
    system.rs.dst_blend = dstBlend;
    system.rs.flags = 4;
    system.order_group = 'HOBO';
    system.order_index = 7;
    system.stride = stride;
    system.update = update;
    system.context = NULL;

    if (oldSystemId == -1) {
        return xPARTICLEBATCHADDSYSTEM(system, __LINE__);
    }

    xParticleBatchReplaceSystem(oldSystemId, system);
    return oldSystemId;
}

void zFrozoneBlast::updateParticle(blastParticle* p, F32 dt)
{
    p->lifetime -= dt;
    p->scaleX += p->growthRateX * dt;
    p->scaleY += p->growthRateY * dt;
    p->loc.AddScale(p->dir, dt);
}

S32 zFrozoneBlast::updateBlastParticles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void*)
{
    ptank_pool__pos_color_size& pool = (ptank_pool__pos_color_size&)apool;
    blastParticle* p = (blastParticle*)mem;
    blastParticle* end = p + count;

    while (p != end) {
        updateParticle(p, dt);

        if (p->lifetime < 0.0f) {
            end--;
            *p = *end;
            continue;
        }

        RwSphere worldsph;
        worldsph.center = p->loc.m_RwV3d;
        worldsph.radius = p->scaleX > p->scaleY ? p->scaleX : p->scaleY;

        if (RwCameraFrustumTestSphere(xglobals->screen->icam, &worldsph) != rwSPHEREOUTSIDE) {
            pool.next();
            if (!pool.valid()) {
                end = p;
                break;
            }

            *pool.pos = p->loc;
            pool.size->assign(p->scaleX, p->scaleY);

            S32 level = p->flags & 0x3;
            F32 fadeTime = (p->flags & 0x4) ? alphaCollideFade : alphaMissFade;
            U32 alpha = blastConfig[level].alpha;
            if (p->lifetime < fadeTime) {
                alpha = alpha * p->lifetime / fadeTime;
            }

            xColorInit(*pool.color, p->r, p->g, p->b, alpha);
        }

        p++;
    }

    return end - (blastParticle*)mem;
}

void zFrozoneBlast::emitBlastParticles(blastStream& stream, F32 dt)
{
    S32 powerupLevel = stream.upgradeLevel;
    stream.emitTimer += dt * blastConfig[powerupLevel].emit_rate;

    S32 emitted = stream.emitTimer;
    if (emitted <= 0) {
        return;
    }

    stream.emitTimer -= emitted;

    F32 randomEmitDir2 = 2.0f * (blastConfig[powerupLevel].randomEmitDir * blastConfig[powerupLevel].randomEmitDir);

    const xMat4x3& viewMat = *(const xMat4x3*)RwCameraGetViewMatrix(globals.screen->icam);
    xVec3 screenVec;
    xMat3x3RMulVec(&screenVec, &viewMat, &stream.direction.dir);

    F32 lengthX = angleStretch * (screenVec.x > 0.0f ? screenVec.x : -screenVec.x) + 1.0f;
    F32 lengthY = angleStretch * (screenVec.y > 0.0f ? screenVec.y : -screenVec.y) + 1.0f;

    while (emitted != 0) {
        U8* mem;
        S32 have = xParticleBatchEmit(mSystemID, emitted, &mem);
        if (have == 0) {
            break;
        }

        emitted -= have;

        blastParticle* part = (blastParticle*)mem;
        blastParticle* endp = part + have;
        while (part != endp) {
            part->flags = powerupLevel;
            if (stream.colliding) {
                part->flags |= 0x4;
            }

            F32 vel = stream.velMultiplier * (blastConfig[powerupLevel].velocityRange * xrand_RandomBipolarUnitFloat() +
                                              blastConfig[powerupLevel].velocity);
            part->lifetime = stream.direction.max_t / vel;

            part->loc.x = blastConfig[powerupLevel].randomEmitLoc * xrand_RandomBipolarUnitFloat() + stream.direction.origin.x;
            part->loc.y = blastConfig[powerupLevel].randomEmitLoc * xrand_RandomBipolarUnitFloat() + stream.direction.origin.y;
            part->loc.z = blastConfig[powerupLevel].randomEmitLoc * xrand_RandomBipolarUnitFloat() + stream.direction.origin.z;

            xVec3 randDir = { 0.0f, 0.0f, 0.0f };
            randDir.x = blastConfig[powerupLevel].randomEmitDir * xrand_RandomBipolarUnitFloat();
            randDir.y = blastConfig[powerupLevel].randomEmitDir * xrand_RandomBipolarUnitFloat();
            randDir.z = blastConfig[powerupLevel].randomEmitDir * xrand_RandomBipolarUnitFloat();

            part->dir.Scale(stream.direction.dir, stream.direction.max_t / part->lifetime);
            part->dir.Add(randDir);

            F32 distance = xVec3Length2(&randDir) / randomEmitDir2;
            F32 colorStrength = 1.0f - distance * distance * blastConfig[powerupLevel].blueTintDist;
            if (colorStrength < 0.0f) {
                colorStrength = 0.0f;
            }

            part->r = (U32)(255.0f - colorStrength * blastConfig[powerupLevel].blueTint * redMultiplier);
            part->g = (U32)(255.0f - colorStrength * blastConfig[powerupLevel].blueTint * greenMultiplier);
            part->b = 255;

            F32 growth = stream.velMultiplier * (blastConfig[powerupLevel].growthRate * xrand_GenRandFloat());
            part->growthRateX = growth * lengthX;
            part->growthRateY = growth * lengthY;
            part->scaleX = stream.initSize * (lengthX * blastConfig[powerupLevel].size);
            part->scaleY = stream.initSize * (lengthY * blastConfig[powerupLevel].size);

            F32 randomDT = -dt * xrand_GenRandFloat();
            randomDT += 0.3f * (part->scaleX / vel);
            updateParticle(part, randomDT);

            part++;
        }
    }
}

void zFrozoneBlast::emitSingleBlastParticles(blastStream& stream, F32 dt)
{
    F32 dTime = dt;
    if (stream.singleBlast && stream.singleTimer < dt) {
        dTime = stream.singleTimer;
        stream.active = false;
    }

    stream.singleTimer -= dt;
    stream.emitTimer += dTime * blastConfig[sPowerupLevel].emit_rate;

    S32 emitted = stream.emitTimer;
    if (emitted <= 0) {
        return;
    }

    stream.emitTimer -= emitted;

    while (emitted != 0) {
        U8* mem;
        S32 have = xParticleBatchEmit(mSystemID, emitted, &mem);
        if (have == 0) {
            break;
        }

        emitted -= have;

        blastParticle* part = (blastParticle*)mem;
        blastParticle* endp = part + have;
        while (part != endp) {
            part->flags = 0x8;
            if (stream.colliding) {
                part->flags |= 0x4;
            }

            part->lifetime = stream.direction.max_t / stream.velocity;

            F32 randomDT = dTime * xrand_GenRandFloat();
            F32 offset = singleRandomEmitLoc * (stream.singleTimer + dt - (dTime - randomDT)) / singleEmitTime;

            part->loc.x = offset * xrand_RandomBipolarUnitFloat() + stream.direction.origin.x;
            part->loc.y = offset * xrand_RandomBipolarUnitFloat() + stream.direction.origin.y;
            part->loc.z = offset * xrand_RandomBipolarUnitFloat() + stream.direction.origin.z;

            part->dir.Scale(stream.direction.dir, stream.direction.max_t / part->lifetime);

            part->growthRateX = part->growthRateY = 0.0f;
            part->scaleX = part->scaleY = singleSize;
            part->r = 255;
            part->g = 255;
            part->b = 255;

            updateParticle(part, -dTime + randomDT);

            part++;
        }
    }
}

void zFrozoneBlast::updateLight(blastStream& stream, F32 dt)
{
    if (!stream.light) {
        if (!stream.active) {
            return;
        }

        stream.light = xLightCreateTemp(false, __LINE__);
        if (!stream.light) {
            return;
        }

        stream.lightTimer = 0.0f;
        xLightSetRadius(stream.light, lightRadius);
        xLightOn(stream.light, true, 0.0f);
    }

    stream.lightIntensity -= lightDecay * dt;
    if (stream.lightIntensity <= 0.0f) {
        if (!stream.active) {
            xLightDestroyTemp(stream.light, __LINE__);
            stream.light = NULL;
            return;
        }

        stream.lightIntensity = 0.0f;
    }

    stream.lightTimer -= dt;
    if (stream.lightTimer <= 0.0f && stream.active) {
        stream.lightTimer = lightSampleDelay * xrand_GenRandFloat();

        F32 newIntensity = maxLightIntensity * xrand_GenRandFloat();
        if (newIntensity > stream.lightIntensity) {
            stream.lightIntensity = newIntensity;
        }
    }

    xLightSetColor(stream.light, stream.lightIntensity, stream.lightIntensity, stream.lightIntensity, 0.0f);

    xVec3 lightPos = stream.direction.origin;
    lightPos.AddScale(stream.direction.dir, 0.5f * stream.direction.max_t);
    xLightSetPos(stream.light, lightPos);
}

void zFrozoneBlast::_SceneEnter()
{
    mSystemID = -1;
    mMistSystemID = -1;
    loadAssets();
    addTweaks();
    _Reset();
}

void zFrozoneBlast::_Reset()
{
    for (S32 i = 0; i < 3; i++) {
        mStreams[i].active = false;
        mStreams[i].light = NULL;
    }
}

void zFrozoneBlast::_Update(F32 dt)
{
    for (S32 i = 0; i < 3; i++) {
        if (mStreams[i].active) {
            if (mStreams[i].singleBlast) {
                emitSingleBlastParticles(mStreams[i], dt);
            } else {
                emitBlastParticles(mStreams[i], dt);
            }
        }

        updateLight(mStreams[i], dt);
    }
}

U32 zFrozoneBlast::_ActivateStream(const xRay3* direction)
{
    for (U32 i = 0; i < 3; i++) {
        blastStream& s = mStreams[i];
        if (!s.active) {
            if (direction) {
                s.direction = *direction;
                F32 length = s.direction.dir.normalize();
                s.direction.max_t *= length;
            }

            s.active = true;
            s.singleBlast = false;
            s.initSize = 1.0f;
            s.upgradeLevel = sPowerupLevel;
            s.velMultiplier = 1.0f;
            return i + 1000;
        }
    }

    xFAIL(__LINE__);
    return 0;
}

void zFrozoneBlast::_UpdateStream(U32 id, const xRay3* newDirection, bool collision)
{
    DECOMP_FORCEREGISTER(collision);

    U32 blastID = id - 1000;
    xASSERT(__LINE__, blastID < 3);
    blastStream& s = mStreams[blastID];
    s.direction = *newDirection;
    s.colliding = true;
    F32 length = s.direction.dir.safe_normalize(xVec3::m_UnitAxisZ);
    s.direction.max_t *= length;
}

void zFrozoneBlast::_StopStream(U32 id)
{
    U32 blastID = id - 1000;
    xASSERT(__LINE__, blastID < 3);
    mStreams[blastID].active = false;
}

void zFrozoneBlast::_SingleBlast(const xRay3* direction, F32 velocity, bool collision)
{
    DECOMP_FORCEREGISTER(collision);

    xASSERT(__LINE__, direction);

    if (velocity < 0.0f) {
        velocity = singleVelocity;
    }

    for (U32 i = 0; i < 3; i++) {
        blastStream& s = mStreams[i];
        if (!s.active) {
            s.active = true;
            s.singleBlast = true;
            s.singleTimer = singleEmitTime * singleVelocity / velocity;
            s.colliding = true;
            s.velocity = velocity;
            s.velMultiplier = 1.0f;
            s.direction = *direction;
            F32 length = s.direction.dir.normalize();
            s.direction.max_t *= length;
            return;
        }
    }
}

S32 zFrozoneBlast::updateMistParticles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void*)
{
    ptank_pool__pos_color_size& pool = (ptank_pool__pos_color_size&)apool;
    mistParticle* p = (mistParticle*)mem;
    mistParticle* end = p + count;

    while (p != end) {
        p->lifetime -= dt;
        if (p->lifetime < 0.0f) {
            end--;
            *p = *end;
            continue;
        }

        p->scale += mistGrowthRate * dt;
        p->loc.AddScale(p->dir, dt);

        RwSphere worldsph;
        worldsph.center = p->loc.m_RwV3d;
        worldsph.radius = p->scale;

        if (RwCameraFrustumTestSphere(xglobals->screen->icam, &worldsph) != rwSPHEREOUTSIDE) {
            pool.next();
            if (!pool.valid()) {
                end = p;
                break;
            }

            *pool.pos = p->loc;
            pool.size->assign(p->scale, p->scale);
            *pool.color = g_WHITE;

            F32 alpha;
            if (mistLifetime - p->lifetime < mistFadeInTime) {
                alpha = mistAlpha * ((mistLifetime - p->lifetime) / mistFadeInTime);
            } else {
                alpha = mistAlpha * (p->lifetime / (mistLifetime - mistFadeInTime));
            }

            pool.color->a = (U32)alpha;
        }

        p++;
    }

    return end - (mistParticle*)mem;
}

void zFrozoneBlast::emitSplashMist(const xVec3* pos, const xVec3*, F32 dt)
{
    mMistEmitTimer += mistEmitRate * dt;

    S32 emitted = mMistEmitTimer;
    if (emitted <= 0) {
        return;
    }

    mMistEmitTimer -= emitted;

    while (emitted != 0) {
        U8* mem;
        S32 have = xParticleBatchEmit(mMistSystemID, emitted, &mem);
        if (have == 0) {
            break;
        }

        emitted -= have;

        mistParticle* part = (mistParticle*)mem;
        mistParticle* endp = part + have;
        while (part != endp) {
            part->lifetime = mistLifetime;
            part->loc = *pos;

            F32 ang = 2.0f * PI * xrand_GenRandFloat();
            F32 uz = 2.0f * xrand_GenRandFloat() - 1.0f;
            F32 r = xsqrt(1.0f - uz * uz);
            part->dir.assign(r * icos(ang), r * isin(ang), uz);
            part->dir.Scale(mistVelocity * xrand_RandomBipolarUnitFloat());

            part->scale = mistMinSize;

            part++;
        }
    }
}

void zFrozoneBlast::emitSplashParticles(const xVec3* pos, const xVec3* normal, F32 dt)
{
    mGlintSplashTimer -= dt;
    if (mGlintSplashTimer > 0.0f || !normal || normal->length2() < 0.25f) {
        return;
    }

    mGlintSplashTimer = glintSplashDelay * xrand_GenRandFloat();

    S32 num_particles = zParticleCustomRequestGlint(glintEmitRate, mGlintSystemID);
    if (num_particles <= 0) {
        return;
    }

    xVec3* locList = (xVec3*)xMEMPUSHTEMP(num_particles * sizeof(xVec3), 0, __LINE__);
    xVec3* dirList = (xVec3*)xMEMPUSHTEMP(num_particles * sizeof(xVec3), 0, __LINE__);
    F32* lenVelList = (F32*)xMEMPUSHTEMP(num_particles * sizeof(F32), 0, __LINE__);

    F32 cosangle = icos(glintDirVary);
    xMat3x3 dirMat;
    xMat3x3LookVec(&dirMat, normal);

    S32 particleCounter;
    xVec3* locPtr = locList;
    xVec3* dirPtr = dirList;
    F32* lenPtr = lenVelList;
    for (particleCounter = num_particles; particleCounter > 0; particleCounter--) {
        *locPtr = *pos;
        locPtr++;

        F32 ang = 2.0f * PI * xurand();
        F32 uz = (1.0f - cosangle) * xurand() + cosangle;
        F32 r = xsqrt(1.0f - uz * uz);
        dirPtr->assign(r * icos(ang), r * isin(ang), -uz);
        xMat3x3RMulVec(dirPtr, &dirMat, dirPtr);
        dirPtr->Scale(glintVelocity);
        dirPtr++;

        F32 length = glintLength + glintLengthRand * xrand_GenRandFloat();
        *lenPtr = length < glintLegnthMax ? length : glintLegnthMax;
        lenPtr++;
    }

    zParticleCustomEmitGlint(locList, dirList, lenVelList, num_particles, glintLifetimeMin, glintLifetimeRange,
                             glintWidth, glintWidthRand, glintLength, glintLegnthMax, glintLengthRand,
                             glintLengthRand, dt, glintGravity, mGlintSystemID);

    xMEMPOPTEMP(lenVelList);
    xMEMPOPTEMP(dirList);
    xMEMPOPTEMP(locList);
}

void zFrozoneBlast::_CollisionSplash(const xVec3* pos, const xVec3* normal, F32 dt)
{
    xASSERT(__LINE__, pos);
    xASSERT(__LINE__, normal);

    emitSplashMist(pos, normal, dt);
    emitSplashParticles(pos, normal, dt);
}
// END RESTORED

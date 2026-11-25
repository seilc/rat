#ifndef ZPARSYSSNOWSPLOSION_H
#define ZPARSYSSNOWSPLOSION_H

#include "types.h"

struct xVec3;

struct zParSysSnowsplosionConfig
{
    F32 minLife;
    F32 maxLife;
    F32 minSize;
    F32 maxSize;
    F32 minSpeed;
    F32 maxSpeed;
    F32 deceleration;
    F32 fadeStartTime;
    F32 speedDown;
    U8 minParticles;
    U8 maxParticles;
    U8 startAlpha;
    bool spawnDown;

#if DEBUG || RELEASE
    void AddTweaks(const char* prefix);
#endif
};

void zParSysSnowsplosionSceneSetup();
void zParSysSnowsplosionEmit(const xVec3& pos, const zParSysSnowsplosionConfig& config);

#endif

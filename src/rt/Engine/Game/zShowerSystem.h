#ifndef ZSHOWERSYSTEM_H
#define ZSHOWERSYSTEM_H

#include "zLightWeight.h"
#include "xShadowSimple.h"
#include "containers.h"
#include "xFX.h"

struct xEnt;
struct xVec3;
struct xLightKit;
struct xModelInstance;
struct xCameraFXShakeParams;
struct zShrapnelAsset;
class zFXShockwaveConfig;

// RESTORED
class zShowerCBInterface
{
public:
    virtual void Collide(const xVec3& pos) = 0;
};

class zShowerParticle
{
public:
    struct ConfigType
    {
        zFXShockwaveConfig* shockwaveConfig;
#if DEBUG || RELEASE
        xCameraFXShakeParams* cameraShakeParams;
#else
        const xCameraFXShakeParams* cameraShakeParams;
#endif
        U32 modelID;
        U32 shrapnelID;
        F32 shrapnelSpeedY;
        F32 radius;
        F32 damageRadius;
        U16 damage;
    };

    void Render();
    void Setup(const ConfigType* cfg, xLightKit* lightKit);
    void Exit();
    void Update(pool_list<zShowerParticle>& list, xEnt* owner, zShowerCBInterface* collideCallback, F32 dt);
#if DEBUG || RELEASE
    void Start(const xVec3& pos, F32 fallSpeed, const xFXRibbon* tweakRibbon);
#else
    void Start(const xVec3& pos, F32 fallSpeed);
#endif

protected:
    xShadowSimpleCache shadow;
    F32 speed;
    F32 time;
    const ConfigType* config;
    xModelInstance* model;
    zShrapnelAsset* shrapnel;
    xFXRibbon* ribbon;
    bool alive;
};

class zShowerSystem : public zLightweightSystemBase
{
public:
    struct ConfigType
    {
        zShowerParticle::ConfigType particleConfig;
        S32 maxParticles;
        F32 minRadius;
        F32 maxRadius;
        F32 showerTime;
        F32 minSpawnTime;
        F32 maxSpawnTime;
        F32 minSpeed;
        F32 maxSpeed;
        F32 endStormTime;
        F32 endSpawnTime;
        F32 stormDamageRadius;
    };

#if DEBUG || RELEASE
    void Setup(xEnt* moveOwner, ConfigType* cfg);
#else
    void Setup(xEnt* moveOwner, const ConfigType* cfg);
#endif
    void Reset();
    void Start(const xVec3& pos, zShowerCBInterface* collideCB);
    void ResetSpawnTimer();
    void Update(F32 dt);
    void Render();
    void Exit();

protected:
#if DEBUG || RELEASE
    ConfigType* config;
#else
    const ConfigType* config;
#endif
    pool_list<zShowerParticle> particleList;
    xVec3 center;
    F32 effectTimer;
    F32 spawnTimer;
    xEnt* owner;
    zShowerCBInterface* collideCallback;
#if DEBUG || RELEASE
    bool renderBounds;
    xFXRibbon tweakRibbon;
#endif
};
// END RESTORED

#endif

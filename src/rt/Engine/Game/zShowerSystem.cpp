#include "zShowerSystem.h"

#include "xCam.h"
#include "xCollideSweptSphere.h"
#include "xDebugTweak.h"
#include "xFX.h"
#include "xMath.h"
#include "xModel.h"
#include "xRand.h"
#include "xScrFx.h"
#include "xstransvc.h"
#include "iMath.h"
#include "zBomb.h"
#include "zFXRibbonPool.h"
#include "zFXShockwave.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zShrapnel.h"

// RESTORED: from RotU GC
namespace {
const xFXRibbon::curve_node RIBBON_CURVE[] = {
    { 0.0f, { 0xFF, 0xFF, 0xFF, 0x20 }, 1.0f },
    { 1.0f, { 0xFF, 0xFF, 0xFF, 0x00 }, 0.0f },
};

xFXRibbon::config_type RIBBON_CONFIG = { 0.15f, rwBLENDSRCALPHA, rwBLENDONE, 0.5f, 0, RIBBON_CURVE, 2, NULL };

xTWEAKABLE F32 TIMEOUT = 5.0f;
xTWEAKABLE F32 Y_COLLISION_START = 7.0f;
xTWEAKABLE F32 Y_HEIGHT_SHIFT = 15.0f;

xLightKit* GetGlobalLightKit();
}

void zShowerParticle::Render()
{
    if (alive) {
        xModelRender(model);
    }
}

void zShowerParticle::Setup(const ConfigType* cfg, xLightKit* lightKit)
{
    config = cfg;

    RpAtomic* iModel = xModelFindAtomic(config->modelID, NULL);
    if (iModel) {
        model = xModelInstanceAlloc(iModel, NULL, 0, 0, NULL);
        model->modelID = config->modelID;
        xModelSetLightKit(model, lightKit, 0.0f);
    }

    xShadowSimple_CacheInit(&shadow, 0x50);

    if (config->shrapnelID) {
        shrapnel = (zShrapnelAsset*)xSTFindAsset(config->shrapnelID, NULL);
    } else {
        shrapnel = NULL;
    }

    ribbon = NULL;
}

void zShowerParticle::Exit()
{
    if (ribbon) {
        zFXRibbonPoolDelete(ribbon);
    }
    xModelInstanceFree(model, true);
    model = NULL;
}

void zShowerParticle::Update(pool_list<zShowerParticle>& list, xEnt* owner, zShowerCBInterface* collideCallback, F32 dt)
{
    if (time >= TIMEOUT) {
        alive = false;
    }

    if (alive) {
        time += dt;

        xVec3 end = { model->Mat->pos.x, model->Mat->pos.y - speed * dt, model->Mat->pos.z };

        if (time * speed >= 8.0f) {
            xSweptSphere sws((xVec3&)model->Mat->pos, end, config->radius, 0);

            F32 remainingTime = Y_HEIGHT_SHIFT / speed - time;
            F32 collisionStartY = Y_COLLISION_START + (end.y - speed * remainingTime);
            if (sws.start.y > collisionStartY) {
                sws.start.y = collisionStartY;
            }

            S32 result = xSweptSphereToScene(&sws, globals.sceneCur, NULL, 0x10, 0);
            result |= xSweptSphereToPlayer(&sws, globals.sceneCur, NULL, 0x10, 0);
            if (result) {
                xSweptSphereGetResults(&sws);

                sphere_damage(owner, sws.worldContact, config->damageRadius, config->damage, 8.0f, 8.0f, zHS_NUKE_FRO, zHT_GENERAL);

                if (shrapnel) {
                    xVec3Copy((xVec3*)&model->Mat->pos, &sws.worldContact);
                    xVec3 velocity = { 0.0f, config->shrapnelSpeedY, 0.0f };
                    shrapnel->initCB(shrapnel, model, NULL, &velocity, NULL);
                }

                if (config->shockwaveConfig) {
                    zFXShockwaveStart(config->shockwaveConfig, sws.worldContact, 1.0f, false);
                }

                if (collideCallback) {
                    collideCallback->Collide(sws.worldContact);
                }

                if (config->cameraShakeParams) {
                    xCameraFXShake(*config->cameraShakeParams, &sws.worldContact, NULL);
                }

                if (ribbon) {
                    ribbon->insert(sws.worldPos, globals.cam->mat.right, config->radius, 1.0f, 0);
                }

                alive = false;
                return;
            }
        }

        F32 shadowRadius = config->radius * xmin(1.0f, time * speed / Y_HEIGHT_SHIFT);
        xShadowSimple_Add(&shadow, (xMat4x3*)model->Mat, shadowRadius, 1.0f);

        xVec3Copy((xVec3*)&model->Mat->pos, &end);

        if (ribbon) {
            ribbon->insert((xVec3&)model->Mat->pos, globals.cam->mat.right, config->radius, 1.0f, 0);
        }
    } else if (!ribbon || ribbon->empty()) {
        zFXRibbonPoolDelete(ribbon);
        pool_list<zShowerParticle>::node_type* itr = list.get_iterator(this);
        list.erase(itr);
    }
}

#if DEBUG || RELEASE
void zShowerParticle::Start(const xVec3& pos, F32 fallSpeed, const xFXRibbon* tweakRibbon)
#else
void zShowerParticle::Start(const xVec3& pos, F32 fallSpeed)
#endif
{
    time = 0.0f;
    speed = fallSpeed;
    xVec3Copy((xVec3*)&model->Mat->pos, &pos);
    alive = true;
    ribbon = zFXRibbonPoolNew();
#if DEBUG || RELEASE
    tweakRibbon->copy_parameters(ribbon);
#else
    ribbon->set_config(&RIBBON_CONFIG);
#endif
}

#if DEBUG || RELEASE
void zShowerSystem::Setup(xEnt* moveOwner, ConfigType* cfg)
#else
void zShowerSystem::Setup(xEnt* moveOwner, const ConfigType* cfg)
#endif
{
    owner = moveOwner;
    config = cfg;

    zLightweightManager::RegisterLightweight(this);

    xLightKit* globalLightKit = GetGlobalLightKit();

    particleList.create(config->maxParticles, eMemMgrTag_Shower, NULL);

    pool_list<zShowerParticle>::node_type* currentNode = particleList.get_buffer();
    pool_list<zShowerParticle>::node_type* const endNode = currentNode + particleList.max_size();
    while (currentNode != endNode) {
        currentNode->value.Setup(&config->particleConfig, globalLightKit);
        currentNode++;
    }

    RIBBON_CONFIG.raster = xFXRibbon::get_raster("fx_streak1");

    Reset();
}

namespace {
xLightKit* GetGlobalLightKit()
{
    zEnv* zen = globals.sceneCur->zen;
    if (globals.sceneCur->sceneID != 'MNUS') {
        xASSERT(__LINE__, zen->easset);
        return (xLightKit*)xSTFindAsset(zen->easset->objectLightKit, NULL);
    }
    return NULL;
}
}

void zShowerSystem::Reset()
{
    particleList.clear();
    spawnTimer = HUGE;
    effectTimer = HUGE;
}

void zShowerSystem::Start(const xVec3& pos, zShowerCBInterface* collideCB)
{
    collideCallback = collideCB;
    xVec3Copy(&center, &pos);
    effectTimer = config->showerTime;
    ResetSpawnTimer();
}

void zShowerSystem::ResetSpawnTimer()
{
    if (effectTimer <= config->endStormTime) {
        spawnTimer = config->endSpawnTime;
    } else {
        F32 percentComplete = (effectTimer - config->endStormTime) / (config->showerTime - config->endStormTime);
        spawnTimer = percentComplete * (config->maxSpawnTime - config->minSpawnTime) + config->minSpawnTime;
    }
}

void zShowerSystem::Update(F32 dt)
{
    if (config->stormDamageRadius > 0.0f && effectTimer >= config->endStormTime && effectTimer - dt < config->endStormTime) {
        sphere_damage(owner, center, config->stormDamageRadius, config->particleConfig.damage, 8.0f, 8.0f, zHS_NUKE_FRO, zHT_GENERAL);
    }

    spawnTimer -= dt;
    effectTimer -= dt;

    if (spawnTimer <= 0.0f) {
        F32 radius = xrand_RandomFloatRange(config->minRadius, config->maxRadius);
        F32 angle = xrand_RandomFloatRange(0.0f, 2 * PI);
        xVec3 position = { radius * icos(angle) + center.x, Y_HEIGHT_SHIFT + center.y, radius * isin(angle) + center.z };

        if (particleList.size() < particleList.max_size()) {
            particleList.push_front();
            F32 speed = xrand_RandomFloatRange(config->minSpeed, config->maxSpeed);
#if DEBUG || RELEASE
            particleList.front().Start(position, speed, &tweakRibbon);
#else
            particleList.front().Start(position, speed);
#endif

            if (effectTimer > 0.0f) {
                ResetSpawnTimer();
            } else {
                spawnTimer = HUGE;
            }
        }
    }

    pool_list<zShowerParticle>::node_type* itr = particleList.begin();
    pool_list<zShowerParticle>::node_type* end = particleList.end();
    pool_list<zShowerParticle>::node_type* itrNext;
    for (; itr != end; itr = itrNext) {
        itrNext = itr->next;
        itr->value.Update(particleList, owner, collideCallback, dt);
    }
}

void zShowerSystem::Render()
{
    pool_list<zShowerParticle>::node_type* itr = particleList.begin();
    pool_list<zShowerParticle>::node_type* end = particleList.end();
    for (; itr != end; itr = itr->next) {
        itr->value.Render();
    }
}

void zShowerSystem::Exit()
{
    pool_list<zShowerParticle>::node_type* currentNode = particleList.get_buffer();
    pool_list<zShowerParticle>::node_type* const endNode = currentNode + particleList.max_size();
    while (currentNode != endNode) {
        currentNode->value.Exit();
        currentNode++;
    }
}
// END RESTORED

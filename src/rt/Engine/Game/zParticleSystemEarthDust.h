#ifndef ZPARTICLESYSTEMEARTHDUST_H
#define ZPARTICLESYSTEMEARTHDUST_H

#include "xParticleBatch.h"
#include "zParticleGenerator.h"
#include "zParticleLocator.h"

struct xModelInstance;
struct zParticleGeneratorEarthDust;
struct zShrapnelAsset;

class zParticleSystemEarthDust : public zParticleSystem
{
    xParticleBatchGroup batch_group;

public:
    struct particle_instance
    {
        xVec3 loc;
        F32 age;
        xVec3 vel;
        U32 flags;
        F32 rotation;
        F32 rotSpeed;
        F32 pad[2];
    };

    struct asset_data
    {
        F32 gravity;
        F32 drift;
        F32 size;
        F32 rotSpeed;
        F32 lifetime;
        F32 birthAlpha;
        F32 deathAlpha;
        struct
        {
            U32 shrapID;
            F32 shrapEmitRate;
            F32 initDownVel;
            F32 randVel;
        } shrap;
    };

    zParticleSystemEarthDust() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_EARTH_DUST, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "EarthDust") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
    static S32 update_particles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void* context);

    friend struct zParticleGeneratorEarthDust;
};

struct zParticleGeneratorEarthDust : zParticleGenerator
{
    void update(F32 dt);
    void create(zParticleSystemEarthDust& system, zParticleAsset& asset);
    void reset_attach_to(xEnt* new_attach_to); // UNUSED
    virtual void reset();
    virtual void activate(xEnt*);
    virtual void deactivate(xEnt*);
    virtual bool cullUpdate();
#if DEBUG || RELEASE
    virtual void add_tweaks(const char* prefix);
    virtual void debug_render(bool all);
#endif

protected:
#if DEBUG || RELEASE
    struct debug
    {
        S32 attemptedEmit;
        S32 failedEmit;
    };
#endif

    zParticleSystemEarthDust* system;
    xEnt* attach_to;
    F32 emitted;
    zParticleLocator locator;
    S32 batch_id;
    F32 shrapEmitted;
    zShrapnelAsset* shrapAsset;
    xModelInstance* tempModel;
#if DEBUG || RELEASE
    debug debugInfo;
#endif

    friend class zParticleSystemEarthDust;
};

#endif

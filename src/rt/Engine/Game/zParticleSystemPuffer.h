#ifndef ZPARTICLESYSTEMPUFFER_H
#define ZPARTICLESYSTEMPUFFER_H

#include "xParticleBatch.h"
#include "zParticleGenerator.h"

struct zParticleGeneratorPuffer;

class zParticleSystemPuffer : public zParticleSystem
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
        F32 minTimeOn;
        F32 maxTimeOn;
        F32 minTimeOff;
        F32 maxTimeOff;
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
            S32 numShrap;
            F32 initDownVel;
            F32 randVel;
        } shrap;
    };

    zParticleSystemPuffer() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_PUFFER, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "Puffer") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
    static S32 update_particles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void* context);

    friend struct zParticleGeneratorPuffer;
};

#endif

#ifndef ZPARTICLESYSTEMBURROWKICKUP_H
#define ZPARTICLESYSTEMBURROWKICKUP_H

#include "xParticleBatch.h"
#include "zParticleGenerator.h"

struct zParticleGeneratorBurrowKickup;

class zParticleSystemBurrowKickup : public zParticleSystem
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
        F32 size;
        F32 rotSpeed;
        F32 lifetime;
        F32 maxAlpha;
        F32 initUpVel;
        F32 randOutVel;
        struct
        {
            U32 shrapID;
            F32 shrapEmitRate;
            F32 initUpVel;
            F32 initOutVel;
        } shrap;
    };

    zParticleSystemBurrowKickup() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_BURROW_KICKUP, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "BurrowKickup") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
    static S32 update_particles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void* context);

    friend struct zParticleGeneratorBurrowKickup;
};

#endif

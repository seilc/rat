#ifndef ZPARTICLESYSTEMRAIN_H
#define ZPARTICLESYSTEMRAIN_H

#include "xParticleBatch.h"
#include "zParticleGenerator.h"

struct zParticleGeneratorRain;

class zParticleSystemRain : public zParticleSystem
{
    xParticleBatchGroup batch_group;

public:
    struct particle_instance
    {
        xVec3 loc;
        F32 yOrig;
    };

    struct asset_data
    {
        F32 gravity;
        F32 dropWidth;
        F32 dropLength;
    };

    zParticleSystemRain() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_RAIN, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "Rain") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
    static S32 update_particles(U8* mem, S32 count, ptank_pool& apool, F32 dt, void* context);

    friend struct zParticleGeneratorRain;
};

#endif

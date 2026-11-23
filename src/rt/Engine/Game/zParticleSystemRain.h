#ifndef ZPARTICLESYSTEMRAIN_H
#define ZPARTICLESYSTEMRAIN_H

#include "xParticleBatch.h"
#include "zParticleGenerator.h"

class zParticleSystemRain : public zParticleSystem
{
    xParticleBatchGroup batch_group;

public:
    zParticleSystemRain() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_RAIN, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "Rain") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
};

#endif

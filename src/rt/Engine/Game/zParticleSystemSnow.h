#ifndef ZPARTICLESYSTEMSNOW_H
#define ZPARTICLESYSTEMSNOW_H

#include "xParticleBatch.h"
#include "zParticleGenerator.h"

class zParticleSystemSnow : public zParticleSystem
{
    xParticleBatchGroup batch_group;

public:
    zParticleSystemSnow() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_SNOW, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "Snow") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
};

#endif

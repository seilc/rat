#ifndef ZPARTICLESYSTEMEARTHDUST_H
#define ZPARTICLESYSTEMEARTHDUST_H

#include "xParticleBatch.h"
#include "zParticleGenerator.h"

class zParticleSystemEarthDust : public zParticleSystem
{
    xParticleBatchGroup batch_group;

public:
    zParticleSystemEarthDust() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_EARTH_DUST, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "EarthDust") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
};

#endif

#ifndef ZPARTICLESYSTEMPUFFER_H
#define ZPARTICLESYSTEMPUFFER_H

#include "xParticleBatch.h"
#include "zParticleGenerator.h"

class zParticleSystemPuffer : public zParticleSystem
{
    xParticleBatchGroup batch_group;

public:
    zParticleSystemPuffer() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_PUFFER, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "Puffer") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
};

#endif

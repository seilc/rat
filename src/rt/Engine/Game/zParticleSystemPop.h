#ifndef ZPARTICLESYSTEMPOP_H
#define ZPARTICLESYSTEMPOP_H

#include "xParticleBatch.h"
#include "zParticleGenerator.h"

class zParticleSystemPop : public zParticleSystem
{
    xParticleBatchGroup batch_group;

public:
    zParticleSystemPop() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_POP, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "Pop") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void scene_exit();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
};

#endif

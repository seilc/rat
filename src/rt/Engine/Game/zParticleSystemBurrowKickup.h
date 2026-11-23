#ifndef ZPARTICLESYSTEMBURROWKICKUP_H
#define ZPARTICLESYSTEMBURROWKICKUP_H

#include "xParticleBatch.h"
#include "zParticleGenerator.h"

class zParticleSystemBurrowKickup : public zParticleSystem
{
    xParticleBatchGroup batch_group;

public:
    zParticleSystemBurrowKickup() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_BURROW_KICKUP, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "BurrowKickup") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
};

#endif

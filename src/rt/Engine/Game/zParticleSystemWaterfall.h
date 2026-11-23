#ifndef ZPARTICLESYSTEMWATERFALL_H
#define ZPARTICLESYSTEMWATERFALL_H

#include "xParticleBatch.h"
#include "zParticleGenerator.h"

class zParticleSystemWaterfall : public zParticleSystem
{
    xParticleBatchGroup batch_group;

public:
    zParticleSystemWaterfall() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_WATERFALL, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "Waterfall") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
};

class zParticleSystemWaterfallMist : public zParticleSystem
{
    xParticleBatchGroup batch_group;

public:
    zParticleSystemWaterfallMist() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_WATERFALL_MIST, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "WaterfallMist") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;

#if DEBUG || RELEASE
    virtual void add_tweaks(const char* prefix);
#endif
};

class zParticleSystemWaterfallSplash : public zParticleSystem
{
    xParticleBatchGroup batch_group;

public:
    zParticleSystemWaterfallSplash() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_WATERFALL_SPLASH, NEED_SCENE_ENTER | NEED_RESET | NEED_UPDATE, "WaterfallSplash") {}

    virtual zParticleGenerator* create_generator(zParticleAsset& asset);
    virtual void scene_enter();
    virtual void update(F32 dt);
    virtual S32 get_asset_size() const;
};

#endif

#include "xCommon.h"
#include "zParticleSystems.h"

#include "xCam.h"
#include "xSpacePartitionTree.h"
#include "zParticleSystemBurrowKickup.h"
#include "zParticleSystemEarthDust.h"
#include "zParticleSystemPop.h"
#include "zParticleSystemPuffer.h"
#include "zParticleSystemRain.h"
#include "zParticleSystemSnow.h"
#include "zParticleSystemWaterfall.h"
#include "zPlayerContainer.h"

class zParticleSystemDummy : public zParticleSystem
{
public:
    zParticleSystemDummy() : zPARTICLESYSTEM(zParticleAsset::SYSTEM_DUMMY, 0, "Dummy") {}

    virtual zParticleGenerator* create_generator(zParticleAsset&) { return NULL; }
    virtual S32 get_asset_size() const { return 0; }
};

zParticleSystem** zParticleSystemCreateAll()
{
    zParticleSystem** systems = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleSystem*[zParticleAsset::MAX_SYSTEM];

    systems[zParticleAsset::SYSTEM_DUMMY] = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleSystemDummy;
    systems[zParticleAsset::SYSTEM_WATERFALL] = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleSystemWaterfall;
    systems[zParticleAsset::SYSTEM_WATERFALL_MIST] = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleSystemWaterfallMist;
    systems[zParticleAsset::SYSTEM_WATERFALL_SPLASH] = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleSystemWaterfallSplash;
    systems[zParticleAsset::SYSTEM_SNOW] = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleSystemSnow;
    systems[zParticleAsset::SYSTEM_RAIN] = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleSystemRain;
    systems[zParticleAsset::SYSTEM_EARTH_DUST] = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleSystemEarthDust;
    systems[zParticleAsset::SYSTEM_POP] = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleSystemPop;
    systems[zParticleAsset::SYSTEM_BURROW_KICKUP] = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleSystemBurrowKickup;
    systems[zParticleAsset::SYSTEM_PUFFER] = new (eMemStaticType_0, eMemMgrTag_Particle) zParticleSystemPuffer;

    return systems;
}

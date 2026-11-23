#ifndef ZPARTICLEGENERATOR_H
#define ZPARTICLEGENERATOR_H

#include "xBase.h"
#include "xDebugTweak.h"
#include "xEnt.h"
#include "zParticleAsset.h"

struct zParticleGenerator;

class zParticleSystem
{
public:
    struct relative_ordering
    {
        S32 other;
        bool before_other;
    };

    // need
    enum
    {
        NEED_SCENE_ENTER = 0x1,
        NEED_SCENE_EXIT = 0x2,
        NEED_SETUP = 0x4,
        NEED_RESET = 0x8,
        NEED_UPDATE = 0x10,
        NEED_PRE_RENDER = 0x20,
        NEED_RENDER = 0x40
    };

    S32 type;
    S32 need;
    zParticleGenerator** generators;
    S32 generators_size;
    S32 generators_active;

    virtual zParticleGenerator* create_generator(zParticleAsset& asset) = 0;
    virtual void destroy_generator(zParticleGenerator*) {}
    virtual void scene_enter() {}
    virtual void scene_exit() {}
    virtual void setup() {}
    virtual void reset();
    virtual void update(F32 dt) {}
    virtual void pre_render() {}
    virtual void render() {}
    virtual relative_ordering* get_ordering(S32& size, S32) { size = 0; return NULL; }
    virtual S32 get_asset_size() const = 0;

#if DEBUG || RELEASE
    zParticleSystem(S32 type, S32 need, const char* name) : type(type), need(need), name(name) {}
    virtual void add_tweaks(const char* prefix) {}
    virtual void debug_render(bool all) {}

    const char* name;
#else
    zParticleSystem(S32 type, S32 need) : type(type), need(need) {}
#endif
};

#if DEBUG || RELEASE
#define zPARTICLESYSTEM(type, need, name) zParticleSystem(type, need, name)
#else
#define zPARTICLESYSTEM(type, need, name) zParticleSystem(type, need)
#endif

struct zParticleGenerator : xBase
{
    zParticleAsset* asset;
    S32 flags;
    F32 lifetime;

    void create(zParticleSystem&, zParticleAsset& asset);
    virtual void reset();
    virtual void activate(xEnt*);
    virtual void deactivate(xEnt*);
    virtual bool cullUpdate();
    virtual void pause();
    virtual void resume();
    static void event_handler(xBase* from, xBase* to, U32 event, const F32* argf, xBase* toParamWidget,
                              U32 toParamWidgetID);
    static xEnt* get_attach_entity(const zParticleAsset& asset);
    static void perturb_dir(xVec3& dir, F32 angle);

#if DEBUG || RELEASE
    static void addTweaksCB(const tweak_info& info);
    static void on_tweaked(const tweak_info& info);
    virtual void add_tweaks(const char* prefix);
    virtual void debug_render(bool all);
    virtual void asset_tweaked(const zParticleAsset& asset);

protected:
    char tweakPrefix[128];

private:
    struct debug_info;

    debug_info* debug;
#endif
};

#endif

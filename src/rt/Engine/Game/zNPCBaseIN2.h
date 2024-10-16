#ifndef ZNPCBASEIN2_H
#define ZNPCBASEIN2_H

#include "xEnt.h"
#include "xDynAsset.h"
#include "zNPC.h"
#include "zAnimFx.h"

namespace zNPC {

struct zNPCAssetIN2 : xDynAsset
{
    xEntAsset ent_asset;
    U32 navigation_mesh_id;
    U32 npcFlags;
    U32 respawnCounterID;
    U32 settingsHashID;
    U32 interestPointerGroupID;
    U32 lightKitID;
};

class zNPCBaseIN2 : public xEnt
{
protected:
    zNPCInstance* inst;

public:
    virtual void change_instance(zNPCInstance* new_inst);
    virtual ~zNPCBaseIN2();
    virtual void init(zNPCInstance* init_inst);
    virtual void timestep(xScene* xscn, F32 dt);
    virtual void activate();
    virtual void deactivate();
    virtual void preupdate(F32 dt);
    virtual void reset();
    virtual void update_bounds();
    virtual void scene_setup();
    virtual void render_npc();
    virtual void render_npc_freezing();
    virtual void render_extra();
    virtual void post_render_cleanup();
    virtual bool systemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);
    virtual U32 get_throw_anim_code();
    virtual F32 get_shadow_strength();
    virtual xSndHandle play_sound(zAnimFxSoundGroup* sound);
    virtual void destroy();
    virtual void kill();
    virtual void set_instance(zNPCInstance* inPtr);
    virtual bool CanBeTargeted();
    virtual void ExitingSpawnState();
    virtual void ProjectileCollisionOn();
    virtual void ProjectileCollisionOff();
    virtual void PastUpdateDistCleanup();
    virtual void VisibilityCullCleanup();

#ifdef DEBUGRELEASE
    virtual void ForceEnterDeathState();
#endif

#ifdef DEBUGRELEASE
    U8 npc_selection;
#endif
};

}

#endif

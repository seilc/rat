#include "xCommon.h"
#include "zCamPreset.h"

#include "xDebugTweak.h"
#include "xEnt.h"
#include "xMath.h"
#include "xstransvc.h"
#include "zBase.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zOtherAsset.h"
#include "zPlayer.h"
#include "zScene.h"

#include <stdio.h>
#include <string.h>

#if DEBUG || RELEASE
struct zCamPreset::debug_info
{
    bool player_relative;
    bool snap_to;
};
#endif

namespace {

xVec3 GetCheckpointLocation()
{
    xVec3 checkpointLocation = CURPLAYERS[0]->GetCheckPoint().position;
    xASSERT_FMT_L(18, CURPLAYERS.size() != 0, "There should be players in the scene!");
    for (S32 i = 1; i < CURPLAYERS.size(); i++) {
        xVec3AddTo(checkpointLocation, CURPLAYERS[i]->GetCheckPoint().position);
    }
    xVec3ScaleC(checkpointLocation, 1.0f / CURPLAYERS.size());
    return checkpointLocation;
}

} // namespace

void zCamPreset::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(40, asset, asset_size, zCamPreset::asset_type, zCamPreset::asset_type::type_name());
    ((zCamPreset&)data).load((asset_type&)asset);
}

void zCamPreset::update_all(F32)
{
#if DEBUG || RELEASE
    bool snapped = false;
    zScene& s = *globals.sceneCur;
    zCamPreset* preset = (zCamPreset*)s.baseList[eBaseTypeCamPreset], * const end_preset = preset + s.baseCount[eBaseTypeCamPreset];

    for (; preset != end_preset; preset++) {
        if (snapped) {
            preset->debug->snap_to = false;
        }
        else if (preset->debug->snap_to) {
            preset->apply();
            snapped = true;
        }
    }
#endif
}

void zCamPreset::apply_on_scene_prepare()
{
    zCamPreset* preset = find_preset(true);
    if (preset)
        preset->apply();
}

void zCamPreset::apply_on_reset()
{
    zCamPreset* preset = find_preset(false);
    if (preset)
        preset->apply();
}

zCamPreset* zCamPreset::find_preset(bool scene_prepare)
{
    zCamPreset* best_preset = NULL;
    F32 best_dist2 = HUGE;
    bool best_has_checkpoint = false;

    xVec3 to = GetCheckpointLocation();

    zScene& s = *globals.sceneCur;
    zCamPreset* preset = (zCamPreset*)s.baseList[eBaseTypeCamPreset], * const end_preset = preset + s.baseCount[eBaseTypeCamPreset];

    for (; preset != end_preset; preset++) {
        if (scene_prepare && !(preset->flags & asset_type::FLAG_APPLY_ON_SCENE_PREPARE)) continue;

        if (!scene_prepare && !(preset->flags & asset_type::FLAG_APPLY_ON_RESET)) continue;

        bool has_checkpoint = preset->asset->checkpoint != 0;
        if (best_has_checkpoint && !has_checkpoint) continue;

        if (has_checkpoint && !preset->at_checkpoint()) {
            continue;
        }

        F32 dist2 = preset->distance2(to);
        if (dist2 < best_dist2) {
            best_dist2 = dist2; best_preset = preset;
            best_has_checkpoint = has_checkpoint;
        }
    }

    return best_preset;
}

void zCamPreset::load(const asset_type& a)
{
    asset = (asset_type*)&a;
    xBaseInit(this, asset);
    baseType = eBaseTypeCamPreset;
    eventFunc = event_handler_callback;
    if (linkCount) link = (xLinkAsset*)(asset + 1);
    flags = asset->flags;
    debug_init();
}

bool zCamPreset::get_checkpoint(xVec3& loc, F32& yaw) const
{
    xVec3 check_loc = GetCheckpointLocation();
    PKRAssetTOCInfo ainfo;
    S32 found = xSTGetAssetInfo(asset->checkpoint, &ainfo);
    xASSERT_FMT_L(145, found, "Camera Preset %s - Could not find checkpoint: 0x%08x.", xSTAssetNameOrID(asset->id), asset->checkpoint);

    xBaseAsset* checkpoint = (xBaseAsset*)ainfo.mempos;
    U32 type = ainfo.typeref->typetag;
    if (type == 'PLYR') {
        xEntAsset* player = (xEntAsset*)checkpoint;
        yaw = player->ang.x;
        loc = player->pos;
        return true;
    } else if (type == 'DYNA' && ((xDynAsset*)checkpoint)->type == xStrHash("pointer")) {
        pointer_asset* pointer = (pointer_asset*)checkpoint;
        yaw = (PI / 180.0f) * pointer->yaw;
        loc = pointer->loc;
        return true;
    } else {
        xASSERT_L(169, false && "Unexpected checkpoint type.");
    }

    return false;
}

bool zCamPreset::at_checkpoint() const
{
    xVec3 check_loc;
    F32 check_yaw;
    if (!get_checkpoint(check_loc, check_yaw))
        return false;
    return xVec3Eq(GetCheckpointLocation(), check_loc, 0.1f);
}

F32 zCamPreset::distance2(const xVec3& to) const
{
    xMat4x3 mat;
    get_matrix(mat);
    F32 dx = to.x - mat.pos.x, dy = to.y - mat.pos.y, dz = to.z - mat.pos.z;
    return dx * dx + dy * dy + dz * dz;
}

void zCamPreset::apply()
{
}

bool zCamPreset::get_relative(xVec3& offset, F32& yaw) const
{
#if DEBUG || RELEASE
    if (debug->player_relative) {
        const xMat4x3& player_mat = *xEntGetFrame(CURPLAYERS[0]);
        offset = player_mat.pos;
        yaw = xatan2(player_mat.at.x, player_mat.at.z);
        return true;
    }
#endif

    if (!(asset->flags & asset_type::FLAG_RELATIVE) || !asset->checkpoint)
        return false;
    return get_checkpoint(offset, yaw);
}

void zCamPreset::get_matrix(xMat4x3& mat) const
{
    xMat3x3Euler(&mat, asset->dir.yaw, asset->dir.pitch, asset->dir.roll);
    mat.pos = asset->loc;

    xVec3 rel_offset;
    F32 rel_yaw;
    if (get_relative(rel_offset, rel_yaw)) {
        xMat4x3 check_mat;
        xMat3x3Euler(&check_mat, rel_yaw, 0.0f, 0.0f);
        check_mat.pos = rel_offset;
        xMat4x3Mul(&mat, &mat, &check_mat);
    }
}

void zCamPreset::event_handler(xBase*, U32 event, const F32*, xBase*, U32)
{
    switch (event) {
    case eEventApplyOnResetOn:
        flags |= asset_type::FLAG_APPLY_ON_RESET;
        break;
    case eEventApplyOnResetOff:
        flags &= ~asset_type::FLAG_APPLY_ON_RESET;
        break;
    case eEventSnapTo:
        apply();
        break;
    }
}

void zCamPreset::event_handler_callback(xBase* from, xBase* to, U32 event, const F32* argf, xBase* argw, U32 argwi)
{
    zCamPreset* w = (zCamPreset*)to;
    w->event_handler(from, event, argf, argw, argwi);
}

#if DEBUG || RELEASE
void zCamPreset::debug_init()
{
    debug = new (eMemTweakType_0, eMemMgrTag_Debug) debug_info;
    memset(debug, 0, sizeof(debug_info));
    add_tweaks();
}

void zCamPreset::add_tweaks()
{
    char prefix[128];
    sprintf(prefix, "Camera|Preset|%s|", xSTAssetNameOrID(asset->id));

    xAUTOTWEAK_FLAG(prefix, "apply_on_scene_prepare", &asset->flags, asset_type::FLAG_APPLY_ON_SCENE_PREPARE, NULL, NULL, 0);
    xAUTOTWEAK_FLAG(prefix, "apply_on_reset", &asset->flags, asset_type::FLAG_APPLY_ON_RESET, NULL, NULL, 0);
    xAUTOTWEAK_FLAG(prefix, "relative", &asset->flags, asset_type::FLAG_RELATIVE, NULL, NULL, 0);
    xAUTOTWEAK(prefix, "location X", &asset->loc.x, -1000000000.0f, 1000000000.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "location Y", &asset->loc.y, -1000000000.0f, 1000000000.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "location Z", &asset->loc.z, -1000000000.0f, 1000000000.0f, NULL, NULL, 0, false);
    xAUTOTWEAK(prefix, "direction\001 yaw", &asset->dir.yaw, -1000000000.0f, 1000000000.0f, &cb_tweak_convert_degrees, NULL, 0, false);
    xAUTOTWEAK(prefix, "direction pitch", &asset->dir.pitch, -1000000000.0f, 1000000000.0f, &cb_tweak_convert_degrees, NULL, 0, false);

    xAUTOTWEAK_BOOL(prefix, "debug->player_relative", &debug->player_relative, NULL, NULL, 0, true);
    xAUTOTWEAK_BOOL(prefix, "debug->snap_to", &debug->snap_to, NULL, NULL, 4, true);

    const char** player_labels, ** pointer_labels;
    const U32* player_ids, * pointer_ids;
    U32 players = zSceneGetAssetTweakMap(player_labels, player_ids, 'PLYR', 0);
    U32 pointers = zSceneGetAssetTweakMap(pointer_labels, pointer_ids, 'DYNA', xStrHash(pointer_asset::type_name()));

    pointers--;
    pointer_labels++;
    pointer_ids++;
    U32 checkpoints = players + pointers;
    char** checkpoint_labels = new (eMemTweakType_0, eMemMgrTag_Camera) char*[checkpoints];
    memcpy(checkpoint_labels, player_labels, players * sizeof(char*));
    // BUG: copies checkpoints entries, not pointers
    memcpy(checkpoint_labels + players, pointer_labels, checkpoints * sizeof(char*));
    U32* checkpoint_ids = new (eMemTweakType_0, eMemMgrTag_Camera) U32[checkpoints];
    memcpy(checkpoint_ids, player_ids, players * sizeof(U32));
    // BUG: copies checkpoints entries, not pointers
    memcpy(checkpoint_ids + players, pointer_ids, checkpoints * sizeof(U32));
    xAUTOTWEAK_SELECT(prefix, "asset-> checkpoint", &asset->checkpoint, (const char**)checkpoint_labels, checkpoint_ids, checkpoints, NULL, NULL, 0, true);
}
#endif

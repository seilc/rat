#include "xCommon.h"
#include "zGoo.h"

#include "xEvent.h"
#include "xOutputMgr.h"
#include "xstransvc.h"
#include "xSurface.h"
#include "zEvent.h"
#include "zFX.h"
#include "zFXGoo.h"
#include "zSurface.h"

struct zGooParams
{
    F32 depth;
    F32 orig_depth;
    xEnt* goo_ent;
};

static zGooParams* zgoo_gps;
static S32 zgoo_ngps;
static S32 zgoo_nused;

void zGooInit(S32 nobj)
{
    xASSERT_L(28, nobj > 0);
    zgoo_gps = (zGooParams*)xMEMALLOC_L(nobj * sizeof(zGooParams), 0, eMemMgrTag_ScreenFX, 0, 29);

    xASSERT_L(31, zgoo_gps);

    zgoo_ngps = nobj;
    xASSERT_L(33, zgoo_nused == 0);
}

void zGooExit()
{
    zgoo_gps = NULL;
    zgoo_ngps = 0;
    zgoo_nused = 0;
}

S32 zGooAdd(xEnt* obj, F32 depth, S32 freezeGroup)
{
#if DEBUG || RELEASE
    S32 validGoo = obj->model->Surf && (((zSurfaceProps*)obj->model->Surf->moprops)->uvfx_flags & UVANIM_FLAG_ON);

    if (!validGoo) {
        xASSERT_FMT_L(75, 0, "zGooAdd: all Goo objects MUST have a UV animation surface\nassigned (even if its just \"zero\" translation)\nobj=%s", xSTAssetNameOrID(obj->id));
        obj->model->Data->geometry->matList.materials[0]->texture = NULL;
        return 0;
    }
#endif

    for (S32 i = 0; i < zgoo_nused; i++) {
        if (zgoo_gps[i].goo_ent == obj) {
            return 1;
        }
    }

    if (zgoo_nused >= zgoo_ngps) {
        xASSERT_FMT_L(89, 0, "zGooAdd: out of space, cant add ent %s.", xSTAssetNameOrID(obj->id));

        return 0;
    }

    xASSERT_L(93, obj);
    if (depth < 0.0f) {
        xOutWarn("GAME", "%s: hmm, depth was supposed to be > 0...\n", __FUNCTION__);
    }
    if (!obj->transl) {
        xOutErr("GAME", "%s: shit, we need a translate function...\n", __FUNCTION__);
    }

    zGooParams* gp = &zgoo_gps[zgoo_nused];
    gp->depth = gp->orig_depth = depth;
    gp->goo_ent = obj;
    zgoo_nused++;

    if (obj->model->Data) {
        zFXGooEnable(obj->model, freezeGroup);
    }

    return 1;
}

S32 zGooIs(xEnt* obj, F32& depth, U32 playerCheck)
{
    S32 ret = FALSE, i;

    for (i = 0; i < zgoo_nused; i++) {
        if (obj == zgoo_gps[i].goo_ent) {
            depth = zgoo_gps[i].depth = zgoo_gps[i].orig_depth;
            ret = TRUE;
            break;
        }
    }

    if (ret) {
        depth = zgoo_gps[i].orig_depth;
        ret = zFXGooIs(obj, depth, playerCheck);
        zgoo_gps[i].depth = depth;
    }

    return ret;
}

// UNUSED: needed to match (code from TSSM GC, BFBB GC)
void zGooCollsBegin()
{
    for (S32 i = 0; i < zgoo_nused; i++) {
        xEnt* gooey = zgoo_gps[i].goo_ent;
        F32 ignoredepth;
        zGooIs(gooey, ignoredepth, FALSE);
        if (gooey->transl) {
            xVec3 change;
            change.x = 0.0f;
            change.y = -zgoo_gps[i].depth;
            change.z = 0.0f;
            gooey->transl(gooey, &change, NULL);
        }
    }
}

void zGooMeltFinished(RpAtomic* atomic)
{
    for (S32 i = 0; i < zgoo_nused; i++) {
        xEnt* gooey = zgoo_gps[i].goo_ent;
        if (gooey->model->Data == atomic) {
            zEntEvent(gooey, eEventPlatUnpause, 0.25f, 0.0f, 0.0f, 0.0f);
        }
    }
}

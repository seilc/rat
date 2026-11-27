#include "xCommon.h"
#include "zCineFX.h"

#include "xCutscene.h"
#include "xFX.h"
#include "xPad.h"
#include "xString.h"
#include "xstransvc.h"
#include "zAutoTweak.h"
#include "zCineFXShared.h"
#include "zCineFXTableSceneUT11.h"
#include "zCineFXTableTest.h"
#include "zCutsceneMgr.h"
#include "zParEmitter.h"
#include "zPlayerContainer.h"

#include <stdio.h>
#include <string.h>

#define MAX_CUTMAP 64

struct NCINBeNosey : XCSNNosey
{
    zCutsceneMgr* use_csnmgr;
    NCINEntry* use_fxtab;

    virtual void CanRenderNow();
    virtual void UpdatedAnimated(RpAtomic* model, RwMatrix* animMat, U32 animIndex, U32 dataIndex, U32& visFlags);

    void Init(const zCutsceneMgr* csnmgr, NCINEntry* fxtab, S32 flags)
    {
        use_csnmgr = (zCutsceneMgr*)csnmgr;
        use_fxtab = fxtab; flg_nosey = flags;
    }

    void Done() { use_csnmgr = NULL; use_fxtab = NULL; flg_nosey = 0; }
};

static NCINMap* g_cutmap[MAX_CUTMAP] = { NULL };

void NCINBeNosey::CanRenderNow()
{
    const zCutsceneMgr* csnmgr = use_csnmgr;
    xASSERT_L(122, csnmgr);

    NCINEntry* fxtab = use_fxtab;
    xASSERT_L(125, fxtab);

    if (!csnmgr)
        return;

    if (!fxtab)
        return;

    NCINEntry* nextrec = fxtab;
    while (nextrec->info.fxTypeID) {
        NCINEntry* fxrec = nextrec;
        nextrec++;
        if (!fxrec->cb.fxrend)
            continue;

        if (!(fxrec->flg_stat & NCIN_FXF_ACTIVE))
            continue;

        fxrec->cb.fxrend(csnmgr, fxrec);
    }
}

void NCINBeNosey::UpdatedAnimated(RpAtomic* model, RwMatrix* animMat, U32 animIndex, U32 dataIndex, U32& visFlags)
{
    const zCutsceneMgr* csnmgr = use_csnmgr;
    xASSERT_L(157, csnmgr);

    NCINEntry* fxtab = use_fxtab;
    xASSERT_L(160, fxtab);

    if (!csnmgr)
        return;

    if (!fxtab)
        return;

    NCINEntry* nextrec = fxtab;
    while (nextrec->info.fxTypeID) {
        NCINEntry* fxrec = nextrec;
        nextrec++;
        if (fxrec->index.idx_anim != dataIndex)
            continue;

        if (!fxrec->cb.fxanim)
            continue;

        if (!(fxrec->flg_stat & NCIN_FXF_ACTIVE))
            continue;

        fxrec->cb.fxanim(csnmgr, fxrec, model, animMat, animIndex, dataIndex, visFlags);
    }
}

static NCINBeNosey* g_noz_ncin;

#if DEBUG || RELEASE
static void zCineFXTweakMenu();
static void zCineFXTweakThisTable(const char* prefix_root, NCINEntry* fxtab);
#endif
static void AddTables();

void zCineFXStartup()
{
    static NCINBeNosey nozey_npc_cinematics;
    g_noz_ncin = &nozey_npc_cinematics;

    AddTables();
}

void zCineFXShutdown()
{
}

void zCineFXSceneSetup()
{
#if DEBUG || RELEASE
    zCineFXTweakMenu();
#endif
}

void zCineFXRegisterTable(NCINMap* maplist)
{
    NCINMap* maprec = maplist;
    while (maprec->cinname) {
        S32 entry_entered = FALSE;
        for (S32 i = 0; i < MAX_CUTMAP; i++) {
            if (g_cutmap[i])
                continue;

            g_cutmap[i] = maprec;

            g_cutmap[i]->cinhash = xStrHash(g_cutmap[i]->cinname);

            entry_entered = TRUE;
            break;
        }
        xASSERT_L(249, entry_entered);

        maprec++;
    }
}

static NCINEntry* zCineFXCutscenePickTable(const zCutsceneMgr* csnmgr)
{
    xASSERT_L(262, csnmgr);
    xASSERT_L(263, csnmgr->csn);
    xASSERT_L(264, csnmgr->csn->Info);
    xASSERT_L(265, csnmgr->csn->Info->AssetID);

    NCINEntry* da_fxtab = NULL;
    for (S32 i = 0; i < MAX_CUTMAP; i++) {
        NCINMap* cutrec = g_cutmap[i];
        if (!cutrec)
            continue;

        xASSERT_L(273, cutrec->cinhash);
        if (cutrec->cinhash != csnmgr->csn->Info->AssetID)
            continue;

        da_fxtab = cutrec->cinfxtab;
        break;
    }

    return da_fxtab;
}

S32 zCineFXCutscenePrep(const xScene*, F32, const zCutsceneMgr* csnmgr)
{
    NCINEntry* fxtab = NULL;
    fxtab = zCineFXCutscenePickTable(csnmgr);
    if (!fxtab)
        return FALSE;

    xASSERT_L(299, csnmgr && csnmgr->csn);
    if (csnmgr && csnmgr->csn)
        csnmgr->csn->NoseyClear();

    g_noz_ncin->Init(csnmgr, fxtab, 0x3);

    NCINEntry* nextrec = fxtab;
    while (nextrec->info.fxTypeID) {
        NCINEntry* fxrec = nextrec;
        nextrec++;
        xASSERT_L(314, fxrec->cb.fxupd &&"Newstyle");

        fxrec->flg_stat = 0;
        fxrec->flg_stat = 0x4 | 0x8;

        memset(&fxrec->fxdata, 0, sizeof(NCINData));
    }

    return TRUE;
}

void zCineFXCutsceneDone(const xScene*, F32, const zCutsceneMgr* csnmgr)
{
    NCINEntry* fxtab = NULL;
    fxtab = zCineFXCutscenePickTable(csnmgr);
    if (!fxtab)
        return;

    xASSERT_L(342, csnmgr && csnmgr->csn);
    if (csnmgr && csnmgr->csn)
        csnmgr->csn->NoseyClear();

    g_noz_ncin->Done();

    NCINEntry* nextrec = fxtab;
    while (nextrec->info.fxTypeID) {
        NCINEntry* fxrec = nextrec;
        nextrec++;
        if (fxrec->flg_stat & NCIN_FXF_DONE)
            continue;

        if (!(fxrec->flg_stat & NCIN_FXF_ACTIVE))
            continue;

        xASSERT_L(362, fxrec->cb.fxupd &&"Newstyle");

        fxrec->cb.fxupd(csnmgr, fxrec, 1);
        fxrec->flg_stat &= ~NCIN_FXF_ACTIVE;

        xASSERT_L(368, fxrec->flg_stat & NCIN_FXF_DONE);
    }
}

void zCineFXCutscene(const xScene*, F32, const zCutsceneMgr* csnmgr)
{
    xASSERT_L(380, csnmgr && csnmgr->csn);
    xCutscene* csn = csnmgr->csn;

    NCINEntry* fxtab = zCineFXCutscenePickTable(csnmgr);
    if (!fxtab)
        return;

    S32 need_animated = 0, need_render = 0;
    NCINEntry* nextrec = fxtab;
    while (nextrec->info.fxTypeID) {
        NCINEntry* fxrec = nextrec;
        nextrec++;
        xASSERT_L(400, fxrec->cb.fxupd &&"Newstyle");

        if (csn->Time < fxrec->info.tym_beg)
            continue;

        if (fxrec->flg_stat & NCIN_FXF_DONE)
            continue;

        if (csn->Time > fxrec->info.tym_end && (fxrec->flg_stat & NCIN_FXF_ACTIVE)) {
            fxrec->cb.fxupd(csnmgr, fxrec, 1);
            xASSERT_L(413, fxrec->flg_stat & NCIN_FXF_DONE);
            fxrec->flg_stat &= ~NCIN_FXF_ACTIVE;
            continue;
        } else if (csn->Time > fxrec->info.tym_end && !(fxrec->flg_stat & NCIN_FXF_DONE)) {
            fxrec->flg_stat |= NCIN_FXF_DONE;
            continue;
        }

        fxrec->flg_stat |= NCIN_FXF_ACTIVE;

        fxrec->cb.fxupd(csnmgr, fxrec, 0);

        if (!(fxrec->flg_stat & 0x4))
            fxrec->flg_stat &= ~0x8;
        fxrec->flg_stat &= ~0x4;

        if (fxrec->flg_stat & NCIN_FXF_DONE)
            fxrec->flg_stat &= ~NCIN_FXF_ACTIVE;

        if (!(fxrec->flg_stat & NCIN_FXF_ACTIVE))
            continue;

        if (fxrec->cb.fxanim)
            need_animated++;
        if (fxrec->cb.fxrend)
            need_render++;
    }

    xASSERT_L(467, g_noz_ncin->use_csnmgr);
    xASSERT_L(468, g_noz_ncin->use_fxtab);

    S32 flags = 0;
    if (need_animated)
        flags |= 0x2;
    if (need_render)
        flags |= 0x1;

    if (flags) {
        g_noz_ncin->flg_nosey = flags;
        csn->NoseySet(g_noz_ncin);
    } else {
        csn->NoseyClear();
    }
}

#if DEBUG || RELEASE
static void zCineFXTweakMenu()
{
    for (S32 i = 0; i < MAX_CUTMAP; i++) {
        NCINMap* cutrec = g_cutmap[i];
        if (!cutrec)
            continue;

        xASSERT_L(502, cutrec->cinhash);

        PKRAssetTOCInfo tocdata;
        S32 rc = xSTGetAssetInfo(cutrec->cinhash, &tocdata);
        if (!rc)
            continue;

        char prefix[128];
        sprintf(prefix, "FX|CinemaFX|%s", cutrec->cinname);
        const tweak_info* skipMe = xDebugGetTweak(prefix);
        if (skipMe)
            continue;

        zCineFXTweakThisTable(prefix, cutrec->cinfxtab);
    }
}

static void zCineFXTweakThisTable(const char* prefix_root, NCINEntry* fxtab)
{
    S32 idx_entry = 0;
    NCINEntry* nextrec = fxtab;
    while (nextrec->info.fxTypeID) {
        NCINEntry* fxrec = nextrec;
        nextrec++;
        if (!fxrec->info.twk_name)
            continue;

        if (!fxrec->info.twk_name[0])
            continue;

        TypeInfo* info = &fxrec->info;
        char tweakBase[128], prefix[128];
        sprintf(tweakBase, "%s|%03d %s", prefix_root, ++idx_entry, info->twk_name);
        sprintf(prefix, "%s|%s|", tweakBase, "\002Info");
        zAUTOTWEAK(prefix, "tym_beg", &info->tym_beg, 0.0f, 1e9f, NULL, NULL, 2);
        zAUTOTWEAK(prefix, "tym_end", &info->tym_end, 0.0f, 1e9f, NULL, NULL, 2);

        zCinFX_BasicInput_Twk(tweakBase, fxrec);
        zCinFX_IndexInput_Twk(tweakBase, fxrec);

        if (fxrec->cb.tweaks)
            fxrec->cb.tweaks(tweakBase, fxrec);
    }
}
#endif

static void AddTables()
{
    zCineFX_Register_TestTable();
    zCineFX_Register_SceneUT11();
}

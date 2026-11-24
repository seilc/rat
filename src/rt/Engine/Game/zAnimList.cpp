#include "xCommon.h"
#include "zAnimList.h"

#include "xAnim.h"
#include "xstransvc.h"
#include "zAnimCustom.h"
#include "zAnimListAsset.h"

static S32 nals;
static U32* aids;
static xAnimTable** atbls;
static S32* anused;

static char* astnames[20] = {
    "stop0", "stop1", "stop2", "stop3", "stop4", "stop5", "stop6", "stop7", "stop8", "stop9",
    "loop0", "loop1", "loop2", "loop3", "loop4", "loop5", "loop6", "loop7", "loop8", "loop9"
};

static U32 AlwaysConditional(xAnimTransition*, xAnimSingle*, void*) { return TRUE; }

static void AddAnimCustom(zAnimListAsset* a)
{
    for (S32 i = 0; i < 10; i++) {
        if (a->ids[i] && a->stateHashes[i]) {
            U32 size;
            void* buf = xSTFindAsset(a->ids[i], &size);
            if (buf) {
                xAnimFile* afile = xAnimFileNew(buf, "", 0, NULL);
                afile->ID = a->ids[i];
                zAnimCustomAdd(a->stateHashes[i], afile, a->hasPhysics[i]);
            }
        }
    }
}

void zAnimListInit()
{
    nals = xSTAssetCountByType('ALST');

    if (nals) {
        aids = (U32*)xMEMALLOC_L(nals * sizeof(U32), 0, eMemMgrTag_Anim, 0, 81);
        xASSERT_L(82, aids);
        atbls = (xAnimTable**)xMEMALLOC_L(nals * sizeof(xAnimTable*), 0, eMemMgrTag_Anim, 0, 83);
        xASSERT_L(84, atbls);
        anused = (S32*)xMEMALLOC_L(nals * sizeof(S32), 0, eMemMgrTag_Anim, 0, 85);
        xASSERT_L(86, anused);

        for (S32 i = 0; i < nals; i++) {
            U32 size;
            zAnimListAsset* zala = (zAnimListAsset*)xSTFindAssetByType('ALST', i, &size);
            xASSERT_L(91, zala);

            PKRAssetTOCInfo ainfo;
            xSTGetAssetInfoByType('ALST', i, &ainfo);

            if (zala->stateHashes[0] != 0xA0F81E55) {
                AddAnimCustom(zala);
                continue;
            }

            xAnimTable* atbl = xAnimTableNew("", 0);
            xASSERT_L(103, atbl);

            aids[i] = ainfo.aid;
            atbls[i] = atbl;
            anused[i] = 0;

            xASSERT_FMT_L(109, zala->ids[0] != 0, "Anim list 0x%08X (%s) has no animations", ainfo.aid, ainfo.assetname);
            void* buf = zala->ids[0] == 0 ? NULL : xSTFindAsset(zala->ids[0], &size);
            if (buf) {
                xAnimFile* afile = xAnimFileNew(buf, "", 0, NULL);
                xASSERT_L(113, afile);

                xAnimTableNewState(atbl, "idle", AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
                xAnimTableAddFile(atbl, afile, "idle");

                anused[i]++;
            }
            S32 idle_exists = (S32)buf;

            for (S32 j = 1; j < 10; j++) {
                if (zala->ids[j]) {
                    buf = xSTFindAsset(zala->ids[j], &size);
                    if (buf) {
                        xAnimFile* afile = xAnimFileNew(buf, "", 0, NULL);
                        xASSERT_L(128, afile);

                        if (idle_exists) {
                            xAnimTableNewState(atbl, astnames[j], AnimState_EndTransition, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
                            xAnimTable* table = atbl;
                            xAnimTableNewTransition(table, astnames[j], "idle", AlwaysConditional, NULL, AnimTransition_Default, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
                        } else {
                            xAnimTableNewState(atbl, astnames[j], AnimState_EndStop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
                        }

                        xAnimTableAddFile(atbl, afile, astnames[j]);

                        xAnimTableNewState(atbl, astnames[j + 10], AnimState_EndLoop, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
                        xAnimTableAddFile(atbl, afile, astnames[j + 10]);

                        anused[i]++;
                    }
                }
            }
        }
    }
}

void zAnimListExit()
{
    nals = 0;
    aids = NULL;
    atbls = NULL;
    anused = NULL;
}

xAnimTable* zAnimListGetTable(U32 id)
{
    for (S32 i = 0; i < nals; i++) {
        if (id == aids[i]) {
            return atbls[i];
        }
    }
    return NULL;
}

S32 zAnimListGetNumUsed(U32 id)
{
    for (S32 i = 0; i < nals; i++) {
        if (id == aids[i]) {
            return anused[i];
        }
    }
    return 0;
}

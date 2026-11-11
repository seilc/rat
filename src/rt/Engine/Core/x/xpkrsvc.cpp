#include "xpkrsvc.h"
#include "xDebug.h"
#include "xhipio.h"
#include "xMemMgr.h"
#include "xOutputMgr.h"
#include "xFile.h"
#include "xutil.h"
#include "iTime.h"
#include "zMain.h"

#include "decomp.h"

#include <rwcore.h>
#include <string.h>
#include <time.h>

#define SIMPLE_SHADOW_TABLE_ID 0x98A3F56C // xStrHash("simple shadow table")

#define PKR_MAX_NAMLEN 128
#define PKR_MAX_ARRAY 4096
#define PKR_MAX_TYPES 128
#define PKR_MAX_INSTANCES 32

#define PKR_DUMBNUM 0x33333333
#define SAFELOOP_SMALL 128

#define PKR_INFO_SOURCE_FILE 0x1
#define PKR_INFO_SOURCE_VIRTUAL 0x2
#define PKR_INFO_READ_TRANSFORM 0x4
#define PKR_INFO_WRITE_TRANSFORM 0x8

#define PKR_LDASS_ACTIVE 0x10000
#define PKR_LDASS_READY 0x80000
#define PKR_LDASS_ISDUP 0x100000
#define PKR_LDASS_ISNIL 0x200000

#define PKR_LDLAY_INPROG 0x1000000
#define PKR_LDLAY_ISDONE 0x2000000

enum en_PKR_LAYER_LOAD_DEST
{
    PKR_LDDEST_SKIP,
    PKR_LDDEST_KEEPSTATIC,
    PKR_LDDEST_KEEPMALLOC,
    PKR_LDDEST_RWHANDOFF,
    PKR_LDDEST_NOMORE,
    PKR_LDDEST_FORCE = FORCEENUMSIZEINT
};

struct st_PACKER_LTOC_NODE
{
    en_LAYER_TYPE laytyp;
    st_XORDEREDARRAY assref;
    S32 flg_ldstat;
    S32 danglecnt;
    U32 chksum;
    S32 laysize;
    char* laymem;
    char* laytru;
};

enum en_generr
{
    PKR_GENERR_NONE,
    PKR_GENERR_PACKOPEN,
    PKR_GENERR_ASSETOPEN,
    PKR_GENERR_EMPTYASSET,
    PKR_GENERR_WRITEFAIL,
    PKR_GENERR_VIRTUAL,
    PKR_GENERR_DUPEASSET,
    PKR_GENERR_UNKNOWN
};

struct PKRWriteData
{
    PKRAssetType* types;
    void* userdata;
    S32 lockid;
    U32 pkropt;
    S32 cltver;
    char packfile[256];
    S32 sectalign;
    S32 dassalign;
    st_XORDEREDARRAY asslist;
    st_XORDEREDARRAY laylist;
    S32 off_lg_asset;
    S32 off_lg_layer;
    S32 off_lg_xform;
    char gameName[32];
    char platform[32];
    char videoSystem[32];
    char countryCode[32];
    char hdrfile[256];
    S32(*hdrfunc)(void*, U32, char*, char*);
    S32(*generrf)(U32, en_generr, char*, char*);
};

struct xPkrSvcMemMgrTagLookup
{
    en_LAYER_TYPE laytype;
    eMemMgrTag memtag;
};

#if DEBUG || RELEASE
static xPkrSvcMemMgrTagLookup xPkrSvcMemMgrTagLookupTable[] = {
    { PKR_LTYPE_DEFAULT, eMemMgrTag_PackerDefault },
    { PKR_LTYPE_TEXTURE, eMemMgrTag_Texture },
    { PKR_LTYPE_TEXTURE_STRM, eMemMgrTag_TextureStream },
    { PKR_LTYPE_BSP, eMemMgrTag_EnvSpatial },
    { PKR_LTYPE_MODEL, eMemMgrTag_Model },
    { PKR_LTYPE_ANIMATION, eMemMgrTag_Anim },
    { PKR_LTYPE_VRAM, eMemMgrTag_VideoRAMAsset },
    { PKR_LTYPE_SRAM, eMemMgrTag_SoundRAMAsset },
    { PKR_LTYPE_SNDTOC, eMemMgrTag_Audio },
    { PKR_LTYPE_CUTSCENE, eMemMgrTag_Cinematic },
    { PKR_LTYPE_CUTSCENETOC, eMemMgrTag_Cinematic },
    { PKR_LTYPE_JSPINFO, eMemMgrTag_EnvSpatial }
};

static U32 PKR_lookupMemtag(en_LAYER_TYPE laytyp)
{
    S32 memtag = -1;

    for (U32 i = 0; i < ARRAY_LENGTH(xPkrSvcMemMgrTagLookupTable); i++) {
        if (laytyp == xPkrSvcMemMgrTagLookupTable[i].laytype) {
            memtag = xPkrSvcMemMgrTagLookupTable[i].memtag;
            break;
        }
    }
    if (memtag == -1) {
        memtag = eMemMgrTag_PackerUnknown;
    }
    return memtag;
}
#else
#define PKR_lookupMemtag(laytyp) 0
#endif

static st_PACKER_READ_DATA* PKR_ReadInit(void* userdata, const char* pkgfile, U32 opts, S32* cltver, PKRAssetType* typelist, S32 fileflags);
static void PKR_ReadDone(st_PACKER_READ_DATA* pr);
static S32 PKR_SetActive(st_PACKER_READ_DATA* pr, en_LAYER_TYPE layer);
static S32 PKR_parse_TOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 PKR_LoadStep_Async();
static char* PKR_LayerMemReserve(st_PACKER_READ_DATA* pr, st_PACKER_LTOC_NODE* layer);
static void PKR_LayerMemRelease(st_PACKER_READ_DATA*, st_PACKER_LTOC_NODE* layer);
static en_PKR_LAYER_LOAD_DEST PKR_layerLoadDest(en_LAYER_TYPE laytyp);
static S32 PKR_layerTypeNeedsXForm(en_LAYER_TYPE laytyp);
static S32 PKR_findNextLayerToLoad(st_PACKER_READ_DATA** work_on_pkg, st_PACKER_LTOC_NODE** next_layer);
static void PKR_updateLayerAssets(st_PACKER_LTOC_NODE* laynode);
static void PKR_xformLayerAssets(st_PACKER_LTOC_NODE* laynode);
static void PKR_xform_asset(st_PACKER_ATOC_NODE* assnode, S32 dumpable_layer);
static void* PKR_FindAsset(st_PACKER_READ_DATA* pr, U32 aid, S32* assetsize);
static S32 PKR_LoadLayer(st_PACKER_READ_DATA*, en_LAYER_TYPE);
static void* PKR_LoadAsset(st_PACKER_READ_DATA* pr, U32 aid, const char*, void*, S32* assetsize);
static U32 PKR_GetAssetSize(st_PACKER_READ_DATA* pr, U32 aid);
static S32 PKR_AssetCount(st_PACKER_READ_DATA* pr, U32 type);
static void* PKR_AssetByType(st_PACKER_READ_DATA* pr, U32 type, S32 idx, U32* size);
static S32 PKR_IsAssetReady(st_PACKER_READ_DATA* pr, U32 aid);
static U32 PKR_getPackTimestamp(st_PACKER_READ_DATA* pr);
static void PKR_Disconnect(st_PACKER_READ_DATA* pr);
static const char* PKR_AssetName(st_PACKER_READ_DATA* pr, U32 aid);
static U32 PKR_GetBaseSector(st_PACKER_READ_DATA* pr);
static S32 PKR_GetAssetInfo(st_PACKER_READ_DATA* pr, U32 aid, PKRAssetTOCInfo* tocinfo);
static S32 PKR_GetAssetInfoByType(st_PACKER_READ_DATA* pr, U32 type, S32 idx, PKRAssetTOCInfo* tocinfo);
static S32 PKR_PkgHasAsset(st_PACKER_READ_DATA* pr, U32 aid);
static S32 PKR_FRIEND_assetIsGameDup(U32 aid, const st_PACKER_READ_DATA* skippr, S32 oursize, U32 ourtype, U32 chksum, char* our_fnam);
static S32 PKR_makepool_anode(st_PACKER_READ_DATA* pr, S32 cnt);
static void PKR_kiilpool_anode(st_PACKER_READ_DATA* pr);
static st_PACKER_ATOC_NODE* PKR_newassnode(st_PACKER_READ_DATA* pr, U32 aid);
static st_PACKER_LTOC_NODE* PKR_newlaynode(en_LAYER_TYPE laytyp, S32 refcnt);
static void PKR_oldlaynode(st_PACKER_LTOC_NODE* laynode);
static S32 OrdComp_R_Asset(void* vkey, void* vitem);
static S32 OrdTest_R_AssetID(const void* vkey, void* vitem);
static S32 LOD_r_HIPA(st_HIPLOADDATA*, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PACK(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PVER(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PFLG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*);
static S32 LOD_r_PCNT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PCRT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PMOD(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PLAT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_DICT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_ATOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_AINF(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*);
static S32 LOD_r_AHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_ADBG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr, st_PACKER_ATOC_NODE* assnode);
static S32 LOD_r_LTOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_LINF(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*);
static S32 LOD_r_LHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_LDBG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr, st_PACKER_LTOC_NODE* laynode);
static S32 LOD_r_STRM(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_DHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*);
static S32 LOD_r_DPAK(st_HIPLOADDATA*, st_PACKER_READ_DATA*);
static void PKR_bld_typecnt(st_PACKER_READ_DATA* pr);
static S32 PKR_typeHdlr_cnt(st_PACKER_READ_DATA* pr);
static S32 PKR_typeHdlr_idx(st_PACKER_READ_DATA* pr, U32 type);
static void PKR_spew_verhist();
static S32 PKR_push_memmark();
static S32 PKR_pop_memmark();
#if DEBUG
static void PKR_r_rpt_stats(st_PACKER_READ_DATA* pr);
static void PKR_r_rpt_typeInfo(st_PACKER_READ_DATA* pr);
static void PKR_r_rpt_typeTally(st_PACKER_READ_DATA* pr);
static void PKR_r_rpt_layers(st_PACKER_READ_DATA* pr);
static void PKR_r_rpt_assets(st_PACKER_READ_DATA* pr);
static void PKR_r_rpt_kosher(st_PACKER_READ_DATA* pr);
static void PKR_r_rpt_timetrak(st_PACKER_READ_DATA* pr, S32 flg_opts);
#endif

static PKRReadFuncs g_pkr_read_funcmap_original = {
    1,
    PKR_ReadInit,
    PKR_ReadDone,
    PKR_LoadLayer,
    PKR_GetAssetSize,
    PKR_LoadAsset,
    PKR_AssetByType,
    PKR_AssetCount,
    PKR_IsAssetReady,
    PKR_SetActive,
    PKR_AssetName,
    PKR_GetBaseSector,
    PKR_GetAssetInfo,
    PKR_GetAssetInfoByType,
    PKR_PkgHasAsset,
    PKR_getPackTimestamp,
    PKR_Disconnect,
#if DEBUG
    PKR_r_rpt_stats,
    PKR_r_rpt_typeInfo,
    PKR_r_rpt_typeTally,
    PKR_r_rpt_layers,
    PKR_r_rpt_assets,
    PKR_r_rpt_kosher,
    PKR_r_rpt_timetrak
#endif
};
static PKRReadFuncs g_pkr_read_funcmap = g_pkr_read_funcmap_original;

static st_HIPLOADFUNCS* g_hiprf;
static st_PACKER_READ_DATA g_readdatainst[PKR_MAX_INSTANCES] = {};
static U32 g_loadlock;
S32 pkr_sector_size;
static S32 g_packinit;
static S32 g_memalloc_pair;
static S32 g_memalloc_runtot;
static S32 g_memalloc_runfree;
static char* g_strz_layername[] = {
    "DEFAULT",
    "TEXTURE",
    "TEXTURE_STRM",
    "BSP",
    "MODEL",
    "ANIMATION",
    "VRAM",
    "SRAM",
    "SNDTOC",
    "CUTSCENE",
    "CUTSCENETOC",
    "JSPINFO"
};
#if DEBUG
static iTime g_tym_traklast[4];
#endif

PKRReadFuncs* PKRGetReadFuncs(S32 apiver)
{
    switch (apiver) {
    case 1:
        return &g_pkr_read_funcmap;
    }
    return NULL;
}

S32 PKRStartup()
{
    if (!g_packinit++) {
        xOutInfo("PACKER", "PKR: Starting Packer Services\n");

        g_pkr_read_funcmap = g_pkr_read_funcmap_original;

        g_hiprf = get_HIPLFuncs();

        xOutBabble("PACKER", "PKR: -- data size --\n");
        xOutBabble("PACKER", "PKR: PKRReadData  %d\n", sizeof(st_PACKER_READ_DATA));
        xOutBabble("PACKER", "PKR: PKRANode     %d\n", sizeof(st_PACKER_ATOC_NODE));
        xOutBabble("PACKER", "PKR: PKRLNode     %d\n", sizeof(st_PACKER_LTOC_NODE));
        xOutBabble("PACKER", "PKR: --             \n");
        xOutBabble("PACKER", "PKR: PKRWriteData %d\n", sizeof(PKRWriteData));
        xOutBabble("PACKER", "PKR: PKRAsset     %d\n", 0x238);
        xOutBabble("PACKER", "PKR: PKRLayer     %d\n", 0x1C);
        xOutBabble("PACKER", "PKR: ---------------\n");

        pkr_sector_size = 32;

        xOutInfo("PACKER", "PKR: Using ASYNC ASynchronous Implementation\n");

#if DEBUG
        xASSERT_L(320, pkr_sector_size==xFileGetSectorSize(XFILE_DRIVE_CDVD));
#endif
    }
    return g_packinit;
}

S32 PKRShutdown()
{
    if (!--g_packinit) {
        xOutInfo("PACKER", "PKR: Shutting down Packer Services\n");
    }
    return g_packinit;
}

static PKRAssetType* PKRAssetInfoFromWriteFuncs(PKRWriteData* wd, U32 assetType); // UNUSED
static char* PackFilenameFromWriteFuncs(PKRWriteData* wd); // UNUSED

S32 PKRLoadStep(S32 block)
{
    S32 more_todo = TRUE;

    if (block) {
        xOutInfo("PACKER", "PKR: 'Block' mode loading\n");
    }

    more_todo = PKR_LoadStep_Async();

    return more_todo;
}

static st_PACKER_READ_DATA* PKR_ReadInit(void* userdata, const char* pkgfile, U32 opts, S32* cltver, PKRAssetType* typelist, S32 fileflags)
{
    st_PACKER_READ_DATA* pr = NULL;
    S32 i = 0;
    S32 uselock = -1;
    S32 rc = 0;
    char* tocbuf_RAW = NULL;
    char* tocbuf_aligned = NULL;

    xASSERT_L(497, pkgfile);
    xASSERT_L(498, opts);
    xASSERT_L(499, typelist);

    PKR_alloc_chkidx();

    tocbuf_aligned = (char*)PKR_getmem('PTOC', 0x8000, 'PTOC', 64, eMemMgrTag_Packer, TRUE, &tocbuf_RAW);
    xASSERT_L(507, tocbuf_aligned);
    xASSERT_L(508, tocbuf_RAW);

    for (i = 0; i < PKR_MAX_INSTANCES; i++) {
        if (!(g_loadlock & (1 << i))) {
            g_loadlock |= (1 << i);
            pr = &g_readdatainst[i];
            uselock = i;
            break;
        }
    }
    xASSERT_MSG_L(519, pr, "Out of load manager instances");

    if (pr) {
        memset(pr, 0, sizeof(st_PACKER_READ_DATA));

        pr->lockid = uselock;
        pr->userdata = userdata;
        pr->opts = opts;
        pr->types = typelist;
        pr->cltver = -1;

        xASSERT_L(532, strlen(pkgfile) && (strlen(pkgfile)<PKR_MAX_NAMLEN-2));
        strncpy(pr->packfile, pkgfile, PKR_MAX_NAMLEN);

        if (!tocbuf_aligned) {
            pr->pkg = g_hiprf->create(pkgfile, NULL, 0, fileflags);
        } else {
            pr->pkg = g_hiprf->create(pkgfile, tocbuf_aligned, 0x8000, fileflags);
        }

        xASSERT_L(543, pr->pkg);

        if (pr->pkg) {
            xOutInfo("PACKER", "\nPKR: --------\nPKR: Opened %s\nPKR: --------\n", pr->packfile);
            xOutInfo("PACKER", "PKR: TempBuffer: RAW 0x%08X :: Align(%d) 0x%08X\n", tocbuf_RAW, 64, tocbuf_aligned);

#if DEBUG
            g_tym_traklast[2] = iTimeGet();
#endif

            pr->base_sector = g_hiprf->basesector(pr->pkg);

            rc = PKR_parse_TOC(pr->pkg, pr);
            xASSERT_L(571, rc);

            xASSERT_L(575, pr->cltver!=-1);
            *cltver = pr->cltver;

            g_hiprf->setBypass(pr->pkg, TRUE, TRUE);

#if DEBUG
            pr->dbg_readtime[2] += iTimeGet() - g_tym_traklast[2];
#endif
        } else {
            xOutWarn("PACKER", "PKR: HIP Open for Read Failed (%s)\n", pkgfile);

            PKR_ReadDone(pr);

            pr = NULL;
            *cltver = -1;
        }
    } else {
        xOutWarn("PACKER", "PKR: HIP ReadInit Failed (%s)\n", pkgfile);

        pr = NULL;
        *cltver = -1;
    }

    PKR_relmem('PTOC', 0x8000, tocbuf_RAW, 'PTOC', TRUE);

    tocbuf_RAW = NULL;

#if DEBUG
    if (pr) {
        st_PACKER_LTOC_NODE* tmplay = NULL;
        st_PACKER_ATOC_NODE* tmpass = NULL;

        xOutInfo("PACKER", "========\n");
        xOutInfo("PACKER", "PKR: -- Layers in HIP %s\n", pr->packfile);
        xOutInfo("PACKER", " (listed in load order)\n");
        xOutInfo("PACKER", "\n");
        for (i = 0; i < pr->laytoc.cnt; i++) {
            tmplay = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
            xOutInfo("PACKER", "\tIndex       %d\n", i);
            xOutInfo("PACKER", "\tLayer Type %2d - %-12s (%s)\n", tmplay->laytyp, g_strz_layername[tmplay->laytyp],
                     (PKR_layerLoadDest(tmplay->laytyp) == PKR_LDDEST_SKIP) ? "Not loaded" : "Loaded");
            xOutInfo("PACKER", "\t# Assets    %d\n", tmplay->assref.cnt);
            xOutInfo("PACKER", "\tSize        %d\n", tmplay->laysize);
            if (tmplay->assref.cnt > 0) {
                tmpass = (st_PACKER_ATOC_NODE*)tmplay->assref.list[0];
                xASSERT_L(663, tmpass->d_off>0);
                xOutInfo("PACKER", "\tHIPPos      %d\n", tmpass->d_off);
            } else {
                xOutInfo("PACKER", "\t(Empty layer will be skipped)\n");
            }
            xOutInfo("PACKER", "\n");
        }
        xOutInfo("PACKER", "PKR: -- End Layer Info\n", pr->packfile);
        xOutInfo("PACKER", "========\n");
    }
#endif

    return pr;
}

static void PKR_ReadDone(st_PACKER_READ_DATA* pr)
{
    S32 i = 0;
    S32 j = 0;
    S32 lockid = -1;
    st_PACKER_ATOC_NODE* assnode = NULL;
    st_PACKER_LTOC_NODE* laynode = NULL;
    st_XORDEREDARRAY* tmplist = NULL;

    xASSERT_L(690, pr);
    if (!pr) {
        return;
    }

    for (j = pr->laytoc.cnt - 1; j >= 0; j--) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[j];

#if DEBUG || RELEASE
        en_PKR_LAYER_LOAD_DEST loadDest = PKR_layerLoadDest(laynode->laytyp);
#endif

        for (i = laynode->assref.cnt - 1; i >= 0; i--) {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];

#if DEBUG || RELEASE
            U32 memTag = PKR_lookupMemtag(laynode->laytyp);
            U32 assSize = (assnode->x_size > assnode->d_size) ? assnode->x_size : assnode->d_size;
            if (loadDest == PKR_LDDEST_KEEPSTATIC)
                xMemMgrPackerStaticRemove(assSize, memTag);
            else if (loadDest == PKR_LDDEST_KEEPMALLOC)
                xMemMgrPackerMallocRemove(assSize, memTag);
            else if (loadDest == PKR_LDDEST_RWHANDOFF)
                xMemMgrPackerApproxRWMallocRemove(assSize, memTag);
#endif

            if (assnode->typeref)
                if (assnode->typeref->assetUnloaded)
                    if (!(assnode->loadflag & PKR_LDASS_ISDUP))
                        assnode->typeref->assetUnloaded(assnode->memloc, assnode->aid);
        }
    }

    for (i = 0; i < pr->laytoc.cnt; i++) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        if (!laynode->laymem) {
            continue;
        }

        xASSERT_L(791, (PKR_layerLoadDest(laynode->laytyp) == PKR_LDDEST_KEEPSTATIC) || (PKR_layerLoadDest(laynode->laytyp) == PKR_LDDEST_KEEPMALLOC));
        PKR_LayerMemRelease(pr, laynode);
        laynode->laymem = NULL;
    }

    PKR_kiilpool_anode(pr);

    for (i = 0; i < pr->laytoc.cnt; i++) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        xASSERT_L(814, !laynode->danglecnt);
        PKR_oldlaynode(laynode);
    }

    XOrdDone(&pr->asstoc, FALSE);
    XOrdDone(&pr->laytoc, FALSE);

    for (i = 0; i < PKR_MAX_TYPES+1; i++) {
        tmplist = &pr->typelist[i];
        if (tmplist->max)
            XOrdDone(tmplist, FALSE);
    }

    if (pr->pkg) {
        g_hiprf->destroy(pr->pkg);
        pr->pkg = NULL;
    }

    lockid = pr->lockid;
    memset(pr, 0, sizeof(st_PACKER_READ_DATA));
    g_loadlock &= ~(1 << lockid);
}

static S32 PKR_SetActive(st_PACKER_READ_DATA* pr, en_LAYER_TYPE layer)
{
    S32 result = TRUE;
    S32 rc = 0;
    S32 i = 0;
    S32 j = 0;
    st_PACKER_ATOC_NODE* assnode = NULL;
    st_PACKER_LTOC_NODE* laynode = NULL;

    xASSERT_L(867, pr);
    if (!pr) return FALSE;

    xOutInfo("PACKER", "PKR: Activating Assets\n");

    for (i = 0; i < pr->laytoc.cnt; i++) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        xASSERT_L(875, laynode);

        if (layer > PKR_LTYPE_DEFAULT && laynode->laytyp != layer) {
            continue;
        }

        for (j = 0; j < laynode->assref.cnt; j++) {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[j];
            xASSERT_L(883, assnode);

            xOutBabble("PACKER", "PKR: Setting Active LID: %d (%s) AID: 0x%08X (%s)\n", laynode->laytyp,
                       g_strz_layername[laynode->laytyp], assnode->aid,
                       (assnode->loadflag & PKR_LDASS_READY) ? "Ready" : "Not Ready! - Skipping");

            if (assnode->loadflag & PKR_LDASS_ACTIVE) continue;
            if (!(assnode->loadflag & PKR_LDASS_READY)) {
                continue;
            }

            xASSERT_L(902, assnode->typeref);
            if (!assnode->typeref) {
                xOutWarn("PACKER", "PKR: Asset '%s' has unhandled type %s (default support: Retrieve asset by ID only!)\n",
                         assnode->name, xUtil_idtag2string(assnode->asstype, 0));
                continue;
            }

            if (!assnode->typeref->assetLoaded) continue;

            xASSERT_L(912, assnode->memloc);
            rc = assnode->typeref->assetLoaded(pr->userdata, assnode->aid, assnode->memloc, assnode->d_size);
            xASSERT_L(918, rc);
            if (!rc) {
                xOutWarn("PACKER", "PKR: AssetLoad Client Failed LID: %d AID: 0x%08X\n", laynode->laytyp, assnode->aid);
                result = FALSE;
            } else {
                assnode->loadflag |= PKR_LDASS_ACTIVE;
            }
        }
    }

    xOutInfo("PACKER", "PKR: Activation Complete (%s)\n\n", pr->packfile);

    return result;
}

static S32 PKR_parse_TOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 is_ok = FALSE;
    U32 cid = 0;
    S32 done = FALSE;

    cid = g_hiprf->enter(pkg);
    xASSERT_L(951, cid);
    while (cid) {
        switch (cid) {
        case 'HIPA':
            is_ok = LOD_r_HIPA(pkg, pr);
            break;
        case 'PACK':
            LOD_r_PACK(pkg, pr);
            xASSERT_L(964, pr->asscnt < PKR_MAX_ARRAY);
            xASSERT_L(965, pr->laycnt < PKR_MAX_ARRAY);
            if (pr->asscnt <= 0)
                xOutWarn("PACKER", "PKR: No assets\n");
            xASSERT_L(971, pr->laycnt>0);
            if (is_ok) {
                if (pr->asscnt > 0) {
                    XOrdInit(&pr->asstoc, pr->asscnt, eMemMgrTag_PackerTOC, FALSE);
                    PKR_makepool_anode(pr, pr->asscnt);
                }
                if (pr->laycnt > 0) XOrdInit(&pr->laytoc, pr->laycnt, eMemMgrTag_PackerTOC, FALSE);
            }
            break;
        case 'DICT':
            LOD_r_DICT(pkg, pr);
            PKR_bld_typecnt(pr);
            done = TRUE;
            break;
        case 'STRM':
            xOutWarn("PACKER", "PKR: Failed no-parse Asset Data Stream\n");
            LOD_r_STRM(pkg, pr);
            break;
        }
        if (!is_ok) {
            xOutWarn("PACKER", "PKR: Not HIP File or Unsupported Version\n");
            break;
        }
        g_hiprf->exit(pkg);
        if (done) break; cid = g_hiprf->enter(pkg);
    }

    return is_ok;
}

static S32 PKR_LoadStep_Async()
{
    S32 moretodo = TRUE;
    S32 rc = 0;
    en_READ_ASYNC_STATUS readstat = HIP_RDSTAT_NONE;
    st_PACKER_ATOC_NODE* tmpass;

    static st_PACKER_READ_DATA* curpr = NULL;
    static st_PACKER_LTOC_NODE* asynlay = NULL;

    xVALIDATE_L(1474, g_hiprf);

    if (!asynlay) {
        xOutInfo("PACKER", "PKR: Searching for next layer to load\n");

        PKR_findNextLayerToLoad(&curpr, &asynlay);
        if (asynlay) {
            xOutInfo("PACKER", "PKR: {%s} Layer %d (%s) - Start of processing\n", curpr->packfile, asynlay->laytyp,
                     g_strz_layername[asynlay->laytyp]);

            en_PKR_LAYER_LOAD_DEST loaddest = PKR_layerLoadDest(asynlay->laytyp);
            if (loaddest != PKR_LDDEST_SKIP && asynlay->laysize > 1 && asynlay->assref.cnt > 0) {
                xOutInfo("PACKER", "PKR: {%s} Layer %d (%s) - Loading (size:%d)\n", curpr->packfile, asynlay->laytyp,
                         g_strz_layername[asynlay->laytyp], asynlay->laysize);

                asynlay->laymem = PKR_LayerMemReserve(curpr, asynlay);
                xASSERT_L(1515, asynlay->laymem);

                PKR_drv_guardLayer(asynlay);

                tmpass = (st_PACKER_ATOC_NODE*)asynlay->assref.list[0];
                xVALIDATE_L(1527, tmpass);
                xVALIDATE_L(1528, tmpass->d_off>0);

                g_hiprf->setSpot(tmpass->ownpkg, tmpass->d_off);

#if DEBUG
                static S32 gametotal = 0;
                gametotal += asynlay->laysize;
                xOutInfo("PACKER", "PKR: Total Bytes Read: %u\n", gametotal);
#endif

                rc = g_hiprf->readBytes(tmpass->ownpkg, asynlay->laymem, asynlay->laysize);
                xVALIDATE_L(1569, rc);
                if (rc) {
                    xOutInfo("PACKER", "PKR: {%s} Layer %d (%s) - Queued\n", curpr->packfile, asynlay->laytyp,
                             g_strz_layername[asynlay->laytyp]);
                    asynlay->flg_ldstat |= PKR_LDLAY_INPROG;
                } else {
                    xOutWarn("PACKER", "PKR: {%s} Layer %d (%s) - Que Read:Fail\n", curpr->packfile, asynlay->laytyp,
                             g_strz_layername[asynlay->laytyp]);
                    PKR_LayerMemRelease(curpr, asynlay);
                    asynlay->flg_ldstat &= ~(PKR_LDLAY_INPROG | PKR_LDLAY_ISDONE);
                    curpr = NULL;
                    asynlay = NULL;
                }

                moretodo = TRUE;
            } else {
                xOutInfo("PACKER", "PKR: {%s} Layer %d (%s) - Skipping\n", curpr->packfile, asynlay->laytyp,
                         g_strz_layername[asynlay->laytyp]);
                asynlay->flg_ldstat |= PKR_LDLAY_ISDONE;
                asynlay = NULL;
                moretodo = TRUE;
            }
        } else {
            curpr = NULL;
            asynlay = NULL;
            moretodo = FALSE;
        }
    } else {
        xVALIDATE_L(1636, curpr);
        xVALIDATE_L(1637, asynlay->flg_ldstat & PKR_LDLAY_INPROG);
        xVALIDATE_L(1638, !(asynlay->flg_ldstat & PKR_LDLAY_ISDONE));

        readstat = g_hiprf->pollRead(curpr->pkg);
        if (readstat == HIP_RDSTAT_SUCCESS) {
            readstat = PKR_drv_guardVerify(asynlay);
            xASSERT_FMT_L(1648, (readstat==HIP_RDSTAT_SUCCESS), "Load failure - master build would reque layer %s->%d\n",
                          curpr->packfile, asynlay->laytyp);
        }

        xVALIDATE_L(1687, readstat != HIP_RDSTAT_NOBYPASS);
        xVALIDATE_L(1688, readstat != HIP_RDSTAT_NOASYNC);
        xVALIDATE_L(1689, readstat != HIP_RDSTAT_NONE);
        xVALIDATE_L(1690, readstat != HIP_RDSTAT_FAILED);

        if (readstat == HIP_RDSTAT_INPROG) {
            xOutBabble("PACKER", "PKR: {%s} Layer %d (%s) - in progress...\n", curpr->packfile, asynlay->laytyp,
                       g_strz_layername[asynlay->laytyp]);
            moretodo = TRUE;
        }
        else if (readstat == HIP_RDSTAT_SUCCESS) {
            xOutInfo("PACKER", "PKR: {%s} Layer %d (%s) - Load Complete\n", curpr->packfile, asynlay->laytyp,
                     g_strz_layername[asynlay->laytyp]);
            xOutInfo("PACKER", "PKR: {%s} Layer %d (%s) - Updating Assets\n", curpr->packfile, asynlay->laytyp,
                     g_strz_layername[asynlay->laytyp]);
            PKR_updateLayerAssets(asynlay);

            if (PKR_layerTypeNeedsXForm(asynlay->laytyp)) {
                xOutInfo("PACKER", "PKR: {%s} Layer %d (%s) - Transforming\n", curpr->packfile, asynlay->laytyp,
                         g_strz_layername[asynlay->laytyp]);
                PKR_xformLayerAssets(asynlay);
            }

            en_PKR_LAYER_LOAD_DEST loaddest = PKR_layerLoadDest(asynlay->laytyp);
            if (loaddest == PKR_LDDEST_RWHANDOFF) {
                PKR_LayerMemRelease(curpr, asynlay);
                xVALIDATE_L(1762, !(asynlay->laymem));
            }

            asynlay->flg_ldstat |= PKR_LDLAY_ISDONE;
            asynlay = NULL;
            moretodo = TRUE;
        } else {
            xOutWarn("PACKER", "PKR: {%s} Layer %d (%s) - AsyncLoad FAILED (%d)(reque)\n", curpr->packfile,
                     asynlay->laytyp, g_strz_layername[asynlay->laytyp], asynlay->flg_ldstat);

            en_PKR_LAYER_LOAD_DEST loaddest = PKR_layerLoadDest(asynlay->laytyp);
            if (asynlay->laymem && loaddest == PKR_LDDEST_RWHANDOFF) {
                PKR_LayerMemRelease(curpr, asynlay);
                xVALIDATE_L(1800, !(asynlay->laymem));
            }

            asynlay->flg_ldstat &= ~(PKR_LDLAY_INPROG | PKR_LDLAY_ISDONE);
            asynlay = NULL;
            curpr = NULL;
            moretodo = TRUE;
        }
    }

    return moretodo;
}

static char* PKR_LayerMemReserve(st_PACKER_READ_DATA* pr, st_PACKER_LTOC_NODE* layer)
{
    char* mem = NULL;
    en_PKR_LAYER_LOAD_DEST loaddest;

    if (layer->laymem) return layer->laymem;

    loaddest = PKR_layerLoadDest(layer->laytyp);
    switch (loaddest) {
    case PKR_LDDEST_SKIP:
        xFAIL_MSG_L(1853, "Huh?");
        break;
    case PKR_LDDEST_KEEPSTATIC:
        mem = (char*)PKR_getmem('LYR\0', layer->laysize, 0x8000 + layer->laytyp, 64, PKR_lookupMemtag(layer->laytyp));
        break;
    case PKR_LDDEST_KEEPMALLOC:
        mem = (char*)PKR_getmem('LYR\0', layer->laysize, 0x8000 + layer->laytyp, 64, PKR_lookupMemtag(layer->laytyp), TRUE,
                                &layer->laytru);
        break;
    case PKR_LDDEST_RWHANDOFF:
        PKR_push_memmark();
        mem = (char*)PKR_getmem('LYR\0', layer->laysize, 0x8000 + layer->laytyp, 64, PKR_lookupMemtag(layer->laytyp));
        break;
    default:
        xFAIL_MSG_L(1913, "Unknown layer allocate type");
        break;
    }

    xASSERT_FMT_L(1919, mem, "Layer Alloc Failed: %s -> LIDX %d (%s)\n", pr->packfile, layer->laytyp,
                  g_strz_layername[layer->laytyp]);

    return mem;
}

static void PKR_LayerMemRelease(st_PACKER_READ_DATA*, st_PACKER_LTOC_NODE* layer)
{
    en_PKR_LAYER_LOAD_DEST loaddest = PKR_layerLoadDest(layer->laytyp);

    switch (loaddest) {
    case PKR_LDDEST_SKIP:
        break;
    case PKR_LDDEST_RWHANDOFF:
        PKR_relmem('LYR\0', layer->laysize, layer->laymem, 0x8000 + layer->laytyp, FALSE);
        PKR_pop_memmark();
        layer->laymem = NULL;
        break;
    case PKR_LDDEST_KEEPSTATIC:
        PKR_relmem('LYR\0', layer->laysize, layer->laymem, 0x8000 + layer->laytyp, FALSE);
        break;
    case PKR_LDDEST_KEEPMALLOC:
        PKR_relmem('LYR\0', layer->laysize, layer->laytru, 0x8000 + layer->laytyp, TRUE);
        layer->laymem = NULL;
        layer->laytru = NULL;
        break;
    default:
        xFAIL_MSG_L(2002, "Unknown layer load dest type");
        break;
    }
}

void PKR_drv_guardLayer(st_PACKER_LTOC_NODE*)
{
}

en_READ_ASYNC_STATUS PKR_drv_guardVerify(st_PACKER_LTOC_NODE*)
{
    en_READ_ASYNC_STATUS confirm = HIP_RDSTAT_SUCCESS;

    return confirm;
}

static en_PKR_LAYER_LOAD_DEST PKR_layerLoadDest(en_LAYER_TYPE laytyp)
{
    en_PKR_LAYER_LOAD_DEST load_place = PKR_LDDEST_KEEPSTATIC;

    switch (laytyp) {
    case PKR_LTYPE_TEXTURE_STRM:
    case PKR_LTYPE_SRAM:
    case PKR_LTYPE_SNDTOC:
    case PKR_LTYPE_CUTSCENE:
        load_place = PKR_LDDEST_SKIP;
        break;
    case PKR_LTYPE_TEXTURE:
    case PKR_LTYPE_BSP:
    case PKR_LTYPE_MODEL:
        load_place = PKR_LDDEST_RWHANDOFF;
        break;
    case PKR_LTYPE_JSPINFO:
        load_place = PKR_LDDEST_KEEPMALLOC;
        break;
    case PKR_LTYPE_DEFAULT:
    case PKR_LTYPE_ANIMATION:
    case PKR_LTYPE_VRAM:
    case PKR_LTYPE_CUTSCENETOC:
        load_place = PKR_LDDEST_KEEPSTATIC;
        break;
    default:
        xFAIL_FMT_L(2134, "Unrecognized layer type (%d)", laytyp);
        load_place = PKR_LDDEST_KEEPSTATIC;
        break;
    }

    return load_place;
}

static S32 PKR_layerTypeNeedsXForm(en_LAYER_TYPE laytyp)
{
    S32 do_xform = FALSE;

    switch (laytyp) {
    case PKR_LTYPE_TEXTURE:
    case PKR_LTYPE_BSP:
    case PKR_LTYPE_MODEL:
        do_xform = TRUE;
        break;
    case PKR_LTYPE_DEFAULT:
    case PKR_LTYPE_ANIMATION:
    case PKR_LTYPE_VRAM:
    case PKR_LTYPE_SNDTOC:
    case PKR_LTYPE_CUTSCENETOC:
    case PKR_LTYPE_JSPINFO:
        do_xform = TRUE;
        break;
    case PKR_LTYPE_SRAM:
    case PKR_LTYPE_CUTSCENE:
        do_xform = FALSE;
        break;
    default:
        xOutWarn("PACKER", "PKR: Do layer transform unknown type (%d)\n", laytyp);
        do_xform = FALSE;
        break;
    }

    return do_xform;
}

static S32 PKR_findNextLayerToLoad(st_PACKER_READ_DATA** work_on_pkg, st_PACKER_LTOC_NODE** next_layer)
{
    st_PACKER_READ_DATA* tmppr = NULL;
    st_PACKER_LTOC_NODE* tmplay = NULL;
    S32 i = 0;
    S32 j = 0;

    *next_layer = NULL;

    tmppr = *work_on_pkg;
    if (tmppr) {
        for (i = 0; i < tmppr->laytoc.cnt; i++) {
            tmplay = (st_PACKER_LTOC_NODE*)tmppr->laytoc.list[i];
            if (tmplay->flg_ldstat & PKR_LDLAY_ISDONE) continue;

            *next_layer = tmplay;
            *work_on_pkg = tmppr;
            break;
        }
    }

    if (!*next_layer) {
        for (j = 0; j < PKR_MAX_INSTANCES; j++) {
            if (!(g_loadlock & (1 << j))) continue;

            tmppr = &g_readdatainst[j];
            if (tmppr == *work_on_pkg) continue;

            for (i = 0; i < tmppr->laytoc.cnt; i++) {
                tmplay = (st_PACKER_LTOC_NODE*)tmppr->laytoc.list[i];
                if (tmplay->flg_ldstat & PKR_LDLAY_ISDONE) continue;

                *next_layer = tmplay;
                *work_on_pkg = tmppr;
                break;
            }
            if (*next_layer) break;
        }
    }

    return *next_layer ? TRUE : FALSE;
}

static void PKR_updateLayerAssets(st_PACKER_LTOC_NODE* laynode)
{
    xASSERT_L(2228, laynode->assref.cnt);

    S32 i = 0;
    st_PACKER_ATOC_NODE* tmpass = NULL;
    for (i = 0; i < laynode->assref.cnt; i++) {
        tmpass = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];
        xVALIDATE_L(2236, tmpass);
        if (tmpass->d_off > 0 && tmpass->d_size > 0) break;
        tmpass = NULL;
    }

    xASSERT_L(2241, tmpass);

    xASSERT_FMT_L(2243, (i==0), "Layer begins with asset of no size\n'%s'\n(very bad)",
                  ((st_PACKER_ATOC_NODE*)laynode->assref.list[0])->name);

    S32 lay_hip_pos = tmpass->d_off;

    if (lay_hip_pos % pkr_sector_size) {
        xASSERT_L(2251, 0&&"Detected layer out of alignment");
    }

    xOutInfo("PACKER", "PKR: Layer (type %d: %s); Pos - file: 0x%08X mem: 0x%08X\n", laynode->laytyp,
             g_strz_layername[laynode->laytyp], lay_hip_pos, laynode->laymem);

    for (i = 0; i < laynode->assref.cnt; i++) {
        tmpass = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];
        xVALIDATE_L(2264, tmpass);

        if (tmpass->loadflag & PKR_LDASS_ISDUP) {
            continue;
        }

        if (tmpass->loadflag & PKR_LDASS_ISNIL) {
            xOutWarn("PACKER", "PKR: .. Asset 0x%08X '%s' (size 0) skipped\n", tmpass->aid, tmpass->name);
            tmpass->memloc = NULL;
            continue;
        }

        xVALIDATE_L(2281, tmpass->d_size > 0);

        xVALIDATE_L(2283, tmpass->d_off >= lay_hip_pos);
        S32 reloff = tmpass->d_off - lay_hip_pos;
        xVALIDATE_L(2285, reloff < laynode->laysize);

        tmpass->memloc = laynode->laymem + reloff;
        tmpass->loadflag |= PKR_LDASS_READY;

        xOutBabble("PACKER", "PKR: Placed AID 0x%08X at address 0x%08X (%s)\n", tmpass->aid, tmpass->memloc, tmpass->name);

        static xTWEAKABLE S32 doIntegrityChecks = FALSE;
        if (doIntegrityChecks) {
            if (tmpass->ownpr->subver > 1) {
                xVALIDATE_L(2302, tmpass->d_size > 0);

                U32 tmpchk = xUtil_crc_init();
                tmpchk = xUtil_crc_update(tmpchk, tmpass->memloc, tmpass->d_size);

                xVALIDATE_L(2307, tmpchk==tmpass->d_chksum);
                if (tmpchk != tmpass->d_chksum) {
                    xOutWarn("PACKER", "PKR: .. AID 0x%08X Integrity FAILURE (0x%08X/0x%08X)\n", tmpass->aid, tmpass->name,
                             tmpchk, tmpass->d_chksum);
                } else {
                    xOutBabble("PACKER", "PKR: .. AID 0x%08X Integrity OK (0x%08X)\n", tmpass->aid, tmpass->name, tmpchk);
                }
            }
        }
    }

    static xTWEAKABLE S32 dumpRawBuffer = FALSE;
    if (dumpRawBuffer) {
        xOutInfo("PACKER", "PKR: Layer memory block ... will be %s\n",
                 (PKR_layerLoadDest(laynode->laytyp) != PKR_LDDEST_SKIP) ? "kept" : "dumped");

        static S32 dump_output = FALSE;
        if (dump_output) {
            xUtil_niceBufDump(laynode->laymem, laynode->laysize, TRUE, lay_hip_pos);
            xDebugVSync();
        }
    }
}

static void PKR_xformLayerAssets(st_PACKER_LTOC_NODE* laynode)
{
    S32 i = 0;
    S32 will_be_dumped = FALSE;
    st_PACKER_ATOC_NODE* tmpass = NULL;

    xVALIDATE_L(2347, laynode->assref.cnt);

    en_PKR_LAYER_LOAD_DEST loaddest = PKR_layerLoadDest(laynode->laytyp);
    if (loaddest == PKR_LDDEST_RWHANDOFF) will_be_dumped = TRUE;

    for (i = 0; i < laynode->assref.cnt; i++) {
        tmpass = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];
        if (tmpass->loadflag & PKR_LDASS_ISDUP) {
            continue;
        }

        PKR_xform_asset(tmpass, will_be_dumped);

        if (will_be_dumped && tmpass->x_size < 1) {
            xOutWarn("PACKER", "PKR: WARN: AID 0x%08X '%s' in a dumpable layer but either no transform or transform did not relocate asset's data (which is being set to NULL)\n",
                     tmpass->aid, tmpass->name);
            tmpass->memloc = NULL;
        }
    }
}

static void PKR_xform_asset(st_PACKER_ATOC_NODE* assnode, S32 dumpable_layer)
{
    char* xformloc = NULL;
    const PKRAssetType* atype;

    xVALIDATE_L(2390, assnode);

    if (!(assnode->infoflag & PKR_INFO_READ_TRANSFORM)) {
        if (assnode->typeref && assnode->typeref->readXForm) {
            xOutWarn("PACKER", "PKR: Asset '%s', (of type %s) supports read transformations but was not flagged for transform processing during loading - is this intentional?\n",
                     assnode->name, xUtil_idtag2string(assnode->asstype, 0));
        } else {
            return;
        }
    }

    atype = assnode->typeref;
    xVALIDATE_L(2408, atype);
    if (!atype) {
        xOutWarn("PACKER", "PKR: Asset %s flagged transform but unknown type\n", assnode->name);
        return;
    }

    xVALIDATE_L(2417, atype->readXForm);
    if (!atype->readXForm) {
        xOutWarn("PACKER", "PKR: Asset %s flagged transform but no callback\n", assnode->name);
        return;
    }

    if (assnode->d_size == 0) {
        xOutWarn("PACKER", "PKR: Read XForm of %s (%s) has no data - processing anyway\n", assnode->name,
                 xUtil_idtag2string(assnode->asstype, 0));
        assnode->memloc = NULL;
    }

#if DEBUG
    g_tym_traklast[1] = iTimeGet();
#endif

    xformloc = (char*)atype->readXForm(assnode->ownpr->userdata, assnode->aid, assnode->memloc, assnode->d_size,
                                       (U32*)&assnode->x_size);

    if (assnode->aid != 0x7AB6743A && assnode->d_size != 0) {
        xASSERT_FMT_L(2458, assnode->x_size, "AID: 0x%08X - bad transform size\n", assnode->aid);
        if (dumpable_layer) {
            xASSERT_FMT_L(2462, (assnode->memloc != xformloc), "AID: 0x%08X - bad transform memory location\n", assnode->aid);
        }
    }

    if (!dumpable_layer && assnode->memloc == xformloc && assnode->x_size) {
        xOutWarn("PACKER", "PKR: BEWARE: In-place transform AID: 0x%08X '%s' (size %d)\n", assnode->aid, assnode->name,
                 assnode->x_size);
    }
    else if (assnode->d_size == 0 || assnode->x_size == 0) {
        assnode->memloc = NULL;
        xVALIDATE_L(2480, assnode->loadflag & PKR_LDASS_ISNIL);
        assnode->loadflag |= PKR_LDASS_ISNIL;
    } else
        assnode->memloc = xformloc;

#if DEBUG
    assnode->dbg_loadtime[1] += iTimeGet() - g_tym_traklast[1];
#endif
}

static void* PKR_FindAsset(st_PACKER_READ_DATA* pr, U32 aid, S32* assetsize)
{
    S32 idx = -1;
    st_PACKER_ATOC_NODE* assnode = NULL;

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx >= 0) {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
    }

    if (assetsize) {
        if (!assnode) {
            *assetsize = 0;
        }
        else if (!assnode->memloc) {
            *assetsize = 0;
        }
        else if (assnode->x_size > 0) {
            *assetsize = assnode->x_size;
        } else {
            *assetsize = assnode->d_size;
        }
    }

    if (assnode) {
        xVALIDATE_L(2531, !(assnode->loadflag & PKR_LDASS_ISDUP));
        xVALIDATE_L(2532, !(assnode->loadflag & PKR_LDASS_ISNIL));
        return assnode->memloc;
    }

    return NULL;
}

st_PACKER_ATOC_NODE* PKR_FindAssetNode(st_PACKER_READ_DATA* pr, U32 aid, S32* assetsize)
{
    S32 idx = -1;
    st_PACKER_ATOC_NODE* assnode = NULL;

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx >= 0) {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
    }

    if (assetsize) {
        if (!assnode) {
            *assetsize = 0;
        }
        else if (!assnode->memloc) {
            *assetsize = 0;
        }
        else if (assnode->x_size > 0) {
            *assetsize = assnode->x_size;
        } else {
            *assetsize = assnode->d_size;
        }
    }

    if (assnode) {
        xVALIDATE_L(2580, !(assnode->loadflag & PKR_LDASS_ISDUP));
        xVALIDATE_L(2581, !(assnode->loadflag & PKR_LDASS_ISNIL));
        return assnode;
    }

    return NULL;
}

static S32 PKR_LoadLayer(st_PACKER_READ_DATA*, en_LAYER_TYPE)
{
    return 0;
}

static void* PKR_LoadAsset(st_PACKER_READ_DATA* pr, U32 aid, const char*, void*, S32* assetsize)
{
    return PKR_FindAsset(pr, aid, assetsize);
}

static U32 PKR_GetAssetSize(st_PACKER_READ_DATA* pr, U32 aid)
{
    S32 idx = -1;
    const st_PACKER_ATOC_NODE* assnode = NULL;

    xVALIDATE_L(2619, pr);
    xVALIDATE_L(2620, aid);

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    xVALIDATE_L(2624, idx>-1);
    if (idx > -1) {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
        xVALIDATE_L(2628, assnode);
    } else {
        xOutWarn("PACKER", "PKR: Invalid GetAssetSize request: unknown ID (0x%08X)\n", aid);
    }

    if (assnode) {
        if (assnode->x_size > 0) return assnode->x_size;
        return assnode->d_size;
    }

    return 0;
}

static S32 PKR_AssetCount(st_PACKER_READ_DATA* pr, U32 type)
{
    S32 cnt = 0;
    S32 idx = -1;
    st_XORDEREDARRAY* tmplist = NULL;

    xVALIDATE_L(2649, pr);
    if (type == 0) return pr->asstoc.cnt;

    idx = PKR_typeHdlr_idx(pr, type);

    if (idx >= 0) {
        tmplist = &pr->typelist[idx];
        cnt = tmplist->cnt;
    }

    return cnt;
}

static void* PKR_AssetByType(st_PACKER_READ_DATA* pr, U32 type, S32 idx, U32* size)
{
    void* memloc = NULL;
    S32 typidx = 0;
    st_XORDEREDARRAY* typlist = NULL;
    st_PACKER_ATOC_NODE* assnode = NULL;

#if DEBUG
    g_tym_traklast[3] = iTimeGet();
#endif

    if (size) *size = 0;

    if (idx < 0) idx = 0;

    typidx = PKR_typeHdlr_idx(pr, type);
    xVALIDATE_L(2684, typidx>=0);
    if (typidx < 0) return memloc;

    typlist = &pr->typelist[typidx];
    xVALIDATE_L(2689, idx<typlist->cnt);
    if (idx >= typlist->cnt) return memloc;

    assnode = (st_PACKER_ATOC_NODE*)typlist->list[idx];
    xVALIDATE_L(2696, assnode);
    xVALIDATE_L(2697, assnode->loadflag & PKR_LDASS_READY);

    if (size) *size = assnode->d_size;

    memloc = assnode->memloc;

#if DEBUG
    pr->dbg_readtime[3] += iTimeGet() - g_tym_traklast[3];
#endif

    return memloc;
}

static S32 PKR_IsAssetReady(st_PACKER_READ_DATA* pr, U32 aid)
{
    xASSERT_L(2715, 0&&"Legacy: Who still needs this?");

    S32 is_ok = FALSE;
    st_PACKER_ATOC_NODE* assnode;
    S32 idx = -1;

    xVALIDATE_L(2720, pr);
    xVALIDATE_L(2721, aid);

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx < 0) {
        xOutWarn("PACKER", "PKR: Unknown Asset ID 0x%08X\n", aid);
    } else {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];

        xVALIDATE_L(2730, !(assnode->loadflag & PKR_LDASS_ISDUP));
        xVALIDATE_L(2731, !(assnode->loadflag & PKR_LDASS_ISNIL));

        if (assnode->loadflag & PKR_LDASS_READY) is_ok = TRUE;
        else is_ok = FALSE;
    }

    return is_ok;
}

static U32 PKR_getPackTimestamp(st_PACKER_READ_DATA* pr)
{
    xVALIDATE_EQ_L(2742, sizeof(time_t), sizeof(U32));
    return pr->time_made;
}

static void PKR_Disconnect(st_PACKER_READ_DATA* pr)
{
    xVALIDATE_L(2761, pr);
    xVALIDATE_L(2762, pr->pkg);

    if (pr->pkg) {
        DBprintf(DBML_DBG, "PKR: Disconnecting from '%s'\n", pr->packfile);
        g_hiprf->destroy(pr->pkg);
        pr->pkg = NULL;
    }
}

U32 PKRAssetIDFromInst(void* asset_inst)
{
    st_PACKER_ATOC_NODE* assnode = (st_PACKER_ATOC_NODE*)asset_inst;
    return assnode->aid;
}

static const char* PKR_AssetName(st_PACKER_READ_DATA* pr, U32 aid)
{
    const char* da_name = NULL;
    S32 idx = -1;
    st_PACKER_ATOC_NODE* assnode = NULL;

    if (!aid) return da_name;

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx >= 0) {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
        da_name = assnode->name;
    }

    return da_name;
}

static U32 PKR_GetBaseSector(st_PACKER_READ_DATA* pr)
{
    return pr->base_sector;
}

static S32 PKR_GetAssetInfo(st_PACKER_READ_DATA* pr, U32 aid, PKRAssetTOCInfo* tocinfo)
{
    S32 idx = -1;
    st_PACKER_ATOC_NODE* assnode = NULL;

    memset(tocinfo, 0, sizeof(PKRAssetTOCInfo));

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx < 0) return FALSE;

    assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];

    tocinfo->aid = aid;
    tocinfo->typeref = assnode->typeref;
    tocinfo->sector = assnode->d_off / pkr_sector_size;
    tocinfo->plus_offset = assnode->d_off % pkr_sector_size;
    tocinfo->size = assnode->d_size;
    tocinfo->mempos = assnode->memloc;
#if DEBUG
    tocinfo->assetname = assnode->name;
    tocinfo->filename = assnode->fullname;
#endif

    DBprintf(DBML_VDBG, "PKR: Asset '%s' SectorInfo: FileBase: %d Asset: %d Plus: %d\n", assnode->name, pr->base_sector,
             tocinfo->sector, tocinfo->plus_offset);

    return TRUE;
}

static S32 PKR_GetAssetInfoByType(st_PACKER_READ_DATA* pr, U32 type, S32 idx, PKRAssetTOCInfo* tocinfo)
{
    st_PACKER_ATOC_NODE* assnode = NULL;
    S32 typidx = 0;
    st_XORDEREDARRAY* typlist = NULL;

#if DEBUG
    g_tym_traklast[3] = iTimeGet();
#endif

    memset(tocinfo, 0, sizeof(PKRAssetTOCInfo));

    if (idx < 0) idx = 0;

    typidx = PKR_typeHdlr_idx(pr, type);
    xVALIDATE_L(2857, typidx>=0);
    if (typidx < 0) return FALSE;

    typlist = &pr->typelist[typidx];
    xVALIDATE_L(2862, (idx>=0) && (idx<typlist->cnt));
    if (idx >= typlist->cnt) return FALSE;

    assnode = (st_PACKER_ATOC_NODE*)typlist->list[idx];
    xVALIDATE_L(2869, assnode);
    xVALIDATE_L(2870, assnode->loadflag & PKR_LDASS_READY);

    tocinfo->aid = assnode->aid;
    tocinfo->typeref = assnode->typeref;
    tocinfo->sector = assnode->d_off / pkr_sector_size;
    tocinfo->plus_offset = assnode->d_off % pkr_sector_size;
    tocinfo->size = assnode->d_size;
    tocinfo->mempos = assnode->memloc;
#if DEBUG
    tocinfo->assetname = assnode->name;
    tocinfo->filename = assnode->fullname;
#endif

    DBprintf(DBML_VDBG, "PKR: Asset '%s' SectorInfo: FileBase: %d Asset: %d Plus: %d\n", assnode->name, pr->base_sector,
             tocinfo->sector, tocinfo->plus_offset);

#if DEBUG
    pr->dbg_readtime[3] += iTimeGet() - g_tym_traklast[3];
#endif

    return TRUE;
}

static S32 PKR_PkgHasAsset(st_PACKER_READ_DATA* pr, U32 aid)
{
    S32 rc = FALSE;
    S32 idx = -1;
    st_PACKER_ATOC_NODE* assnode = NULL;

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);

    if (idx < 0) rc = FALSE;
    else {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];

        rc = TRUE;

        if (assnode->loadflag & PKR_LDASS_ISDUP) rc = FALSE;
        else if (assnode->loadflag & PKR_LDASS_ISNIL) rc = FALSE;
    }

    return rc;
}

#if DEBUG || RELEASE
en_pkrfindmyass PKRFindMyAsset(st_PACKER_READ_DATA* pr, PKRAssetTOCInfo* tocinfo, const void* ptr_raw)
{
    xASSERT_L(2965, ptr_raw);
    S32 i_rawaddr = (S32)ptr_raw;
    S32 i;
    for (i = 0; i < pr->asstoc.cnt; i++) {
        const st_PACKER_ATOC_NODE* assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[i];
        xASSERT_L(2970, assnode);

        if (!assnode) continue;

        if (assnode->loadflag & PKR_LDASS_ISDUP) continue;

        if (assnode->loadflag & PKR_LDASS_ISNIL) continue;

        if (!assnode->memloc) {
            continue;
        }

        S32 isXForm = FALSE;
        if (assnode->x_size > 0) {
            isXForm = TRUE;
        }

        if (ptr_raw == assnode->memloc) {
            PKR_GetAssetInfo(pr, assnode->aid, tocinfo);

            if (isXForm) {
                return PKR_FIND_EXACT_XFORM;
            }
            return PKR_FIND_EXACT;
        }

        S32 addr_beg = (S32)assnode->memloc;
        S32 addr_end = addr_beg + (isXForm ? assnode->x_size : assnode->d_size);
        if (i_rawaddr >= addr_beg && i_rawaddr < addr_end) {
            if (isXForm) {
                return PKR_FIND_RANGE_XFORM;
            }
            return PKR_FIND_RANGE;
        }
    }

    return PKR_FIND_FAILED;
}
#endif

static S32 PKR_FRIEND_assetIsGameDup(U32 aid, const st_PACKER_READ_DATA* skippr); // UNUSED

static S32 PKR_FRIEND_assetIsGameDup(U32 aid, const st_PACKER_READ_DATA* skippr, S32 oursize, U32 ourtype, U32 chksum, char* our_fnam)
{
    S32 is_dup = FALSE;
    st_PACKER_READ_DATA* tmp_pr = NULL;
    const st_PACKER_ATOC_NODE* tmp_ass = NULL;
    S32 i = 0;
    S32 idx = -1;
    S32 bonus = 0;
    S32 is_a_sound = FALSE;

#if DEBUG
    static U32 stopOnAsset = 0;
    if (stopOnAsset && aid == stopOnAsset) {
        iDebugBreak();
    }
#endif

    if (aid == SIMPLE_SHADOW_TABLE_ID) return is_dup;

    for (i = 0; i < PKR_MAX_INSTANCES; i++) {
        if (!(g_loadlock & (1 << i))) {
            continue;
        }

        tmp_pr = &g_readdatainst[i];
        if (tmp_pr == skippr) {
            continue;
        }

        idx = XOrdLookup(&tmp_pr->asstoc, (void*)aid, OrdTest_R_AssetID);
        if (idx < 0) {
            continue;
        }

        tmp_ass = (st_PACKER_ATOC_NODE*)tmp_pr->asstoc.list[idx];
        xVALIDATE_L(3090, tmp_ass);

        is_a_sound = FALSE;
        if (!(tmp_ass->loadflag & PKR_LDASS_READY)) {
            xVALIDATE_MSG_L(3110, tmp_ass->asstype == ourtype, "NonReady asset of diff type");
            if (tmp_ass->asstype != 'SND ' && tmp_ass->asstype != 'SNDS') {
                continue;
            }

            is_a_sound = TRUE;
        }

        xOutWarn("PACKER", "PKR: Dupe Game Asset 0x%08X '%s' (%s)\n\t(Orig: '%s' Dupe: '%s')\n", tmp_ass->aid, tmp_ass->name,
                 xUtil_idtag2string(tmp_ass->asstype, 0), tmp_pr->packfile, !skippr ? "<unknown>" : skippr->packfile);

        if (is_a_sound) {
            xASSERT_FMT_L(3138, !is_a_sound, "PKR: Dupe Game Asset 0x%08X '%s' (%s)\n(Orig: '%s' Dupe: '%s')", tmp_ass->aid,
                          tmp_ass->name, xUtil_idtag2string(tmp_ass->asstype, 0), tmp_pr->packfile, skippr->packfile);
        }

        xVALIDATE_L(3148, ourtype!=0);
        if (ourtype != 0 && ourtype != tmp_ass->asstype) {
            xOutCrit("PACKER", "---: ---- '%s' - inconsistent types (%s/%s)\n", tmp_ass->name,
                     xUtil_idtag2string(tmp_ass->asstype, 0), xUtil_idtag2string(ourtype, 1));
            bonus++;
        }

        xVALIDATE_L(3157, oursize>=0);
        if (oursize >= 0 && oursize != tmp_ass->d_size) {
            xOutErr("PACKER", "---: ---- '%s' - inconsistent sizes (%d/%d) (%s)\n", tmp_ass->name, tmp_ass->d_size, oursize,
                    xUtil_idtag2string(tmp_ass->asstype, 0));
            bonus++;
        }

        xVALIDATE_L(3167, chksum!=0);
        if (chksum != 0 && chksum != tmp_ass->d_chksum) {
            xOutErr("PACKER", "---: ---- '%s' - inconsistent data (0x%08X/0x%08X) (%s)\n", tmp_ass->name, tmp_ass->d_chksum,
                    chksum, xUtil_idtag2string(tmp_ass->asstype, 0));
            bonus++;
        }

#if DEBUG
        if (bonus) {
            xOutWarn("PACKER", "---: ---- '%s' Source: %s Duplicate: %s\n", tmp_ass->name,
                     tmp_ass->fullname[0] ? tmp_ass->fullname : tmp_ass->name,
                     our_fnam ? (our_fnam[0] ? our_fnam : "<unknown>") : "<unknown>");
        }
#endif

        xASSERT_FMT_L(3195, (ourtype==0) || (ourtype == tmp_ass->asstype),
                      "Asset 0x%08X '%s' type '%s' in '%s' conflicts with asset 0x%08X '%s' type '%s' in '%s'\n", aid,
                      our_fnam ? (our_fnam[0] ? our_fnam : "<unknown>") : "<unknown>", xUtil_idtag2string(ourtype, 0),
                      skippr ? skippr->packfile : "??", tmp_ass->aid, tmp_ass->name, xUtil_idtag2string(tmp_ass->asstype, 0),
                      tmp_ass->ownpr->packfile);

        is_dup = TRUE;
        break;
    }

    return is_dup;
}

static S32 PKR_makepool_anode(st_PACKER_READ_DATA* pr, S32 cnt)
{
    st_PACKER_ATOC_NODE* asspool = NULL;
    S32 amount = 0;

    xVALIDATE_L(3223, cnt);
    if (!cnt) return amount;

    xVALIDATE_L(3226, !pr->pool_anode);
    xVALIDATE_L(3227, pr->pool_nextaidx==0);
    xVALIDATE_L(3228, pr->asscnt == cnt);

    amount = cnt * sizeof(st_PACKER_ATOC_NODE);
    asspool = (st_PACKER_ATOC_NODE*)PKR_getmem('ANOD', amount, 'FAKE', 64, eMemMgrTag_PackerAssetPool);

    if (asspool) {
        pr->pool_anode = asspool;
        pr->pool_nextaidx = 0;
    }

    xVALIDATE_L(3238, pr->pool_anode);

    return asspool ? amount : 0;
}

static void PKR_kiilpool_anode(st_PACKER_READ_DATA* pr)
{
    S32 amount = 0;

    if (pr->asscnt) {
        amount = pr->asscnt * sizeof(st_PACKER_ATOC_NODE);
        PKR_relmem('ANOD', amount, pr->pool_anode, 'FAKE', FALSE);
        pr->pool_anode = NULL;
        pr->pool_nextaidx = 0;
    }
}

static st_PACKER_ATOC_NODE* PKR_newassnode(st_PACKER_READ_DATA* pr, U32 aid)
{
    st_PACKER_ATOC_NODE* assnode = NULL;

    xVALIDATE_L(3266, aid);
    xVALIDATE_L(3267, pr);
    xVALIDATE_L(3268, pr->pool_anode);
    xASSERT_L(3269, pr->pool_nextaidx < pr->asscnt);

    assnode = &pr->pool_anode[pr->pool_nextaidx];
    pr->pool_nextaidx++;

    memset(assnode, 0, sizeof(st_PACKER_ATOC_NODE));
    assnode->aid = aid;

    return assnode;
}

static void PKR_oldassnode(st_PACKER_READ_DATA*, st_PACKER_ATOC_NODE* assnode); // UNUSED

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("PKR: AID:0x%08X instance 0x%08X going out of scope\n")
#endif

static st_PACKER_LTOC_NODE* PKR_newlaynode(en_LAYER_TYPE laytyp, S32 refcnt)
{
    st_PACKER_LTOC_NODE* laynode = NULL;

    laynode = (st_PACKER_LTOC_NODE*)PKR_getmem('LNOD', sizeof(st_PACKER_LTOC_NODE), 0x8000 + laytyp, 64,
                                               PKR_lookupMemtag(laytyp));
    xVALIDATE_L(3324, laynode);
    memset(laynode, 0, sizeof(st_PACKER_LTOC_NODE));

    laynode->laytyp = laytyp;
    XOrdInit(&laynode->assref, (refcnt > 1) ? refcnt : 2, PKR_lookupMemtag(laytyp), FALSE);

    return laynode;
}

static void PKR_oldlaynode(st_PACKER_LTOC_NODE* laynode)
{
    xVALIDATE_L(3343, laynode);
    xVALIDATE_L(3344, !laynode->danglecnt);
    XOrdDone(&laynode->assref, FALSE);
    PKR_relmem('LNOD', sizeof(st_PACKER_LTOC_NODE), laynode, 0x8000 + laynode->laytyp, FALSE);
}

static S32 OrdTest_R_Asset(const void* vkey, void* vitem); // UNUSED

static S32 OrdComp_R_Asset(void* vkey, void* vitem)
{
    S32 rc = 0;
    st_PACKER_ATOC_NODE* key = (st_PACKER_ATOC_NODE*)vkey;
    st_PACKER_ATOC_NODE* item = (st_PACKER_ATOC_NODE*)vitem;

    if (key->aid < item->aid) rc = -1;
    else if (key->aid > item->aid) rc = 1;
    else rc = 0;

    return rc;
}

static S32 OrdTest_R_AssetID(const void* vkey, void* vitem)
{
    S32 rc = 0;
    U32 key = (U32)vkey;
    st_PACKER_ATOC_NODE* item = (st_PACKER_ATOC_NODE*)vitem;

    xASSERT_FMT_L(3393, (key&&item), "Key == 0x%08X; item == 0x%08X; tested ID == 0x%08X", key, item, item ? item->aid : 0);

    if (key < item->aid) rc = -1;
    else if (key > item->aid) rc = 1;
    else rc = 0;

    return rc;
}

static S32 OrdTest_R_AssetType(const void* vkey, void* vitem); // UNUSED

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("Key == 0x%08X; item == 0x%08X; tested type == 0x%08X")
#endif

static S32 OrdComp_R_AssetType(void* vkey, void* vitem); // UNUSED
static S32 OrdTest_R_Layer(const void* vkey, void* vitem); // UNUSED
static S32 OrdTest_R_LayerType(const void* vkey, void* vitem); // UNUSED

static S32 LOD_r_HIPA(st_HIPLOADDATA*, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;

    pr->pkgver = 'HIPA';

    return result;
}

static S32 LOD_r_PACK(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'PVER':
            LOD_r_PVER(pkg, pr);
            break;
        case 'PFLG':
            LOD_r_PFLG(pkg, pr);
            break;
        case 'PCNT':
            LOD_r_PCNT(pkg, pr);
            break;
        case 'PCRT':
            LOD_r_PCRT(pkg, pr);
            break;
        case 'PMOD':
            LOD_r_PMOD(pkg, pr);
            break;
        case 'PLAT':
            LOD_r_PLAT(pkg, pr);
            break;
        default:
            xVALIDATE_FAIL_L(3552);
            xOutWarn("PACKER", "PKR: Unknown SubChunk ID 0x%08X in PACK", cid);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    return result;
}

static S32 LOD_r_PVER(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    S32 ver = 0;
    S32 amt = 0;

    amt = g_hiprf->readLongs(pkg, &ver, 1);
    pr->subver = ver;
    if (ver < 2) {
        DBprintf(DBML_VDBG, "PKR: Legacy HIP file detected (%d). Some information may not be available.\n");
        PKR_spew_verhist();
    }

    ver = -1;
    amt = g_hiprf->readLongs(pkg, &ver, 1);
    if (amt != 1) {
        DBprintf(DBML_VDBG, "PKR: Legacy HIP: PVER no client version\n");
    }
    pr->cltver = ver;

    ver = -1;
    amt = g_hiprf->readLongs(pkg, &ver, 1);
    if (amt != 1) {
        DBprintf(DBML_VDBG, "PKR: Legacy HIP: PVER incompatible version\n");
        pr->compatver = 1;
    } else {
        pr->compatver = ver;
    }

    iprintf("--> '%s' internal version 0x%08X; client version 0x%08X\n", pr->packfile, pr->subver, pr->cltver);

    return result;
}

static S32 LOD_r_PFLG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*)
{
    S32 result = TRUE;
    S32 pkgflags = 0;

    g_hiprf->readLongs(pkg, &pkgflags, 1);

    return result;
}

static S32 LOD_r_PCNT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    S32 ival = 0;

    g_hiprf->readLongs(pkg, &ival, 1);
    xVALIDATE_L(3668, ival);
    pr->asscnt = ival;

    g_hiprf->readLongs(pkg, &ival, 1);
    xVALIDATE_L(3673, ival);
    pr->laycnt = ival;

    g_hiprf->readLongs(pkg, &ival, 1);

    g_hiprf->readLongs(pkg, &ival, 1);

    g_hiprf->readLongs(pkg, &ival, 1);

    return result;
}

static S32 LOD_r_PCRT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    S32 ival = 0;
    char tymbuf[256] = {};

    g_hiprf->readLongs(pkg, &ival, 1);
    pr->time_made = ival;

    if (pr->subver > 1) {
        g_hiprf->readString(pkg, tymbuf);
    }

    return result;
}

static S32 LOD_r_PMOD(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    S32 ival = 0;

    g_hiprf->readLongs(pkg, &ival, 1);
    pr->time_mod = ival;

    return result;
}

static S32 ValidatePlatform(st_HIPLOADDATA*, st_PACKER_READ_DATA*, char* plat, char* vid, char* lang, char* title)
{
    xASSERT_L(3780, strlen(plat));
    xASSERT_L(3781, strlen(vid));

    char fullname[128] = {};
    sprintf(fullname, "%s %s %s %s", plat, vid, lang, title);

    S32 rc = !stricmp(plat, "GC");
    xASSERT_FMT_L(3813, rc, "Target Platform? '%s'", fullname);
    if (!rc) return FALSE;

    if (zMainIsNTSC())
        rc = !stricmp(vid, "NTSC");
    else if (zMainIsPAL())
        rc = !stricmp(vid, "PAL");
    else
        xASSERT_FMT_L(3826, rc, "Target Video? '%s'", fullname);
    if (!rc) return FALSE;

    rc = !stricmp(title, "Incredibles");
    xASSERT_FMT_L(3838, rc, "Target Game Title? '%s'", fullname);
    if (!rc) return FALSE;

    return TRUE;
}

static S32 LOD_r_PLAT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    char platname[32] = {};
    char vidname[32] = {};
    char langname[32] = {};
    char titlename[32] = {};
    S32 n, rc;

    n = g_hiprf->readString(pkg, platname);
    xASSERT_MSG_L(3857, n, "Missing Platform Designator");

    n = g_hiprf->readString(pkg, vidname);
    xASSERT_MSG_L(3860, n, "Missing Video Display Designator");

    n = g_hiprf->readString(pkg, langname);
    xASSERT_MSG_L(3863, n, "Missing Language Designator");

    n = g_hiprf->readString(pkg, titlename);
    xASSERT_MSG_L(3866, n, "Missing Title Designator");
    if (!n) strcpy(titlename, "<Unknown>");

    rc = ValidatePlatform(pkg, pr, platname, vidname, langname, titlename);
    if (!rc) result = FALSE;

    return result;
}

static S32 LOD_r_DICT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'ATOC':
            LOD_r_ATOC(pkg, pr);

            DBprintf(DBML_VDBG, "PKR: Asset TOC Sorting ...\n");
            XOrdSort(&pr->asstoc, OrdComp_R_Asset);
            DBprintf(DBML_VDBG, "PKR: ... Sorted\n");
            break;
        case 'LTOC':
            LOD_r_LTOC(pkg, pr);
            break;
        default:
            xVALIDATE_FAIL_L(3917);
            xOutWarn("PACKER", "PKR: Unknown SubChunk ID 0x%08X in DICT", cid);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    return result;
}

static S32 LOD_r_ATOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;

    DBprintf(DBML_VDBG, "PKR: Asset TOC Loading ...\n");

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'AINF':
            LOD_r_AINF(pkg, pr);
            break;
        case 'AHDR':
            LOD_r_AHDR(pkg, pr);
            break;
        default:
            xVALIDATE_FAIL_L(3963);
            xOutWarn("PACKER", "PKR: Unknown SubChunk ID 0x%08X in ATOC\n", cid);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    DBprintf(DBML_VDBG, "PKR: Asset TOC Loaded %d Entries\n", pr->asstoc.cnt);

    return result;
}

static S32 LOD_r_AINF(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*)
{
    S32 result = TRUE;
    S32 ival = 0;

    g_hiprf->readLongs(pkg, &ival, 1);

    return result;
}

static S32 LOD_r_AHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;
    S32 ival = 0;
    S32 isdup = FALSE;
    st_PACKER_ATOC_NODE* assnode = NULL;

    g_hiprf->readLongs(pkg, &ival, 1);
    xVALIDATE_L(4038, ival);

    DBprintf(DBML_VDBG, "PKR: Adding TOC Entry (AID:0x%08X)\n", ival);
    assnode = PKR_newassnode(pr, ival);
    assnode->ownpr = pr;
    assnode->ownpkg = pkg;
    XOrdAppend(&pr->asstoc, assnode);
    DBprintf(DBML_VDBG, "PKR: TOC Entry Added (AID:0x%08X)\n", ival);

    g_hiprf->readLongs(pkg, &ival, 1);
    xVALIDATE_L(4123, ival);
    assnode->asstype = ival;
    assnode->typeref = PKR_type2typeref(assnode->asstype, pr->types);

    g_hiprf->readLongs(pkg, &ival, 1);
    xVALIDATE_L(4129, ival!=PKR_DUMBNUM);
    assnode->d_off = ival;
    xVALIDATE_L(4131, assnode->d_off);

    g_hiprf->readLongs(pkg, &ival, 1);
    xVALIDATE_L(4135, ival!=PKR_DUMBNUM);
    assnode->d_size = ival;
    assnode->readrem = ival;
    assnode->readcnt = 0;
    xVALIDATE_L(4139, assnode->d_size);
    if (assnode->d_size < 1) assnode->loadflag |= PKR_LDASS_ISNIL;

    g_hiprf->readLongs(pkg, &ival, 1);
    xVALIDATE_L(4144, ival!=PKR_DUMBNUM);
    assnode->d_pad = ival;

    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->infoflag = ival;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'ADBG':
            LOD_r_ADBG(pkg, pr, assnode);
            break;
        default:
            xVALIDATE_FAIL_L(4175);
            xOutWarn("PACKER", "PKR: Unknown SubChunk ID 0x%08X in AHDR", cid);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

#if DEBUG
    isdup = PKR_FRIEND_assetIsGameDup(assnode->aid, pr, assnode->d_size, assnode->asstype, assnode->d_chksum,
                                      assnode->fullname[0] ? assnode->fullname : assnode->name);
#else
    isdup = PKR_FRIEND_assetIsGameDup(assnode->aid, pr, assnode->d_size, assnode->asstype, assnode->d_chksum, NULL);
#endif
    if (isdup) assnode->loadflag |= PKR_LDASS_ISDUP;

#if DEBUG
    if (result) {
        if (!assnode->aid) {
            xOutWarn("PACKER", "PKR: Asset '%s' (type: %s) has no Asset ID!!!\n",
                     assnode->name[0] ? assnode->name : "Unnamed?!? (Bingo! Problem++)",
                     xUtil_idtag2string(assnode->asstype, 0));
        }

        if (!assnode->d_size) {
            xOutWarn("PACKER", "PKR: Asset '%s' (type: %s) has data size of 0\n",
                     assnode->name[0] ? assnode->name : "Unnamed?!? (Bingo! Problem++)",
                     xUtil_idtag2string(assnode->asstype, 0));
        }

        if (assnode->d_size >= 500000) {
            DBprintf(DBML_VDBG, "PKR: AID 0x%08X ('%s') requires large xform buffer (%d)\n", assnode->aid, assnode->name,
                     assnode->d_size);
        }
    }
#endif

    return result;
}

static S32 LOD_r_ADBG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr, st_PACKER_ATOC_NODE* assnode)
{
    S32 result = TRUE;
    S32 ival = 0;
    char tmpbuf[256] = {};
    S32 amt;

    amt = g_hiprf->readLongs(pkg, &ival, 1);
    assnode->assalign = ival;

    g_hiprf->readString(pkg, tmpbuf);
    strncpy(assnode->name, tmpbuf, sizeof(assnode->name) - 1);
    assnode->name[sizeof(assnode->name) - 1] = '\0';

    tmpbuf[0] = '\0';
    g_hiprf->readString(pkg, tmpbuf);
#if DEBUG
    strncpy(assnode->fullname, tmpbuf, sizeof(assnode->fullname) - 1);
    assnode->fullname[sizeof(assnode->fullname) - 1] = '\0';
#endif

    if (pr->subver > 1) {
        amt = g_hiprf->readLongs(pkg, &ival, 1);
        xVALIDATE_L(4285, amt==1);
        xVALIDATE_L(4286, ival!=PKR_DUMBNUM);
        assnode->d_chksum = ival;
    }

    return result;
}

static S32 LOD_r_LTOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'LINF':
            LOD_r_LINF(pkg, pr);
            break;
        case 'LHDR':
            LOD_r_LHDR(pkg, pr);
            break;
        default:
            xVALIDATE_FAIL_L(4326);
            xOutWarn("PACKER", "PKR: Unknown SubChunk ID 0x%08X in LTOC\n", cid);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    return result;
}

static S32 LOD_r_LINF(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*)
{
    S32 result = TRUE;
    S32 ival = 0;

    g_hiprf->readLongs(pkg, &ival, 1);

    return result;
}

static S32 LOD_r_LHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;
    S32 ival = 0;
    en_LAYER_TYPE laytyp = PKR_LTYPE_NOMORE;
    S32 refcnt = 0;
    S32 idx = -1;
    S32 i = 0;
    st_PACKER_LTOC_NODE* laynode = NULL;
    st_PACKER_ATOC_NODE* assnode = NULL;

    g_hiprf->readLongs(pkg, &ival, 1);
    laytyp = (en_LAYER_TYPE)ival;

    g_hiprf->readLongs(pkg, &refcnt, 1);
    xVALIDATE_L(4401, refcnt < PKR_MAX_ARRAY);

    laynode = PKR_newlaynode(laytyp, refcnt);
    XOrdAppend(&pr->laytoc, laynode);

    for (i = 0; i < refcnt; i++) {
        g_hiprf->readLongs(pkg, &ival, 1);
        xVALIDATE_L(4461, ival);

        idx = XOrdLookup(&pr->asstoc, (void*)ival, OrdTest_R_AssetID);
        xVALIDATE_L(4475, idx>-1);
        if (idx < 0) {
            xOutWarn("PACKER", "PKR: Yikes! Layer %d had bad AID 0x%08X\n", laynode->laytyp);
        }

        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
        xVALIDATE_L(4481, assnode);
        XOrdAppend(&laynode->assref, assnode);

        if (i != refcnt - 1) {
            laynode->laysize += assnode->d_size + assnode->d_pad;
        } else {
            laynode->laysize += assnode->d_size;
            DBprintf(DBML_VDBG, "PKR: LSize accum - skipped pad waste of %d\n", assnode->d_pad);
        }
    }

    if (laynode->laysize > 0) {
        DBprintf(DBML_VDBG, "PKR: Layer (type:%d) is size %d bytes; ", laynode->laytyp, laynode->laysize);
        laynode->laysize = (laynode->laysize + 2047) & ~2047;
        DBprintf(DBML_VDBG, "rounding to %d bytes\n", laynode->laysize);
    }

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'LDBG':
            LOD_r_LDBG(pkg, pr, laynode);
            break;
        default:
            xVALIDATE_FAIL_L(4522);
            xOutWarn("PACKER", "PKR: Unknown SubChunk ID 0x%08X in LHDR", cid);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    return result;
}

static S32 LOD_r_LDBG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr, st_PACKER_LTOC_NODE* laynode)
{
    S32 result = TRUE;
    S32 ival = 0;
    S32 amt = 0;

    if (pr->subver > 1) {
        amt = g_hiprf->readLongs(pkg, &ival, 1);
        xVALIDATE_L(4559, amt==1);
        xVALIDATE_L(4560, ival!=PKR_DUMBNUM);
        laynode->chksum = ival;
    }

    return result;
}

static S32 LOD_r_STRM(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'DHDR':
            LOD_r_DHDR(pkg, pr);
            break;
        case 'DPAK':
            LOD_r_DPAK(pkg, pr);
            break;
        default:
            xVALIDATE_FAIL_L(4601);
            xOutWarn("PACKER", "PKR: Unknown SubChunk ID 0x%08X in STRM", cid);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    return result;
}

static S32 LOD_r_DHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*)
{
    S32 result = TRUE;
    S32 ival = 0;

    g_hiprf->readLongs(pkg, &ival, 1);

    return result;
}

static S32 LOD_r_DPAK(st_HIPLOADDATA*, st_PACKER_READ_DATA*)
{
    S32 result = TRUE;

    return result;
}

static void PKR_spew_verhist()
{
    DBprintf(DBML_VDBG, "PKR: HIP file version update history\n");

    DBprintf(DBML_VDBG, "\n.. Version 1:\n- baseline\n");

    DBprintf(DBML_VDBG, "\n.. Version 2:\n");
    DBprintf(DBML_VDBG, ".. - Compatible version support information\n");
    DBprintf(DBML_VDBG, ".. - Asset data checksum\n");
    DBprintf(DBML_VDBG, ".. - Layer data checksum\n");
    DBprintf(DBML_VDBG, ".. - Creation/Modification date\n");

    DBprintf(DBML_VDBG, "\n.. Version 3:\n");
    DBprintf(DBML_VDBG, "\n.. - HIP Layers represent asset memory placement\n");
    DBprintf(DBML_VDBG, "\n.. - Layers are not compacted to reduce waste\n");
    DBprintf(DBML_VDBG, "\n.. - Loading is based on layers (was by asset)\n");
    DBprintf(DBML_VDBG, "\n.. - Added asynchronous loading support\n");
}

PKRAssetType* PKR_type2typeref(U32 type, PKRAssetType* typelist)
{
    PKRAssetType* da_type = NULL;
    PKRAssetType* tmptype = NULL;
    S32 safety = 0;

    tmptype = typelist;
    xVALIDATE_L(4726, tmptype);
    if (tmptype) {
        while (tmptype->typetag) {
            if (tmptype->typetag == type) {
                da_type = tmptype;
                break;
            }

            tmptype++;

            xVALIDATE_L(4737, safety++ < SAFELOOP_SMALL);
        }
    }

    xVALIDATE_L(4741, da_type);
    if (!da_type) {
        xOutErr("PACKER", "PKR: Unknown asset type '%s'\n", xUtil_idtag2string(type, 0));
        xFAIL_MSG_L(4744, "Unknown Asset Type detected");
    }

    return da_type;
}

static void PKR_bld_typecnt(st_PACKER_READ_DATA* pr)
{
    st_PACKER_LTOC_NODE* laynode = NULL;
    st_PACKER_ATOC_NODE* assnode = NULL;
    S32 i = 0;
    S32 j = 0;
    S32 idx = -1;
    S32 typcnt[PKR_MAX_TYPES+1] = {};
    st_XORDEREDARRAY* tmplist = NULL;
    U32 lasttype = 0;
    S32 lasttidx = 0;
    S32 hits = 0;
    S32 miss = 0;

    for (j = 0; j < pr->laytoc.cnt; j++) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[j];

#if DEBUG || RELEASE
        en_PKR_LAYER_LOAD_DEST loadDest = PKR_layerLoadDest(laynode->laytyp);
#endif

        for (i = 0; i < laynode->assref.cnt; i++) {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];

            if (assnode->loadflag & PKR_LDASS_ISNIL) {
                DBprintf(DBML_VDBG, "PKR: TYPE preproc - Skip empty asset\n");
                continue;
            }

#if DEBUG || RELEASE
            U32 memTag = PKR_lookupMemtag(laynode->laytyp);
            U32 assSize = (assnode->x_size > assnode->d_size) ? assnode->x_size : assnode->d_size;
            if (loadDest == PKR_LDDEST_KEEPSTATIC)
                xMemMgrPackerStaticTally(assSize, memTag);
            else if (loadDest == PKR_LDDEST_KEEPMALLOC)
                xMemMgrPackerMallocTally(assSize, memTag);
            else if (loadDest == PKR_LDDEST_RWHANDOFF)
                xMemMgrPackerApproxRWMallocTally(assSize, memTag);
#endif

            if (assnode->loadflag & PKR_LDASS_ISDUP) {
                DBprintf(DBML_VDBG, "PKR: TYPE preproc - Skip duplicate\n");
                continue;
            }

            if (lasttype && assnode->asstype == lasttype) {
                idx = lasttidx;
                hits++;
            } else {
                idx = PKR_typeHdlr_idx(pr, assnode->asstype);

                lasttype = assnode->asstype; lasttidx = idx;
                miss++;
            }

            xVALIDATE_L(4856, idx<PKR_MAX_TYPES);

            if (idx < 0) {
                xOutWarn("PACKER", "PKR: Unknown asset type '%s' for AID: 0x%08X (%s)\n",
                         xUtil_idtag2string(assnode->asstype, 0), assnode->aid, assnode->name);
                typcnt[PKR_MAX_TYPES]++;
            } else {
                typcnt[idx]++;
            }
        }
    }

    for (i = 0; i < PKR_MAX_TYPES+1; i++) {
        if (typcnt[i] < 1) {
            continue;
        }

        DBprintf(DBML_VDBG, "PKR: Asset Type List %3d had %6d entries\n", i, typcnt[i]);

        tmplist = &pr->typelist[i];
        XOrdInit(tmplist, (typcnt[i] > 1) ? typcnt[i] : 2, PKR_lookupMemtag((en_LAYER_TYPE)i), FALSE);
    }

    for (j = 0; j < pr->laytoc.cnt; j++) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[j];

        for (i = 0; i < laynode->assref.cnt; i++) {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];

            if (assnode->loadflag & PKR_LDASS_ISDUP) continue;
            if (assnode->loadflag & PKR_LDASS_ISNIL) {
                continue;
            }

            if (lasttype && assnode->asstype == lasttype) {
                idx = lasttidx;
                hits++;
            } else {
                idx = PKR_typeHdlr_idx(pr, assnode->asstype);

                lasttype = assnode->asstype; lasttidx = idx;
                miss++;
            }

            if (idx < 0) tmplist = &pr->typelist[PKR_MAX_TYPES];
            else
                tmplist = &pr->typelist[idx];

            XOrdAppend(tmplist, assnode);
        }
    }

#if DEBUG
    DBprintf(DBML_VDBG, "PKR: TYPE preproc H:%d M:%d\n", hits, miss);
#endif
}

static S32 PKR_typeHdlr_cnt(st_PACKER_READ_DATA* pr)
{
    S32 num_types = 0;
    const PKRAssetType* atype = NULL;

    atype = pr->types;
    xVALIDATE_L(4945, atype);
    while (atype->typetag) {
        num_types++;
        atype++;
    }

    return num_types;
}

static S32 PKR_typeHdlr_idx(st_PACKER_READ_DATA* pr, U32 type)
{
    S32 da_idx = -1;
    S32 cnt = 0;
    const PKRAssetType* atype = NULL;

    atype = pr->types;
    xVALIDATE_L(4961, atype);
    while (atype->typetag) {
        if (atype->typetag == type) {
            da_idx = cnt;
            break;
        }

        cnt++;
        atype++;
        xVALIDATE_L(4969, cnt<65000);
    }

    return da_idx;
}

void PKR_alloc_chkidx()
{
    xOutInfo("PACKER", "PKR: ... Memory nest index checkpoint ... %d\n", g_memalloc_pair);

    xOutInfo("PACKER", "PKR: ... Sum Mem Alloc'd: %8d Free'd: %8d (Est. Curr: %d)\n", g_memalloc_runtot, g_memalloc_runfree,
             g_memalloc_runtot - g_memalloc_runfree);
}

void* PKR_getmem(U32 id, S32 amount, U32 aid, S32 align, U32 memtag)
{
    return PKR_getmem(id, amount, aid, align, memtag, FALSE, NULL);
}

void* PKR_getmem(U32 id, S32 amount, U32 aid, S32 align, U32 memtag, S32 isTemp, char** memtru)
{
    void* memptr = NULL;

    xASSERT_L(5023, id);
    xASSERT_L(5024, aid);

    xASSERT_FMT_L(5026, (amount>0), "PKR: Zero byte allocation for %s\n", xUtil_idtag2string(id, 0));
    if (amount == 0) return memptr;

    xASSERT_L(5031, !isTemp||(isTemp&&memtru));

    xVALIDATE_L(5033, g_memalloc_pair>=0);
    if (g_memalloc_pair < 0) {
        xOutWarn("PACKER", "PKR: Alloc Pair Failure (%d) when doing 0x%08X\n", g_memalloc_pair, aid);
    }

    if (isTemp) {
        memptr = RwMallocL(amount + align, rwMEMHINTDUR_NADURATION, 5055);
        if (memtru) *memtru = (char*)memptr;
        if (align) memptr = (void*)(((S32)memptr + align - 1) & -align);
    } else {
        memptr = xMEMALLOC_L(amount, align, memtag, id, 5059);
    }

    xASSERT_L(5061, !(((S32)memptr) % align));

    xASSERT_L(5065, memptr);
    if (memptr) memset(memptr, 0, amount);

    g_memalloc_pair++; g_memalloc_runtot += amount;
    if (g_memalloc_runtot < 0) g_memalloc_runtot = amount;

    if (memptr) {
        xOutInfo("PACKER", "PKR: MID:%s AID:0x%08X grab at 0x%08X size %8d (paired #%3d)\n", xUtil_idtag2string(id, 0), aid,
                 memptr, amount, g_memalloc_pair);
    } else {
        xOutWarn("PACKER", "PKR: Allocation for %s size %d failed\n", xUtil_idtag2string(id, 0), amount);
    }

    return memptr;
}

void PKR_relmem(U32 id, S32 blksize, void* memptr, U32 aid, S32 isTemp)
{
    xVALIDATE_L(5100, memptr);
    xVALIDATE_L(5101, g_memalloc_pair>0);
    xASSERT_L(5102, memptr);
    xASSERT_L(5103, blksize>0);

    g_memalloc_pair--; g_memalloc_runfree += blksize;
    if (g_memalloc_runfree < 0) g_memalloc_runfree = blksize;

    xOutInfo("PACKER", "PKR: MID:%s AID:0x%08X free at 0x%08X size %8d (paired #%3d)\n", xUtil_idtag2string(id, 0), aid,
             memptr, blksize, g_memalloc_pair);

    if (memptr) {
        if (blksize > 0) {
#if DEBUG
            memset(memptr, 0x69, blksize);
#endif

            if (isTemp) RwFree(memptr);
            else
                xOutInfo("PACKER", "PKR: ... note - skipped dealloc (game) (MID: %s 0x%08X)\n", xUtil_idtag2string(id, 1),
                         memptr);
        }
    }
}

static S32 PKR_push_memmark()
{
    S32 curlvl = 0;

    curlvl = xMemPushBase();
    xOutInfo("PACKER", "PKR: Watermark push (now at %d)\n", curlvl + 1);
    return curlvl;
}

static S32 PKR_pop_memmark()
{
    S32 curlvl = 0;

    curlvl = xMemGetBase() - 1;
    curlvl = xMemPopBase(curlvl);
    xOutInfo("PACKER", "PKR: Watermark pop (now at %d)\n", curlvl);
    return curlvl;
}

static S32 PKR_pop_memmark(S32 destmark); // UNUSED
static S32 PKR_get_memmark(); // UNUSED

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("PKR: Watermark pop w/dest (current %d)\n", "PKR: Watermark - In Current Level %d\n")
#endif

#if DEBUG
static void PKR_r_rpt_stats(st_PACKER_READ_DATA* pr)
{
    DBprintf(DBML_USER, "\nPacker Library Information\n");
    DBprintf(DBML_USER, "---------------------------\n");

    DBprintf(DBML_USER, "Library Version        : %d\n", 1);

    DBprintf(DBML_USER, "\nGeneral Package Information\n");
    DBprintf(DBML_USER, "---------------------------\n");
    DBprintf(DBML_USER, "Package File    : %s\n", pr->packfile);
    DBprintf(DBML_USER, "Created         : %s", ctime(&pr->time_made));
    DBprintf(DBML_USER, "Modified        : %s", ctime(&pr->time_mod));
    DBprintf(DBML_USER, "Package Version : %s\n", xUtil_idtag2string(pr->pkgver, 0));
    DBprintf(DBML_USER, "Sub-Version     : %d\n", pr->subver);
    DBprintf(DBML_USER, "Compatible      : %d\n", pr->compatver);

    DBprintf(DBML_USER, "Client Version  : %d.%d (%d)\n", pr->cltver >> 16, pr->cltver & 0xFFFF, pr->cltver);
    DBprintf(DBML_USER, "# of Layers     : %d\n", pr->laycnt);
    DBprintf(DBML_USER, "# of Assets     : %d\n", pr->asscnt);
    DBprintf(DBML_USER, "Package Settings:\n");

    DBprintf(DBML_USER, "                   GameTime               %s\n", (pr->opts & 0x1) ? "Enabled" : "Disabled");

    DBprintf(DBML_USER, "                   Statistics Tracking    %s\n", (pr->opts & 0x2) ? "Enabled" : "Disabled");

    DBprintf(DBML_USER, "                   Extra Info             %s\n", (pr->opts & 0x8) ? "Available" : "Unavailable");

    DBprintf(DBML_USER, "                   Layered Sorting        %s\n", (pr->opts & 0x20) ? "Enabled" : "Disabled");
}

static void PKR_r_rpt_typeInfo(st_PACKER_READ_DATA* pr)
{
    const PKRAssetType* atype = NULL;

    atype = pr->types;
    xVALIDATE_L(5260, atype);

    DBprintf(DBML_USER, "\n\nSupported Package Types:\n");
    DBprintf(DBML_USER, "------------------------\n");
    if (atype) {
        while (atype->typetag) {
            DBprintf(DBML_USER, "\t%s::Flags%08LX\n\t{\n", xUtil_idtag2string(atype->typetag, 0), atype->tflags);
            DBprintf(DBML_USER, "\t\tDefault Align: %d\n", atype->typalign);

            DBprintf(DBML_USER, "\t\tCallbacks:\n");

            DBprintf(DBML_USER, "\t\t(Reading)\n");

            DBprintf(DBML_USER, "\t\t\t    ReadXForm: %s;\n", atype->readXForm ? "yes" : "no");

            DBprintf(DBML_USER, "\t\t\t  AssetLoaded: %s;\n", atype->assetLoaded ? "yes" : "no");

            DBprintf(DBML_USER, "\t\t\tAssetUnLoaded: %s;\n", atype->assetUnloaded ? "yes" : "no");
            DBprintf(DBML_USER, "\n");

            DBprintf(DBML_USER, "\t\t(Writing)\n");

            DBprintf(DBML_USER, "\t\t\t   WriteXForm: %s;\n", atype->writeXForm ? "yes" : "no");

            DBprintf(DBML_USER, "\t\t\t     MakeData: %s;\n", atype->makeData ? "yes" : "no");

            DBprintf(DBML_USER, "\t\t\t WriteCleanup: %s;\n", atype->cleanup ? "yes" : "no");

            DBprintf(DBML_USER, "\t\t\t    WritePeek: %s;\n", atype->writePeek ? "yes" : "no");

            DBprintf(DBML_USER, "\t}\n\n");

            atype++;
        }
    } else {
        DBprintf(DBML_USER, "(none);\n");
    }
}

static void PKR_r_rpt_typeTally(st_PACKER_READ_DATA* pr)
{
    S32* cnt_tally = NULL;
    S32* cnt_bytes = NULL;
    S32* cnt_padin = NULL;
    S32* cnt_load = NULL;
    S32* cnt_dump = NULL;
    S32* cnt_skip = NULL;

    char* tru_tally = NULL;
    char* tru_bytes = NULL;
    char* tru_padin = NULL;
    char* tru_load = NULL;
    char* tru_dump = NULL;
    char* tru_skip = NULL;

    S32 sum_tally = 0;
    S32 sum_bytes = 0;
    S32 sum_padin = 0;
    S32 sum_load = 0;
    S32 sum_dump = 0;
    S32 sum_skip = 0;

    const PKRAssetType* atype = NULL;
    S32 num_types = 0;
    S32 idx = -1;
    S32 i = 0;
    S32 j = 0;
    S32 alloc_size = 0;
    st_PACKER_ATOC_NODE* curass = NULL;
    st_PACKER_LTOC_NODE* curlay = NULL;
    en_PKR_LAYER_LOAD_DEST loaddest;

    num_types = PKR_typeHdlr_cnt(pr);

    DBprintf(DBML_USER, "\nType Totals\n");
    DBprintf(DBML_USER, "-----------\n");

    xVALIDATE_L(5353, num_types<PKR_MAX_TYPES);
    if (num_types == 0) {
        DBprintf(DBML_USER, "Skipping Type Totals summary - no known types\n");
        return;
    }

    DBprintf(DBML_USER, "Following types are defined using indicated byte totals\n\n");

    alloc_size = (num_types + 1) * sizeof(S32);
    cnt_tally = (S32*)PKR_getmem('PRPT', alloc_size, 'PRPT', 64, eMemMgrTag_Packer, TRUE, &tru_tally);

    cnt_bytes = (S32*)PKR_getmem('PRPT', alloc_size, 'PRPT', 64, eMemMgrTag_Packer, TRUE, &tru_bytes);

    cnt_padin = (S32*)PKR_getmem('PRPT', alloc_size, 'PRPT', 64, eMemMgrTag_Packer, TRUE, &tru_padin);

    cnt_load = (S32*)PKR_getmem('PRPT', alloc_size, 'PRPT', 64, eMemMgrTag_Packer, TRUE, &tru_load);

    cnt_dump = (S32*)PKR_getmem('PRPT', alloc_size, 'PRPT', 64, eMemMgrTag_Packer, TRUE, &tru_dump);

    cnt_skip = (S32*)PKR_getmem('PRPT', alloc_size, 'PRPT', 64, eMemMgrTag_Packer, TRUE, &tru_skip);

    DBprintf(DBML_USER, "processing\n");
    for (i = 0; i < pr->asstoc.cnt; i++) {
        curass = (st_PACKER_ATOC_NODE*)pr->asstoc.list[i];

        if (i && !(i % 2304)) DBprintf(DBML_USER, ".\n");
        else if (i && !(i % 32)) DBprintf(DBML_USER, ".");

        idx = PKR_typeHdlr_idx(pr, curass->asstype);

        if (idx < 0) {
            idx = num_types;
            DBprintf(DBML_USER, "%s(!unknown type!)", xUtil_idtag2string(curass->asstype, 0));
        }

        cnt_tally[idx]++;
        cnt_bytes[idx] += curass->d_size;
        cnt_padin[idx] += curass->d_pad;

        sum_tally++;
        sum_bytes += curass->d_size;
        sum_padin += curass->d_pad;
    }

    DBprintf(DBML_USER, "\n");

    for (j = 0; j < pr->laytoc.cnt; j++) {
        curlay = (st_PACKER_LTOC_NODE*)pr->laytoc.list[j];

        DBprintf(DBML_USER, "+");

        if (curlay->assref.cnt < 1) {
            continue;
        }

        for (i = 0; i < curlay->assref.cnt; i++) {
            curass = (st_PACKER_ATOC_NODE*)curlay->assref.list[i];

            idx = PKR_typeHdlr_idx(pr, curass->asstype);
            if (idx < 0) idx = num_types;

            loaddest = PKR_layerLoadDest(curlay->laytyp);
            switch (loaddest) {
            case PKR_LDDEST_SKIP:
                cnt_skip[idx] += curass->d_size + curass->d_pad;
                sum_skip += curass->d_size + curass->d_pad;
                break;
            case PKR_LDDEST_RWHANDOFF:
                cnt_dump[idx] += curass->d_size;
                sum_dump += curass->d_size;
                if (i != curlay->assref.cnt - 1) {
                    cnt_dump[idx] += curass->d_pad;
                    sum_dump += curass->d_pad;
                }
            case PKR_LDDEST_KEEPSTATIC:
            case PKR_LDDEST_KEEPMALLOC:
                cnt_load[idx] += curass->d_size;
                sum_load += curass->d_size;
                if (i != curlay->assref.cnt - 1) {
                    cnt_load[idx] += curass->d_pad;
                    sum_load += curass->d_pad;
                } else {
                    cnt_skip[idx] += curass->d_pad;
                    sum_skip += curass->d_pad;
                }
                break;
            default:
                xOutWarn("PACKER", "PKR: Unknown layer load option\n");

                cnt_load[idx] += curass->d_size;
                sum_load += curass->d_size;
                if (i != curlay->assref.cnt - 1) {
                    cnt_load[idx] += curass->d_pad;
                    sum_load += curass->d_pad;
                } else {
                    cnt_skip[idx] += curass->d_pad;
                    sum_skip += curass->d_pad;
                }
                break;
            }
        }
    }

    DBprintf(DBML_USER, "\n");

    atype = pr->types;
    j = 0;
    DBprintf(DBML_USER, "  %4s: %6s %10s %10s %10s %10s %10s\n", "Type", "Count", "#DataBytes", "#Pad(Raw)", "#Loaded",
             "#Dumped", "#Ignored");
    DBprintf(DBML_USER, "  %4s: %6s %10s %10s %10s %10s %10s\n", "----", "-----", "----------", "---------", "-------",
             "-------", "--------");
    while (atype->typetag) {
        DBprintf(DBML_USER, "  %4s: %6d %10d %10d %10d %10d %10d\n", xUtil_idtag2string(atype->typetag, 0), cnt_tally[j],
                 cnt_bytes[j], cnt_padin[j], cnt_load[j], cnt_dump[j], cnt_skip[j]);

        atype++; j++;
    }

    j = num_types;
    if (cnt_tally[j] || cnt_bytes[j] || cnt_padin[j]) {
        DBprintf(DBML_USER, "  %4s: %6d %10d %10d %10d %10d %10d\n", "?UNK", cnt_tally[j], cnt_bytes[j], cnt_padin[j],
                 cnt_load[j], cnt_dump[j], cnt_skip[j]);
        DBprintf(DBML_USER, "\t(UNK - Unrecongnized Asset Types)\n");
    }

    DBprintf(DBML_USER, "\n  %4s: %6d %10d %10d %10d %10d %10d\n", "Sums", sum_tally, sum_bytes, sum_padin, sum_load, sum_dump,
             sum_skip);

    PKR_relmem('PRPT', alloc_size, tru_tally, 'PRPT', TRUE);
    PKR_relmem('PRPT', alloc_size, tru_bytes, 'PRPT', TRUE);
    PKR_relmem('PRPT', alloc_size, tru_padin, 'PRPT', TRUE);
    PKR_relmem('PRPT', alloc_size, tru_load, 'PRPT', TRUE);
    PKR_relmem('PRPT', alloc_size, tru_dump, 'PRPT', TRUE);
    PKR_relmem('PRPT', alloc_size, tru_skip, 'PRPT', TRUE);

    cnt_tally = NULL; tru_tally = NULL;
    cnt_bytes = NULL; tru_bytes = NULL;
    cnt_padin = NULL; tru_padin = NULL;
    cnt_load = NULL; tru_load = NULL;
    cnt_dump = NULL; tru_dump = NULL;
    cnt_skip = NULL; tru_skip = NULL;
}

static void PKR_r_rpt_layers(st_PACKER_READ_DATA* pr)
{
    S32 i = 0;
    S32 j = 0;
    st_PACKER_LTOC_NODE* curlay = NULL;
    st_PACKER_ATOC_NODE* curass = NULL;
    S32 sum_assdata = 0;
    S32 sum_laysize = 0;
    S32 sum_padtot = 0;
    S32 sum_paduse = 0;
    S32 readfill = 0;

    DBprintf(DBML_USER, "\nLayer Information\n");
    DBprintf(DBML_USER, "-----------------\n");

    DBprintf(DBML_USER, "Following layers are defined (ordering is load order)\n\n");

    for (i = 0; i < pr->laytoc.cnt; i++) {
        curlay = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        xVALIDATE_L(5691, curlay);

        sum_assdata = 0, sum_laysize = 0, sum_padtot = 0, sum_paduse = 0;

        DBprintf(DBML_USER, "\nLayer #%d is Type %-4d (%s)\n", i + 1, curlay->laytyp, g_strz_layername[curlay->laytyp]);
        DBprintf(DBML_USER, "Referenced Assets:\n");

        for (j = 0; j < curlay->assref.cnt; j++) {
            curass = (st_PACKER_ATOC_NODE*)curlay->assref.list[j];
            xVALIDATE_L(5708, curass);

            DBprintf(DBML_USER, "\t0x%08X  (%s)  %-32s  (%12d)\n", curass->aid, xUtil_idtag2string(curass->asstype, 0),
                     curass->name, curass->d_size);

            sum_assdata += curass->d_size;
            sum_padtot += curass->d_pad;
            if (j != curlay->assref.cnt - 1) sum_paduse += curass->d_pad;
        }

        sum_laysize = sum_assdata + sum_paduse;
        readfill = ((sum_laysize + pkr_sector_size - 1) & -pkr_sector_size) - sum_laysize;

        DBprintf(DBML_USER, "\t------------------------------------\n");

        DBprintf(DBML_USER, "\tSize (base/forLoad)...: %12d / %d\n", sum_laysize, sum_laysize + readfill);

        DBprintf(DBML_USER, "\tSize Expected (TOC)...: %12d\n", curlay->laysize);

        DBprintf(DBML_USER, "\tAmt Asset Data........: %12d\n", sum_assdata);

        DBprintf(DBML_USER, "\tAmt Pad (total).......: %12d\n", sum_padtot);

        DBprintf(DBML_USER, "\tAmt Pad (used)........: %12d\n", sum_paduse);

        DBprintf(DBML_USER, "\tAmt Read Filler.......: %12d\n", readfill);
        DBprintf(DBML_USER, "\t====================================\n");
    }
}

static void PKR_r_rpt_assets(st_PACKER_READ_DATA* pr)
{
    S32 i = 0;
    st_PACKER_ATOC_NODE* curass = NULL;

    DBprintf(DBML_USER, "\nAsset Details\n");
    DBprintf(DBML_USER, "-------------\n");

    for (i = 0; i < pr->asstoc.cnt; i++) {
        curass = (st_PACKER_ATOC_NODE*)pr->asstoc.list[i];
        xVALIDATE_L(5759, curass);

        DBprintf(DBML_USER, "AID: 0x%08X  (%s)\n", curass->aid, curass->name);
        DBprintf(DBML_USER, "{\n");

        DBprintf(DBML_USER, "\t    Type:  %10s\n", xUtil_idtag2string(curass->asstype, 0));
        DBprintf(DBML_USER, "\t    Size:  %10d\n", curass->d_size);
        DBprintf(DBML_USER, "\t  Offset:  %10d\n", curass->d_off);
        DBprintf(DBML_USER, "\t    Plus:  %10d\n", curass->d_pad);
        DBprintf(DBML_USER, "\t   Check:  0x%08X\n", curass->d_chksum);

        DBprintf(DBML_USER, "\tFilename:  %s\n", curass->fullname[0] ? curass->fullname : "N/A");

        DBprintf(DBML_USER, "\t   Flags:  ");
        if (curass->infoflag == 0) {
            DBprintf(DBML_USER, "(none)\n");
        } else {
            if (curass->infoflag & PKR_INFO_SOURCE_FILE) {
                DBprintf(DBML_USER, "SOURCE_FILE; ");
            }

            if (curass->infoflag & PKR_INFO_SOURCE_VIRTUAL) {
                DBprintf(DBML_USER, "SOURCE_VIRTUAL; ");
            }

            if (curass->infoflag & PKR_INFO_WRITE_TRANSFORM) {
                DBprintf(DBML_USER, "WRITE_TRANSFORM; ");
            }

            if (curass->infoflag & PKR_INFO_READ_TRANSFORM) {
                DBprintf(DBML_USER, "READ_TRANSFORM; ");
            }

            DBprintf(DBML_USER, "\n");
        }

        DBprintf(DBML_USER, "}\n\n");
    }
}

static void PKR_r_rpt_kosher(st_PACKER_READ_DATA* pr)
{
    S32 i = 0;
    S32 j = 0;
    st_PACKER_ATOC_NODE* curass = NULL;
    st_PACKER_LTOC_NODE* curlay = NULL;
    S32 reqalign = 0;
    char* memptr = NULL;
    char* memtru = NULL;
    U32 newchk = 0;
    st_PACKER_ATOC_NODE* bogus;
    S32 rc;
    en_READ_ASYNC_STATUS readstat;

    DBprintf(DBML_USER, "\nLayer Alignment\n");
    DBprintf(DBML_USER, "---------------\n");

    DBprintf(DBML_USER, "%-6s %5s %12s %10s %12s\n", "Index", "Type", "SectorSize", "Aligned?", "HIPPosition");

    DBprintf(DBML_USER, "%-6s %5s %12s %10s %12s\n", "-----", "----", "----------", "--------", "-----------");

    for (i = 0; i < pr->laytoc.cnt; i++) {
        curlay = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        xVALIDATE_L(5833, curlay);

        for (j = 0; j < curlay->assref.cnt; j++) {
            curass = (st_PACKER_ATOC_NODE*)curlay->assref.list[j];
            xVALIDATE_L(5838, curass);
            if (curass->d_off > 0 && curass->d_size > 0) break;
            curass = NULL;
        }

        if (j) {
            DBprintf(DBML_USER, "                  ERROR                  \n");
            DBprintf(DBML_USER, "            ERROR ERROR ERROR            \n");
            DBprintf(DBML_USER, "      ERROR ERROR ERROR ERROR ERROR      \n");
            DBprintf(DBML_USER, "      ERROR ERROR ERROR ERROR ERROR      \n");
            DBprintf(DBML_USER, "ERROR ERROR ERROR ERROR ERROR ERROR ERROR\n");
            DBprintf(DBML_USER, "ERROR ERROR ERROR ERROR ERROR ERROR ERROR\n");
            DBprintf(DBML_USER, "                                         \n");
            DBprintf(DBML_USER, "      EEEEE RRRR  RRRR   OOO  RRRR       \n");
            DBprintf(DBML_USER, "      E     R   R R   R O   O R   R      \n");
            DBprintf(DBML_USER, "      EEE   RRRR  RRRR  O   O RRRR       \n");
            DBprintf(DBML_USER, "      E     R  R  R  R  O   O R  R       \n");
            DBprintf(DBML_USER, "      EEEEE R   R R   R  OOO  R   R      \n");
            DBprintf(DBML_USER, "                                         \n");
            DBprintf(DBML_USER, "ERROR ERROR ERROR ERROR ERROR ERROR ERROR\n");
            DBprintf(DBML_USER, "ERROR ERROR ERROR ERROR ERROR ERROR ERROR\n");
            DBprintf(DBML_USER, "\n\n\n\n\n");
            DBprintf(DBML_USER, "Layer %d almost nearly complete evil!\n", i);

            bogus = (st_PACKER_ATOC_NODE*)curlay->assref.list[0];

            DBprintf(DBML_USER, "Asset '%s' (AID:0x%08X Type:'%s') has size %d!\n", bogus->name, bogus->aid,
                     xUtil_idtag2string(bogus->asstype, 0), bogus->d_size);

            DBprintf(DBML_USER, "\n\n\n\n\n");
            DBprintf(DBML_USER, "ERROR ERROR ERROR ERROR ERROR ERROR ERROR\n");
            DBprintf(DBML_USER, "ERROR ERROR ERROR ERROR ERROR ERROR ERROR\n");
            DBprintf(DBML_USER, "                                         \n");
            DBprintf(DBML_USER, "      EEEEE RRRR  RRRR   OOO  RRRR       \n");
            DBprintf(DBML_USER, "      E     R   R R   R O   O R   R      \n");
            DBprintf(DBML_USER, "      EEE   RRRR  RRRR  O   O RRRR       \n");
            DBprintf(DBML_USER, "      E     R  R  R  R  O   O R  R       \n");
            DBprintf(DBML_USER, "      EEEEE R   R R   R  OOO  R   R      \n");
            DBprintf(DBML_USER, "                                         \n");
            DBprintf(DBML_USER, "ERROR ERROR ERROR ERROR ERROR ERROR ERROR\n");
            DBprintf(DBML_USER, "ERROR ERROR ERROR ERROR ERROR ERROR ERROR\n");
            DBprintf(DBML_USER, "      ERROR ERROR ERROR ERROR ERROR      \n");
            DBprintf(DBML_USER, "      ERROR ERROR ERROR ERROR ERROR      \n");
            DBprintf(DBML_USER, "            ERROR ERROR ERROR            \n");
            DBprintf(DBML_USER, "                  ERROR                  \n");
            iDebugBreak();
        }

        if (j) {
            return;
        }

        if (curlay->assref.cnt > 0) {
            DBprintf(DBML_USER, "%-6d %5d %12d %10s %12d\n", i, curlay->laytyp, pkr_sector_size,
                     (curass->d_off % pkr_sector_size == 0) ? "Yes" : "NO", curass->d_off);
        } else {
            DBprintf(DBML_USER, "%-6d %5d %12d %10s %12s\n", i, curlay->laytyp, pkr_sector_size, "N/A", "N/A");
        }
    }

    DBprintf(DBML_USER, "\nAsset Alignment\n");
    DBprintf(DBML_USER, "---------------\n");

    DBprintf(DBML_USER, "%-10s %-32s %12s %10s\n", "AssetID", "Name", "Requested", "Aligned?");

    DBprintf(DBML_USER, "%-10s %-32s %12s %10s\n", "-------", "----", "---------", "--------");

    for (i = 0; i < pr->asstoc.cnt; i++) {
        curass = (st_PACKER_ATOC_NODE*)pr->asstoc.list[i];
        xVALIDATE_L(5941, curass);

        reqalign = 2048;
        if (curass->assalign > 0) reqalign = curass->assalign;
        else if (curass->typeref) {
            if (curass->typeref->typalign > 0) {
                reqalign = curass->typeref->typalign;
            }
        }

        DBprintf(DBML_USER, "0x%08X %-32s %12d %10s\n", curass->aid, curass->name, reqalign,
                 (curass->d_off % reqalign == 0) ? "Yes" : "NO");
    }

    if (pr->subver < 2) {
        DBprintf(DBML_USER, "Checksum validation not available\n");
        return;
    }

    DBprintf(DBML_USER, "\nChecksum Validation\n");
    DBprintf(DBML_USER, "-------------------\n");

    DBprintf(DBML_USER, "%-10s  %-32s %-10s  %-10s  %-8s\n", "AssetID", "Name", "Packed", "UnPacked", "Status");

    DBprintf(DBML_USER, "%-10s  %-32s %-10s  %-10s  %-8s\n", "-------", "----", "------", "--------", "------");

    for (i = 0; i < pr->asstoc.cnt; i++) {
        curass = (st_PACKER_ATOC_NODE*)pr->asstoc.list[i];
        xVALIDATE_L(5979, curass);

        if (curass->d_size < 1) {
            continue;
        }

        memptr = (char*)PKR_getmem('PRPT', curass->d_size, curass->aid, 64, eMemMgrTag_Packer, TRUE, &memtru);

        xVALIDATE_L(5987, memptr);

        g_hiprf->setSpot(curass->ownpkg, curass->d_off);

        readstat = HIP_RDSTAT_SUCCESS;
        rc = g_hiprf->readBytes(curass->ownpkg, memptr, curass->d_size);

        xVALIDATE_L(6003, rc);

        rc = -1;
        while (rc < 0) {
            readstat = g_hiprf->pollRead(curass->ownpkg);
            switch (readstat) {
            case HIP_RDSTAT_SUCCESS:
            case HIP_RDSTAT_NOBYPASS:
            case HIP_RDSTAT_NOASYNC:
                rc = 1;
                break;
            case HIP_RDSTAT_NONE:
                xOutWarn("PACKER", "PKR:RPT: ASync op acting odd\n");
            case HIP_RDSTAT_FAILED:
                rc = 0;
                break;
            case HIP_RDSTAT_INPROG:
                break;
            default:
                xOutWarn("PACKER", "Unknown HIP async read status %d\n", readstat);
                break;
            }
        }
        xVALIDATE_L(6033, rc && (readstat == HIP_RDSTAT_SUCCESS));

        newchk = xUtil_crc_init();
        newchk = xUtil_crc_update(newchk, memptr, curass->d_size);

        PKR_relmem('PRPT', curass->d_size, memtru, curass->aid, TRUE);

        DBprintf(DBML_USER, "0x%08X  %-32s 0x%08X  0x%08X  %-8s\n", curass->aid, curass->name, curass->d_chksum, newchk,
                 (curass->d_chksum == newchk) ? "ok" : "FAILED!");
    }
}

static void PKR_r_rpt_timetrak(st_PACKER_READ_DATA* pr, S32 flg_opts)
{
    S32 i;
    st_PACKER_ATOC_NODE* curass;
    S32 idx;
    S32 cntout = 0;
    S32 condense = FALSE;
    S32 num_types = 0;
    F32 tym_tmp;
    F32 tym_full = 0.0f;
    F32 tym_xfm;
    F32 tym_justxform = 0.0f;
    S32 nice = TRUE;
    en_VERBOSE_MSGLEVEL mlvl = DBML_USER;

    xVALIDATE_L(6079, PKR_typeHdlr_cnt ( pr ) < PKR_MAX_TYPES);

    if (flg_opts & 0x10) {
        for (i = 0; i < PKR_MAX_TYPES+1; i++) {
            pr->dbg_typlodtym[i] = 0.0f;
            pr->dbg_typxformtym[i] = 0.0f;
        }
        pr->dbg_typlodtym_done = FALSE;
    }

    if (!pr->dbg_typlodtym_done) {
        for (i = 0; i < pr->asstoc.cnt; i++) {
            curass = (st_PACKER_ATOC_NODE*)pr->asstoc.list[i];
            xVALIDATE_L(6096, curass);

            if (!curass->dbg_loadtime[0]) {
                continue;
            }

            tym_xfm = iTimeDiffSec(curass->dbg_loadtime[1]);
            tym_tmp = iTimeDiffSec(curass->dbg_loadtime[0]);

            tym_tmp += tym_xfm;

            idx = PKR_typeHdlr_idx(pr, curass->asstype);

            if (idx < 0) {
                pr->dbg_typlodtym[PKR_MAX_TYPES] += tym_tmp;
                pr->dbg_typxformtym[PKR_MAX_TYPES] += tym_xfm;
            } else {
                pr->dbg_typlodtym[idx] += tym_tmp;
                pr->dbg_typxformtym[idx] += tym_xfm;
            }

            tym_full += tym_tmp;
            tym_justxform += tym_xfm;
        }

        pr->dbg_typlodtym[PKR_MAX_TYPES+1] = tym_full;
        pr->dbg_typxformtym[PKR_MAX_TYPES+1] = tym_justxform;

        pr->dbg_typlodtym_done = TRUE;
    }

    xVALIDATE_L(6136, pr->dbg_typlodtym_done);

    if (flg_opts & 0x1) {
        mlvl = DBML_DISP;
        nice = FALSE;
        if (flg_opts & 0x2) {
            xOutWarn("PACKER", "PKR: LoadTime - show asset == LOTS!\n");
        }
    }

    if (nice) {
        DBprintf(mlvl, "\n");
        DBprintf(mlvl, "HIP Load Statistics\n");
        DBprintf(mlvl, "-------------------\n");
        DBprintf(mlvl, "Package: %s\n", pr->packfile);
    } else {
        if (pr->packfile[2] != '\\') DBprintf(mlvl, "%s - ", pr->packfile);
        else DBprintf(mlvl, "%s - ", &pr->packfile[3]);
    }

    if (nice) {
        DBprintf(mlvl, "\n");

        DBprintf(mlvl, "Overall Read time: %.4f seconds\n", iTimeDiffSec(pr->dbg_readtime[2]));

        DBprintf(mlvl, "AssetByType time: %.4f seconds\n", iTimeDiffSec(pr->dbg_readtime[3]));

        DBprintf(mlvl, "Load time: %.4f seconds (includes %.4f in xform)\n", pr->dbg_typlodtym[PKR_MAX_TYPES+1],
                 pr->dbg_typxformtym[PKR_MAX_TYPES+1]);
    } else {
        DBprintf(mlvl, "Load time: %.4f seconds\n", pr->dbg_typlodtym[PKR_MAX_TYPES+1]);
    }

    if (flg_opts & 0x2) {
        if (nice) DBprintf(mlvl, "Asset Load Times:\n");
        for (i = 0; i < pr->asstoc.cnt; i++) {
            if (!(i % 100)) xDebugVSync();

            curass = (st_PACKER_ATOC_NODE*)pr->asstoc.list[i];
            xVALIDATE_L(6191, curass);
            if (!nice) DBprintf(mlvl, "####");
            else DBprintf(mlvl, "\t");

            DBprintf(mlvl, "0x%08X %-32s I/O: %.4f XFrm: %.4f sec\n", curass->aid, curass->name,
                     iTimeDiffSec(curass->dbg_loadtime[0]), iTimeDiffSec(curass->dbg_loadtime[1]));
        }
    }

    condense = (flg_opts & 0x8) ? TRUE : FALSE;
    if (flg_opts & 0x4) {
        if (nice) {
            DBprintf(mlvl, "Load Times By Type:\n");
            DBprintf(mlvl, "() indicates portion spent in transform\n");
        }

        num_types = PKR_typeHdlr_cnt(pr);
        for (i = 0; i < num_types; i++) {
            if (!(i % 100)) xDebugVSync();

            if (!condense || pr->dbg_typlodtym[i] > 0.0f) {
                if (!(cntout % 3)) DBprintf(mlvl, "%s", nice ? "\t" : "#####");

                if (nice) {
                    DBprintf(mlvl, "%4s %7.3f (%.3f)  || ", xUtil_idtag2string(pr->types[i].typetag, 0),
                             pr->dbg_typlodtym[i], pr->dbg_typxformtym[i]);
                } else {
                    DBprintf(mlvl, "%4s %7.3f : ", xUtil_idtag2string(pr->types[i].typetag, 0), pr->dbg_typlodtym[i]);
                }

                cntout++;
                if (!(cntout % 3)) DBprintf(mlvl, "\n");
            }
        }

        if (cntout % 3) DBprintf(mlvl, "\n");
    }

    if (pr->dbg_typlodtym[PKR_MAX_TYPES] > 0.0f || pr->dbg_typxformtym[PKR_MAX_TYPES] > 0.0f) {
        xOutWarn("PACKER", "PKR: TimeTally Bucket: %.4f (includes %.4f in transform)\n", pr->dbg_typlodtym[PKR_MAX_TYPES],
                 pr->dbg_typxformtym[PKR_MAX_TYPES]);
    }

    if (nice) DBprintf(mlvl, "-------------------\n");
}
#endif

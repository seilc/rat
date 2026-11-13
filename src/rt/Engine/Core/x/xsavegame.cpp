#include "xCommon.h"
#include "xsavegame.h"

#include "xsavegamepv.h"
#include "xDebug.h"
#include "xMemMgr.h"
#include "xutil.h"

#include "decomp.h"

#include <stdio.h>
#include <string.h>

#define XSG_STG_INIT (1<<0)
#define XSG_STG_LOADCLT (1<<1)
#define XSG_STG_SAVECLT (1<<2)
#define XSG_STG_SETUP (1<<3)
#define XSG_STG_PROCESS (1<<4)
#define XSG_STG_ABORT (1<<30)

#define XSG_MAX_LEADERS 3

#define XSG_SAVETAG IDTAG('G','D','A','T')
#define XSG_SAVE_VERSION 1
#define XSG_DUMMY_VAL 0x33333333
#define LEADER_TAG IDTAG('L','E','D','R')

#define ISODD(x) ((x)&1)

#define XSG_NOOP

#if DEBUG || RELEASE
#define XSG_PRINT DBprintf
#else
#define XSG_PRINT
#endif

struct st_XSAVEGAME_LEADER
{
    char gameLabel[XSG_MAXLEN_LABEL];
    S32 progress;
    S64 gametime;
    char thumbIconIdx;
};

static S32 g_xsginit;
static st_XSAVEGAME_DATA g_xsgdata = { 0 };
static st_XSAVEGAME_LEADER g_leaders[XSG_MAX_LEADERS] = { 0 };

static S32 xSG_cb_leader_svinfo(void* cltdata, st_XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame);
static S32 xSG_cb_leader_svproc(void* cltdata, st_XSAVEGAME_DATA* original_xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt);
static S32 xSG_cb_leader_load(void* cltdata, st_XSAVEGAME_DATA* original_xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, U32, S32);
static S32 xSG_grab_leaders(st_XSAVEGAME_DATA* xsgdata);
static void xSG_areaComposeLabel(char* strbuf, S32, char*, S32);
static void ASG_ISG_changed(void* cltdata, en_CHGCODE what);

S32 xSGStartup()
{
    if (g_xsginit++ == 0) {
        XSG_PRINT(DBML_VDBG, "xSG: Starting Save Game Instance Module\n");
        xUtilStartup();
        iSGStartup();
        memset(&g_xsgdata, 0, sizeof(st_XSAVEGAME_DATA));
    }

    xSGAutoSave_Startup();

    return g_xsginit;
}

S32 xSGShutdown()
{
    if (--g_xsginit == 0) {
        iSGShutdown();
        xUtilShutdown();
        XSG_PRINT(DBML_VDBG, "xSG: Save Game Module Shut Down\n");
    }

    xASSERT_L(124, g_xsginit>=0);

    return g_xsginit;
}

st_XSAVEGAME_DATA* xSGInit(en_SAVEGAME_MODE mode)
{
    st_XSAVEGAME_DATA* xsgdata = &g_xsgdata;

    xASSERT_L(142, xsgdata->stage==0);
    xASSERT_L(143, xsgdata->stkcnt==0);

    if (xsgdata->stage != 0) {
        XSG_PRINT(DBML_WARN, "xSGI: Unable to reset SaveGame data.\n\tPerhaps xSGDone was not called?\n");
        return NULL;
    }

    xsgdata->mode = mode;
    xsgdata->chksum = xUtil_crc_init();
    xsgdata->upd_tally = TRUE;
    xsgdata->file_chksum = 0;
    xsgdata->read_chksum = xUtil_crc_init();
    xsgdata->stage |= XSG_STG_INIT;
    XSG_PRINT(DBML_VDBG, "xSGI: Initialized save instance\n");

    xsgdata->isgsess = iSGSessionBegin(xsgdata, xSG_cb_ISGChange, 0);
    xASSERT_L(172, xsgdata->isgsess);

    if (mode == XSG_MODE_LOAD) {
        xSGAddLoadClient(xsgdata, LEADER_TAG, xsgdata, xSG_cb_leader_load);
    } else {
        xASSERT_L(181, mode==XSG_MODE_SAVE);
        xSGAddSaveClient(xsgdata, LEADER_TAG, xsgdata, xSG_cb_leader_svinfo, xSG_cb_leader_svproc);
    }

    memset(g_leaders, 0, sizeof(g_leaders));
    return xsgdata;
}

S32 xSGDone(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;

    xASSERT_L(196, xsgdata);
    if (!(xsgdata->stage & XSG_STG_INIT)) {
        XSG_PRINT(DBML_WARN, "xSGD: for un-initialized instance\n");
        result = FALSE;
    }

    if (xsgdata->stage & XSG_STG_ABORT) {
        XSG_PRINT(DBML_VDBG, "xSGD: Save Aborted - releasing instance\n");
    }

    XSG_NOOP("EndPoint", 0);

    if (xsgdata->isgsess) {
        iSGSessionEnd(xsgdata->isgsess);
    }

    if (xsgdata->mode == XSG_MODE_SAVE) {
        if (xsgdata->membuf) {
            XSG_PRINT(DBML_VDBG, "Popping temp alloc for %s sized %d\n", xUtil_idtag2string('XSGS', 0), xsgdata->memsize);
            xMEMPOPTEMP(xsgdata->membuf);
            xsgdata->membuf = NULL;
            xsgdata->memsize = 0;
            xsgdata->buf_curpos = NULL;
        }
    }
    else if (xsgdata->mode == XSG_MODE_LOAD) {
        if (xsgdata->membuf) {
            XSG_PRINT(DBML_VDBG, "Popping temp alloc for %s sized %d\n", xUtil_idtag2string('XSGL', 0), xsgdata->loadsize);
            xMEMPOPTEMP(xsgdata->loadbuf);
            xsgdata->loadbuf = NULL;
            xsgdata->loadsize = 0;
            xsgdata->walkpos = NULL;
            xsgdata->walkremain = 0;
        }

        if (xsgdata->loadbuf) {
            xMEMPOPTEMP(xsgdata->loadbuf);
            xsgdata->loadbuf = NULL;
            xsgdata->walkpos = NULL;
            xsgdata->walkremain = 0;
        }
    } else {
        XSG_PRINT(DBML_WARN, "Unknown load/save mode %d\n", xsgdata->mode);
    }

    memset(&g_xsgdata, 0, sizeof(st_XSAVEGAME_DATA));
    memset(g_leaders, 0, sizeof(g_leaders));

    return result;
}

S32 xSGTgtCount(st_XSAVEGAME_DATA* xsgdata, S32* max)
{
    S32 cnt = 1;
    cnt = iSGTgtCount(xsgdata->isgsess, max);
    return cnt;
}

S32 xSGTgtPhysSlotIdx(st_XSAVEGAME_DATA* xsgdata, S32 tidx)
{
    S32 result = iSGTgtPhysSlotIdx(xsgdata->isgsess, tidx);
    xASSERT_L(293, result != -1);
    return result;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("XSG: MemCard has %d free blocks (%d basis)\n");
#endif

S32 xSGTgtIsFormat(st_XSAVEGAME_DATA* xsgdata, S32 tidx, S32* badEncode)
{
    S32 result = FALSE;
    S32 rc = TRUE;

    rc = iSGTgtState(xsgdata->isgsess, tidx, NULL);

    if (rc & 0x4000000) {
        if (badEncode)
            *badEncode = TRUE;
    }
    else if (rc & 0x2) {
        result = TRUE;
    }

    if (rc & 0x1000000) {
        return -1;
    }
    return result;
}

S32 xSGTgtFormatTgt(st_XSAVEGAME_DATA* xsgdata, S32 tidx, S32* canRecover)
{
    S32 result = TRUE;
    result = iSGTgtFormat(xsgdata->isgsess, tidx, 0, canRecover);
    return result;
}

S32 xSGTgtSelect(st_XSAVEGAME_DATA* xsgdata, S32 tidx)
{
    S32 result = TRUE;
    S32 rc = FALSE;

    result = iSGTgtSetActive(xsgdata->isgsess, tidx);
    if (result) {
        rc = xSG_chdir_gamedir(xsgdata);
        if (rc) xSG_grab_leaders(xsgdata);
    }

    return result;
}

S32 xSGTgtHasGameDir(st_XSAVEGAME_DATA* xsgdata, S32 tidx)
{
    S32 rc = FALSE;
    rc = iSGTgtSetActive(xsgdata->isgsess, tidx);
    if (rc) {
        rc = xSG_chdir_gamedir(xsgdata);
    }
    return rc;
}

S32 xSGTgtHaveRoom(st_XSAVEGAME_DATA* xsgdata, S32 tidx, S32 fsize, S32 slotidx, S32* bytesNeeded, S32* availOnDisk,
                   S32* needFile)
{
    S32 isroom = FALSE;
    char fname[256] = { 0 };

    if (slotidx < 0) {
        isroom = iSGTgtHaveRoom(xsgdata->isgsess, tidx, fsize, NULL, NULL, bytesNeeded, availOnDisk, needFile);
    } else {
        strcpy(fname, iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, slotidx));
        isroom = iSGTgtHaveRoom(xsgdata->isgsess, tidx, fsize, NULL, fname, bytesNeeded, availOnDisk, needFile);
    }

    return isroom;
}

bool xSGCheckMemoryCard(st_XSAVEGAME_DATA* xsgdata, S32 index)
{
    return iSGCheckMemoryCard(xsgdata->isgsess, index);
}

void xSGGameSet(st_XSAVEGAME_DATA* xsgdata, S32 gidx)
{
    xsgdata->gslot = gidx;
}

S32 xSGGameIsEmpty(st_XSAVEGAME_DATA* xsgdata, S32 gidx)
{
    S32 exists = xSGGameSize(xsgdata, gidx);
    return exists < 0;
}

S32 xSGGameSize(st_XSAVEGAME_DATA* xsgdata, S32 gidx)
{
    S32 size = -1;
    const char* da_name = NULL;

    if (xSG_chdir_gamedir(xsgdata)) {
        da_name = xSG_cm_slotname(xsgdata, gidx);
        size = iSGFileSize(xsgdata->isgsess, da_name);
    }

    return size;
}

char* xSGGameModDate(st_XSAVEGAME_DATA* xsgdata, S32 gidx)
{
    static char da_date[32] = { 0 };
    const char* da_name = NULL;

    da_date[0] = '\0';

    if (xSG_chdir_gamedir(xsgdata)) {
        da_name = xSG_cm_slotname(xsgdata, gidx);
        const char* date = iSGFileModDate(xsgdata->isgsess, da_name);

        if (date) {
            strcpy(da_date, date);
        } else {
            strcpy(da_date, "");
        }
    }

    return da_date;
}

char* xSGGameLabel(st_XSAVEGAME_DATA* xsgdata, S32 gidx)
{
    const char* da_name = NULL;
    st_XSAVEGAME_LEADER* lead = NULL;

    if (xSG_chdir_gamedir(xsgdata)) {
        lead = &g_leaders[gidx];
        if (lead) da_name = lead->gameLabel;
    }

    return (char*)da_name;
}

S32 xSGGameThumbIndex(st_XSAVEGAME_DATA* xsgdata, S32 gidx)
{
    S32 idx_thum = -1;
    st_XSAVEGAME_LEADER* lead = NULL;

    if (xSG_chdir_gamedir(xsgdata)) {
        lead = &g_leaders[gidx];
        idx_thum = lead->thumbIconIdx;
    }

    return idx_thum;
}

S32 xSGGameProgress(st_XSAVEGAME_DATA* xsgdata, S32 gidx)
{
    S32 idx_thum = -1;
    st_XSAVEGAME_LEADER* lead = NULL;

    if (xSG_chdir_gamedir(xsgdata)) {
        lead = &g_leaders[gidx];
        idx_thum = lead->progress;
    }

    return idx_thum;
}

S32 xSGAddSaveClient(st_XSAVEGAME_DATA* xsgdata, U32 clttag, void* cltdata, XSGCltInfoFunc infofunc,
                     XSGCltProcFunc procfunc)
{
    S32 result = TRUE;
    S32 is_dup = FALSE;
    S32 i = 0;
    st_XSAVEGAME_CLIENT* clt = NULL;

    xASSERT_L(674, clttag);
    xASSERT_L(675, xsgdata);
    if (xsgdata->stage & XSG_STG_ABORT) result = FALSE;
    else if (!(xsgdata->stage & XSG_STG_INIT)) {
        XSG_PRINT(DBML_WARN, "xSGASC: un-initialized instance\n");
        result = FALSE;
    }

    xASSERT_L(682, xsgdata->mode == XSG_MODE_SAVE);
    if (xsgdata->mode != XSG_MODE_SAVE) {
        XSG_PRINT(DBML_WARN, "Adding save client to a load context\n");
    }

    xASSERT_L(688, xsgdata->stkcnt+1 < XSG_MAX_CLIENT);

    if (xsgdata->stkcnt + 1 >= XSG_MAX_CLIENT) {
        XSG_PRINT(DBML_WARN, "xSGASC: Unable to register client %s - max limit exceeded\n", xUtil_idtag2string(clttag, 0));
        result = FALSE;
    }

#if DEBUG
    if (result) {
        for (i = 0; i < xsgdata->stkcnt; i++) {
            clt = &xsgdata->cltstk[i];

            if (clt->idtag == clttag) {
                result = FALSE;
                XSG_PRINT(DBML_WARN, "xSGASC: Client %s is duplicate\n", xUtil_idtag2string(clttag, 0));
                break;
            }
        }
    }
#endif

    clt = &xsgdata->cltstk[xsgdata->stkcnt++];
    clt->idtag = clttag;
    clt->cltinfo = infofunc;
    clt->cltproc = procfunc;
    clt->cltdata = cltdata;
    clt->needamt = 0;
    clt->realamt = 0;
    clt->buf_sizepos = NULL;

    xsgdata->stage |= XSG_STG_SAVECLT;
    if (!result) xsgdata->stage |= XSG_STG_ABORT;
    return result;
}

S32 xSGAddLoadClient(st_XSAVEGAME_DATA* xsgdata, U32 clttag, void* cltdata, XSGCltLoadFunc loadfunc)
{
    S32 result = TRUE;
    S32 is_dup = FALSE;
    S32 i = 0;
    st_XSAVEGAME_CLIENT* clt = NULL;

    xASSERT_L(756, xsgdata);
    if (xsgdata->stage & XSG_STG_ABORT) result = FALSE;
    else if (!(xsgdata->stage & XSG_STG_INIT)) {
        XSG_PRINT(DBML_WARN, "ASALF: un-initialized instance\n");
        result = FALSE;
    }
    else if (xsgdata->stkcnt + 1 >= XSG_MAX_CLIENT) {
        XSG_PRINT(DBML_WARN, "xSG:ALC: Unable to register client %s - max limit exceeded\n", xUtil_idtag2string(clttag, 0));
        result = FALSE;
    }

    xASSERT_L(769, xsgdata->mode == XSG_MODE_LOAD);
    if (xsgdata->mode != XSG_MODE_LOAD) {
        XSG_PRINT(DBML_WARN, "Adding load client to a save context\n");
    }

#if DEBUG
    if (result) {
        for (i = 0; i < xsgdata->stkcnt; i++) {
            clt = &xsgdata->cltstk[i];

            if (clt->idtag == clttag) {
                result = FALSE;
                XSG_PRINT(DBML_WARN, "xSG:ALC: Load Client %s is duplicate\n", xUtil_idtag2string(clttag, 0));
                break;
            }
        }
    }
#endif

    if (clttag == 0 && xsgdata->dfltloadclt.idtag) {
        XSG_PRINT(DBML_WARN, "xSG: Default loader client already assigned at 0x%08X, incoming client from 0x%08X ignored\n",
                  xsgdata->dfltloadclt.cltload, loadfunc);
    }
    else if (clttag == 0) {
        XSG_PRINT(DBML_DBG, "xSG: Adding dflt load client (id:0) from 0x%08X\n", loadfunc);
        xsgdata->dfltloadclt.idtag = 1;
        xsgdata->dfltloadclt.cltload = loadfunc;
        xsgdata->dfltloadclt.cltdata = cltdata;
        xsgdata->dfltloadclt.blokact = 0;
        xsgdata->dfltloadclt.blokmax = 0;
        xsgdata->dfltloadclt.blokpos = NULL;
        xsgdata->dfltloadclt.readpos = NULL;
        xsgdata->dfltloadclt.readremain = 0;
        xsgdata->dfltloadclt.readamt = 0;
    } else {
        clt = &xsgdata->cltstk[xsgdata->stkcnt++];
        clt->idtag = clttag;
        clt->cltload = loadfunc;
        clt->cltdata = cltdata;
        clt->blokact = 0;
        clt->blokmax = 0;
        clt->blokpos = NULL;
        clt->readpos = NULL;
        clt->readremain = 0;
        clt->readamt = 0;
    }

    xsgdata->stage |= XSG_STG_LOADCLT;
    if (!result) xsgdata->stage |= XSG_STG_ABORT;
    return result;
}

S32 xSGSetup(st_XSAVEGAME_DATA* xsgdata)
{
    xASSERT_L(859, xsgdata->mode == XSG_MODE_LOAD);
    return xSGSetup(xsgdata, 0, "nothing", -1, 0, 0);
}

S32 xSGSetup(st_XSAVEGAME_DATA* xsgdata, S32 gidx, char* label, S32 progress, S64 playtime, S32 thumbIconIdx)
{
    S32 result = TRUE;
    S32 rc = FALSE;

    xASSERT_L(869, xsgdata);
    xASSERT_L(870, xsgdata->stage & XSG_STG_INIT);
    if (xsgdata->stage & XSG_STG_ABORT) result = FALSE;
    else if (!(xsgdata->stage & XSG_STG_INIT)) {
        XSG_PRINT(DBML_WARN, "xSGS: No INIT???\n");
        result = FALSE;
    }

    if (xsgdata->stkcnt < 1) XSG_PRINT(DBML_WARN, "xSGS: No clients?\n");

    if (xsgdata->mode == XSG_MODE_SAVE) {
        xsgdata->gslot = gidx;

        xASSERT_L(885, label);
        xASSERT_L(886, label[0]);
        xASSERT_L(887, strlen(label)<XSG_MAXLEN_LABEL);
        strncpy(xsgdata->label, label, XSG_MAXLEN_LABEL - 1);
        xsgdata->label[XSG_MAXLEN_LABEL - 1] = '\0';
        xsgdata->playtime = playtime;
        xsgdata->progress = progress;
        xsgdata->thumbIconIdx = thumbIconIdx;

        if (result) {
            rc = xSG_sv_flipinfo(xsgdata);
            if (!rc) result = FALSE;
        }

        if (result) {
            rc = xSG_sv_prepdest(xsgdata);
            if (!rc) result = FALSE;
        }
    } else {
        xASSERT_L(908, xsgdata->mode == XSG_MODE_LOAD);

        rc = xSG_ld_prepload(xsgdata);
        if (!rc) result = FALSE;
    }

    xsgdata->stage |= XSG_STG_SETUP;
    if (!result) xsgdata->stage |= XSG_STG_ABORT;
    return result;
}

S32 xSGProcess(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 rc = TRUE;

    xASSERT_L(935, xsgdata);

    if (xsgdata->stage & XSG_STG_ABORT) result = FALSE;
    else if (!(xsgdata->stage & XSG_STG_INIT)) {
        XSG_PRINT(DBML_WARN, "xSGP: un-initialized instance\n");
        result = FALSE;
    }
    else if (!(xsgdata->stage & XSG_STG_SETUP)) {
        XSG_PRINT(DBML_WARN, "xSGP: xSGSetup not called - aborting save\n");
        result = FALSE;
    }

    if (result && xsgdata->mode == XSG_MODE_SAVE) {
        rc = xSG_smem_blkopen(xsgdata);
        if (!rc) result = FALSE;

        if (result) {
            rc = xSG_sv_flipproc(xsgdata);
            if (!rc) result = FALSE;
        }

        XSG_NOOP("MidPoint *clients processed* ", 0);

        if (result) {
            rc = xSG_sv_bldchksum(xsgdata);
            if (!rc) result = FALSE;
        }

        if (result) {
            rc = xSG_smem_blkclose(xsgdata);
            if (!rc) result = FALSE;
        }

        if (result) {
            xSG_sv_commit(xsgdata);
        }
    }
    else if (result && xsgdata->mode == XSG_MODE_LOAD) {
        rc = xSG_ld_readgame(xsgdata);
        if (!rc) result = FALSE;
    }

    xsgdata->stage |= XSG_STG_PROCESS;
    if (!result) xsgdata->stage |= XSG_STG_ABORT;
    return result;
}

S32 xSGWrapup(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 rc = FALSE;

    if (xsgdata->stage & XSG_STG_ABORT) result = FALSE;
    else if (!(xsgdata->stage & XSG_STG_INIT)) {
        XSG_PRINT(DBML_WARN, "xSGW: un-initialized instance\n");
        result = FALSE;
    }
    else if (!(xsgdata->stage & XSG_STG_PROCESS)) {
        xsgdata->stage |= XSG_STG_ABORT;
        XSG_PRINT(DBML_WARN, "xSGW: xSGProcess not called - aborting\n");
        result = FALSE;
    }

    if (xsgdata->mode != XSG_MODE_LOAD) return result;
    if (!result) return result;

    XSG_PRINT(DBML_TEST, "Processing GameData Load\n");

    if (result) {
        rc = xSG_ld_readhead(xsgdata);
    }

    if (rc < 0) {
        xsgdata->stage |= XSG_STG_ABORT;
        return rc;
    }

    if (rc == 0) {
        result = FALSE;
    }

    if (result) {
        rc = xSG_ld_validate(xsgdata);
    }

    if (rc < 0) {
        xsgdata->stage |= XSG_STG_ABORT;
        return rc;
    }

    if (rc == 0) {
        result = FALSE;
    }

    if (result) rc = xSG_ld_findcltblk(xsgdata);
    if (!rc) result = FALSE;

    if (result) rc = xSG_ld_flipload(xsgdata);
    if (!rc) result = FALSE;

    if (!result) xsgdata->stage |= XSG_STG_ABORT;
    return result;
}

en_XSGASYNC_STATUS xSGAsyncStatus(st_XSAVEGAME_DATA* xsgdata, S32 block, en_XSG_WHYFAIL* whyFail, char* errmsg)
{
    en_XSGASYNC_STATUS xstat = XSG_ASTAT_NOOP;
    en_ASYNC_OPSTAT istat = ISG_OPSTAT_INPROG;
    en_ASYNC_OPERR whyerr = ISG_OPERR_NONE;
    S32 rc = FALSE;

    if (errmsg) errmsg[0] = '\0';

    istat = iSGPollStatus(xsgdata->isgsess, NULL, block);

    switch (istat) {
    case ISG_OPSTAT_FAILURE:
        xstat = XSG_ASTAT_FAILED;
        whyerr = iSGOpError(xsgdata->isgsess, errmsg);
        break;
    case ISG_OPSTAT_INPROG:
        xstat = XSG_ASTAT_INPROG;
        break;
    case ISG_OPSTAT_SUCCESS:
        xstat = XSG_ASTAT_SUCCESS;
        break;
    default:
        XSG_PRINT(DBML_WARN, "Unknown async operation status %d\n", istat);
        break;
    }

    if (whyFail && whyerr) {
        en_XSG_WHYFAIL con = XSG_WHYERR_NONE;
        switch (whyerr) {
        case ISG_OPERR_NONE:
            break;
        case ISG_OPERR_NOCARD:
            con = XSG_WHYERR_NOCARD;
            break;
        case ISG_OPERR_NOROOM:
        case ISG_OPERR_SVNOSPACE:
            con = XSG_WHYERR_NOROOM;
            break;
        case ISG_OPERR_DAMAGE:
            con = XSG_WHYERR_DAMAGE;
            break;
        case ISG_OPERR_SVWRITE:
            con = XSG_WHYERR_CARDYANKED;
            break;
        default:
            con = XSG_WHYERR_OTHER;
            break;
        }

        *whyFail = con;
    }

    return xstat;
}

static S32 xSG_cb_leader_svinfo(void* cltdata, st_XSAVEGAME_DATA*, S32* cur_space, S32* max_fullgame)
{
    st_XSAVEGAME_DATA* xsg = (st_XSAVEGAME_DATA*)cltdata;
    xASSERT_L(1164, xsg);

    *cur_space = 0x100;
    *max_fullgame = 0x100;

    return TRUE;
}

static S32 xSG_cb_leader_svproc(void* cltdata, st_XSAVEGAME_DATA* original_xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt)
{
    st_XSAVEGAME_DATA* xsg = (st_XSAVEGAME_DATA*)cltdata;
    xASSERT_L(1179, xsg);

    st_XSAVEGAME_LEADER leader = { 0 };
    char fundata[] = "--TakeMeToYourLeader--";

    strncpy(leader.gameLabel, xsg->label, sizeof(leader.gameLabel));
    leader.gameLabel[sizeof(leader.gameLabel) - 1] = '\0';
    leader.gametime = xsg->playtime;
    leader.progress = xsg->progress;
    leader.thumbIconIdx = xsg->thumbIconIdx;

    xSGWriteData(original_xsgdata, wctxt, (char*)&leader, sizeof(st_XSAVEGAME_LEADER));
    xSGWriteData(original_xsgdata, wctxt, fundata, 22);

    return TRUE;
}

static S32 xSG_cb_leader_load(void* cltdata, st_XSAVEGAME_DATA* original_xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, U32, S32)
{
    st_XSAVEGAME_DATA* xsg = (st_XSAVEGAME_DATA*)cltdata;
    xASSERT_L(1211, xsg);

    char fundata[32] = { 0 };
    st_XSAVEGAME_LEADER discard = { 0 };

    xSGReadData(original_xsgdata, rctxt, (char*)&discard, sizeof(st_XSAVEGAME_LEADER));
    xSGReadData(original_xsgdata, rctxt, fundata, 22);

    xASSERT_L(1221, !memcmp(fundata,"--TakeMeToYourLeader--",22));

    return TRUE;
}

S32 xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, char* data, S32 elesiz, S32 n)
{
    S32 cnt = 0;
    st_XSAVEGAME_CLIENT* clt = (st_XSAVEGAME_CLIENT*)wctxt;
    S32 is_ok = TRUE;
    void* mcprc = NULL;
    S32 rc = TRUE;

    xASSERT_L(1254, xsgdata);
    if (xsgdata->stage & XSG_STG_ABORT) return 0;

    xASSERT_L(1257, elesiz);
    if (!elesiz) return 0;

    cnt = n * elesiz;
    xASSERT_L(1262, cnt);
    if (!cnt) return cnt;

    if (xsgdata->upd_tally) {
        xASSERT_L(1269, (xsgdata->totamt + cnt) <= xsgdata->memsize);
        if ((xsgdata->totamt + cnt) > xsgdata->memsize) {
            XSG_PRINT(DBML_WARN, "xSGWR: Write Buffer Overrun\n");
            xsgdata->stage |= XSG_STG_ABORT;
            return 0;
        }
    }

    xASSERT_L(1283, xsgdata->buf_curpos);
    mcprc = memcpy(xsgdata->buf_curpos, data, cnt);
    xASSERT_L(1285, mcprc);
    if (!mcprc) {
        XSG_PRINT(DBML_WARN, "xSGWR: Buffer copy failed!\n");
        is_ok = FALSE;
    } else {
        XSG_PRINT(DBML_VDBG, "xSGWR: Updating position (+%d)\n", cnt);
        xsgdata->buf_curpos += cnt;
    }

    if (is_ok & xsgdata->upd_tally) {
        xsgdata->totamt += cnt;
        if (clt) clt->realamt += cnt;
        XSG_PRINT(DBML_VDBG, "xSGWR: Updating tally (+%d) Clt %d Tot %d\n", cnt, clt ? clt->realamt : -1, xsgdata->totamt);
    } else {
        XSG_PRINT(DBML_VDBG, "xSGWR: Tally update skipped\n");
    }

    if (!is_ok) {
        xsgdata->stage |= XSG_STG_ABORT;
        return 0;
    }

    return cnt / elesiz;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("str", "len", "cnt==len");
#endif

S32 xSGWriteStrLen(const char* str)
{
    S32 len = 0;
    xASSERT_L(1339, str);

    len = strlen(str);
    len += ISODD(len + 1) + 1;
    return len;
}

S32 xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, char* data, S32 n)
{
    S32 cnt = 0;
    xASSERT_L(1352, xsgdata);
    cnt = xSGWriteData(xsgdata, wctxt, data, 1, n);
    return cnt;
}

S32 xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, S32* data, S32 n)
{
    S32 cnt = 0;
    xASSERT_L(1400, xsgdata);
    cnt = xSGWriteData(xsgdata, wctxt, (char*)data, sizeof(S32), n);
    return cnt;
}

S32 xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, U32* data, S32 n)
{
    S32 cnt = 0;
    xASSERT_L(1410, xsgdata);

    cnt = xSGWriteData(xsgdata, wctxt, (char*)data, sizeof(U32), n);
    xASSERT_L(1413, cnt==n);
    return cnt;
}

S32 xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, F32* data, S32 n)
{
    S32 cnt = 0;

    xASSERT_L(1423, xsgdata);

    cnt = xSGWriteData(xsgdata, wctxt, (char*)data, sizeof(F32), n);
    return cnt;
}

S32 xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, char* buff, S32 elesiz, S32 n)
{
    S32 cnt = 0;
    st_XSAVEGAME_CLIENT* clt = (st_XSAVEGAME_CLIENT*)rctxt;
    void* mcprc = NULL;
    S32 act = 0;

    xASSERT_L(1445, elesiz);
    if (!elesiz) return 0;

    cnt = n * elesiz;
    xASSERT_L(1450, cnt);
    if (!cnt) return cnt;

    if (clt) {
        if (cnt > clt->readremain) {
            XSG_PRINT(DBML_WARN, "xSGRR: Buffer overread by client '%s'\n", xUtil_idtag2string(clt->idtag, 0));
            return 0;
        }
    }

    if (clt) mcprc = memcpy(buff, clt->readpos, cnt);
    else mcprc = memcpy(buff, xsgdata->walkpos, cnt);

    xsgdata->walkpos += cnt;
    xsgdata->walkremain -= cnt;

    if (clt) {
        clt->readpos += cnt;
        clt->readamt += cnt;
        clt->readremain -= cnt;
        xASSERT_L(1478, clt->readremain>=0);
    }

    xASSERT_L(1481, mcprc);
    if (!mcprc) XSG_PRINT(DBML_WARN, "xSGRR: Read Failed\n");

    return cnt / elesiz;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("amt", "safety++<10000", "xSGRS: String buffer insufficient (provided %d; need %d;)\n", "byt=='\\0'");
#endif

S32 xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, char* buff, S32 n)
{
    S32 cnt = 0;
    cnt = xSGReadData(xsgdata, rctxt, buff, 1, n);
    return cnt;
}

S32 xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, S32* buff, S32 n)
{
    S32 cnt = 0;
    cnt = xSGReadData(xsgdata, rctxt, (char*)buff, sizeof(S32), n);
    return cnt;
}

S32 xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, U32* buff, S32 n)
{
    S32 cnt = 0;
    cnt = xSGReadData(xsgdata, rctxt, (char*)buff, sizeof(U32), n);
    return cnt;
}

S32 xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, F32* buff, S32 n)
{
    S32 cnt = 0;
    cnt = xSGReadData(xsgdata, rctxt, (char*)buff, sizeof(F32), n);
    return cnt;
}

static S32 xSG_grab_leaders(st_XSAVEGAME_DATA* xsgdata)
{
    S32 num_found = 0;
    S32 i = 0;
    char readbuf[116] = { 0 };
    const char* namestr;
    st_XSAVEGAME_LEADER* lead;
    char* ptr;
    S32 rc = FALSE;

    memset(g_leaders, 0, sizeof(g_leaders));

    rc = xSG_chdir_gamedir(xsgdata);
    if (!rc) return num_found;

    for (i = 0; i < XSG_MAX_LEADERS; i++) {
        if (xSGGameIsEmpty(xsgdata, i)) continue;

        lead = &g_leaders[i];
        (void)lead;

        namestr = xSG_cm_slotname(xsgdata, i);
        XSG_PRINT(DBML_DBG, "XSG: Leader Read %s\n", namestr);
        rc = iSGReadLeader(xsgdata->isgsess, namestr, readbuf, sizeof(readbuf), 0);

        if (rc) {
            ptr = readbuf + 28;
            (void)ptr;
            num_found++; memcpy(lead, ptr, sizeof(st_XSAVEGAME_LEADER));
        }
    }

    return num_found;
}

S32 xSG_chdir_gamedir(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 rc = FALSE;
    const char* namestr = NULL;

    namestr = iSGMakeName(ISG_NGTYP_GAMEDIR, NULL, 0);
    rc = iSGSelectGameDir(xsgdata->isgsess, namestr);
    if (!rc) {
        result = FALSE;
    }

    return result;
}

char* xSG_cm_slotname(st_XSAVEGAME_DATA* xsgdata, S32 slotidx)
{
    static char slotname[32] = { 0 };
    const char* sptr = NULL;

    slotname[0] = '\0';

    if (slotidx == 2469) {
        sptr = iSGMakeName(ISG_NGTYP_CONFIG, NULL, 0);
    } else {
        xASSERT_L(1695, slotidx>=0);
        xASSERT_L(1696, slotidx<iSGMaxSaveGame(xsgdata->isgsess));
        sptr = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, slotidx);
    }

    xASSERT_L(1700, sptr);
    strcpy(slotname, sptr);

    return slotname;
}

static void xSG_areaComposeLabel(char* strbuf, S32, char*, S32)
{
    char stamp[256] = { 0 };
    iSGMakeTimeStamp(stamp);
    sprintf(strbuf, "%s", stamp);
}

S32 xSG_sv_flipinfo(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 i = 0;
    S32 rc = FALSE;
    st_XSAVEGAME_CLIENT* clt = NULL;
    S32 cltamt = 0;
    S32 cltmax = 0;

    xASSERT_L(1763, xsgdata);

    xASSERT_L(1765, xsgdata->stkcnt);
    for (i = 0; i < xsgdata->stkcnt; i++) {
        cltamt = 0;
        cltmax = 0;
        clt = &xsgdata->cltstk[i];

#if DEBUG
        if (i == 0) {
            xASSERT_L(1776, clt->idtag == LEADER_TAG);
        }
#endif

        xASSERT_L(1781, clt->cltinfo);
        rc = clt->cltinfo(clt->cltdata, xsgdata, &cltamt, &cltmax);
        if (!rc) {
            XSG_PRINT(DBML_WARN, "xSGFI: Client (%s) XSGCltInfoFunc failed\n", xUtil_idtag2string(clt->idtag, 0));
            result = FALSE;
        } else {
            xASSERT_L(1790, cltamt);
            clt->needamt = cltamt;
            clt->maxamt = cltmax;
            xsgdata->cltneed += cltamt;
            xsgdata->cltmax += cltmax;
            xsgdata->chdrneed += 3 * sizeof(S32);
        }

        if (!result) {
            xsgdata->stage |= XSG_STG_ABORT;
            break;
        }
    }

    xASSERT_L(1805, xsgdata->cltneed>0);

    return result;
}

S32 xSG_sv_prepdest(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 rc = FALSE;

    if (xsgdata->cltneed < 1) {
        XSG_PRINT(DBML_WARN, "xSGPD: No save space required (?!?)- Aborting");
        result = FALSE;
    } else {
        xsgdata->memsize = xsgdata->cltmax + xsgdata->chdrneed + 16;
        XSG_NOOP("Preliminary", 0);
        xsgdata->membuf = (char*)xMEMPUSHTEMP_L(xsgdata->memsize, 'XSGS', 1832);
        xASSERT_L(1833, xsgdata->membuf);
        if (!xsgdata->membuf) {
            XSG_PRINT(DBML_WARN, "xSGPD: Buffered allocation failed\n");
            result = FALSE;
        } else {
            XSG_PRINT(DBML_VDBG, "xSGPD: Pushed temp alloc for %s sized %d\n", xUtil_idtag2string('XSGS', 0), xsgdata->memsize);
            memset(xsgdata->membuf, 0, xsgdata->memsize);
            xsgdata->buf_curpos = xsgdata->membuf;
        }
    }

    return result;
}

S32 xSG_sv_flipproc(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 rc = FALSE;
    S32 i = 0;
    st_XSAVEGAME_CLIENT* clt = NULL;
    char bfill = 0xBF;
    S32 needfill = 0;

    for (i = 0; i < xsgdata->stkcnt; i++) {
        clt = &xsgdata->cltstk[i];

        rc = xSG_smem_cltopen(xsgdata, clt);
        xASSERT_L(1869, rc);

        if (rc) {
            rc = clt->cltproc(clt->cltdata, xsgdata, (st_XSAVEGAME_WRITECONTEXT*)clt);
            xASSERT_L(1874, rc);
            if (!rc) {
                XSG_PRINT(DBML_WARN, "xSGFP: Client (%s) XSGCltProcFunc failed\n", xUtil_idtag2string(clt->idtag, 0));
                result = FALSE;
            }

            needfill = clt->maxamt - clt->realamt;
            XSG_PRINT(DBML_VDBG, "xSGFP: Adding %d bytes filler data for client (%s)\n", needfill, xUtil_idtag2string(clt->idtag, 0));

            while (needfill-- > 0) xSGWriteData(xsgdata, NULL, &bfill, 1);
        }

        if (rc) {
            rc = xSG_smem_cltclose(xsgdata, clt);
            xASSERT_L(1894, rc);
            if (!rc) {
                result = FALSE;
                XSG_PRINT(DBML_WARN, "xSGFP: block close failed (%s)\n", xUtil_idtag2string(clt->idtag, 0));
            }
        }

        if (!result) {
            XSG_PRINT(DBML_WARN, "xSGFP: Process Failure - Aborting ...\n");
            break;
        }
    }

    return result;
}

S32 xSG_sv_bldchksum(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    char* bufptr = NULL;
    S32 bufamt = 0;

    XSG_NOOP(xsgdata->membuf, xsgdata->memsize);

    xASSERT_L(1921, xsgdata->readsize==xsgdata->loadsize);

    bufptr = xsgdata->membuf + 16;
    bufamt = xsgdata->memsize - 16;

    xsgdata->chksum = xUtil_crc_update(xsgdata->chksum, bufptr, bufamt);

    return result;
}

S32 xSG_smem_blkopen(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 ival = XSG_DUMMY_VAL;
    S32 savetag = XSG_SAVETAG;
    S32 savever = XSG_SAVE_VERSION;

    xASSERT_L(1943, xsgdata);
    xASSERT_L(1944, xsgdata->upd_tally);

    XSG_PRINT(DBML_VDBG, "XSG: Opening Main Data Block (Writing Header)\n");

    xSGWriteData(xsgdata, NULL, &savetag, 1);
    xSGWriteData(xsgdata, NULL, &savever, 1);

    xsgdata->buf_sizespot = xsgdata->buf_curpos;
    xSGWriteData(xsgdata, NULL, &ival, 1);

    xsgdata->buf_cksmspot = xsgdata->buf_curpos;
    xSGWriteData(xsgdata, NULL, &ival, 1);

    XSG_PRINT(DBML_VDBG, "XSG: Main Data Block Header Ready\n");

    return result;
}

S32 xSG_smem_blkclose(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    char* last_bufpos = NULL;
    S32 last_directpos = 0;

    xASSERT_L(1977, xsgdata);
    xASSERT_L(1978, xsgdata->upd_tally);

    XSG_PRINT(DBML_VDBG, "XSG: Closing Main Data Block (Updating Header)\n");

    xsgdata->upd_tally = FALSE;

    xASSERT_L(1985, xsgdata->memsize <= xsgdata->totamt);
    if (xsgdata->memsize > xsgdata->totamt) {
        XSG_PRINT(DBML_VDBG, "SaveGame: Module expected %d bytes max; used %d bytes!\n", xsgdata->memsize, xsgdata->totamt);
    }

    last_bufpos = xsgdata->buf_curpos;
    xsgdata->buf_curpos = xsgdata->buf_sizespot;
    xSGWriteData(xsgdata, NULL, &xsgdata->totamt, 1);
    xsgdata->buf_curpos = last_bufpos;

    last_bufpos = xsgdata->buf_curpos;
    xsgdata->buf_curpos = xsgdata->buf_cksmspot;
    xSGWriteData(xsgdata, NULL, &xsgdata->chksum, 1);
    xsgdata->buf_curpos = last_bufpos;

    xsgdata->upd_tally = TRUE;

    XSG_PRINT(DBML_VDBG, "XSG: Main Data Block Header Updated\n");

    return result;
}

S32 xSG_smem_cltopen(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_CLIENT* clt)
{
    S32 result = TRUE;
    S32 dummy = XSG_DUMMY_VAL;
    S32 ival = 0;

    xASSERT_L(2028, xsgdata);
    xASSERT_L(2029, clt);
    xASSERT_L(2030, xsgdata->upd_tally);

    XSG_PRINT(DBML_VDBG, "XSG: Opening Client Data Block Header for '%s'\n", xUtil_idtag2string(clt->idtag, 0));

    ival = clt->idtag;
    xSGWriteData(xsgdata, NULL, &ival, 1);

    clt->buf_maxpos = xsgdata->buf_curpos;
    xSGWriteData(xsgdata, NULL, &dummy, 1);

    clt->buf_sizepos = xsgdata->buf_curpos;
    xSGWriteData(xsgdata, NULL, &dummy, 1);

    XSG_PRINT(DBML_VDBG, "XSG: Client Data Block Header Ready (%s)\n", xUtil_idtag2string(clt->idtag, 0));

    return result;
}

S32 xSG_smem_cltclose(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_CLIENT* clt)
{
    S32 result = TRUE;
    char* last_bufpos = NULL;
    S32 last_directpos = 0;

    xASSERT_L(2062, xsgdata);
    xASSERT_L(2063, clt);

    XSG_PRINT(DBML_VDBG, "XSG: Closing Client Data Block Header for '%s'\n", xUtil_idtag2string(clt->idtag, 0));

    xsgdata->upd_tally = FALSE;

    xASSERT_L(2071, clt->realamt <= clt->needamt);
    if (clt->needamt < clt->realamt) {
        XSG_PRINT(DBML_WARN, "SaveGame: Client (%s) requested %d bytes but used %d bytes!\n", xUtil_idtag2string(clt->idtag, 0),
                  clt->needamt, clt->realamt);
    }

    last_bufpos = xsgdata->buf_curpos;

    xsgdata->buf_curpos = clt->buf_maxpos;
    xSGWriteData(xsgdata, NULL, &clt->maxamt, 1);
    xsgdata->buf_curpos = last_bufpos;

    xsgdata->buf_curpos = clt->buf_sizepos;
    xSGWriteData(xsgdata, NULL, &clt->realamt, 1);
    xsgdata->buf_curpos = last_bufpos;

    xsgdata->upd_tally = TRUE;

    XSG_PRINT(DBML_VDBG, "XSG: Client Data Block Header Updated ('%s')\n", xUtil_idtag2string(clt->idtag, 0));

    return result;
}

S32 xSG_sv_commit(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 rc = FALSE;
    const char* name = NULL;
    char browselabel[256] = { 0 };

    xASSERT_L(2116, xsgdata);
    xASSERT_L(2117, !(xsgdata->stage & XSG_STG_ABORT));
    xASSERT_L(2118, xsgdata->mode == XSG_MODE_SAVE);

    XSG_NOOP(0, 0);

    XSG_PRINT(DBML_VDBG, "Committing Save Game to disk\n");

    name = iSGMakeName(ISG_NGTYP_GAMEDIR, NULL, 0);
    rc = iSGSetupGameDir(xsgdata->isgsess, name, 1);
    if (!rc) result = FALSE;
    else {
        xSG_areaComposeLabel(browselabel, xsgdata->thumbIconIdx, xsgdata->label, xsgdata->gslot + 1);

        name = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, xsgdata->gslot);
        rc = iSGSaveFile(xsgdata->isgsess, name, xsgdata->membuf, xsgdata->memsize, 1, browselabel);
        xASSERT_L(2147, rc);
        if (!rc) result = FALSE;
    }

    return result;
}

void xSG_cb_ISGChange(void* cltdata, en_CHGCODE what)
{
    st_XSAVEGAME_DATA* xsgdata = (st_XSAVEGAME_DATA*)cltdata;
    xASSERT_L(2159, xsgdata);

    XSGAutoData* asg = xSGAutoSave_GetCache();
    xASSERT_L(2164, asg);

    switch (what) {
    case ISG_CHG_TARGET:
        XSG_PRINT(DBML_DBG, "XSG notified of ISG change event (target)\n");
        asg->MarkInvalid();
        break;
    case ISG_CHG_GAMELIST:
        XSG_PRINT(DBML_DBG, "XSG notified of ISG change event (gamelist)\n");
        asg->MarkInvalid();
        break;
    case ISG_CHG_NONE:
    default:
        xASSERT_L(2180, 0&&"Unknown event");
        XSG_PRINT(DBML_DBG, "XSG notified of ISG unknown change event (%d)\n", what);
        break;
    }
}

S32 xSG_ld_prepload(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    const char* name = NULL;

    name = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, xsgdata->gslot);
    xsgdata->loadsize = iSGFileSize(xsgdata->isgsess, name);
    if (xsgdata->loadsize < 1) {
        result = FALSE;
    } else {
        XSG_PRINT(DBML_VDBG, "xSG_ld_prepload: PrepLoad of file %s sized %d\n", name, xsgdata->loadsize);

        xsgdata->loadbuf = (char*)xMEMPUSHTEMP_L(xsgdata->loadsize, 'XSGL', 2222);
        xVALIDATE_L(2223, xsgdata->loadbuf);
        memset(xsgdata->loadbuf, 0, xsgdata->loadsize);
    }

    return result;
}

S32 xSG_ld_readgame(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 rc = FALSE;
    const char* name = NULL;

    if (xsgdata->stage & XSG_STG_ABORT) result = FALSE;

    name = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, xsgdata->gslot);
    rc = iSGLoadFile(xsgdata->isgsess, name, xsgdata->loadbuf, 1);
    if (!rc) result = FALSE;

    if (!result) xsgdata->stage |= XSG_STG_ABORT;
    return result;
}

S32 xSG_ld_readhead(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 ival = 0;
    bool damagedSave = false;

    xsgdata->walkpos = xsgdata->loadbuf;
    xsgdata->walkremain = xsgdata->loadsize;

    xSGReadData(xsgdata, NULL, &ival, 1);
    xASSERT_L(2268, ival == XSG_SAVETAG);
    if (ival != XSG_SAVETAG) {
        result = FALSE;
        damagedSave = true;
    }

    xSGReadData(xsgdata, NULL, &ival, 1);
    xASSERT_L(2279, ival==XSG_SAVE_VERSION);
    if (ival != XSG_SAVE_VERSION) {
        result = FALSE;
        damagedSave = true;
    }

    xSGReadData(xsgdata, NULL, &ival, 1);
    xsgdata->readsize = ival;
    xASSERT_L(2293, xsgdata->readsize<=xsgdata->loadsize);

    xSGReadData(xsgdata, NULL, &ival, 1);
    xsgdata->file_chksum = ival;
    xASSERT_L(2301, xsgdata->file_chksum!=0);
    xASSERT_L(2302, xsgdata->file_chksum!=XSG_DUMMY_VAL);

    if (damagedSave) {
        return -1;
    }

    return result;
}

S32 xSG_ld_validate(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;

    XSG_NOOP(xsgdata->loadbuf, xsgdata->loadsize);

    xASSERT_L(2328, xsgdata->readsize<=xsgdata->loadsize);
    xsgdata->loadsize = xsgdata->readsize;

    xsgdata->walkpos = xsgdata->loadbuf + 16;
    xsgdata->walkremain = xsgdata->loadsize - 16;

    xsgdata->read_chksum = xUtil_crc_update(xsgdata->read_chksum, xsgdata->walkpos, xsgdata->walkremain);

    xASSERT_L(2346, xsgdata->read_chksum == xsgdata->file_chksum);
    if (xsgdata->read_chksum != xsgdata->file_chksum) {
        XSG_PRINT(DBML_WARN, "xSGLC: Integrity error: Expected: %08X Found %08X\n", xsgdata->file_chksum, xsgdata->read_chksum);
        result = -1;
    }

    return result;
}

S32 xSG_ld_findcltblk(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 ival = 0;
    U32 tag = 0;
    S32 maxamt = 0;
    S32 actamt = 0;
    S32 safety = 0;
    st_XSAVEGAME_CLIENT* clt = NULL;
    S32 i = 0;
    S32 found = FALSE;
    S32 rc = FALSE;
    st_XSAVEGAME_CLIENT* dfltclt = NULL;

    dfltclt = &xsgdata->dfltloadclt;

    xsgdata->walkpos = xsgdata->loadbuf;
    xsgdata->walkremain = xsgdata->loadsize;

    xSGReadData(xsgdata, NULL, &ival, 1);
    xSGReadData(xsgdata, NULL, &ival, 1);
    xSGReadData(xsgdata, NULL, &ival, 1);
    xSGReadData(xsgdata, NULL, &ival, 1);

    while (xsgdata->walkremain > 0) {
        tag = 0;
        maxamt = 0;
        actamt = 0;
        found = FALSE;

        xSGReadData(xsgdata, NULL, &tag, 1);
        xSGReadData(xsgdata, NULL, &maxamt, 1);
        xSGReadData(xsgdata, NULL, &actamt, 1);

        XSG_PRINT(DBML_VDBG, "xSG_ld_findcltblk: Clt %s; amount: %d of %d @%08X\n", xUtil_idtag2string(tag, 0), actamt, maxamt,
                  xsgdata->walkpos);

        for (i = 0; i < xsgdata->stkcnt; i++) {
            clt = &xsgdata->cltstk[i];
            if (clt->idtag == tag) {
                found = TRUE;
                xASSERT_L(2421, !clt->readpos);

                clt->blokpos = xsgdata->walkpos;
                clt->blokact = actamt;
                clt->blokmax = maxamt;
                clt->readpos = xsgdata->walkpos;
                clt->readamt = 0;
                clt->readremain = actamt;
                break;
            }
        }

        if (!found) {
            if (dfltclt->idtag) {
                xASSERT_L(2445, dfltclt->cltload);
                XSG_PRINT(DBML_DBG, "xSG: Using Default Load Client for %s\n", xUtil_idtag2string(tag, 0));

                dfltclt->blokpos = xsgdata->walkpos;
                dfltclt->blokact = actamt;
                dfltclt->blokmax = maxamt;
                dfltclt->readpos = xsgdata->walkpos;
                dfltclt->readamt = 0;
                dfltclt->readremain = actamt;

                rc = dfltclt->cltload(dfltclt->cltdata, xsgdata, (st_XSAVEGAME_READCONTEXT*)dfltclt, tag, dfltclt->blokact);
                if (!rc) {
                    XSG_PRINT(DBML_WARN, "xSG: Default Loader Client reports error for %s\n", xUtil_idtag2string(tag, 0));
                }

                if (dfltclt->readremain > 0) {
                    xsgdata->walkpos += dfltclt->readremain + (maxamt - actamt);
                    xsgdata->walkremain -= dfltclt->readremain + (maxamt - actamt);
                }
            } else {
                XSG_PRINT(DBML_WARN, "xSG: SaveGame data block '%s' has no owner (and no default loader client exists\n",
                          xUtil_idtag2string(tag, 0));
                xsgdata->walkpos += maxamt;
                xsgdata->walkremain -= maxamt;
            }
        } else {
            xsgdata->walkpos += maxamt;
            xsgdata->walkremain -= maxamt;
        }

        xASSERT_L(2510, safety++<65000);
    }

#if DEBUG
    for (i = 0; i < xsgdata->stkcnt; i++) {
        clt = &xsgdata->cltstk[i];
        xASSERT_L(2518, clt->blokpos && !ISODD((S32)clt->blokpos));
        xASSERT_L(2519, clt->blokact && clt->blokmax && (clt->blokact<=clt->blokmax));
        xASSERT_L(2520, clt->readpos && !ISODD((S32)clt->readpos));
        xASSERT_L(2521, clt->readremain);
    }
#endif

    return result;
}

S32 xSG_ld_flipload(st_XSAVEGAME_DATA* xsgdata)
{
    S32 result = TRUE;
    S32 rc = FALSE;
    S32 i = 0;
    st_XSAVEGAME_CLIENT* clt = NULL;

    XSG_PRINT(DBML_VDBG, "Flipping client loads ...\n");

    for (i = 0; i < xsgdata->stkcnt; i++) {
        clt = &xsgdata->cltstk[i];
        if (clt->cltload && clt->blokact && clt->blokpos) {
            XSG_PRINT(DBML_VDBG, "FlipClient %s ...\n", xUtil_idtag2string(clt->idtag, 0));
            rc = clt->cltload(clt->cltdata, xsgdata, (st_XSAVEGAME_READCONTEXT*)clt, clt->idtag, clt->blokact);
            xASSERT_L(2546, clt->readremain >= 0);

            if (!rc) {
                XSG_PRINT(DBML_WARN, "FlipClient %s - reports error\n", xUtil_idtag2string(clt->idtag, 0));
                result = FALSE;
            }
        } else {
            XSG_PRINT(DBML_WARN, "FlipClient %s - no data?\n", xUtil_idtag2string(clt->idtag, 0));
        }

        if (!result) break;
    }

    return result;
}

static XSGAutoData g_autodata;

XSGAutoData* xSGAutoSave_GetCache() { return &g_autodata; }

void xSGAutoSave_Startup()
{
    XSGAutoData* asg = xSGAutoSave_GetCache();

    asg->flg_autodata = 0;
    asg->lastGame = -1;
    asg->lastTarg = -1;
    asg->lastPhysicalSlot = -1;

    iSGAutoSave_Startup();
}

S32 XSGAutoData::IsValid()
{
    if (this->flg_autodata & 1) {
        if (!this->HWCheckConnect(this->lastTarg)) {
            this->flg_autodata &= ~1;
        }
    }

    return this->flg_autodata & 1;
}

void XSGAutoData::MarkInvalid()
{
    this->flg_autodata &= ~1;
}

S32 XSGAutoData::SetCache(S32 targ, S32 game, S32 physicalSlot)
{
    this->Discard();

    this->isg_monitor = this->HWConnect(targ);
    if (!this->isg_monitor) {
        return FALSE;
    }

    this->lastTarg = targ;
    this->lastGame = game;
    this->lastPhysicalSlot = physicalSlot;
    this->flg_autodata |= 1;

    return TRUE;
}

void XSGAutoData::Discard()
{
    this->flg_autodata = 0;
    this->lastTarg = -1;
    this->lastGame = -1;
    this->lastPhysicalSlot = -1;
    if (this->isg_monitor) this->HWDisconnect(this->isg_monitor);
    this->isg_monitor = NULL;
}

st_ISGSESSION* XSGAutoData::HWConnect(S32 idx_target)
{
    return iSGAutoSave_Connect(idx_target, xSGAutoSave_GetCache(), ASG_ISG_changed);
}

void XSGAutoData::HWDisconnect(st_ISGSESSION* isg)
{
    iSGAutoSave_Disconnect(isg);
}

S32 XSGAutoData::HWCheckConnect(S32)
{
    return iSGAutoSave_Monitor(this->isg_monitor, this->lastTarg);
}

void XSGAutoData::Refresh()
{
    if (this->lastTarg == -1 || this->lastGame == -1 || this->lastPhysicalSlot == -1 || !(this->flg_autodata & 1)) {
        return;
    }

    this->isg_monitor = this->HWConnect(this->lastTarg);
    if (this->isg_monitor) {
        return;
    }
}

static void ASG_ISG_changed(void* cltdata, en_CHGCODE what)
{
    xASSERT_L(2693, cltdata == xSGAutoSave_GetCache());
    XSGAutoData* asg = xSGAutoSave_GetCache();

    switch (what) {
    case ISG_CHG_TARGET:
        XSG_PRINT(DBML_DBG, "XSG notified of ISG change event (target)\n");
        asg->MarkInvalid();
        break;
    case ISG_CHG_GAMELIST:
        XSG_PRINT(DBML_DBG, "XSG notified of ISG change event (gamelist)\n");
        asg->MarkInvalid();
        break;
    case ISG_CHG_NONE:
    default:
        xASSERT_L(2713, 0&&"Unknown event");
        XSG_PRINT(DBML_DBG, "XSG notified of ISG unknown change event (%d)\n", what);
        break;
    }
}

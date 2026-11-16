#include "xCommon.h"
#include "isavegame.h"

#include "xMath.h"
#include "xRegionSupport.h"
#include "xstransvc.h"
#include "xSndMgr.h"
#include "xutil.h"
#include "iFile.h"
#include "iTRC.h"
#include "zGlobals.h"
#include "zMain.h"

#include "decomp.h"

#include <dolphin.h>
#include <stdio.h>
#include <string.h>

#define ISG_MAXSTR_SAVENAME 32
#define ISG_MAX_TARGET 2
#define ISG_MAX_SAVEGAME 3

#define ISG_ROUNDUP(n, a) (((n) + (a) - 1) & -(a))
#define ISG_TRACE(what, rc) DBprintf(_g_mlvl, "ISG: -- %s (%d)\n", what, rc)

enum en_ISGMC_ERRSTATUS
{
    ISGMC_ERR_NONE,
    ISGMC_ERR_NOMEMCARD,
    ISGMC_ERR_MKDIR,
    ISGMC_ERR_OPEN,
    ISGMC_ERR_CLOSE,
    ISGMC_ERR_READ,
    ISGMC_ERR_WRITE
};

struct st_ISG_MEMCARD_DATA
{
    S32 ismounted;
    S32 myphys;
    S32 sectsize;
    CARDFileInfo cur_finf;
    CARDStat cur_stat;
    S64 cur_ftime;
    S32 off_nextwrite;
    CARDFileInfo finfcache;
    CARDStat statcache;
    S64 timecache;
    en_ISGMC_ERRSTATUS mcerr;
    S32 cardIsIll;
};

enum en_ISG_WHYFAIL
{
    ISG_WHYERR_NONE,
    ISG_WHYERR_NOCARD,
    ISG_WHYERR_NOROOM,
    ISG_WHYERR_DAMAGE,
    ISG_WHYERR_OTHER,
    ISG_WHYERR_NOMORE
};

struct st_ISGSESSION
{
    st_ISG_MEMCARD_DATA physcard[ISG_MAX_TARGET];
    S32 idx_actvcard;
    en_CHGCODE chgcode;
    en_ASYNC_OPERR as_operr;
    en_ASYNC_OPSTAT as_fakestat;
    en_ISG_WHYFAIL whyFail;
    ISGChangeCallback chgfunc;
    void* cltdata;
};

struct st_ERROR_LOOKUP
{
    S32 errid;
    char* errmsg;
};

struct st_ISG_TPL_TEXHEADER
{
    U16 height;
    U16 width;
    u32 format;
    char* data;
    GXTexWrapMode wrapS;
    GXTexWrapMode wrapT;
    GXTexFilter minFilter;
    GXTexFilter magFilter;
    F32 LODBias;
    U8 edgeLODEnable;
    U8 minLOD;
    U8 maxLOD;
    U8 unpacked;
};

struct st_ISG_TPL_CLUTHEADER
{
    U16 numEntries;
    U8 unpacked;
    U8 pad8;
    GXTlutFmt format;
    char* data;
};

struct st_ISG_TPL_TEXDESCRIP
{
    st_ISG_TPL_TEXHEADER* textureHeader;
    st_ISG_TPL_CLUTHEADER* CLUTHeader;
};

struct st_ISG_TPL_TEXPALETTE
{
    u32 versionNumber;
    u32 numDescriptors;
    st_ISG_TPL_TEXDESCRIP* descriptorArray;
};

struct st_ISG_CARD_ICONBLOCK
{
    U8 Title[32];
    U8 Description[32];
    U8 Banner[6144];
    U8 Icons[8][2048];
    U8 buncho[512];
    char integchk[512];
};

enum en_ISG_IOMODE
{
    ISG_IOMODE_READ = 1,
    ISG_IOMODE_WRITE,
    ISG_IOMODE_APPEND
};

static en_VERBOSE_MSGLEVEL _g_mlvl = DBML_VDBG;
static char* ISG_GC_GAMENAME = "GICE";
static S32 g_isginit;
static st_ISGSESSION g_isgdata_MAIN = { 0 };
static char cardwork[ISG_MAX_TARGET][CARD_WORKAREA_SIZE] ATTRIBUTE_ALIGN(32);
static char* g_rawicon;
static char* g_rawbanr;
static S32 g_iconsize;
static S32 g_banrsize;
#if DEBUG
static U32 g_chkicon;
static U32 g_chkbanr;
#endif
static S32 g_legalSectSize[3] = { 0x2000, 0, -1 };
static bool isMounted;

static S32 iSG_start_your_engines();
static S32 iSG_mc_exists(S32 cardslot);
static S32 iSG_mc_isformatted(st_ISG_MEMCARD_DATA* mcdata);
static S32 iSG_mc_isGCcard(st_ISG_MEMCARD_DATA* mcdata, S32* badencode, S32* wornout);
static S32 iSG_isSpaceForFile(st_ISG_MEMCARD_DATA* mcdata, S32 fsize, const char* fname, S32* amtneed, S32* amtfree,
                              S32* needFiles);
static S32 iSG_mc_settgt(st_ISG_MEMCARD_DATA* mcdata, S32 cardslot);
static S32 iSG_get_finfo(st_ISG_MEMCARD_DATA* mcdata, const char* fname);
static S32 iSG_get_fsize(st_ISG_MEMCARD_DATA* mcdata, const char* fname);
static S32 iSG_get_fmoddate(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32* sec, S32* min, S32* hr, S32* mon,
                            S32* day, S32* yr);
static S32 iSG_cubeicon_size(S32 cardslot, S32 sectsize);
static void iSG_timestamp(CARDStat*);
static char* iSG_bfr_icondata(char* buffer, CARDStat*, char* label, S32 sectsize);
static void iSG_upd_icostat(CARDStat*, CARDStat* realstat);
static S32 iSG_mc_fopen(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32 fsize, en_ISG_IOMODE mode,
                        en_ASYNC_OPERR* whyFail);
static S32 iSG_mc_fclose(st_ISG_MEMCARD_DATA* mcdata, CARDStat* statcopy);
static S32 iSG_mc_fdel(st_ISG_MEMCARD_DATA* mcdata, const char* fname);
static S32 iSG_mc_fclose(st_ISG_MEMCARD_DATA* mcdata);
static S32 iSG_mc_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize, S32 skipbyte);
static S32 iSG_mcqa_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize, S32 skipbyte);
static S32 iSG_mc_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* data, S32 n);
static S32 iSG_mcqa_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* data, S32 n);
static void iSG_cb_asyndone(s32, s32 retcode);
static S32 iSG_curKosher(CARDStat* stat, CARDFileInfo* finf);
static S32 iSG_load_icondata();
static S32 iSG_chk_icondata();
static void iSG_discard_icondata();
static S32 iSG_tpl_unpack(st_ISG_TPL_TEXPALETTE* pal);
static st_ISG_TPL_TEXDESCRIP* iSG_tpl_TEXGet(st_ISG_TPL_TEXPALETTE* pal, U32 idx);
static S32 iSG_bnr_unpack(st_ISG_TPL_TEXPALETTE* pal);
static S32 iSG_mc_mount(S32 cardslot);
static S32 iSG_mc_unmount(S32 cardslot);
static void iSG_cb_unmount(s32 chan, s32 retcode);
static S32 iSG_mc_format(st_ISG_MEMCARD_DATA* mcdata, S32 force, S32* canRecover);
static char* iSGDB_tlate_cardres(S32 errid);
#if DEBUG
static void iSGDB_dump_curstat(CARDStat* stat, S32 doWarn);
static void iSGDB_dump_curfinf(CARDFileInfo* finf, S32);
static S32 iSGDB_mc_formatforce(st_ISG_MEMCARD_DATA* mcdata);
static void NukaDaCarda();
static void FilaDaCarda();
#else
#define iSGDB_dump_curstat(stat, doWarn)
#define iSGDB_dump_curfinf(finf, doWarn)
#endif

S32 iSGStartup()
{
    S32 rc = 1;

    if (g_isginit++) return g_isginit;

    DBprintf(DBML_VDBG, "iSG: Starting Save Game Module (GameCube)\n");

    DBprintf(DBML_VDBG, "iSaveGame using GameTitle product key ... %s\n", "Incrdbls");

    DBprintf(DBML_VDBG, "ISGData          %d\n", sizeof(st_ISGSESSION));
    DBprintf(DBML_VDBG, "ISGMCData (2)    %d\n", sizeof(st_ISG_MEMCARD_DATA) * 2);
    DBprintf(DBML_VDBG, "CARDStat (2)     %d\n", sizeof(CARDStat) * 2);
    DBprintf(DBML_VDBG, "CARDFileInfo (2) %d\n", sizeof(CARDFileInfo) * 2);
    DBprintf(DBML_VDBG, "Card Work Area   %d\n", sizeof(cardwork));
    DBprintf(DBML_VDBG, "ISGAsync         %d\n", 12);

    rc = iSG_start_your_engines();
    if (!rc) {
        DBprintf(DBML_ERR, "Save Module System Startup Failed!\n");
        xASSERT_L(158, rc);
    }

    rc = iSG_load_icondata();
    xASSERT_L(163, rc);

#if DEBUG
    static S32 nuka_da_carda = 0;
    if (nuka_da_carda) {
        NukaDaCarda();
    }

    FilaDaCarda();
#endif

    return g_isginit;
}

S32 iSGShutdown()
{
    iSG_discard_icondata();

    return 1;
}

char* iSGMakeName(en_NAMEGEN_TYPE type, const char* base, S32 idx)
{
    static char rotatebuf[8][32] = { 0 };
    static S32 rotate = 0;
    char* use_buf;
    const char* fmt_sb = "%s%08s";
    const char* fmt_sd = "%s%02d";
    const char* fmt_sbd = "%s%06s%02d";

    use_buf = rotatebuf[rotate++];
    if (rotate == 8) rotate = 0;

    use_buf[0] = '\0';
    xVALIDATE_L(231, idx<100);
    xVALIDATE_L(232, idx>=0);

    switch (type) {
    case ISG_NGTYP_GAMEFILE:
        if (base) sprintf(use_buf, fmt_sd, base, idx + 1);
        else sprintf(use_buf, fmt_sd, "IncrediblesRoU", idx + 1);
        break;
    case ISG_NGTYP_GAMEDIR:
    case ISG_NGTYP_CONFIG:
    case ISG_NGTYP_ICONTHUM:
        DBprintf(DBML_VDBG, "ISG: Ignoring unsupported name gen type (%d)\n", type);
        break;
    default:
        DBprintf(DBML_WARN, "iSG: Unknown name gen type %d\n", type);
        break;
    }

    xVALIDATE_L(255, strlen(use_buf)<ISG_MAXSTR_SAVENAME);

    return use_buf;
}

st_ISGSESSION* iSGSessionBegin(void* cltdata, ISGChangeCallback chgfunc, S32)
{
    st_ISGSESSION* isgdata = NULL;

    isgdata = &g_isgdata_MAIN;

    S32 rc = 0;

    iTRC_CHECK_L(282);

    memset(isgdata, 0, sizeof(st_ISGSESSION));
    isgdata->idx_actvcard = -1;
    isgdata->chgfunc = chgfunc;
    isgdata->cltdata = cltdata;
    rc = iSG_chk_icondata();
    xVALIDATE_L(291, rc);

    return isgdata;
}

void iSGSessionEnd(st_ISGSESSION* isgdata)
{
    S32 i = 0;
    S32 rc = 0;
    st_ISG_MEMCARD_DATA* mcd = NULL;

    xVALIDATE_L(303, isgdata);

    iTRC_CHECK_L(306);

    for (i = 0; i < ISG_MAX_TARGET; i++) {
        mcd = &isgdata->physcard[i];
        if (!mcd->ismounted) continue;
        rc = iSG_mc_unmount(i);
        xVALIDATE_L(312, rc);
        mcd->ismounted = 0;
    }

    memset(isgdata, 0, sizeof(st_ISGSESSION));
    isgdata->idx_actvcard = -1;
}

S32 iSGMaxSaveGame(st_ISGSESSION*)
{
    return ISG_MAX_SAVEGAME;
}

S32 iSGTgtCount(st_ISGSESSION*, S32* max)
{
    S32 cnt = 0;
    S32 rc = 0;
    s32 memsize = 0;
    s32 sectorsize = 0;
    S32 i = 0;
    char ab[9] = "AB------";

    if (max) *max = ISG_MAX_TARGET;

    for (i = 0; i < ISG_MAX_TARGET; i++) {
        iTRC_CHECK_L(362);

        do {
            rc = CARDProbeEx(i, &memsize, &sectorsize);
            ISG_TRACE("CARDProbeEx", rc);
        } while (rc == CARD_RESULT_BUSY);

        if (rc == CARD_RESULT_READY) {
            cnt++;
            DBprintf(DBML_VDBG, "ISG: MemCard slot %c has sector size %d\n", ab[i], sectorsize);
        } else {
            DBprintf(DBML_VDBG, "ISG: MemCard slot %c failed - %s\n", ab[i], iSGDB_tlate_cardres(rc));
        }
    }

    return cnt;
}

S32 iSGTgtPhysSlotIdx(st_ISGSESSION*, S32 tidx)
{
    S32 physslot = -1;
    S32 rc = 0;

    rc = iSG_mcidx2slot(tidx, &physslot, NULL);
    xVALIDATE_L(392, rc);
    if (rc) return physslot;
    return -1;
}

#if DEBUG
DECOMP_FORCEACTIVE("%s(%d) : (rc||(form==ISG_TNAM_FMTNOSLOT)) in '%s'\n");
#endif
DECOMP_FORCEACTIVE("Slot %c", "Memory Card Slot %c", "Nintendo GameCube Memory Card in Slot %c",
                   "Nintendo GameCube%s Memory Card in Slot %c", "^", "Nintendo GameCube%s Memory Card");

S32 iSGTgtFormat(st_ISGSESSION* isgdata, S32 tgtidx, S32, S32* canRecover)
{
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 slot = 0;
    S32 rc = 0;

    iTRC_CHECK_L(574);

    rc = iSG_mc_exists(isgdata->idx_actvcard);
    xVALIDATE_L(577, rc);
    if (!rc) return 0;

    rc = iSG_mcidx2slot(tgtidx, &slot, NULL);
    xVALIDATE_L(581, rc);

    xVALIDATE_L(583, slot == isgdata->idx_actvcard);
    if (slot != isgdata->idx_actvcard) return 0;

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    xVALIDATE_L(589, mcdata->ismounted);
    if (!mcdata->ismounted) return 0;

    rc = iSG_mc_isGCcard(mcdata, NULL, NULL);
    xVALIDATE_L(594, rc);
    if (!rc) return 0;

    rc = iSG_mc_format(mcdata, 0, canRecover);

    if (rc == -1) {
        return -1;
    } else if (rc == 0) {
        return 0;
    }

    return 1;
}

U32 iSGTgtState(st_ISGSESSION* isgdata, S32 tgtidx, const char*)
{
    U32 state = 0;
    S32 rc = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 physslot = 0;
    S32 prob_encode = 0;
    S32 prob_wornout = 0;
    S32 i = 0;
    S32 isok = 0;

    iTRC_CHECK_L(631);

    rc = iSG_mcidx2slot(tgtidx, &physslot, NULL);
    xVALIDATE_L(637, rc);
    if (physslot != isgdata->idx_actvcard) {
        DBprintf(DBML_WARN, "ISG: TgtState - override switch of active card\n");

        rc = iSGTgtSetActive(isgdata, tgtidx);
        xVALIDATE_L(645, rc || (!rc && (isgdata->idx_actvcard<0)));
    }

    if (isgdata->idx_actvcard < 0) {
        state |= 0x1000000;
        return state;
    }

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];
    if (!mcdata->ismounted) {
        DBprintf(DBML_WARN, "ISG: Skipped Target State on UnMounted Card\n");
        return state;
    }

    rc = iSG_mc_exists(isgdata->idx_actvcard);
    if (rc) state |= 0x1;
    else return state;

    if (mcdata->cardIsIll) {
        state |= 0x1000000;
        return state;
    }

    rc = iSG_mc_isGCcard(mcdata, &prob_encode, &prob_wornout);
    if (prob_encode) {
        state |= 0x4000000;
    }
    if (prob_wornout) {
        state |= 0x2000000;
    }
    if (rc) {
        state |= 0x4;
    } else {
        return state;
    }

    if (g_legalSectSize[0] > 0) {
        i = 0;
        while (g_legalSectSize[i] > 0) {
            if (mcdata->sectsize == g_legalSectSize[i]) {
                isok = 1;
                break;
            }
            i++;
        }
        if (!isok) {
            state |= 0x8000000;
            state &= ~0x4;
            return state;
        }
    }

    rc = iSG_mc_isformatted(mcdata);
    if (rc) state |= 0xA;
    else return state;

    return state;
}

DECOMP_FORCEACTIVE("is damaged and cannot be used", "unsupprted sector size", "is formatted for another market",
                   "encountered unexpected error (%d)");

S32 iSGTgtFreeSpace(st_ISGSESSION* isgdata, S32 tgtidx, S32* clustsize, S32* availFile)
{
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    s32 bytefree = 0;
    s32 filefree = 0;
    U32 tgtstate = 0;
    S32 rc = 0;

    if (clustsize) *clustsize = 0;
    if (availFile) *availFile = 0;

    xVALIDATE_L(773, isgdata->idx_actvcard >= 0);

    xVALIDATE_L(775, !(isgdata->idx_actvcard<0));
    if (isgdata->idx_actvcard < 0) return 0;

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    iTRC_CHECK_L(781);

    tgtstate = iSGTgtState(isgdata, tgtidx, NULL);
    if (!tgtstate) return 0;

    rc = CARDFreeBlocks(isgdata->idx_actvcard, &bytefree, &filefree);
    ISG_TRACE("CARDFreeBlocks", rc);

    if (availFile) *availFile = filefree;

    if (clustsize) {
        *clustsize = ISG_ROUNDUP(512, mcdata->sectsize);
    }

    xVALIDATE_L(802, mcdata->sectsize>0);

    DBprintf(DBML_DBG, "ISG: MemCard has %d free blocks (%d basis)\n", bytefree / mcdata->sectsize, mcdata->sectsize);

    return bytefree;
}

S32 iSGTgtSetActive(st_ISGSESSION* isgdata, S32 tgtidx)
{
    S32 result = 0;
    S32 rc = 0;
    S32 slot = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;

    rc = iSG_mcidx2slot(tgtidx, &slot, NULL);
    xVALIDATE_L(819, rc);

    mcdata = &isgdata->physcard[slot];

    iTRC_CHECK_L(824);

    if (mcdata->ismounted) result = 1;
    else {
        rc = iSG_mc_settgt(mcdata, slot);
        if (rc) {
            isgdata->idx_actvcard = slot;
            result = 1;
        }
    }

    return result;
}

S32 iSGTgtHaveRoom(st_ISGSESSION* isgdata, S32, S32 fsize, const char*, const char* fname, S32* needbyte,
                   S32* availbyte, S32* needFile)
{
    S32 result = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 i = 0;

    if (isgdata->idx_actvcard < 0) return result;

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    xVALIDATE_L(861, mcdata->ismounted);
    if (!mcdata->ismounted) return result;

    S32 gameFileCount = 0;
    if (!fname) {
        for (i = 0; i < ISG_MAX_SAVEGAME; i++) {
            iTRC_CHECK_L(877);

            fname = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i);
            if (iSG_get_finfo(mcdata, fname)) {
                gameFileCount++;
            }
        }
    }

    result = iSG_isSpaceForFile(mcdata, fsize, fname, needbyte, availbyte, needFile);

    if (gameFileCount > 0 && *needbyte > *availbyte) {
        if (needFile && *needbyte > *availbyte) {
            *needFile = 0;
            return 0;
        }
        return 1;
    }

    S32 fileCount = 0;
    for (i = 0; i < CARD_MAX_FILE && isgdata->idx_actvcard >= 0 && isgdata->idx_actvcard < ISG_MAX_TARGET; i++) {
        CARDFileInfo fileInfo;
        S32 gcResult = CARDFastOpen(isgdata->idx_actvcard, i, &fileInfo);
        if (gcResult == CARD_RESULT_READY) {
            fileCount++;
            CARDClose(&fileInfo);
        }
    }

    if (fileCount >= CARD_MAX_FILE) {
        if (gameFileCount > 0) {
            if (needFile) {
                *needFile = 0;
            }
            return 1;
        }
        return 0;
    }

    if (*needbyte > *availbyte) {
        return 0;
    }

    return result;
}

#if DEBUG
DECOMP_FORCEACTIVE("%s(%d) : (isgdata->idx_actvcard>=0) in '%s'\n",
                   "%s(%d) : (mcdata->myphys == isgdata->idx_actvcard) in '%s'\n");
#endif
#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("ISG: FileCount Query - MemCard not mounted\n");
#endif

S32 iSGFileSize(st_ISGSESSION* isgdata, const char* fname)
{
    S32 size = -1;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 skipsize = 0;

    xVALIDATE_L(1171, !(isgdata->idx_actvcard<0));
    if (isgdata->idx_actvcard < 0) return size;

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    iTRC_CHECK_L(1176);

    size = iSG_get_fsize(mcdata, fname);

    if (size >= 0) {
        skipsize = iSG_cubeicon_size(mcdata->myphys, mcdata->sectsize);
        xVALIDATE_L(1187, skipsize<size);
        size -= skipsize;
        if (size < 0) size = -1;
    }

    return size;
}

const char* iSGFileModDate(st_ISGSESSION* isgdata, const char* fname)
{
    const char* date_str = NULL;
    date_str = iSGFileModDate(isgdata, fname, NULL, NULL, NULL, NULL, NULL, NULL);
    return date_str;
}

char* iSGFileModDate(st_ISGSESSION* isgdata, const char* fname, S32* usr_sec, S32* usr_min, S32* usr_hr,
                     S32* usr_mon, S32* usr_day, S32* usr_yr)
{
    static char datestr[256] = { 0 };
    S32 rc = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 sec = 0, min = 0, hr = 0, mon = 0, day = 0, yr = 0;

    xVALIDATE_L(1265, !(isgdata->idx_actvcard<0));
    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    rc = iSG_get_fmoddate(mcdata, fname, &sec, &min, &hr, &mon, &day, &yr);
    if (rc) {
        if (zMainIsPAL())
            sprintf(datestr, "%02d.%02d.%04d %02d:%02d:%02d", day, mon, yr, hr, min, sec);
        else
            sprintf(datestr, "%02d/%02d/%04d %02d:%02d:%02d", mon, day, yr, hr, min, sec);

        if (usr_sec) *usr_sec = sec;
        if (usr_min) *usr_min = min;
        if (usr_hr) *usr_hr = hr;
        if (usr_mon) *usr_mon = mon;
        if (usr_day) *usr_day = day;
        if (usr_yr) *usr_yr = yr;
    } else {
        sprintf(datestr, "<Unknown Modification>");
    }

    return datestr;
}

void iSGMakeTimeStamp(char* str)
{
    OSCalendarTime caltym = { 0 };

    xVALIDATE_L(1302, str);

    OSTicksToCalendarTime(OSGetTime(), &caltym);

    if (zMainIsPAL())
        sprintf(str, "%02d.%02d.%04d %02d:%02d:%02d", caltym.mday, caltym.mon + 1, caltym.year, caltym.hour,
                caltym.min, caltym.sec);
    else
        sprintf(str, "%02d/%02d/%04d %02d:%02d:%02d", caltym.mon + 1, caltym.mday, caltym.year, caltym.hour,
                caltym.min, caltym.sec);
}

#if DEBUG
DECOMP_FORCEACTIVE("%s(%d) : (fname) in '%s'\n", "%s(%d) : (didDel) in '%s'\n");
#endif

S32 iSGSelectGameDir(st_ISGSESSION* isgdata, const char*)
{
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 rc = 0;

    if (isgdata->idx_actvcard < 0) return 0;
    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    xVALIDATE_L(1415, mcdata->ismounted);
    if (!mcdata->ismounted) return 0;

    rc = iSG_mc_isGCcard(mcdata, NULL, NULL);
    xVALIDATE_L(1420, rc);
    if (!rc) return 0;

    rc = iSG_mc_isformatted(mcdata);
    if (!rc) return 0;

    S32 fileCount = 0;

    for (S32 i = 0; i < ISG_MAX_SAVEGAME; i++) {
        const char* fname = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i);
        if (iSG_get_finfo(mcdata, fname)) {
            fileCount++;
        }
    }
    if (!fileCount) {
        return 0;
    }

    return 1;
}

S32 iSGSetupGameDir(st_ISGSESSION*, const char*, S32)
{
    S32 result = 1;
    return result;
}

S32 iSGSaveFile(st_ISGSESSION* isgdata, const char* fname, char* data, S32 n, S32, char* label)
{
    S32 result = 0;
    S32 rc = 0;
    en_ASYNC_OPERR whyFail = ISG_OPERR_NONE;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    CARDStat dumystat = { 0 };
    CARDStat poststat = { 0 };
    char* fulbuf;
    S32 fulsiz = 0;
    char* bufptr;
    S32 bufsiz;
    char* runptr;
    S32 iconpart = 0;
    S32 sizemult = 0;

    ResetButton::DisableReset();

    xVALIDATE_L(1534, !(isgdata->idx_actvcard<0));
    if (isgdata->idx_actvcard < 0) {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_NOCARD;
        return result;
    }

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    if (mcdata->cardIsIll) {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_DAMAGE;
        return result;
    }

    iTRC_CHECK_L(1550);

    iconpart = iSG_cubeicon_size(isgdata->idx_actvcard, mcdata->sectsize);

    xVALIDATE_L(1581, mcdata->sectsize>0);
    sizemult = ISG_ROUNDUP(512, mcdata->sectsize);
    bufsiz = iconpart + ISG_ROUNDUP(n, sizemult);
    fulsiz = bufsiz + 31;

    fulbuf = (char*)xMEMPUSHTEMP_L(fulsiz, 'ISGT', 1587);
    xASSERT_L(1588, fulbuf);
    memset(fulbuf, 0, fulsiz);

    bufptr = (char*)OSRoundUp32B(fulbuf);

    runptr = iSG_bfr_icondata(bufptr, &dumystat, label, mcdata->sectsize);
    xVALIDATE_L(1597, runptr>=bufptr);
    xVALIDATE_L(1598, runptr<(bufptr+bufsiz));

    memcpy(runptr, data, n);

    iTRC_CHECK_L(1606);

    rc = iSG_mc_fopen(mcdata, fname, bufsiz, ISG_IOMODE_WRITE, &whyFail);
    if (rc) {
        rc = iSG_mc_fwrite(mcdata, bufptr, bufsiz);
        xVALIDATE_L(1620, rc);
        if (rc) result = 1;
        else {
            whyFail = ISG_OPERR_SVWRITE; result = 0;
        }

        if (result) {
            iSG_upd_icostat(&dumystat, &mcdata->cur_stat);

            iSG_timestamp(&mcdata->cur_stat);

            do {
                rc = CARDSetStatus(mcdata->myphys, mcdata->cur_finf.fileNo, &mcdata->cur_stat);
                ISG_TRACE("CARDSetStatus", rc);
            } while (rc == CARD_RESULT_BUSY);

            if (rc != CARD_RESULT_READY) {
                result = 0;
                DBprintf(DBML_WARN, "ISG: Write SetStatus failed - %s\n", iSGDB_tlate_cardres(rc));
            }
        }

        if (result) {
            U8 attrib = CARD_ATTR_PUBLIC;
            do {
                rc = CARDSetAttributes(mcdata->myphys, mcdata->cur_finf.fileNo, attrib);
                ISG_TRACE("CARDSetAttributes", rc);
            } while (rc == CARD_RESULT_BUSY);
            xVALIDATE_L(1677, rc == CARD_RESULT_READY);
            if (rc != CARD_RESULT_READY) {
                DBprintf(DBML_DBG, "ISG: File Attrib (hide) failed %s - %s\n", fname, iSGDB_tlate_cardres(rc));
            }
        }

        iSG_mc_fclose(mcdata, &poststat);
    }

    xMEMPOPTEMP(fulbuf);

    if (!result && !mcdata->cardIsIll) {
        iSG_mc_fdel(mcdata, fname);
    }

    ResetButton::EnableReset();

    iTRC_CHECK_L(1712);

    isgdata->as_operr = ISG_OPERR_NONE;
    if (result) isgdata->as_fakestat = ISG_OPSTAT_SUCCESS;
    else {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        if (whyFail) isgdata->as_operr = whyFail;
        else isgdata->as_operr = ISG_OPERR_UNKNOWN;
    }

    return result;
}

S32 iSGLoadFile(st_ISGSESSION* isgdata, const char* fname, char* databuf, S32 async)
{
    S32 result = 1;
    S32 size = 1;

    size = iSGFileSize(isgdata, fname);
    xVALIDATE_L(1743, size>0);
    result = iSGReadLeader(isgdata, fname, databuf, size, async);
    return result;
}

S32 iSGReadLeader(st_ISGSESSION* isgdata, const char* fname, char* databuf, S32 numbytes, S32)
{
    S32 result = 0;
    S32 rc = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 offset = 0;
    char* fulbuf = NULL;
    S32 fulsiz = 0;
    char* bufptr = NULL;
    S32 bufsiz = 0;
    S32 sizemult = 0;
    en_ASYNC_OPERR whyFail = ISG_OPERR_NONE;

    xVALIDATE_L(1780, !(isgdata->idx_actvcard<0));
    if (isgdata->idx_actvcard < 0) {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_NOCARD;
        return result;
    }

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    if (mcdata->cardIsIll) {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_DAMAGE;
        return result;
    }

    iTRC_CHECK_L(1796);

    offset = iSG_cubeicon_size(mcdata->myphys, mcdata->sectsize);

    sizemult = ISG_ROUNDUP(512, mcdata->sectsize);
    if ((S32)databuf % 32 || numbytes % sizemult) {
        DBprintf(DBML_VDBG, "ISG: Allocating temporary read buffer\n");

        fulsiz = ISG_ROUNDUP(numbytes, 512) + 31;
        xVALIDATE_L(1834, fulsiz>0);

        fulbuf = (char*)xMEMPUSHTEMP_L(fulsiz, 'ISGT', 1836);
        xASSERT_L(1837, fulbuf);
        memset(fulbuf, 0, fulsiz);

        bufptr = (char*)OSRoundUp32B(fulbuf);
        bufsiz = ISG_ROUNDUP(numbytes, 512);
    } else {
        DBprintf(DBML_DBG, "ISG: Natural read buffer size\n");
        bufptr = databuf;
        bufsiz = numbytes;
    }
    xASSERT_L(1849, bufptr);

    iTRC_CHECK_L(1852);

    rc = iSG_mc_fopen(mcdata, fname, -1, ISG_IOMODE_READ, &whyFail);
    xVALIDATE_L(1856, rc);
    if (rc) {
        rc = iSG_mc_fread(mcdata, bufptr, bufsiz, offset);
        if (rc) result = 1;
        else result = 0;

        iSG_mc_fclose(mcdata);
    }

    if (result) {
        if (fulbuf) {
            memcpy(databuf, bufptr, numbytes);
        }
    }

    if (fulbuf) {
        xVALIDATE_L(1881, fulsiz>0);
        xMEMPOPTEMP(fulbuf);
        fulbuf = NULL;
        bufsiz = 0;
    }

    isgdata->as_operr = ISG_OPERR_NONE;
    if (result) isgdata->as_fakestat = ISG_OPSTAT_SUCCESS;
    else {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        if (mcdata->cardIsIll) isgdata->as_operr = ISG_OPERR_DAMAGE;
        else isgdata->as_operr = ISG_OPERR_OTHER;
    }

    return result;
}

en_ASYNC_OPSTAT iSGPollStatus(st_ISGSESSION* isgdata, en_ASYNC_OPCODE*, S32)
{
    return isgdata->as_fakestat;
}

en_ASYNC_OPERR iSGOpError(st_ISGSESSION* isgdata, char* errbuf)
{
#if DEBUG
    static char* codenames[ISG_OPERR_NOMORE + 1] = {
        "ISG_OPERR_NONE",
        "ISG_OPERR_NOOPER",
        "ISG_OPERR_MULTIOPER",
        "ISG_OPERR_INITFAIL",
        "ISG_OPERR_GAMEDIR",
        "ISG_OPERR_NOCARD",
        "ISG_OPERR_NOROOM",
        "ISG_OPERR_DAMAGE",
        "ISG_OPERR_CORRUPT",
        "ISG_OPERR_OTHER",
        "ISG_OPERR_SVNOSPACE",
        "ISG_OPERR_SVINIT",
        "ISG_OPERR_SVWRITE",
        "ISG_OPERR_SVOPEN",
        "ISG_OPERR_LDINIT",
        "ISG_OPERR_LDREAD",
        "ISG_OPERR_LDOPEN",
        "ISG_OPERR_TGTERR",
        "ISG_OPERR_TGTREM",
        "ISG_OPERR_TGTPREP",
        "ISG_OPERR_UNKNOWN",
        NULL
    };
#endif
    static char* errmsgs[ISG_OPERR_NOMORE + 1] = {
        "No current error",
        "No operation in async queue",
        "Too many async ops queued simultaneously",
        "Init Failed",
        "Unable to access Save Game Directory",
        "Access Error - no card ?!? (eg yanked out)",
        "Access Error - no room on card (file handles free bytes, etc)",
        "Access Error - card is damaged or something bad",
        "Access Error - file being loaded appears to be corrupt (I-Level)",
        "Access Error - general problem",
        "Save Error - Not enough free space to save file",
        "Save Error - during initalization (async queue)",
        "Save Error - during write",
        "Save Error - opening file",
        "Load Error - during initalization (async queue)",
        "Load Error - during read",
        "Load Error - opening file",
        "Target problem (general error)",
        "Target Error - media removed or changed",
        "Target Error - Not ready for I/O (unformatted?)",
        "Operation encountered unknown error",
        NULL
    };

#if DEBUG
    DBprintf(DBML_DBG, "ISG: Async Error %s\n", codenames[isgdata->as_operr]);
#endif

    if (!errbuf) return isgdata->as_operr;

    if (isgdata->as_operr < ISG_OPERR_NOMORE) {
        strncpy(errbuf, errmsgs[isgdata->as_operr], 128);
    } else {
        strncpy(errbuf, errmsgs[ISG_OPERR_UNKNOWN], 128);
    }

    errbuf[127] = '\0';

    return isgdata->as_operr;
}

static S32 iSG_start_your_engines()
{
    S32 result = 1;

    CARDInit();
    DBprintf(DBML_VDBG, "ISG: CARD sub-system started\n");

    return result;
}

static S32 iSG_stop_your_engines(); // UNUSED

S32 iSG_mcidx2slot(S32 mcidx, S32* slot, S32* concnt)
{
    S32 result = 0;
    S32 rc = 0;
    S32 idx = 0;
    s32 duma = 0;
    s32 dumb = 0;
    S32 haveit[ISG_MAX_TARGET] = { 0 };
    S32 i = 0;

    *slot = -1;

    for (i = 0; i < ISG_MAX_TARGET; i++) {
        do {
            rc = CARDProbeEx(i, &duma, &dumb);
            ISG_TRACE("CARDProbeEx", rc);
        } while (rc == CARD_RESULT_BUSY);

        if (rc == CARD_RESULT_READY) haveit[i] = 1;
        else if (rc != CARD_RESULT_NOCARD) {
            DBprintf(DBML_WARN, "Probe on %d failed - %s\n", i, iSGDB_tlate_cardres(rc));
        }
    }

    for (i = 0; i < ISG_MAX_TARGET; i++) {
        if (!haveit[i]) continue;
        if (idx == mcidx) {
            *slot = i;
            result = 1;
            break;
        }
        idx++;
    }

    if (concnt) {
        for (i = 0; i < ISG_MAX_TARGET; i++) {
            concnt[i] = haveit[i];
        }
    }

    return result;
}

static S32 iSG_mc_exists(S32 cardslot)
{
    S32 result = 0;
    S32 rc = 0;
    s32 memsize = 0;
    s32 sectsize = 0;

    DBprintf(DBML_VDBG, "ISG: Checking card (slot) %d exists\n", cardslot);

    if (cardslot < -1) return result;

    do {
        rc = CARDProbeEx(cardslot, &memsize, &sectsize);
        ISG_TRACE("CARDProbeEx", rc);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;
    else {
        DBprintf(DBML_DBG, "ISG: Card slot %d not present - %s\n", cardslot, iSGDB_tlate_cardres(rc));
    }

    if (result) DBprintf(DBML_VDBG, "Detected MemCard at %d\n", cardslot);
    else DBprintf(DBML_VDBG, "No MemCard found at %d\n", cardslot);

    return result;
}

static S32 callbackResult;
static bool checkExInProgress;

void CheckExCallback(s32, s32 result)
{
    callbackResult = result; checkExInProgress = false;
}

inline S32 iSG_CARDCheckEx(S32 chan, s32* xferBytes)
{
    checkExInProgress = true;
    S32 rc = CARDCheckExAsync(chan, xferBytes, CheckExCallback);
    if (rc != CARD_RESULT_READY) {
        checkExInProgress = false;
        return rc;
    }

    while (checkExInProgress)
        iTRC_CHECK_L(2158);
    return callbackResult;
}

static S32 iSG_mc_tryRepair(st_ISG_MEMCARD_DATA* mcdata)
{
    S32 result = 0;
    S32 rc = 0;
    S32 sanity = 0;
    s32 repaired = 0;

    xVALIDATE_L(2176, mcdata->ismounted);
    if (!mcdata->ismounted) return result;

    if (mcdata->cardIsIll) return result;

    do {
        rc = iSG_CARDCheckEx(mcdata->myphys, &repaired);
        ISG_TRACE("CARDCheckExAsync", rc);
        xVALIDATE_L(2185, sanity<6000);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;
    else if (rc == CARD_RESULT_ENCODING) result = 1;
    else {
        DBprintf(DBML_DBG, "ISG: CardCheck Extended - slot %d fail - %s\n", mcdata->myphys,
                 iSGDB_tlate_cardres(rc));

        if (rc != CARD_RESULT_BROKEN) mcdata->cardIsIll = 1;

        result = 0;
    }

    if (repaired) {
        DBprintf(DBML_WARN, "ISG: Repaired %d bytes of MemCard slot %d (%s)\n", repaired, mcdata->myphys,
                 result ? "seems well now" : "still ill");
    } else {
        if (!result) {
            DBprintf(DBML_WARN, "ISG: Unable to repair MemCard slot %d\n", mcdata->myphys);
        }
    }

    return result;
}

static S32 iSG_mc_isformatted(st_ISG_MEMCARD_DATA* mcdata)
{
    S32 is_fmtd = 0;
    S32 rc = 0;
    S32 sanity = 0;
    s32 repaired = 0;

    xVALIDATE_L(2226, mcdata->ismounted);
    if (!mcdata->ismounted) return is_fmtd;

    if (mcdata->cardIsIll) return is_fmtd;

    do {
        rc = CARDCheckEx(mcdata->myphys, &repaired);
        ISG_TRACE("CARDCheckEx", rc);
        xVALIDATE_L(2235, sanity<6000);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) is_fmtd = 1;
    else if (rc == CARD_RESULT_BROKEN) is_fmtd = 0;
    else {
        is_fmtd = 0;
        DBprintf(DBML_DBG, "ISG: Card in phys slot %d not ready - %s\n", mcdata->myphys, iSGDB_tlate_cardres(rc));
    }

    if (repaired) {
        DBprintf(DBML_WARN, "ISG: Repaired %d bytes of MemCard phys %d\n", repaired, mcdata->myphys);
    }

    if (is_fmtd) DBprintf(DBML_VDBG, "isFMT? MemCard is formatted\n");
    else DBprintf(DBML_VDBG, "isFMT? MemCard is NOT formatted\n");

    return is_fmtd;
}

static S32 iSG_mc_isGCcard(st_ISG_MEMCARD_DATA* mcdata, S32* badencode, S32* wornout)
{
    S32 is_gc = 0;
    S32 rc = 0;
    S32 sanity = 0;
    s32 repaired = 0;
    u16 typ_enc = 0;
    s32 duma = 0;
    s32 dumb = 0;

    if (badencode) *badencode = 0;
    if (wornout) *wornout = 0;

    xVALIDATE_L(2272, mcdata->ismounted);
    if (!mcdata->ismounted) return is_gc;

    if (mcdata->cardIsIll) return is_gc;

    do {
        rc = CARDProbeEx(mcdata->myphys, &duma, &dumb);
        ISG_TRACE("CARDProbeEx", rc);
        xVALIDATE_L(2286, sanity<6000);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) is_gc = 1;
    else {
        DBprintf(DBML_DBG, "ISG: Card in slot %d not ready - %s\n", mcdata->myphys, iSGDB_tlate_cardres(rc));
    }

    if (is_gc) {
        sanity = 0;

        do {
            rc = CARDCheckEx(mcdata->myphys, &repaired);
            ISG_TRACE("CARDCheckEx", rc);
            xVALIDATE_L(2306, sanity<6000);
        } while (rc == CARD_RESULT_BUSY);

        if (repaired) {
            DBprintf(DBML_WARN, "ISG: Repaired %d bytes of MemCard phys %d\n", repaired, mcdata->myphys);
        }

        if (rc == CARD_RESULT_READY) is_gc = 1;
        else if (rc == CARD_RESULT_BROKEN) is_gc = 2;
        else if (rc == CARD_RESULT_ENCODING) {
            is_gc = 3;
            if (badencode) *badencode = 1;

            DBprintf(DBML_DBG, "ISG: MemCard phys %d - unsupported encoding\n", mcdata->myphys);
        } else {
            is_gc = 0;
            if (rc == CARD_RESULT_ENCODING && badencode) *badencode = 1;
            if (rc == CARD_RESULT_IOERROR && wornout) *wornout = 1;

            DBprintf(DBML_VDBG, "ISG: Card in slot %d not ready - %s\n", mcdata->myphys, iSGDB_tlate_cardres(rc));
        }
    }

    if (is_gc == 1) {
        do {
            rc = CARDGetEncoding(mcdata->myphys, &typ_enc);
            ISG_TRACE("CARDGetEncoding", rc);
        } while (rc == CARD_RESULT_BUSY);
        xVALIDATE_L(2351, (rc==CARD_RESULT_READY) &&"Really strange");
        if (rc == CARD_RESULT_READY) {
            u16 target_enc = CARD_ENCODE_ANSI;
            if (xSTGetLocalizationEnum() == eRegionJP) {
                target_enc = CARD_ENCODE_SJIS;
            }

            if (typ_enc != target_enc) {
                if (badencode) *badencode = 1;

                DBprintf(DBML_DBG, "ISG: MemCard phys %d using %s encoding\n", mcdata->myphys,
                         typ_enc == CARD_ENCODE_ANSI ? "ANSI" : typ_enc == CARD_ENCODE_SJIS ? "SJIS" : "Unknown");
            }
        } else {
            DBprintf(DBML_WARN, "ISG: Unable to determine encoding - %s\n", iSGDB_tlate_cardres(rc));
        }
    }

    if (is_gc) DBprintf(DBML_VDBG, "ISG: Device is supported MemCard\n");
    else DBprintf(DBML_VDBG, "ISG: Device not a supported MemCard\n");

    return is_gc;
}

static S32 iSG_isSpaceForFile(st_ISG_MEMCARD_DATA* mcdata, S32 fsize, const char* fname, S32* amtneed, S32* amtfree,
                              S32* needFiles)
{
    S32 haveroom = 0;
    S32 needamt = 0;
    S32 rc = 0;
    s32 freebyte = 0;
    s32 freefile = 0;

    xVALIDATE_L(2390, mcdata->ismounted);
    if (!mcdata->ismounted) {
        return haveroom;
    }

    needamt = iSG_cubeicon_size(mcdata->myphys, mcdata->sectsize);

    needamt += fsize;
    needamt = ISG_ROUNDUP(needamt, mcdata->sectsize);

    do {
        rc = CARDFreeBlocks(mcdata->myphys, &freebyte, &freefile);
        ISG_TRACE("CARDFreeBlocks", rc);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) {
        if (amtfree) {
            *amtfree = freebyte / mcdata->sectsize;
        }
        if (amtneed) {
            *amtneed = needamt / mcdata->sectsize;
        }
    } else {
        DBprintf(DBML_WARN, "ISG: Unable to determine free file count - %s\n", iSGDB_tlate_cardres(rc));
        return haveroom;
    }

    if (needFiles) *needFiles = 1;

    rc = iSG_get_finfo(mcdata, fname);
    if (rc) {
        if (needFiles) *needFiles = MAX(0, *needFiles - 1);

        xVALIDATE_L(2464, needamt<=mcdata->statcache.length);
        if (needamt <= mcdata->statcache.length + freebyte) {
            haveroom = 1;
        }
    } else {
        if (needamt <= freebyte && freefile > 0) {
            haveroom = 1;
        }
    }

    return haveroom;
}

static S32 iSG_mc_settgt(st_ISG_MEMCARD_DATA* mcdata, S32 cardslot)
{
    S32 rc = 0;

    xVALIDATE_L(2495, !mcdata->ismounted);

    rc = iSG_mc_mount(cardslot);
    if (rc) {
        mcdata->ismounted = 1;
        mcdata->myphys = cardslot;

        S32 trc = iSG_mc_tryRepair(mcdata);

        rc = CARDGetSectorSize(cardslot, (u32*)&mcdata->sectsize);
        ISG_TRACE("CARDGetSectorSize", rc);
        xVALIDATE_L(2520, rc == CARD_RESULT_READY);
        DBprintf(DBML_VDBG, "ISG: Mounted MemCard phys %d\n", cardslot);
    } else {
        mcdata->ismounted = 0;
    }

    return mcdata->ismounted ? 1 : 0;
}

static S32 iSG_find_nthfile(st_ISG_MEMCARD_DATA* mcdata, S32 nth, char* namebuf, S32* tot); // UNUSED

#if DEBUG
DECOMP_FORCEACTIVE("%s(%d) : (namebuf || tot) in '%s'\n");
#endif

static S32 iSG_get_fcnt(st_ISG_MEMCARD_DATA* mcdata); // UNUSED
static S32 iSG_fidx_fname(st_ISG_MEMCARD_DATA* mcdata, S32 fidx, char* namebuf); // UNUSED

static S32 iSG_get_finfo(st_ISG_MEMCARD_DATA* mcdata, const char* fname)
{
    S32 result = 0;
    S32 rc = 0;
    CARDStat* stat = NULL;
    en_ASYNC_OPERR whyFail = ISG_OPERR_NONE;

    rc = iSG_mc_fopen(mcdata, fname, -1, ISG_IOMODE_READ, &whyFail);
    if (rc) {
        result = 1;
        memcpy(&mcdata->statcache, &mcdata->cur_stat, sizeof(CARDStat));

        memcpy(&mcdata->finfcache, &mcdata->cur_finf, sizeof(CARDFileInfo));

        stat = &mcdata->statcache;
        DBprintf(DBML_VDBG, "ISG: MemCard File Stats:\n");
        DBprintf(DBML_VDBG, "\tGameName %.4s\n", stat->gameName);
        DBprintf(DBML_VDBG, "\tCompany  %.2s\n", stat->company);
        DBprintf(DBML_VDBG, "\tLength   %d\n", stat->length);
        DBprintf(DBML_VDBG, "\tFileName %.32s\n", stat->fileName);
        DBprintf(DBML_VDBG, "\tTime     %d\n", stat->time);

        iSG_mc_fclose(mcdata);
    }

    return result;
}

static S32 iSG_curKosher(CARDStat* stat, CARDFileInfo* finf)
{
    S32 isok = 1;

    xVALIDATE_L(2687, stat);
    xVALIDATE_L(2688, finf);

    if (!((stat->iconAddr >= 1 && stat->iconAddr <= 0x7FFFFFFF) ||
          (stat->commentAddr >= 1 && stat->commentAddr <= 0x7FFFFFFF))) {
        isok = 0;
    } else {
        S32 bs = 0;
        char* b = NULL;
        char* ba = NULL;
        s32 ik = 0;
        st_ISG_CARD_ICONBLOCK* blkmap = NULL;
        char tb[512] = { 0 };
        S32 rc, ak;

        sprintf(tb, "IncrediblesRoU");

        bs = sizeof(st_ISG_CARD_ICONBLOCK);
        bs = ISG_ROUNDUP(bs, 512);
        b = (char*)xMEMPUSHTEMP_L(bs + 31, 'ISGT', 2711);
        ba = (char*)OSRoundUp32B(b);

        do {
            ik = CARDRead(finf, ba, bs, 0);
            rc = ik;
            ISG_TRACE("CARDRead", rc);
        } while (ik == CARD_RESULT_BUSY);

        if (ik == CARD_RESULT_READY) {
            blkmap = (st_ISG_CARD_ICONBLOCK*)ba;
            ak = memcmp(blkmap->integchk, tb, strlen(tb) - 1);
            if (ak) {
                isok = 0;
            }

            U8 game_name[32] = { 0 };
            if (xSTGetLocalizationEnum() == eRegionFR)
                strcpy((char*)game_name, "Les Indestructibles : LTADD");
            else if (xSTGetLocalizationEnum() == eRegionDE)
                strcpy((char*)game_name, "Die Unglaublichen: DADT");
            else if (xSTGetLocalizationEnum() == eRegionJP) {
                U8 mister[32] = "Mr.";
                U8 incredibles_sjis[32] = "\x83\x43\x83\x93\x83\x4E\x83\x8C\x83\x66\x83\x42\x83\x75\x83\x8B";
                strtosjis(mister, game_name);
                strcpy((char*)&game_name[6], (char*)incredibles_sjis);
            } else
                strcpy((char*)game_name, "The Incredibles: RotU");

            U32 langConfig = xSTGetDiscLanguageConfig();
            if (langConfig == 0x110 || langConfig == 0x100100) {
                strcpy((char*)game_name, "Les Indestructibles : LTADD");
                ak = memcmp(blkmap->Title, game_name, strlen((char*)game_name) - 1);
                if (ak) {
                    if (langConfig == 0x110)
                        strcpy((char*)game_name, "Die Unglaublichen: DADT");
                    else
                        strcpy((char*)game_name, "The Incredibles: RotU");

                    ak = memcmp(blkmap->Title, game_name, strlen((char*)game_name) - 1);
                }
            } else {
                ak = memcmp(blkmap->Title, game_name, strlen((char*)game_name) - 1);
            }

            if (ak) {
                isok = 0;
            }
        } else {
            DBprintf(DBML_WARN, "ISG: IntegChk Failed '%s' - %s\n", stat->fileName, iSGDB_tlate_cardres(ik));
        }

        xMEMPOPTEMP(b);
    }

    if (!isok) {
        DBprintf(DBML_WARN, "ISG: '%s' appears to be corrupt!\n", stat->fileName);
    }

    return isok;
}

static S32 iSG_fileKosher(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32 tossRotten, S32* didit)
{
    S32 result = -1;
    S32 rc = 0;
    CARDStat* stat = NULL;
    CARDFileInfo* finfo = NULL;
    en_ASYNC_OPERR whyFail = ISG_OPERR_NONE;

    if (didit) *didit = 0;

    rc = iSG_mc_fopen(mcdata, fname, -1, ISG_IOMODE_READ, &whyFail);
    if (!rc) return result;

    rc = iSG_curKosher(&mcdata->cur_stat, &mcdata->cur_finf);

    iSG_mc_fclose(mcdata);

    if (!rc) {
        result = 0;
        if (tossRotten) {
            rc = iSG_mc_fdel(mcdata, fname);
            if (rc && didit) *didit = 1;
        }
    } else {
        result = 1;
    }

    return result;
}

static S32 iSG_get_fsize(st_ISG_MEMCARD_DATA* mcdata, const char* fname)
{
    S32 size = -1;
    S32 rc = 0;

    rc = iSG_get_finfo(mcdata, fname);
    if (rc) size = mcdata->statcache.length;
    if (size < 0) size = -1;

    return size;
}

static S32 iSG_get_fmoddate(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32* sec, S32* min, S32* hr, S32* mon,
                            S32* day, S32* yr)
{
    S32 result = 1;
    S32 rc = 0;
    const CARDStat* stat = NULL;
    OSCalendarTime caltym = { 0 };

    rc = iSG_get_finfo(mcdata, fname);
    if (!rc) result = 0;
    else {
        stat = &mcdata->statcache;
        S64 biggun = stat->time; biggun = OSSecondsToTicks(biggun);
        OSTicksToCalendarTime(biggun, &caltym);

        if (sec) *sec = caltym.sec;
        if (min) *min = caltym.min;
        if (hr) *hr = caltym.hour;
        if (mon) *mon = caltym.mon + 1;
        if (day) *day = caltym.mday;
        if (mon) *mon = caltym.mon + 1;
        if (day) *day = caltym.mday;
        if (yr) *yr = caltym.year;
    }

    return result;
}

static void iSG_timestamp(CARDStat*)
{
}

static S32 iSG_confirm_write(st_ISG_MEMCARD_DATA*, const char*); // UNUSED

static S32 iSG_cubeicon_size(S32 cardslot, S32 sectsize)
{
    S32 amt = 0;
    S32 jump = 0;
    S32 fixed = 0;

    xASSERT_L(2915, (cardslot==0)||(cardslot==1));
    if (!((cardslot == 0) || (cardslot == 1))) return -1;

    if (sectsize % 512) {
        DBprintf(DBML_WARN, "ISG: ** DANGER ** Sector size unfriendly multiple\n");
    }

    jump = ISG_ROUNDUP(512, sectsize); fixed = sizeof(st_ISG_CARD_ICONBLOCK);
    amt = ISG_ROUNDUP(fixed, jump);

    return amt;
}

static S32 iSG_chk_icondata()
{
    S32 result = 1;
#if DEBUG
    U32 chksum = 0;

    chksum = xUtil_crc_init();
    chksum = xUtil_crc_update(chksum, g_rawicon, g_iconsize);
    xASSERT_L(2953, g_chkicon == chksum);
    if (g_chkicon != chksum) result = 0;

    chksum = xUtil_crc_init();
    chksum = xUtil_crc_update(chksum, g_rawbanr, g_banrsize);
    xASSERT_L(2959, g_chkbanr == chksum);
    if (g_chkbanr != chksum) result = 0;
#endif

    return result;
}

static S32 iSG_load_icondata()
{
    S32 rc = 0;

    g_rawicon = (char*)iFileLoad("/ICON.TPL", NULL, (U32*)&g_iconsize, 0);
    xVALIDATE_L(2978, g_rawicon); xVALIDATE_L(2978, g_iconsize);

    g_rawbanr = (char*)iFileLoad("/BANNER.TPL", NULL, (U32*)&g_banrsize, 0);
    xVALIDATE_L(2981, g_rawbanr); xVALIDATE_L(2981, g_banrsize);

    xASSERT_L(2983, g_rawicon && g_iconsize && g_rawbanr && g_banrsize);

    rc = iSG_tpl_unpack((st_ISG_TPL_TEXPALETTE*)g_rawicon);
    xVALIDATE_L(2991, rc);

    rc = iSG_bnr_unpack((st_ISG_TPL_TEXPALETTE*)g_rawbanr);
    xVALIDATE_L(2994, rc);

#if DEBUG
    g_chkicon = xUtil_crc_init();
    g_chkicon = xUtil_crc_update(g_chkicon, g_rawicon, g_iconsize);
    xVALIDATE_L(3004, g_chkicon != xUtil_crc_init());

    g_chkbanr = xUtil_crc_init();
    g_chkbanr = xUtil_crc_update(g_chkbanr, g_rawbanr, g_banrsize);
    xVALIDATE_L(3008, g_chkbanr != xUtil_crc_init());
#endif

    return (g_rawicon && g_iconsize && g_rawbanr && g_banrsize) ? 1 : 0;
}

static void iSG_discard_icondata()
{
    DBprintf(DBML_WARN, "ISG: Releasing TPL data: assumes malloc/OSAlloc\n");
    OSFree(g_rawicon);
    OSFree(g_rawbanr);
    g_rawicon = NULL; g_iconsize = 0;
    g_rawbanr = NULL; g_banrsize = 0;
#if DEBUG
    g_chkicon = 0;
    g_chkbanr = 0;
#endif
}

static char* iSG_bfr_icondata(char* buffer, CARDStat*, char* label, S32 sectsize)
{
    st_ISG_CARD_ICONBLOCK block = { 0 };
    char* destptr = NULL;
    static st_ISG_TPL_TEXPALETTE* ico_pal;
    static st_ISG_TPL_TEXDESCRIP* ico_desc;
    S32 runsum = 0;
    static u32 i = 0;

    sprintf(block.integchk, "IncrediblesRoU");

    U8 game_name[32] = { 0 };
    if (xSTGetLocalizationEnum() == eRegionFR)
        strcpy((char*)game_name, "Les Indestructibles : LTADD");
    else if (xSTGetLocalizationEnum() == eRegionDE)
        strcpy((char*)game_name, "Die Unglaublichen: DADT");
    else if (xSTGetLocalizationEnum() == eRegionJP) {
        U8 mister[32] = "Mr.";
        U8 incredibles_sjis[32] = "\x83\x43\x83\x93\x83\x4E\x83\x8C\x83\x66\x83\x42\x83\x75\x83\x8B";
        strtosjis(mister, game_name);
        strcpy((char*)&game_name[6], (char*)incredibles_sjis);
    } else
        strcpy((char*)game_name, "The Incredibles: RotU");
    strncpy((char*)block.Title, (char*)game_name, sizeof(block.Title));

    block.Title[sizeof(block.Title) - 1] = '\0';
    if (label) strncpy((char*)block.Description, label, sizeof(block.Description));
    block.Description[sizeof(block.Description) - 1] = '\0';

    ico_pal = (st_ISG_TPL_TEXPALETTE*)g_rawbanr;

    ico_desc = iSG_tpl_TEXGet(ico_pal, 0);

    memcpy(block.Banner, ico_desc->textureHeader->data, sizeof(block.Banner));

    ico_pal = (st_ISG_TPL_TEXPALETTE*)g_rawicon;

    U32 texcount = CARD_ICON_MAX;
    if (texcount > ico_pal->numDescriptors)
        texcount = ico_pal->numDescriptors;
    for (i = 0; i < texcount; i++) {
        ico_desc = iSG_tpl_TEXGet(ico_pal, i);

        memcpy(block.Icons[i], ico_desc->textureHeader->data, sizeof(block.Icons[i]));
    }

    memcpy(buffer, &block, sizeof(block));

    S32 sizemult = 0;
    sizemult = ISG_ROUNDUP(512, sectsize);
    destptr = buffer + ISG_ROUNDUP(sizeof(block), sizemult);

    return destptr;
}

static void iSG_upd_icostat(CARDStat*, CARDStat* realstat)
{
    S32 i = 0;

    ISG_TRACE("CARD : Icon Setup", 0);

    CARDSetCommentAddress(realstat, 0);

    CARDSetBannerFormat(realstat, CARD_STAT_BANNER_RGB5A3);
    CARDSetIconAddress(realstat, CARD_COMMENT_SIZE);
    CARDSetIconAnim(realstat, CARD_STAT_ANIM_LOOP);

    for (i = 0; i < CARD_ICON_MAX; i++) {
        CARDSetIconFormat(realstat, i, CARD_STAT_ICON_RGB5A3);
        CARDSetIconSpeed(realstat, i, CARD_STAT_SPEED_MIDDLE);
    }
}

static S32 iSG_tpl_unpack(st_ISG_TPL_TEXPALETTE* pal)
{
    S32 i = 0;
    S32 rc = 0;
    st_ISG_TPL_TEXDESCRIP* tmpdesc = NULL;
    u32 palpos = (u32)pal;
    u32 tmppos = 0;

    xVALIDATE_L(3200, pal);
    xVALIDATE_L(3201, pal->versionNumber == 2142000);
    if (pal->versionNumber != 2142000) return 0;

    tmppos = palpos + (u32)pal->descriptorArray;
    pal->descriptorArray = (st_ISG_TPL_TEXDESCRIP*)tmppos;

    for (i = 0; i < pal->numDescriptors; i++) {
        tmpdesc = &pal->descriptorArray[i];

        if (tmpdesc->textureHeader) {
            tmppos = palpos + (u32)tmpdesc->textureHeader;
            tmpdesc->textureHeader = (st_ISG_TPL_TEXHEADER*)tmppos;

            if (!tmpdesc->textureHeader->unpacked) {
                tmppos = palpos + (u32)tmpdesc->textureHeader->data;
                tmpdesc->textureHeader->data = (char*)tmppos;
                tmpdesc->textureHeader->unpacked = 1;
            }
        }

        if (tmpdesc->CLUTHeader) {
            tmppos = palpos + (u32)tmpdesc->CLUTHeader;
            tmpdesc->CLUTHeader = (st_ISG_TPL_CLUTHEADER*)tmppos;

            if (!tmpdesc->CLUTHeader->unpacked) {
                tmppos = palpos;
                tmpdesc->CLUTHeader->data = (char*)palpos;
                tmpdesc->CLUTHeader->unpacked = 1;
            }
        }
    }

    return 1;
}

static S32 iSG_bnr_unpack(st_ISG_TPL_TEXPALETTE* pal)
{
    DBprintf(DBML_DBG, "ISG: Using TPL version of banner\n");
    return iSG_tpl_unpack(pal);
}

static st_ISG_TPL_TEXDESCRIP* iSG_tpl_TEXGet(st_ISG_TPL_TEXPALETTE* pal, U32 idx)
{
    xASSERT_L(3269, idx < pal->numDescriptors);

    return &pal->descriptorArray[idx];
}

static S32 iSG_mc_mount(S32 cardslot)
{
    S32 result = 0;
    volatile S32 rc = 0;

    do {
        rc = CARDMount(cardslot, cardwork[cardslot], iSG_cb_unmount);
        isMounted = true;
        ISG_TRACE("CARDMount", rc);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;
    else if (rc == CARD_RESULT_BROKEN || rc == CARD_RESULT_ENCODING) {
        result = 1;
        DBprintf(DBML_WARN, "ISG: mount success except - %s\n", iSGDB_tlate_cardres(rc));
    } else {
        DBprintf(DBML_WARN, "ISG: mount failed - %s\n", iSGDB_tlate_cardres(rc));
    }

    return result;
}

static S32 iSG_mc_unmount(S32 cardslot)
{
    S32 result = 0;
    S32 rc = 0;
    S32 sanity = 0;

    do {
        rc = CARDUnmount(cardslot);
        ISG_TRACE("CARDUnmount", rc);
        xVALIDATE_L(3320, sanity++ < 6000);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;
    else if (rc == CARD_RESULT_NOCARD) result = 1;
    else {
        DBprintf(DBML_WARN, "ISG: UnMount cmd result - %s\n", iSGDB_tlate_cardres(rc));
    }

    return result;
}

static void iSG_cb_unmount(s32 chan, s32 retcode)
{
    st_ISGSESSION* isgdata = &g_isgdata_MAIN;

    xASSERT_L(3341, chan == isgdata->idx_actvcard);

    DBprintf(DBML_DBG, "ISG: UnMount notification - %s\n", iSGDB_tlate_cardres(retcode));

    if (chan == isgdata->idx_actvcard) {
        isgdata->idx_actvcard = -1;
        if (isgdata->chgfunc) {
            isgdata->chgfunc(isgdata->cltdata, ISG_CHG_TARGET);
        }
        memset(&isgdata->physcard[chan], 0, sizeof(st_ISG_MEMCARD_DATA));
    }
}

static S32 iSG_mc_format(st_ISG_MEMCARD_DATA* mcdata, S32 force, S32* canRecover)
{
    S32 result = 0;
    S32 rc = 0;

    xVALIDATE_L(3373, !(mcdata->cardIsIll));
    if (mcdata->cardIsIll) {
        if (canRecover) *canRecover = 0;
        return result;
    }
    if (canRecover) *canRecover = 1;

#if DEBUG
    rc = iSG_mc_isformatted(mcdata);
    if (rc && !force) {
        DBprintf(DBML_WARN, "WARNING: Caller asked to format a formatted MemCard.\n"
                            "         Unwilling to comply with format request.\n"
                            "\t      ---- skipping format operation ----\n"
                            "         If you really wish to force the format call\n"
                            "         with force enabled.\n");
    }
#endif

    do {
        rc = CARDFormat(mcdata->myphys);
        ISG_TRACE("CARDFormat", rc);
    } while (rc == CARD_RESULT_BUSY);
    if (rc == CARD_RESULT_READY) result = 1;
    else {
        result = 0;
        DBprintf(DBML_DBG, "ISG: Format attempt phys %d failed - %s\n", mcdata->myphys, iSGDB_tlate_cardres(rc));

        switch (rc) {
        case CARD_RESULT_IOERROR:
            mcdata->cardIsIll = 1;
            if (canRecover) {
                *canRecover = 0;
            }
            break;

        case CARD_RESULT_NOCARD:
            if (canRecover) {
                *canRecover = -1;
            }
            result = -1;
            break;

        case CARD_RESULT_NOFILE:
        case CARD_RESULT_FATAL_ERROR: rc = 0; break;
        case CARD_RESULT_READY: case CARD_RESULT_BUSY: break;
        }
    }

    if (!result) DBprintf(DBML_WARN, "Formatting Failed\n");
    else DBprintf(DBML_VDBG, "Format Operation Completed OK\n");

    return result;
}

static bool cardCreateInProgress;

void CardCreateCallback(s32, s32 result)
{
    callbackResult = result; cardCreateInProgress = false;
}

inline S32 iSG_CARDCreate(s32 chan, const char* fileName, u32 size, CARDFileInfo* fileInfo)
{
    cardCreateInProgress = true;
    S32 rc = CARDCreateAsync(chan, fileName, size, fileInfo, CardCreateCallback);
    if (rc != CARD_RESULT_READY) {
        cardCreateInProgress = false;
        return rc;
    }

    while (cardCreateInProgress)
        iTRC_CHECK_L(3463);
    return callbackResult;
}

static S32 iSG_mc_fopen(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32 fsize, en_ISG_IOMODE mode,
                        en_ASYNC_OPERR* whyFail)
{
    S32 result = 0;
    S32 rc = 0;
    CARDFileInfo* finf = &mcdata->cur_finf;
    CARDStat* stat = &mcdata->cur_stat;

    xVALIDATE_L(3507, fname);

    if (whyFail) *whyFail = ISG_OPERR_NONE;

    xASSERT_L(3511, mcdata->ismounted);
    if (mode == ISG_IOMODE_READ) {
        do {
            rc = CARDOpen(mcdata->myphys, fname, finf);
            ISG_TRACE("CARDOpen", rc);
        } while (rc == CARD_RESULT_BUSY);

        if (rc == CARD_RESULT_READY) {
            result = 1;
        } else {
            DBprintf(rc == CARD_RESULT_NOFILE ? DBML_VDBG : DBML_DBG, "ISG: Open for Read Failed '%s' - %s\n", fname,
                     iSGDB_tlate_cardres(rc));
        }
    } else {
        xVALIDATE_L(3536, mode==ISG_IOMODE_WRITE);

        rc = iSG_mc_fdel(mcdata, fname);
        xVALIDATE_L(3541, rc);

        if (!mcdata->cardIsIll && rc > 0) {
            do {
                rc = iSG_CARDCreate(mcdata->myphys, fname, fsize, finf);
                ISG_TRACE("CARDCreate", rc);
            } while (rc == CARD_RESULT_BUSY);

            if (rc == CARD_RESULT_READY) {
                mcdata->off_nextwrite = 0; result = 1;
            } else {
                DBprintf(DBML_WARN, "ISG: Open for Write Failed '%s' - %s\n", fname, iSGDB_tlate_cardres(rc));

                if (whyFail) {
                    switch (rc) {
                    case CARD_RESULT_READY:
                    case CARD_RESULT_BUSY:
                        break;
                    case CARD_RESULT_WRONGDEVICE:
                    case CARD_RESULT_NOCARD:
                        *whyFail = ISG_OPERR_NOCARD;
                        break;
                    case CARD_RESULT_IOERROR:
                    case CARD_RESULT_BROKEN:
                        mcdata->cardIsIll = 1;
                        *whyFail = ISG_OPERR_DAMAGE;
                        break;
                    case CARD_RESULT_EXIST:
                    case CARD_RESULT_NOPERM:
                        break;
                    case CARD_RESULT_NOENT:
                    case CARD_RESULT_INSSPACE:
                        *whyFail = ISG_OPERR_NOROOM;
                        break;
                    default:
                        *whyFail = ISG_OPERR_OTHER;
                        break;
                    }
                }
            }
        }
    }

    if (mcdata->cardIsIll) {
        xVALIDATE_L(3597, result==0);
    }

    if (result) {
        iSGDB_dump_curfinf(finf, mode == ISG_IOMODE_READ);

        do {
            rc = CARDGetStatus(mcdata->myphys, finf->fileNo, stat);
            ISG_TRACE("CARDGetStatus", rc);
        } while (rc == CARD_RESULT_BUSY);

        if (rc == CARD_RESULT_READY) {
            result = 1;
            iSGDB_dump_curstat(stat, mode == ISG_IOMODE_READ);
        } else {
            DBprintf(DBML_WARN, "ISG: Get Status failed %s - %s\n", fname, iSGDB_tlate_cardres(rc));
        }
    }

    U32 langConfig = xSTGetDiscLanguageConfig();
    switch (langConfig) {
    case 0x1:
        ISG_GC_GAMENAME = "GIQE";
        break;
    case 0x100100:
        ISG_GC_GAMENAME = "GIQX";
        break;
    case 0x110:
        ISG_GC_GAMENAME = "GIQY";
        break;
    case 0x400:
        ISG_GC_GAMENAME = "GIQE";
        break;
    }

    finf = &mcdata->cur_finf;
    stat = &mcdata->cur_stat;
    stat = NULL;

#if DEBUG
    if (result && mode == ISG_IOMODE_READ) {
        S32 isok = iSG_curKosher(&mcdata->cur_stat, &mcdata->cur_finf);
        xVALIDATE_L(3683, isok);
        if (!isok && whyFail) *whyFail = ISG_OPERR_CORRUPT;
    }
#endif

    if (!result) DBprintf(DBML_VDBG, "ISG: Open Failed '%s'\n", fname);
    else DBprintf(DBML_VDBG, "ISG: Open OK '%s'\n", fname);

    return result;
}

static S32 iSG_mc_fclose(st_ISG_MEMCARD_DATA* mcdata)
{
    return iSG_mc_fclose(mcdata, NULL);
}

static S32 iSG_mc_fclose(st_ISG_MEMCARD_DATA* mcdata, CARDStat* statcopy)
{
    S32 result = 0;
    S32 rc = 0;

    do {
        rc = CARDClose(&mcdata->cur_finf);
        ISG_TRACE("CARDClose", rc);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;
    else {
        DBprintf(DBML_WARN, "ISG: FileClose Failed '%s' - %s\n", mcdata->cur_stat.fileName, iSGDB_tlate_cardres(rc));
    }

    if (statcopy) memcpy(statcopy, &mcdata->cur_stat, sizeof(CARDStat));

    memset(&mcdata->cur_finf, 0, sizeof(CARDFileInfo));
    memset(&mcdata->cur_stat, 0, sizeof(CARDStat));

    if (!result) DBprintf(DBML_VDBG, "ISG: Close Failed\n");
    else DBprintf(DBML_VDBG, "ISG: Close OK\n");

    return result;
}

static bool cardDeleteInProgress;

void CardDeleteCallback(s32, s32 result)
{
    callbackResult = result; cardDeleteInProgress = false;
}

inline S32 iSG_CARDDelete(s32 chan, const char* fileName)
{
    cardDeleteInProgress = true;
    S32 rc = CARDDeleteAsync(chan, fileName, CardDeleteCallback);
    if (rc != CARD_RESULT_READY) {
        cardDeleteInProgress = false;
        return rc;
    }

    while (cardDeleteInProgress)
        iTRC_CHECK_L(3763);
    return callbackResult;
}

static S32 iSG_mc_fdel(st_ISG_MEMCARD_DATA* mcdata, const char* fname)
{
    S32 result = 0;
    S32 rc = 0;
    st_ISGSESSION* isgdata;

    if (mcdata->myphys < 0 || mcdata->myphys > 1) {
        return 0;
    }

    isgdata = &g_isgdata_MAIN;
    if (!isMounted) {
        return 0;
    }

    do {
        rc = iSG_CARDDelete(mcdata->myphys, fname);
        ISG_TRACE("CARDDelete", rc);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_NOFILE) result = 1;
    else if (rc == CARD_RESULT_READY) result = 1;
    else {
        result = 0;
        DBprintf(DBML_WARN, "ISG: Delete Failed '%s' - %s\n", fname, iSGDB_tlate_cardres(rc));

        en_ASYNC_OPERR whyFail;
        switch (rc) {
        case CARD_RESULT_NOFILE:
            whyFail = ISG_OPERR_NOCARD;
        case CARD_RESULT_NOCARD:
            whyFail = ISG_OPERR_NOCARD;
        case CARD_RESULT_NOPERM:
            whyFail = ISG_OPERR_OTHER;
            break;

        case CARD_RESULT_IOERROR:
            mcdata->cardIsIll = 1;
            whyFail = ISG_OPERR_DAMAGE;
            break;
        case CARD_RESULT_FATAL_ERROR:
            DBprintf(DBML_WARN, "ISG: Programmer Error ...\n");
            whyFail = ISG_OPERR_OTHER;
            break;
        case CARD_RESULT_READY:
        case CARD_RESULT_BUSY:
            whyFail = ISG_OPERR_NONE;
            break;
        }
    }

    if (!result) DBprintf(DBML_VDBG, "Delete Failed '%s'\n", fname);
    else DBprintf(DBML_VDBG, "Delete Operation Completed OK '%s'\n", fname);

    return result;
}

static S32 iSG_mc_frename(st_ISG_MEMCARD_DATA* mcdata, const char* oldname, const char* newname); // UNUSED

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("CARDRename", "ISG: Rename Failed '%s' -> '%s' - %s\n", "Rename OK ('%s' --> '%s')\n");
#endif

static void iSG_cb_asyndone(s32, s32 retcode)
{
    DBprintf(DBML_VDBG, "ISG: Async Op finished - %s\n", iSGDB_tlate_cardres(retcode));
}

static S32 iSG_mc_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize); // UNUSED

static S32 iSG_mc_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize, S32 skipbyte)
{
    S32 result = 1;
    S32 rc = 0;
    en_ASYNC_OPERR whyFail;

    return iSG_mcqa_fread(mcdata, buf, bufsize, skipbyte);

    DECOMP_FORCEREGISTER(rc);
    DECOMP_FORCEREGISTER(rc);
    DECOMP_FORCEREGISTER(rc);
    (void)rc;
    DECOMP_FORCEREGISTER(result);
    DECOMP_FORCEREGISTER(result);
    DECOMP_FORCEREGISTER(result);
    DECOMP_FORCEREGISTER(whyFail);
    DECOMP_FORCEREGISTER(whyFail);
    DECOMP_FORCEREGISTER(whyFail);
    DECOMP_FORCEREGISTER(mcdata);
    DECOMP_FORCEREGISTER(mcdata);
    DECOMP_FORCEREGISTER(skipbyte);
    (void)skipbyte;
    DECOMP_FORCEREGISTER(bufsize);
    (void)buf;
}

static S32 iSG_mcqa_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize, S32 skipbyte)
{
    S32 result = 0;
    S32 rc = 0;
    S32 amt_start = 0;
    S32 amt = 0;
    S32 updintv = 1000;
    S32 polltillready = 1;

    if (polltillready) {
        CARDGetResultCode(mcdata->myphys);
        do {
            rc = CARDGetResultCode(mcdata->myphys);
        } while (rc == CARD_RESULT_BUSY);
        if (rc != CARD_RESULT_READY) {
            DBprintf(DBML_WARN, "ISG: Wait for green light failed - %s\n", iSGDB_tlate_cardres(rc));
            return result;
        }
    }

    amt_start = CARDGetXferredBytes(mcdata->myphys);
    ISG_TRACE("CARDGetXferredBytes", rc);

    do {
        rc = CARDReadAsync(&mcdata->cur_finf, buf, bufsize, skipbyte, iSG_cb_asyndone);
        ISG_TRACE("CARDReadAsync", rc);
    } while (rc == CARD_RESULT_BUSY);
    xVALIDATE_L(3983, rc == CARD_RESULT_READY);

    if (rc != CARD_RESULT_READY) {
        DBprintf(DBML_WARN, "ISG: Async Read queue failed - %s\n", iSGDB_tlate_cardres(rc));
        return result;
    }

    rc = CARD_RESULT_BUSY;

    do {
        if (updintv++ > 500) {
            DBprintf(DBML_VDBG, "ISG: updating sound system\n");
            xSndMgrUpdate(0.0f);

            amt = CARDGetXferredBytes(mcdata->myphys);
            ISG_TRACE("CARDGetXferredBytes", rc);

            DBprintf(DBML_VDBG, "ISG: Read %d of %d [system: 0?] [prior: %d]\n", amt - amt_start, bufsize, skipbyte);

            rc = CARDGetResultCode(mcdata->myphys);

            updintv = 0;
        }

        iTRC_CHECK_L(4018);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;
    else {
        result = 0;
        DBprintf(DBML_WARN, "ISG: Async Read op failed - %s\n", iSGDB_tlate_cardres(rc));

        en_ASYNC_OPERR whyFail;
        switch (rc) {
        case CARD_RESULT_NOPERM:
        case CARD_RESULT_CANCELED:
        case CARD_RESULT_NOFILE: whyFail = ISG_OPERR_LDREAD;
        case CARD_RESULT_NOCARD: whyFail = ISG_OPERR_NOCARD;
            break;

        case CARD_RESULT_LIMIT:
        case CARD_RESULT_FATAL_ERROR:
            DBprintf(DBML_WARN, "ISG: Programmer Error ...\n");
            break;

        case CARD_RESULT_READY:
        case CARD_RESULT_BUSY:
            whyFail = ISG_OPERR_NONE;
            break;
        }
    }

    return result;
}

static S32 iSG_mc_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* data, S32 n)
{
    S32 result = 0;
    S32 rc = 0;
    CARDFileInfo* finf = &mcdata->cur_finf;
    CARDStat* stat = &mcdata->cur_stat;

    return iSG_mcqa_fwrite(mcdata, data, n);

    DECOMP_FORCEREGISTER(mcdata);
    DECOMP_FORCEREGISTER(mcdata);
    (void)mcdata;
    DECOMP_FORCEREGISTER(rc);
    DECOMP_FORCEREGISTER(rc);
    DECOMP_FORCEREGISTER(rc);
    DECOMP_FORCEREGISTER(result);
    DECOMP_FORCEREGISTER(result);
    (void)result;
    DECOMP_FORCEREGISTER(finf);
    DECOMP_FORCEREGISTER(finf);
    DECOMP_FORCEREGISTER(n);
    (void)n;
    DECOMP_FORCEREGISTER(stat);
    (void)data;
}

static S32 iSG_mcqa_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* data, S32 n)
{
    S32 result = 0;
    S32 rc = 0;
    S32 updintv = 1000;
    S32 amt_start = 0;
    S32 amt = 0;
    S32 asynpoll = 0;
    CARDFileInfo* finf = &mcdata->cur_finf;
    CARDStat* stat = &mcdata->cur_stat;
    S32 polltillready = 1;

    xVALIDATE_L(4113, (mcdata->off_nextwrite==0)&&"this is a warning unless testing");

    if (polltillready) {
        do {
            rc = CARDGetResultCode(mcdata->myphys);
            ISG_TRACE("CARDGetResultCode(discard", rc);
        } while (rc == CARD_RESULT_BUSY);
        if (rc != CARD_RESULT_READY) {
            DBprintf(DBML_WARN, "ISG: Wait for green light failed - %s\n", iSGDB_tlate_cardres(rc));
            return result;
        }
    }

    amt_start = CARDGetXferredBytes(mcdata->myphys);
    ISG_TRACE("CARDGetXferredBytes(start)", amt_start);

    do {
        rc = CARDWriteAsync(&mcdata->cur_finf, data, n, mcdata->off_nextwrite, iSG_cb_asyndone);
        ISG_TRACE("CARDWriteAsync", rc);
    } while (rc == CARD_RESULT_BUSY);

    if (rc != CARD_RESULT_READY) {
        DBprintf(DBML_WARN, "ISG: Async Write queue failed - %s\n", iSGDB_tlate_cardres(rc));
        return result;
    }

    asynpoll = CARD_RESULT_BUSY;

    do {
        if (updintv++ > 500) {
            asynpoll = CARDGetResultCode(mcdata->myphys);
            ISG_TRACE("CARDGetResultCode", asynpoll);

            DBprintf(DBML_VDBG, "ISG: updating sound system\n");
            xSndMgrUpdate(0.0f);

            amt = CARDGetXferredBytes(mcdata->myphys);
            ISG_TRACE("CARDGetXferredBytes", amt - amt_start);

            DBprintf(DBML_VDBG, "ISG: Saved %d of %d [system: %d] [prior: %d]\n", amt - amt_start, n + 8192, 8192,
                     mcdata->off_nextwrite);

            updintv = 0;
        }

        iTRC_CHECK_L(4178);
    } while (asynpoll == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) {
        mcdata->off_nextwrite += n;

        do {
            rc = CARDGetStatus(mcdata->myphys, finf->fileNo, stat);
            ISG_TRACE("CARDGetStatus", rc);
        } while (rc == CARD_RESULT_BUSY);
        if (rc == CARD_RESULT_READY) result = 1;
        else {
            DBprintf(DBML_WARN, "ISG: Async Write op failed - %s\n", iSGDB_tlate_cardres(rc));
        }
    }

    if (!result) DBprintf(DBML_WARN, "ISG: Async Write Failed\n");
    else DBprintf(DBML_VDBG, "ISG: Write Operation Completed OK\n");

    return result;
}

static S32 iSG_is_MCOP_realerr(S32 mcop, S32 que_rc); // UNUSED

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("Operation '%s' Queued Successfully\n", "Operation '%s' Failed - reason unkown (%d)\n",
                   "Operation '%s' Failed -  '%s' is in progress\n", "MemCard: Unknown SCE MCOP code (%d)\n");
#endif

S32 iSGCheckForWrongDevice()
{
    char* workArea = (char*)RwMallocL(CARD_WORKAREA_SIZE, rwMEMHINTDUR_NADURATION, 4503);
    S32 resultCode;

    S32 badCard = -1;

    for (S32 i = 0; i < ISG_MAX_TARGET; i++) {
        resultCode = CARDMount(i, workArea, NULL);
        switch (resultCode) {
        case CARD_RESULT_READY:
        case CARD_RESULT_BROKEN:
        case CARD_RESULT_ENCODING:
            CARDUnmount(i);
            break;

        case CARD_RESULT_WRONGDEVICE:
            badCard = i;

            i = ISG_MAX_TARGET;
            break;
        }
    }
    RwFree(workArea);

    return badCard;
}

bool iSGIsGameCorrupt(st_ISGSESSION* sess, S32 gidx)
{
    S32 result = 1;
    S32 rc = 0;
    const char* fname = NULL;
    st_ISG_MEMCARD_DATA* mcdata = NULL;

    xVALIDATE_L(4576, !(sess->idx_actvcard<0));
    if (sess->idx_actvcard < 0) {
        return 0;
    }

    mcdata = &sess->physcard[sess->idx_actvcard];

    fname = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, gidx);
    xVALIDATE_L(4586, fname);

    rc = iSG_fileKosher(mcdata, fname, 0, NULL);
    if (rc == 0) {
        return true;
    }
    return false;
}

bool iSGCheckMemoryCard(st_ISGSESSION*, S32 index)
{
    s32 memSize, sectorSize;

    S32 resultCode = CARDProbeEx(index, &memSize, &sectorSize);
    switch (resultCode) {
    case CARD_RESULT_READY:
        return true;

    default:
        return false;
    }
}

void iSGAutoSave_Startup()
{
}

st_ISGSESSION* iSGAutoSave_Connect(S32 idx_target, void* cltdata, ISGChangeCallback chg)
{
    st_ISGSESSION* isg = iSGSessionBegin(cltdata, chg, 1);
    if (!isg) {
        return isg;
    }

    S32 rc = iSGTgtSetActive(isg, idx_target);
    xASSERT_L(4634, rc);
    if (!rc) {
        iSGSessionEnd(isg);
        isg = NULL;
    }

    return isg;
}

void iSGAutoSave_Disconnect(st_ISGSESSION* isg)
{
    iSGSessionEnd(isg);
}

S32 iSGAutoSave_Monitor(st_ISGSESSION* isg, S32 idx_target)
{
    if (!isg) return 0;

    U32 stat = iSGTgtState(isg, idx_target, NULL);
    if (!stat || !(stat & 0x1)) {
        globals.autoSaveFeature = false;
        return 0;
    }

    return 1;
}

#if DEBUG || RELEASE
static char* iSGDB_tlate_cardres(S32 errid)
{
    static st_ERROR_LOOKUP errlook[17] = {
        { CARD_RESULT_READY, "Ready to start the next operation." },
        { CARD_RESULT_BUSY, "Busy." },
        { CARD_RESULT_WRONGDEVICE, "A device is detected, but it is not a memory card." },
        { CARD_RESULT_NOCARD, "Memory card is not detected (or not mounted yet)." },
        { CARD_RESULT_NOFILE, "Specified file was not found." },
        { CARD_RESULT_IOERROR, "Memory card has reached limit of useable life." },
        { CARD_RESULT_BROKEN, "File system is broken." },
        { CARD_RESULT_EXIST, "The filename about to be created/renamed already exists." },
        { CARD_RESULT_NOENT, "No more free directory entries." },
        { CARD_RESULT_INSSPACE, "Insufficient free space in data blocks." },
        { CARD_RESULT_NOPERM, "No file access permission." },
        { CARD_RESULT_LIMIT, "Tried to read/write over the file size limit." },
        { CARD_RESULT_NAMETOOLONG, "The filename about to be created/renamed is too long." },
        { CARD_RESULT_ENCODING, "Character set encoding is mismatched." },
        { CARD_RESULT_CANCELED, "The read/write operation is canceled." },
        { CARD_RESULT_FATAL_ERROR, "Error due to program design (e.g., parameter range error, etc.)." },
        { 0, NULL }
    };
    char* da_errmsg = NULL;
    st_ERROR_LOOKUP* tmprec = NULL;

    tmprec = errlook;
    while (tmprec->errmsg) {
        if (tmprec->errid == errid) {
            da_errmsg = tmprec->errmsg;
            break;
        }
        tmprec++;
    }

    if (!da_errmsg) {
        da_errmsg = "<ERROR:ERRIDOutOfRange>";
        DBprintf(DBML_WARN, "ISG: Unknown err message translation (%d)\n", errid);
    }

    return da_errmsg;
}
#endif

#if DEBUG || RELEASE
static char* iSGDB_tlate_mcopid(S32 id); // UNUSED

DECOMP_FORCEACTIVE("ISG_EXEC_NONE", "ISG_EXEC_CARDCancel", "ISG_EXEC_CARDCheck", "ISG_EXEC_CARDCheckAsync",
                   "ISG_EXEC_CARDCheckEx", "ISG_EXEC_CARDCheckExAsync", "ISG_EXEC_CARDClose", "ISG_EXEC_CARDCreate",
                   "ISG_EXEC_CARDCreateAsync", "ISG_EXEC_CARDDelete", "ISG_EXEC_CARDDeleteAsync",
                   "ISG_EXEC_CARDFastDelete", "ISG_EXEC_CARDFastDeleteAsync", "ISG_EXEC_CARDFastOpen",
                   "ISG_EXEC_CARDFormat", "ISG_EXEC_CARDFormatAsync", "ISG_EXEC_CARDFreeBlocks",
                   "ISG_EXEC_CARDGetAttributes", "ISG_EXEC_CARDGetBannerFormat", "ISG_EXEC_CARDGetEncoding",
                   "ISG_EXEC_CARDGetFileNo", "ISG_EXEC_CARDGetIconAnim", "ISG_EXEC_CARDGetIconFormat",
                   "ISG_EXEC_CARDGetIconSpeed", "ISG_EXEC_CARDGetMemSize", "ISG_EXEC_CARDGetResultCode",
                   "ISG_EXEC_CARDGetSectorSize", "ISG_EXEC_CARDGetSerialNo", "ISG_EXEC_CARDGetStatus",
                   "ISG_EXEC_CARDGetXferredBytes", "ISG_EXEC_CARDInit", "ISG_EXEC_CARDMount",
                   "ISG_EXEC_CARDMountAsync", "ISG_EXEC_CARDOpen", "ISG_EXEC_CARDProbe", "ISG_EXEC_CARDProbeEx",
                   "ISG_EXEC_CARDRead", "ISG_EXEC_CARDReadAsync", "ISG_EXEC_CARDRename", "ISG_EXEC_CARDRenameAsync",
                   "ISG_EXEC_CARDSetAttributes", "ISG_EXEC_CARDSetAttributesAsync ", "ISG_EXEC_CARDSetBannerFormat",
                   "ISG_EXEC_CARDSetCommentAddress", "ISG_EXEC_CARDSetIconAddress", "ISG_EXEC_CARDSetIconAnim",
                   "ISG_EXEC_CARDSetIconFormat", "ISG_EXEC_CARDSetIconSpeed", "ISG_EXEC_CARDSetStatus",
                   "ISG_EXEC_CARDSetStatusAsync", "ISG_EXEC_CARDUnmount", "ISG_EXEC_CARDWrite",
                   "ISG_EXEC_CARDWriteAsync");
#endif
#if DEBUG
DECOMP_FORCEACTIVE("%s(%d) : (id>=ISG_EXEC_NONE) in '%s'\n", "%s(%d) : (id<ISG_EXEC_NOMORE) in '%s'\n");
#endif
#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("<ERROR:EXECIDOutOfRange>");
#endif

#if DEBUG
void iSGDB_TgtForceFormat(st_ISGSESSION* session, S32 tgtidx)
{
    S32 rc = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;

    rc = iSGTgtSetActive(session, tgtidx);
    if (rc) {
        xVALIDATE_L(4818, session->idx_actvcard>=0);
        mcdata = &session->physcard[session->idx_actvcard];
        iSGDB_mc_formatforce(mcdata);
    }
}

static S32 iSGDB_mc_contents(st_ISG_MEMCARD_DATA*, const char*, S32); // UNUSED

DECOMP_FORCEACTIVE("%s(%d) : (result) in '%s'\n");

static S32 iSGDB_mc_fill(st_ISG_MEMCARD_DATA* mcdata, char* fname, char* data, S32 datasize, S32 fillamt)
{
    S32 result = 1;
    S32 rc = 0;
    S32 remain = fillamt;
    S32 amt = 0;

    result = iSG_mc_fopen(mcdata, fname, fillamt, ISG_IOMODE_WRITE, NULL);
    if (result) {
        while (remain > 0) {
            amt = MIN(remain, datasize);
            rc = iSG_mc_fwrite(mcdata, data, amt);
            if (!rc) {
                result = 0;
                break;
            }

            remain -= amt;
        }

        do {
            rc = CARDSetStatus(mcdata->myphys, mcdata->cur_finf.fileNo, &mcdata->cur_stat);
            ISG_TRACE("CARDSetStatus", rc);
        } while (rc == CARD_RESULT_BUSY);
        xVALIDATE_L(5019, rc == CARD_RESULT_READY);
        if (rc != CARD_RESULT_READY) {
            DBprintf(DBML_WARN, "ISG: Write SetStatus failed - %s\n", iSGDB_tlate_cardres(rc));
        }

        if (result) {
            do {
                rc = CARDSetAttributes(mcdata->myphys, mcdata->cur_finf.fileNo, 0);
                ISG_TRACE("CARDSetAttributes", rc);
            } while (rc == CARD_RESULT_BUSY);
            xVALIDATE_L(5034, rc == CARD_RESULT_READY);
            if (rc != CARD_RESULT_READY) {
                DBprintf(DBML_DBG, "ISG: File Attrib (hide) failed %s - %s\n", fname, iSGDB_tlate_cardres(rc));
            }
        }

        iSG_mc_fclose(mcdata);
    }

    if (result) {
        DBprintf(DBML_DBG, "MC: Wrote %d bytes filler\n", fillamt - remain);
    } else {
        DBprintf(DBML_DBG, "MC: Filler write failed\n");
    }

    return result;
}

static S32 iSGDB_mc_mkfiles(st_ISG_MEMCARD_DATA*); // UNUSED

static S32 iSGDB_mc_formatforce(st_ISG_MEMCARD_DATA* mcdata)
{
    S32 result = 1;
    S32 canRecover = 0;

    result = iSG_mc_format(mcdata, 1, &canRecover);
    xVALIDATE_L(5073, result);
    return result;
}

static void NukaDaCarda()
{
    st_ISGSESSION* session = NULL;
    S32 numtgt = 0;
    S32 maxtgt = 0;
    S32 i = 0;
    S32 rc = 0;

    static S32 oops_skip_nuke = 1;
    if (!oops_skip_nuke) {
        session = iSGSessionBegin(NULL, NULL, 0);
        numtgt = iSGTgtCount(session, &maxtgt);
        for (i = 0; i < numtgt; i++) {
            iSGDB_TgtForceFormat(session, i);
        }
        iSGSessionEnd(session);
    }
}

static void FilaDaCarda()
{
    static S32 _fillme[3] = { 0, 0, 0 };
    static S32 _keepblok[3] = { 1, 1, 1 };
    st_ISGSESSION* session = NULL;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 numtgt = 0;
    S32 maxtgt = 0;
    S32 i = 0;
    S32 rc = 0;
    U32 ts = 0;
    char* fname = "bogi";
    S32 availbyte = 0;
    S32 clustsize = 0;
    S32 keep = 0;
    char* memptr = NULL;
    S32 memsiz = 0;
    S32 fsize = 0;
    S32 availHandle = 0;
    char sample[53] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    S32 j;

    for (i = 0; i < 3; i++) _keepblok[i] = 1;

    session = iSGSessionBegin(NULL, NULL, 0);
    numtgt = iSGTgtCount(session, &maxtgt);
    for (i = 0; i < numtgt; i++) {
        if (!_fillme[i]) continue;

        rc = iSGTgtSetActive(session, i);
        if (!rc) continue;

        ts = iSGTgtState(session, i, NULL);
        if (!(ts & 0x2)) continue;

        availbyte = iSGTgtFreeSpace(session, i, &clustsize, &availHandle);
        if (availHandle < 1) continue;

        memsiz = clustsize;
        if (memsiz < 1000) continue;

        keep = MIN(_keepblok[i], availbyte / clustsize);
        fsize = clustsize * (availbyte / clustsize - keep);
        if (fsize < 1000) continue;

        xVALIDATE_L(5159, session->idx_actvcard>=0);
        mcdata = &session->physcard[session->idx_actvcard];
        xVALIDATE_L(5161, mcdata->sectsize==clustsize);

        memptr = (char*)xMEMPUSHTEMP_L(memsiz, 'ISGT', 5169);
        xVALIDATE_L(5170, memptr);
        if (!memptr) continue;

        for (j = 0; j < memsiz; j++) memptr[j] = sample[j % 52];

        rc = iSGDB_mc_fill(mcdata, fname, memptr, memsiz, fsize);
        xVALIDATE_L(5178, rc);

        xMEMPOPTEMP(memptr);
    }
    iSGSessionEnd(session);
}

static void iSGDB_dump_curstat(CARDStat* stat, S32 doWarn)
{
    static S32 ignoreme = 1;
    en_VERBOSE_MSGLEVEL mlvl = DBML_DBG;

    if (doWarn) {
        xVALIDATE_L(5199, stat->iconAddr>0);
        xVALIDATE_L(5200, stat->iconAddr!=-1);
        xVALIDATE_L(5201, stat->iconAddr<0x7FFFFFFF);

        xVALIDATE_L(5203, stat->commentAddr!=-1);
        xVALIDATE_L(5204, stat->commentAddr<0x7FFFFFFF);
    }

    if (ignoreme) return;

    DBprintf(mlvl, "\tfileName         %d\n", stat->fileName);
    DBprintf(mlvl, "\tlength           %d\n", stat->length);
    DBprintf(mlvl, "\ttime             %d\n", stat->time);

    DBprintf(mlvl, "\tgameName         %c%c%c%c\n", stat->gameName[0], stat->gameName[1], stat->gameName[2],
             stat->gameName[3]);
    DBprintf(mlvl, "\tCompany          %c%c\n", stat->company[0], stat->company[1]);
    DBprintf(mlvl, "\tbannerFormat     %d\n", stat->bannerFormat);
    DBprintf(mlvl, "\ticonAddr         %d\n", stat->iconAddr);
    DBprintf(mlvl, "\ticonFormat       %d\n", stat->iconFormat);
    DBprintf(mlvl, "\ticonSpeed        %d\n", stat->iconSpeed);
    DBprintf(mlvl, "\tcommentAddr      %d\n", stat->commentAddr);
    DBprintf(mlvl, "\toffsetBanner     %d\n", stat->offsetBanner);
    DBprintf(mlvl, "\toffsetBannerTlut %d\n", stat->offsetBannerTlut);
    DBprintf(mlvl, "\toffsetIcon0      %d\n", stat->offsetIcon[0]);
    DBprintf(mlvl, "\toffsetIconTlut   %d\n", stat->offsetIconTlut);
    DBprintf(mlvl, "\toffsetData       %d\n", stat->offsetData);
}

static void iSGDB_dump_curfinf(CARDFileInfo* finf, S32)
{
    static S32 ignoreme = 1;
    en_VERBOSE_MSGLEVEL mlvl = DBML_DBG;

    if (!ignoreme) {
        DBprintf(mlvl, "\tChan             %d\n", finf->chan);
        DBprintf(mlvl, "\tfileNo           %d\n", finf->fileNo);
        DBprintf(mlvl, "\tOffset           %d\n", finf->offset);
        DBprintf(mlvl, "\tLength           %d\n", finf->length);
        DBprintf(mlvl, "\tiBlock           %d\n", finf->iBlock);
    }
}
#endif

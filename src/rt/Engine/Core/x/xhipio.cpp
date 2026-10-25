#include "xhipio.h"

#include "xbinio.h"
#include "xDebug.h"

#include "decomp.h"

#include <string.h>

#define MAX_LOADINST 8

static U32 g_loadlock = 0;
static st_HIPLOADDATA g_hiploadinst[MAX_LOADINST] = {};

static st_HIPLOADDATA* HIPLCreate(const char* filename, char* dblbuf, S32 bufsize, S32 fileflags);
static void HIPLDestroy(st_HIPLOADDATA* lddata);
static U32 HIPLBaseSector(st_HIPLOADDATA* lddata);
static S32 HIPLSetBypass(st_HIPLOADDATA* lddata, S32 enable, S32 use_async);
static void HIPLSetSpot(st_HIPLOADDATA* lddata, S32 spot);
static U32 HIPLBlockEnter(st_HIPLOADDATA* lddata);
static void HIPLBlockExit(st_HIPLOADDATA* lddata);
static S32 HIPLBlockRead(st_HIPLOADDATA* lddata, void* data, S32 cnt, S32 size);
static S32 HIPLBypassRead(st_HIPLOADDATA* lddata, void* data, S32 cnt, S32 size);
static S32 HIPLReadAsync(st_HIPLOADDATA* lddata, S32 pos, char* data, S32 cnt, S32 elesize);
static en_READ_ASYNC_STATUS HIPLPollRead(st_HIPLOADDATA* lddata);
static S32 HIPLReadBytes(st_HIPLOADDATA* lddata, char* data, S32 cnt);
static S32 HIPLReadShorts(st_HIPLOADDATA* lddata, S16* data, S32 cnt);
static S32 HIPLReadLongs(st_HIPLOADDATA* lddata, S32* data, S32 cnt);
static S32 HIPLReadFloats(st_HIPLOADDATA* lddata, F32* data, S32 cnt);
static S32 HIPLReadString(st_HIPLOADDATA* lddata, char* buf);

static st_HIPLOADFUNCS g_map_HIPL_funcmap = {
    HIPLCreate,
    HIPLDestroy,
    HIPLBaseSector,
    HIPLBlockEnter,
    HIPLBlockExit,
    HIPLReadBytes,
    HIPLReadShorts,
    HIPLReadLongs,
    HIPLReadFloats,
    HIPLReadString,
    HIPLSetBypass,
    HIPLSetSpot,
    HIPLPollRead
};

st_HIPLOADFUNCS* get_HIPLFuncs()
{
    return &g_map_HIPL_funcmap;
}

static st_HIPLOADDATA* HIPLCreate(const char* filename, char* dblbuf, S32 bufsize, S32 fileflags)
{
    st_HIPLOADDATA* lddata = NULL;
    st_FILELOADINFO* fli = NULL;
    st_HIPLOADBLOCK* tmp_blk = NULL;
    S32 i = 0;
    S32 uselock = -1;

    xVALIDATE_L(232, filename);

    for (i = 0; i < MAX_LOADINST; i++) {
        if (!(g_loadlock & (1 << i))) {
            uselock = i;
            g_loadlock |= 1 << i;
            lddata = &g_hiploadinst[i];
            break;
        }
    }

    xASSERT_L(243, lddata);

    if (lddata) {
        memset(lddata, 0, sizeof(st_HIPLOADDATA));
        lddata->lockid = uselock;
        lddata->top = -1;
        lddata->base_sector = 0;
        lddata->use_async = 0;
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        lddata->bypass = 0;
        lddata->bypass_recover = -1;
        lddata->pos = 0;
        lddata->readTop = 0;

        for (i = 0; i < MAX_OPENBLK; i++) {
            tmp_blk = &lddata->stk[i];
            tmp_blk->endpos = 0;
            tmp_blk->blk_id = 0;
            tmp_blk->blk_remain = 0;
            tmp_blk->flags = 0;
        }

        fli = xBinioLoadCreate(filename, fileflags);
        if (fli) {
            lddata->fli = fli;
            lddata->base_sector = fli->basesector;
            if (dblbuf && bufsize > 0) {
                fli->setDoubleBuf(fli, dblbuf, bufsize);
            }
        } else {
            HIPLDestroy(lddata);
            lddata = NULL;
        }
    }

    return lddata;
}

static void HIPLDestroy(st_HIPLOADDATA* lddata)
{
    S32 lockid = -1;

    if (lddata) {
        if (lddata->fli) {
            lddata->fli->destroy(lddata->fli);
        }

        lockid = lddata->lockid;
        memset(lddata, 0, sizeof(st_HIPLOADDATA));
        g_loadlock &= ~(1 << lockid);
    }
}

static U32 HIPLBaseSector(st_HIPLOADDATA* lddata)
{
    return lddata->base_sector;
}

static S32 HIPLSetBypass(st_HIPLOADDATA* lddata, S32 enable, S32 use_async)
{
    xVALIDATE_L(337, lddata);

    lddata->fli->discardDblBuf(lddata->fli);

    if (enable && lddata->bypass) {
        xWARN("HIP Bypass Enable - already enabled!\n");
        return 0;
    } else if (!enable && !lddata->bypass) {
        xWARN("HIP Bypass Disable - already disabled!\n");
        return 0;
    }

    if (enable) {
        xVALIDATE_L(354, lddata->bypass_recover == -1);
        lddata->bypass = 1;
        lddata->use_async = use_async;
        lddata->bypass_recover = lddata->fli->position;
    } else {
        lddata->fli->seekSpot(lddata->fli, lddata->bypass_recover);
        lddata->bypass = 0;
        lddata->use_async = 0;
        lddata->bypass_recover = -1;
    }

    return 1;
}

static void HIPLSetSpot(st_HIPLOADDATA* lddata, S32 spot)
{
    S32 rc = 0;

    xVALIDATE_L(377, lddata);
    xVALIDATE_L(379, lddata->bypass);

    if (!lddata->bypass) {
        xWARN("HIP Bypass seeking WITHOUT Bypass Enabled!\n");
        return;
    }

    lddata->pos = spot;

    rc = lddata->fli->seekSpot(lddata->fli, spot);
    xVALIDATE_L(401, rc);
}

static U32 HIPLBlockEnter(st_HIPLOADDATA* lddata)
{
    st_HIPLOADBLOCK* top = NULL;
    U32 cid = 0;
    S32 size = 0;
    S32 cnt = 0;
    S32 padit = 0;

    xVALIDATE_L(428, lddata);

    if (lddata->bypass) {
        xWARN("HIP Block Enter with Bypass Enabled!\n");
        return 0;
    }

    if (lddata->top >= 0) {
        xVALIDATE_L(440, lddata->stk[lddata->top].blk_remain >= 0);
        if (lddata->stk[lddata->top].blk_remain <= 0) {
            return 0;
        }
    }

    cnt = HIPLReadLongs(lddata, (S32*)&cid, -1);
    if (cnt == 0) {
        cid = 0;
    } else {
        xVALIDATE_L(450, cnt == 1);

        cnt = HIPLReadLongs(lddata, &size, -1);
        xVALIDATE_L(452, cnt == 1);

        if (lddata->top >= 0) {
            lddata->stk[lddata->top].blk_remain -= size;
        }

        top = &lddata->stk[++lddata->top];

        xASSERT_L(466, lddata->top < MAX_OPENBLK);

        top->blk_id = cid;
        top->blk_remain = size;
        padit = top->blk_remain & 0x1;
        top->endpos = lddata->pos + top->blk_remain + padit;
        top->flags = 0;
    }

    return cid;
}

static void HIPLBlockExit(st_HIPLOADDATA* lddata)
{
    st_HIPLOADBLOCK* top = NULL;

    xVALIDATE_L(487, lddata);
    xVALIDATE_L(488, !lddata->bypass);

    if (lddata->bypass) {
        xWARN("HIP Block Exit with Bypass Enabled!\n");
        return;
    }

    top = &lddata->stk[lddata->top--];
    
    lddata->fli->skipBytes(lddata->fli, top->endpos - lddata->pos);
    lddata->pos = top->endpos;
}

static S32 HIPLBlockRead(st_HIPLOADDATA* lddata, void* data, S32 cnt, S32 size)
{
    st_HIPLOADBLOCK* top = NULL;
    S32 got = 0;
    S32 left = 0;
    S32 head = 0;

    xVALIDATE_L(527, lddata);
    xVALIDATE_L(528, data);
    xVALIDATE_L(531, !lddata->bypass);

    if (lddata->bypass) {
        return 0;
    }

    if (cnt == 0) {
        return 0;
    }

    if (lddata->top < 0) {
        top = NULL;
    } else {
        top = &lddata->stk[lddata->top];
        left = top->blk_remain / size;
    }

    if (cnt < 0) {
        cnt = -cnt;
        head = 1;
        if (top && cnt > left) {
            cnt = left;
        }
    } else {
        xVALIDATE_L(548, cnt > 0);
        xVALIDATE_L(549, top);
        xVALIDATE_L(550, cnt <= left);
    }

    if (!head && left < cnt) {
        cnt = left;
    }

    xVALIDATE_L(555, (size == 1) || (size == 2) || (size == 4));

    if (cnt == 0) {
        got = 0;
    } else if (size == 1) {
        got = lddata->fli->readBytes(lddata->fli, (char*)data, cnt);
    } else if (size == 2) {
        got = lddata->fli->readMShorts(lddata->fli, (S16*)data, cnt);
    } else if (size == 4) {
        got = lddata->fli->readMLongs(lddata->fli, (S32*)data, cnt);
    }

    if (top && got != cnt) {
        xVALIDATE_FAIL_L(568);
    }

    lddata->pos += got * size;

    if (top) {
        top->blk_remain -= got * size;
    }

    return got * size;
}

static S32 HIPLBypassRead(st_HIPLOADDATA* lddata, void* data, S32 cnt, S32 size)
{
    S32 got = 0;
    S32 rc = 0;

    xVALIDATE_L(600, lddata);
    xVALIDATE_L(601, data);
    xVALIDATE_L(604, lddata->bypass);

    if (!lddata->bypass) {
        return 0;
    }

    if (lddata->use_async) {
        xVERBOSE("HIP - Async Read\n");
        rc = HIPLReadAsync(lddata, lddata->pos, (char*)data, cnt, size);
        return rc;
    }

    if (cnt == 0) {
        return 0;
    }

    xVALIDATE_L(617, (size == 1) || (size == 2) || (size == 4));

    if (cnt == 0) {
        got = 0;
    } else if (size == 1) {
        got = lddata->fli->readBytes(lddata->fli, (char*)data, cnt);
    } else if (size == 2) {
        got = lddata->fli->readMShorts(lddata->fli, (S16*)data, cnt);
    } else if (size == 4) {
        got = lddata->fli->readMLongs(lddata->fli, (S32*)data, cnt);
    }

    xVALIDATE_L(630, got == cnt);

    return got * size;
}

static S32 HIPLReadAsync(st_HIPLOADDATA* lddata, S32 pos, char* data, S32 cnt, S32 elesize)
{
    S32 regok = 0;

    xVALIDATE_L(642, lddata->bypass);
    xVALIDATE_L(643, lddata->use_async);
    xVALIDATE_L(644, lddata->asyn_stat == HIP_RDSTAT_NONE);

    lddata->asyn_stat = HIP_RDSTAT_NONE;

    regok = lddata->fli->asyncMRead(lddata->fli, pos, data, cnt, elesize);
    xVALIDATE_L(652, regok!=0);

    lddata->asyn_stat = HIP_RDSTAT_INPROG;
    return regok;
}

static en_READ_ASYNC_STATUS HIPLPollRead(st_HIPLOADDATA* lddata)
{
    en_READ_ASYNC_STATUS rdstat = HIP_RDSTAT_INPROG;
    en_BIO_ASYNC_ERRCODES pollstat = BINIO_ASYNC_INPROG;

    xVALIDATE_L(665, lddata);
    xVALIDATE_L(666, lddata->bypass);

    if (!lddata->bypass) {
        xWARN("HIP 'PollRead' WITHOUT Bypass Enabled!\n");
        return HIP_RDSTAT_NOBYPASS;
    }

    if (!lddata->use_async) {
        xWARN("HIP 'PollRead' WITHOUT Aynchronous Enabled!\n");
        return HIP_RDSTAT_NOASYNC;
    }

    xVALIDATE_L(678, lddata->asyn_stat == HIP_RDSTAT_INPROG);

    pollstat = lddata->fli->asyncReadStatus(lddata->fli);
    switch (pollstat) {
    case BINIO_ASYNC_INPROG:
        rdstat = HIP_RDSTAT_INPROG;
        break;
    case BINIO_ASYNC_DONE:
        rdstat = HIP_RDSTAT_SUCCESS;
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        break;
    case BINIO_ASYNC_FAIL:
        rdstat = HIP_RDSTAT_FAILED;
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        break;
    default:
        xWARN("Unknown Bin I/O async read status %d\n", pollstat);
        xFAIL_FMT_L(700, "%s", "Unknown state - this probably means a request failed");
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        break;
    }

    return rdstat;
}

static S32 HIPLReadBytes(st_HIPLOADDATA* lddata, char* data, S32 cnt)
{
    xVALIDATE_L(724, lddata);
    xVALIDATE_L(725, data);

    if (lddata->bypass) {
        return HIPLBypassRead(lddata, data, cnt, 1);
    } else {
        return HIPLBlockRead(lddata, data, cnt, 1);
    }
}

static S32 HIPLReadShorts(st_HIPLOADDATA* lddata, S16* data, S32 cnt)
{
    S32 got = 0;

    xVALIDATE_L(732, lddata);
    xVALIDATE_L(733, data);

    if (lddata->bypass) {
        got = HIPLBypassRead(lddata, data, cnt, 2);
    } else {
        got = HIPLBlockRead(lddata, data, cnt, 2);
    }

    return got / 2;
}

static S32 HIPLReadLongs(st_HIPLOADDATA* lddata, S32* data, S32 cnt)
{
    S32 got = 0;

    xVALIDATE_L(741, lddata);
    xVALIDATE_L(742, data);

    if (lddata->bypass) {
        got = HIPLBypassRead(lddata, data, cnt, 4);
    } else {
        got = HIPLBlockRead(lddata, data, cnt, 4);
    }

    return got / 4;
}

static S32 HIPLReadFloats(st_HIPLOADDATA* lddata, F32* data, S32 cnt)
{
    S32 got = 0;

    xVALIDATE_L(750, lddata);
    xVALIDATE_L(751, data);

    if (lddata->bypass) {
        got = HIPLBypassRead(lddata, data, cnt, 4);
    } else {
        got = HIPLBlockRead(lddata, data, cnt, 4);
    }

    return got / 4;
}

#if DEBUG
DECOMP_FORCEACTIVE("%s(%d) : (lddata->top >= 0) in '%s'\n")
#endif

static S32 HIPLReadString(st_HIPLOADDATA* lddata, char* buf)
{
    S32 n = 0;
    char pad = 0;

    xVALIDATE_L(768, lddata);
    xVALIDATE_L(769, buf);

    if (lddata->bypass) {
        while (HIPLBypassRead(lddata, buf + n, 1, sizeof(char))) {
            if (buf[n] == '\0') {
                if (!(n & 1)) {
                    HIPLBypassRead(lddata, &pad, 1, sizeof(char));
                }
                break;
            }
            n++;
        }
    } else {
        while (HIPLBlockRead(lddata, buf + n, 1, sizeof(char))) {
            if (buf[n] == '\0') {
                if (!(n & 1)) {
                    HIPLBlockRead(lddata, &pad, 1, sizeof(char));
                }
                break;
            }
            n++;
        }
    }

    return n;
}

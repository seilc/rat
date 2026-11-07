#include "xDebug.h"

#include "iAsync.h"
#include "iCamera.h"
#include "iFile.h"
#include "iMemMgr.h"
#include "iMath.h"
#include "iTime.h"
#include "xDebugTweak.h"
#include "xFile.h"
#include "xFX.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xMemMgr.h"
#include "xMath.h"
#include "xModel.h"
#include "xPad.h"
#include "xRegionSupport.h"
#include "xRenderState.h"
#include "xScrFx.h"
#include "xSndMgr.h"
#include "xSystem.h"
#include "xTimestamp.h"
#include "xDraw.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zFMV.h"
#include "zGame.h"
#include "zGameState.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zCam.h"
#include "zCutsceneMgr.h"
#include "zPad.h"
#include "zPlayer.h"
#include "zScene.h"

#include "decomp.h"

#include <rpmatfx.h>
#include <rttilerd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct xSB
{
    char* buf;
    U32 max;
    char* cur;
    char* disp;
};

#define MAX_DEBUG_MODES 68

struct DebugModeData
{
    xDebugModeCallback callback;
    const char* name;
    S32 virtualID;
};

static U8 gInRenderLoop;
#if DEBUG || RELEASE
static S32 g_STFU;
static void (*g_msgnotify)(en_VERBOSE_MSGLEVEL, const char*);
static FILE* g_userfp;
static FILE* g_errfp;
static S32 g_dbinit;
static iTime g_lasttym;
static char g_lprbuf[2048] = { 0 };
#if RELEASE
static en_VERBOSE_MSGLEVEL g_msglvl = DBML_USER;
#else
static en_VERBOSE_MSGLEVEL g_msglvl = DBML_DBG;
#endif
static S32 g_prepend_msglevel = TRUE;
static char* g_dbml_strz[13] = {
    NULL, NULL, NULL, NULL, "ERROR:", NULL, "WARNING:", "INVALID:", NULL, NULL, NULL, NULL, "(Screen)"
};

bool xDebugIDontWantYourXprintsOnMyScreen()
{
    return (xDebugModeGet() == 8 || xDebugModeGet() == 19) || xglobals->dumpCutscene;
}

xDebugModeCallback dm_callbacks[20] = {
    NULL, debug_mode_page, debug_mode_tweak, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, debug_mode_fontperf
};

static char* dm_names[20] = {
    "",
    "** Page Selector **",
    "* Tweak Parameters *",
    "dm_profiler",
    "dm_collision",
    "dm_components",
    "dm_ent",
    "dm_shadow",
    "dm_snd",
    "dm_graph",
    "dm_par",
    "dm_render",
    "dm_mem",
    "dm_memtracker",
    "dm_vram",
    "dm_pad",
    "dm_lensflare",
    "dm_model",
    "dm_movie",
    "dm_fontperf"
};

static char xStatsFieldName[35][32] = {
    "cpu",
    "render",
    "world_render",
    "model_render",
    "ent_update",
    "shadow",
    "shadow_track",
    "shadow_texrender",
    "shadow_collideworld",
    "shadow_renderworld",
    "shadow_receivesetup",
    "shadow_frustumtest",
    "shadow_receive",
    "collision",
    "anim",
    "particle",
    "sound",
    "font",
    "Villain_Update",
    "Villain_Collide",
    "Villain_DBG",
    "Villain_Input",
    "Villain_Anim",
    "Projectile_Update",
    "Projectile_Collide",
    "Projectile_Effects",
    "Projectile_Solver",
    "player_update",
    "Particle_Render",
    "Render_UI",
    "scene_update",
    "scene_render",
    "x1",
    "x2",
    "x3"
};

#endif

U32 gFrameCount;
U32 gSceneFrameCount;
#if DEBUG || RELEASE
static S32 sDebugMode;
S32 last_debug_mode;
U32 xprintf_display = 1;
U32 xprintf_display_modes = 2;
DebugModeData sDebugModeData[MAX_DEBUG_MODES];
S32 sDebugModeLimit;
S32 sDebugModeNextVirtualID;

static S32 mapVirtualIDToRealID(S32 virtualID)
{
    for (S32 i = 0; i < sDebugModeLimit; i++)
        if (virtualID == sDebugModeData[i].virtualID)
            return i;
    return -1;
}

#endif

void xDebugValidateFailed()
{
    static S32 break_enabled = FALSE;

#if DEBUG || RELEASE
    if (break_enabled)
        iDebugBreak();
#endif
}

#if DEBUG || RELEASE
static xSB screenSB;
static xSB relMemInfoSB;
#endif

void xSBCreate(xSB* sb, U32 size)
{
    xASSERT_L(353, sb != 0);
    xASSERT_L(356, sb->buf == 0);
    sb->buf = (char*)RwMallocL(size, rwMEMHINTDUR_NADURATION, 357);
    sb->buf[0] = '\0';
    sb->max = size;
    sb->cur = sb->disp = sb->buf;
}

char* xSBAppend(xSB* sb, const char* str)
{
    xASSERT_L(365, sb != 0);
    if (str == NULL) {
        return NULL;
    }
    char* pRetVal = sb->cur;
    U32 size = strlen(str);
    if (size + (sb->cur - sb->buf) < sb->max - 2) {
        strcpy(sb->cur, str);
        sb->cur += size;
    }
    return pRetVal;
}

void xSBRender(xSB* sb)
{
    if (!RwCameraGetCurrentCamera()) {
        return;
    }

    xtextbox::log_layout_stats(false);
    static xtextbox tb = xtextbox::create(
        xfont::create(2, NSCREENX(12.0f), NSCREENY(12.0f), NSCREENX(2.0f), g_GRAY80, screen_bounds, g_BLACK, 2.0f, 2.0f),
        screen_bounds, 0, NSCREENY(2.0f), 0.0f, 0.0f, 0.0f);
#if DEBUG || RELEASE
    if (xDebugModeGet() == 8) {
        tb.font.width = NSCREENX(7.0f);
        tb.font.height = NSCREENY(7.0f);
    } else
#endif
    {
        tb.font.width = NSCREENX(12.0f);
        tb.font.height = NSCREENY(12.0f);
    }
    tb.bounds = screen_bounds;
    tb.bounds.contract(NSCREENX(28.0f), NSCREENY(80.0f), NSCREENX(28.0f), NSCREENY(0.0f));
    S32 lineCount = 1;
    for (S32 i = 0; sb->disp[i] != '\0'; i++) {
        if (sb->disp[i] == '\n') {
            lineCount++;
            if (lineCount >= 32) {
                sb->disp[i] = '\0';
                break;
            }
        }
    }
    tb.set_text(sb->disp);
    tb.render(false);
    xtextbox::log_layout_stats(false);
}

void xSBReset(xSB* sb)
{
    xASSERT_L(440, sb != 0);
    sb->cur = sb->buf;
}

void xSBDestroy(xSB* sb)
{
    xASSERT_L(447, sb != 0);
    RwFree(sb->buf);
    sb->buf = NULL;
}

#if DEBUG || RELEASE
void DBStartup(en_VERBOSE_MSGLEVEL setlvl, char* logfile, char* errfile,
               void (*msgnotify)(en_VERBOSE_MSGLEVEL, const char*))
{
    if (g_dbinit++ == 0) {
        g_msglvl = setlvl;
        g_msgnotify = msgnotify;
        if (logfile) g_userfp = fopen(logfile, "w");
        if (errfile) g_errfp = fopen(errfile, "w");
        g_lasttym = iTimeGet();
        xSBCreate(&screenSB, 0x3000);
        xSBCreate(&relMemInfoSB, 0x400);
    }
    DBprintf(DBML_VDBG, "xDebug: DBMsg services started ...\n");
}

void DBShutdown()
{
    if (--g_dbinit == 0) {
        if (g_userfp) fclose(g_userfp);
        g_userfp = NULL;
        if (g_errfp) fclose(g_errfp);
        g_errfp = NULL;
        g_msgnotify = NULL;
        xSBDestroy(&screenSB);
        xSBDestroy(&relMemInfoSB);
    }
}

S32 DBChkMsgLvl(en_VERBOSE_MSGLEVEL msglvl)
{
    if (msglvl <= DBML_NONE) {
        fprintf(stdout, "xDebug:WARN: Message uses DBML_NONE --> use DBML_USER\n");
        iDebugBreak();
        return FALSE;
    }
    if (msglvl > g_msglvl) {
        return FALSE;
    }
    if (g_msglvl == DBML_TIME && msglvl != DBML_TIME) {
        return FALSE;
    }
    return TRUE;
}

void DBprintf(en_VERBOSE_MSGLEVEL msglvl, const char* fmt, ...)
{
    va_list marker;
    char* skipptr;
    static iTime elap = 0;

    g_lprbuf[0] = '\0';
    if (g_STFU || !DBChkMsgLvl(msglvl)) {
        return;
    }

    if (msglvl == DBML_TIME) {
        iTime tym = iTimeGet();
        if (tym < g_lasttym) elap = tym + (0xFFFFFFFF - g_lasttym);
        else elap = tym - g_lasttym;
        g_lasttym = tym;
        sprintf(g_lprbuf, "{Game: %fs xTime: %.5f Elap: %.5f} ", iTimeGetGame(), iTimeDiffSec(tym), iTimeDiffSec(elap));
        skipptr = g_lprbuf + strlen(g_lprbuf);
    }
    else if (g_prepend_msglevel) {
        if (g_dbml_strz[msglvl]) strcpy(g_lprbuf, g_dbml_strz[msglvl]);
        else g_lprbuf[0] = '\0';
        skipptr = g_lprbuf + strlen(g_lprbuf);
    } else {
        skipptr = g_lprbuf;
    }

    va_start(marker, fmt);
    vsprintf(skipptr, fmt, marker);
    va_end(marker);

    if (msglvl == DBML_DISP) {
        xSBAppend(&screenSB, g_lprbuf);
        return;
    }
    if (msglvl == DBML_RELDISP) {
#if RELEASE
        xSBAppend(&relMemInfoSB, g_lprbuf);
#endif
        return;
    }

    fprintf(stdout, g_lprbuf);
    fflush(stdout);
    if (g_msgnotify) g_msgnotify(msglvl, g_lprbuf);
    if (g_errfp) {
        fprintf(g_errfp, g_lprbuf);
        fflush(g_errfp);
    }
    if (g_userfp && msglvl == DBML_USER) {
        fprintf(g_userfp, g_lprbuf);
        fflush(g_userfp);
    }
    iDebugIDEOutput(g_lprbuf);
}

void xDebugVSync()
{
    iDebugVSync();
}

#endif

void xprintf(const char* fmt, ...)
{
#if DEBUG || RELEASE
    va_list marker;
    char buf[512];
    char buf2[512];
    buf[0] = '\0';
    va_start(marker, fmt);
    vsprintf(buf, fmt, marker);
    va_end(marker);

    char* s = buf;
    char* d = buf2;
    while (*s) {
        if (*s == '%')
            *d++ = *s;
        *d++ = *s++;
    }
    *d = '\0';
    DBprintf(DBML_DISP, buf2);
#endif
}

#if DEBUG || RELEASE
static const char* _assert2func;
static const char* _assert2file;
static U32 _assert2line;
static const char* _assert2expr;

void xDebug_assert2_info(const char* func, const char* file, U32 line, const char* expr)
{
    _assert2func = func;
    _assert2file = file;
    _assert2line = line;
    _assert2expr = expr;
}

void xDebugReportAssertion(const char* sfile, S32 line, const char* func, const char* expr, const char* message)
{
    xFile file;
    char workingBuffer[512];
    U32 result;
    const char* platform;
    char* it;
    static bool first = true;

    if (first) {
        result = iFileOpen("Assert.log", 0x86, &file);
        if (result == 0) {
            sprintf(workingBuffer, "%s\tPLATFORM\tREGION\tFILE\tLINE\tFUNC\tMESSAGE\r\n", zStealthDebugGetAssertionHeader());
            iFileWrite(&file, workingBuffer, strlen(workingBuffer));
            first = false;
        }
    } else {
        result = iFileOpen("Assert.log", 0x281, &file);
    }

    if (result != 0) {
        iprintf("cannot open assert.log!\n");
        return;
    }

    iFileSeek(&file, 0, 2);

    platform = "GCN";

    sprintf(workingBuffer, "%s\t%s\t%s\t%s\t%d\t%s\t", zStealthDebugGetAssertionInfo(), platform,
            xRegionCode(xSTGetLocalizationEnum()), sfile, line, func);
    iFileWrite(&file, workingBuffer, strlen(workingBuffer));
    iFileWrite(&file, (void*)expr, strlen(expr));
    iFileWrite(&file, (void*)"\xff", 1);

    strcpy(workingBuffer, message);
    for (it = workingBuffer; *it; it++)
        if (*it == '\n')
            *it = '\xff';
    iFileWrite(&file, workingBuffer, strlen(workingBuffer));
    iFileWrite(&file, (void*)"\r\n", 2);
    iFileClose(&file);
}

void xDebug_assert2(const char* fmt, ...)
{
    va_list argp;
    char buf1[512];
    char buf2[512];
    char pretty_buf[640];

    sprintf(buf1, "%s(%d) : ASSERTION FAILED (%s) in %s\n", _assert2file, _assert2line, _assert2expr, _assert2func);
    sprintf(pretty_buf,
            "{c=ff00aa00}%s{~:c}({c=ff00aa00}%d{~:c}) : ASSERTION FAILED ({c=ffaaaa00}%s{~:c}) in {c=ff00aa00}%s{~:c}\n",
            _assert2file, _assert2line, _assert2expr, _assert2func);

    va_start(argp, fmt);
    vsprintf(buf2, fmt, argp);
    va_end(argp);

    xDebugReportAssertion(_assert2file, _assert2line, _assert2func, _assert2expr, buf2);

    gzAsyncRenderDisable = 1;
    xDebugAssertScreen(buf1, buf2, pretty_buf);
    gzAsyncRenderDisable = 0;
}

static RwCamera* g_visass_cam;
static F32 s_visass_y[2];

static S32 SkipButtonPressedAnyPad()
{
    S32 skipButton = 0x100000;
    S32 pressed = FALSE;
    S32 i;

    for (i = 0; i < k_XPAD_MAX; i++) {
        if (mPad[i].pressed & skipButton)
            pressed = TRUE;
    }

    return pressed;
}

S32 LetItSkip_and_LetItCrash;

U32 xDebugBoing()
{
    if (gSoak || xglobals->skipAsserts || xglobals->dumpCutscene) {
        return 0;
    }
    if (LetItSkip_and_LetItCrash) {
        return 1;
    }

    iAsyncTerminate();

    do {
        xPadUpdateAllPads(0.0f);

        if (!gInRenderLoop) {
            RwCamera* cam = g_visass_cam;
            if (cam) {
                RwCameraBeginUpdate(cam);
                basic_rect<F32> top, bot;
                xColor bg;
                top.assign(0.0f, 0.0f, 1.0f, s_visass_y[0]);
                bot.assign(0.0f, s_visass_y[1], 1.0f, 1.0f);
                bg.r = 32 + (S32)(32.0f * isin(0.0625f * gFrameCount));
                bg.g = bg.b = 0;
                bg.a = 255;
                render_fill_rect(top, bg);
                render_fill_rect(bot, bg);
                RwCameraEndUpdate(cam);
                RwCameraShowRaster(cam, NULL, 1);
                gFrameCount++;
            }
        }
    } while (!SkipButtonPressedAnyPad());

    if (zPad::GetDebugPad()->Pressed(0x20000)) {
        return 1;
    }
    return 0;
}

#endif

S32 xDebugModeAdd(const char* debugModeName, xDebugModeCallback func)
{
#if DEBUG || RELEASE
    for (S32 i = 0; i < sDebugModeLimit; i++) {
        if (stricmp(sDebugModeData[i].name, debugModeName) == 0) {
            DBprintf(DBML_WARN, "DebugMode '%s' already registered\n", debugModeName);
            return -1;
        }
    }

    if (sDebugModeLimit >= MAX_DEBUG_MODES) {
        return -1;
    }

    S32 positionToAdd;
    for (positionToAdd = 0; positionToAdd < sDebugModeLimit; positionToAdd++) {
        if (stricmp(sDebugModeData[positionToAdd].name, debugModeName) > 0) {
            break;
        }
    }

    for (S32 i = sDebugModeLimit; i > positionToAdd; i--) {
        sDebugModeData[i] = sDebugModeData[i - 1];
    }
    sDebugModeLimit++;

    if (sDebugMode >= positionToAdd) {
        sDebugMode++;
    }

    sDebugModeData[positionToAdd].name = debugModeName;
    sDebugModeData[positionToAdd].callback = func;
    S32 virtualID = sDebugModeNextVirtualID++;
    sDebugModeData[positionToAdd].virtualID = virtualID;

    return virtualID;
#else
    return -1;
#endif
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("mode > 0")

void xDebugModeSet(S32 mode)
{
    S32 oldmode = sDebugModeData[sDebugMode].virtualID;

    if (mode == 2 && oldmode != 2)
        debug_mode_tweak_begin();
    else if (mode != 2 && oldmode == 2)
        debug_mode_tweak_end();

    S32 realMode = mapVirtualIDToRealID(mode);
    if (realMode != -1)
        sDebugMode = realMode;
}

static void xDebugModeNext()
{
    S32 mode = (sDebugMode == sDebugModeLimit - 1) ? 0 : sDebugMode + 1;
    xDebugModeSet(sDebugModeData[mode].virtualID);
}

static void xDebugModePrev()
{
    S32 mode = (sDebugMode == 0) ? sDebugModeLimit - 1 : sDebugMode - 1;
    xDebugModeSet(sDebugModeData[mode].virtualID);
}

S32 xDebugModeGet()
{
    return sDebugModeData[sDebugMode].virtualID;
}

void xDebugModeSetByName(const char* name)
{
    for (S32 i = 0; i < sDebugModeLimit; i++)
        if (stricmp(sDebugModeData[i].name, name) == 0) {
            xDebugModeSet(sDebugModeData[i].virtualID);
            break;
        }
}

#endif

void xDebugModeRegisterCallback(S32 mode, xDebugModeCallback func)
{
#if DEBUG || RELEASE
    S32 realID = mapVirtualIDToRealID(mode);
    xASSERT_L(1195, realID != -1);
    sDebugModeData[realID].callback = func;
#endif
}

void xDebugRenderLoopStart()
{
    gInRenderLoop = 1;
}

void xDebugRenderLoopEnd()
{
    gInRenderLoop = 0;
}

#if DEBUG || RELEASE
struct ScreenCapture
{
    S32 width;
    S32 height;
};

static ScreenCapture sScreenCaptureModes[15] = {
    { 0, 0 },
    { 640, 480 },
    { 800, 600 },
    { 1024, 768 },
    { 1280, 960 },
    { 1600, 1200 },
    { 1920, 1440 },
    { 2048, 1536 },
    { 2560, 1920 },
    { 3072, 2304 },
    { 3200, 2400 },
    { 4800, 3600 },
    { 6400, 4800 },
    { 7200, 5400 },
    { 9600, 7200 }
};
static char* sScreenCaptureNames[15];
static char sScreenCaptureNamesBuffer[210];
static U32 sScreenCaptureMode;
static S32 sDumpFileNumber;
static U8 sDumpFileValid;
static char sDumpFile[512];
char gScreenDumpPath[256];
xScreenDump gScreenDump;

char* xDebugGetNextDumpFile()
{
    if (!sDumpFileValid) {
        while (TRUE) {
            xFile file;
            sprintf(sDumpFile, "%sdump%05d.bmp", gScreenDumpPath, sDumpFileNumber);
            sDumpFileNumber++;
            if (iFileOpen(sDumpFile, 0x11, &file) != 0) break;
            iFileClose(&file);
        }
        sDumpFileValid = 1;
    }

    return sDumpFile;
}

void xDebugAdvanceDumpFile()
{
    sDumpFileValid = 0;
}

struct gMassiveDumpParams
{
    S32 width;
    S32 height;
    S32 tile_width;
    S32 tile_height;
    S32 cur_x_tile;
    S32 cur_y_tile;
};

gMassiveDumpParams gMassiveDump;
static U8* sMassiveBuffer;
static S32 sMassiveBufferMax;
S32 sSeekMin;
S32 sSeekMax;

static void xDebugBufferedWriteFlush(xFile* file)
{
    xASSERT_L(1337, sSeekMax > sSeekMin && sSeekMin > 0);

    iFileSeek(file, sSeekMin, 0);
    S32 n = iFileWrite(file, sMassiveBuffer, sMassiveBufferMax);
    xASSERT_L(1342, n == sMassiveBufferMax);
}

static void xDebugBufferedWrite(xFile* file, S32 seek, U8* data, S32 size)
{
    xASSERT_L(1348, file != NULL);
    xASSERT_L(1349, sSeekMax > sSeekMin);

    if (data) {
        memcpy(sMassiveBuffer + seek - sSeekMin, data, size);
    }
}

#pragma push
#pragma optimization_level 4

static const S32 sizeofBmpHeader = sizeof(BmpMegaHeader) - 2;

static bool xDebugCreateBMP(const char* fname, S32 width, S32 height, xFile* file)
{
    BmpMegaHeader mhdr;
    U32 n;
    U8 zero;

    mhdr.bfType[0] = 'B';
    mhdr.bfType[1] = 'M';
    mhdr.bmFile.bfSize = gMassiveDump.height * (gMassiveDump.width * 3) + sizeofBmpHeader;
    mhdr.bmFile.bfReserved1 = 0;
    mhdr.bmFile.bfReserved2 = 0;
    mhdr.bmFile.bfOffBits = sizeofBmpHeader;
    mhdr.bmInfo.biSize = sizeof(BITMAPINFOHEADER);
    mhdr.bmInfo.biWidth = width;
    mhdr.bmInfo.biHeight = height;
    mhdr.bmInfo.biPlanes = 1;
    mhdr.bmInfo.biBitCount = 24;
    mhdr.bmInfo.biCompression = 0;
    mhdr.bmInfo.biSizeImage = width * height * 3;
    mhdr.bmInfo.biXPelsPerMeter = 0;
    mhdr.bmInfo.biYPelsPerMeter = 0;
    mhdr.bmInfo.biClrUsed = 0;
    mhdr.bmInfo.biClrImportant = 0;

    if (iFileOpen(fname, 0x9E, file) != 0) {
        xASSERT_MSG_L(1410, false, fname);
        return false;
    }

    n = iFileWrite(file, mhdr.bfType, sizeofBmpHeader);
    xASSERT_L(1416, n == sizeofBmpHeader);

    iFileSeek(file, mhdr.bmFile.bfSize - 1, 0);
    zero = 0;
    iFileWrite(file, &zero, 1);
    return true;
}

static void xDebugWriteBMPTile(xFile* file, S32 width, S32 height, S32 x, S32 y, U8* pixels)
{
    y = height - 1 - y;
    S32 seek = (x + y * width) * 3 + sizeofBmpHeader;
    U8* sp = pixels;
    for (S32 yy = 0; yy < gMassiveDump.tile_height; yy++) {
        U8 rgb[1920];
        for (S32 xx = 0; xx < gMassiveDump.tile_width; xx++) {
            rgb[xx * 3 + 0] = sp[xx * 4 + 2];
            rgb[xx * 3 + 1] = sp[xx * 4 + 1];
            rgb[xx * 3 + 2] = sp[xx * 4 + 0];
        }
        xDebugBufferedWrite(file, seek, rgb, gMassiveDump.tile_width * 3);
        seek -= width * 3;
        sp += gMassiveDump.tile_width * 4;
    }
}

static void xDebugCreateRAW(const char* fname, S32 width, S32 height); // UNUSED
static void xDebugWriteRAWTile(const char* fname, S32 width, S32 height, S32 x, S32 y, U8* pixels); // UNUSED

#pragma pop

DECOMP_FORCEACTIVE("raw")

static RwCamera* RenderTile(RwCamera* camera, S32 tx, S32 ty, void*)
{
    iprintf("RenderTile %d,%d (%d,%d - %d,%d)\n", tx, ty, tx * gMassiveDump.tile_width, ty * gMassiveDump.tile_height,
            tx * gMassiveDump.tile_width + gMassiveDump.tile_width - 1,
            ty * gMassiveDump.tile_height + gMassiveDump.tile_height - 1);

    gMassiveDump.cur_x_tile = tx;
    gMassiveDump.cur_y_tile = ty;

    RwRGBA bkcol;
    bkcol.red = 255;
    bkcol.green = 0;
    bkcol.blue = 255;
    bkcol.alpha = 255;

    RwCameraClear(camera, &bkcol, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);

    zScene* s = xglobals->sceneCur;

    RpWorld* world = s->env->geom->world;
    RpWorldAddCamera(world, camera);

    zGamePreRender();

    if (RwCameraBeginUpdateWrapper(camera)) {
        zSceneRender();
        RwCameraEndUpdate(camera);
        RwCameraShowRaster(camera, NULL, 0);
    }

    RpWorldRemoveCamera(world, camera);

    return camera;
}

static RwImage* SaveTile(RwImage* image, S32 tx, S32 ty, void* pData)
{
    iprintf("SaveTile   %d,%d (%d,%d - %d,%d)\n", tx, ty, tx * gMassiveDump.tile_width, ty * gMassiveDump.tile_height,
            tx * gMassiveDump.tile_width + gMassiveDump.tile_width - 1,
            ty * gMassiveDump.tile_height + gMassiveDump.tile_height - 1);

    if (tx == 0) {
        S32 y = gMassiveDump.height - ty * gMassiveDump.tile_height;
        y -= gMassiveDump.tile_height;
        sSeekMin = gMassiveDump.width * (y * 3) + sizeofBmpHeader;
        sSeekMax = sSeekMin + gMassiveDump.tile_height * (gMassiveDump.width * 3);
        iprintf("seek min:  %d\n", sSeekMin);
        iprintf("seek max:  %d\n", sSeekMax);
    }

    U8* pixels = RwImageGetPixels(image);
    xDebugWriteBMPTile((xFile*)pData, gMassiveDump.width, gMassiveDump.height, tx * gMassiveDump.tile_width,
                       ty * gMassiveDump.tile_height, pixels);

    if (gMassiveDump.width == (tx + 1) * gMassiveDump.tile_width) {
        xDebugBufferedWriteFlush((xFile*)pData);
    }

    return image;
}

xMat4x3 gMassiveDumpCameraMatrix;
xVec2 gMassiveDumpViewWindow;

static void xDebugTakeMassiveDump(RwCamera* camera)
{
    xASSERT_L(1617, camera);
    xASSERT_L(1618, gMassiveDump.width > 0);
    xASSERT_L(1619, gMassiveDump.height > 0);

    RwRaster* camras = RwCameraGetRaster(camera);
    S32 nx = 0.9999f + (F32)gMassiveDump.width / RwRasterGetWidth(camras);
    S32 ny = 0.9999f + (F32)gMassiveDump.height / RwRasterGetHeight(camras);

    while (gMassiveDump.width % nx)
        nx++;
    while (gMassiveDump.height % ny)
        ny++;

    gMassiveDump.tile_width = 0.9999f + gMassiveDump.width / nx;
    gMassiveDump.tile_height = 0.9999f + gMassiveDump.height / ny;

    iprintf("Taking a massive dump -- %d x %d pixels (%d x %d tiles)\n", gMassiveDump.width, gMassiveDump.height, nx, ny);
    iprintf("Selected tile size of %d x %d pixels\n", gMassiveDump.tile_width, gMassiveDump.tile_height);

    sMassiveBufferMax = gMassiveDump.width * gMassiveDump.tile_height * 3;
    iprintf("Massive buffer:  %d KB\n", (sMassiveBufferMax + 512) >> 10);
    sMassiveBuffer = (U8*)RwMallocL(sMassiveBufferMax, rwMEMHINTDUR_NADURATION, 1646);

    const char* file;
    xFile fileObject;
    file = xDebugGetNextDumpFile();

    if (!xDebugCreateBMP(file, gMassiveDump.width, gMassiveDump.height, &fileObject)) {
        return;
    }

    RtTileRender(camera, gMassiveDump.width, gMassiveDump.height, gMassiveDump.tile_width,
                 gMassiveDump.tile_height, RenderTile, SaveTile, &fileObject);

    RwFree(sMassiveBuffer);
    sMassiveBuffer = NULL;

    iFileClose(&fileObject);

    xDebugAdvanceDumpFile();
    gScreenDump = exSD_MASSIVE_RESET;

    zCamExitDebugCam();

    iprintf("Done.\n");
}

static void TakeScreenShotCallback(const tweak_info&)
{
    if (sScreenCaptureModes[sScreenCaptureMode].width == 0)
        gScreenDump = exSD_NATIVE;
    else {
        gMassiveDump.width = sScreenCaptureModes[sScreenCaptureMode].width;
        gMassiveDump.height = sScreenCaptureModes[sScreenCaptureMode].height;
        gScreenDump = exSD_MASSIVE;
        RwCamera* cam = globals.screen->icam;
        gMassiveDumpCameraMatrix = *(xMat4x3*)RwFrameGetMatrix(RwCameraGetFrame(cam));
        gMassiveDumpViewWindow = *(xVec2*)RwCameraGetViewWindow(cam);
    }
}

static void NextNameScreenShotCallback(const tweak_info&, void* value)
{
    const char** str = (const char**)value;
    const char* f = xDebugGetNextDumpFile();
    const char* it = f + strlen(f) - 1;

    while (it > f) {
        if (*it == ':' || *it == '\\' || *it == '/') {
            it++;
            break;
        }
        it--;
    }
    *str = it;
}

static S32 envmode;
#endif

void xDebugModeService()
{
#if DEBUG || RELEASE
    if ((xglobals->dumpCutscene && globals.cmgr) ||
        (xglobals->PlaybackMode == 3 && gSceneFrameCount >= xglobals->PlaybackStartFrame &&
         gSceneFrameCount <= xglobals->PlaybackEndFrame)) {
        U32 num;

        if (xglobals->dumpCutscene) {
            num = sDumpFileNumber;
        } else {
            num = gSceneFrameCount;
        }

        sDumpFileValid = 1;

        sprintf(sDumpFile, "%smovie%05d.bmp", gScreenDumpPath, num);

        if (xglobals->dumpCutscene) {
            if (globals.cmgr && globals.cmgr->csn && globals.cmgr->csn->started) {
                sDumpFileNumber++;

                if (xglobals->PlaybackResolution == 0)
                    gScreenDump = exSD_NATIVE;
                else {
                    gScreenDump = exSD_MASSIVE;
                    gMassiveDump.width = sScreenCaptureModes[xglobals->PlaybackResolution].width;
                    gMassiveDump.height = sScreenCaptureModes[xglobals->PlaybackResolution].height;
                    RwCamera* cam = globals.screen->icam;
                    gMassiveDumpCameraMatrix = *(xMat4x3*)RwFrameGetMatrix(RwCameraGetFrame(cam));
                    gMassiveDumpViewWindow = *(xVec2*)RwCameraGetViewWindow(cam);
                }
            } else
                gScreenDump = exSD_NONE;
        } else {
            if (xglobals->PlaybackResolution == 0)
                gScreenDump = exSD_NATIVE;
            else {
                gScreenDump = exSD_MASSIVE;
                gMassiveDump.width = sScreenCaptureModes[xglobals->PlaybackResolution].width;
                gMassiveDump.height = sScreenCaptureModes[xglobals->PlaybackResolution].height;
                RwCamera* cam = globals.screen->icam;
                gMassiveDumpCameraMatrix = *(xMat4x3*)RwFrameGetMatrix(RwCameraGetFrame(cam));
                gMassiveDumpViewWindow = *(xVec2*)RwCameraGetViewWindow(cam);
            }
        }
    }
    else if (xglobals->dumpCutscene) {
        exit(0);
    }

    xprintf_display_modes = 2;

    if (sDebugModeData[sDebugMode].callback) {
        sDebugModeData[sDebugMode].callback();
    }

    last_debug_mode = sDebugMode;

    if (gScreenDump == exSD_MASSIVE) {
        xDebugTakeMassiveDump(RwCameraGetCurrentCamera());
    } else if (gScreenDump == exSD_MASSIVE_RESET) {
        gScreenDump = exSD_NONE;
    }

    if (zPad::GetDebugPad()->Active()) {
        if (xglobals->PlaybackMode <= 2) {
            if (zPad::GetDebugPad()->Pressed(0x1)) {
                xprintf_display = (xprintf_display + 1) % xprintf_display_modes;
            }
        } else {
            xprintf_display = 0;
        }

        if (xglobals->PlaybackMode == 2) {
            xprintf("frame %d\n", gSceneFrameCount);
        }

        if (xprintf_display == 1) {
            xSBRender(&screenSB);
        }
        xSBReset(&screenSB);

        if (xglobals->sceneCur) {
            if (CURPLAYERS[0]->model && CURPLAYERS[0]->model->Mat) {
                char str[8];
                xVec3* p = xEntGetPos(CURPLAYERS[0]);
                if (p->x && p->y && p->y) {
                    SCENEIDSTR(xglobals->sceneCur->sceneID, str);

                    if (sDebugMode >= sDebugModeLimit) xDebugModeSet(0);
                    if (sDebugModeData[sDebugMode].name) {
                        if (sDebugMode == 0) {
                            DBprintf(DBML_DISP, "\n%s   %.1f %.1f %.1f\n", str, p->x, p->y, p->z);
                        } else {
                            DBprintf(DBML_DISP, "\n%s %-24s\n", str, sDebugModeData[sDebugMode].name);
                        }
                    } else {
                        xFAIL_L(1883);
                    }
                }
            }
        }

        static S32 delay_switch = 0;
        if (zPad::GetDebugPad()->Pressed(0x100) && zPad::GetDebugPad()->Pressed(0x1000)) {
            xDebugModeSet(1);
            delay_switch = 0;
        }
        else if (zPad::GetDebugPad()->Pressed(0x100)) {
            xDebugModePrev();
            delay_switch = 0;
        }
        else if (zPad::GetDebugPad()->Pressed(0x1000)) {
            xDebugModeNext();
            delay_switch = 0;
        }
        else if (zPad::GetDebugPad()->On(0x100)) {
            if (delay_switch++ > 12) {
                xDebugModePrev();
                delay_switch = 0;
            }
        }
        else if (zPad::GetDebugPad()->On(0x1000)) {
            if (delay_switch++ > 12) {
                xDebugModeNext();
                delay_switch = 0;
            }
        }
        else delay_switch = 0;

        if (xDebugModeGet() == 12 && zPad::GetDebugPad()->Pressed(0x10000)) {
            U32 usage = iMemSniffStackUsage();
            iprintf("maximum stack usage:  %d bytes (%6.2f KB)\n", usage, usage / 1024.0f);
        }

        if (xDebugModeGet() == 8 && xprintf_display == 1) {
            xSndMgrDebugRender();
        }

        if (xDebugModeGet() == 16) {
            if (zPad::GetDebugPad()->Pressed(0x10000))
                gxLensFlareDebug ^= 1;
            if (zPad::GetDebugPad()->Pressed(0x20000)) {
                gxLensFlareDebug ^= 2;
            }
        }

        if (xDebugModeGet() == 17) {
            xModelDebugPrintPoolUsage();
        }

        if (xDebugModeGet() == 14 && gGameState == 1 && xprintf_display == 1) {
            RpWorld* world;
            RpMaterial* mat;
            static F32 coeff = 1.0f;
            static S32 matno = 0;
            static S32 texno = 0;
            static S32 drawsize = 0;
            static bool showbadtextures = false;

            world = xglobals->sceneCur->env->geom->world;
            S32 nmat = RpWorldGetNumMaterials(world);
            S32 ntex = xSTAssetCountByType('RWTX');
            RwTexture* tex;
            S32 i;

            if (zPad::GetDebugPad()->Pressed(0x80))
                matno--;
            if (zPad::GetDebugPad()->Pressed(0x20))
                matno++;
            if (matno < 0)
                matno = nmat - 1;
            if (matno > nmat - 1)
                matno = 0;
            if (zPad::GetDebugPad()->On(0x10))
                coeff *= 1.01f;
            if (zPad::GetDebugPad()->On(0x40))
                coeff /= 1.01f;
            coeff = CLAMP(coeff, 0.0f, 100.0f);
            if (zPad::GetDebugPad()->Pressed(0x200))
                texno--;
            if (zPad::GetDebugPad()->Pressed(0x2000))
                texno++;
            if (texno < 0)
                texno = ntex - 1;
            if (texno > ntex - 1)
                texno = 0;
            if (zPad::GetDebugPad()->Pressed(0x40000)) {
                drawsize ^= 1;
            }

            if (showbadtextures) {
                xprintf("bad texture list:\n");
                for (i = 0; i < ntex; i++) {
                    PKRAssetTOCInfo xinfo;
                    xSTGetAssetInfoByType('RWTX', i, &xinfo);
                    tex = (RwTexture*)xinfo.mempos;
                    if (tex && (tex->raster->width > 128 || tex->raster->height > 128 || tex->raster->depth > 8)) {
                        xprintf("%03d: %-24s %3dx%3dx%2d\n", i, tex->name, tex->raster->width, tex->raster->height,
                                tex->raster->depth);
                    }
                }
            } else {
                if (nmat > 0) {
                    PKRAssetTOCInfo xinfo;
                    xSTGetAssetInfoByType('RWTX', texno, &xinfo);
                    tex = (RwTexture*)xinfo.mempos;

                    mat = RpWorldGetMaterial(world, matno);
                    S32 fx = RpMatFXMaterialGetEffects(mat);

                    if (zPad::GetDebugPad()->Pressed(0x20000)) {
                        envmode++;
                        if (envmode == 1 || envmode == 2) {
                            U32 flags = 0x1;
                            if (envmode == 2)
                                flags |= 0x10;
                            xFXDebugSetSurfaceFlags(flags);
                            xFXEnvMap(mat, xinfo.aid, coeff);
                        } else {
                            envmode = 0;
                            xFXDebugSetSurfaceFlags(0);
                            RpMatFXMaterialSetEffects(mat, rpMATFXEFFECTNULL);
                        }
                    }

                    if (zPad::GetDebugPad()->Pressed(0x40000)) {
                        if (fx == rpMATFXEFFECTNULL) {
                            xFXBumpMap(mat, xinfo.aid, coeff);
                        } else {
                            RpMatFXMaterialSetEffects(mat, rpMATFXEFFECTNULL);
                        }
                    }

                    if (mat->texture && xprintf_display == 1) {
                        xprintf("material %3d of %3d:  %8s  %3dx%3d, %2d bpp\n", matno, nmat, mat->texture->name,
                                mat->texture->raster->width, mat->texture->raster->height, mat->texture->raster->depth);

                        F32 x, y, dx, dy, size, recipCamZ;

                        recipCamZ = 1.0f / RwCameraGetNearClipPlane(xglobals->screen->icam);

                        xRenderStateBlendAndZModesSet(rwBLENDONE, rwBLENDZERO, FALSE, TRUE);
                        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, mat->texture->raster);

                        size = 96.0f;
                        dx = size * FB_XRES / 640.0f;
                        dy = size * FB_YRES / 480.0f;
                        x = 0.025f * FB_XRES;
                        y = 0.5f * FB_YRES - dy;
                        Im2DRenderQuad(x, y, x + dx, y + dy, RwIm2DGetNearScreenZ(), recipCamZ, 0.0f);

                        if (tex) {
                            xprintf("texture  %3d of %3d:  %s\n%3dx%3d %2d bpp\n", texno, ntex, tex->name,
                                    tex->raster->width, tex->raster->height, tex->raster->depth);

                            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, tex->raster);

                            y += dy;
                            Im2DRenderQuad(x, y, x + dx, y + dy, RwIm2DGetNearScreenZ(), recipCamZ, 0.0f);
                        }

                        xRenderStateZModesSet(TRUE, TRUE);
                        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
                    }

                    xprintf("coeff:  %.3f\n", coeff);
                    switch (RpMatFXMaterialGetEffects(mat)) {
                    case rpMATFXEFFECTENVMAP:
                        if (xFXDebugGetSurfaceFlags() & 0x10)
                            xprintf("env mapped (specular)\n");
                        else
                            xprintf("env mapped\n");
                        break;
                    case rpMATFXEFFECTBUMPMAP:
                        xprintf("bump mapped\n");
                        break;
                    case rpMATFXEFFECTBUMPENVMAP:
                        if (xFXDebugGetSurfaceFlags() & 0x10)
                            xprintf("bump + env (specular)\n");
                        else
                            xprintf("bump + env\n");
                        break;
                    case rpMATFXEFFECTNULL:
                        break;
                    }
                }

                if (ntex > 0) {
                    PKRAssetTOCInfo xinfo;
                    xSTGetAssetInfoByType('RWTX', texno, &xinfo);

                    F32 x, y, dx, dy, size, recipCamZ;

                    tex = (RwTexture*)xinfo.mempos;
                    recipCamZ = 1.0f / RwCameraGetNearClipPlane(xglobals->screen->icam);

                    xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, TRUE, FALSE);

                    size = (drawsize == 0) ? 256.0f : tex->raster->width;
                    dx = size * FB_XRES / 640.0f;
                    dy = size * FB_YRES / 480.0f;
                    x = 0.025f * FB_XRES;
                    y = 0.5f * FB_YRES;

                    if (tex) {
                        const char* name = tex->name[0] ? tex->name : xSTAssetName(xinfo.aid);
                        xprintf("texture  %3d of %3d:  %8s  %3dx%3d, %2d bpp\n", texno, ntex, name, tex->raster->width,
                                tex->raster->height, tex->raster->depth);

                        RwRasterLock(tex->raster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKREAD | rwRASTERLOCKRAW);
                        xprintf("cpPixels:  %08X\n", tex->raster->cpPixels);
                        RwRasterUnlock(tex->raster);

                        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, tex->raster);

                        Im2DRenderQuad(x, y, x + dx, y + dy, RwIm2DGetNearScreenZ(), recipCamZ, 0.0f);
                    }

                    xRenderStateZModesSet(TRUE, TRUE);
                    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
                }
            }
        }
    }
#if RELEASE
    else if (xglobals->showReleaseMemInfo) {
        xSBRender(&relMemInfoSB);
        xSBReset(&relMemInfoSB);
    }
#endif
#endif
}

#if DEBUG || RELEASE
xStats gStats;
xStats gPendingStats;

U32 XDG_XMIN = 20;
U32 XDG_XMAX = 620;
U32 XDG_YHEIGHT = 400;
F32 XDG_XSCALE;

struct DGstate
{
    U32 flags;
    U32 draw;
    char* name[6];
    U32 pick;
    S8 field[6];
};

DGstate gDG;

struct XDG
{
    U16 flags;
    U16 idx;
    F32 value[101];
    F32 scale;
    xColor graphcolor;
    F32 tickmin;
    F32 tickmax;
    F32 tickstep;
    xColor tickcolor;
};

static XDG dg[6];
static xColor dg_color[6];
static char dg_color_str[6][8] = { "red", "green", "blue", "yellow", "orange", "cyan" };

void xDebugGraphSetup(XDG* gp, F32 scale, xColor graphcolor, F32 tickmin, F32 tickmax, F32 tickstep, xColor tickcolor)
{
    xASSERT_L(2272, gp);

    gp->scale = scale;

    gp->idx = 0;
    memset(gp->value, 0, sizeof(gp->value));

    gp->graphcolor = graphcolor;

    gp->tickmin = gp->tickmax = 0.0f;
    gp->tickstep = 0.0f;

    if (tickmin == tickmax)
        tickstep = 0.0f;
    if (tickstep == 0.0f) {
        tickmax = tickmin;
    }

    xASSERT_L(2294, (tickmax == tickmin) || (tickmax > tickmin && tickstep > 0) || (tickmax < tickmin && tickstep < 0));

    gp->tickmin = tickmin;
    gp->tickmax = tickmax;
    gp->tickstep = tickstep;
    gp->tickcolor = tickcolor;

    XDG_XMIN = 0.05f * FB_XRES;
    XDG_XMAX = FB_XRES - XDG_XMIN;
    XDG_YHEIGHT = 0.95f * FB_YRES;
    XDG_XSCALE = (XDG_XMAX - XDG_XMIN) / 100.0f;
}

void xDebugGraphUpdate(XDG* gp, F32 new_value)
{
    xASSERT_L(2312, gp);

    new_value *= gp->scale;
    gp->value[gp->idx] = new_value;
    gp->idx = (gp->idx >= 100) ? 0 : (U16)(gp->idx + 1);
}

void xDebugGraphRender(XDG* gp)
{
    S32 i, idx;
    F32 f;
    xVec3 p0, p1;
    xVec2 pt[101], *vp;
    xASSERT_L(2326, gp);

    if (gp->tickstep) {
        xDrawSetColor(gp->tickcolor);
        for (f = gp->tickmin; f <= gp->tickmax; f += gp->tickstep) {
            p0.x = XDG_XMIN;
            p1.x = XDG_XMAX;
            p0.y = p1.y = XDG_YHEIGHT - (S32)(f * gp->scale);
            xDrawLine2D(p0.x, p0.y, p1.x, p1.y);
        }
    }

    p0.x = XDG_XMIN;
    p0.y = XDG_YHEIGHT - gp->value[gp->idx];
    idx = (gp->idx >= 100) ? 0 : gp->idx + 1;
    xDrawSetColor(gp->graphcolor);

    idx = gp->idx; vp = pt;
    for (i = 0; i < 101; i++, vp++) {
        vp->x = XDG_XMIN + XDG_XSCALE * i;
        vp->y = XDG_YHEIGHT - gp->value[idx];
        idx = (idx >= 100) ? 0 : idx + 1;
    }
    xDrawPolyLine2D(pt, ARRAY_LENGTH(pt));
}

void xDebugGraphInit()
{
    U32 i;
    xColor gray = { 0x40, 0x40, 0x40, 0xFF };

    xColorInit(dg_color[0], 0x80, 0, 0, 0xFF);
    xColorInit(dg_color[1], 0, 0x80, 0, 0xFF);
    xColorInit(dg_color[2], 0, 0, 0x80, 0xFF);
    xColorInit(dg_color[3], 0x80, 0x80, 0, 0xFF);
    xColorInit(dg_color[4], 0x80, 0x40, 0, 0xFF);
    xColorInit(dg_color[5], 0, 0x80, 0x80, 0xFF);

    for (i = 0; i < ARRAY_LENGTH(dg); i++) {
        xDebugGraphSetup(&dg[i], FB_YRES / 16.0f / (OS_TIMER_CLOCK * (1.0f / VBLANKS_PER_SEC)), dg_color[i], 0.0f,
                         3.1f * (OS_TIMER_CLOCK * (1.0f / VBLANKS_PER_SEC)), OS_TIMER_CLOCK * (1.0f / VBLANKS_PER_SEC), gray);
    }

    gDG.flags = 0x4;
    gDG.pick = 0;
    gDG.field[0] = 5;
    gDG.field[1] = 12;
    gDG.field[2] = 13;
    gDG.field[3] = 14;
    gDG.draw = 0x1F;
}

struct xDGsort
{
    S64 val;
    U32 idx;
};

static S32 dgcmp(const void* A, const void* B)
{
    xDGsort* a = (xDGsort*)A;
    xDGsort* b = (xDGsort*)B;

    if (a->val > b->val)
        return -1;
    if (a->val < b->val)
        return 1;
    return 0;
}

static void xDebugGraphSort()
{
    U32 i;
    xDGsort* list;
    list = (xDGsort*)xMEMPUSHTEMP_L(sizeof(xDGsort) * ARRAY_LENGTH(xStatsFieldName), 0, 2443);

    S64* xp = (S64*)&gStats;
    for (i = 0; i < 35; i++) {
        list[i].val = *xp++;
        list[i].idx = i;
    }

    qsort(list, ARRAY_LENGTH(xStatsFieldName), sizeof(xDGsort), dgcmp);

    gDG.field[0] = list[1].idx;
    gDG.field[1] = list[2].idx;
    gDG.field[2] = list[3].idx;
    gDG.field[3] = list[4].idx;

    xMEMPOPTEMP(list);
}

static S32 g_xgdb_usage;

void xDebugGraphUsageMsg()
{
    DBprintf(DBML_DISP, "#        R2 - Toggle Usage\n");
    if (g_xgdb_usage) {
        DBprintf(DBML_DISP, "#        L2 - Toggle Select Mode\n");

        if (gDG.flags & 0x8) {
            DBprintf(DBML_DISP, "#-----Select:\n");
            DBprintf(DBML_DISP, "#        Up - select above\n");
            DBprintf(DBML_DISP, "#      Down - select below\n");
            DBprintf(DBML_DISP, "#      Left - [unused]\n");
            DBprintf(DBML_DISP, "#     Right - [unused]\n");
            DBprintf(DBML_DISP, "#       Tri - 4 worst offenders\n");
            DBprintf(DBML_DISP, "#         O - next field\n");
            DBprintf(DBML_DISP, "#         X - toggle graph on/off\n");
            DBprintf(DBML_DISP, "#        SQ - prev field\n");
            DBprintf(DBML_DISP, "Left Analog - supposedly faster selection\n");
        } else {
            DBprintf(DBML_DISP, "#-----Normal:\n");
            DBprintf(DBML_DISP, "#        Up - increase scale\n");
            DBprintf(DBML_DISP, "#      Down - decrease scale\n");
            DBprintf(DBML_DISP, "#      Left - [unused]\n");
            DBprintf(DBML_DISP, "#     Right - [unused]\n");
            DBprintf(DBML_DISP, "#       Tri - toggle additive\n");
            DBprintf(DBML_DISP, "#         O - toggle legend\n");
            DBprintf(DBML_DISP, "#         X - [unused]\n");
            DBprintf(DBML_DISP, "#        SQ - toggle always draw\n");
        }
    }
}

void xDebugGraphService()
{
    U32 i, j;
    S64* ptr;
    F32 x, y, angle, mag, scale;
    U32 stack, val, stack_it, n;

    if (xDebugModeGet() == 9) {
        if (zPad::GetDebugPad()->Pressed(0x2000)) g_xgdb_usage = !g_xgdb_usage;

        if (zPad::GetDebugPad()->Pressed(0x200)) {
            if (gDG.flags & 0x8) gDG.flags &= ~0x8;
            else gDG.flags |= 0x8;
        }

        if (gDG.flags & 0x8) {
            if (zPad::GetDebugPad()->Pressed(0x10))
                gDG.pick = (gDG.pick == 0) ? 3 : gDG.pick - 1;
            if (zPad::GetDebugPad()->Pressed(0x40)) {
                gDG.pick = (gDG.pick == 3) ? 0 : gDG.pick + 1;
            }

            zPad::GetDebugPad()->GetAnalog1()->Get(x, y, angle, mag);
            if (mag >= 0.7f) {
                angle *= 1.0f / (2.0f * PI);
                gDG.field[gDG.pick] = angle * mag;
                if (gDG.field[gDG.pick] == mag) {
                    gDG.field[gDG.pick] = -1;
                }
            }

            if (zPad::GetDebugPad()->Pressed(0x20000)) gDG.field[gDG.pick]++;
            if (zPad::GetDebugPad()->Pressed(0x80000)) gDG.field[gDG.pick]--;
            if (gDG.field[gDG.pick] < 0) gDG.field[gDG.pick] = mag - 1.0f;
            if (gDG.field[gDG.pick] >= mag) gDG.field[gDG.pick] = 0;

            if (zPad::GetDebugPad()->Pressed(0x10000)) gDG.draw ^= 1 << gDG.pick;
            if (zPad::GetDebugPad()->Pressed(0x40000)) xDebugGraphSort();
        } else {
            if (zPad::GetDebugPad()->On(0x10) || zPad::GetDebugPad()->On(0x40)) {
                scale = zPad::GetDebugPad()->On(0x10) ? 1.05f : 0.952381f;
                for (i = 0; i < ARRAY_LENGTH(dg); i++) {
                    dg[i].scale *= scale;
                    for (j = 0; j < 101; j++)
                        dg[i].value[j] *= scale;
                }
            }

            if (zPad::GetDebugPad()->Pressed(0x20000)) gDG.flags ^= 0x4;
            if (zPad::GetDebugPad()->Pressed(0x40000)) gDG.flags ^= 0x2;
            if (zPad::GetDebugPad()->Pressed(0x80000)) gDG.flags ^= 0x1;
        }

        if (gDG.flags & 0x2)
            xprintf("additive\n");
        if (gDG.flags & 0x1) {
            xprintf("always draw\n");
        }

        xDebugGraphUsageMsg();
    }

    stack = 0;

    for (i = 0; i < 4; i++) {
        if (gDG.field[i] != -1) {
            ptr = (S64*)((U32)&gStats + gDG.field[i] * sizeof(S64));
            val = *ptr;
            stack_it = (gDG.flags & 0x2) && (gDG.draw & (1 << i));
            if (gDG.field[i] == 0)
                stack_it = 0;
            if (stack_it) {
                stack += val;
                xDebugGraphUpdate(&dg[i], stack);
            } else
                xDebugGraphUpdate(&dg[i], val);
        }
    }

    xDebugGraphUpdate(&dg[4], (U32)gStats.cpu);

    if (xDebugModeGet() == 9 || (gDG.flags & 0x1)) {
        for (i = 0; i < ARRAY_LENGTH(dg); i++)
            if (gDG.draw & (1 << i)) {
                if (xDebugModeGet() == 9 && (gDG.flags & 0x4)) {
                    if (i < 4) {
                        val = gDG.field[i];
                        if ((gDG.flags & 0x8) && i == gDG.pick && (gFrameCount & 0x1F) < 16) {
                            xprintf("%-6s -\n", dg_color_str[i]);
                        } else {
                            ptr = (S64*)((U32)&gStats + gDG.field[i] * sizeof(S64));
                            n = *ptr;
                            xprintf("%-6s - %2d %-16s %7d  %2d\n", dg_color_str[i], val, xStatsFieldName[val], n,
                                    (U32)(100.0f * n / (U32)gStats.cpu));
                        }
                    }
                }
                xDebugGraphRender(&dg[i]);
            }
    }

    if (xDebugModeGet() == 9) {
        xDebugGraphSort();
    }
}

#endif

void xDebugInit()
{
#if DEBUG || RELEASE
    sDebugModeData[0].callback = NULL;
    sDebugModeData[0].name = "";
    sDebugModeData[0].virtualID = 0;

    sDebugModeLimit = 1;
    sDebugModeNextVirtualID = 1;
    for (S32 i = 1; i < 20; i++)
        xDebugModeAdd(dm_names[i], dm_callbacks[i]);

    xDebugGraphInit();

    sScreenCaptureNames[0] = "Native";
    char* p = sScreenCaptureNamesBuffer;
    for (U32 i = 1; i < ARRAY_LENGTH(sScreenCaptureModes); i++) {
        sprintf(p, "%dx%d", sScreenCaptureModes[i].width, sScreenCaptureModes[i].height);
        sScreenCaptureNames[i] = p;
        p += strlen(p) + 1;
    }
    xTWEAK_SELECT("Screen Capture|Size", &sScreenCaptureMode, (const char**)sScreenCaptureNames, NULL, ARRAY_LENGTH(sScreenCaptureNames), NULL, NULL, 0);
    static tweak_callback cb = tweak_callback::create_change(TakeScreenShotCallback);
    cb.convert_mem_to_tweak = NextNameScreenShotCallback;
    xTWEAK_MESSAGE("Screen Capture|Take Screenshot", "", &cb, NULL, 0);
#endif
}

void xDebugUpdate()
{
#if DEBUG || RELEASE
    xDebugModeService();

    xDebugGraphService();

    iDebugUpdate();
#endif
}

void xDebugExit()
{
}

#if DEBUG || RELEASE
static U32 gReadBytes;
static F32 gReadTime = 0.00001f;

void xDebugReportLoadStats(char* msg, U32 flags)
{
    char out[128];

    sprintf(out, "load stats:  %7.2f KB read in %7.4f sec (%7.2f KB/sec)\n", gReadBytes / 1024.0f, gReadTime,
            gReadBytes / 1024.0f / gReadTime);
    iprintf(out);

    if (flags & 0x2) {
        xASSERT_L(2840, msg != NULL);
        sprintf(out, "%s:  %7.4f sec\n", msg, iTimeDiffSec(0, iTimeGet()));
        iprintf(out);
    }

    if (flags & 0x1) {
        gReadBytes = 0;
        gReadTime = 0.0f;
    }
}

#endif

void xDebugTimestampScreen()
{
#if DEBUG || RELEASE
    RwCamera* cam = iCameraCreate(FB_XRES, FB_YRES, FALSE);
    RwRGBA bg = { 0, 0, 0, 0 };

    xtextbox tb = xtextbox::create(xfont::create(0, NSCREENX(21.0f), NSCREENY(22.0f), 0.0f, g_WHITE), screen_bounds, 0);
    xColorInit(tb.font.color, 0xC0, 0xC0, 0xC0, 0xFF);
    tb.flags |= 0x2;
    tb.bounds.assign(0.0f, 0.4125f, 1.0f, 0.25f);
    tb.bounds.contract(0.01f);

    tb.set_text(timestamp);
    tb.bounds.h = tb.yextent(true);
    tb.bounds.y = 0.5f - 0.5f * tb.bounds.h;
    tb.font.clip = tb.bounds;
    tb.font.clip.expand(0.01f);

    for (U32 i = 0; i < 5; i++) {
        RwCameraClear(cam, &bg, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);
        RwCameraBeginUpdateWrapper(cam);

        xColor c = { 0x40, 0x40, 0x40, 0xFF };
        render_fill_rect(tb.font.clip, c);
        tb.render(false);

        RwCameraEndUpdate(cam);

        if (i < 2) {
            RwCameraClear(cam, &bg, rwCAMERACLEARIMAGE);
        }

        RwCameraShowRaster(cam, NULL, 1);
    }

    iCameraDestroy(cam);
#endif
}

#if DEBUG || RELEASE
struct OldRenderState
{
    S32 fogenable;
    S32 vertexalphaenable;
    S32 zwriteenable;
    S32 ztestenable;
    S32 cullmode;
    RwBlendFunction srcblend;
    RwBlendFunction destblend;
    U32 shademode;
    RwTextureFilterMode filter;
};

OldRenderState sOldRenderState;

void xDebugAssertStartup()
{
    g_visass_cam = iCameraCreate(FB_XRES, FB_YRES, FALSE);
}

void xDebugAssertShutdown()
{
    iCameraDestroy(g_visass_cam);
    g_visass_cam = NULL;
}

void xDebugAssertScreen(const char* str1, const char* str2, const char* pretty_str)
{
    RwCamera* cam, *ccam;
    if (xglobals->dumpCutscene) {
        return;
    }

    static bool in_this_function = false;

    RwRGBA bg = { 64, 0, 0, 0 };
    char str[2048];
    char* c;

    ccam = RwCameraGetCurrentCamera();
    if (ccam) {
        RwCameraEndUpdate(ccam);
    }

    sprintf(str, "%s\n%s\n", str1, str2);
    printf("%s", str);
    sprintf(str, "%s\n%s\n", pretty_str, str2);

    for (c = str; *c; c++) {
        if (*c == '\\') {
            *c = '/';
        }
    }

    cam = g_visass_cam;
    if (in_this_function) {
        fprintf(stdout, "*** Oi! Recursive ASSERT averted, amigo.\n");
        return;
    }

    if (cam == NULL) {
        fprintf(stdout, "!!! ERROR !!! Unable to display Visual Assert\n");
        return;
    }

    in_this_function = true;

    xtextbox tb = xtextbox::create(xfont::create(1, NSCREENX(19.0f), NSCREENY(22.0f), 0.0f, g_WHITE), screen_bounds, 0);
    xColorInit(tb.font.color, 0xC0, 0xC0, 0xC0, 0xFF);
    tb.flags |= 0x2;
    tb.bounds.assign(0.0f, 0.4125f, 1.0f, 0.25f);
    tb.bounds.contract(0.025f);
    tb.set_text(str);
    tb.bounds.h = tb.yextent(true);
    tb.bounds.y = 0.5f - 0.5f * tb.bounds.h;
    tb.font.clip = tb.bounds;
    tb.font.clip.expand(0.025f);

    s_visass_y[0] = tb.font.clip.y;
    s_visass_y[1] = tb.font.clip.y + tb.font.clip.h;

    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &sOldRenderState.fogenable);
    xRenderStateBlendModesGet(&sOldRenderState.srcblend, &sOldRenderState.destblend);
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &sOldRenderState.vertexalphaenable);
    RwRenderStateGet(rwRENDERSTATESHADEMODE, &sOldRenderState.shademode);
    RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &sOldRenderState.zwriteenable);
    RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &sOldRenderState.ztestenable);
    RwRenderStateGet(rwRENDERSTATECULLMODE, &sOldRenderState.cullmode);
    RwRenderStateGet(rwRENDERSTATETEXTUREFILTER, &sOldRenderState.filter);

    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    xRenderStateBlendAndZModesSet(rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, FALSE, FALSE);

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEFLAT);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

    for (U32 i = 0; i < 4; i++) {
        RwCameraClear(cam, &bg, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);
        RwCameraBeginUpdateWrapper(cam);

        render_fill_rect(tb.font.clip, g_BLACK);
        tb.render(false);

        RwCameraEndUpdate(cam);

        RwCameraShowRaster(cam, NULL, 1);
    }

    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)sOldRenderState.fogenable);
    xRenderStateBlendAndZModesSet(sOldRenderState.srcblend, sOldRenderState.destblend, sOldRenderState.zwriteenable,
                                  sOldRenderState.ztestenable);

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)sOldRenderState.vertexalphaenable);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)sOldRenderState.shademode);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)sOldRenderState.filter);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)sOldRenderState.cullmode);

    if (ccam) {
        RwCameraBeginUpdateWrapper(ccam);
    }

    in_this_function = false;
}

void xDebugStackTrace()
{
    iDebugStackTrace();
    xSystem_GapTrackReport();
}

void debug_mode_page()
{
    static S32 page = 0;

    static S32 selected = 0;

    if (page * 20 >= sDebugModeLimit) {
        page = 0;
    }

    static S32 delay_switch = 0;
    if (zPad::GetDebugPad()->Pressed(0x10)) {
        selected--;
        delay_switch = 0;
    }
    else if (zPad::GetDebugPad()->Pressed(0x40)) {
        selected++;
        delay_switch = 0;
    }
    else if (zPad::GetDebugPad()->On(0x10)) {
        if (delay_switch++ > 12) {
            selected--;
            delay_switch = 0;
        }
    }
    else if (zPad::GetDebugPad()->On(0x40)) {
        if (delay_switch++ > 12) {
            selected++;
            delay_switch = 0;
        }
    }
    else if (zPad::GetDebugPad()->Pressed(0x80)) {
        selected -= 20;
        delay_switch = 0;
    }
    else if (zPad::GetDebugPad()->Pressed(0x20)) {
        selected += 20;
        delay_switch = 0;
    }
    else if (zPad::GetDebugPad()->Pressed(0x200)) {
        page += 2;
        delay_switch = 0;
    }
    else if (zPad::GetDebugPad()->Pressed(0x2000)) {
        page += 2;
        delay_switch = 0;
    }
    else delay_switch = 0;

    while (selected < 0) {
        selected += 20;
        page--;
    }
    while (selected >= 40) {
        selected -= 20;
        page++;
    }
    if (selected + page * 20 >= sDebugModeLimit) {
        selected = sDebugModeLimit - page * 20 - 1;
    }

    if (page < 0)
        page = 0;
    S32 numPages = (sDebugModeLimit + 19) / 20;
    if (page == numPages - 1) {
        if (selected > 20) {
            if (page > 0)
                page--;
            else selected -= 20;
        }
        else if (page > 0) {
            page--;
            selected += 20;
        }
    }

    if (page >= numPages) {
        page = numPages - 1;
    }

    if (zPad::GetDebugPad()->Pressed(0x10000)) {
        S32 mode = selected + page * 20;
        xDebugModeSet(sDebugModeData[mode].virtualID);
        return;
    }

    xprintf("\n");

    for (S32 i = 0; i < 20; i++) {
        S32 aItem = i + page * 20;
        char a[18];
        memset(a, 0, sizeof(a));
        if (aItem < sDebugModeLimit)
            strncpy(a, sDebugModeData[aItem].name, sizeof(a) - 1);
        bool aSel = (selected == i);
        S32 bItem = aItem + 20;
        char b[18];
        memset(b, 0, sizeof(b));
        if (bItem < sDebugModeLimit)
            strncpy(b, sDebugModeData[bItem].name, sizeof(b) - 1);
        bool bSel = (selected == i + 20);

        xprintf("%s%-18s%s %s%-18s%s\n", aSel ? "<" : "{nbsp}", a, aSel ? ">" : " ", bSel ? "<" : " ", b,
                bSel ? ">" : " ");
    }
}

struct TexInfo
{
    const char* name;
    U32 size;
    U32 firstPixel;
    U32 width : 13;
    U32 height : 13;
    U32 depth : 6;
};

static S32 texcmp(const void* A, const void* B)
{
    TexInfo* a = (TexInfo*)A;
    TexInfo* b = (TexInfo*)B;

    if (a->size > b->size)
        return -1;
    if (a->size < b->size) {
        return 1;
    }

    return strcmp(a->name, b->name);
}

void xDebugTextureReport()
{
    S32 i;
    if (gGameState != 1) {
        return;
    }

    S32 ntex = xSTAssetCountByType('RWTX');
    if (ntex <= 0) {
        return;
    }

    TexInfo* texlist = (TexInfo*)xMEMPUSHTEMP_L(ntex * sizeof(TexInfo), 0, 3220);
    if (texlist == NULL) {
        iprintf("ERROR -- not enough free RAM to dump texture report!\n");
        return;
    }
    memset(texlist, 0, ntex * sizeof(TexInfo));

    TexInfo* tp = texlist;
    for (i = 0; i < ntex; i++, tp++) {
        PKRAssetTOCInfo xinfo;
        xSTGetAssetInfoByType('RWTX', i, &xinfo);
        RwTexture* tex = (RwTexture*)xinfo.mempos;

        if (tex == NULL) {
            tp->name = "- lookup failed -";
            tp->width = 0;
            tp->height = 0;
            tp->depth = 0;
            tp->size = 0;
        } else {
            RwRaster* raster = tex->raster;
            tp->name = tex->name[0] ? tex->name : xSTAssetName(xinfo.aid);
            tp->width = raster->width;
            tp->height = raster->height;
            tp->depth = raster->depth;
            if (raster->cpPixels)
                tp->firstPixel = *(U32*)raster->cpPixels;
            else
                tp->firstPixel = 0;

            U32 palette_size = 0;
            switch (tp->depth) {
            case 4:
                tp->size = tp->width * tp->height / 2;
                palette_size = 16 * 4;
                break;
            case 8:
                tp->size = tp->width * tp->height;
                palette_size = 256 * 4;
                break;
            case 16:
                tp->size = tp->width * tp->height * 2;
                break;
            case 32:
                tp->size = tp->width * tp->height * 4;
                break;
            default:
                iprintf("Funky bit depth:  %dx%dx%dbpp?!?\n", tp->width, tp->height, tp->depth);
                tp->size = 0;
                break;
            }

            tp->size += palette_size;
        }
    }

    qsort(texlist, ntex, sizeof(TexInfo), texcmp);

    char str[8];
    SCENEIDSTR(xglobals->sceneCur->sceneID, str);
    iprintf("-------------- %s texture report --------------\n", str);
    iprintf("-size-  --dim----------  --name------------------\n");
    tp = texlist;
    U32 total = 0;
    for (i = 0; i < ntex; i++, tp++) {
        iprintf("%-6d  %3dx%3d, %2d bpp  0x%08X  %-24s\n", tp->size, tp->width, tp->height, tp->depth, tp->firstPixel,
                tp->name);
        total += tp->size;
        if ((i & 0x1F) == 0)
            iDebugVSync();
    }
    iprintf("---- %s texture summary -----------------------\n", str);
    iprintf("%d total textures, %d bytes (% 6.3f MB)\n", ntex, total, total / 1048576.0f);
    iprintf("-------------------------------------------------\n");

    xMEMPOPTEMP(texlist);
}

static S32 sndcmp(const void* A, const void* B); // UNUSED
static U32 found(U32 num, U32* list); // UNUSED
static void SWAP16(S32 n, void* vaddr);
static void SWAP32(S32 n, void* vaddr);
static void xDebugDumpImage(const char* pDestFile, RwImage* pImage); // UNUSED
static void xDebugDumpImageRAW(const char* pDestFile, RwImage* pImage); // UNUSED

enum xTexDumpType
{
    xTexDump_BMP,
    xTexDump_RAW
};

static void xDebugDumpTexture(const char* pDestFile, RwTexture* texture, xTexDumpType type); // UNUSED
static void xDebugDumpTexture(const char* pDestFile, U32 textureID, xTexDumpType type); // UNUSED

DECOMP_FORCEACTIVE("((U32)vaddr & 1) == 0", "((U32)vaddr & 3) == 0", "Dumping buffer to '%s'\n",
                   "n == sizeof(BmpMegaHeader) - sizeof(mhdr.prepad)", "n == bufsize", "ticks:  %d  %6.4f s\n", ".raw",
                   ".bmp")
#endif

struct xMtx4x3
{
    union
    {
        F32 m_a[16];
        F32 m_aa[4][4];
        xMat3x3 m_xMat3x3;
        xMat4x3 m_xMat4x3;
        RwMatrix m_RwMatrix;
        struct
        {
            xVec3 right;
            S32 flags;
            xVec3 up;
            U32 pad1;
            xVec3 at;
            U32 pad2;
            xVec3 pos;
            U32 pad3;
        };
    };
};

#if DEBUG || RELEASE
typedef struct _xDrawTextInfo_t
{
    char* pszText;
    xVec3 vPosWS;
    F32 fWidth;
    F32 fHeight;
} xDrawTextInfo;

static xDrawTextInfo* _pInfo;
static xSB _xDrawTextSB;
static U32 _nDrawTextInfo;
static U32 _nDrawTextInfoMaxLines = 1;

void xDrawTextInit(U32 uBufferSize, U32 uNumLinesMax)
{
    if (uBufferSize == 0 || uNumLinesMax == 0) {
        _nDrawTextInfo = 0;
        _nDrawTextInfoMaxLines = 1;
        return;
    }

    _nDrawTextInfoMaxLines = uNumLinesMax;
    xSBCreate(&_xDrawTextSB, uBufferSize);
    _pInfo = (xDrawTextInfo*)RwMallocL(_nDrawTextInfoMaxLines * sizeof(xDrawTextInfo), rwMEMHINTDUR_NADURATION, 3679);
    _nDrawTextInfo = 0;
}

void xDrawTextExit()
{
    xSBDestroy(&_xDrawTextSB);
    RwFree(_pInfo);
    _pInfo = NULL;
}

void xDrawTextRender()
{
    if (!_pInfo) {
        return;
    }

    xColor color = { 0xFF, 0xFF, 0x00, 0xC8 };
    xVec2 vUnitScreen;

    for (U32 i = 0; i < _nDrawTextInfo; i++) {
        if (xDrawTextComputeScreenPt(vUnitScreen, _pInfo[i].vPosWS)) {
            xfont font = xfont::create(1, NSCREENX(_pInfo[i].fWidth), NSCREENY(_pInfo[i].fHeight), 0.0f, g_WHITE);
            font.color = color;
            basic_rect<F32> bounds = font.bounds(_pInfo[i].pszText);
            font.render(_pInfo[i].pszText, vUnitScreen.x - 0.5f * bounds.w - bounds.x, vUnitScreen.y - bounds.y);
        }
    }
    _nDrawTextInfo = 0;
    xSBReset(&_xDrawTextSB);
}

void xDrawText(const xVec3& vWorld, const char* fmt, ...)
{
    if (!_pInfo) {
        return;
    }

    va_list marker;
    g_lprbuf[0] = '\0';
    va_start(marker, fmt);
    vsprintf(g_lprbuf, fmt, marker);
    va_end(marker);
    xDrawText(vWorld, 16.0f, 16.0f, g_lprbuf);
}

void xDrawText(const xVec3& vWorld, F32 fWidth, F32 fHeight, const char* pszText)
{
    if (!_pInfo) {
        return;
    }

    xDrawTextInfo* pIn = &_pInfo[_nDrawTextInfo]; _nDrawTextInfo++;
    if (_nDrawTextInfo >= _nDrawTextInfoMaxLines)
        _nDrawTextInfo = _nDrawTextInfoMaxLines - 1;
    pIn->fWidth = fWidth;
    pIn->fHeight = fHeight;
    pIn->vPosWS = vWorld;
    pIn->pszText = xSBAppend(&_xDrawTextSB, pszText);
    _xDrawTextSB.cur++;
}
#else
void xDrawTextInit(U32, U32)
{
}

void xDrawTextRender()
{
}

void xDrawText(const xVec3&, const char*, ...)
{
}

void xDrawText(const xVec3&, F32, F32, const char*)
{
}
#endif

bool xDrawTextComputeScreenPt(xVec2& vUnitScreen, const xVec3& vWorldSpace)
{
    RwCamera* pCam = RwCameraGetCurrentCamera();
    if (!pCam) {
        return false;
    }

    RwReal fNearClip = RwCameraGetNearClipPlane(pCam);
    RwReal fFarClip = RwCameraGetFarClipPlane(pCam);

    xMtx4x3 mtxView;
    mtxView.m_RwMatrix = *RwCameraGetViewMatrix(pCam);

    xVec3 vCameraSpace;
    xVec3 vScreenSpace;
    RwV3dTransformPoints((RwV3d*)&vCameraSpace, (const RwV3d*)&vWorldSpace, 1, &mtxView.m_RwMatrix);
    F32 fRecipZ = 1.0f / vCameraSpace.z;
    vScreenSpace.x = pCam->frameBuffer->width * (vCameraSpace.x * fRecipZ) + pCam->frameBuffer->nOffsetX;
    vScreenSpace.y = pCam->frameBuffer->height * (vCameraSpace.y * fRecipZ) + pCam->frameBuffer->nOffsetY;
    vScreenSpace.z = fRecipZ * pCam->zScale + pCam->zShift;

    vUnitScreen.x = NSCREENX(vScreenSpace.x);
    vUnitScreen.y = NSCREENY(vScreenSpace.y);

    if (vScreenSpace.x < 0.0f || vScreenSpace.y < 0.0f || vUnitScreen.x > 1.0f || vUnitScreen.y > 1.0f ||
        vCameraSpace.z < fNearClip || vCameraSpace.z > fFarClip) {
        return false;
    }

    return true;
}

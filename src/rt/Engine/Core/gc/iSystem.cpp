#include "xCommon.h"
#include "iSystem.h"

#include "xFX.h"
#include "xMath.h"
#include "xPad.h"
#include "xRenderState.h"
#include "xShadow.h"
#include "xSndMgr.h"
#include "xString.h"
#include "xstransvc.h"
#include "iAsync.h"
#include "iCamera.h"
#include "iDebug.h"
#include "iFile.h"
#include "iFMV.h"
#include "iMemMgr.h"
#include "iPad.h"
#include "iSnd.h"
#include "iTime.h"
#include "iTRC.h"
#include "water/texcache.h"
#include "zMain.h"

#include "decomp.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rpcollis.h>
#include <rpmatfx.h>
#include <rpskin.h>
#include <rphanim.h>
#include <rpusrdat.h>
#include <rpptank.h>
#include <dolphin.h>
#include <stdio.h>
#include <string.h>

#define MAX_ARAM_BLOCKS 32

struct AramAllocNode
{
    U32 pAramAddress;
    U32 uAllocByteCount;
    U32 uFlags;
};

struct GCAlarm
{
    OSAlarm alarm;
    OSThread* thread;
};

static U32 iRenderWareInit1();
static U32 iRenderWareInit2();
#if DEBUG || RELEASE
static void iSystemRWDebugModeCB();
#endif
static void NewRwDebugHandler(RwDebugType type, const RwChar* string);
static RwTexture* TextureRead(const RwChar* name, const RwChar* maskName);

void* hackResourceArenaPointer;
static RwDebugHandler oldDebugHandler;
RwVideoMode sVideoMode;

extern "C" {
static void __mystart(); // UNUSED
}

S32 iRwResourcesEmptyArena()
{
    RwInt32 arena_size = RwResourcesGetArenaSize();
    RwResEntry* entry = RwResourcesAllocateResEntry(NULL, NULL, arena_size, NULL);
    while (entry == NULL && arena_size > 0) {
        arena_size -= 1024; entry = RwResourcesAllocateResEntry(NULL, NULL, arena_size, NULL);
    }
    if (entry) {
        U32* p = (U32*)(entry + 1);
        for (S32 i = 0; i < arena_size / sizeof(U32); i++) {
            p[i] = 0xDEADBEEF;
        }
        RwResourcesFreeResEntry(entry);
        return 1;
    }
    return 0;
}

void psDebugMessageHandler(RwDebugType, const RwChar* str)
{
#ifdef RWDEBUG
    OSReport("%s\n", str);
#endif
}

void iVSync()
{
    VIWaitForRetrace();
}

S32 spew_warnings;

static void myvcb(GXWarningLevel level, u32 id, const char* msg)
{
    if (spew_warnings) {
        OSReport("Level %d, Warning $03d: %s\n", level, id, msg);
    }
}

GXDrawSyncCallback old_dsc;

void my_dsc(U16 token)
{
    (void)token;
}

static void* xfb1;
static void* xfb2;

class StackCrawl
{
public:
    U32 addresses[16];
    S32 numLevels;

    StackCrawl(); // UNUSED
    void Dump(); // UNUSED
};

#if RELEASE
DECOMP_FORCEACTIVE("%08x: %s + 0x%X\n", "%08x\n")
#endif
DECOMP_FORCEACTIVE(" ", "-- Build %d Exception %d (context %08X) --", "-- Build %d Exception %d (context %08X) --\n",
                   "LR    = %08X     CR = %08X", "LR    = %08X     CR = %08X\n", "SRR0  = %08X   SRR1 = %08X",
                   "SRR0  = %08X   SRR1 = %08X\n", "DSISR = %08X    DAR = %08X", "DSISR = %08X    DAR = %08X\n",
                   "%08x: %s + 0x%X\n", "%08x\n", "F%-2d   = %08X    F%-2d = %08X",
                   "F%-2d   = %08X    F%-2d = %08X\n", "R%-2d   = %08X    R%-2d = %08X", "\n",
                   "\nInstruction at 0x%X (read from SRR0) attempted to access invalid address 0x%X (read from DAR)\n")

static void exception_info(OSError error, OSContext* context, u32 dsisr, u32 dar, void* xfb); // UNUSED
static void MyErrorHandler(OSError error, OSContext* context, u32 dsisr, u32 dar); // UNUSED
static void exception_out_of_mem_info(const char*, U32, void*, const char*); // UNUSED

void iExceptionMemCrash(const char* location, U32 size, const char* optionalString)
{
}

static void FloatingPointErrorHandler(OSError error, OSContext* context, u32 dsisr, u32 dar); // UNUSED

DECOMP_FORCEACTIVE("FPE: Invalid operation: ", "SNaN\n", "Infinity - Infinity\n", "Infinity / Infinity\n", "0 / 0\n",
                   "Infinity * 0\n", "Invalid compare\n", "Software request\n", "Invalid square root\n",
                   "Invalid integer convert\n", "FPE: Overflow\n", "FPE: Underflow\n", "FPE: Zero division\n",
                   "FPE: Inexact result\n")

static void TRCInit()
{
    iTRCDisk::Init();
    iTRCDisk::SetPadStopRumblingFunction(iPadStopRumble);
    iTRCDisk::SetSndSuspendFunction(iSndSuspend);
    iTRCDisk::SetSndResumeFunction(iSndResume);
    iTRCDisk::SetMovieSuspendFunction(iFMVSuspend);
    iTRCDisk::SetMovieResumeFunction(iFMVResume);
    iTRCDisk::SetAsyncLoadSuspendFunction(iAsyncSuspend);
    iTRCDisk::SetAsyncLoadResumeFunction(iAsyncResume);
    ResetButton::SetSndKillFunction(iSndKill);
    ResetButton::SetMovieKillFunction(iFMVKill);
}

void iSystemGetWidthHeight(U32& uWidth, U32& uHeight, bool bPALMode)
{
    if (bPALMode) {
        uWidth = 512;
        uHeight = 528;
    } else {
        uWidth = 512;
        uHeight = 480;
    }
}

void iSystemInit(U32)
{
    GXSetMisc(GX_MT_XF_FLUSH, 8);
    OSInit();
    DVDInit();
    VIInit();
    xDebugInit();
    xMemInit();
    iFileInit();
    iTimeInit();
    xPadInit();
    iRenderWareInit1();
    xSndMgrInit();
    TRCInit();
#if DEBUG
    GXSetVerifyCallback((GXVerifyCallback)myvcb);
#endif
    xMathInit();
    xMath3Init();
}

void iSystemInitGraphics()
{
    iRenderWareInit2();
    TexCacheInit();
    xShadowInit();
    xFXInit();
#if DEBUG || RELEASE
    xDebugModeAdd("DM_RENDERWARE", iSystemRWDebugModeCB);
    iDebugInit();
#else
    old_dsc = GXSetDrawSyncCallback(my_dsc);
#endif
}

void iSystemExit()
{
    xDebugExit();
    xMathExit();
    iSystemRWExit();
    xSndMgrExit();
    xPadKill();
    iFileExit();
    iTimeExit();
    xMemExit();
    OSHalt_L("bye", RELEASE ? 836 : 794);
}

#if DEBUG || RELEASE
static void iSystemRWDebugModeCB()
{
    RwInt32 size, used;

    xprintf("Res: (%d x %d x %d)\n", sVideoMode.width, sVideoMode.height, sVideoMode.depth);
    xprintf("DeepZBuffer = %d\n", 0);
    size = RwResourcesGetArenaSize();
    used = RwResourcesGetArenaUsage();
    xprintf("\nResource Arena\n\n");
    xprintf("     Size  = %dK\n", size / 1024);
    xprintf("     Usage = %dK\n", used / 1024);
    xprintf("     Free  = %dK\n", (size - used) / 1024);
}
#endif

static U32 RWAttachPlugins()
{
    if (!RpWorldPluginAttach()) {
        return 1;
    }
    if (!RpCollisionPluginAttach()) {
        return 1;
    }
    if (!RpMatFXPluginAttach()) {
        return 1;
    }
    if (!RpSkinPluginAttach()) {
        return 1;
    }
    if (!RpHAnimPluginAttach()) {
        return 1;
    }
    if (!RpUserDataPluginAttach()) {
        return 1;
    }
    if (!RpPTankPluginAttach()) {
        return 1;
    }
    return 0;
}

#if DEBUG
static U8 debugMemoryDoAsserts = 1;
static S32 debugMallocBlockCount;
static void* debugBlockArray[10000];
static U32 debugBlockSizeArray[10000];
static S32 debugMallocBlockCountSave;
static void* debugBlockArraySave[10000];
static U32 debugBlockSizeArraySave[10000];

static void* debugMalloc(unsigned long size, U32); // UNUSED
static void debugFree(void* mem); // UNUSED
static void* debugRealloc(void* mem, unsigned long newSize, U32); // UNUSED
static void* debugCalloc(unsigned long numObj, unsigned long sizeObj, U32); // UNUSED

DECOMP_FORCEACTIVE("%p: %d %d\n", "retVal", "OUT OF MEMORY\nallocating %d bytes",
                   "FATAL ERROR - freeing a block that was never allocated\n", "*always*", "%s",
                   "FATAL ERROR - reallocing a block that was never allocated\n", "0")

void debugMallocSave()
{
    memcpy(debugBlockArraySave, debugBlockArray, debugMallocBlockCount * sizeof(void*));
    memcpy(debugBlockSizeArraySave, debugBlockSizeArray, debugMallocBlockCount * sizeof(U32));
    debugMallocBlockCountSave = debugMallocBlockCount;
}

void debugMallocDiff()
{
    for (U32 j = 0; j < debugMallocBlockCount; j++) {
        U8 found = 0;
        for (U32 i = 0; i < debugMallocBlockCountSave; i++) {
            if (debugBlockArray[j] == debugBlockArraySave[i]) {
                found = debugBlockSizeArray[j] == debugBlockSizeArraySave[i];
                break;
            }
        }
    }
}
#endif

static U32 iRenderWareInit1()
{
    RwMemoryFunctions* pMemFuncs = NULL;
    RwMemoryFunctions newMemFuncs = { iMemMgrMalloc, iMemMgrFree, iMemMgrRealloc, iMemMgrCalloc };

    pMemFuncs = &newMemFuncs;
    if (!RwEngineInit(pMemFuncs, rwENGINEINITNOFREELISTS, 0x100000)) {
        return 1;
    }
    RwResourcesSetArenaSize(0x100000);
    RwDebugSetHandler(psDebugMessageHandler);
    RwDebugSendMessage_L(rwDEBUGMESSAGE, "The Incredibles: Rise of the Underminer", "Debugging Initialized", 1185);
#ifdef RWDEBUG
    oldDebugHandler = RWSRCGLOBAL(debugFunction);
    RWSRCGLOBAL(debugFunction) = NewRwDebugHandler;
#endif
    if (RWAttachPlugins()) {
        return 1;
    }
    return 0;
}

RwGameCubeDeviceConfig deviceConfig;
GXRenderModeObj renderObject;

GXRenderModeObj myGXNtsc480IntDf = {
    VI_TVMODE_NTSC_INT,
    640,
    480,
    480,
    40,
    0,
    640,
    480,
    VI_XFBMODE_DF,
    GX_FALSE,
    GX_FALSE,
    { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 },
    { 8, 8, 10, 12, 10, 8, 8 },
};

GXRenderModeObj myGXPal528IntDf = {
    VI_TVMODE_PAL_INT,
    640,
    528,
    528,
    40,
    23,
    640,
    528,
    VI_XFBMODE_DF,
    GX_FALSE,
    GX_FALSE,
    { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 },
    { 8, 8, 10, 12, 10, 8, 8 },
};

static U32 iRenderWareInit2()
{
    VISetBlack(TRUE);
    VIFlush();
    VIWaitForRetrace();
    renderObject = myGXNtsc480IntDf;
    if (zMainIsNTSC())
        renderObject = myGXNtsc480IntDf;
    else if (zMainIsPAL())
        renderObject = myGXPal528IntDf;
    renderObject.fbWidth = FB_XRES;
    renderObject.efbHeight = FB_YRES;
    renderObject.xfbHeight = FB_YRES;
    deviceConfig.renderMode = &renderObject;
    deviceConfig.pixFmt = 1;
    deviceConfig.fifoSize = 0x20000;
    RwEngineOpenParams openParams;
    openParams.displayID = &deviceConfig;
    if (!RwEngineOpen(&openParams)) {
        VISetBlack(FALSE);
        VIFlush();
        VIWaitForRetrace();
        RwEngineTerm();
        return 1;
    }
    RwInt32 idx = RwEngineGetCurrentVideoMode();
    RwEngineGetVideoModeInfo(&sVideoMode, idx);
#if DEBUG
    debugMemoryDoAsserts = 0;
#endif
    if (!RwEngineStart()) {
        RwEngineClose();
        RwEngineTerm();
        VISetBlack(FALSE);
        VIFlush();
        return 1;
    }
#if DEBUG
    debugMemoryDoAsserts = 1;
#endif
    RwGameCubeGetXFBs(&xfb1, &xfb2);
    RwTextureSetReadCallBack(TextureRead);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
    xShadowInit();
    xFXInit();
#if DEBUG
    GXSetVerifyLevel(GX_WARN_NONE);
#endif
    RwTextureSetMipmapping(TRUE);
    RwTextureSetAutoMipmapping(TRUE);
    RwCamera* cam = iCameraCreate(FB_XRES, FB_YRES, 0);
    RwRGBA bg = { 0, 0, 0, 0 };
    RwCameraClear(cam, &bg, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);
    RwCameraShowRaster(cam, NULL, 1);
    RwCameraShowRaster(cam, NULL, 1);
    iCameraDestroy(cam);
    VIWaitForRetrace();
    VISetBlack(FALSE);
    VIFlush();
    return 0;
}

void iSystemRWExit()
{
    RwBool ret;

    ret = RwEngineStop();
    ret = RwEngineClose();
    ret = RwEngineTerm();
}

void iSystemExitRWFailSafe()
{
    RwBool ret;

    iprintf("========= Shutting down Fail Safe RenderWare =========\n");
    ret = RwEngineStop();
    ret = RwEngineClose();
    _RwDlRenderMode = NULL;
    iprintf("========= Done.\n =========");
}

static RwTexture* TextureRead(const RwChar* name, const RwChar* maskName)
{
    char tmpname[256];
    S32 npWidth = 0, npHeight = 0, npDepth = 0, npFormat = 0;
    RwImage* img = NULL;
    RwRaster* rast = NULL;
    RwTexture* result;
    RwGameCubeRasterExtension* ext = NULL;
    U32 assetid;
    U32 tmpsize;

    sprintf(tmpname, "%s.rw3", name);
    assetid = xStrHash(tmpname);
    result = (RwTexture*)xSTFindAsset(assetid, &tmpsize);
    if (result && result->raster && result->raster->depth < 8) {
        ext = RwGameCubeRasterGetExtension(result->raster);
        if (!ext || ext->format != GX_TF_CMPR) {
            result = NULL;
        }
    }
    if (result) {
        strcpy(result->name, name);
        strcpy(result->mask, maskName);
    }
    return result;
}

static S32 _rwDolphinHeapInit(); // UNUSED

void null_func()
{
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("0", "%s")
#endif

extern "C" {

static void mem_null(unsigned long a, unsigned long s); // UNUSED
static void* alloc_high(U32 memsize, U32 memalign); // UNUSED
static void free_high(void* ptr); // UNUSED

char* malloc(s32 msize)
{
    extern S32 the_heap;
    void* rt;

    if (msize <= 0) {
        return NULL;
    }
    rt = OSAllocFromHeap(the_heap, msize);
    if (rt == NULL) {
        null_func();
    }
    return (char*)rt;
}

void free(char* p)
{
    extern S32 the_heap;

    if (p != NULL) {
        OSFreeToHeap(the_heap, p);
    }
}

}

static void _rwDolphinHeapFree(void* memory); // UNUSED
static void* _rwDolphinHeapAlloc(unsigned long size, U32); // UNUSED
static void* _rwDolphinHeapCalloc(unsigned long numObj, unsigned long, U32); // UNUSED
static void* _rwDolphinHeapRealloc(void* memory, unsigned long size, U32); // UNUSED

s32 iGetMinute()
{
    OSCalendarTime td;

    OSTicksToCalendarTime(OSGetTime(), &td);
    return td.min;
}

s32 iGetHour()
{
    OSCalendarTime td;

    OSTicksToCalendarTime(OSGetTime(), &td);
    return td.hour;
}

s32 iGetDay()
{
    OSCalendarTime td;

    OSTicksToCalendarTime(OSGetTime(), &td);
    return td.mday;
}

s32 iGetMonth()
{
    OSCalendarTime td;

    OSTicksToCalendarTime(OSGetTime(), &td);
    return td.mon + 1;
}

char* months[12] = { "January ", "February ", "March ",     "April ",   "May ",      "June ",
                     "July ",    "August ",   "September ", "October ", "November ", "December " };
char* dotw[7] = { "Sunday ", "Monday ", "Tuesday ", "Wednesday ", "Thursday ", "Friday ", "Saturday " };

u32 iGetCurrFormattedDate(char* p)
{
    OSCalendarTime td;
    char* tp = p;
    OSTicksToCalendarTime(OSGetTime(), &td);
    strcpy(p, dotw[td.wday]);
    strcat(p, months[td.mon]);
    p += strlen(p);
    if (td.mday >= 10) {
        *p++ = td.mday / 10 + '0';
    }
    *p++ = td.mday % 10 + '0';
    *p++ = ',';
    *p++ = ' ';
    *p++ = td.year / 1000 + '0';
    *p++ = td.year / 100 % 10 + '0';
    *p++ = td.year / 10 % 100 + '0';
    *p++ = td.year % 10 + '0';
    *p++ = '\0';
    return p - tp;
}

u32 iGetCurrFormattedTime(char* p)
{
    OSCalendarTime td;
    char* tp = p;
    S32 am = FALSE;

    OSTicksToCalendarTime(OSGetTime(), &td);
    if (td.hour < 12) {
        am = TRUE;
    } else {
        td.hour -= 12;
    }
    if (td.hour == 0) {
        td.hour = 12;
    }
    if (td.hour >= 10) {
        *p++ = td.hour / 10 + '0';
    }
    *p++ = td.hour % 10 + '0';
    *p++ = ':';
    *p++ = td.min / 10 + '0';
    *p++ = td.min % 10 + '0';
    *p++ = ':';
    *p++ = td.sec / 10 + '0';
    *p++ = td.sec % 10 + '0';
    *p++ = ' ';
    if (am) {
        *p++ = 'A';
        *p++ = '.';
        *p++ = 'M';
        *p++ = '.';
    } else {
        *p++ = 'P';
        *p++ = '.';
        *p++ = 'M';
        *p++ = '.';
    }
    *p++ = '\0';
    return p - tp;
}

static AramAllocNode AramMgr[MAX_ARAM_BLOCKS];

void* iSystemARAMAlloc(U32 numbytes_to_use)
{
    U32 uNumBytes = (numbytes_to_use + 31) & ~31;
    for (U32 blockIndex = 0; blockIndex < MAX_ARAM_BLOCKS - 1; blockIndex++) {
        if (!(AramMgr[blockIndex].uFlags & 1)) {
            U32 segIndex = blockIndex + 1;
            while (uNumBytes > AramMgr[blockIndex].uAllocByteCount) {
                if (AramMgr[segIndex].uFlags & 1) {
                    break;
                }
                AramMgr[blockIndex].uAllocByteCount += AramMgr[segIndex].uAllocByteCount;
                AramMgr[segIndex].uAllocByteCount = 0;
                AramMgr[segIndex].pAramAddress =
                    AramMgr[blockIndex].pAramAddress + AramMgr[blockIndex].uAllocByteCount;
                segIndex++;
            }
            if (uNumBytes <= AramMgr[blockIndex].uAllocByteCount) {
                AramMgr[blockIndex].uFlags |= 1;
                AramMgr[blockIndex].uAllocByteCount = uNumBytes;
                if (!(AramMgr[blockIndex + 1].uFlags & 1)) {
                    U32 pOldEndAddress =
                        AramMgr[blockIndex + 1].pAramAddress + AramMgr[blockIndex + 1].uAllocByteCount;
                    AramMgr[blockIndex + 1].pAramAddress = AramMgr[blockIndex].pAramAddress + uNumBytes;
                    AramMgr[blockIndex + 1].uAllocByteCount = pOldEndAddress - AramMgr[blockIndex + 1].pAramAddress;
                }
                return (void*)AramMgr[blockIndex].pAramAddress;
            }
        }
    }
    iprintf("arammalloc failed malloc!!!\n");
    return NULL;
}

void iSystemARAMFree(void* ptr)
{
    for (U32 blockIndex = 0; blockIndex < MAX_ARAM_BLOCKS - 1; blockIndex++) {
        if ((U32)ptr == AramMgr[blockIndex].pAramAddress) {
            AramMgr[blockIndex].uFlags &= ~1;
            return;
        }
    }
    iprintf("Bad Ptr to aramfree\n");
}

static U32 pARAMBaseAddressBink;
static U32 ARAMBytesForBink;
static U32 pARAMBaseAddressFMOD;
static U32 ARAMBytesForFMOD;

void iSystemARAMInit()
{
    ARInit(NULL, 0);
    pARAMBaseAddressFMOD = ARGetBaseAddress();
    pARAMBaseAddressFMOD = (pARAMBaseAddressFMOD + 255) & ~255;
    ARAMBytesForFMOD = 0xD00000;
    pARAMBaseAddressBink = pARAMBaseAddressFMOD + ARAMBytesForFMOD;
    ARAMBytesForBink = 0x1000000 - pARAMBaseAddressBink;
    for (S32 i = 0; i < MAX_ARAM_BLOCKS; i++) {
        U32 blocksize = ARAMBytesForBink / MAX_ARAM_BLOCKS;
        blocksize &= ~0xE0;
        AramMgr[i].pAramAddress = pARAMBaseAddressBink + i * blocksize;
        AramMgr[i].uAllocByteCount = blocksize;
        AramMgr[i].uFlags = 0;
    }
}

U32 iSystemARAMBasePtrForFMOD()
{
    return pARAMBaseAddressFMOD;
}

U32 iSystemARAMSizeForFMOD()
{
    return ARAMBytesForFMOD;
}

static void NewRwDebugHandler(RwDebugType type, const RwChar* string)
{
    if (strstr(string, "bacamera.c")) {
        return;
    }
    if (strstr(string, "babinary.c")) {
        return;
    }
    if (strstr(string, "baresour.c"))
        return;
    if (strstr(string, "RwMatrixInvert: rwMatrixIsIdentity"))
        return;
    oldDebugHandler(type, string);
}

void iSystemInitRWFailSafe()
{
    iprintf("========= Starting up Fail Safe RenderWare =========\n");
    iRenderWareInit1();
    RwEngineOpenParams openParams;
    u32 videoMode = VIGetTvFormat();
    switch (videoMode) {
    case VI_NTSC:
        iprintf("Found NTSC Video Mode!\n");
        renderObject = myGXNtsc480IntDf;
        iSystemGetWidthHeight(FB_XRES, FB_YRES, false);
        break;
    case VI_PAL:
        iprintf("Found PAL Video Mode!\n");
        renderObject = myGXPal528IntDf;
        iSystemGetWidthHeight(FB_XRES, FB_YRES, true);
        break;
    case VI_EURGB60:
        iprintf("Found EURGB60 Video Mode! Treating as PAL\n");
        renderObject = myGXPal528IntDf;
        iSystemGetWidthHeight(FB_XRES, FB_YRES, true);
        break;
    case VI_MPAL:
        iprintf("Found M-PAL Video Mode! Treating as NTSC\n");
        renderObject = myGXNtsc480IntDf;
        iSystemGetWidthHeight(FB_XRES, FB_YRES, false);
        break;
    default:
        iprintf("No Valid Video Mode! This should never happen! Defaulting to NTSC!");
        renderObject = myGXNtsc480IntDf;
        iSystemGetWidthHeight(FB_XRES, FB_YRES, false);
        break;
    }
    deviceConfig.renderMode = &renderObject;
    deviceConfig.pixFmt = 1;
    deviceConfig.fifoSize = 0x20000;
    openParams.displayID = &deviceConfig;
    if (!RwEngineOpen(&openParams)) {
        RwEngineTerm();
        iprintf("RwEngineOpen Failed!!\n");
    }
    RwInt32 idx = RwEngineGetCurrentVideoMode();
    RwEngineGetVideoModeInfo(&sVideoMode, idx);
    if (!RwEngineStart()) {
        RwEngineClose();
        RwEngineTerm();
        iprintf("RwEngineStart Failed!!\n");
    }
    iprintf("========= Done. =========\n");
}

void iSystemAlarmCallBack(OSAlarm* alarm, OSContext*)
{
    GCAlarm* gcAlarm = (GCAlarm*)alarm;
    OSResumeThread(gcAlarm->thread);
}

void iSystemSleep(S32 milliSecondsToSleep, U32 cancelAlarmsTag)
{
    GCAlarm gcAlarm;

    gcAlarm.thread = OSGetCurrentThread();
    OSCreateAlarm(&gcAlarm.alarm);
    OSSetAlarm(&gcAlarm.alarm, OSMillisecondsToTicks(milliSecondsToSleep), iSystemAlarmCallBack);
    OSSetAlarmTag(&gcAlarm.alarm, cancelAlarmsTag);
    OSSuspendThread(OSGetCurrentThread());
}

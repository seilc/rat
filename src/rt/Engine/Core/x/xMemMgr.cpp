#include "xMemMgr.h"

#include "iSystem.h"
#include "xFile.h"
#include "xGlobals.h"
#include "xMath.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zGame.h"
#include "zPad.h"
#if RELEASE
#include "zConfiguration.h"
#include "zGlobals.h"
#include "zScene.h"
#endif

#include "decomp.h"

#include <rwcore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XMEM_GUARD_BREAK_ADDR 0x234D060
#define MAX_ALLOC_IDENTIFIERS 4
#define MAX_GENERIC_MALLOC_TAGS 8

enum
{
    eMemActive = 0x2,
    eMemHeapGuarded = 0x20,
    eMemHeapGrowsDown = 0x100,
    eMemHeapHeaderInBlock = 0x8000,
    eMemHeapBlockList = 0x10000,
};

struct xMemBlkInfo
{
    xMemBlock* header;
    U32 pre;
    U32 block;
    U32 post;
    U32 curr;
    U32 waste;
    U32 total;
};

struct allocIdentifierData
{
    S32 bytesAllocated;
    U32 assetID;
};

xMemInfo gMemInfo;
U32 gActiveHeap;
xMemHeap gxHeap[XMEM_MAX_HEAPS];
xMemBaseNotifyFunc sMemBaseNotifyFunc;
S32 gMemReporting;
#if !MASTER
U32 gMemReportingThreshold = 0x4000;
#endif

#if DEBUG
static allocIdentifierData allocList[MAX_ALLOC_IDENTIFIERS];
#endif

static U32 xMemCheckHeap(xMemHeap* heap, U32 dump);

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("xMemMgr.cpp", "heap != 0L", "%s", "heap != NULL")

static void xMemDebugModeCB()
{
    iMemMgrXPrintDebugInfo();

    if (zPad::GetDebugPad()->Pressed(0x10000)) {
        xMemHeap* heap = gxHeap;
        for (S32 i = 0; i < XMEM_MAX_HEAPS; i++, heap++) {
        }
        DBprintf(DBML_VDBG, "xMemCheck() complete.\n");
    }

#if DEBUG
    if (zPad::GetDebugPad()->Pressed(0x20000)) {
        iMemMgrDumpActiveList();
    }
#endif
}
#endif

#if !MASTER
static xFile DBSSoakLogFile;
static bool DBSSoakLogOpen;
static S32 DBSSceneMemFree = 0x7FFFFFFF;
static U32 DBSLastSceneId;
#endif

void xMemDebug_SoakLog(U32 SceneId)
{
    static S32 soakcnt = 0;
    static S32 soakidx = 0;

    if (soakcnt <= 0) {
        while (soaklevels[soakcnt] != NULL)
            soakcnt++;
    }

#if !MASTER
    printf("MEM FREE = %d K\n", iMemMgrGetMinFreeSpace() / 1024 - 24 * 1024);

#if DEBUG
    const char* DBSFileName = "DebugSoakLog.txt";
#elif RELEASE
    const char* DBSFileName = "GCReleaseSoakLog.txt";
#else
    const char* DBSFileName = "GCMasterSoakLog.txt";
#endif
    char soakbuf[128];

    if (!DBSSoakLogOpen) {
        DBSSoakLogOpen = true;
        iprintf(DBSFileName);
        iFileOpen(DBSFileName, 0x8E, &DBSSoakLogFile);
        sprintf(soakbuf, "GC\r\n");
        iFileWrite(&DBSSoakLogFile, soakbuf, strlen(soakbuf));
        DBSLastSceneId = SceneId;
    }
    else if (SceneId != DBSLastSceneId) {
        sprintf(soakbuf, "Scene = %s Memfree = %d\r\n", xUtil_idtag2string(DBSLastSceneId, 0), DBSSceneMemFree);
        if (soakidx < soakcnt) {
#if DEBUG || RELEASE
            char dfname[128];
            strcpy(dfname, xUtil_idtag2string(DBSLastSceneId, 0));
            strcpy(dfname + 4, "_Logfile.txt");
            xMemMgrUsageTrackerSetDumpFile(dfname);
            xMemMgrUsageTrackerDumpLog("Scene = %s Memfree = %d\n", xUtil_idtag2string(DBSLastSceneId, 0), DBSSceneMemFree);
            xMemMgrUsageTrackerCloseLog();
#endif
            soakidx++;
        }
        iFileWrite(&DBSSoakLogFile, soakbuf, strlen(soakbuf));
        DBSSceneMemFree = 0x7FFFFFFF;
        DBSLastSceneId = SceneId;
    }

    S32 tmemfree = iMemMgrGetMinFreeSpace() / 1024 - 24 * 1024;
    DBSSceneMemFree = MIN(DBSSceneMemFree, tmemfree);
#endif
}

#if DEBUG
void xMem_DB_DispStatSum()
{
    xHeapState* sp = NULL;
    xMemHeap* heap = NULL;

    heap = gxHeap;
    sp = &heap->state[heap->state_idx];

    DBprintf(DBML_DISP, "\n\n\n\n");
    DBprintf(DBML_DISP, "Summary:   %9d / %9d [%5.2f %%]\n", sp->used, heap->size,
             100.0f * sp->used / heap->size);
    DBprintf(DBML_DISP, "Overhead:  %9d / %9d [%5.2f %%]\n", sp->wasted, sp->used,
             100.0f * sp->wasted / sp->used);
}
#endif

void xMemInit()
{
    iMemInit();

    xMemInitHeap(&gxHeap[0], gMemInfo.DRAM.addr, gMemInfo.DRAM.size, 0x8826);
    xMemInitHeap(&gxHeap[1], gMemInfo.DRAM.addr + gMemInfo.DRAM.size, gMemInfo.DRAM.size, 0x892A);

    gxHeap[0].opp_heap[0] = 1;
    gxHeap[0].opp_heap[1] = -1;
    gxHeap[1].opp_heap[0] = 0;
    gxHeap[1].opp_heap[1] = -1;

    gActiveHeap = 0;

#if DEBUG || RELEASE
    xDebugModeRegisterCallback(0xC, xMemDebugModeCB);
    xMemMgrUsageTrackerInit();
#endif

#if DEBUG
    for (S32 i = 0; i < ARRAY_LENGTH(allocList); i++) {
        allocList[i].bytesAllocated = -1;
    }
    allocList[0].assetID = 'XMDL';
    allocList[0].bytesAllocated = 0;
#endif
}

void xMemExit()
{
#if DEBUG || RELEASE
    xMemMgrUsageTrackerShutdown();
#endif
    iMemExit();
}

#if DEBUG || RELEASE
static S32 sMemMgrTrckLogDisplayDumpedDelay;
static S32 sMemMgrUsageTrackerTopLine;
static xFile sMemMgrTrckLog;
static bool sMemMgrTrckLogIsOpen;
static U32 sMemMgrGenericMallocTag[MAX_GENERIC_MALLOC_TAGS];
static U32 sMemMgrUsageTrackerPackerMallocMem[eMemMgrTag_NumTags] = { 0 };
static U32 sMemMgrUsageTrackerPackerApproxRWMallocMem[eMemMgrTag_NumTags] = { 0 };
static U32 sMemMgrUsageTrackerGenericMallocMem[eMemMgrTag_NumTags] = { 0 };

enum eMemMgrDeptTag
{
    eMemMgrDeptTag_Anim,
    eMemMgrDeptTag_Art,
    eMemMgrDeptTag_Design,
    eMemMgrDeptTag_Prog,
    eMemMgrDeptTag_NumDepts
};

struct xMemMgrUsageTrackerLookup
{
    eMemMgrTag tagID;
    eMemMgrDeptTag deptID;
    char* tagNameString;
};

struct xMemMgrDeptLookup
{
    eMemMgrDeptTag deptID;
    char* deptNameString;
    U32 maxMem;
    U32 throwAssert;
};

static xMemMgrUsageTrackerLookup xMemMgrUsageTrackerLookupTable[eMemMgrTag_NumTags] = {
    { eMemMgrTag_2D, eMemMgrDeptTag_Prog, "2D" },
    { eMemMgrTag_Anim, eMemMgrDeptTag_Anim, "Animation" },
    { eMemMgrTag_Audio, eMemMgrDeptTag_Prog, "Audio" },
    { eMemMgrTag_Bomb, eMemMgrDeptTag_Design, "Small Throwable Bombs" },
    { eMemMgrTag_Camera, eMemMgrDeptTag_Design, "Camera" },
    { eMemMgrTag_Cinematic, eMemMgrDeptTag_Anim, "Cinematic" },
    { eMemMgrTag_Components, eMemMgrDeptTag_Prog, "Components System" },
    { eMemMgrTag_Destructible, eMemMgrDeptTag_Design, "Destructible Objects" },
    { eMemMgrTag_Debug, eMemMgrDeptTag_Prog, "Debug" },
    { eMemMgrTag_Decal, eMemMgrDeptTag_Prog, "Decal (Internal)" },
    { eMemMgrTag_Duplicator, eMemMgrDeptTag_Design, "Enemy Duplicator" },
    { eMemMgrTag_Event, eMemMgrDeptTag_Design, "Progress Script" },
    { eMemMgrTag_FMV, eMemMgrDeptTag_Prog, "FMV" },
    { eMemMgrTag_FXActionLine, eMemMgrDeptTag_Prog, "Swift Motion Lines" },
    { eMemMgrTag_FXRibbon, eMemMgrDeptTag_Prog, "FX Ribbon" },
    { eMemMgrTag_Enemy, eMemMgrDeptTag_Design, "Enemy" },
    { eMemMgrTag_EnergyStream, eMemMgrDeptTag_Prog, "Energy Stream" },
    { eMemMgrTag_EntityMisc, eMemMgrDeptTag_Prog, "Managed Entity Buffer" },
    { eMemMgrTag_EnvSpatial, eMemMgrDeptTag_Art, "Spatial Environment" },
    { eMemMgrTag_EnvAbstract, eMemMgrDeptTag_Design, "Abstract Environment" },
    { eMemMgrTag_Explosion, eMemMgrDeptTag_Prog, "Explosion" },
    { eMemMgrTag_Flame, eMemMgrDeptTag_Prog, "Flame" },
    { eMemMgrTag_FlameThrower, eMemMgrDeptTag_Prog, "Flamethrower" },
    { eMemMgrTag_ForceFieldEffect, eMemMgrDeptTag_Prog, "Force Field" },
    { eMemMgrTag_FrozenObject, eMemMgrDeptTag_Prog, "Frozen Objects" },
    { eMemMgrTag_Grass, eMemMgrDeptTag_Design, "Grass" },
    { eMemMgrTag_HierarchyBound, eMemMgrDeptTag_Prog, "Hierarchy Bounding Sphere" },
    { eMemMgrTag_HUD, eMemMgrDeptTag_Prog, "HUD" },
    { eMemMgrTag_IncrediSlam, eMemMgrDeptTag_Prog, "IncrediSlam" },
    { eMemMgrTag_LaserBeam, eMemMgrDeptTag_Design, "Laserbeam" },
    { eMemMgrTag_LaserBolt, eMemMgrDeptTag_Design, "Laserbolt" },
    { eMemMgrTag_Light, eMemMgrDeptTag_Prog, "Light" },
    { eMemMgrTag_Lightning, eMemMgrDeptTag_Prog, "Lightning" },
    { eMemMgrTag_Lightweight, eMemMgrDeptTag_Prog, "Lightweight System" },
    { eMemMgrTag_Message, eMemMgrDeptTag_Prog, "Dispatcher" },
    { eMemMgrTag_Model, eMemMgrDeptTag_Art, "Model" },
    { eMemMgrTag_MotionBlur, eMemMgrDeptTag_Prog, "Motion Blur" },
    { eMemMgrTag_Movable, eMemMgrDeptTag_Design, "Movable Geometry" },
    { eMemMgrTag_MovePoint, eMemMgrDeptTag_Design, "Move Point" },
    { eMemMgrTag_NavigationMesh, eMemMgrDeptTag_Art, "Navigation Mesh" },
    { eMemMgrTag_NPCFX, eMemMgrDeptTag_Anim, "NPC FX" },
    { eMemMgrTag_NPCWalls, eMemMgrDeptTag_Design, "NPC Walls" },
    { eMemMgrTag_NPCGate, eMemMgrDeptTag_Design, "NPC Gate" },
    { eMemMgrTag_OneLiner, eMemMgrDeptTag_Prog, "Oneliner" },
    { eMemMgrTag_OrdArrayMgr, eMemMgrDeptTag_Prog, "Ord Array Manager" },
    { eMemMgrTag_Packer, eMemMgrDeptTag_Prog, "Packer Other" },
    { eMemMgrTag_PackerAssetPool, eMemMgrDeptTag_Prog, "Packer Asset Pool" },
    { eMemMgrTag_PackerDefault, eMemMgrDeptTag_Design, "Packer Default (Widgets)" },
    { eMemMgrTag_PackerTOC, eMemMgrDeptTag_Prog, "Packer TOC" },
    { eMemMgrTag_PackerUnknown, eMemMgrDeptTag_Prog, "Packer Unknown" },
    { eMemMgrTag_Particle, eMemMgrDeptTag_Prog, "Particle" },
    { eMemMgrTag_Physics, eMemMgrDeptTag_Prog, "Physics" },
    { eMemMgrTag_Player, eMemMgrDeptTag_Prog, "Player" },
    { eMemMgrTag_Projectile, eMemMgrDeptTag_Design, "Projectile" },
    { eMemMgrTag_RenderBuffer, eMemMgrDeptTag_Prog, "Render Buffer" },
    { eMemMgrTag_Rubble, eMemMgrDeptTag_Design, "Omnidroid Rubble" },
    { eMemMgrTag_Scene, eMemMgrDeptTag_Prog, "Scene" },
    { eMemMgrTag_ScreenFX, eMemMgrDeptTag_Prog, "Screen FX" },
    { eMemMgrTag_Serializer, eMemMgrDeptTag_Prog, "Serializer" },
    { eMemMgrTag_Shadow, eMemMgrDeptTag_Prog, "Shadow" },
    { eMemMgrTag_Shower, eMemMgrDeptTag_Prog, "Shower" },
    { eMemMgrTag_SmashRing, eMemMgrDeptTag_Prog, "Smash Ring" },
    { eMemMgrTag_SoundRAMAsset, eMemMgrDeptTag_Prog, "SoundRAM Asset" },
    { eMemMgrTag_Splash, eMemMgrDeptTag_Prog, "Water Splash" },
    { eMemMgrTag_Spline, eMemMgrDeptTag_Design, "Spline" },
    { eMemMgrTag_SpotLight, eMemMgrDeptTag_Design, "Spotlight" },
    { eMemMgrTag_Subtitle, eMemMgrDeptTag_Prog, "Subtitle" },
    { eMemMgrTag_Texture, eMemMgrDeptTag_Art, "Texture" },
    { eMemMgrTag_TextureStream, eMemMgrDeptTag_Prog, "Texture Stream" },
    { eMemMgrTag_Throwable, eMemMgrDeptTag_Design, "Throwable Object" },
    { eMemMgrTag_ThrowableSystem, eMemMgrDeptTag_Prog, "Throwable Object System" },
    { eMemMgrTag_Trigger, eMemMgrDeptTag_Design, "Trigger" },
    { eMemMgrTag_UberLaser, eMemMgrDeptTag_Design, "Uber-laser" },
    { eMemMgrTag_UI, eMemMgrDeptTag_Prog, "UI" },
    { eMemMgrTag_UpdateCullMgr, eMemMgrDeptTag_Prog, "Update Cull Manager" },
    { eMemMgrTag_Im3DHelper, eMemMgrDeptTag_Prog, "Im3DHelper" },
    { eMemMgrTag_ReactiveAnim, eMemMgrDeptTag_Anim, "Reactive Animation" },
    { eMemMgrTag_RenderWare, eMemMgrDeptTag_Prog, "RenderWare" },
    { eMemMgrTag_Shrapnel, eMemMgrDeptTag_Design, "Shrapnel" },
    { eMemMgrTag_VideoRAMAsset, eMemMgrDeptTag_Prog, "VideoRAM Asset" },
    { eMemMgrTag_Water, eMemMgrDeptTag_Prog, "Water" },
    { eMemMgrTag_WaterHose, eMemMgrDeptTag_Prog, "Waterhose" },
    { eMemMgrTag_AnimCustom, eMemMgrDeptTag_Anim, "Custom Animations" },
    { eMemMgrTag_SwitchLever, eMemMgrDeptTag_Design, "Switch Lever" },
    { eMemMgrTag_PlayerFX, eMemMgrDeptTag_Prog, "PlayerFX" },
    { eMemMgrTag_RenderWareMisc, eMemMgrDeptTag_Prog, "RenderWare Misc" },
    { eMemMgrTag_DecalRWMalloc, eMemMgrDeptTag_Prog, "Decal Renderware Malloc" },
    { eMemMgrTag_Heists, eMemMgrDeptTag_Design, "Heists" },
    { eMemMgrTag_MiniGame, eMemMgrDeptTag_Prog, "Mini Games" },
    { eMemMgrTag_CookingMinigame, eMemMgrDeptTag_Design, "Cooking Minigames" },
};

xMemMgrDeptLookup xMemMgrDeptLookupTable[eMemMgrDeptTag_NumDepts] = {
    { eMemMgrDeptTag_Anim, "Animation Dept", 0, TRUE },
    { eMemMgrDeptTag_Art, "Art Dept", 0, TRUE },
    { eMemMgrDeptTag_Design, "Design Dept", 0, TRUE },
    { eMemMgrDeptTag_Prog, "Programming Dept", 0, TRUE },
};

static void xMemMgrUsageTrackerGetDeptTotals(U32* totArray);
static void xMemMgrUsageTrackerPrintTotals();
static void xMemMgrUsageTrackerGetInput();

void xMemMgrSetDeptMaxMem(U32 deptID, U32 deptMaxMem)
{
    U32 heapID = 0;
    xASSERT_L(765, heapID >= 0 && heapID <= XMEM_MAX_HEAPS);
    xMemHeap* heap = &gxHeap[heapID];
    xHeapState* sp = &heap->state[heap->state_idx];

    for (U32 i = 0; i < eMemMgrDeptTag_NumDepts; i++) {
        if (deptID == xMemMgrDeptLookupTable[i].deptID) {
            xMemMgrDeptLookupTable[i].maxMem = deptMaxMem;
            sp->memoryUsed.TrackAllocation(0, deptID);
            break;
        }
    }
}

void xMemMgrUsageTrackerInit()
{
    for (S32 i = 0; i < eMemMgrTag_NumTags; i++) {
        while (i != xMemMgrUsageTrackerLookupTable[i].tagID) {
            xMemMgrUsageTrackerLookup tagswapstorage;
            tagswapstorage = xMemMgrUsageTrackerLookupTable[xMemMgrUsageTrackerLookupTable[i].tagID];
            xMemMgrUsageTrackerLookupTable[xMemMgrUsageTrackerLookupTable[i].tagID] = xMemMgrUsageTrackerLookupTable[i];
            xMemMgrUsageTrackerLookupTable[i] = tagswapstorage;
        }
    }

    for (S32 j = 0; j < eMemMgrDeptTag_NumDepts; j++) {
        while (j != xMemMgrUsageTrackerLookupTable[j].tagID) {
            xMemMgrDeptLookup deptswapstorage;
            deptswapstorage = xMemMgrDeptLookupTable[xMemMgrDeptLookupTable[j].deptID];
            xMemMgrDeptLookupTable[xMemMgrDeptLookupTable[j].deptID] = xMemMgrDeptLookupTable[j];
            xMemMgrDeptLookupTable[j] = deptswapstorage;
        }
    }

    for (S32 k = 0; k < MAX_GENERIC_MALLOC_TAGS; k++) {
        sMemMgrGenericMallocTag[k] = eMemMgrTag_NumTags;
    }

    strcpy(xglobals->MemTrackLogFileName, "memtracklog.txt");
    xDebugModeRegisterCallback(0xD, xMemMgrUsageTrackerCB);
}

void xMemMgrUsageTrackerShutdown()
{
    if (sMemMgrTrckLogIsOpen) {
        iFileClose(&sMemMgrTrckLog);
    }
}

void xMemMgrUsageTrackerCB()
{
    xMemMgrUsageTrackerPrintTotals();
    xMemMgrUsageTrackerGetInput();
}

static void xMemMgrUsageTrackerGetDeptTotals(U32* totArray)
{
    memset(totArray, 0, eMemMgrDeptTag_NumDepts * sizeof(U32));
    for (U32 i = 0; i < eMemMgrTag_NumTags; i++) {
        totArray[xMemMgrUsageTrackerLookupTable[i].deptID] += xMemMgrGetTotalMemUsageByTag(xMemMgrUsageTrackerLookupTable[i].tagID);
    }
}

static void xMemMgrUsageTrackerPrintTotals()
{
    S32 curline = -sMemMgrUsageTrackerTopLine;

    xprintf("*MEMORY USAGE*\n");
    xprintf("*USE DPAD UP AND DOWN TO SCROLL*\n");
    if (sMemMgrTrckLogDisplayDumpedDelay == 0) {
        xprintf("*PRESS A (OR EQUIV) TO DUMP TO LOG*\n\n");
    } else {
        xprintf("*DUMPED TO LOG*\n\n");
        sMemMgrTrckLogDisplayDumpedDelay--;
    }

    U32 deptTotals[eMemMgrDeptTag_NumDepts];
    xMemMgrUsageTrackerGetDeptTotals(deptTotals);

    for (S32 deptit = 0; deptit < eMemMgrDeptTag_NumDepts; deptit++) {
        xprintf("*** %-24s %-5d K ***\n", xMemMgrDeptLookupTable[deptit].deptNameString, deptTotals[xMemMgrDeptLookupTable[deptit].deptID] / 1024);
    }

    xprintf("\n");

    for (S32 j = 0; j < eMemMgrDeptTag_NumDepts && curline < sMemMgrUsageTrackerTopLine + 11; j++) {
        for (S8 k = 0; k < 3; k++) {
            if (k == 1 && curline > -1) {
                xprintf("*** %-24s %-5d K ***\n", xMemMgrDeptLookupTable[j].deptNameString, deptTotals[xMemMgrDeptLookupTable[j].deptID] / 1024);
            }
            else if (curline > -1) {
                xprintf("\n");
            }
            curline++;
        }

        for (S32 i = 0; i < eMemMgrTag_NumTags && curline < sMemMgrUsageTrackerTopLine + 11; i++) {
            if (xMemMgrUsageTrackerLookupTable[i].deptID == xMemMgrDeptLookupTable[j].deptID) {
                if (curline > -1) {
                    xprintf("%-28s %-5d K\n", xMemMgrUsageTrackerLookupTable[i].tagNameString, xMemMgrGetTotalMemUsageByTag(xMemMgrUsageTrackerLookupTable[i].tagID) / 1024);
                }
                curline++;
            }
        }
    }
}

static void xMemMgrUsageTrackerGetInput()
{
    if (zPad::GetDebugPad()->On(0x10) && sMemMgrUsageTrackerTopLine > 0) {
        sMemMgrUsageTrackerTopLine--;
    }
    else if (zPad::GetDebugPad()->On(0x40) && sMemMgrUsageTrackerTopLine < 91) {
        sMemMgrUsageTrackerTopLine++;
    }
    else if (zPad::GetDebugPad()->Pressed(0x10000)) {
        xMemMgrUsageTrackerDumpLog("Dumped by request at tweak menu\n");
        sMemMgrTrckLogDisplayDumpedDelay = 100;
    }
}

void xMemMgrUsageTrackerSetDumpFile(char* filename)
{
    strcpy(xglobals->MemTrackLogFileName, filename);
}

void xMemMgrUsageTrackerDumpLog(char* fmt, ...)
{
    if (!sMemMgrTrckLogIsOpen) {
        iFileOpen(xglobals->MemTrackLogFileName, 0x18E, &sMemMgrTrckLog);
        sMemMgrTrckLogIsOpen = true;
    }
    char logfilebuf[2048];
    va_list marker;

    va_start(marker, fmt);
    vsprintf(logfilebuf, fmt, marker);
    va_end(marker);

    char* lfbufloc = logfilebuf + strlen(logfilebuf);

    U32 deptTotals[eMemMgrDeptTag_NumDepts];
    xMemMgrUsageTrackerGetDeptTotals(deptTotals);

    sprintf(lfbufloc, "***MEMORY USAGE***\n\n");

    for (S32 deptit = 0; deptit < eMemMgrDeptTag_NumDepts; deptit++) {
        if (strlen(logfilebuf) > sizeof(logfilebuf) - 200) {
            iFileWrite(&sMemMgrTrckLog, logfilebuf, strlen(logfilebuf));
            logfilebuf[0] = '\0';
        }
        lfbufloc = logfilebuf + strlen(logfilebuf);
        sprintf(lfbufloc, "*** %-24s %-5d K ***\n", xMemMgrDeptLookupTable[deptit].deptNameString, deptTotals[xMemMgrDeptLookupTable[deptit].deptID] / 1024);
    }

    lfbufloc = logfilebuf + strlen(logfilebuf);
    sprintf(lfbufloc, "\n");

    for (S32 j = 0; j < eMemMgrDeptTag_NumDepts; j++) {
        if (strlen(logfilebuf) > sizeof(logfilebuf) - 200) {
            iFileWrite(&sMemMgrTrckLog, logfilebuf, strlen(logfilebuf));
            logfilebuf[0] = '\0';
        }

        lfbufloc = logfilebuf + strlen(logfilebuf);
        sprintf(lfbufloc, "\n*** %-24s %-5d K ***\n\n", xMemMgrDeptLookupTable[j].deptNameString, deptTotals[xMemMgrDeptLookupTable[j].deptID] / 1024);
        lfbufloc = logfilebuf + strlen(logfilebuf);

        for (S32 i = sMemMgrUsageTrackerTopLine; i < eMemMgrTag_NumTags; i++) {
            if (xMemMgrUsageTrackerLookupTable[i].deptID == xMemMgrDeptLookupTable[j].deptID) {
                if (strlen(logfilebuf) > sizeof(logfilebuf) - 200) {
                    iFileWrite(&sMemMgrTrckLog, logfilebuf, strlen(logfilebuf));
                    logfilebuf[0] = '\0';
                }

                lfbufloc = logfilebuf + strlen(logfilebuf);
                sprintf(lfbufloc, "%-28s %-5d K\n", xMemMgrUsageTrackerLookupTable[i].tagNameString, xMemMgrGetTotalMemUsageByTag(xMemMgrUsageTrackerLookupTable[i].tagID) / 1024);
            }
        }
    }

    lfbufloc = logfilebuf + strlen(logfilebuf);
    sprintf(lfbufloc, "\n");
    iFileWrite(&sMemMgrTrckLog, logfilebuf, strlen(logfilebuf));
}

void xMemMgrUsageTrackerCloseLog()
{
    iFileClose(&sMemMgrTrckLog);
    sMemMgrTrckLogIsOpen = false;
}

void xMemMgrPackerStaticTally(U32, U32)
{
}

void xMemMgrPackerStaticRemove(U32, U32)
{
}

void xMemMgrPackerMallocTally(U32 size, U32 tag)
{
#if RELEASE
    if (xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].maxMem) {
        U32 deptTotals[eMemMgrDeptTag_NumDepts];
        xMemMgrUsageTrackerGetDeptTotals(deptTotals);

        U32 depmaxmemk = xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].maxMem;
        const char* depname = xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].deptNameString;
        U32 deptot = deptTotals[xMemMgrUsageTrackerLookupTable[tag].deptID];

        if (xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].throwAssert) {
            xASSERT_FMT_L(1060, (deptot + size) < ( depmaxmemk * 1024), "%s tried to allocate more memory than it is allowed ( Maximum %d K allowed )", depname, depmaxmemk);
            xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].throwAssert = FALSE;
        }
    }
#endif
    sMemMgrUsageTrackerPackerMallocMem[tag] += size;
}

void xMemMgrPackerMallocRemove(U32 size, U32 tag)
{
    sMemMgrUsageTrackerPackerMallocMem[tag] -= size;
}

void xMemMgrPackerApproxRWMallocTally(U32 size, U32 tag)
{
#if RELEASE
    if (xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].maxMem) {
        U32 deptTotals[eMemMgrDeptTag_NumDepts];
        xMemMgrUsageTrackerGetDeptTotals(deptTotals);

        U32 depmaxmemk = xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].maxMem;
        const char* depname = xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].deptNameString;
        U32 deptot = deptTotals[xMemMgrUsageTrackerLookupTable[tag].deptID];

        if (xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].throwAssert) {
            xASSERT_FMT_L(1090, (deptot + size) < ( depmaxmemk * 1024), "%s tried to allocate more memory than it is allowed ( Maximum %d K allowed )", depname, depmaxmemk);
            xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].throwAssert = FALSE;
        }
    }
#endif
    sMemMgrUsageTrackerPackerApproxRWMallocMem[tag] += size;
}

void xMemMgrPackerApproxRWMallocRemove(U32 size, U32 tag)
{
    sMemMgrUsageTrackerPackerApproxRWMallocMem[tag] -= size;
}

void xMemMgrGenericMallocTally(U32 size, U32 tag)
{
#if RELEASE
    if (xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].maxMem) {
        U32 deptTotals[eMemMgrDeptTag_NumDepts];
        xMemMgrUsageTrackerGetDeptTotals(deptTotals);

        U32 depmaxmemk = xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].maxMem;
        const char* depname = xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].deptNameString;
        U32 deptot = deptTotals[xMemMgrUsageTrackerLookupTable[tag].deptID];

        if (xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].throwAssert) {
            xASSERT_FMT_L(1120, (deptot + size) < ( depmaxmemk * 1024), "%s tried to allocate more memory than it is allowed ( Maximum %d K allowed )", depname, depmaxmemk);
            xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].throwAssert = FALSE;
        }
    }
#endif
    sMemMgrUsageTrackerGenericMallocMem[tag] += size;
}

void xMemMgrGenericMallocRemove(U32 size, U32 tag)
{
    if (size <= sMemMgrUsageTrackerGenericMallocMem[tag]) {
        sMemMgrUsageTrackerGenericMallocMem[tag] -= size;
    } else {
        sMemMgrUsageTrackerGenericMallocMem[tag] = 0;
    }
}

void xMemMgrGenericMallocPushTag(U32 tag)
{
    for (U8 i = 0; i < MAX_GENERIC_MALLOC_TAGS; i++) {
        if (sMemMgrGenericMallocTag[i] == eMemMgrTag_NumTags) {
            sMemMgrGenericMallocTag[i] = tag;
            return;
        }
    }

    xASSERT_L(DEBUG ? 1150 : 1156, 0 &&"sMemMgrGenericMallocTag stack full, can't push another tag");
}

U32 xMemMgrGenericMallocGetTag()
{
    for (U8 i = 0; i < MAX_GENERIC_MALLOC_TAGS; i++) {
        if (sMemMgrGenericMallocTag[i] == eMemMgrTag_NumTags) {
            if (i != 0) {
                return sMemMgrGenericMallocTag[i - 1];
            }
        }
    }

    return sMemMgrGenericMallocTag[MAX_GENERIC_MALLOC_TAGS - 1];
}

void xMemMgrGenericMallocPopTag()
{
    for (U8 i = 0; i < MAX_GENERIC_MALLOC_TAGS; i++) {
        if (sMemMgrGenericMallocTag[i] == eMemMgrTag_NumTags) {
            xASSERT_L(DEBUG ? 1174 : 1180, i &&"Tried to pop without a corresponding push");
            if (i != 0) {
                sMemMgrGenericMallocTag[i - 1] = eMemMgrTag_NumTags;
                return;
            }
        }
    }

    sMemMgrGenericMallocTag[MAX_GENERIC_MALLOC_TAGS - 1] = eMemMgrTag_NumTags;
}

bool xMemMgrGenericMallocVerifyClearTag()
{
    return sMemMgrGenericMallocTag[0] == eMemMgrTag_NumTags;
}

void xMemMgrUsageTracker::Init()
{
    memset(this, 0, sizeof(xMemMgrUsageTracker));
}

void xMemMgrUsageTracker::TrackAllocation(U32 size, U32 tag)
{
    xASSERT_L(DEBUG ? 1195 : 1201, tag < eMemMgrTag_NumTags);

#if RELEASE
    if (xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].maxMem) {
        U32 deptTotals[eMemMgrDeptTag_NumDepts];
        xMemMgrUsageTrackerGetDeptTotals(deptTotals);

        U32 depmaxmemk = xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].maxMem;
        const char* depname = xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].deptNameString;
        U32 deptot = deptTotals[xMemMgrUsageTrackerLookupTable[tag].deptID];

        if (xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].throwAssert) {
            xASSERT_FMT_L(1217, (deptot + size) < ( depmaxmemk * 1024), "%s tried to allocate more memory than it is allowed ( Maximum %d K allowed )", depname, depmaxmemk);
            xMemMgrDeptLookupTable[xMemMgrUsageTrackerLookupTable[tag].deptID].throwAssert = FALSE;
        }
    }
#endif

    usageByTag[tag] += size;
    if (xglobals->MemTrackingLvl >= 2) {
        xMemMgrUsageTrackerDumpLog("%s : Mem Allocated\n", xMemMgrUsageTrackerLookupTable[tag].tagNameString);
    }
}

U32 xMemMgrUsageTracker::GetMemUsageByTag(U32 tag) const
{
    xASSERT_L(DEBUG ? 1226 : 1232, tag < eMemMgrTag_NumTags);
    return usageByTag[tag] + sMemMgrUsageTrackerPackerApproxRWMallocMem[tag] + sMemMgrUsageTrackerPackerMallocMem[tag] + sMemMgrUsageTrackerGenericMallocMem[tag];
}

U32 xMemMgrGetTotalMemUsageByTag(U32 tag)
{
    return gxHeap[0].state[gxHeap[0].state_idx].memoryUsed.GetMemUsageByTag(tag);
}
#endif

void xMemInitHeap(xMemHeap* heap, U32 base, U32 size, U32 flags)
{
    U32 old_base = base;

    xASSERT_L(DEBUG ? 1253 : 1259, heap);
    xASSERT_L(DEBUG ? 1254 : 1260, base);

    S32 align = 1 << ((flags >> 9) & 0x1F);
    if (flags & eMemHeapGrowsDown) {
        base = old_base & -align;
        size -= old_base - base;
    } else {
        base = -align & (old_base + align - 1);
        size -= base - old_base;
    }

    xASSERT_L(DEBUG ? 1270 : 1276, (base & (align-1)) == 0);
    xASSERT_L(DEBUG ? 1271 : 1277, size >= (U32)align);

    heap->state_idx = 0;
    heap->hard_base = base;
    heap->size = size;
    heap->flags = flags;
    heap->lastblk = NULL;

    xHeapState* sp = &heap->state[heap->state_idx];
    sp->curr = base;
    sp->blk_ct = 0;
    sp->free_funcs = NULL;

#if DEBUG || RELEASE
    sp->memoryUsed.Init();
#endif

#if DEBUG
    sp->used = 0;
    sp->wasted = 0;
#endif

    if (flags & eMemHeapBlockList) {
        heap->max_blks = 1 << ((flags >> 14) & 1);
        heap->blk = (xMemBlock*)malloc(heap->max_blks * sizeof(xMemBlock));
        xASSERT_L(DEBUG ? 1302 : 1308, heap->blk != NULL);
        memset(heap->blk, 0, heap->max_blks * sizeof(xMemBlock));
    } else {
        heap->max_blks = 0xFFFF;
        heap->blk = NULL;
    }

    heap->opp_heap[0] = -1;
    heap->opp_heap[1] = -1;
}

static U32 xMemGetBlockInfo(xMemHeap* heap, U32 size, S32 align, xMemBlkInfo* info)
{
#if DEBUG
    S32 guard_size;
#endif
    S32 header_size, total, hdr, pre, block, post, dir;

    xASSERT_L(DEBUG ? 1324 : 1330, heap != NULL);
    xASSERT_L(DEBUG ? 1325 : 1331, info != NULL);

    xHeapState* sp = &heap->state[heap->state_idx];
    dir = (heap->flags & eMemHeapGrowsDown) ? -1 : 1;
#if DEBUG
    guard_size = (heap->flags & eMemHeapGuarded) ? 8 : 0;
#endif
    header_size = (heap->flags & eMemHeapHeaderInBlock) ? (S32)sizeof(xMemBlock) : 0;

    if (heap->flags & eMemHeapGrowsDown) {
#if DEBUG
        hdr = -header_size;
        S32 remainder = (sp->curr - header_size - guard_size - size) & (align - 1);
        block = -header_size - guard_size - size - remainder;
        pre = block - guard_size;
#else
        S32 remainder = (sp->curr - header_size - size) & (align - 1);
        hdr = -header_size;
        block = -(header_size + size + remainder);
        pre = block;
#endif
        post = block + size;
        total = -pre;
    } else {
        hdr = 0;
#if DEBUG
        S32 remainder = (sp->curr + header_size + guard_size) & (align - 1);
#else
        S32 remainder = (sp->curr + header_size) & (align - 1);
#endif
        if (remainder == 0) remainder = align;
#if DEBUG
        block = header_size + guard_size + align - remainder;
        pre = block - guard_size;
        post = block + size;
        total = post + guard_size;
#else
        block = align + header_size - remainder;
        pre = block;
        post = block + size;
        total = post;
#endif
    }

    total = (total + 3) & ~3;

    xASSERT_L(DEBUG ? 1376 : 1382, ((sp->curr + block) & (align-1)) == 0);
    xASSERT_L(DEBUG ? 1377 : 1383, total >= 0);

    if (heap->flags & eMemHeapBlockList) {
        info->header = &heap->blk[sp->blk_ct];
    } else {
        info->header = (xMemBlock*)(sp->curr + hdr);
    }

    info->pre = sp->curr + pre;
    info->block = sp->curr + block;
    info->post = sp->curr + post;
    info->curr = sp->curr + dir * total;
    info->waste = total - (header_size + size);
    info->total = total;

    return total;
}

#if DEBUG || RELEASE
void* xMemGrowAlloc(U32 heapID, U32 size, U32 tag)
#else
void* xMemGrowAlloc(U32 heapID, U32 size)
#endif
{
#if DEBUG
    S32 align;
#endif
    U32 oldalignsize, newalignsize;
    xMemHeap* heap;
    xMemBlock* hdr;
    xHeapState* sp;
    void* memptr;
#if DEBUG
    U32 guard;
#endif

    xASSERT_L(DEBUG ? 1434 : 1440, !iMemHasTempAllocated());

    size = (size + 3) & ~3;

    xASSERT_L(DEBUG ? 1440 : 1446, heapID >= 0 && heapID <= XMEM_MAX_HEAPS);
    heap = &gxHeap[heapID];
    sp = &heap->state[heap->state_idx];
    hdr = heap->lastblk;
#if DEBUG
    align = hdr->align;
#endif
    xASSERT_L(DEBUG ? 1445 : 1451, hdr);

    oldalignsize = hdr->size;
    newalignsize = hdr->size + size;

    if (sp->used + (newalignsize - oldalignsize) > heap->size) {
        iprintf("ERROR grow-allocating %d bytes:  over by %d bytes\n", size, sp->used + size - heap->size);
        xASSERT_FMT_L(DEBUG ? 1457 : 1463, sp->used + size <= heap->size, "out of memory grow-allocating %d bytes\n(over by %d bytes)\n", size, sp->used + size - heap->size);
        return NULL;
    }

    if (heap->flags & eMemHeapGrowsDown) {
        memptr = (void*)(hdr->addr - size);
        hdr->addr -= size;
        sp->curr -= newalignsize - oldalignsize;
#if DEBUG
        guard = hdr->addr - 8;
        iMemSet((void*)guard, 0x50, 8);
        if (guard <= XMEM_GUARD_BREAK_ADDR && guard + 8 > XMEM_GUARD_BREAK_ADDR)
            iprintf("put a breakpoint here\n");
#endif
    } else {
        memptr = (void*)(hdr->addr + hdr->size);
        sp->curr += newalignsize - oldalignsize;
#if DEBUG
        guard = hdr->addr + newalignsize;
        iMemSet((void*)guard, 0x51, 8);
        if (guard <= XMEM_GUARD_BREAK_ADDR && guard + 8 >= XMEM_GUARD_BREAK_ADDR)
            iprintf("put a breakpoint here\n");
#endif
    }

    sp->used += newalignsize - oldalignsize;
#if DEBUG || RELEASE
    sp->memoryUsed.TrackAllocation(size, tag);
#endif
    hdr->size += size;
    memset(memptr, 0, size);
    return memptr;
}

#if DEBUG
void* xMemAlloc(U32 heapID, U32 size, S32 align, U32 tag, U32 assetID, const char* file, const char* func, S32 line)
#elif RELEASE
void* xMemAlloc(U32 heapID, U32 size, S32 align, U32 tag)
#else
void* xMemAlloc(U32 heapID, U32 size, S32 align)
#endif
{
#if RELEASE
    S32 reservedMem, availMem;
#endif
    xMemHeap* heap;
    xMemBlock* hdr;
    xHeapState* sp;
    xMemBlkInfo info;
    U32 i, total;

#if DEBUG
    assetID = 0;
#endif

#if RELEASE
    if (globals.sceneCur && globals.sceneCur->parameters) {
        reservedMem = (zConfigGetSceneMemReserve(globals.sceneCur->sceneID) + globals.sceneCur->parameters->piggybank) * 1024;
        availMem = iMemMgrGetMinFreeSpace();
        xASSERT_DESIGN_L(1555, (availMem + (xMemMgrGetTotalMemUsageByTag( eMemMgrTag_DecalRWMalloc )/1024) - reservedMem) > (S32) size,
                         "Ran out of reserved memory!\nTrying to allocate %d bytes, but only %d available", size, availMem + xMemMgrGetTotalMemUsageByTag(eMemMgrTag_DecalRWMalloc) / 1024 - reservedMem);
    }
#endif

    xASSERT_L(DEBUG ? 1560 : 1566, !iMemHasTempAllocated());
    xASSERT_L(DEBUG ? 1563 : 1569, heapID >= 0 && heapID <= XMEM_MAX_HEAPS);
    heap = &gxHeap[heapID];
    sp = &heap->state[heap->state_idx];

    xASSERT_L(DEBUG ? 1568 : 1574, heap->flags & eMemActive);
    align = MAX(align, 1 << ((heap->flags >> 9) & 0x1F));
    xASSERT_L(DEBUG ? 1572 : 1578, (align & (align - 1)) == 0);

    if (size == 0) {
        xVALIDATE_L(1579, size != 0);
        return (void*)0xDEADBEEF;
    }

    total = xMemGetBlockInfo(heap, size, align, &info);
    hdr = info.header;

#if DEBUG || RELEASE
    sp->memoryUsed.TrackAllocation(total, tag);
#endif

    if (sp->used + total > heap->size) {
        iprintf("ERROR allocating %d bytes:  over by %d bytes\n", total, sp->used + total - heap->size);
#if DEBUG
        iprintf("heap %d asset %08X file %s func %s line %d\n", heapID, assetID, file, func, line);
        xMemCheckHeap(heap, TRUE);
        xASSERT_FMT_L(1607, sp->used + total <= heap->size, "out of memory allocating %d bytes\n(over by %d bytes). Func %s, File %s, line %d, Asset %s\n", total, sp->used + total - heap->size, func, file, line, xSTAssetName(assetID));
#else
        xASSERT_FMT_L(1621, sp->used + total <= heap->size, "out of memory allocating %d bytes\n(over by %d bytes)\n", total, sp->used + total - heap->size);
#endif
        iExceptionMemCrash("xMemAlloc", size, NULL);
        return NULL;
    }

    for (i = 0; i < XMEM_MAX_HEAPS; i++) {
#if DEBUG || RELEASE
        S16 idx = heap->opp_heap[i];
        if (idx != -1) {
            xMemHeap* other = &gxHeap[idx];
            other = &gxHeap[idx];
            xHeapState* op = &other->state[other->state_idx];
            U32 collision = FALSE;
            xASSERT_L(DEBUG ? 1637 : 1647, (((heap->flags ^ other->flags) & eMemHeapGrowsDown) != 0,"xMemMgr internal error"));
            collision = (heap->flags & eMemHeapGrowsDown) ? (sp->curr < op->curr) : (sp->curr > op->curr);
            if (collision) {
                iprintf("Heap collision detected between heap %d and heap %d (%08X <?> %08X)\n", gActiveHeap, idx, sp->curr, op->curr);
            }
            xASSERT_L(DEBUG ? 1644 : 1654, !collision);
        }
#endif
    }

    sp->curr = info.curr;
    sp->blk_ct++;
    if (heap->flags & eMemHeapBlockList) {
        xASSERT_L(DEBUG ? 1652 : 1662, sp->blk_ct <= heap->max_blks);
        if (sp->blk_ct > heap->max_blks)
            iprintf("xMemAlloc ERROR:  too many blocks\n");
        return NULL;
    }

    hdr->addr = info.block;
    hdr->size = size;
    hdr->align = align;

    if (iMemMgrCheckCollisionWithMallocHeap((void*)hdr->addr, hdr->size)) {
        iExceptionMemCrash("xMemAlloc", size, "heap overlaps malloc pool");
    }

#if DEBUG
    iMemSet((void*)info.pre, 0x50, 8);
    iMemSet((void*)info.post, 0x51, 8);
    if ((info.pre <= XMEM_GUARD_BREAK_ADDR && info.pre + 8 > XMEM_GUARD_BREAK_ADDR) ||
        (info.post <= XMEM_GUARD_BREAK_ADDR && info.post + 8 > XMEM_GUARD_BREAK_ADDR)) {
        iprintf("put a breakpoint here\n");
    }

    hdr->assetID = assetID;
    hdr->file = file;
    hdr->line = line;
    hdr->func = func;
#endif

    sp->used += total;
    sp->wasted += info.waste;

    xASSERT_L(DEBUG ? 1704 : 1714, sp->used == (U32)abs((S32)sp->curr - (S32)heap->hard_base));

    memset((void*)hdr->addr, 0, size);
    heap->lastblk = hdr;

#if DEBUG
    DBprintf(DBML_VDBG, "xMemAlloc %7d bytes at %08X (%s:%s line %d)\n", size, hdr->addr, file, func, line);

    if (gMemReporting) {
        if (size >= gMemReportingThreshold) {
            iprintf(" * * * * * * xMemAlloc 0x%05X (%7d) bytes at 0x%08X (%s:%s line %d)\n", size, size, hdr->addr, file, func, line);
        }
    }

    bool tallied = false;
    for (S32 i = 0; i < ARRAY_LENGTH(allocList); i++) {
        if (allocList[i].bytesAllocated > -1 && assetID == allocList[i].assetID) {
            allocList[i].bytesAllocated += size;
            tallied = true;
        }
    }
    if (!tallied) {
        for (S32 i = 0; i < ARRAY_LENGTH(allocList); i++) {
            if (allocList[i].bytesAllocated == -1) {
                allocList[i].assetID = assetID;
                allocList[i].bytesAllocated = size;
                tallied = true;
                break;
            }
        }
        xASSERT_FMT_L(1746, tallied, "Insufficient MAX_ALLOC_IDENTIFIERS to track all allocations");
    }

    static U32 ignoredAllocations[] = { 'LNOD', 'LYR\0', 'ANOD', 'XMDL' };
    bool ignore = false;
    for (U32 i = 0; i < ARRAY_LENGTH(ignoredAllocations); i++) {
        if (assetID == ignoredAllocations[i])
            ignore = true;
    }
    if (!ignore && size > 0x2800) {
        S32 a = 0;
    }
#endif

    return (void*)hdr->addr;
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE(
    "temp->state[temp->state_idx].blk_ct == 0",
    "xMemSwitchHeaps:  unknown type %08X\n",
    "*always*"
)
#endif

#if DEBUG
DECOMP_FORCEACTIVE(
    "xMemTest:\n",
    "### Test static allocations:\n",
    "### Test xMemPushBase():\n",
    "### Test xMemPopBase():\n",
    "### Test xMemSwitchHeaps():\n",
    "### Switch heaps\n",
    "### Allocating...\n",
    "### Test xMemPushTemp():\n",
    "### Test xMemPopTemp():\n",
    "### xMem tests were successful.\n",
    "xMemMgr.h",
    "ptr",
    "for %d bytes from %s, in %s line %d"
)

static U32 xMemCheckHeap(xMemHeap* heap, U32 dump)
{
    xMemBlock* hdr;
    xMemBlkInfo info;
    xHeapState* sp;
    U32 j, total, align, save_curr, heapID;

    xASSERT_L(2093, heap != NULL);

    sp = &heap->state[heap->state_idx];
    heapID = ((U32)heap - (U32)gxHeap) / sizeof(xMemHeap);

    if (dump) {
        iprintf("\n");
        if (heap->flags & eMemHeapGrowsDown) {
            iprintf("xMemCheckHeap %d:  %08X - %08X, %d blocks, curr %08X\n", heapID, heap->hard_base - 1, heap->hard_base - heap->size, sp->blk_ct, sp->curr);
        } else {
            iprintf("xMemCheckHeap %d:  %08X - %08X, %d blocks, curr %08X\n", heapID, heap->hard_base, heap->hard_base + heap->size - 1, sp->blk_ct, sp->curr);
        }
    }

    if (heap->flags & eMemHeapGrowsDown) {
        DBprintf(DBML_VDBG, "xMemCheckHeap %d:  %08X - %08X, %d blocks, curr %08X\n", heapID, heap->hard_base - 1, heap->hard_base - heap->size, sp->blk_ct, sp->curr);
    } else {
        DBprintf(DBML_VDBG, "xMemCheckHeap %d:  %08X - %08X, %d blocks, curr %08X\n", heapID, heap->hard_base, heap->hard_base + heap->size - 1, sp->blk_ct, sp->curr);
    }

    xASSERT_L(2133, heap->hard_base != NULL);
    xASSERT_L(2134, (U32)abs((S32)heap->hard_base - (S32)sp->curr) <= heap->size);
    xASSERT_L(2135, sp->blk_ct <= heap->max_blks);

    save_curr = sp->curr;
    sp->curr = heap->hard_base;
    align = 1 << ((heap->flags >> 9) & 0x1F);

    for (j = 0; j < sp->blk_ct; j++) {
        if (heap->flags & eMemHeapHeaderInBlock) {
            if (heap->flags & eMemHeapGrowsDown) {
                hdr = (xMemBlock*)(sp->curr - sizeof(xMemBlock));
            } else {
                hdr = (xMemBlock*)sp->curr;
            }
        } else {
            hdr = &heap->blk[j];
        }

        if (dump) {
            iprintf("block %d:  %d bytes at %08X   %s:%d\n", j, hdr->size, hdr->addr, hdr->file, hdr->line);
        }

        total = xMemGetBlockInfo(heap, hdr->size, hdr->align, &info);
        sp->curr = info.curr;

        xASSERT_L(2162, (hdr->addr & (hdr->align-1)) == 0);

        if (heap->flags & eMemHeapGrowsDown) {
            xASSERT_L(2165, heap->hard_base >= hdr->addr);
            xASSERT_L(2166, hdr->addr >= sp->curr);
        } else {
            xASSERT_L(2168, heap->hard_base <= hdr->addr);
            xASSERT_L(2169, hdr->addr <= sp->curr);
        }

        iMemVerify((void*)info.pre, 0x50, 8);
        iMemVerify((void*)info.post, 0x51, 8);
    }

    if (sp->used != 0 || sp->wasted != 0) {
        DBprintf(DBML_VDBG, "Heap %08X:  %d blocks allocated  (%s)\n", heap, sp->blk_ct, (heap->flags & eMemActive) ? "active" : "locked");
        DBprintf(DBML_VDBG, "Total bytes allocated:  %9d/%9d (%5.2f %%)\n", sp->used, heap->size,
                 100.0f * sp->used / heap->size);
        DBprintf(DBML_VDBG, "Total bytes wasted:     %9d/%9d (%5.2f %%)\n", sp->wasted, sp->used,
                 100.0f * sp->wasted / sp->used);
    }

    if (dump) {
        iprintf("\n");
    }

    xASSERT_L(2191, sp->curr == save_curr);

    return 0;
}

U32 xMemCheck()
{
    U32 i;
    xMemHeap* heap = gxHeap;

    for (i = 0; i < XMEM_MAX_HEAPS; i++, heap++) {
        xMemCheckHeap(heap, FALSE);
    }
    DBprintf(DBML_VDBG, "xMemCheck() complete.\n");
    return 0;
}
#endif

S32 xMemPushBase(U32 heapID)
{
#if DEBUG
    DBprintf(DBML_VDBG, "xMemPushBase:  heapID %d\n", heapID);
    DBprintf(DBML_VDBG, "xMemCheck:\n", xMemCheck());
#endif

    xASSERT_L(DEBUG ? 2242 : 2252, heapID >= 0 && heapID <= XMEM_MAX_HEAPS);
    xMemHeap* heap = &gxHeap[heapID];

#if DEBUG || RELEASE
    if (heap->state_idx >= XMEMMAXSTACK-1) {
        iprintf("xMemPushBase: ERROR: stack overflow (idx == %d)\n", heap->state_idx);
        xASSERT_L(DEBUG ? 2249 : 2259, heap->state_idx < XMEMMAXSTACK-1);
    }
#endif

    heap->state_idx++;
    xHeapState* sp = &heap->state[heap->state_idx];
    *sp = *(sp - 1);
    sp->free_funcs = NULL;

    if (sMemBaseNotifyFunc) {
        sMemBaseNotifyFunc();
    }

    return heap->state_idx - 1;
}

S32 xMemPushBase()
{
    return xMemPushBase(gActiveHeap);
}

void* xMemAllocFreeable(void* alloced_memory, S32 asked_for_size, xMemFreeFunc func, void* param)
{
    xMemFreeLink* link = (xMemFreeLink*)((U8*)alloced_memory + asked_for_size);
    link->func = func;
    link->param = param;

    xMemHeap* heap = &gxHeap[gActiveHeap];
    link->next = heap->state[heap->state_idx].free_funcs;
    heap->state[heap->state_idx].free_funcs = link;

    return alloced_memory;
}

S32 xMemPopBase(U32 heapID, S32 depth)
{
    xMemHeap* heap;
    xHeapState* sp;
    S32 new_idx;

#if DEBUG
    DBprintf(DBML_VDBG, "xMemPopBase:  heapID %d, depth %d\n", heapID, depth);
    DBprintf(DBML_VDBG, "xMemCheck:\n", xMemCheck());
#endif

    xASSERT_L(DEBUG ? 2318 : 2328, heapID >= 0 && heapID <= XMEM_MAX_HEAPS);
    heap = &gxHeap[heapID];

    if (depth < 0) {
        new_idx = heap->state_idx + depth;
    } else {
        new_idx = depth;
    }

#if DEBUG || RELEASE
    if (new_idx < 0 || new_idx >= XMEMMAXSTACK-1) {
        iprintf("xMemPopBase:  invalid depth (idx == %d)\n", new_idx);
        xASSERT_L(DEBUG ? 2331 : 2341, new_idx >= 0 && new_idx < XMEMMAXSTACK-1);
    }
#endif

    for (S32 popped = heap->state_idx; popped > new_idx; popped--) {
        xMemFreeLink* list = heap->state[popped].free_funcs;
        while (list) {
            list->func(list->param);
            list = list->next;
        }
    }

    heap->state_idx = new_idx;
    sp = &heap->state[heap->state_idx];

    if (sMemBaseNotifyFunc) {
        sMemBaseNotifyFunc();
    }

    return heap->state_idx;
}

S32 xMemPopBase(S32 depth)
{
    return xMemPopBase(gActiveHeap, depth);
}

S32 xMemGetBase(U32 heapID)
{
    xASSERT_L(DEBUG ? 2377 : 2387, heapID >= 0 && heapID <= XMEM_MAX_HEAPS);

    return gxHeap[heapID].state_idx;
}

void xMemRegisterBaseNotifyFunc(xMemBaseNotifyFunc func)
{
    sMemBaseNotifyFunc = func;
}

S32 xMemGetBase()
{
    return xMemGetBase(gActiveHeap);
}

static void xMemPoolAddElements(xMemPool* pool, void* buffer, U32 count)
{
    S32 i;
    void* curr = buffer;
    xMemPoolInitCallback initCB = pool->InitCB;
    U32 next = pool->NextOffset;
    U32 size = pool->Size;

    for (i = 0; i < (S32)count - 1; i++) {
        *(void**)((U8*)curr + next) = (U8*)curr + size;
        if (initCB) initCB(pool, curr);
        curr = (U8*)curr + size;
    }
    *(void**)((U8*)curr + next) = pool->FreeList;
    if (initCB) initCB(pool, curr);

    pool->FreeList = buffer;
    pool->Total += count;
}

void xMemPoolSetup(xMemPool* pool, void* buffer, U32 nextOffset, U32 flags, xMemPoolInitCallback initCB, U32 size, U32 count, U32 numRealloc)
{
    pool->FreeList = NULL;
    pool->NextOffset = nextOffset;
    pool->Flags = flags;
    pool->UsedList = NULL;
    pool->InitCB = initCB;
    pool->Buffer = buffer;
    pool->Size = size;
    pool->NumRealloc = numRealloc;
    pool->Total = 0;

    xMemPoolAddElements(pool, buffer, count);
}

void* xMemPoolAlloc(xMemPool* pool, U32 memtag)
{
    void* retval = pool->FreeList;
    U32 next = pool->NextOffset;
    U32 flags = pool->Flags;

    if (!retval) {
        xASSERT_L(DEBUG ? 2467 : 2477, pool->NumRealloc);
        xMemPoolAddElements(pool, xMEMALLOC_L(pool->NumRealloc * pool->Size, 0, memtag, 0, 2469), pool->NumRealloc);
        retval = pool->FreeList;
    }

    pool->FreeList = *(void**)((U8*)retval + next);

    if (flags & 0x1) {
        *(void**)((U8*)retval + next) = pool->UsedList;
        pool->UsedList = retval;
    }

#if DEBUG
    if (retval) {
        pool->Allocated++;
    }
#endif

    return retval;
}

void xMemPoolFree(xMemPool* pool, void* data)
{
    if (data) {
        void* freeList = pool->FreeList;
        U32 next = pool->NextOffset;
        U32 flags = pool->Flags;

        if (flags & 0x1) {
            void* prev = &pool->UsedList;
            void* curr = pool->UsedList;
            while (curr && curr != data) {
                prev = (U8*)curr + next;
                curr = *(void**)((U8*)curr + next);
            }
            if (curr) {
                *(void**)prev = *(void**)((U8*)curr + next);
            }
        }

#if DEBUG
        void* freeListCheck = pool->FreeList;
        while (freeListCheck) {
            xASSERT_L(2524, freeListCheck != data);
            freeListCheck = *(void**)((U8*)freeListCheck + next);
        }
#endif

        *(void**)((U8*)data + next) = freeList;
        pool->FreeList = data;

#if DEBUG
        xASSERT_L(2535, pool->Allocated != 0);
        pool->Allocated--;
#endif
    }
}

U32 scratch_mem_used;

namespace
{
#define MAX_SCRATCH_ALLOC 128

static U16 scratch_stack[MAX_SCRATCH_ALLOC];
static U8 scratch_buffer[0x4000] ALIGN(16);
static S32 scratch_allocations;
}

void* xMemScratchPush(U32 size, U32 align)
{
    xASSERT_L(DEBUG ? 2566 : 2576, size > 0);
    xASSERT_FMT_L(DEBUG ? 2568 : 2578, scratch_allocations < MAX_SCRATCH_ALLOC, "Reached scratch-pad allocation limit.  Try not to make so many tiny allocations.");

#if DEBUG
    bool rendering = xglobals->inLoadingScreen || RwCameraGetCurrentCamera() != NULL;
#endif

    U8* scratch = scratch_buffer;

    if (align < 4)
        align = 4;
    U32 align_mask = align - 1;
    xASSERT_L(DEBUG ? 2602 : 2612, (align & align_mask) == 0);

    U8* mem = (U8*)((U32)(scratch + scratch_mem_used + align_mask) & ~align_mask);
    U32 used = size + (mem - scratch);
    if (used > sizeof(scratch_buffer))
        return NULL;

    scratch_stack[scratch_allocations] = used - scratch_mem_used;
    scratch_mem_used = used;
    scratch_allocations++;

    return mem;
}

void xMemScratchPop(void* mem)
{
#if DEBUG || RELEASE
    U8* scratch = scratch_buffer;

    xASSERT_L(DEBUG ? 2638 : 2648, scratch_allocations > 0);
    xASSERT_L(DEBUG ? 2639 : 2649, (mem >= scratch) && (mem < (scratch + scratch_mem_used)));

    U16 last_size = scratch_stack[scratch_allocations - 1];
    U8* last_mem = scratch + scratch_mem_used - last_size;
    xASSERT_FMT_L(DEBUG ? 2642 : 2652, mem == last_mem, "Out-of-order scratch-pad deallocation (I think)");
#endif

    scratch_allocations--;
    scratch_mem_used -= scratch_stack[scratch_allocations];
}

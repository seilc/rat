#ifndef XDEBUG_H
#define XDEBUG_H

#include "iDebug.h"

enum en_VERBOSE_MSGLEVEL
{
    DBML_NONE,
    DBML_RELDISP,
    DBML_DISP,
    DBML_USER,
    DBML_ERR,
    DBML_TIME,
    DBML_WARN,
    DBML_VALID,
    DBML_INFO,
    DBML_DBG,
    DBML_TEST,
    DBML_VDBG,
    DBML_SPEW
};

#if DEBUG
#define _xVALIDATE(line, cond, msg)                                                               \
do {                                                                                              \
    static S32 been_here;                                                                         \
    if (!(cond) && !been_here) {                                                                  \
        DBprintf(DBML_VALID, "%s(%d) : (" msg ") in '%s'\n", __FILE__, line, __FUNCTION__);       \
        xDebugValidateFailed();                                                                   \
        been_here = TRUE;                                                                         \
    }                                                                                             \
} while (0)

#define _xVALIDATE_EQ(line, a, b, msg)                                                            \
do {                                                                                              \
    static S32 been_here;                                                                         \
    if (!been_here) {                                                                             \
        size_t l = (a), r = (b);                                                                  \
        if (l != r) {                                                                             \
            DBprintf(DBML_VALID, "%s(%d) : (" msg ") in '%s'\n", __FILE__, line, __FUNCTION__);   \
            xDebugValidateFailed();                                                               \
            been_here = TRUE;                                                                     \
        }                                                                                         \
    }                                                                                             \
} while (0)

#define xVALIDATE(cond) _xVALIDATE(__LINE__, cond, #cond)
#define xVALIDATE_L(line, cond) _xVALIDATE(line, cond, #cond)
#define xVALIDATE_FAIL_L(line) _xVALIDATE(line, 0, "*always*")
#define xVALIDATE_MSG_L(line, cond, msg) _xVALIDATE(line, cond, msg)
#define xVALIDATE_EQ(a, b) _xVALIDATE_EQ(__LINE__, a, b, #a "==" #b)
#define xVALIDATE_EQ_L(line, a, b) _xVALIDATE_EQ(line, a, b, #a "==" #b)
#else
#define xVALIDATE(cond)
#define xVALIDATE_L(line, cond)
#define xVALIDATE_FAIL_L(line)
#define xVALIDATE_MSG_L(line, cond, msg)
#define xVALIDATE_EQ(a, b)
#define xVALIDATE_EQ_L(line, a, b)
#endif

#define xVALIDATE_FAIL() xVALIDATE_FAIL_L(__LINE__)
#define xVALIDATE_MSG(cond, msg) xVALIDATE_MSG_L(__LINE__, cond, msg)

#if DEBUG || RELEASE
#define _xSTR(x) #x

#define _xFAIL(line, condstr, ...)                                                                \
{                                                                                                 \
    xDebug_assert2_info(__FUNCTION__, __FILE__, line, condstr);                                   \
    xDebug_assert2(__VA_ARGS__);                                                                  \
    xDebugStackTrace();                                                                           \
    if (xDebugBoing()) iDebugBreak();                                                             \
}

#define _xASSERT(line, cond, condstr, ...)      \
do {                                            \
    if (!(cond)) {                              \
        _xFAIL(line, condstr, __VA_ARGS__);     \
    }                                           \
} while (0)

#define xASSERT(cond) _xASSERT(__LINE__, cond, _xSTR(cond), "%s", #cond)
#define xASSERT_L(line, cond) _xASSERT(line, cond, _xSTR(cond), "%s", #cond)
#define xASSERT_MSG(cond, msg) _xASSERT(__LINE__, cond, _xSTR(cond), "%s", msg)
#define xASSERT_MSG_L(line, cond, msg) _xASSERT(line, cond, _xSTR(cond), "%s", msg)
#define xASSERT_FMT(cond, ...) _xASSERT(__LINE__, cond, #cond, __VA_ARGS__)
#define xASSERT_FMT_L(line, cond, ...) _xASSERT(line, cond, #cond, __VA_ARGS__)

#define xFAIL_MSG_L(line, msg) do { _xFAIL(line, "*always*", "%s", msg) } while (0)
#define xFAIL_FMT_L(line, ...) do { _xFAIL(line, "*always*", __VA_ARGS__) } while (0)
#define xFAIL_RETURN_VALUE_L(line, retval) do { _xFAIL(line, "*always*", "%s", "*always*") return (retval); } while (0)

#define xFAIL_ONCE_L(line, ...)                                                                   \
do {                                                                                              \
    static bool done = false;                                                                     \
    if (!done) {                                                                                  \
        done = true;                                                                              \
        _xFAIL(line, "*once only*", __VA_ARGS__);                                                 \
    }                                                                                             \
} while (0)

#define _xASSERT_ONCE(line, cond, condstr)      \
do {                                            \
    static bool done = false;                   \
    if (!done && !(cond)) {                     \
        xFAIL_MSG_L(line, condstr);             \
        done = true;                            \
    }                                           \
} while (0)

#define xASSERT_ONCE(cond) _xASSERT_ONCE(__LINE__, cond, #cond)
#define xASSERT_ONCE_L(line, cond) _xASSERT_ONCE(line, cond, #cond)

#define _xASSERT_DESIGN(line, cond, condstr, ...)                                                 \
do {                                                                                              \
    static bool already_asserted = false;                                                         \
    if (!already_asserted && !(cond)) {                                                           \
        already_asserted = true;                                                                  \
        _xFAIL(line, condstr, __VA_ARGS__);                                                       \
    }                                                                                             \
} while (0)

#define xASSERT_DESIGN(cond, ...) _xASSERT_DESIGN(__LINE__, cond, _xSTR(cond), __VA_ARGS__)
#define xASSERT_DESIGN_L(line, cond, ...) _xASSERT_DESIGN(line, cond, _xSTR(cond), __VA_ARGS__)

#define _xASSERT_EQ(line, a, b, msg)                                                              \
do {                                                                                              \
    size_t il = (a), ir = (b);                                                                    \
    _xASSERT(line, il == ir, _xSTR(il == ir), "%s", msg);                                         \
} while (0)

#define xASSERT_EQ(a, b) _xASSERT_EQ(__LINE__, a, b, #a "==" #b)
#define xASSERT_EQ_L(line, a, b) _xASSERT_EQ(line, a, b, #a "==" #b)

#define _xASSERT_LE(line, a, b, msg)                                                              \
do {                                                                                              \
    size_t il = (a), ir = (b);                                                                    \
    _xASSERT(line, il <= ir, _xSTR(il <= ir), "%s", msg);                                         \
} while (0)

#define xASSERT_LE(a, b) _xASSERT_LE(__LINE__, a, b, #a "<=" #b)
#define xASSERT_LE_L(line, a, b) _xASSERT_LE(line, a, b, #a "<=" #b)
#else
#define _xFAIL(line, condstr, ...)
#define xASSERT(cond)
#define xASSERT_L(line, cond)
#define xASSERT_MSG(cond, msg)
#define xASSERT_MSG_L(line, cond, msg)
#define xASSERT_FMT(cond, ...)
#define xASSERT_FMT_L(line, cond, ...)
#define xFAIL_MSG_L(line, msg)
#define xFAIL_FMT_L(line, ...)
#define xFAIL_RETURN_VALUE_L(line, retval) return (retval)
#define xFAIL_ONCE_L(line, ...)
#define xASSERT_ONCE(cond)
#define xASSERT_ONCE_L(line, cond)
#define xASSERT_DESIGN(cond, ...)
#define xASSERT_DESIGN_L(line, cond, ...)
#define xASSERT_EQ(a, b)
#define xASSERT_EQ_L(line, a, b)
#define xASSERT_LE(a, b)
#define xASSERT_LE_L(line, a, b)
#endif

#define xFAIL_L(line) xFAIL_MSG_L(line, "*always*")
#define xFAIL() xFAIL_L(__LINE__)
#define xFAIL_FMT(...) xFAIL_FMT_L(__LINE__, __VA_ARGS__)
#define xFAIL_MSG(msg) xFAIL_MSG_L(__LINE__, msg)
#define xFAIL_RETURN_VALUE(retval) xFAIL_RETURN_VALUE_L(__LINE__, retval)
#define xFAIL_ONCE(...) xFAIL_ONCE_L(__LINE__, __VA_ARGS__)

#define _xFAIL_RETURN_IF(line, cond, condstr, retval)                                             \
do {                                                                                              \
    if ((cond)) {                                                                                 \
        _xFAIL(line, "*always*", "%s", condstr)                                                   \
        return retval;                                                                            \
    }                                                                                             \
} while (0)

#define xFAIL_RETURN_IF(cond) _xFAIL_RETURN_IF(__LINE__, cond, #cond, )
#define xFAIL_RETURN_IF_L(line, cond) _xFAIL_RETURN_IF(line, cond, #cond, )
#define xFAIL_RETURN_VALUE_IF(cond, retval) _xFAIL_RETURN_IF(__LINE__, cond, #cond, (retval))
#define xFAIL_RETURN_VALUE_IF_L(line, cond, retval) _xFAIL_RETURN_IF(line, cond, #cond, (retval))

#define xFIX_NAN_L(line, x)                     \
if ((x) && !((x) > 0.0f) && !((x) < 0.0f)) {    \
    static bool done = false;                   \
    if (!done) {                                \
        xFAIL_MSG_L(line, "NAN");               \
        done = true;                            \
    }                                           \
    (x) = 0.0f;                                 \
}

#define xFIX_NAN(x) xFIX_NAN_L(__LINE__, x)

#if DEBUG || RELEASE
#define xFIX_NAN_ELEM_L(line, x)                \
if ((x) && !((x) > 0.0f) && !((x) < 0.0f)) {    \
    xFAIL_MSG_L(line, "NAN");                   \
    (x) = 0.0f;                                 \
}
#else
#define xFIX_NAN_ELEM_L(line, x)
#endif

#define xFIX_NAN_ELEM(x) xFIX_NAN_ELEM_L(__LINE__, x)

typedef void(*xDebugModeCallback)();

void xDebugValidateFailed();

inline char* xbtoa(U32 b)
{
    return (char*)(b ? "true " : "false");
}

struct xSB;
struct xVec2;
struct xVec3;

bool xDebugIDontWantYourXprintsOnMyScreen();
void xSBCreate(xSB* sb, U32 size);
char* xSBAppend(xSB* sb, const char* str);
void xSBRender(xSB* sb);
void xSBReset(xSB* sb);
void xSBDestroy(xSB* sb);
void DBStartup(en_VERBOSE_MSGLEVEL setlvl, char* logfile, char* errfile, void (*msgnotify)(en_VERBOSE_MSGLEVEL, const char*));
void DBShutdown();
en_VERBOSE_MSGLEVEL DBCurMsgLvl(); // UNUSED
S32 DBChkMsgLvl(en_VERBOSE_MSGLEVEL msglvl);
void xDebugVSync();
void xprintf(const char* fmt, ...);
void xDebugReportAssertion(const char* sfile, S32 line, const char* func, const char* expr, const char* message);
void xDebugModeSet(S32 mode);
void xDebugModeSetByName(const char* name);
char* xDebugModeGetName(); // UNUSED
void xDebugRenderLoopStart();
void xDebugRenderLoopEnd();
char* xDebugGetNextDumpFile();
void xDebugAdvanceDumpFile();
void xDebugModeService();
void xDebugGraphUsageMsg();
void xDebugGraphService();
void xDebugInit();
void xDebugUpdate();
void xDebugExit();
void xDebugUpdateLoadStats(U32 bytes_read, F32 sec); // UNUSED
void xDebugReportLoadStats(char* msg, U32 flags);
void xDebugTimestampScreen();
void xDebugAssertStartup();
void xDebugAssertShutdown();
void xDebugTextureReport();
void xDrawTextInit(U32 uBufferSize, U32 uNumLinesMax);
void xDrawTextExit();
void xDrawTextRender();
void xDrawText(const xVec3& vWorld, const char* fmt, ...);
void xDrawText(const xVec3& vWorld, F32 fWidth, F32 fHeight, const char* pszText);
bool xDrawTextComputeScreenPt(xVec2& vUnitScreen, const xVec3& vWorldSpace);

#if DEBUG || RELEASE
void DBprintf(en_VERBOSE_MSGLEVEL msglvl, const char* fmt, ...);
void xDebug_assert2_info(const char* func, const char* file, U32 line, const char* expr);
void xDebug_assert2(const char* fmt, ...);
U32 xDebugBoing();
#else
#define DBprintf(...)
#endif

S32 xDebugModeAdd(const char* debugModeName, xDebugModeCallback func);
void xDebugModeRemove(S32 mode); // UNUSED
void xDebugModeRegisterCallback(S32 mode, xDebugModeCallback func);

#if DEBUG || RELEASE
S32 xDebugModeGet();
void xDebugStackTrace();
void xDebugAssertScreen(const char* str1, const char* str2, const char* pretty_str);
void debug_mode_page();

enum xScreenDump
{
    exSD_NONE,
    exSD_NATIVE,
    exSD_MASSIVE,
    exSD_MASSIVE_RESET
};

extern xScreenDump gScreenDump;
extern char gScreenDumpPath[256];
extern U32 xprintf_display;
extern U32 xprintf_display_modes;
#endif

extern U32 gFrameCount;

typedef struct tagBITMAPFILEHEADER
{
    S32 bfSize;
    S16 bfReserved1;
    S16 bfReserved2;
    S32 bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
    S32 biSize;
    S32 biWidth;
    S32 biHeight;
    S16 biPlanes;
    S16 biBitCount;
    S32 biCompression;
    S32 biSizeImage;
    S32 biXPelsPerMeter;
    S32 biYPelsPerMeter;
    S32 biClrUsed;
    S32 biClrImportant;
} BITMAPINFOHEADER;

struct BmpMegaHeader
{
    char prepad[2];
    char bfType[2];
    BITMAPFILEHEADER bmFile;
    BITMAPINFOHEADER bmInfo;
};

#if DEBUG || RELEASE
struct xStats
{
    S64 cpu;
    S64 render;
    S64 world_render;
    S64 model_render;
    S64 ent_update;
    S64 shadow;
    S64 shadow_track;
    S64 shadow_texrender;
    S64 shadow_collideworld;
    S64 shadow_renderworld;
    S64 shadow_receivesetup;
    S64 shadow_frustumtest;
    S64 shadow_receive;
    S64 collision;
    S64 anim;
    S64 particle;
    S64 sound;
    S64 font;
    S64 villainUpdate;
    S64 villainCollide;
    S64 villainDBG;
    S64 villainInput;
    S64 villainAnim;
    S64 zprjUpdate;
    S64 zprjCollide;
    S64 zprjEffects;
    S64 zprjSolver;
    S64 player_update;
    S64 particleRender;
    S64 renderUI;
    S64 scene_update;
    S64 scene_render;
    S64 x1;
    S64 x2;
    S64 x3;
};

extern xStats gStats;
extern xStats gPendingStats;

#define xSTATS_BEGIN(t) iTime t = iTimeGet()
#define xSTATS_END(t, stat) gPendingStats.stat += iTimeGet() - (t)
#else
#define xSTATS_BEGIN(t)
#define xSTATS_END(t, stat)
#endif

#if DEBUG || RELEASE
#define xTWEAKABLE
#else
#define xTWEAKABLE const
#endif

#endif

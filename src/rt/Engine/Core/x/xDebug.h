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

#define xFAIL_L(line) do { _xFAIL(line, "*always*", "%s", "*always*") } while (0)
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
        xFAIL_FMT_L(line, "%s", condstr);       \
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
#else
#define _xFAIL(line, condstr, ...)
#define xASSERT(cond)
#define xASSERT_L(line, cond)
#define xASSERT_MSG(cond, msg)
#define xASSERT_MSG_L(line, cond, msg)
#define xASSERT_FMT(cond, ...)
#define xASSERT_FMT_L(line, cond, ...)
#define xFAIL_L(line)
#define xFAIL_FMT_L(line, ...)
#define xFAIL_RETURN_VALUE_L(line, retval) return (retval)
#define xFAIL_ONCE_L(line, ...)
#define xASSERT_ONCE(cond)
#define xASSERT_ONCE_L(line, cond)
#define xASSERT_DESIGN(cond, ...)
#define xASSERT_DESIGN_L(line, cond, ...)
#define xASSERT_EQ(a, b)
#define xASSERT_EQ_L(line, a, b)
#endif

#define xFAIL() xFAIL_L(__LINE__)
#define xFAIL_FMT(...) xFAIL_FMT_L(__LINE__, __VA_ARGS__)
#define xFAIL_MSG_L(line, msg) xASSERT_FMT_L(line, (0), msg)
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
        xFAIL_FMT_L(line, "%s", "NAN");         \
        done = true;                            \
    }                                           \
    (x) = 0.0f;                                 \
}

#define xFIX_NAN(x) xFIX_NAN_L(__LINE__, x)

typedef void(*xDebugModeCallback)();

void xDebugValidateFailed();

inline char* xbtoa(U32 b)
{
    return (char*)(b ? "true " : "false");
}

void xprintf(const char* fmt, ...);

#if DEBUG || RELEASE
void DBprintf(en_VERBOSE_MSGLEVEL msglvl, const char* fmt, ...);
void xDebug_assert2_info(const char* func, const char* file, U32 line, const char* expr);
void xDebug_assert2(const char* fmt, ...);
U32 xDebugBoing();
#else
#define DBprintf(...)
#endif

S32 xDebugModeAdd(const char* debugModeName, xDebugModeCallback func);
void xDebugModeRegisterCallback(S32 mode, xDebugModeCallback func);

#if DEBUG || RELEASE
S32 xDebugModeGet();
void xDebugStackTrace();

enum xScreenDump
{
    exSD_NONE,
    exSD_NATIVE,
    exSD_MASSIVE,
    exSD_MASSIVE_RESET
};

extern xScreenDump gScreenDump;
#endif

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

extern xStats gPendingStats;

#define xSTATS_BEGIN(t) iTime t = iTimeGet()
#define xSTATS_END(t, stat) gPendingStats.stat += iTimeGet() - (t)
#else
#define xSTATS_BEGIN(t)
#define xSTATS_END(t, stat)
#endif

#endif

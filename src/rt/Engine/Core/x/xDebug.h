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

#ifdef DEBUGRELEASE
#define xWARN(...) DBprintf(DBML_WARN, __VA_ARGS__)
#define xVERBOSE(...) DBprintf(DBML_VDBG, __VA_ARGS__)
#else
#define xWARN(...)
#define xVERBOSE(...)
#endif

#ifdef DEBUG
#define _xVALIDATEFAIL(line, cond, msg)                                                           \
do {                                                                                              \
    static S32 been_here;                                                                         \
    if (!been_here) {                                                                             \
        DBprintf(DBML_VALID, "%s(%d) : (" msg ") in '%s'\n", __FILE__, line, __FUNCTION__);       \
        xDebugValidateFailed();                                                                   \
        been_here = TRUE;                                                                         \
    }                                                                                             \
} while (0)

#define xVALIDATE(line, cond)                                                                     \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        _xVALIDATEFAIL(line, cond, #cond);                                                        \
    }                                                                                             \
} while (0)

#define xVALIDATEM(line, cond, msg)                                                               \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        _xVALIDATEFAIL(line, cond, msg);                                                          \
    }                                                                                             \
} while (0)
#else
#define xVALIDATE(line, cond)
#define xVALIDATEM(line, cond, msg)
#endif

#ifdef DEBUGRELEASE
#define _xASSERTFAIL(line, cond, ...)                                                             \
do {                                                                                              \
    xDebug_assert2_info(__FUNCTION__, __FILE__, line, #cond);                                     \
    xDebug_assert2(__VA_ARGS__);                                                                  \
    xDebugStackTrace();                                                                           \
    if (xDebugBoing()) iDebugBreak();                                                             \
} while (0)

#define xASSERT(line, cond)                                                                       \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        _xASSERTFAIL(line, cond, "%s", #cond);                                                    \
    }                                                                                             \
} while (0)

#define xASSERTM(line, cond, ...)                                                                 \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        _xASSERTFAIL(line, ##cond, __VA_ARGS__);                                                  \
    }                                                                                             \
} while (0)

#define xFAIL(line) _xASSERTFAIL(line, *always*, "%s", "*always*")
#define xFAILM(line, ...) _xASSERTFAIL(line, *always*, __VA_ARGS__)

#define xFAIL_ONCE_M(line, ...)                                                                   \
do {                                                                                              \
    static bool done = false;                                                                     \
    if (!done) {                                                                                  \
        done = true;                                                                              \
        _xASSERTFAIL(line, *once only*, __VA_ARGS__);                                             \
    }                                                                                             \
} while (0)

#define xASSERT_ONCE(line, cond)                                                                  \
do {                                                                                              \
    static bool done = false;                                                                     \
    if (!done && !(cond)) {                                                                       \
        xFAILM(line, "%s", #cond);                                                                \
        done = true;                                                                              \
    }                                                                                             \
} while (0)

#define xASSERT_DESIGN_M(line, cond, msg)                                                         \
do {                                                                                              \
    static bool already_asserted = false;                                                         \
    if (!already_asserted && !(cond)) {                                                           \
        already_asserted = true;                                                                  \
        _xASSERTFAIL(line, cond, msg);                                                            \
    }                                                                                             \
} while (0)
#else
#define xASSERT(line, cond)
#define xASSERTM(line, cond, ...)
#define xFAIL(line)
#define xFAILM(line, ...)
#define xFAIL_ONCE_M(line, ...)
#define xASSERT_ONCE(line, cond)
#define xASSERT_DESIGN_M(line, cond, msg)
#endif

#define xFAIL_AND_RETURN_IF(line, cond)                                                                     \
do {                                                                                              \
    if ((cond)) {                                                                                 \
        xFAILM(line, "%s", #cond);                                                                \
        return;                                                                                   \
    }                                                                                             \
} while (0)

#define xFAIL_AND_RETURN_VALUE_IF(line, cond, retval)                                                            \
do {                                                                                              \
    if ((cond)) {                                                                                 \
        xFAILM(line, "%s", #cond);                                                                \
        return (retval);                                                                          \
    }                                                                                             \
} while (0)

typedef void(*xDebugModeCallback)();

void xDebugValidateFailed();
void xprintf(const char* fmt, ...);

#ifdef DEBUGRELEASE
void DBprintf(en_VERBOSE_MSGLEVEL msglvl, const char* fmt, ...);
void xDebug_assert2_info(const char* func, const char* file, U32 line, const char* expr);
void xDebug_assert2(const char* fmt, ...);
U32 xDebugBoing();
#endif

S32 xDebugModeAdd(const char* debugModeName, xDebugModeCallback func);

#ifdef DEBUGRELEASE
S32 xDebugModeGet();
void xDebugStackTrace();
#endif

#endif

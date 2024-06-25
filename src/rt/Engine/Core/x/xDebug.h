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

#ifdef DEBUG
#define xVALIDATE(line, cond)                                                                     \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        static S32 been_here;                                                                     \
        if (!been_here) {                                                                         \
            DBprintf(DBML_VALID, "%s(%d) : (" #cond ") in '%s'\n", __FILE__, line, __FUNCTION__); \
            xDebugValidateFailed();                                                               \
            been_here = TRUE;                                                                     \
        }                                                                                         \
    }                                                                                             \
} while (0)
#else
#define xVALIDATE(line, cond)
#endif

#ifdef DEBUGRELEASE  
#define xASSERT(line, cond)                                                                       \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        xDebug_assert2_info(__FUNCTION__, __FILE__, line, #cond);                                 \
        xDebug_assert2("%s", #cond);                                                              \
        xDebugStackTrace();                                                                       \
        if (xDebugBoing()) iDebugBreak();                                                         \
    }                                                                                             \
} while (0)

#define xASSERTMSG(line, cond, msg)                                                               \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        xDebug_assert2_info(__FUNCTION__, __FILE__, line, #cond);                                 \
        xDebug_assert2(msg);                                                                      \
        xDebugStackTrace();                                                                       \
        if (xDebugBoing()) iDebugBreak();                                                         \
    }                                                                                             \
} while (0);

#define xASSERTFMT(line, cond, fmt, ...)                                                          \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        xDebug_assert2_info(__FUNCTION__, __FILE__, line, #cond);                                 \
        xDebug_assert2(fmt, __VA_ARGS__);                                                         \
        xDebugStackTrace();                                                                       \
        if (xDebugBoing()) iDebugBreak();                                                         \
    }                                                                                             \
} while (0);
#else
#define xASSERT(line, cond)
#define xASSERTMSG(line, cond, msg)
#define xASSERTFMT(line, cond, fmt, ...)
#endif

void xDebugValidateFailed();

#ifdef DEBUGRELEASE
void DBprintf(en_VERBOSE_MSGLEVEL msglvl, const char* fmt, ...);
void xDebug_assert2_info(const char* func, const char* file, U32 line, const char* expr);
void xDebug_assert2(const char* fmt, ...);
U32 xDebugBoing();
void xDebugStackTrace();
#endif

#endif

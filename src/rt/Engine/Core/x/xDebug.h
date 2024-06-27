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
#define _xASSERT(line, cond, ...)                                                                 \
do {                                                                                              \
    if (!(cond)) {                                                                                \
        xDebug_assert2_info(__FUNCTION__, __FILE__, line, #cond);                                 \
        xDebug_assert2(__VA_ARGS__);                                                              \
        xDebugStackTrace();                                                                       \
        if (xDebugBoing()) iDebugBreak();                                                         \
    }                                                                                             \
} while (0);

#define xASSERT(line, cond) _xASSERT(line, cond, "%s", #cond)
#define xASSERTMSG(line, cond, msg) _xASSERT(line, cond, msg)
#define xASSERTFMT(line, cond, fmt, ...) _xASSERT(line, cond, fmt, __VA_ARGS__)
#else
#define xASSERT(line, cond)
#define xASSERTMSG(line, cond, msg)
#define xASSERTFMT(line, cond, fmt, ...)
#endif

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

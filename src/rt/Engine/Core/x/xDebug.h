#ifndef XDEBUG_H
#define XDEBUG_H

#include "iDebug.h"

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
#else
#define xASSERT(line, cond)
#endif

#ifdef DEBUGRELEASE
void xDebug_assert2_info(const char* func, const char* file, U32 line, const char* expr);
void xDebug_assert2(const char* fmt, ...);
U32 xDebugBoing();
void xDebugStackTrace();
#endif

#endif
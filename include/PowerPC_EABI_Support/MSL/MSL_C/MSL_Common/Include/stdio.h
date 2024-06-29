#ifndef _MSL_STDIO_H
#define _MSL_STDIO_H

#include <size_t.h>
#include <va_list.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int sprintf(char*, const char*, ...);
extern int vsprintf(char* s, const char* format, va_list arg);

#ifdef __cplusplus
}
#endif

#endif

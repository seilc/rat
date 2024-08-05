#ifndef XDEBUGGRAPH_H
#define XDEBUGGRAPH_H

#include "types.h"

void xDebugGraphSetScale(const char* name, F32 scale);
void xDebugGraphSetOriginValue(const char* name, F32 originValue);
void xDebugGraphAdd(const char* name, S32 value);

#endif

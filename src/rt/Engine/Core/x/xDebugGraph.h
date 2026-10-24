#ifndef XDEBUGGRAPH_H
#define XDEBUGGRAPH_H

#include "types.h"

void xDebugGraphSceneEnter();
void xDebugGraphSceneExit();
void xDebugGraphUpdate(F32 dt);
void xDebugGraphRender();
void xDebugGraphSetScale(const char* name, F32 scale);
void xDebugGraphSetOriginValue(const char* name, F32 originValue);
void xDebugGraphAdd(const char* name, F32 value);
void xDebugGraphAdd(const char* name, S32 value);
void xDebugGraphBind(const char* name, const F32* value, F32 update_delay);

#endif

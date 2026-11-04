#ifndef XDEBUGGRAPH_H
#define XDEBUGGRAPH_H

#include "xColor.h"

void xDebugGraphSceneEnter();
void xDebugGraphSceneExit();
void xDebugGraphUpdate(F32 dt);
void xDebugGraphRender();
void xDebugGraphReset(const char* name);
void xDebugGraphClear(const char* name);
void xDebugGraphShow(const char* name);
void xDebugGraphHide(const char* name);
void xDebugGraphPause(const char* name);
void xDebugGraphUnpause(const char* name);
void xDebugGraphSetColor(const char* name, xColor color);
void xDebugGraphSetScale(const char* name, F32 scale);
void xDebugGraphSetOriginValue(const char* name, F32 originValue);
void xDebugGraphSetOffset(const char* name, F32 offset);
void xDebugGraphAdd(const char* name, F32 value);
void xDebugGraphAdd(const char* name, S32 value);
void xDebugGraphBind(const char* name, const F32* value, F32 update_delay);
void xDebugGraphBind(const char* name, const S32* value, F32 update_delay);

#endif

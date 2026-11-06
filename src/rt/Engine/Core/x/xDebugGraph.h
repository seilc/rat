#ifndef XDEBUGGRAPH_H
#define XDEBUGGRAPH_H

#include "xColor.h"

void xDebugGraphSceneEnter();
void xDebugGraphSceneExit();
void xDebugGraphUpdate(F32 dt);
void xDebugGraphRender();
void xDebugGraphReset(const char* name); // UNUSED
void xDebugGraphClear(const char* name); // UNUSED
void xDebugGraphShow(const char* name); // UNUSED
void xDebugGraphHide(const char* name); // UNUSED
void xDebugGraphPause(const char* name); // UNUSED
void xDebugGraphUnpause(const char* name); // UNUSED
void xDebugGraphSetColor(const char* name, xColor color); // UNUSED
void xDebugGraphSetScale(const char* name, F32 scale);
void xDebugGraphSetOriginValue(const char* name, F32 originValue);
void xDebugGraphSetOffset(const char* name, F32 offset); // UNUSED
void xDebugGraphAdd(const char* name, F32 value);
void xDebugGraphAdd(const char* name, S32 value);
void xDebugGraphBind(const char* name, const F32* value, F32 update_delay);
void xDebugGraphBind(const char* name, const S32* value, F32 update_delay); // UNUSED

#endif

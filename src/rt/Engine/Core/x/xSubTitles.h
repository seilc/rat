#ifndef XSUBTITLES_H
#define XSUBTITLES_H

#include "types.h"

void xSubTitlesInit();
void* xSubTitlesRead(void*, U32 aid, void* indata, U32 insize, U32* outsize);
void xSubTitlesUnloaded(void* userdata, U32);
bool xSubTitlesRender(U32 aid, F32 fTime);
void xSubTitlesSetSize(F32 size);
F32 xSubTitlesGetSize();

#endif

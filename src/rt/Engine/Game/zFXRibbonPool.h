#ifndef ZFXRIBBONPOOL_H
#define ZFXRIBBONPOOL_H

#include "types.h"

struct xFXRibbon;

void zFXRibbonPoolInit(U32 maxNumRibbons);
xFXRibbon* zFXRibbonPoolNew();
void zFXRibbonPoolDelete(xFXRibbon*& ribbon);
void zFXRibbonPool_SceneReset();

#endif

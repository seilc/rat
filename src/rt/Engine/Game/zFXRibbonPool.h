#ifndef ZFXRIBBONPOOL_H
#define ZFXRIBBONPOOL_H

#include "types.h"

struct xFXRibbon;

xFXRibbon* zFXRibbonPoolNew();
void zFXRibbonPoolDelete(xFXRibbon*& ribbon);

#endif

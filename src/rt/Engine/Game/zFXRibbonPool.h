#ifndef ZFXRIBBONPOOL_H
#define ZFXRIBBONPOOL_H

struct xFXRibbon;

xFXRibbon* zFXRibbonPoolNew();
void zFXRibbonPoolDelete(xFXRibbon*& ribbon);

#endif

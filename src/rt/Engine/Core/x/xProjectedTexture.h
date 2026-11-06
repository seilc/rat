#ifndef XPROJECTEDTEXTURE_H
#define XPROJECTEDTEXTURE_H

struct RpAtomic;
struct xMat4x3;
struct xModelInstance;

void xProjectedTextureStartup();
void xProjectedTextureShutdown();
void xProjectedTextureRender(RpAtomic*, const xMat4x3&); // UNUSED
void xProjectedTextureRender(xModelInstance*, const xMat4x3&); // UNUSED

#endif

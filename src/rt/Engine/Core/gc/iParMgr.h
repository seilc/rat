#ifndef IPARMGR_H
#define IPARMGR_H

#include "types.h"

struct xMat4x3;
struct xPar;
struct xParCmdTex;
struct xParGroup;

void iParMgrInit();
void iParMgrUpdate(F32);
void iParMgrRender(); // UNUSED
void iParMgrRenderParSys_Sprite(void* data, xParGroup* ps);
void iRenderSetCameraViewMatrix(xMat4x3* m);
void iRenderPushQuadStreak(xPar* p, xParCmdTex* tex);
void iRenderFlush();
void iRenderTrianglesImmediate(S32, S32, void* vertices, S32 vertexCount, U16* indices, S32 indexCount);
void iParMgrRenderParSys_Streak(void* data, xParGroup* ps);
void iParMgrRenderParSys_InvStreak(void* data, xParGroup* ps);
void iParMgrRenderParSys_QuadStreak(void* data, xParGroup* ps);
void iParMgrRenderParSys_Static(void* data, xParGroup*);
void iParMgrRenderParSys_Ground(void* data, xParGroup* ps);
void iParMgrRenderParSys_Flat(void* data, xParGroup* ps);

#endif

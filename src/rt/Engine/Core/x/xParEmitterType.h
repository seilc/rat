#ifndef XPAREMITTERTYPE_H
#define XPAREMITTERTYPE_H

#include "types.h"

struct xEnt;
struct xMat4x3;
struct xPar;
struct xParEmitter;
struct xParEmitterAsset;
struct xVec3;
class xVolume;

void xParEmitterEmitPoint(xPar* p, xParEmitterAsset* a, F32);
void xParEmitterEmitCircleEdge(xPar* p, xParEmitterAsset* a, F32);
void xParEmitterEmitCircle(xPar* p, xParEmitterAsset* a, F32);
void xParEmitterEmitRect(xPar* p, xParEmitterAsset* a, F32);
void xParEmitterEmitRectEdge(xPar* p, xParEmitterAsset* a, F32);
void xParEmitterEmitLine(xPar* p, xParEmitterAsset* a, F32);
void xParEmitterEmitSphere(xPar* p, xParEmitterAsset* a, F32);
void xParEmitterEmitSphereEdge(xPar* p, xParEmitterAsset* a, F32, S32 subtype);
void xParEmitterEmitVolume(xPar* p, xParEmitterAsset* a, F32, xVolume* vol);
void xParEmitterEmitEntity(xPar* p, xParEmitterAsset* a, F32, xEnt* ent);
void xParEmitterEmitOffsetPoint(xParEmitter* pe, xPar* p, xParEmitterAsset* a, F32, xEnt* ent);
void xParEmitterEmitVCylEdge(xPar* p, xParEmitterAsset* a, F32);
void xParEmitterEmitOCircleEdge(xPar* p, xParEmitterAsset* a, F32 dt);
void xParEmitterEmitOCircle(xPar* p, xParEmitterAsset* a, F32 dt);
xMat4x3* xParEmitterTransformEntBone(xVec3& loc, xVec3& vel, const xParEmitterAsset& a, const xEnt& ent);
void xParEmitterTransformEntBone(xVec3& loc, xVec3& vel, const xParEmitterAsset& a, const xMat4x3& mat);
void xParEmitterEmitEntBone(xPar* p, xParEmitterAsset* a, F32, const xMat4x3& mat);
void xParEmitterEmitEntBound(xPar* p, xParEmitterAsset* a, F32, const xEnt* ent);

#endif

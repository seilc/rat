#ifndef XDRAW_H
#define XDRAW_H

#include "xColor.h"
#include "xMath2.h"
#include "xMath3.h"

struct xOBB;
class xCone;
struct xQCData;
struct xConic;

#if DEBUG || RELEASE
void xDrawBegin();
void xDrawEnd();
void xDrawSetColor(xColor color);
void xDrawSetColor(U8 r, U8 g, U8 b, U8 a);
void xDrawLine(const xVec3* a, const xVec3* b);
void xDrawLine(F32 x1, F32 y1, F32 z1, F32 x2, F32 y2, F32 z2);
void xDrawArrow(const xVec3* a, const xVec3* b, const xVec3* n, bool asRay, F32 length);
void xDrawArrow(const xVec3* from, const xVec3* dir, bool asRay, F32 length);
void xDrawGrid(const xVec3& c1, const xVec3& c2, const xVec3& up, F32 spacing_hint);
void xDrawArrowXZ(const xVec3* a, const xVec3* b, bool asRay, F32 length);
void xDrawArrowXZ(F32 x1, F32 y1, F32 z1, F32 x2, F32 y2, F32 z2, bool asRay, F32 length);
void xDrawArrowXY(const xVec3* a, const xVec3* b, bool asRay, F32 length);
void xDrawArrowXY(F32 x1, F32 y1, F32 z1, F32 x2, F32 y2, F32 z2, bool asRay, F32 length);
void xDrawArrowYZ(const xVec3* a, const xVec3* b, bool asRay, F32 length);
void xDrawArrowYZ(F32 x1, F32 y1, F32 z1, F32 x2, F32 y2, F32 z2, bool asRay, F32 length);
void xDrawLine2D(F32 x1, F32 y1, F32 x2, F32 y2);
void xDrawLine2D(const xVec2* a, const xVec2* b);
void xDrawPolyLine2D(const xVec2* p, U32 len);
void xDrawCircle2D(F32 x, F32 y, F32 radius, S32 segments);
void xDrawSphere(const xSphere* sph, U32 flags);
void xDrawSphere(const xVec3* center, F32 r, U32 flags);
void xDrawBox(const xBox* b);
void xDrawBox(const xBox* b, const xMat4x3* mat);
void xDrawBox(const xOBB* b);
void xDrawBox(F32 x1, F32 y1, F32 z1, F32 x2, F32 y2, F32 z2);
void xDrawSphere(F32 x, F32 y, F32 z, F32 r, U32 flags);
void xDrawSphere2(F32 x, F32 y, F32 z, F32 r, U32 lod);
void xDrawSphere2(const xSphere* sph, U32 lod);
void xDrawSphere2(const xVec3* pos, F32 r, U32 lod);
void xDrawCapsule(const xVec3* start, const xVec3* end, F32 r, U32 lod);
void xDrawOBB(const xBox* b, const xMat4x3* m);
void xDrawOBBShaded(const xBox* b, const xMat4x3* m);
void xDrawCircle(const xVec3* center, F32 radius, U32 flags);
void xDrawCircle(const xVec3* center, F32 radius, const xVec3* orient, U32 flags);
void xDrawCircle(const xMat4x3* mat, F32 radius, U32 flags);
void xDrawCircle3D(const xMat4x3* mat, F32 radius, U32 flags);
void xDrawCylinder(F32 x, F32 y, F32 z, F32 r, F32 h);
void xDrawCyl(F32 x, F32 y, F32 z, F32 r, F32 h, U32 flags);
void xDrawCyl(const xVec3* draw_at, F32 r, F32 h, U32 flags);
void xDrawCyl(const xVec3* draw_at, F32 radius, F32 height, const xVec3* orient, U32 flags);
void xDrawCyl(const xCylinder* cylinder, U32 flags);
void xDrawCylinder(const xVec3& start, const xVec3& end, F32 radius, S32 sides, S32 rings);
void xDrawCone(const xVec3* origin, F32 radius, F32 height, const xVec3* orient, U32 flags);
void xDrawCone(const xCone& cone, U32 flags);
void xDrawCone(const xVec3* origin, F32 radius, F32 height, const xMat3x3* rotmat, U32 flags);
void xDrawCone(F32 radius, F32 height, const xMat4x3* xform, U32 flags);
void xDrawCone(const xVec3& p1, F32 r1, const xVec3& p2, F32 r2, S32 sides, S32 rings);
void xDrawTri(F32 x1, F32 y1, F32 z1, F32 x2, F32 y2, F32 z2, F32 x3, F32 y3, F32 z3);
void xDrawTri(const xVec3* a, const xVec3* b, const xVec3* c);
void xDrawQuickCull(xQCData* qcd);
void xDrawConic(xConic* conic, F32 maxrange, S32 numsegments);
void xDrawFuncPieces(xFuncPiece* func, F32 initParam, F32 step, xMat4x3* mat);
void xDrawAxis(const xVec3* pos, const xVec3* dir_at, const xVec3* dir_up, F32 scale);
void xDrawAxis(const xVec3* pos, const xMat3x3* mat_rot, F32 scale);
void xDrawAxis(const xMat4x3* mat_orient, F32 scale);
void xDrawMarker(const xVec3* pos, F32 size, U32 flags);
void xDrawArc(const xMat4x3& mat, F32 radius, F32 arc_length, S32 segments);
void xDrawCard(RwRaster* raster, const xFRect& src_rect, const xFRect& dst_rect, bool zwrite);
void xDrawCard(RwRaster* raster, F32 x, F32 y, F32 w, F32 h, bool zwrite);
void xDrawCard(RwTexture* texture, F32 x, F32 y, F32 w, F32 h, bool zwrite);
void xDrawFrustumEdges(RwCamera* cam, bool);
#else
inline void xDrawSetColor(xColor color) {}
inline void xDrawSetColor(U8 r, U8 g, U8 b, U8 a) {}
inline void xDrawLine(const xVec3* a, const xVec3* b) {}
inline void xDrawSphere(const xSphere* sph, U32 flags) {}
inline void xDrawSphere(const xVec3* center, F32 r, U32 flags) {}
inline void xDrawBox(const xBox* b) {}
inline void xDrawSphere2(const xSphere* sph, U32 lod) {}
inline void xDrawSphere2(const xVec3* pos, F32 r, U32 lod) {}
inline void xDrawOBB(const xBox* b, const xMat4x3* m) {}
inline void xDrawCard(RwRaster* raster, const xFRect& src_rect, const xFRect& dst_rect, bool zwrite) {}
#endif

S32 Im2DRenderQuad(F32 x1, F32 y1, F32 x2, F32 y2, F32 z, F32 recipCamZ, F32 uvOffset);
S32 Im2DRenderQuadAlpha(F32 x1, F32 y1, F32 x2, F32 y2, F32 alpha, F32 z, F32 recipCamZ, F32 uvOffset);

#endif

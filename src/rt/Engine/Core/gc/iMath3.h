#ifndef IMATH3_H
#define IMATH3_H

#include "types.h"

struct xVec3;
struct xVec4;
struct xBox;
struct xBBox;
struct xSphere;
struct xCylinder;
struct xRay3;
struct xIsect;
struct xMat3x3;
struct xMat4x3;
struct xTri3;
class xPlane;

void iMath3Init();
void iMath3Exit(); // UNUSED
F32 iTri3GetArea(const xTri3* t); // UNUSED
void iSphereIsectVec(const xSphere* s, const xVec3* v, xIsect* isx);
void iSphereIsectRay(const xSphere* s, const xRay3* r, xIsect* isx);
void iSphereIsectTri(const xSphere* s, const xTri3* t, xIsect* isx); // UNUSED
void iSphereIsectSphere(const xSphere* s, const xSphere* p, xIsect* isx);
void iSphereInitBoundVec(xSphere* s, const xVec3* v);
void iSphereInitBoundTri(xSphere* s, const xTri3* t); // UNUSED
void iSphereInitBoundBox(xSphere* s, const xBox* b); // UNUSED
void iSphereInitBoundOBB(xSphere* s, const xBox* b, const xMat4x3* m);
void iSphereBoundVec(xSphere* o, const xSphere* s, const xVec3* v);
void iSphereBoundTri(xSphere* o, const xSphere* s, const xTri3* t); // UNUSED
void iSphereBoundSphere(xSphere* o, const xSphere* a, const xSphere* b);
void iCylinderIsectVec(const xCylinder* c, const xVec3* v, xIsect* isx);
void iCylinderIsectRay(const xCylinder* c, const xRay3* r, xIsect* isx); // UNUSED
void iCylinderIsectCylinder(const xCylinder* c, const xCylinder* p, xIsect* isx); // UNUSED
void iBoxVecDist(const xBox* box, const xVec3* v, xIsect* isx);
void iBoxIsectVec(const xBox* b, const xVec3* v, xIsect* isx);
void iBoxIsectRay(const xBox* b, const xRay3* r, xIsect* isx);
void iBoxIsectTri(const xBox* b, const xTri3* t, xIsect* isx); // UNUSED
void iBoxIsectSphere(const xBox* box, const xSphere* p, xIsect* isx);
void iBoxIsectBox(const xBox* b, const xBox* p, xIsect* isx); // UNUSED
void iBoxInitBoundVec(xBox* b, const xVec3* v);
void iBoxInitBoundTri(xBox* b, const xTri3* t); // UNUSED
void iBoxBoundVec(xBox* o, const xBox* b, const xVec3* v);
void iBoxBoundTri(xBox* o, const xBox* b, const xTri3* t); // UNUSED
void iBoxBoundSphere(xBox* o, const xBox* b, const xSphere* s); // UNUSED
void iBoxBoundCylinder(xBox* o, const xBox* b, const xCylinder* c); // UNUSED
void iBoxBoundBox(xBox* o, const xBox* a, const xBox* b); // UNUSED
void iPlaneIsectSphere(const xPlane* p, const xSphere* s, xIsect* isx); // UNUSED

#endif

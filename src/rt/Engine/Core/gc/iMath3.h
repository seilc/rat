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

void iBoxIsectRay(const xBox* b, const xRay3* r, xIsect* isx);

#endif
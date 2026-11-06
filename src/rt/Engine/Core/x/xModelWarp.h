#ifndef XMODELWARP_H
#define XMODELWARP_H

#include "xMath2.h"
#include "xMath3.h"
#include "xColor.h"

#include <rwcore.h>
#include <rpworld.h>

class xFXCameraTexture;

void xModelWarpSceneEnter();
void xModelWarpSceneExit();
void xModelWarpSetEffect(RpAtomic* atomic, const xBox* box, const xFColor* color);
void xModelWarpRemoveEffect(RpAtomic* atomic); // UNUSED
void xModelWarpSphereMapUVsByNormals(xVec2* uv, S32 verts_size, const xVec3* norm);
void xModelWarpSphereMapUVsByOffsets(xVec2* uv, S32 verts_size, const xVec3* vert, const xVec3& center);
void xModelWarpSphereMapUVs(RpAtomic* atomic, bool use_normals);
xFXCameraTexture* xModelWarpGrabScreen(const xSphere& sphere, const xFColor& color, xMat4x3* subview_mat); // UNUSED

#endif

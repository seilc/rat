#ifndef XBOUND_H
#define XBOUND_H

#include "xQuickCull.h"

#define k_XBOUNDTYPE_NONE 0
#define k_XBOUNDTYPE_SPHERE 1
#define k_XBOUNDTYPE_BOX 2
#define k_XBOUNDTYPE_CYLINDER 3
#define k_XBOUNDTYPE_OBB 4

struct xBound
{
    xQCData qcd;
    U8 type;
    U8 pad[3];
    union
    {
        xSphere sph;
        xBBox box;
        xCylinder cyl;
    };
    xMat4x3* mat;
};

void xBoundGetBox(xBox& box, const xBound& bound);

#endif

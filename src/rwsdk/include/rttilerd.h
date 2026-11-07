#ifndef RTTILERD_H
#define RTTILERD_H

#include "rwcore.h"

typedef RwCamera* (*RtTileRenderCallBack)(RwCamera* camera, RwInt32 x, RwInt32 y, void* pData);
typedef RwImage* (*RtTileArchiveCallBack)(RwImage* image, RwInt32 x, RwInt32 y, void* pData);

#ifdef __cplusplus
extern "C" {
#endif

RwCamera* RtTileRender(RwCamera* camera, RwInt32 imageWidth, RwInt32 imageHeight, RwInt32 tileWidth, RwInt32 tileHeight, RtTileRenderCallBack renderCallBack, RtTileArchiveCallBack archiveCallBack, void* pData);

#ifdef __cplusplus
}
#endif

#endif

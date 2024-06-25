#ifndef XRENDERBUFFER_H
#define XRENDERBUFFER_H

#include "iRender.h"
#include "i2dQuadBuffer.h"

extern tagiRenderArrays gRenderArr;
extern tagiRenderInput gRenderBuffer;
extern i2dQuadBuffer* gQuadBuffer;

void xRenderBufferInit();

#endif

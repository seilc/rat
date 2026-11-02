#ifndef XRENDERBUFFER_H
#define XRENDERBUFFER_H

#include "iRender.h"
#include "i2dQuadBuffer.h"

extern iRenderArrays gRenderArr;
extern iRenderInput gRenderBuffer;
extern i2dQuadBuffer* gQuadBuffer;

void xRenderBufferInit();

#endif

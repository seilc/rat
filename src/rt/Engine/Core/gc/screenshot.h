#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*SCREENSHOTAllocator)(u32 size);
typedef void (*SCREENSHOTDeallocator)(void* block);

void SCREENSHOTService(void* xfb, SCREENSHOTAllocator alloc, SCREENSHOTDeallocator dealloc);

#ifdef __cplusplus
}
#endif

#endif

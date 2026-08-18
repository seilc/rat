#include "types.h"
#include "../../../../rwsdk/include/gcn/rwcore.h"

RwCamera* RwCameraCreateWrapper();
#define RwCameraCreate RwCameraCreateWrapper

RwCamera* RwCameraBeginUpdateWrapper(RwCamera* camera);
#define RwCameraBeginUpdate RwCameraBeginUpdateWrapper

#if DEBUG || RELEASE
void RwRenderStateSetWrapper(RwRenderState state, void* value);
#define RwRenderStateSet RwRenderStateSetWrapper

RwBool RwRenderStateGetWrapper(RwRenderState state, void* value);
#define RwRenderStateGet RwRenderStateGetWrapper
#endif

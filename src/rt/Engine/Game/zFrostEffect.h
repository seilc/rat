#ifndef ZFROSTEFFECT_H
#define ZFROSTEFFECT_H

#include "xDecal.h"
#include "xMath.h"
#include "xMath3.h"

#include <rwcore.h>
#include <rpworld.h>

struct xEnt;

class zFrostEffect
{
public:
    static void Init(); // UNUSED
    static void Update(F32 dt); // UNUSED
    static void Render(); // UNUSED
    static void ApplyFrost(xRay3& ray, F32 emitRate, F32 emitDistance);

private:
    void _Init(); // UNUSED
    void InitDebugTweak(); // UNUSED
    void _Update(F32 dt); // UNUSED
    void _Render(); // UNUSED
    void _ApplyFrost(xRay3& ray, F32 emitRate, F32 emitDistance);

    xResponseCurve mDecalResponseCurve;
    xDecal::emit_context mDecalEmitContext;
    RwTexture* mFrostTexture;
};

class zFrostIcicle
{
public:
    struct config
    {
        F32 areaPerIcicle;
        F32 lifetime;
        F32 maxScale;
        F32 minScale;
        F32 growTime;
        F32 shrinkTime;
    };

    static void GetDefaultConfig(config* cfg); // UNUSED
    static void Init(); // UNUSED
    static void DecalCreateAtomicCB(RpAtomic* atomic, const xMat4x3* driver, const xEnt* driver_ent,
                                    void* context); // UNUSED
    static void DecalCreateVertListCB(const xVec3* verts, const U16* vertIndexes, S32 vertIndexSize,
                                      const xMat4x3* driver, const xEnt* driver_ent, void* context); // UNUSED
    static void ProcessTriangle(const xVec3* vert0, const xVec3* vert1, const xVec3* vert2, const xMat4x3* driver,
                                const xEnt* driver_ent, void* context); // UNUSED
    static void AddIcicle(const xVec3* pos, const xVec3* norm, const config* iceConfig, const xMat4x3* driver,
                          const xEnt* driver_ent); // UNUSED
    static void UpdateIcicles(F32 dt); // UNUSED
    static void Render(); // UNUSED
    static void InitDebugTweak(); // UNUSED
};

#endif

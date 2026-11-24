#ifndef ZNPCWALLS_H
#define ZNPCWALLS_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xVec3.h"
#include "zNPCWallsAsset.h"
#if RELEASE
#include "zSweptCircle.h"
#endif

struct xGroup;

class zNPCWalls : public xBase
{
public:
    class zWall
    {
    public:
#if RELEASE
        void Initialize(xVec2& point1, xVec2& point2);

        inline void InitializeXZ(xVec3& point1, xVec3& point2);

        bool IntersectsRay(const xVec2& iRayStart, const xVec2& iRayDelta, F32& oDistRatio, xVec2& oIPoint);
        F32 IntersectsLine(const xVec2& iLinePoint, const xVec2& iLineDelta);
        void ProjectPointInFront(xVec2& point, F32 distance); // UNUSED
        bool IntersectsSweptCircle(zSweptCircle& swCircle);
        void DebugRenderXZ(); // UNUSED

        xVec2 start;
        xVec2 end;
        xVec2 dir;
        xVec2 normal;
        F32 length;
#else
        void Initialize(xVec3* point1, xVec3* point2);
        bool Intersects(const xVec3& iPointA, const xVec3& iDirection, F32& oDistRatio, xVec3& oIPoint);
#if DEBUG
        void DebugRender(); // UNUSED
#endif

        F32 x;
        F32 z;
        F32 dx;
        F32 dz;
        F32 nx;
        F32 nz;
        F32 length;
#endif
    };

    static void InitDynamic(xBase& data, xDynAsset& asset, size_t asset_size);

    zNPCWalls(zNPCWallsAsset* asset, U32);

    void Setup();
    void Initialize(xGroup* group, bool invertWallNormals);
#if RELEASE
    bool IntersectsRayXZ(const xVec3& iRayStart, const xVec3& iRayDelta, F32& oDistRatio, xVec3& oIPoint,
                         xVec3& oNormal);
    void ProjectPointInFront(S32 wallID, xVec2& point, F32 distance); // UNUSED
    bool IntersectsSweptCircle(zSweptCircle& swCircle, S32& wallID);
    void DebugRenderXZ(); // UNUSED

    xVec2* GetWallNormal(S32 wallID)
    {
        xASSERT_L(69, wallID<numberOfWalls && wallID>=0);
        return &walls[wallID].normal;
    }
#else
    bool Intersects(const xVec3& iPointA, const xVec3& iDirection, F32& oDistRatio, xVec3& oIPoint, xVec3& oNormal);
#if DEBUG
    void DebugRender(); // UNUSED
#endif
#endif

private:
    zNPCWallsAsset* npcWallsAsset;
    zWall* walls;
    S32 numberOfWalls;
};

#if RELEASE
inline void zNPCWalls::zWall::InitializeXZ(xVec3& point1, xVec3& point2)
{
    xVec2 p1, p2;
    p1.assign(point1.x, point1.z); p2.assign(point2.x, point2.z);
    Initialize(p1, p2);
}
#endif

#endif

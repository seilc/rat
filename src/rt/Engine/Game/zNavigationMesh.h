#ifndef ZNAVIGATIONMESH_H
#define ZNAVIGATIONMESH_H

#include "xBase.h"

struct xEnt;
struct xVec3;
class xNavigationMeshAsset;
class zMeshCircle;

void zNavigationMesh_Init(void* base, void* asset);
void zNavigationMeshEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);

class zNavigationMesh : public xBase
{
    static zNavigationMesh* activeNavMesh;

    xNavigationMeshAsset* mAsset;
    zNavigationMesh* nextNavMesh;

public:
    zNavigationMesh(); // UNUSED
    void Init(xNavigationMeshAsset* asset);
    static void SceneReset();
    static void SceneEnd();
    static zMeshCircle* FindMeshCircleForEnt(const xEnt* ent);
    static zNavigationMesh* FindMeshForPosition(const xVec3& pos, S32& submeshIdx, S32& triIdx);
    void AddNavMeshToLinkedList(zNavigationMesh* navMesh);
    void FindActiveNavMesh();
    void SetEnable(bool state);
    bool IsEnabled();
    void SetNavigationMeshActive();
    static zNavigationMesh* GetActiveNavMesh();
    static xNavigationMeshAsset* GetActiveNavMeshAsset();

    xNavigationMeshAsset* GetNavMeshAsset()
    {
        return mAsset;
    }
};

#endif

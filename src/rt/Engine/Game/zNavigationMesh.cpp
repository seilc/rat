#include "xCommon.h"
#include "zNavigationMesh.h"

#include "containers.h"
#include "xCam.h"
#include "xEnt.h"
#include "xNavigationMeshAsset.h"
#include "xPtankPool.h"
#include "xstransvc.h"
#include "zNPCMgr.h"
#include "zPlayerContainer.h"

zNavigationMesh* zNavigationMesh::activeNavMesh;

void zNavigationMesh_Init(void* base, void* asset)
{
    ((zNavigationMesh*)base)->Init((xNavigationMeshAsset*)asset);
}

void zNavigationMeshEventCB(xBase*, xBase* to, U32, const F32*, xBase*, U32)
{
    xASSERT_L(27, to);

    zNavigationMesh* navMesh = (zNavigationMesh*)to;
}

void zNavigationMesh::Init(xNavigationMeshAsset* asset)
{
    xBaseInit(this, asset);
    mAsset = asset;
    AddNavMeshToLinkedList(this);
    FindActiveNavMesh();
    eventFunc = zNavigationMeshEventCB;
}

void zNavigationMesh::SceneReset()
{
    zNavigationMesh* activeNavMesh = zNavigationMesh::activeNavMesh;
    if (!activeNavMesh)
        return;

    zNavigationMesh* curNavMesh = activeNavMesh;
    if (curNavMesh) {
        do {
            xBaseInit(curNavMesh, curNavMesh->mAsset);
            curNavMesh = curNavMesh->nextNavMesh;
        } while (curNavMesh != activeNavMesh);
    }

    if (activeNavMesh)
        activeNavMesh->FindActiveNavMesh();
}

void zNavigationMesh::SceneEnd()
{
    activeNavMesh = NULL;
}

zMeshCircle* zNavigationMesh::FindMeshCircleForEnt(const xEnt* ent)
{
    zNavigationMesh* currMesh = activeNavMesh;
    zMeshCircle* circle = NULL;

    do {
        circle = currMesh->GetNavMeshAsset()->FindMeshCircleForEnt(ent);
        if (circle)
            break;
        currMesh = currMesh->nextNavMesh;
    } while (currMesh != activeNavMesh);

    xASSERT_FMT_L(95, circle, "Unable to find mesh circle for object 0x%X. Make sure NPC Collision is enabled in the editor.", ent->id);
    return circle;
}

zNavigationMesh* zNavigationMesh::FindMeshForPosition(const xVec3& pos, S32& submeshIdx, S32& triIdx)
{
    zNavigationMesh* currMesh = GetActiveNavMesh();

    do {
        if (currMesh->GetNavMeshAsset()->GetTriangle(pos, 0, 0, submeshIdx, triIdx))
            return currMesh;
        currMesh = currMesh->nextNavMesh;
    } while (currMesh != GetActiveNavMesh());

    return NULL;
}

void zNavigationMesh::AddNavMeshToLinkedList(zNavigationMesh* navMesh)
{
    if (activeNavMesh == NULL) {
        activeNavMesh = navMesh;
        navMesh->nextNavMesh = navMesh;
    } else {
        navMesh->nextNavMesh = activeNavMesh->nextNavMesh;
        activeNavMesh->nextNavMesh = navMesh;
    }
}

void zNavigationMesh::FindActiveNavMesh()
{
    zNavigationMesh* testNavMesh = activeNavMesh;

    do {
        if (testNavMesh->IsEnabled()) {
            activeNavMesh = testNavMesh;
            break;
        }
        testNavMesh = testNavMesh->nextNavMesh;
    } while (testNavMesh != activeNavMesh);
}

void zNavigationMesh::SetEnable(bool state)
{
    if (state)
        baseFlags |= k_XBASE_IS_ENABLED;
    else
        baseFlags &= (U16)~k_XBASE_IS_ENABLED;
}

bool zNavigationMesh::IsEnabled()
{
    return baseFlags & k_XBASE_IS_ENABLED;
}

void zNavigationMesh::SetNavigationMeshActive()
{
    xASSERT_FMT_L(172, activeNavMesh, "activeNavMesh is NULL. This should never happen!");

    zNavigationMesh* testNavMesh = activeNavMesh;
    do {
        if (testNavMesh->IsEnabled())
            testNavMesh->SetEnable(false);
        testNavMesh = testNavMesh->nextNavMesh;
    } while (testNavMesh != activeNavMesh);

    SetEnable(true);
    activeNavMesh = this;

    zNPCMgr_switchedNavmeshes();
}

zNavigationMesh* zNavigationMesh::GetActiveNavMesh()
{
    xASSERT_FMT_L(197, zNavigationMesh::activeNavMesh, "activeNavMesh is NULL. Most likely there are no NavMeshes in the scene!");
    xASSERT_FMT_L(198, zNavigationMesh::activeNavMesh->IsEnabled(), "activeNavMesh is not enabled. At least one navMesh needs to be enabled at the beginning of the scene");

    return activeNavMesh;
}

xNavigationMeshAsset* zNavigationMesh::GetActiveNavMeshAsset()
{
    zNavigationMesh* activeNavMesh = GetActiveNavMesh();
    if (!activeNavMesh)
        return NULL;
    return activeNavMesh->GetNavMeshAsset();
}

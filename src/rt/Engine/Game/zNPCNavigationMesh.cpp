#include "xCommon.h"
#include "zNPCNavigationMesh.h"

#include "containers.h"
#include "xColor.h"
#include "xDebugTweak.h"
#include "xDraw.h"
#include "xEntMotion.h"
#include "xGlobals.h"
#include "xHud.h"
#include "xNavigationMeshAsset.h"
#include "xPtankPool.h"
#include "xScene.h"
#include "xstransvc.h"
#include "zCam.h"
#include "zNPCCommon.h"
#include "zScene.h"

namespace zNPC {

    static bool draw_mesh;
    static bool draw_bounds;
    static bool draw_decision;
    static bool draw_direction;
    static bool draw_portals;

} // namespace zNPC

using namespace zNPC;

bool zNPCNavigationMesh::static_setup()
{
    xTWEAK_BOOL("NPC OLD|systems|navigation|Draw Mesh", &draw_mesh, NULL, NULL, 0);
    xTWEAK_BOOL("NPC OLD|systems|navigation|Draw Bounds", &draw_bounds, NULL, NULL, 0);
    xTWEAK_BOOL("NPC OLD|systems|navigation|Draw Decision Direction", &draw_decision, NULL, NULL, 0);
    xTWEAK_BOOL("NPC OLD|systems|navigation|Draw True Direction", &draw_direction, NULL, NULL, 0);
    xTWEAK_BOOL("NPC OLD|systems|navigation|Draw Portals", &draw_portals, NULL, NULL, 0);

    return true;
}

void zNPCNavigationMesh::draw_all_meshes()
{
    if (!draw_mesh) {
        return;
    }

    const xMat4x3& camMat = zCamGetMatrix();

    for (S32 count = 0; count < xSTAssetCountByType('NGMS'); count++) {
        xNavigationMeshAsset* asset = (xNavigationMeshAsset*)xSTFindAssetByType('NGMS', count, NULL);
        if (draw_bounds) {
            xDrawSetColor(g_RED);

            for (zMeshCircle* currentCircle = asset->circleList; currentCircle != NULL;
                 currentCircle = currentCircle->masterListNext) {
                xVec3 center = currentCircle->center;
                center.y += 0.1f;

                if (center.Distance2(camMat.pos) > 3600.0f) {
                    continue;
                }

                xDrawCircle(&center, currentCircle->radius, 12);
                xDrawText(currentCircle->center, "%s", xSceneID2Name(xglobals->sceneCur, currentCircle->ownerEntity->id));
            }
        }

        if (draw_mesh) {
            xDrawSetColor(g_CHARTREUSE);

            xVec3 offset = { 0.0f, 0.0f, 0.0f };

            for (S32 m = 0; m < asset->numSubMeshes; m++) {
                xNavigationMeshAssetSubMesh* sub_mesh = &asset->subMeshes[m];

                for (S32 i = 0; i < sub_mesh->numTriangles; i++) {
                    xVec3 a, b, c;
                    a.Add(sub_mesh->vertices[sub_mesh->triangles[i].a], offset);
                    b.Add(sub_mesh->vertices[sub_mesh->triangles[i].b], offset);
                    c.Add(sub_mesh->vertices[sub_mesh->triangles[i].c], offset);

                    xDrawLine(&a, &b);
                    xDrawLine(&b, &c);
                    xDrawLine(&c, &a);
                }
            }
        }
    }
}

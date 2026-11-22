#ifndef ZNPCNAVIGATIONMESH_H
#define ZNPCNAVIGATIONMESH_H

struct xVec2;
struct xVec3;

class zNPCNavigationMesh
{
public:
    static bool static_setup();
    static void draw_all_meshes();
    static void draw(const xVec3& position, const xVec3& facing, const xVec3& new_direction, const xVec2* portal); // UNUSED
};

#endif

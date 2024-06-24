#include "xTransient.h"

namespace xTransient {

xSpacePartitionTree2D space_tree;

void scene_enter(const xBox& world_box)
{
    space_tree.create(world_box.lower.x - 5.0f,
                      world_box.lower.z - 5.0f,
                      world_box.upper.x + 5.0f,
                      world_box.upper.z + 5.0f,
                      0.5f,
                      5.0f,
                      500);
}

void scene_exit()
{
}

void reset()
{
    space_tree.clear();
}

#ifdef DEBUGRELEASE
void debug_update(F32 dt)
{
    space_tree.debug_update(dt);
}

void debug_render()
{
}
#endif

}
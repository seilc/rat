#ifndef ZNPCNAVIGATION_H
#define ZNPCNAVIGATION_H

#include "zMeshCircle.h"

class zMeshPosition;

namespace zNPC {
class zNPCCommon;
}

class zNPCNavigate
{
    F32 stuck_timer;
    bool dpos_apply;
    bool reported_stuck;
    bool stuck;
    bool arrived;
    bool destination_off_mesh;
    zMeshPosition* meshPosition;
    zNPC::zNPCCommon* npc;
    zMeshCircle my_circle;
    zMeshCircle* currentThreatCircle;
    const xVec3* last_destination;
    bool move_away;

public:
    zMeshPosition& GetMeshPosition() { return *meshPosition; }
};

#endif

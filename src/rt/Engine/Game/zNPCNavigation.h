#ifndef ZNPCNAVIGATION_H
#define ZNPCNAVIGATION_H

#include "zMeshCircle.h"

class zMeshPosition;

namespace zNPC {
class zNPCCommon;
}

class zNPCJump
{
    bool active;
    F32 height;
    F32 speed;
    F32 gravity;
    F32 velocity;
    F32 time;
    F32 totalTime;
    F32 timeUp;
    xVec3 target;
    xVec3 source;
    xVec3 position;

public:
    void Init(const xVec3& inSource, const xVec3& inTarget, F32 inGravity, F32 inHeight); // UNUSED
    bool PastApex() const; // UNUSED
    void Update(F32 dt); // UNUSED
    void GetDirection(xVec3& direction) const; // UNUSED
    bool IsActive() const; // UNUSED
};

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
    zNPCNavigate(zNPC::zNPCCommon* npc);

    void reset();
    U8 PickRandomPos(const xVec3& centerPos, F32 innerRadius, F32 outerRadius, xVec3& pointOnMesh) const; // UNUSED
    U8 GetClosestPosition(const xVec3& anyPosition, xVec3& pointOnMesh); // UNUSED
    bool findChangedNavMesh();
    void collide(F32 dt);
    bool IsStuck() const;
    bool HasNavmesh() const;
    void apply_dpos(F32 dt);
    bool on_mesh(); // UNUSED
    void remove();
    U8 add(); // UNUSED
    void GetDirection(const xVec3& destination, F32, xVec3& direction);
    void update_stuck(F32 dt);
    void InvalidateMeshPosition();

    void reset_stuck()
    {
        stuck = reported_stuck = false;
        stuck_timer = 0.0f;
    }

    zMeshPosition& GetMeshPosition() { return *meshPosition; }
};

#endif

#ifndef ZNPCMOVE_H
#define ZNPCMOVE_H

#include "zNPCBehavior.h"

class zNPCMove : public zNPC::move
{
public:
    virtual const char* getName() { return "zNPCMove"; }
    virtual void setup();
    virtual bool systemEvent(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32 toParamWidgetID);
    virtual void jump_to(xVec3&, F32, bool);
    virtual void move_to(const xVec3& destination, F32 dt, bool move_away, bool face_dir, bool change_anim_speed, F32 arrived_threshold2);
    virtual void move_away_from(const xVec3& point, F32 dt);
    virtual bool physics_move(F32 dt);
    virtual void step_move(const xVec3& direction, F32 distance, F32 dt);
    virtual bool have_arrived() const;

    bool has_mesh();
    void check_movement(F32 dt); // UNUSED
    bool is_stuck(const xVec3& destination, bool move_away) const; // UNUSED
    bool is_stuck() const;
    bool stopped_on_edge() const; // UNUSED
    U8 PickRandomPos(const xVec3& centerPos, F32 minRadius, F32 maxRadius, xVec3& pointOnMesh) const; // UNUSED
    inline U8 PickRandomPos(const xVec3& center, F32 radius, xVec3& pos) const; // UNUSED

private:
    xVec3 last_destination;
    F32 cos_min_move_angle;
};

#if DEBUG || RELEASE
extern bool draw_move_to;
#endif

#endif

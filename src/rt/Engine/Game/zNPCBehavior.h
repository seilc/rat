#ifndef ZNPCBEHAVIOR_H
#define ZNPCBEHAVIOR_H

#include "zBehavior.h"
#include "zNPCCommon.h"

namespace zNPC {

class move : public zBehaviorImplementation<zNPCCommon>
{
public:
    virtual const char* getName() { return "move"; }
    virtual void setup();
    virtual void reset();
#if DEBUG || RELEASE
    virtual void add_tweaks(const char* prefix);
#endif

    bool is_facing_player(F32 min_facing_cos); // UNUSED
    F32 face_player(F32 dt, F32 turning_speed); // UNUSED
    F32 get_angle_difference(const xVec3* direction) const;
    F32 get_angle_difference(const xVec3* startDir, const xVec3* endDir) const; // UNUSED
    void turn(F32 angle_difference); // UNUSED
    F32 turn_to_face(const xVec3* direction, F32 dt, F32 turning_speed);
    F32 turn_to_face_pos(xVec3* pos, F32 dt, F32 turning_speed); // UNUSED
    F32 apply_angle_diff(F32 angle_difference, F32 dt, F32 turning_speed);
    F32 turn_to_face(F32 angle, F32 dt, F32 turning_speed); // UNUSED
    void accelerate(F32 dt, const xVec3& direction, bool change_anim_speed, F32 move_speed);
    void stop();

protected:
    F32 turn_speed;
    F32 move_speed;
    F32 min_speed;
    F32 acceleration;
    F32 speed;
};

class take_damage : public zBehaviorImplementation<zNPCCommon>
{
public:
    static void create_anim_table(xAnimTable* table); // UNUSED

    // UNUSED: needed to match (code from Incredibles GC)
    virtual const char* getName() { return "take_damage"; }
    virtual void enter_state(const zNPCCommonBehavior*); // UNUSED
    virtual void setup(); // UNUSED
    virtual void reset(); // UNUSED
    virtual bool runnable(F32 dt); // UNUSED
#if DEBUG || RELEASE
    virtual void add_tweaks(const char*); // UNUSED
#endif
    virtual bool damage(zCombatDamageInfo& damageInfo); // UNUSED
    virtual bool grabbable(GrabType); // UNUSED

private:
    bool hasBeenDamaged;
    xVec3 contact;
    xVec3 direction;
};

class idle : public zBehaviorImplementation<zNPCCommon>
{
public:
    static void create_anim_table(xAnimTable* table); // UNUSED

    // UNUSED: needed to match (code from RotU GC, Incredibles GC)
    virtual const char* getName() { return "idle"; }
    virtual void enter_state(const zNPCCommonBehavior*); // UNUSED
    inline virtual bool runnable(F32); // UNUSED
};

class npc_death : public zBehaviorImplementation<zNPCCommon>
{
public:
    void Kill(); // UNUSED
};

}

#endif

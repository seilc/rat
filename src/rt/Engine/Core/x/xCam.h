#ifndef XCAM_H
#define XCAM_H

#include "xMath3.h"
#include "xColor.h"
#include "xPad.h"

#include <rwcore.h>

struct xCamGroup;
struct xCamBlend;
class xCamTransition;
struct xScene;
class zCamSplineCommonMix;
class zCam2Player;

enum xCamCoordType
{
    XCAM_COORD_INVALID = -1,
    XCAM_COORD_CART = 0,
    XCAM_COORD_CYLINDER,
    XCAM_COORD_SPHERE,
    XCAM_COORD_MAX
};

struct xCamCoordCylinder
{
    xVec3 origin;
    F32 dist;
    F32 height;
    F32 theta;
};

struct xCamCoordSphere
{
    xVec3 origin;
    F32 dist;
    xQuat dir;
};

struct xCamCoordPolar
{
    xVec3 origin;
    F32 theta;
    F32 phi;
    F32 dist;
};

struct xCamCoord
{
    union
    {
        xVec3 cart;
        xCamCoordCylinder cylinder;
        xCamCoordSphere sphere;
    };
};

enum xCamOrientType
{
    XCAM_ORIENT_INVALID = -1,
    XCAM_ORIENT_QUAT = 0,
    XCAM_ORIENT_EULER,
    XCAM_ORIENT_MAX
};

struct xCamOrientEuler
{
    F32 yaw;
    F32 pitch;
    F32 roll;
};

struct xCamOrient
{
    union
    {
        xQuat quat;
        xCamOrientEuler euler;
    };
};

struct xCamSpatialInfo
{
    xCamCoord coord;
    xCamOrient orient;
};

struct xCamConfigCommon
{
    U8 priority;
    U8 pad1;
    U8 pad2;
    U8 pad3;
    F32 blend_time;
};

struct xCamConfigFollow
{
    struct zone_data
    {
        xVec3 offset;
        xVec3 face;
    };

    zone_data zone_rest;
    zone_data zone_above;
    zone_data zone_below;
    F32 speed_zone_offset;
    F32 speed_zone_face;
    F32 speed_move_orbit;
};

struct xCamTransitionParams : xCamConfigCommon
{
    xCamTransition* mTransitionObject;
};

struct xCam
{
public:
    xMat4x3 mat;
    xMat4x3 coll_mat;
    F32 fov;
    S32 flags;
    U32 owner;
    xCamGroup* group;
    xPad::analog_data analog;
    xCamCoordType coord_type;
    xCamOrientType orient_type;
    xCamSpatialInfo spatial;
    xCamSpatialInfo coll_spatial;
    xCamConfigCommon cfg_common;
    
    virtual xCam* get_final_dest() { return this; }
    virtual zCamSplineCommonMix* get_common_mix() { return NULL; }
    virtual void create();
    virtual void destroy() {}
    virtual void start();
    virtual void stop();
    virtual void update(xScene& scene, F32 dt) = 0;
    virtual void pre_update(xScene&) {}
    virtual void post_update(xScene&) {}
    virtual xCam* get_next() { return flags & 0x1 ? this : NULL; }
    virtual xCam* find_camera(U32 ownerID) { return owner == ownerID ? this : NULL; }
    virtual zCam2Player* get_zCam2Player() const { return NULL; }
    virtual F32 getCameraPlayerAudioBias() const { return 0.0f; }
    virtual xCamConfigFollow* config_follow();

    static void coord_to_world(xVec3& loc, const xCamCoordCylinder& c);
    static void coord_to_local(xVec3& c, const xVec3& loc);
    static void orient_to_local(xQuat& o, const xMat3x3& mat);
    static xVec3& convert(xVec3& o, const xCamCoordCylinder& i);
    static xVec3& convert(xVec3& o, const xCamCoordPolar& i);
    static void blend3(F32& o, F32 i0, F32 i1, F32 i2, F32 s);
    static void blend3(xVec3& o, const xVec3& i0, const xVec3& i1, const xVec3& i2, F32 s);
    static void blend3(xCamConfigFollow::zone_data& o, const xCamConfigFollow::zone_data& i0,
                       const xCamConfigFollow::zone_data& i1, const xCamConfigFollow::zone_data& i2, F32 s);
    static void spring_interp(F32& s, F32& v, F32 s0, F32 s1, F32 dt, F32 speed, F32 e);
    template <class T, class U>
    static void spring_interp(T& s, U& v, const T& s0, const T& s1, F32 dt, F32 speed);

private:
    S32 group_flags;
    xCamBlend* blender;

#if DEBUG || RELEASE
public:
    struct
    {
        S32 flags;
        xColor color[3];
    } debug;

    virtual void debug_render();
    virtual void debug_mode_draw() {}
    virtual void add_tweaks(const char* prefix);
#endif
};

inline void xCam::coord_to_world(xVec3& loc, const xCamCoordCylinder& c)
{
    convert(loc, c);
}

inline void xCam::coord_to_local(xVec3& c, const xVec3& loc)
{
    c = loc;
}

inline void xCam::orient_to_local(xQuat& o, const xMat3x3& mat)
{
    xQuatFromMat(&o, &mat);
}

inline xVec3& xCam::convert(xVec3& o, const xCamCoordCylinder& i)
{
    o.x = i.origin.x + i.dist * isin(i.theta);
    o.y = i.origin.y + i.height;
    o.z = i.origin.z + i.dist * icos(i.theta);
    return o;
}

inline xVec3& xCam::convert(xVec3& o, const xCamCoordPolar& i)
{
    F32 cp = i.dist * icos(i.phi);
    o.x = i.origin.x + cp * isin(i.theta);
    o.y = i.origin.y + i.dist * isin(i.phi);
    o.z = i.origin.z + cp * icos(i.theta);
    return o;
}

inline void xCam::blend3(F32& o, F32 i0, F32 i1, F32 i2, F32 s)
{
    o = i1 + s * (s >= 0.0f ? i2 - i1 : i1 - i0);
}

inline void xCam::blend3(xVec3& o, const xVec3& i0, const xVec3& i1, const xVec3& i2, F32 s)
{
    blend3(o.x, i0.x, i1.x, i2.x, s);
    blend3(o.y, i0.y, i1.y, i2.y, s);
    blend3(o.z, i0.z, i1.z, i2.z, s);
}

inline void xCam::blend3(xCamConfigFollow::zone_data& o, const xCamConfigFollow::zone_data& i0,
                         const xCamConfigFollow::zone_data& i1, const xCamConfigFollow::zone_data& i2, F32 s)
{
    blend3(o.offset, i0.offset, i1.offset, i2.offset, s);
    blend3(o.face, i0.face, i1.face, i2.face, s);
}

inline void xCam::spring_interp(F32& s, F32& v, F32 s0, F32 s1, F32 dt, F32 speed, F32 e)
{
    F32 ds = s0 - s1;
    xDampSpring(ds, v, dt, speed, e);
    s = s1 + ds;
}

template <class T, class U>
void xCam::spring_interp(T& s, U& v, const T& s0, const T& s1, F32 dt, F32 speed)
{
    F32 e = xDampSpringCoeff(dt, speed);
    spring_interp(s, v, s0, s1, dt, speed, e);
}

struct xCamGroup
{
public:
    void transition_to(xCam& cam, const xCamTransitionParams* params, bool force_cut, bool force_restart);

    xMat4x3 mat;
    xMat4x3 coll_mat;
    xVec3 coll_atXZ;
    F32 cameraPlayerAudioBias;
    xVec3 vel;
    F32 fov;
    F32 fov_default;
    S32 flags;
    xCam* primary;
    xPad::analog_data analog;

private:
    S32 child_flags;
    S32 child_flags_mask;
    xCamBlend* blend_cam[4];
};

struct xCamScreen
{
    RwCamera* icam;
    F32 fov;
};

class xCamBias
{
public:
    xCamBias();

    virtual void Start() = 0;
    virtual void Update(F32 dt) = 0;
    virtual F32 GetBias() = 0;
    virtual void Reverse() = 0;
    virtual bool IsActive() = 0;
    virtual bool DidComplete() = 0;

protected:
    bool mIsBiDirectional;
};

class xCamTimeBias : public xCamBias
{
public:
    virtual void Start();
    virtual void Update(F32 dt);
    virtual F32 GetBias();
    virtual void Reverse();
    virtual bool IsActive();
    virtual bool DidComplete();

private:
    F32 bias;
    F32 time;
    F32 blendTime;
};

struct xCamBlend : xCam
{
    xCam* src;
    xCam* dst;
    xCamBias* bias;
    xCamTimeBias timeBias;
    xCamTransition* transition;
};

#endif

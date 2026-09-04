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
    
    virtual xCam* get_final_dest();
    virtual zCamSplineCommonMix* get_common_mix();
    virtual void create();
    virtual void destroy();
    virtual void start();
    virtual void stop();
    virtual void update(xScene& scene, F32 dt) = 0;
    virtual void pre_update(xScene& scene);
    virtual void post_update(xScene& scene);
    virtual xCam* get_next();
    virtual xCam* find_camera(U32 ownerID);
    virtual zCam2Player* get_zCam2Player() const;
    virtual F32 getCameraPlayerAudioBias() const;
    virtual xCamConfigFollow* config_follow();

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
    virtual void debug_mode_draw();
    virtual void add_tweaks(const char* prefix);
#endif
};

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

#ifndef XCAM_H
#define XCAM_H

#include "xMath3.h"
#include "xColor.h"
#include "xPad.h"

struct xCamBlend;
class xCamTransition;

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
    void* __vptr;

private:
    S32 group_flags;
    xCamBlend* blender;

public:
    struct
    {
        S32 flags;
        xColor color[3];
    } debug;
};

struct xCamGroup
{
public:
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

class xCamBias
{
public:
    void* __vptr;

protected:
    bool mIsBiDirectional;
};

class xCamTimeBias : public xCamBias
{
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

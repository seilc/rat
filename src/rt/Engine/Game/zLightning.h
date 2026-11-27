#ifndef ZLIGHTNING_H
#define ZLIGHTNING_H

#include "iSnd.h"
#include "xBase.h"
#include "xColor.h"
#include "xVec3.h"
#include "zLightningAsset.h"

#include <rwcore.h>

struct xEnt;
struct zLightning;

class zLightningWidget : private xBase
{
public:
    zLightning* l;

private:
    zLightningAsset* asset;
};

typedef enum _tagLightningType
{
    eLT_Line,
    eLT_Rotating,
    eLT_Zeus,
    eLT_Func
} LightningType;

struct zLightning
{
    U32 type;
    U32 flags;
    struct
    {
        xVec3 endPoint[2];
        xVec3 endPointB;
        xVec3 direction;
        F32 length;
        F32 scale;
        F32 width;
        F32 endParam[2];
        F32 endVel[2];
        F32 paramSpan[2];
    } func;
    zLightningWidget* widget;
    U32 numStrips;
    F32 alphaRamp;
    xColor color;
    F32 time_left;
    F32 time_total;
    zLightning* nextBranch;
    zLightning* prevBranch;
    F32 parentSegment;
    F32 lastParentSegment;
    F32 segmentsPerMeter;
    F32 branchSpeed;
    S32 damage;
    F32 knockBackSpeed;
    RwRaster* mainTexture;
    RwRaster* branchTexture;
    F32 sparkTimer;
    xVec3 collisionPoint;
    F32 genTime;
    bool randomizeBranchEndPoint;
    bool collisionEnabled;
    xSndHandle sndHandle;
    xEnt* followStart;
    xVec3 followStartOffset;
    xEnt* followEnd;
    xVec3 followEndOffset;
    void (*renderCB)(zLightning*, RxObjSpace3DVertex*, RxObjSpace3DVertex*, U32);
    void* context;
    F32 weightParam[12];
};

typedef struct _tagLightningAdd
{
    U32 type;
    F32 setup_degrees;
    F32 move_degrees;
    F32 rot_radius;
    xVec3* start;
    xVec3* end;
    S16 total_points;
    S16 end_points;
    F32 time;
    F32 arc_height;
    F32 thickness;
    F32 segmentsPerMeter;
    xColor color;
    F32 rand_radius;
    U32 flags;
    F32 zeus_normal_offset;
    F32 zeus_back_offset;
    F32 zeus_side_offset;
    F32 branchSpeed;
    U32 mainTexture;
    U32 branchTexture;
    S32 damage;
    F32 knockBackSpeed;
    xEnt* followStart;
    xEnt* followEnd;
    xSndHandle sndHandle;
    bool collisionEnabled;
    bool isMain;
} LightningAdd;

zLightning* zLightningAdd(LightningAdd* add);
void zLightningKill(zLightning* l);
void zLightningModifyEndpoints(zLightning* l, const xVec3* start, const xVec3* end);

#endif

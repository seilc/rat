#ifndef ZZIPLINE_H
#define ZZIPLINE_H

#include "xBase.h"
#include "xCam.h"
#include "zZipLineAsset.h"

struct xScene;
struct zScene;

struct zZipLine : xBase
{
    zZipLineAsset* asset;
    bool enabled;
    bool gotDismount;
};

struct zZipLineCam : xCam
{
    struct config : xCamConfigFollow
    {
        F32 speed_move;
        F32 speed_turn;
        zone_data zone_side;
        F32 side_yaw;
    };

    config cfg;

    void load_cfg(); // UNUSED
    virtual void create(); // UNUSED
    virtual void start(); // UNUSED
    virtual void update(xScene&, F32 dt); // UNUSED
    inline virtual xCamConfigFollow* config_follow(); // UNUSED

#if DEBUG || RELEASE
    virtual void debug_render(); // UNUSED
    virtual void add_tweaks(const char* prefix); // UNUSED
#endif

private:
    void refresh_rest(); // UNUSED
    void refresh_end_coord(); // UNUSED
    void apply_spring(F32 dt); // UNUSED
    void face_target(); // UNUSED

    xMat4x3 basis;
    xCamCoordSphere end_coord;
    xVec3 end_offset;
    F32 vel_dist;
    F32 vel_turn;
    xCamConfigFollow::zone_data zone;
    xCamConfigFollow::zone_data end_zone;
    xCamConfigFollow::zone_data vel_zone;
};

void zZipLineInit(void* plat, void* asset);
void zZipLineInit(zZipLine* zipline, zZipLineAsset* asset);
void zZipLineSetup(zZipLine* zipline, zScene*);
void zZipLineEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif

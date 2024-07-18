#ifndef XFXCAMERATEXTURE_H
#define XFXCAMERATEXTURE_H

#include "xColor.h"

#include <rwcore.h>
#include <rpworld.h>

class xFXCameraTexture
{
public:
    RwCamera* cam;
    RwRaster* raster;
    RwRaster* bgraster;
    RpWorld* world;
    RwTexture* texture;
    size_t vert_buffer_used;
    xColor bgcolor;

    bool create(S32 w, S32 h, bool need_zwrite, S32 bpp, S32 zbpp);
    void destroy();

    void set_background(xColor color);

private:
    RwCamera* oldcam;
    RpWorld* oldworld;
    F32 rcz;
    F32 w;
    F32 h;
    RwRaster* zraster;
    S32 max_tw;
    S32 max_th;
    F32 old_near_plane;
    F32 old_far_plane;
};

#endif

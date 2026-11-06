#ifndef XCAMSUPPORT_H
#define XCAMSUPPORT_H

#include "xVec3.h"

struct xCamCoordPolar;

class xCamSupportPath
{
public:
    struct config
    {
        F32 min_segment_dist;
        F32 smooth_factor;
        S32 smooth_passes;
        F32 subdiv_thresh;
    };

    config cfg;

#if DEBUG || RELEASE
    enum
    {
        DEBUG_FLAG_RENDER = 0x1
    };

    S32 debug_flags;
#endif

    void create(S32 max_size, S32 debug_max_size);
    void reset(const xVec3& start_loc);
    void refresh_config();
    S32 polar_subdivide(const xCamCoordPolar& start, const xCamCoordPolar& end);
    void prune(S32 start, S32 size);
    void smooth(S32 start, S32 size);
    F32 traverse(xVec3& rloc, F32 velmag, F32 dt);
    void refresh_dist();
#if DEBUG || RELEASE
    void add_tweaks(const char* prefix, const char* debug_prefix);
    void debug_render();
#endif

    F32 total_distance()
    {
        if (dist < 0.0f) {
            refresh_dist();
        }

        F32 rdist = dist - subdist;
        return rdist >= 0.0f ? rdist : 0.0f;
    }

    S32 size() const
    {
        return used;
    }

private:
#if DEBUG || RELEASE
    struct debug_context;

    debug_context* debug;
#endif

    struct path_node
    {
        xVec3 loc;
    };

    path_node* path;
    S32 power;
    S32 max_size;
    S32 front;
    S32 used;
    F32 dist;
    F32 subdist;
    F32 weights[31];

    S32 mod_index(S32 index) const
    {
        return index & (max_size - 1);
    }

    static S32 polar_subdivide(path_node* vert, S32 vert_size, const xCamCoordPolar& start, const xCamCoordPolar& end, F32 thresh);
    static void compute_weights(F32* w, S32 n, F32 s);
#if DEBUG || RELEASE
    void debug_init(S32 debug_max_size);
    void debug_refresh_config();
#else
    void debug_init(S32 debug_max_size) {}
    void debug_refresh_config() {}
#endif
};

class xCamSupportLOSBar
{
public:
    struct config
    {
        F32 rail_ymin;
        F32 rail_ymax;
        F32 pivot_bar_radius;
        F32 compress_bar_radius;
        F32 compress_ball_radius;
        F32 compress_dist_min;
    };

    static config cfg;

    xVec3 origin;
    xVec3 last_origin;
    xVec3 stern_loc;
    xVec3 last_stern_loc;
    F32 rail_yoffset;
    F32 last_rail_yoffset;

    void reset(const xVec3& origin, const xVec3& stern_loc);
    void begin_update(const xVec3& origin);
    void precollide();
    bool collide_pivot();
    bool collide_compress();
    bool collide_llod(); // UNUSED
#if DEBUG || RELEASE
    void add_tweaks(const char* prefix, const char* debug_prefix);
    void debug_render();
#endif
};

#endif

#ifndef XCAMSUPPORT_H
#define XCAMSUPPORT_H

#include "xVec3.h"

struct xCamSupportPath
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

#ifdef DEBUGRELEASE
    S32 debug_flags;
#endif

    void refresh_dist();

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
#ifdef DEBUGRELEASE
    struct debug_context
    {
        char weight_text[256];
        S32 max_nodes_used;
    };

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
};

#endif

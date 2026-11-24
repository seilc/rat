#ifndef ZRAFTTRACK_H
#define ZRAFTTRACK_H

#include "xBase.h"
#include "xDynAsset.h"

struct xEnt;

#define MAX_RAFTTRACKS 10

struct zRaftTrack : xBase
{
    struct asset_type : xDynAsset
    {
        enum { VERSION = 1 };

        static const char* type_name() { return "RaftTrack"; }

        U32 track;
        U32 fastTrack;
        F32 minSpeed;
        F32 normSpeed;
        F32 maxSpeed;
    };

    const asset_type* asset;
    S32 trackCount;
    xEnt* trackEnt[MAX_RAFTTRACKS];
    S32 fastTrackCount;
    xEnt* fastTrackEnt[MAX_RAFTTRACKS];

    void load(const asset_type& a);
    static void reset(); // UNUSED

    static void load(xBase& data, xDynAsset& asset, size_t asset_size);
    static void sceneEnter(); // UNUSED
    static void sceneExit(); // UNUSED
    static void setupAll(); // UNUSED
    void setup(); // UNUSED
#if DEBUG || RELEASE
    void debugInit();
    void addTweaks();
#else
    void debugInit() {}
#endif
};

#endif

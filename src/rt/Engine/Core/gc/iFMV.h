#ifndef IFMV_H
#define IFMV_H

#include <bink.h>
#include "types.h"
#include <rwcore.h>
#include <dolphin.h>

struct MOVIE
{
    U32 aid;
    HBINK hBink;
    DVDFileInfo file;
    RwTexture* pBinkTexture;
    U32 uMovieOffsetBytes;
    U32 movie_is_suspended : 1;
    U32 other_movie_flags : 7;
    U32 disk_open_frame : 24;
};

#endif

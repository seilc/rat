#ifndef XPKRSVC_H
#define XPKRSVC_H

#include "types.h"

struct PKRAssetType
{
    U32 typetag;
    U32 tflags;
    S32 typalign;
    void*(*readXForm)(void*, U32, void*, U32, U32*);
    void*(*writeXForm)(void*, U32, void*, void*, U32, U32*);
    S32(*assetLoaded)(void*, U32, void*, S32);
    void*(*makeData)(void*, U32, void*, S32*, S32*);
    void(*cleanup)(void*, U32, void*);
    void(*assetUnloaded)(void*, U32);
    void(*writePeek)(void*, U32, void*, char*);
};

struct PKRAssetTOCInfo
{
    U32 aid;
    PKRAssetType* typeref;
    U32 sector;
    U32 plus_offset;
    U32 size;
    void* mempos;
#if DEBUG || RELEASE
    const char* assetname;
    const char* filename;
#endif
};

struct st_HIPLOADDATA;
struct st_PACKER_READ_DATA;

struct st_PACKER_ATOC_NODE
{
    U32 aid;
    U32 asstype;
    S32 d_off;
    S32 d_size;
    S32 d_pad;
    U32 d_chksum;
    S32 assalign;
    S32 infoflag;
    S32 loadflag;
    char* memloc;
    S32 x_size;
    S32 readcnt;
    S32 readrem;
    PKRAssetType* typeref;
    st_HIPLOADDATA* ownpkg;
    st_PACKER_READ_DATA* ownpr;
    char name[32];
    char fullname[128];
    S64 dbg_loadtime[4];
};

void* PKR_getmem(U32 id, S32 amount, U32 aid, S32 align, U32 memtag);
void PKR_relmem(U32 id, S32 blksize, void* memptr, U32 aid, S32 isTemp);

#endif

#ifndef IFILE_H
#define IFILE_H

#include "types.h"

#include <dolphin.h>

typedef struct tag_xFile xFile;

typedef struct tag_iFile
{
    U32 flags;
    char path[128];
    S32 fd;
    DVDFileInfo file;
    void(*cb)(tag_xFile*);
    S32 akey;
    S32 buffer[8];
    S32 real_pos;
} iFile;

U32* iFileLoad(const char* name, U32* buffer, U32* size, S32 flags);
U32 iFileOpen(const char* name, S32 flags, xFile* file);
U32 iFileRead(xFile* file, void* buf, U32 size);
U32 iFileClose(xFile* file);
U32 iFileGetSize(xFile* file);

#endif

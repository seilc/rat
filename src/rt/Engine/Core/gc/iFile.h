#ifndef IFILE_H
#define IFILE_H

#include "types.h"

#include <dolphin.h>

#define IFILE_NAMELEN_MAX 128

// iFile::flags
#define IFILE_OPENED 1
#define IFILE_READ (1<<0)
#define IFILE_WRITE (1<<1)
#define IFILE_FULLPATH (1<<4)
#define IFILE_HOST_IO (1<<7)
#define IFILE_TEXT (1<<8)
#define IFILE_APPEND (1<<9)

typedef struct tag_xFile xFile;
typedef void (*iFileReadAsyncCallback)(xFile*);

enum xFileDriveType
{
    XFILE_DRIVE_CDVD,
    XFILE_DRIVE_CONSOLE_HD
};

enum xFileDriveStatus
{
    XFILE_TRAY_OPEN,
    XFILE_STOPPED,
    XFILE_SPINNING,
    XFILE_READING,
    XFILE_PAUSED,
    XFILE_SEEKING,
    XFILE_ERROR
};

enum IFILE_READSECTOR_STATUS
{
    IFILE_RDSTAT_NOOP,
    IFILE_RDSTAT_INPROG,
    IFILE_RDSTAT_DONE,
    IFILE_RDSTAT_FAIL,
    IFILE_RDSTAT_QUEUED,
    IFILE_RDSTAT_EXPIRED
};

typedef struct tag_iFile
{
    U32 flags;
    char path[IFILE_NAMELEN_MAX];
    S32 fd;
    DVDFileInfo file;
    iFileReadAsyncCallback cb;
    S32 akey;
    S32 buffer[8];
    S32 real_pos;
} iFile;

void iFileInit();
void iFileExit();
U32* iFileLoad(const char* name, U32* buffer, U32* size, S32 flags);
void iFileFree(U32* buffer);
U32 iFileOpen(const char* name, S32 flags, xFile* file);
S32 iFileSeek(xFile* file, S32 offset, S32 whence);
U32 iFileRead(xFile* file, void* buf, U32 size);
void iFileAsyncService();
S32 iFileReadAsync(xFile* file, void* buf, U32 asize, iFileReadAsyncCallback dcb, S32);
IFILE_READSECTOR_STATUS iFileReadAsyncStatus(S32 key, S32* bytes_read);
U32 iFileWrite(xFile* file, void* buf, U32 size);
U32 iFileClose(xFile* file);
U32 iFileGetSize(xFile* file);
U32 iFileGetSize(const char* name); // UNUSED
bool iFileExists(const char* name, S32);
U32 iFileReadStop();
void iFileFullPath(const char* relname, char* fullname);
void iFileSetPath(const char* path);
#if DEBUG || RELEASE
void iFileFixAbsolutePath(char* path);
#endif
U32 iFileFind(const char* name, S32 fileflags, xFile* file);
void iFileGetInfo(xFile* file, U32* starting_sector, U32* size_in_bytes);
xFileDriveStatus iFileGetStatus(xFileDriveType); // UNUSED
U32 iFileGetSectorSize(xFileDriveType drive);

#endif

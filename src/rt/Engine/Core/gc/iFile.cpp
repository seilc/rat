#include "xCommon.h"
#include "iFile.h"

#include "xFile.h"
#include "xPad.h"
#include "xString.h"
#include "xTRC.h"
#include "xutil.h"
#include "iDebug.h"
#include "iTRC.h"
#include "zMain.h"

#include <rwcore.h>
#include <stdio.h>
#include <string.h>

#define MAX_FILE_QUEUE 4

struct file_queue_record
{
    xFile* finfo;
    void* buf;
    U32 read_size;
    U32 cur_read;
    IFILE_READSECTOR_STATUS status;
    iFileReadAsyncCallback done;
    u32 key;
};

static S32 tbuffer[1032];
static S32* buffer32;

void iFileInit()
{
    S32 ba = (S32)tbuffer;
    ba += 31;
    ba >>= 5;
    ba <<= 5;
    buffer32 = (S32*)ba;
}

void iFileExit()
{
}

U32* iFileLoad(const char* name, U32* buffer, U32* size, S32 flags)
{
    char fullpath[IFILE_NAMELEN_MAX];
    xFile file;

    iFileFullPath(name, fullpath);

    // BUG: opens name, not fullpath
    U32 failure = iFileOpen(name, flags | IFILE_FULLPATH, &file);
    if (failure)
        return NULL;
    U32 fsize = iFileGetSize(&file);

    if (*size) {
    }

    fsize += 31;
    fsize &= ~31;
    if (!buffer) {
        buffer = (U32*)RwMallocL(fsize, rwMEMHINTDUR_NADURATION, 144);
    }
    xASSERT_L(147, buffer != NULL);
    iFileRead(&file, buffer, fsize);
    if (size) *size = fsize;
    iFileClose(&file);
    return buffer;
}

void iFileFree(U32* buffer)
{
    RwFree(buffer);
}

U32 iFileOpen(const char* name, S32 flags, xFile* file)
{
    xASSERT_L(248, name != NULL);
    xASSERT_L(249, file != NULL);

    iFile* ps = &file->ps;

    xASSERT_L(253, ps != NULL);
    xASSERT_L(254, ps->path != NULL);

    if (flags & IFILE_FULLPATH) {
        strcpy(ps->path, name);
        flags &= ~IFILE_FULLPATH;
    } else {
        iFileFullPath(name, ps->path);
    }

#if !MASTER
    if (flags & IFILE_HOST_IO) {
        char hostIOName[256];
        char openFlags[16];

        sprintf(hostIOName, "host://%s", ps->path);

        if (flags & IFILE_WRITE) {
            if (flags & IFILE_TEXT) {
                strcpy(openFlags, "w");
            } else {
                strcpy(openFlags, "wb");
            }
        }
        else if (flags & IFILE_APPEND) {
            if (flags & IFILE_TEXT) {
                strcpy(openFlags, "a+");
            } else {
                strcpy(openFlags, "ab+");
            }
        }
        else if (flags & IFILE_READ) {
            strcpy(openFlags, "rb");
        } else {
            xFAIL_FMT_L(303, "iFileOpen: Unkown flags 0x%X", flags);
            return 1;
        }

        // BUG: opens ps->path, not hostIOName
        ps->fd = (S32)fopen(ps->path, openFlags);

        if (!ps->fd)
            return 1;
        ps->flags = (flags | IFILE_OPENED) & 0xFFFF;
        iFileSeek(file, 0, SEEK_SET);
        return 0;
    }
#endif

    ps->fd = DVDConvertPathToEntrynum(ps->path);
    if (ps->fd == -1)
        return 1;
    if (!DVDFastOpen(ps->fd, &ps->file)) {
        ps->fd = -1;
        return 1;
    }

    ps->cb = NULL;
    ps->flags = IFILE_OPENED;
    iFileSeek(file, 0, SEEK_SET);
    return 0;
}

S32 iFileSeek(xFile* file, S32 offset, S32 whence)
{
    iFile* ps = &file->ps;
    S32 real_offset;

    xASSERT_L(356, file != NULL);
    xASSERT_L(360, ps->flags & IFILE_OPENED);

    switch (whence) {
    case SEEK_SET:
        real_offset = offset;
        break;
    case SEEK_CUR:
        if (DVDGetCommandBlockStatus(&ps->file.cb) == DVD_STATE_BUSY) {
            return -1;
        }
        real_offset = offset + ps->real_pos;
        break;
    case SEEK_END:
#if !MASTER
        real_offset = iFileGetSize(file) - offset;
#else
        real_offset = ps->file.length - offset;
#endif
        if (real_offset < 0) real_offset = 0;
        break;
    default:
        xFAIL_MSG_L(386, "bad seek mode");
        real_offset = offset;
        break;
    }

    ps->real_pos = real_offset;
    return ps->real_pos;
}

U32 iFileSyncAsyncReadActive;

static void ifilereadCB(xFile*)
{
    iFileSyncAsyncReadActive = FALSE;
}

U32 iFileRead(xFile* file, void* buf, U32 size)
{
    iFile* ps = &file->ps;

    xASSERT_L(407, file != NULL);
    xASSERT_L(408, buf != NULL);
    xASSERT_L(409, ps->flags & IFILE_OPENED);

#if !MASTER
    if (ps->flags & IFILE_HOST_IO) {
        U32 numItemsRead = fread(buf, 1, size, (FILE*)ps->fd);
        // BUG: fread already returns bytes
        file->ps.real_pos += numItemsRead * size;
        return numItemsRead;
    }
#endif

    xASSERT_L(420, !(size&0x3));
    iFileSeek(file, file->ps.real_pos, SEEK_SET);
    iFileSyncAsyncReadActive = TRUE;

    for (;;) {
        S32 result = iFileReadAsync(file, buf, size, ifilereadCB, 0);
        if (result != -1) {
            break;
        }

        xPadUpdateAllPads(SECS_PER_VBLANK);
        iTRC_CHECK_L(433);
        iFileAsyncService();
    }

    while (iFileSyncAsyncReadActive) {
        xPadUpdateAllPads(SECS_PER_VBLANK);
        iTRC_CHECK_L(441);
        iFileAsyncService();
    }
    file->ps.real_pos += size;
    return size;
}

file_queue_record file_queue[MAX_FILE_QUEUE];

static void async_cb(s32 result, DVDFileInfo* finfo)
{
    S32 i = (S32)((DVDCommandBlock*)finfo)->userData;
    file_queue_record* fq = &file_queue[i];
    char* p;
    s32 next_read;
    s32 bytes = -1;

    switch (result) {
    case DVD_RESULT_CANCELED:
        DBprintf(DBML_WARN, "iFile: AsyncRead CANCEL Event\n");
        break;
    case DVD_RESULT_FATAL_ERROR:
        xTRCDisk(TRC_DiskFatal);
        DBprintf(DBML_WARN, "iFile: AsyncRead FATAL ERROR Event\n");
        return;
    default:
        if (result >= 0) bytes = result;
        else
            DBprintf(DBML_WARN, "iFile: AsyncRead - Unexpected result %d", result);
        break;
    }

    if (bytes < 0) {
        fq->status = IFILE_RDSTAT_FAIL;
        return;
    }

    if (bytes + fq->cur_read >= fq->read_size ||
        bytes + fq->cur_read + fq->finfo->ps.real_pos >= fq->finfo->ps.file.length) {
        fq->status = IFILE_RDSTAT_DONE;
        fq->cur_read = fq->read_size;
        return;
    }

    fq->cur_read += bytes;
    fq->status = IFILE_RDSTAT_INPROG;
    if (fq->read_size - fq->cur_read < 0x8000) {
        next_read = fq->read_size - fq->cur_read;
        next_read += 3;
        next_read &= ~3;
    } else {
        next_read = 0x8000;
    }

    if (next_read + fq->cur_read + fq->finfo->ps.real_pos > fq->finfo->ps.file.length) {
        next_read = fq->finfo->ps.file.length;
        next_read -= fq->finfo->ps.real_pos;
        next_read -= fq->cur_read;
        xASSERT_L(630, next_read > 0);
        next_read += 31;
        next_read &= ~31;
    }

    p = (char*)fq->buf;
    p += fq->cur_read;
    DVDReadAsync(&fq->finfo->ps.file, p, next_read, fq->finfo->ps.real_pos + fq->cur_read, async_cb);
}

void iFileAsyncService()
{
    for (S32 i = 0; i < MAX_FILE_QUEUE; i++) {
        file_queue_record& fq = file_queue[i];
        if (fq.done && (fq.status == IFILE_RDSTAT_DONE || fq.status == IFILE_RDSTAT_FAIL)) {
            fq.done(fq.finfo);
            fq.done = NULL;
            fq.finfo->ps.akey = -1;
        }
    }
}

S32 iFileReadAsync(xFile* file, void* buf, U32 asize, iFileReadAsyncCallback dcb, S32)
{
    static u32 fopcount = 1;

    for (S32 i = 0; i < MAX_FILE_QUEUE; i++) {
        file_queue_record& fq = file_queue[i];
        if (!fq.done && fq.status != IFILE_RDSTAT_QUEUED && fq.status != IFILE_RDSTAT_INPROG) {
            s32 read_size = 32;
            u32 key = fopcount++;
            key <<= 2;

            fq.finfo = file;
            fq.buf = buf;
            fq.read_size = asize;
            fq.cur_read = 0;
            fq.status = IFILE_RDSTAT_QUEUED;
            fq.done = dcb;
            fq.key = key + i;
            if (read_size > asize) {
                read_size = asize;
                read_size += 3;
                read_size &= ~3;
            }

            file->ps.file.cb.userData = (void*)i;
            DVDReadAsync(&file->ps.file, buf, read_size, file->ps.real_pos, async_cb);
            file->ps.akey = key + i;
            return (S32)key + i;
        }
    }
    return -1;
}

IFILE_READSECTOR_STATUS iFileReadAsyncStatus(S32 key, S32* bytes_read)
{
    u32 i = (u32)key & 3;

    if (key != file_queue[i].key) {
        return IFILE_RDSTAT_EXPIRED;
    }
    if (bytes_read) {
        *bytes_read = file_queue[i].cur_read;
    }
    return file_queue[i].status;
}

#if !MASTER
U32 iFileWrite(xFile* file, void* buf, U32 size)
{
    xASSERT_L(737, file != NULL);
    iFile* ps = &file->ps;
    xASSERT_L(739, (ps->flags & IFILE_HOST_IO) && (ps->flags & (IFILE_WRITE|IFILE_APPEND) ));

    U32 n = fwrite(buf, 1, size, (FILE*)ps->fd);
    fflush((FILE*)ps->fd);
    return n;
}
#endif

U32 iFileClose(xFile* file)
{
    xASSERT_L(762, file != NULL);
    iFile* ps = &file->ps;

#if !MASTER
    if (ps->flags & IFILE_HOST_IO) {
        fclose((FILE*)ps->fd);
        ps->flags = 0;
        return 0;
    }
#endif

    S32 ret = DVDClose(&ps->file);
    if (!ret) {
        return 1;
    }

    ps->flags = 0;
    return 0;
}

U32 iFileGetSize(xFile* file)
{
    xASSERT_L(802, file != NULL);
    iFile* ps = &file->ps;

#if !MASTER
    if (ps->flags & IFILE_HOST_IO) {
        S32 cur = ftell((FILE*)ps->fd);
        fseek((FILE*)ps->fd, 0, SEEK_END);
        s32 end = ftell((FILE*)ps->fd);
        fseek((FILE*)ps->fd, cur, SEEK_SET);
        return end;
    }
#endif

    return ps->file.length;
}

bool iFileExists(const char* name, S32)
{
    char fullname[IFILE_NAMELEN_MAX];

    xASSERT_L(875, name != NULL);

    iFileFullPath(name, fullname);

    S32 fd = DVDConvertPathToEntrynum(fullname);
    return fd != -1;
}

U32 iFileReadStop()
{
    return DVDCancelAllAsync(NULL);
}

char gHostPath[256];

void iFileFullPath(const char* relname, char* fullname)
{
    xASSERT_L(922, relname != NULL);
    xASSERT_L(923, fullname != NULL);
    xASSERT_L(924, strlen(relname) < IFILE_NAMELEN_MAX - 20);

    const char* inchar = relname;
    char* outchar = fullname;
    while (*inchar) {
        if (*inchar == '\\')
            *outchar = '/';
        else
            *outchar = *inchar;
        inchar++;
        outchar++;
    }
    *outchar = '\0';
}

void iFileSetPath(const char* path)
{
    xASSERT_L(945, path);
    U32 len = strlen(path);
    xASSERT_L(947, len+1 < sizeof(gHostPath));

    iprintf("iFileSetPath:  \"%s\"\n", path);
    strcpy(gHostPath, path);
    if (len && gHostPath[len - 1] != '/' && gHostPath[len - 1] != '\\') {
        gHostPath[len] = '/';
        gHostPath[len + 1] = '\0';
    }
}

#if DEBUG || RELEASE
void iFileFixAbsolutePath(char* path)
{
    char tmppath[256];
    static const struct
    {
        char* bad;
        char* good;
    } fixes[] = {
        { "C:/IN2", ".." },
        { "C:\\IN2", ".." },
    };

    for (S32 i = 0; i < ARRAY_LENGTH(fixes); i++) {
        if (xMemicmp(path, fixes[i].bad, strlen(fixes[i].bad)) == 0) {
            strcpy(tmppath, path);
            strcpy(path, fixes[i].good);
            strcat(path, tmppath + strlen(fixes[i].bad));
        }
    }
}
#endif

U32 iFileFind(const char* name, S32 fileflags, xFile* file)
{
    return iFileOpen(name, fileflags, file);
}

void iFileGetInfo(xFile* file, U32* starting_sector, U32* size_in_bytes)
{
    xASSERT_L(1011, file);
    iFile* ps = &file->ps;
    xASSERT_L(1015, ps->flags & IFILE_OPENED);

    if (starting_sector) {
        *starting_sector = ps->file.startAddr;
    }
    if (size_in_bytes) {
        *size_in_bytes = ps->file.length;
    }
}

U32 iFileGetSectorSize(xFileDriveType drive)
{
    switch (drive) {
    case XFILE_DRIVE_CDVD:
        return 32;
    default:
        iprintf("Unknown drive type in iFileGetSectorSize:  %d\n", drive);
        xFAIL_L(1062);
        break;
    }
    return 0;
}

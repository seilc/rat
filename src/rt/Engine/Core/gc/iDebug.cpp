#include "xCommon.h"
#include "iDebug.h"

#include "iFile.h"
#include "iSystem.h"
#include "iTime.h"
#include "xGlobals.h"
#include "xString.h"
#include "zScene.h"
#include "zUIImage.h"

#include <rtcharse.h>
#include <string.h>

static RtCharset* mCharset;
static RwRGBA mForegroundColor = { 200, 200, 200, 255 };
static RwRGBA mBackgroundColor = { 32, 32, 32, 0 };

void iDebugInit()
{
    if (!RtCharsetOpen()) {
        xFAIL_L(254);
    }

    mCharset = RtCharsetCreate(&mForegroundColor, &mBackgroundColor);
    if (!mCharset) {
        DBprintf(DBML_WARN, "xDebugInit, Cannot create raster charset.");
    }

    old_dsc = GXSetDrawSyncCallback(my_dsc);
}

void iDebugUpdate()
{
}

void iDebugWriteProfile()
{
}

void iDebugVSync()
{
    VIWaitForRetrace();
}

void iDebugStackTrace()
{
}

void iDebugIDEOutput(char* buf)
{
    OSReport(buf);
}

static void SWAP16(S32 n, void* vaddr)
{
    xASSERT_L(447, ((U32)vaddr & 1) == 0);
    U8* addr = (U8*)vaddr;
    while (n--) {
        U8 tmp = addr[1];
        addr[1] = addr[0];
        addr[0] = tmp;
        addr += 2;
    }
}

static void SWAP32(S32 n, void* vaddr)
{
    xASSERT_L(459, ((U32)vaddr & 3) == 0);
    U8* addr = (U8*)vaddr;
    while (n--) {
        U8 tmp = addr[3];
        addr[3] = addr[0];
        addr[0] = tmp;
        tmp = addr[2];
        addr[2] = addr[1];
        addr[1] = tmp;
        addr += 4;
    }
}

void iDebugScreenDump(RwCamera* camera)
{
    RwRaster* camRas;
    RwInt32 width, height, depth;
    xFile file;
    zUIImage* picImage;
    U32 size, n, x, y;
    char fname[64];
    U8* bp, *buf;
    iTime t0, t1;

    picImage = (zUIImage*)zSceneFindObject(xStrHash("UI GPAUSE PIC IMAGE"));
    picImage->StopMovie();

    camRas = RwCameraGetRaster(camera);

    width = RwRasterGetWidth(camRas);
    height = RwRasterGetHeight(camRas);
    depth = RwRasterGetDepth(camRas) >> 3;

    t0 = iTimeGet();

    size = FB_XRES * 3 * FB_YRES;

    strcpy(fname, xDebugGetNextDumpFile());
    xDebugAdvanceDumpFile();
    OSReport("Dumping frame buffer to '%s'\n", fname);

    if (iFileOpen(fname, IFILE_WRITE | IFILE_FULLPATH | IFILE_HOST_IO | 0xC, &file) != 0) {
        xASSERT_MSG_L(622, false, ("Cannot open file %s - is your path correct?", fname));
        return;
    }

    BmpMegaHeader mhdr;
    mhdr.bfType[0] = 'B';
    mhdr.bfType[1] = 'M';
    mhdr.bmFile.bfSize = size + sizeof(BmpMegaHeader) - sizeof(mhdr.prepad);
    mhdr.bmFile.bfReserved1 = 0;
    mhdr.bmFile.bfReserved2 = 0;
    mhdr.bmFile.bfOffBits = sizeof(BmpMegaHeader) - sizeof(mhdr.prepad);
    mhdr.bmInfo.biSize = sizeof(BITMAPINFOHEADER);
    mhdr.bmInfo.biWidth = FB_XRES;
    mhdr.bmInfo.biHeight = FB_YRES;
    mhdr.bmInfo.biPlanes = 1;
    mhdr.bmInfo.biBitCount = 24;
    mhdr.bmInfo.biCompression = 0;
    mhdr.bmInfo.biSizeImage = 0;
    mhdr.bmInfo.biXPelsPerMeter = 0;
    mhdr.bmInfo.biYPelsPerMeter = 0;
    mhdr.bmInfo.biClrUsed = 0;
    mhdr.bmInfo.biClrImportant = 0;

    SWAP32(1, &mhdr.bmFile.bfSize);
    SWAP16(2, &mhdr.bmFile.bfReserved1);
    SWAP32(1, &mhdr.bmFile.bfOffBits);

    SWAP32(3, &mhdr.bmInfo.biSize);
    SWAP16(2, &mhdr.bmInfo.biPlanes);
    SWAP32(6, &mhdr.bmInfo.biCompression);

    n = iFileWrite(&file, mhdr.bfType, sizeof(BmpMegaHeader) - sizeof(mhdr.prepad));
    xASSERT_L(703, n == sizeof(BmpMegaHeader) - sizeof(mhdr.prepad));

    U32 bufsize = FB_XRES * (3 * 16);
    bp = buf = (U8*)xMEMPUSHTEMP_L(bufsize, 0, 707);

    for (y = 0; y < FB_YRES; y++) {
        for (x = 0; x < FB_XRES; x++) {
            u32 pixval;
            GXPeekARGB(x, FB_YRES - 1 - y, &pixval);
            *bp++ = pixval & 0xFF;
            *bp++ = (pixval >> 8) & 0xFF;
            *bp++ = (pixval >> 16) & 0xFF;
        }

        if (bp - buf >= bufsize) {
            n = iFileWrite(&file, buf, bufsize);
            xASSERT_L(734, n == bufsize);
            bp = buf;
        }
    }

    xMEMPOPTEMP(buf);
    iFileClose(&file);

    OSReport("Done.\n");

    t1 = iTimeGet();
    OSReport("ticks:  %d  %6.4f s\n", t1 - t0, iTimeDiffSec(t0, t1));

    gScreenDump = exSD_NONE;
}

void iDebugDumpEFB(char* name)
{
    char filename[32];
    xFile file;
    U8 buf[4096];
    U16 x, y;

    GXDrawDone();
    GXPokeAlphaRead(GX_READ_NONE);

    strcpy(filename, name);
    strcat(filename, ".raw");
    iFileOpen(filename, IFILE_WRITE | IFILE_HOST_IO | 0xC, &file);

    for (y = 0; y < 528; y++) {
        U8* bp = buf;
        for (x = 0; x < 640; x++) {
            u32 pixval;
            GXPeekARGB(x, y, &pixval);
            *bp++ = (pixval >> 16) & 0xFF;
            *bp++ = (pixval >> 8) & 0xFF;
            *bp++ = pixval & 0xFF;
            *bp++ = (pixval >> 24) & 0xFF;
        }
        iFileWrite(&file, buf, 640 * 4);
    }

    iFileClose(&file);
}

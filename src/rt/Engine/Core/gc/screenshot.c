#include "screenshot.h"

#include <string.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define XFB_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT * 2)
#define EFB_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT * 3)
#define CHUNK_SIZE 0x19000
#define PORTION_SIZE 0x400
#define NUM_PORTIONS 100
#define HIO_BUFFER_ADDR 0x500
#define MAIL_XFB_CHUNK 1
#define MAIL_XFB_CHUNK_END 7
#define MAIL_EFB_CHUNK 11
#define MAIL_EFB_CHUNK_END 20
#define MAIL_PING 20
#define MAIL_PING_END 29
#define MAIL_MINIMIZE_ON 35
#define MAIL_MINIMIZE_OFF 36
#define MAIL_TIMEOUT 150000000

s32 g_usb_channel = -1;
BOOL g_connected;
u32 g_received_signal;
BOOL g_mail_waiting;
BOOL g_minimize_buffer;
void* g_data;

static BOOL HostIOEnumCallback(s32 chan)
{
    g_usb_channel = chan;
    return FALSE;
}

static void HostIOCallback()
{
    g_mail_waiting = TRUE;
}

static void CopyoutEFB(u8* buffer, u32 width, u32 height)
{
    u16 x, y;
    u32 color;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            GXPeekARGB(x, y, &color);
            buffer[2] = color;
            buffer[1] = color >> 8;
            buffer[0] = color >> 16;
            buffer += 3;
        }
    }
}

static void CopyoutPortionEFB(u8* buffer, u32 width, u32 offset, u32 size)
{
    u16 x = (offset / 3) % width;
    u16 y = (offset / 3) / width;
    u32 component = offset % 3;
    u32 count = 0;
    u32 color;

    while (count < size) {
        GXPeekARGB(x, y, &color);
        if (component == 0) {
            buffer[0] = color >> 16;
            count++;
        }
        if (component <= 1 && count < size) {
            buffer[1 - component] = color >> 8;
            count++;
        }
        if (component <= 2 && count < size) {
            buffer[2 - component] = color;
            count++;
        }
        x++;
        buffer += 3 - component;
        component = 0;
        if (x >= width) {
            x = 0;
            y++;
        }
    }
}

static void TakeScreenshotEFB(SCREENSHOTAllocator alloc)
{
    if (g_data == 0) {
        g_data = alloc(EFB_SIZE);
    }
    CopyoutEFB((u8*)g_data, SCREEN_WIDTH, SCREEN_HEIGHT);
    DCFlushRange(g_data, EFB_SIZE);
    OSReport("SCREENSHOT: Saved off screenshot to local memory.\n");
}

static void TakeScreenshotXFB(void* xfb, SCREENSHOTAllocator alloc)
{
    u8* src = (u8*)xfb;

    if (g_data == 0) {
        g_data = alloc(XFB_SIZE);
    }
    memcpy(g_data, src, XFB_SIZE);
    DCFlushRange(g_data, XFB_SIZE);
    OSReport("SCREENSHOT: Saved off screenshot to local memory.\n");
}

static void WriteScreenshotPortionEFBtoUSB(u32 chunk, SCREENSHOTAllocator alloc)
{
    u32 portionSize = PORTION_SIZE;
    u32 i;
    u32 chunkSize = portionSize * NUM_PORTIONS;

    if (g_data == 0) {
        g_data = alloc(portionSize);
    }
    for (i = 0; i < NUM_PORTIONS; i++) {
        CopyoutPortionEFB((u8*)g_data, SCREEN_WIDTH, chunk * chunkSize + i * portionSize, portionSize);
        DCFlushRange(g_data, portionSize);
        while (!HIOWrite(HIO_BUFFER_ADDR + i * portionSize, g_data, portionSize)) {
        }
    }
}

static BOOL GrabChunk(u32 signal, void* xfb, SCREENSHOTAllocator alloc, SCREENSHOTDeallocator dealloc)
{
    if (signal >= MAIL_XFB_CHUNK && signal < MAIL_XFB_CHUNK_END) {
        u8* data;

        if (signal == MAIL_XFB_CHUNK && !g_minimize_buffer) {
            TakeScreenshotXFB(xfb, alloc);
        }
        if (g_minimize_buffer) {
            u8* src = (u8*)xfb;
            data = src + (signal - MAIL_XFB_CHUNK) * CHUNK_SIZE;
        } else {
            data = (u8*)g_data + (signal - MAIL_XFB_CHUNK) * CHUNK_SIZE;
        }
        while (!HIOWrite(HIO_BUFFER_ADDR, data, CHUNK_SIZE)) {
        }
        OSReport("SCREENSHOT: Wrote chunk #%d.\n", signal);
        while (!HIOWriteMailbox(signal)) {
        }
        OSReport("SCREENSHOT: Notify of write chunk #%d put in mailbox.\n", g_received_signal);
    } else if (signal >= MAIL_EFB_CHUNK && signal < MAIL_EFB_CHUNK_END) {
        if (signal == MAIL_EFB_CHUNK && !g_minimize_buffer) {
            TakeScreenshotEFB(alloc);
        }
        if (g_minimize_buffer) {
            WriteScreenshotPortionEFBtoUSB(signal - MAIL_EFB_CHUNK, alloc);
        } else {
            u8* data = (u8*)g_data + (signal - MAIL_EFB_CHUNK) * CHUNK_SIZE;
            while (!HIOWrite(HIO_BUFFER_ADDR, data, CHUNK_SIZE)) {
            }
        }
        OSReport("SCREENSHOT: Wrote chunk #%d.\n", signal);
        while (!HIOWriteMailbox(signal)) {
        }
        OSReport("SCREENSHOT: Notify of write chunk #%d put in mailbox.\n", g_received_signal);
    }
    if (signal == MAIL_XFB_CHUNK_END - 1 || signal == MAIL_EFB_CHUNK_END - 1) {
        if (g_data != 0) {
            dealloc(g_data);
            g_data = 0;
        }
        return FALSE;
    }
    return TRUE;
}

static void CheckMail(void* xfb, SCREENSHOTAllocator alloc, SCREENSHOTDeallocator dealloc)
{
    u32 count = 0;
    BOOL waiting = TRUE;
    u32 mail = 0;

    while (waiting == TRUE) {
        count++;
        if (g_mail_waiting && HIOReadMailbox(&mail)) {
            g_mail_waiting = FALSE;
            waiting = FALSE;
            if (mail >= MAIL_PING && mail < MAIL_PING_END) {
                while (!HIOWriteMailbox(mail)) {
                }
                OSReport("SCREENSHOT: Sent ping back.\n");
            } else if ((mail >= MAIL_XFB_CHUNK && mail < MAIL_XFB_CHUNK_END) ||
                       (mail >= MAIL_EFB_CHUNK && mail < MAIL_EFB_CHUNK_END)) {
                g_received_signal = mail;
                count = 0;
                OSReport("SCREENSHOT: Grab screenshot request.\n");
                waiting = GrabChunk(g_received_signal, xfb, alloc, dealloc);
            } else if (mail == MAIL_MINIMIZE_ON) {
                g_minimize_buffer = TRUE;
            } else if (mail == MAIL_MINIMIZE_OFF) {
                g_minimize_buffer = FALSE;
            }
        }
        if (!g_minimize_buffer) {
            break;
        }
        if (g_minimize_buffer && count > MAIL_TIMEOUT) {
            if (g_data != 0) {
                dealloc(g_data);
                g_data = 0;
            }
            break;
        }
    }
}

static BOOL ConnectToUSB()
{
    if (!g_connected) {
        if (HIOEnumDevices(HostIOEnumCallback) && g_usb_channel >= 0 && HIOInit(g_usb_channel, HostIOCallback)) {
            u32 mail = 0;
            HIOReadMailbox(&mail);
            g_connected = TRUE;
            OSReport("SCREENSHOT: USB connected\n");
        }
    }
    return g_connected;
}

void SCREENSHOTService(void* xfb, SCREENSHOTAllocator alloc, SCREENSHOTDeallocator dealloc)
{
    if (!g_connected) {
        ConnectToUSB();
    } else if (g_mail_waiting) {
        CheckMail(xfb, alloc, dealloc);
    }
}

#include "xOutputMgr.h"

#include "xDebug.h"
#include "xString.h"

#include <stdarg.h>
#include <stdio.h>

struct xOutModstat
{
    U32 hval;
    char* name;
    xOutSeverity trumpsev;
};

#define XOUT_MAX_MOD_STAT 128

static xOutModstat xout_mss[XOUT_MAX_MOD_STAT];
static S32 xout_sev = xOutSevWarn;

void xOutSetSev(S32 sev)
{
    xout_sev = sev;
}

S32 xOutGetSev()
{
    return xout_sev;
}

static void xOutEnable(U32 hval, char* name, xOutSeverity trumpsev)
{
    U32 k = (hval % XOUT_MAX_MOD_STAT);
    U32 ok = k;

    while (xout_mss[k].hval != 0) {
        if (hval == xout_mss[k].hval) {
            xout_mss[k].trumpsev = trumpsev;
            return;
        }
        k = (k + 1) % XOUT_MAX_MOD_STAT;
        if (k == ok) {
            iprintf("xOutEnable: ERROR: overrun hashtable!!\n");
            return;
        }
    }

    xout_mss[k].hval = hval;
    xout_mss[k].name = name;
    xout_mss[k].trumpsev = trumpsev;
}

void xOutEnable(char* name, xOutSeverity trumpsev)
{
    xASSERT(111, name);
    xOutEnable(xStrHash(name), name, trumpsev);
}

S32 xOutEnabled(char* name)
{
    xASSERT(125, name);
    return xOutEnabled(xStrHash(name));
}

S32 xOutTrumps(char* name, xOutSeverity usersev)
{
    xASSERT(132, name);
    return xOutTrumps(xStrHash(name), usersev);
}

S32 xOutEnabled(U32 hval)
{
    U32 k = (hval % XOUT_MAX_MOD_STAT);
    U32 ok = k;

    while (hval != xout_mss[k].hval) {
        k = (k + 1) % XOUT_MAX_MOD_STAT;
        if (k == ok) {
            return FALSE;
        }
    }

    return TRUE;
}

S32 xOutTrumps(U32 hval, xOutSeverity usersev)
{
    U32 k = (hval % XOUT_MAX_MOD_STAT);
    U32 ok = k;

    while (hval != xout_mss[k].hval) {
        k = (k + 1) % XOUT_MAX_MOD_STAT;
        if (k == ok) {
            return FALSE;
        }
    }

    return xout_mss[k].trumpsev >= usersev;
}

#ifndef NON_MATCHING
void xOutputMgrDump()
{
    iprintf("xOutputMgr: dumping list of enabled modules:\n");
    iprintf(" --hashval-- | ----------name---------- | --trumps--\n");
    iprintf("yes");
    iprintf("no");
#ifdef DEBUG
    iprintf("<none>");
#endif
    iprintf("  0x%08x   %24s   %s\n");
    iprintf("<none>");
    iprintf("\t\t      --empty--\n");
}
#endif

void xOutCrit(const char* fmt, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    iprintf("[[[*** %s", buf);
}

void xOutErr(char* name, const char* fmt, ...)
{
    if (!xOutTrumps(name, xOutSevErr) && (!xOutEnabled(name) || xout_sev < xOutSevErr)) {
        return;
    }

    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    iprintf("[[[ %s", buf);
}

void xOutWarn(char* name, const char* fmt, ...)
{
    if (!xOutTrumps(name, xOutSevWarn) && (!xOutEnabled(name) || xout_sev < xOutSevWarn)) {
        return;
    }

    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    iprintf(">>> %s", buf);
}

void xOutInfo(char* name, const char* fmt, ...)
{
    if (!xOutTrumps(name, xOutSevInfo) && (!xOutEnabled(name) || xout_sev < xOutSevInfo)) {
        return;
    }

    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    iprintf("-- %s", buf);
}

void xOutBabble(char* name, const char* fmt, ...)
{
    if (!xOutTrumps(name, xOutSevBabble) && (!xOutEnabled(name) || xout_sev < xOutSevBabble)) {
        return;
    }

    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    iprintf("%s", buf);
}

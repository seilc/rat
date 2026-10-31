#include "xutil.h"

#include "xDebug.h"
#include "xRand.h"

#include "decomp.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static S32 g_xutilinit;
static S32 g_crc_needinit = 1;
static U32 g_crc32_table[256] = { 0 };

static U16 ascii_table[3][2] = {
    { 0x824F, 0x30 },
    { 0x8260, 0x41 },
    { 0x8281, 0x61 },
};

static U16 ascii_k_table[33] = {
    0x8140, 0x8149, 0x8168, 0x8194, 0x8190, 0x8193, 0x8195, 0x8166, 0x8169, 0x816A, 0x8196,
    0x817B, 0x8143, 0x817C, 0x8144, 0x815E, 0x8146, 0x8147, 0x8171, 0x8181, 0x8172, 0x8148,
    0x8197, 0x816D, 0x818F, 0x816E, 0x814F, 0x8151, 0x8165, 0x816F, 0x8162, 0x8170, 0x8150,
};

S32 xUtilStartup()
{
    if (g_xutilinit++ == 0) {
        DBprintf(DBML_VDBG, "xUtil: Startup\n");
        xUtil_crc_init();
    }

    return g_xutilinit;
}

S32 xUtilShutdown()
{
    if (--g_xutilinit == 0) {
        DBprintf(DBML_VDBG, "xUtil: Shutdown\n");
    }

    xVALIDATE_L(195, g_xutilinit>=0);
    return g_xutilinit;
}

char* xUtil_idtag2string(U32 srctag, S32 bufidx)
{
    U32 tag = srctag;
    char* strptr = NULL;
    char* uc = (char*)&tag;
    S32 is_mot = 0;
    static char buf[6][10] = { 0 };

    if (bufidx < 0 || bufidx >= 7) strptr = buf[0];
    else strptr = buf[bufidx];

    S32 l = 1;
    char* c = (char*)&l;
    xVALIDATE_EQ_L(221, sizeof(S32), 4);
    is_mot = c[3];

    if (is_mot) {
        char t = 0;
        t = uc[0]; uc[0] = uc[3]; uc[3] = t;
        t = uc[1]; uc[1] = uc[2]; uc[2] = t;
    }

    switch (bufidx) {
    case 4:
    case 5:
        strptr[0] = isprint(uc[0]) ? uc[0] : '?';
        strptr[1] = isprint(uc[1]) ? uc[1] : '?';
        strptr[2] = isprint(uc[2]) ? uc[2] : '?';
        strptr[3] = isprint(uc[3]) ? uc[3] : '?';
        break;
    case 6:
    default:
        strptr[0] = isprint(uc[3]) ? uc[3] : '?';
        strptr[1] = isprint(uc[2]) ? uc[2] : '?';
        strptr[2] = isprint(uc[1]) ? uc[1] : '?';
        strptr[3] = isprint(uc[0]) ? uc[0] : '?';
        break;
    }

    strptr[4] = '\0';

    if (bufidx == 6) {
        strptr[4] = '/';
        strptr[5] = isprint(uc[0]) ? uc[0] : '?';
        strptr[6] = isprint(uc[1]) ? uc[1] : '?';
        strptr[7] = isprint(uc[2]) ? uc[2] : '?';
        strptr[8] = isprint(uc[3]) ? uc[3] : '?';
        strptr[9] = '\0';
    }

    return strptr;
}

#if DEBUG
void xUtil_niceBufDump(char* membuf, S32 bytcnt, S32 ashex, S32 rptoff)
{
    S32* dabuf = (S32*)membuf;
    char* cptr = NULL;
    S32 i = 0, j = 0, cidx = 0;
    char ch = 0;
    S32 idx_stop = 0;
    const char* sfmt = NULL;
    S32 lnbk = 0, spbk = 0;
    char textbuf[32] = { 0 };
    S32 off = 0;

    if (ashex == 0) {
        sfmt = "%d%d%d%d%d%d%d%d ";
        lnbk = 4;
        spbk = 0x7FFFFFFF;
        DBprintf(DBML_DBG, "-- Writing bytes as bits (%d per line)--\n", lnbk);
    }
    else if (ashex > 0) {
        lnbk = 16;
        spbk = 2;
        sfmt = "%02X";
        DBprintf(DBML_DBG, "-- Writing bytes as HEX (%d per line) --\n", lnbk);
    }

    DBprintf(DBML_DBG, "%-10s %-7s || %s\n", "< Offset >", "< Idx >", "< Byte Values >");

    idx_stop = bytcnt - bytcnt % 4;
    idx_stop /= 4;
    cidx = 0;
    off = 0;
    for (j = 0; j < idx_stop; j++) {
        if (cidx % lnbk == 0) {
            if (cidx) DBprintf(DBML_DBG, "\n");
            DBprintf(DBML_DBG, "0x%08X (%05d) || ", cidx + rptoff, cidx);
        }

        cptr = (char*)&dabuf[j];
        for (i = 0; i < 4; i++) {
            ch = cptr[i];
            cidx++;
            if (ashex) DBprintf(DBML_DBG, sfmt, ch);
            else
                DBprintf(DBML_DBG, sfmt,
                         (ch & 0x01) ? 1 : 0, (ch & 0x02) ? 1 : 0, (ch & 0x04) ? 1 : 0, (ch & 0x08) ? 1 : 0,
                         (ch & 0x10) ? 1 : 0, (ch & 0x20) ? 1 : 0, (ch & 0x40) ? 1 : 0, (ch & 0x80) ? 1 : 0);

            if (cidx % spbk == 0) DBprintf(DBML_DBG, " ");

            if (off < 31) {
                textbuf[off++] = isprint(ch) ? ch : '?';
            }
        }

        if (cidx % lnbk == 0) {
            xVALIDATE_L(371, off<=32);
            textbuf[off] = '\0';
            DBprintf(DBML_DBG, "%s", textbuf);
            off = 0;
        }
    }

    idx_stop *= 4;
    DBprintf(DBML_VDBG, "bytcnt: %d cidx: %d idx_stop: %d\n", bytcnt, cidx, idx_stop);

    cptr = membuf + idx_stop;
    idx_stop = bytcnt - idx_stop;

    if (idx_stop) {
        if (cidx % lnbk == 0) {
            if (cidx) DBprintf(DBML_DBG, "\n");
            DBprintf(DBML_DBG, "0x%08X (%05d) || ", cidx + rptoff, cidx);
        }

        for (i = 0; i < 32; i++) {
            cidx++;
            if (i < idx_stop) ch = cptr[i];
            else ch = ' ';

            if (ashex) DBprintf(DBML_DBG, sfmt, ch);
            else
                DBprintf(DBML_DBG, sfmt,
                         (ch & 0x01) ? 1 : 0, (ch & 0x02) ? 1 : 0, (ch & 0x04) ? 1 : 0, (ch & 0x08) ? 1 : 0,
                         (ch & 0x10) ? 1 : 0, (ch & 0x20) ? 1 : 0, (ch & 0x40) ? 1 : 0, (ch & 0x80) ? 1 : 0);

            if (cidx && cidx % spbk == 0) DBprintf(DBML_DBG, " ");

            if (i < idx_stop) textbuf[off++] = isprint(ch) ? ch : '?';
            else textbuf[off++] = ' ';

            if (cidx % lnbk == 0) {
                textbuf[off] = '\0';
                DBprintf(DBML_DBG, "%s", textbuf);
                break;
            }
        }
    }

    DBprintf(DBML_DBG, "\n");
}
#endif

U32 xUtil_crc_init()
{
    S32 i = 0, j = 0;
    U32 crc_accum = 0;

    if (g_crc_needinit) {
        for (i = 0; i < 256; i++) {
            crc_accum = (U32)i << 24;
            for (j = 0; j < 8; j++) {
                if (crc_accum & 0x80000000) {
                    crc_accum = (crc_accum << 1) ^ 0x04C11DB7;
                } else {
                    crc_accum = crc_accum << 1;
                }
            }
            g_crc32_table[i] = crc_accum;
        }

        g_crc_needinit = 0;
    }

    return 0xFFFFFFFF;
}

U32 xUtil_crc_update(U32 crc_accum, char* data, S32 datasize)
{
    S32 i = 0, j = 0;

    xVALIDATE_L(520, g_crc_needinit==0);
    if (g_crc_needinit) {
        DBprintf(DBML_WARN, "Scary CRC table not ready - initializing now\n");
        xUtil_crc_init();
    }

    for (j = 0; j < datasize; j++) {
        i = ((S32)(crc_accum >> 24) ^ *data++) & 0xFF;
        crc_accum = (crc_accum << 8) ^ g_crc32_table[i];
    }

    return crc_accum;
}

void strtosjis(U8* string, U8* dest)
{
    S32 i, sjis_code, ascii_code;
    U8 stmp = 0x30, stmp2 = 0x30;
    S32 ctr = 0;
    U8* dest2 = dest;

    for (i = 0; i < 32; i++) {
        *dest2++ = 0;
        *dest2++ = 0;
    }

    while (*string) {
        ascii_code = *string++; stmp2 = 0;
        if (ascii_code >= 0x20 && ascii_code <= 0x2F)
            stmp2 = 1;
        else if (ascii_code >= 0x30 && ascii_code <= 0x39)
            stmp = 0;
        else if (ascii_code >= 0x3A && ascii_code <= 0x40)
            stmp2 = 11;
        else if (ascii_code >= 0x41 && ascii_code <= 0x5A)
            stmp = 1;
        else if (ascii_code >= 0x5B && ascii_code <= 0x60)
            stmp2 = 37;
        else if (ascii_code >= 0x61 && ascii_code <= 0x7A)
            stmp = 2;
        else if (ascii_code >= 0x7B && ascii_code <= 0x7E)
            stmp2 = 63;
        else {
            printf("bad ASCII code 0x%x\n", ascii_code);
            exit(1);
        }

        if (stmp2)
            sjis_code = ascii_k_table[ascii_code - 0x20 - (stmp2 - 1)];
        else
            sjis_code = ascii_code + ascii_table[stmp][0] - ascii_table[stmp][1];

        ctr++;
        *dest++ = (sjis_code & 0xFF00) >> 8;
        *dest++ = sjis_code & 0x00FF;
    }
}

DECOMP_FORCEACTIVE("Unknown SJIS code, possibly NULL %x,%x,%x\n")
DECOMP_FORCEACTIVE("BAD CODE\n")
DECOMP_FORCEACTIVE("%x")

S32 xUtil_yesno(F32 wt_yes)
{
    if (wt_yes == 0.0f) return 0;
    if (wt_yes == 1.0f) return 1;
    if (xurand() <= wt_yes) return 1;
    return 0;
}

#if DEBUG
DECOMP_FORCEACTIVE("%s(%d) : (wts) in '%s'\n")
DECOMP_FORCEACTIVE("%s(%d) : (arbref>0.0f) in '%s'\n")
DECOMP_FORCEACTIVE("%s(%d) : (wts[i]>0.0f) in '%s'\n")
DECOMP_FORCEACTIVE("%s(%d) : (sum>1.0E-5f) in '%s'\n")
#endif

#if DEBUG
void xUtil_spit_flagbits(U32 flag, S32 toScreen)
{
    xUtil_spit_flagbits(flag, toScreen, sizeof(flag));
}

void xUtil_spit_flagbits(S32 flag, S32 toScreen, S32 numbytes)
{
    S32 i = 0;
    en_VERBOSE_MSGLEVEL dbml = DBML_DBG;

    if (toScreen) dbml = DBML_DISP;
    if (numbytes > 4) numbytes = 4;
    if (numbytes < 1) numbytes = 4;

    DBprintf(dbml, "b:");
    for (i = 0; i < 8; i++) DBprintf(dbml, " %02d", i);
    if (numbytes == 1) {
        DBprintf(dbml, "\n");
        DBprintf(dbml, "v:");
        for (i = 0; i < 8; i++) DBprintf(dbml, " %2d", (flag & (1 << i)) ? 1 : 0);
        DBprintf(dbml, "\n");
    }

    if (numbytes == 1) {
        return;
    }

    for (i = 8; i < 16; i++) DBprintf(dbml, " %02d", i);
    DBprintf(dbml, "\n");

    DBprintf(dbml, "v:");
    for (i = 0; i < 16; i++) DBprintf(dbml, " %2d", (flag & (1 << i)) ? 1 : 0);
    DBprintf(dbml, "\n");

    if (numbytes == 2) {
        return;
    }

    DBprintf(dbml, "b:");
    for (i = 16; i < 24; i++) DBprintf(dbml, " %02d", i);
    if (numbytes == 3) {
        DBprintf(dbml, "\n");
        DBprintf(dbml, "v:");
        for (i = 16; i < 24; i++) DBprintf(dbml, " %2d", (flag & (1 << i)) ? 1 : 0);
        DBprintf(dbml, "\n");
    }

    if (numbytes == 3) {
        return;
    }

    for (i = 24; i < 32; i++) DBprintf(dbml, " %02d", i);
    DBprintf(dbml, "\n");

    DBprintf(dbml, "v:");
    for (i = 16; i < 32; i++) DBprintf(dbml, " %2d", (flag & (1 << i)) ? 1 : 0);
    DBprintf(dbml, "\n");
}
#endif

namespace {

struct sort_int_functor
{
    size_t key_offset;

    sort_int_functor(size_t key_offset) : key_offset(key_offset) {}

    S32 operator()(const void* e1, const void* e2) const { return *(S32*)((U8*)e1 + key_offset) - *(S32*)((U8*)e2 + key_offset); }
};

struct sort_uint_functor
{
    size_t key_offset;

    sort_uint_functor(size_t key_offset) : key_offset(key_offset) {}

    S32 operator()(const void* e1, const void* e2) const
    {
        U32 uint1 = *(U32*)((U8*)e1 + key_offset);
        U32 uint2 = *(U32*)((U8*)e2 + key_offset);
        if (uint1 > uint2)
            return 1;
        if (uint1 == uint2)
            return 0;
        return -1;
    }
};

struct sort_float_functor
{
    size_t key_offset;

    sort_float_functor(size_t key_offset) : key_offset(key_offset) {}

    S32 operator()(const void* e1, const void* e2) const
    {
        F32 diff = *(F32*)((U8*)e1 + key_offset) - *(F32*)((U8*)e2 + key_offset);
        return *(S32*)&diff;
    }
};

struct sort_str_functor
{
    size_t key_offset;

    sort_str_functor(size_t key_offset);

    S32 operator()(const void* e1, const void* e2) const;
};

struct sort_istr_functor
{
    size_t key_offset;

    sort_istr_functor(size_t key_offset) : key_offset(key_offset) {}

    S32 operator()(const void* e1, const void* e2) const
    {
        const char* s1 = *(const char**)((U8*)e1 + key_offset);
        const char* s2 = *(const char**)((U8*)e2 + key_offset);
        return stricmp(s1, s2);
    }
};

}

void quicksort_int(void* base, size_t num, size_t width, size_t key_offset)
{
    xASSERT_L(1053, (key_offset & 3) == 0);
    quicksort(base, num, width, sort_int_functor(key_offset));
}

void quicksort_uint(void* base, size_t num, size_t width, size_t key_offset)
{
    xASSERT_L(1059, (key_offset & 3) == 0);
    quicksort(base, num, width, sort_uint_functor(key_offset));
}

void quicksort_float(void* base, size_t num, size_t width, size_t key_offset)
{
    xASSERT_L(1065, (key_offset & 3) == 0);
    quicksort(base, num, width, sort_float_functor(key_offset));
}

void quicksort_istr(void* base, size_t num, size_t width, size_t key_offset)
{
    xASSERT_L(1075, (key_offset & 3) == 0);
    quicksort(base, num, width, sort_istr_functor(key_offset));
}

// RESTORED: speculative
void isort_int(void* base, size_t num, size_t width, size_t key_offset)
{
    xASSERT((key_offset & 3) == 0);
    isort(base, num, width, sort_int_functor(key_offset));
}
// END RESTORED

void isort_float(void* base, size_t num, size_t width, size_t key_offset)
{
    xASSERT_L(1086, (key_offset & 3) == 0);
    isort(base, num, width, sort_float_functor(key_offset));
}

// RESTORED: speculative
void isort_istr(void* base, size_t num, size_t width, size_t key_offset)
{
    xASSERT((key_offset & 3) == 0);
    isort(base, num, width, sort_istr_functor(key_offset));
}
// END RESTORED

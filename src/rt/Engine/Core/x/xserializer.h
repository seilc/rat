#ifndef XSERIALIZER_H
#define XSERIALIZER_H

#include "xVec3.h"
#include "xsavegame.h"

struct st_SERIAL_CLIENTINFO;

struct st_SERIAL_PERCID_SIZE
{
    U32 idtag;
    S32 needsize;
};

struct xSerial
{
    ~xSerial();

    void setClient(U32 idtag);

    S32 Write(char* data, S32 elesize, S32 n);
    S32 Write_bn(S32 bits, S32 n);
    S32 Write_bn(U32 bits, S32 n);
    S32 Write_b1(S32 bits);
    S32 Write_b1(U32 bits);
    S32 Write_b2(S32 bits);
    S32 Write_b2(U32 bits);
    S32 Write_b3(S32 bits);
    S32 Write_b3(U32 bits);
    S32 Write_b4(S32 bits);
    S32 Write_b4(U32 bits);
    S32 Write_b5(S32 bits);
    S32 Write_b5(U32 bits);
    S32 Write_b6(S32 bits);
    S32 Write_b6(U32 bits);
    S32 Write_b7(S32 bits);
    S32 Write_b7(U32 bits);
    S32 Write(char data);
    S32 Write(U8 data);
    S32 Write(S8 data);
    S32 Write(S16 data);
    S32 Write(U16 data);
    S32 Write(S32 data);
    S32 Write(U32 data);
    S32 Write(F32 data);
    S32 Write(char* data, S32 n);
    S32 Write(U8* data, S32 n);
    S32 Write(S8* data, S32 n);
    S32 Write(S16* data, S32 n);
    S32 Write(U16* data, S32 n);
    S32 Write(S32* data, S32 n);
    S32 Write(U32* data, S32 n);
    S32 Write(F32* data, S32 n);
    S32 Write(xVec3* data);

    S32 Read(char* buf, S32 elesize, S32 n);
    S32 Read_bn(U32* bits, S32 n);
    S32 Read_bn(S32* bits, S32 n);
    S32 Read_b1(S32* bits);
    S32 Read_b1(U32* bits);
    S32 Read_b2(S32* bits);
    S32 Read_b2(U32* bits);
    S32 Read_b3(S32* bits);
    S32 Read_b3(U32* bits);
    S32 Read_b4(S32* bits);
    S32 Read_b4(U32* bits);
    S32 Read_b5(S32* bits);
    S32 Read_b5(U32* bits);
    S32 Read_b6(S32* bits);
    S32 Read_b6(U32* bits);
    S32 Read_b7(S32* bits);
    S32 Read_b7(U32* bits);
    S32 Read(char* buf);
    S32 Read(U8* buf);
    S32 Read(S8* buf);
    S32 Read(S16* buf);
    S32 Read(U16* buf);
    S32 Read(S32* buf);
    S32 Read(U32* buf);
    S32 Read(F32* buf);
    S32 Read(char* buf, S32 n);
    S32 Read(U8* buf, S32 n);
    S32 Read(S8* buf, S32 n);
    S32 Read(S16* buf, S32 n);
    S32 Read(U16* buf, S32 n);
    S32 Read(S32* buf, S32 n);
    S32 Read(U32* buf, S32 n);
    S32 Read(F32* buf, S32 n);
    S32 Read(xVec3* data);

    void wrbit(S32 is_on);
    S32 rdbit();
    void prepare(U32 idtag);
    void reset();

#if DEBUG || RELEASE
    void DBBitDump(S32 ashex) const;
#endif

private:
    U32 idtag;
    S32 baseoff;
    st_SERIAL_CLIENTINFO* ctxtdata;
    S32 warned;
    S32 curele;
    S32 bitidx;
    S32 bittally;
};

S32 xSerialStartup(S32 count, st_SERIAL_PERCID_SIZE* sizeinfo);
S32 xSerialShutdown();
void xSerialTraverse(S32 (*func)(U32, xSerial*));
void xSerialWipeMainBuffer();
S32 xSerial_svgame_register(st_XSAVEGAME_DATA* sgctxt, en_SAVEGAME_MODE mode);

#if DEBUG || RELEASE
void xSerial_DB_dumpBuffers(S32 ashex);
void xSerial_DB_dumpWatermarks();
#endif

#endif

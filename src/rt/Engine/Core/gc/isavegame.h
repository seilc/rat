#ifndef ISAVEGAME_H
#define ISAVEGAME_H

#include "types.h"

struct st_ISGSESSION;
struct st_ISGTHUMDATA;

enum en_ASYNC_OPCODE
{
    ISG_OPER_NOOP,
    ISG_OPER_INIT,
    ISG_OPER_SAVE,
    ISG_OPER_LOAD
};

enum en_ASYNC_OPSTAT
{
    ISG_OPSTAT_FAILURE = -1,
    ISG_OPSTAT_INPROG,
    ISG_OPSTAT_SUCCESS
};

enum en_ASYNC_OPERR
{
    ISG_OPERR_NONE,
    ISG_OPERR_NOOPER,
    ISG_OPERR_MULTIOPER,
    ISG_OPERR_INITFAIL,
    ISG_OPERR_GAMEDIR,
    ISG_OPERR_NOCARD,
    ISG_OPERR_NOROOM,
    ISG_OPERR_DAMAGE,
    ISG_OPERR_CORRUPT,
    ISG_OPERR_OTHER,
    ISG_OPERR_SVNOSPACE,
    ISG_OPERR_SVINIT,
    ISG_OPERR_SVWRITE,
    ISG_OPERR_SVOPEN,
    ISG_OPERR_LDINIT,
    ISG_OPERR_LDREAD,
    ISG_OPERR_LDOPEN,
    ISG_OPERR_TGTERR,
    ISG_OPERR_TGTREM,
    ISG_OPERR_TGTPREP,
    ISG_OPERR_UNKNOWN,
    ISG_OPERR_NOMORE
};

enum en_CHGCODE
{
    ISG_CHG_NONE,
    ISG_CHG_TARGET,
    ISG_CHG_GAMELIST
};

enum en_NAMEGEN_TYPE
{
    ISG_NGTYP_GAMEDIR,
    ISG_NGTYP_GAMEFILE,
    ISG_NGTYP_CONFIG,
    ISG_NGTYP_ICONTHUM
};

typedef void(*ISGChangeCallback)(void* cltdata, en_CHGCODE what);

S32 iSGStartup();
S32 iSGShutdown();
char* iSGMakeName(en_NAMEGEN_TYPE type, const char* base, S32 idx);
st_ISGSESSION* iSGSessionBegin(void* cltdata, ISGChangeCallback chgfunc, S32 monitor);
void iSGSessionEnd(st_ISGSESSION* isgdata);
S32 iSGMaxSaveGame(st_ISGSESSION* isgdata);
S32 iSGTgtCount(st_ISGSESSION* isgdata, S32* max);
S32 iSGTgtPhysSlotIdx(st_ISGSESSION* isgdata, S32 tidx);
S32 iSGTgtPhysSlotInfo(st_ISGSESSION*, S32 physslot); // UNUSED
char* iSGTgtDispName(st_ISGSESSION*, S32 tgtidx, S32 form); // UNUSED
S32 iSGTgtFormat(st_ISGSESSION* isgdata, S32 tgtidx, S32 async, S32* canRecover);
U32 iSGTgtState(st_ISGSESSION* isgdata, S32 tgtidx, const char* dpath);
S32 iSGStateError(st_ISGSESSION*, char* errbuf, S32 flagcode); // UNUSED
S32 iSGTgtFreeSpace(st_ISGSESSION* isgdata, S32 tgtidx, S32* clustsize, S32* availFile);
S32 iSGTgtSetActive(st_ISGSESSION* isgdata, S32 tgtidx);
st_ISGTHUMDATA* iSGTgtIconThum(st_ISGSESSION*, S32); // UNUSED
S32 iSGTgtHaveRoom(st_ISGSESSION* isgdata, S32 tgtidx, S32 fsize, const char* dpath, const char* fname, S32* needbyte,
                   S32* availbyte, S32* needFile);
S32 iSGTgtHaveRoomStartup(st_ISGSESSION* isgdata, S32, S32 fsize, const char*, const char* fname, S32* needbyte,
                          S32* availbyte, S32* needFile); // UNUSED
S32 iSGFileCount(st_ISGSESSION* isgdata); // UNUSED
char* iSGFileName(st_ISGSESSION* isgdata, S32 fidx); // UNUSED
S32 iSGFileSize(st_ISGSESSION* isgdata, S32 fidx); // UNUSED
S32 iSGFileSize(st_ISGSESSION* isgdata, const char* fname);
st_ISGTHUMDATA* iSGFileIconThum(st_ISGSESSION*, S32); // UNUSED
st_ISGTHUMDATA* iSGFileIconThum(st_ISGSESSION*, const char*); // UNUSED
char* iSGFileModDate(st_ISGSESSION* isgdata, S32 fidx); // UNUSED
const char* iSGFileModDate(st_ISGSESSION* isgdata, const char* fname);
char* iSGFileModDate(st_ISGSESSION* isgdata, S32 fidx, S32* sec, S32* min, S32* hr, S32* mon, S32* day,
                     S32* yr); // UNUSED
char* iSGFileModDate(st_ISGSESSION* isgdata, const char* fname, S32* usr_sec, S32* usr_min, S32* usr_hr, S32* usr_mon,
                     S32* usr_day, S32* usr_yr);
void iSGMakeTimeStamp(char* str);
S32 iSGValidateFiles(st_ISGSESSION* isgdata, S32 allowDelete, S32* badFileHardIndex, S32* firstKill); // UNUSED
S32 iSGSelectGameDir(st_ISGSESSION* isgdata, const char* dname);
S32 iSGSetupGameDir(st_ISGSESSION* isgdata, const char* dname, S32 force_iconfix);
S32 iSGSaveFile(st_ISGSESSION* isgdata, const char* fname, char* data, S32 n, S32 async); // UNUSED
S32 iSGSaveFile(st_ISGSESSION* isgdata, const char* fname, char* data, S32 n, S32 async, char* label);
S32 iSGLoadFile(st_ISGSESSION* isgdata, const char* fname, char* databuf, S32 async);
S32 iSGReadLeader(st_ISGSESSION* isgdata, const char* fname, char* databuf, S32 numbytes, S32 async);
en_ASYNC_OPSTAT iSGPollStatus(st_ISGSESSION* isgdata, en_ASYNC_OPCODE* curop, S32 block);
en_ASYNC_OPERR iSGOpError(st_ISGSESSION* isgdata, char* errbuf);
S32 iSG_mcidx2slot(S32 mcidx, S32* slot, S32* concnt);
void CheckExCallback(s32, s32 result);
void CardCreateCallback(s32, s32 result);
void CardDeleteCallback(s32, s32 result);
S32 iSGCheckForWrongDevice();
S32 iSGCheckForCorruptFiles(st_ISGSESSION* isgdata, char (*fileName)[64]); // UNUSED
bool iSGIsGameCorrupt(st_ISGSESSION* sess, S32 gidx);
bool iSGCheckMemoryCard(st_ISGSESSION* isgdata, S32 index);
void iSGAutoSave_Startup();
void iSGAutoSave_Shutdown(); // UNUSED
st_ISGSESSION* iSGAutoSave_Connect(S32 idx_target, void* cltdata, ISGChangeCallback chg);
void iSGAutoSave_Disconnect(st_ISGSESSION* isg);
S32 iSGAutoSave_Monitor(st_ISGSESSION* isg, S32 idx_target);
#if DEBUG
void iSGDB_TgtReport(st_ISGSESSION* session, S32 tgtidx); // UNUSED
void iSGDB_TgtForceFormat(st_ISGSESSION* session, S32 tgtidx);
S32 iSGGameDisplaySize(st_ISGSESSION**); // UNUSED
#endif

#endif

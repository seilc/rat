#ifndef XDEBUGLINK_H
#define XDEBUGLINK_H

#include "types.h"

class xDebugLink;

enum eError
{
    eReadyForBin = 1,
    eSuccess = 0,
    eGenericError = -1,
    eUnableToInitialize = -2,
    eInvalidCmd = -3,
    eNotConnected = -4,
    eUnableToConnect = -5,
    eUnableToFindAsset = -6,
    eAssetSizeMismatch = -7,
    eInsufficientArgs = -8,
    eUnableToDeInitialize = -9,
    eUnableToDisconnect = -10,
    eUnableToGetCurrentScene = -11,
    eUnableToFindAssetType = -12,
    eUnableToSendCommand = -13,
    eUnableToSendAssetData = -14,
    eUnableToReceiveStatus = -15,
    eConnectionLost = -16,
    eNoActiveDebugLink = -17
};

struct Command
{
    char command[2048];
    void* userData;
};

struct CommandHandlerMap
{
    char command[12];
    eError (xDebugLink::*handler)(char** argv, S32 argc, void*);
};

class xDebugLink
{
public:
    xDebugLink();

    virtual eError Initialize() = 0;
    virtual eError Connect(char** argv, S32 argc, void*);
    virtual eError Disconnect(char** argv, S32 argc, void*);
    virtual eError SendEntEvent(char** argv, S32 argc, void*);
    virtual eError ReadyForAssetData(char**, S32, void*) = 0;
    virtual eError HeartBeatResponse(char** argv, S32 argc, void*) = 0;
    virtual char* GetDevkitName() const;
    virtual char* GetConnectedComputerName() const;
    virtual bool IsConnected() const;
    virtual void Update(F32);

    eError UpdateSceneObject(char* receivedBuffer);
    S32 CmdToArgv(char* str, char** argv, S32 maxargs);

    static char mBuffer[2048];

protected:
    Command mCmdBuffer[16];
    S32 mNumCmd;
    CommandHandlerMap mCommandHandlerMap[16];
    bool mConnected;
    U32 mAssetToUpdate;
    U32 mAssetDataReceivedSize;
    bool mProcessingCmds;
    char mConnectedComputer[256];
    char mDevkitName[256];
};

#endif

#include "xDebugLink.h"

#include "xMemMgr.h"
#include "xstransvc.h"
#include "xEvent.h"
#include "zAssetTypes.h"
#include "zCameraCurveAsset.h"
#include "zEntTrigger.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zNPCAsset.h"
#include "zScene.h"
#include "decomp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define xDebugLinkPrintf

char xDebugLink::mBuffer[2048];

xDebugLink::xDebugLink()
{
    memset(mDevkitName, 0, sizeof(mDevkitName));
    memset(mCmdBuffer, 0, sizeof(mCmdBuffer));
    mNumCmd = 0;
    mProcessingCmds = false;
}

DECOMP_FORCEACTIVE(
    "XCMD",
    "Already processing commands. Discarding message.",
    DEBUG ? "Response = %s, ResponseLen = %d\n" : 0,
    "Unknown command prefix %s!",
    DEBUG ? "Received Command = %s\n" : 0,
    "__UPDTASS__",
    "Successfully performed command: %s",
    "Unable to perform command: %s",
    "Queued %s",
    "Connect command received!",
    "__CONNECT__",
    "Already connected to [%s]. Can not accept connection from [%s]!",
    DEBUG ? "\n%s" : 0,
    DEBUG ? "%s\n" : 0,
    "help",
    "Successfully connected!",
    "Unable to perform command: %s!",
    "Not connected yet!"
)

eError xDebugLink::UpdateSceneObject(char* receivedBuffer)
{
    xBase* sceneObject;
    char* asset;
    U32 oldAssetSize;
    char* oldAsset;
    char* tempAllocation;
    st_PACKER_ATOC_NODE* assetNode;

    tempAllocation = NULL;
    oldAsset = NULL;
    assetNode = NULL;

    asset = (char*)xSTFindAsset(mAssetToUpdate, &oldAssetSize);
    if (asset)
    {
        if (oldAssetSize != mAssetDataReceivedSize)
        {
            xDebugLinkPrintf("Asset size mismatch! Data received was (%d) and scene reports asset size as (%d).\n",
                             mAssetDataReceivedSize, oldAssetSize);

            xDebugLinkPrintf("Getting asset node...\n");
            assetNode = xSTFindAssetNode(mAssetToUpdate, NULL);

            xDebugLinkPrintf("Allocating memory...\n");
            tempAllocation = (char*)PKR_getmem('ASET', assetNode->d_size, assetNode->aid, 64, eMemMgrTag_Packer);
            if (tempAllocation)
            {
                xDebugLinkPrintf("Orginial x_size and d_size = [ %d %d ]...\n", assetNode->x_size, assetNode->d_size);
                oldAsset = assetNode->memloc;

                assetNode->memloc = tempAllocation;
                assetNode->d_size = mAssetDataReceivedSize;
                assetNode->readrem = mAssetDataReceivedSize;
                memcpy(assetNode->memloc, receivedBuffer, mAssetDataReceivedSize);
                asset = assetNode->memloc;
            }
            else
            {
                xDebugLinkPrintf("Failed to allocated %d bytes!\n", mAssetDataReceivedSize);
                return eAssetSizeMismatch;
            }
        }

        const xBaseAsset* baseAsset = (const xBaseAsset*)asset;
        switch (baseAsset->baseType)
        {
        case eBaseTypeSpline:
        {
            memcpy(asset, receivedBuffer, mAssetDataReceivedSize);
            asset = (char*)Spline_Read(NULL, 0, asset, mAssetDataReceivedSize, &mAssetDataReceivedSize);

            xDebugLinkPrintf("Updating Camera Curves. Searching for Curve: (0x%X)\n", mAssetToUpdate);
            S32 numberOfCameraCurvesInScene = xSTAssetCountByType('CCRV');
            S32 count;
            for (count = 0; count < numberOfCameraCurvesInScene; count++)
            {
                zCameraCurveAsset* cameraCurveAsset = (zCameraCurveAsset*)xSTFindAssetByType('CCRV', count, NULL);
                for (U32 i = 0; i < eCamCurveCam_NumCam; i++)
                {
                    if (mAssetToUpdate == cameraCurveAsset->GetCurveID((eCamCurveCam)i))
                    {
                        sceneObject = zSceneFindObject(cameraCurveAsset->id);
                        if (sceneObject)
                        {
                            xDebugLinkPrintf("Sending Debug Reset event to Camera Curve (0x%X)\n", sceneObject->id);
                            if (oldAsset)
                            {
                                PKR_relmem('ASET', oldAssetSize, oldAsset, mAssetToUpdate, 0);
                                oldAsset = NULL;
                            }

                            zEntEvent(NULL, sceneObject, eEventDebugReset);
                        }
                        else
                        {
                            xDebugLinkPrintf("Unable to find scene object (0x%X)\n", mAssetToUpdate);
                            return eUnableToFindAsset;
                        }
                    }
                }
            }

            for (count = 0; count < globals.sceneCur->numSpecialTriggers; count++)
            {
                zEntTrigger& trigger = *(zEntTrigger*)globals.sceneCur->specialTriggers[count];
                const xTriggerAsset& triggerAsset = trigger.Asset();

                if (triggerAsset.flags & 0x1000)
                {
                    if (triggerAsset.camTransition.curveID == mAssetToUpdate)
                    {
                        zEntEvent(NULL, &trigger, eEventDebugReset);
                    }
                }
            }

            break;
        }
        case eBaseTypeUnknown:
            if (zNPCAsset_IsNPC(*(xDynAsset*)asset))
            {
                xDebugLinkPrintf("Unable to update scene object (0x%X) because it's an NPC\n", mAssetToUpdate);
                return eUnableToFindAsset;
            }

        default:
            sceneObject = zSceneFindObject(mAssetToUpdate);
            if (sceneObject)
            {
                if (oldAssetSize == mAssetDataReceivedSize)
                {
                    memcpy(asset, receivedBuffer, mAssetDataReceivedSize);
                }
                else
                {
                    if (!zSceneUpdateAsset(*sceneObject, asset, mAssetDataReceivedSize))
                    {
                        xDebugLinkPrintf("Unable to update new asset\n");
                        if (tempAllocation)
                        {
                            PKR_relmem('ASET', mAssetDataReceivedSize, tempAllocation, mAssetToUpdate, 0);
                        }

                        assetNode->memloc = oldAsset;
                        assetNode->d_size = oldAssetSize;
                        assetNode->readrem = oldAssetSize;
                        return eAssetSizeMismatch;
                    }
                    else
                    {
                        PKR_relmem('ASET', oldAssetSize, oldAsset, mAssetToUpdate, 0);
                    }
                }

                xDebugLinkPrintf("Sending Debug Reset event to (0x%X)\n", sceneObject->id);
                zEntEvent(NULL, sceneObject, eEventDebugReset);
            }
            else
            {
                xDebugLinkPrintf("Unable to find scene object (0x%X)\n", mAssetToUpdate);
                return eUnableToFindAsset;
            }
            break;
        }
    }
    else
    {
        return eUnableToFindAsset;
    }

    return eSuccess;
}

eError xDebugLink::SendEntEvent(char** argv, S32 argc, void*)
{
    F32 params[4];
    S32 i;
    U32 event, widgetID, paramWidgetID;
    xBase* sceneObject;

    if (argc < 2)
    {
        return eInsufficientArgs;
    }

    xDebugLinkPrintf("Argc = %d\n", argc);
    event = atoi(argv[1]);
    widgetID = atoi(argv[0]);

    if (argc - 1 == 6)
    {
        for (i = 2; i <= 5; i++)
        {
            params[i - 2] = atof(argv[i]);
        }
        paramWidgetID = atoi(argv[6]);
    }
    else
    {
        memset(params, 0, sizeof(params));
        paramWidgetID = 0;
    }

    if (event == eEventSetText || event == eEventAddText)
    {
        params[0] = *(F32*)&paramWidgetID;
    }

    sceneObject = zSceneFindObject(widgetID);
    if (sceneObject)
    {
        xDebugLinkPrintf("Sending Event (%d) event to %s (0x%X)\n", event, xSTAssetName(widgetID), widgetID);
        xDebugLinkPrintf("Params: [ %8.4f %8.4f %8.4f %8.4f ] ParamWidgetID: [ 0x%X ]\n",
                         params[0], params[1], params[2], params[3], paramWidgetID);
        zEntEvent(NULL, sceneObject, event, params, zSceneFindObject(paramWidgetID), paramWidgetID);
    }
    else
    {
        xDebugLinkPrintf("Unable to find widget (0x%X)\n", widgetID);
        return eUnableToFindAsset;
    }

    return eSuccess;
}

eError xDebugLink::Connect(char** argv, S32 argc, void*)
{
    mConnected = true;
    if (argc > 0)
    {
        xDebugLinkPrintf("Debug Commands will be accepted from [%s]\n", argv[0]);
        strcpy(mConnectedComputer, argv[0]);
    }

    return eSuccess;
}

eError xDebugLink::Disconnect(char** argv, S32 argc, void*)
{
    mConnected = false;
    if (argc > 0)
    {
        xDebugLinkPrintf("Disconnected from [%s]\n", argv[0]);
    }

    memset(mConnectedComputer, 0, sizeof(mConnectedComputer));
    mNumCmd = 0;
    return eSuccess;
}

S32 xDebugLink::CmdToArgv(char* str, char** argv, S32 maxargs)
{
    S32 argc = 0;
    S32 argcT = 0;
    char* strNil = str + strlen(str);
    xDebugLinkPrintf("[Args = %s]\n", str);

    while (argcT < maxargs)
    {
        while (*str && *str == ' ')
        {
            str++;
        }

        if (*str == '\0')
        {
            argv[argcT] = strNil;
            argcT++;
        }
        else
        {
            char chEnd = (*str == '"' || *str == '\'') ? *str++ : ' ';
            char* strArgEnd = str;

            while (*strArgEnd && *strArgEnd != chEnd)
            {
                strArgEnd++;
            }

            argv[argcT] = str;
            argc = ++argcT;

            str = *strArgEnd ? strArgEnd + 1 : strArgEnd;
            *strArgEnd = '\0';
        }
    }

    return argc;
}

char* xDebugLink::GetDevkitName() const
{
    return (char*)mDevkitName;
}

char* xDebugLink::GetConnectedComputerName() const
{
    return (char*)mConnectedComputer;
}

bool xDebugLink::IsConnected() const
{
    return mConnected;
}

DECOMP_FORCEACTIVE(DEBUG ? "\n" : 0, DEBUG ? "%.2X " : 0)

void xDebugLink::Update(F32)
{
    char command[12];
    char responseString[256];
    char* argv[10];
    char args[256];
    S32 i;
    S32 argc;
    bool knownCommand;
    eError result;
    U32 cmdPrefixLen = strlen("XCMD");

    mProcessingCmds = true;
    for (i = 0; i < mNumCmd; i++)
    {
        memset(command, 0, sizeof(command));
        strncpy(command, mCmdBuffer[i].command + cmdPrefixLen + 1, sizeof(command) - 1);
        xDebugLinkPrintf("Received Command = %s\n", command);

        knownCommand = false;
        strncpy(args, mCmdBuffer[i].command + cmdPrefixLen + 12, sizeof(args));
        argc = CmdToArgv(args, argv, 10);

        if (mConnected)
        {
            if (strcmpi(command, "__CONNECT__") == 0 && strlen(mConnectedComputer) != 0 &&
                strcmpi(argv[0], mConnectedComputer) != 0)
            {
                sprintf(responseString, "Already connected to [%s]. Can not accept connection from [%s]!",
                        mConnectedComputer, argv[0]);
                continue;
            }

            for (i = 0; i < sizeof(mCommandHandlerMap) / sizeof(mCommandHandlerMap[0]); i++)
            {
                if (strcmpi(mCommandHandlerMap[i].command, command) == 0)
                {
                    knownCommand = true;
                    if (strcmpi(mCommandHandlerMap[i].command, "__UPDTASS__") == 0)
                    {
                        if (mAssetDataReceivedSize != 0)
                        {
                            UpdateSceneObject(mBuffer);
                            mAssetDataReceivedSize = 0;
                        }
                    }
                    else
                    {
                        result = (this->*mCommandHandlerMap[i].handler)(argv, argc, mCmdBuffer[i].userData);
                        if (result == eSuccess || result == eReadyForBin)
                        {
                            sprintf(responseString, "Successfully performed command: %s", command);
                        }
                        else
                        {
                            sprintf(responseString, "Unable to perform command: %s", command);
                        }
                    }

                    break;
                }
            }

            if (!knownCommand)
            {
                if (strcmpi(command, "help") == 0)
                {
                }
            }
        }
        else
        {
            if (strcmpi(command, "__CONNECT__") == 0)
            {
                result = Connect(argv, argc, mCmdBuffer[i].userData);
            }
            else
            {
                mConnected = false;
            }
        }
    }

    mNumCmd = 0;
    mProcessingCmds = false;
}

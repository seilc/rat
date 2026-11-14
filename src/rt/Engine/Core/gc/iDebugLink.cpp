#include "xCommon.h"
#include "iDebugLink.h"

#include "xGlobals.h"
#include "xstransvc.h"
#include "xEvent.h"
#include "zScene.h"

#include <string.h>

static s32 exiChan;
u32 mailbox;

BOOL hioEnumCallback(s32 chan)
{
    OSReport("USBAdapter EXI.%d\n", chan);
    exiChan = chan;
    return FALSE;
}

void hioCallback()
{
    procReadMailbox();
}

void procReadMailbox()
{
    if (!HIOReadMailbox(&mailbox)) {
        OSReport("HIOReadMailbox failed\n");
    }
}

void iDebugLink::Allocate()
{
    xglobals->debugLink = new iDebugLink;
    xglobals->debugLink->Initialize();
}

eError iDebugLink::Initialize()
{
    HIOEnumDevices(hioEnumCallback);
    if (!HIOInit(exiChan, hioCallback)) {
        return eUnableToInitialize;
    }

    procReadMailbox();

    mConnected = false;
    memset(mConnectedComputer, 0, sizeof(mConnectedComputer));

    strcpy(mCommandHandlerMap[0].command, "__CONNECT__"); mCommandHandlerMap[0].handler = &xDebugLink::Connect;
    strcpy(mCommandHandlerMap[1].command, "__DISCONN__"); mCommandHandlerMap[1].handler = &xDebugLink::Disconnect;
    strcpy(mCommandHandlerMap[2].command, "__UPDTASS__"); mCommandHandlerMap[2].handler = &xDebugLink::ReadyForAssetData;
    strcpy(mCommandHandlerMap[3].command, "__SNDEVNT__"); mCommandHandlerMap[3].handler = &xDebugLink::SendEntEvent;
    strcpy(mCommandHandlerMap[4].command, "__HRTBEAT__"); mCommandHandlerMap[4].handler = &xDebugLink::HeartBeatResponse;
    return eSuccess;
}

eError iDebugLink::ReadyForAssetData(char**, S32, void*)
{
    if (mConnected) {
        U8 hdrBuffer[32] ATTRIBUTE_ALIGN(32);

        if (!HIORead(0x500, hdrBuffer, sizeof(hdrBuffer))) {
            OSHalt_L("HIORead failed.\n", 91);
        } else {
            DCInvalidateRange(hdrBuffer, sizeof(hdrBuffer));
            mAssetToUpdate = ((U32*)hdrBuffer)[0];
            U32 assetSize = ((U32*)hdrBuffer)[1];
            const char* assetName = xSTAssetName(mAssetToUpdate);
            mAssetDataReceivedSize = assetSize;

            OSReport("\nUpdating asset %s (0x%X) size (%d)\n", assetName, mAssetToUpdate, assetSize);

            if (assetSize > 0x20000 - 0x508) {
                return eGenericError;
            }

            S32 roundedAssetSize = OSRoundUp32B(assetSize);

            U8* buffer = new U8[roundedAssetSize];
            if (!HIORead(0x508, buffer, roundedAssetSize)) {
                OSHalt_L("HIORead failed.\n", 112);
                delete buffer;
            } else {
                DCInvalidateRange(buffer, roundedAssetSize);
                if (roundedAssetSize > assetSize)
                    memset(buffer + assetSize, 0, roundedAssetSize - assetSize);
                eError retval = UpdateSceneObject((char*)buffer);
                delete buffer;
                return retval;
            }
        }

        return eSuccess;
    }

    return eNotConnected;
}

eError iDebugLink::SendEntEvent(char**, S32, void*)
{
    U8 hdrBuffer[32] ATTRIBUTE_ALIGN(32);
    F32 params[4];

    if (!HIORead(0x500, hdrBuffer, sizeof(hdrBuffer))) {
        OSHalt_L("HIORead failed.\n", 143);
        return eGenericError;
    }

    DCInvalidateRange(hdrBuffer, sizeof(hdrBuffer));

    U32 widgetID = ((U32*)hdrBuffer)[0];
    U32 event = ((U32*)hdrBuffer)[1];
    params[0] = ((F32*)hdrBuffer)[2];
    params[1] = ((F32*)hdrBuffer)[3];
    params[2] = ((F32*)hdrBuffer)[4];
    params[3] = ((F32*)hdrBuffer)[5];
    U32 paramWidgetID = ((U32*)hdrBuffer)[6];

    xBase* sceneObject = zSceneFindObject(widgetID);
    if (sceneObject) {
        OSReport("Sending Event (%d) event to %s (0x%X)\n", event, xSTAssetName(widgetID), widgetID);
        OSReport("Params: [ %8.4f %8.4f %8.4f %8.4f ] ParamWidgetID: [ 0x%X ]\n", params[0], params[1], params[2],
                 params[3], paramWidgetID);
        zEntEvent(NULL, sceneObject, event, params, NULL, paramWidgetID);
    } else {
        OSReport("Unable to find widget (0x%X)\n", widgetID);
        return eUnableToFindAsset;
    }

    return eSuccess;
}

void iDebugLink::Update(F32)
{
    if (!mailbox)
        return;

    if (mailbox == 3) {
        OSReport("UpdateAsset\n");
        xglobals->debugLink->ReadyForAssetData(NULL, 0, NULL);
    }
    else if (mailbox == 1) {
        OSReport("Connect\n");
        xglobals->debugLink->Connect(NULL, 0, NULL);
    }
    else if (mailbox == 5) {
        OSReport("SendEvent\n");
        xglobals->debugLink->SendEntEvent(NULL, 0, NULL);
    } else {
        OSReport("Received unknown message.(%08X)\n", mailbox);
    }

    mailbox = 0;
}

eError iDebugLink::HeartBeatResponse(char** argv, S32 argc, void*)
{
    if (argc > 0) {
        xDebugLinkPrintf("\nReceived Hearbeat from %s!", argv[0]);
    }

    return eSuccess;
}

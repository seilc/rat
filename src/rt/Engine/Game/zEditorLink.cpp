#include "zEditorLink.h"

#include "xDebug.h"
#include "xFile.h"
#include "xString.h"
#include "xstransvc.h"
#include "xTRC.h"
#include "zPad.h"

void zEditorLinkInit()
{
    xDebugModeAdd("DM_EDITORLINK", zEditorLinkInitCB);
}

void zEditorLinkRefresh()
{
    xFile file;
    iFileOpen("UPDATED.TXT", 0x3, &file);

    char asset_name[256];
    U32 asset_size = iFileGetSize(&file);
    if (asset_size != 0) {
        iFileRead(&file, asset_name, asset_size);
        iFileClose(&file);

        asset_name[asset_size] = '\0';

        U32 asset_id = xStrHash(asset_name);
        U32 asset_size;
        U32* ingame_asset = (U32*)xSTFindAsset(asset_id, &asset_size);

        char asset_filename[256];
        sprintf(asset_filename, "%d.ASS", asset_id);
        iFileLoad(asset_filename, ingame_asset, &asset_size, 0);
    } else {
        iFileClose(&file);
    }
}

void zEditorLinkInitCB()
{
    xprintf("Hit X to refresh data\n");
    
    if (zPad::GetDebugPad()->Pressed(0x10000)) {
        zEditorLinkRefresh();
    }
}

void zEditorLinkUpdate()
{
    S32 i = -1;
    xTRCPadGetPadPort(i, TRC_PadActive);

    if ((mPad[i].on & 0x7700) == 0x7700) {
        zEditorLinkRefresh();
    }
}

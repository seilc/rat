#include "xSubTitles.h"

#include "xSubtitlesAsset.h"
#include "xFont.h"
#include "xstransvc.h"
#include "xDebugTweak.h"
#include "xMemMgr.h"
#include "zVar.h"

#include <string.h>

namespace {

F32 subtitleFontWidth = 20.0f;
F32 subtitleFontHeight = 20.0f;
F32 subtitleSize;
xSubtitlesAsset* m_pCurrentAsset;
U32 m_uLastTimeKey;

}

void xSubTitlesInit()
{
    subtitleSize = 0.25f;

    switch (xSTGetLocalizationEnum()) {
    case eRegionJP:
        subtitleFontWidth = 30.0f;
        subtitleFontHeight = 30.0f;
        break;
    case eRegionKR:
        subtitleFontWidth = 25.0f;
        subtitleFontHeight = 25.0f;
        break;
    default:
        subtitleFontWidth = 20.0f;
        subtitleFontHeight = 20.0f;
        break;
    }

#ifdef DEBUGRELEASE
    xDebugAddTweak("FX|Subtitles|subtitleFontWidth", &subtitleFontWidth, 0.0f, 100.0f, NULL, NULL, 0);
    xDebugAddTweak("FX|Subtitles|subtitleFontHeight", &subtitleFontHeight, 0.0f, 100.0f, NULL, NULL, 0);
#endif
}

void* xSubTitlesRead(void*, U32 aid, void* indata, U32 insize, U32* outsize)
{
    xSubtitlesAsset* pSubTitleAsset = (xSubtitlesAsset*)xMEMALLOC(insize, 0, eMemMgrTag_Subtitle, aid, 88);
    memcpy(pSubTitleAsset, indata, insize);
    *outsize = insize;
    return pSubTitleAsset;
}

void xSubTitlesUnloaded(void* userdata, U32)
{
    xSubtitlesAsset* pSubTitleAsset = (xSubtitlesAsset*)userdata;
}

bool xSubTitlesRender(U32 aid, F32 fTime)
{
    if (aid == 0 || !zVarEntryCB_SubtitlesOn(NULL)) {
        return false;
    }

    if (!m_pCurrentAsset || m_pCurrentAsset->id != aid) {
        m_pCurrentAsset = (xSubtitlesAsset*)xSTFindAsset(aid, NULL);
        m_uLastTimeKey = 0;
    }

    if (!m_pCurrentAsset) {
        static U32 last_failed_aid = 0;
        if (aid != last_failed_aid) {
            iprintf("Unable to find subtitles asset id %X\n", aid);
            last_failed_aid = aid;
        }
        return false;
    }

    if (m_pCurrentAsset->m_uAssetNumLines == 0) {
        return false;
    }

    if (fTime > m_pCurrentAsset->m_paSubtitleLines[m_pCurrentAsset->m_uAssetNumLines-1].fStopTime ||
        fTime < m_pCurrentAsset->m_paSubtitleLines[0].fStartTime) {
        return false;
    }

    U32 uSearchIndex = 0;
    if (fTime > m_pCurrentAsset->m_paSubtitleLines[m_uLastTimeKey].fStartTime) {
        uSearchIndex = m_uLastTimeKey;
    }

    while (uSearchIndex < m_pCurrentAsset->m_uAssetNumLines) {
        xASSERT(135, fTime < m_pCurrentAsset->m_paSubtitleLines[m_pCurrentAsset->m_uAssetNumLines-1].fStopTime);

        if (fTime >= m_pCurrentAsset->m_paSubtitleLines[uSearchIndex].fStartTime &&
            fTime < m_pCurrentAsset->m_paSubtitleLines[uSearchIndex].fStopTime) {
            m_uLastTimeKey = uSearchIndex;
        } else if (fTime >= m_pCurrentAsset->m_paSubtitleLines[uSearchIndex].fStopTime &&
                   fTime < m_pCurrentAsset->m_paSubtitleLines[uSearchIndex+1].fStartTime) {
            return false;
        }

        uSearchIndex++;
    }

    char* pszDisplay = (char*)(m_pCurrentAsset->m_paSubtitleLines + m_pCurrentAsset->m_uAssetNumLines);

    if (m_uLastTimeKey >= m_pCurrentAsset->m_uAssetNumLines) {
        return false;
    }

    pszDisplay += m_pCurrentAsset->m_paSubtitleLines[m_uLastTimeKey].uStringOffset;

    xColor cText = { 184, 196, 200, 255 };
    xColor cBackGnd = { 16, 16, 16, 188 };

    xtextbox tbSubtitles =
        xtextbox::create(
            xfont::create(
                0,
                NSCREENX(subtitleFontWidth), NSCREENY(subtitleFontHeight), 0.0f,
                cText,
                screen_bounds,
                g_BLACK, 2.0f, 2.0f),
            screen_bounds,
            0x2,
            0.0f, 0.0f, 0.0f, 0.0f);

    tbSubtitles.set_text(pszDisplay);
    tbSubtitles.bounds.x = 0.05f;
    tbSubtitles.bounds.w = 0.9f;
    tbSubtitles.bounds.h = subtitleSize;
    tbSubtitles.bounds.y = 1.0f - tbSubtitles.bounds.h;
    tbSubtitles.font.clip = tbSubtitles.bounds;
    tbSubtitles.render(true);

    return true;
}

void xSubTitlesSetSize(F32 size)
{
    subtitleSize = size;
}

F32 xSubTitlesGetSize()
{
    return subtitleSize;
}

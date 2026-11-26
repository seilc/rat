#include "xCommon.h"
#include "zUICustomScrollers.h"

#include "containers.h"
#include "xDebugTweak.h"
#include "xMath.h"
#include "xPad.h"
#include "xString.h"
#include "xstransvc.h"
#include "zEvent.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zUICustomLevelManager.h"
#include "zUIMotion.h"
#include "zUIText.h"

#include <stdio.h>
#include <string.h>

xTWEAKABLE S32 NUMBER_SCROLLER_ELEMENTS_PER_LINE = 12;
xTWEAKABLE F32 NUMBER_SCROLLER_SWITCH_TIME = 0.4f;
xTWEAKABLE F32 DESCRIPTION_SCROLLER_SPEED = 200.0f;
xTWEAKABLE S32 DESCRIPTION_SCROLLER_FADE_CHARS = 6;
xTWEAKABLE F32 MAP_SCROLLER_MAX_SPEED = 3.0f;
xTWEAKABLE F32 MAP_SCROLLER_ACCELERATION = 0.75f;
xTWEAKABLE F32 MAP_SCROLLER_SLOW_SPEED = 0.5f;
xTWEAKABLE F32 MAP_SCROLLER_SLOW_DISTANCE = 0.055f;

zUICustomDescriptionScroller::zUICustomDescriptionScroller(zUI* ui) : zUICustom(ui)
{
    xASSERT_L(28, ui->asset->type == xStrHash("ui:text"));
}

void zUICustomDescriptionScroller::Reset()
{
    currentTime = 0.0f;
    counter = NULL;
    if (ui->asset->customWidget) {
        xBase* b = zSceneFindObject(ui->asset->customWidget);
        if (b && b->baseType == eBaseTypeCounter)
            counter = (xCounter*)b;
    }

    baseText = ((zUIText*)ui)->GetText();
    buffer[0] = '\0';
    ((zUIText*)ui)->SetText(buffer);
    running = baseText != NULL;
}

namespace {

S32 descstrlen(const char* baseText)
{
    S32 ret = 0;
    bool inBlock = false;

    while (*baseText) {
        U32 ch = *baseText++;
        if (ch >= 0x80) {
            ch = (ch & 0x80) << 7;
            ch |= *baseText++ & 0x7F;
        }

        if (ch == '{')
            inBlock = true;
        else if (ch == '}')
            inBlock = false;
        else if (!inBlock)
            ret++;
    }
    return ret;
}

char* descstrncpy(char* dest, const char* source, S32 chars, char** outDest, const char** outSource)
{
    bool inBlock = false;

    while (*source && chars > 0) {
        U32 ch = *source;
        *dest++ = *source++;
        if (ch >= 0x80) {
            ch = (ch & 0x7F) << 7;
            ch |= *source & 0x7F;
            *dest++ = *source++;
        }
        if (ch == 0x3FFF)
            dest -= 2;

        if (ch == '{')
            inBlock = true;
        else if (!inBlock && ch != 0x3FFF)
            chars--;
        else if (ch == '}')
            inBlock = false;
    }
    *dest = '\0';
    if (outDest)
        *outDest = dest;
    if (outSource)
        *outSource = source;
    return dest;
}

} // namespace

void zUICustomDescriptionScroller::PreUpdate(F32 dt)
{
    if (running) {
        S32 numCharacters = currentTime * DESCRIPTION_SCROLLER_SPEED;
        S32 length = descstrlen(baseText);

        char* outDest = buffer;
        const S32 CHARS = DESCRIPTION_SCROLLER_FADE_CHARS;

        xRegion region = xSTGetLocalizationEnum();
        if ((region == eRegionKR || region == eRegionJP) && globals.demoType == zDT_NONE) {
            strcpy(outDest, "{w*1.3}{h*1.3}");
            outDest += strlen(outDest);
        }

        if (numCharacters >= length + CHARS) {
            strcpy(outDest, baseText);
            running = false;
            return;
        }

        const char* outSource = baseText;
        if (numCharacters > CHARS)
            descstrncpy(outDest, baseText, numCharacters - CHARS, &outDest, &outSource);

        S32 start = numCharacters > CHARS ? 0 : CHARS - numCharacters;
        for (S32 i = start; i < CHARS; i++) {
            const F32 alpha = 1.0f - 1.0f / CHARS;
            sprintf(outDest, "{a=%.2f}{sa=%.2f}", alpha, alpha);
            outDest += strlen(outDest);
            descstrncpy(outDest, outSource, 1, &outDest, &outSource);
        }
        strcpy(outDest, "{a=0}{sa=0}");

        xASSERT_L(159, strlen(buffer) < sizeof(buffer));

        currentTime += dt;
    }
}

bool zUICustomDescriptionScroller::HandleEvent(xBase*, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventSetText:
        currentTime = 0.0f;
        buffer[0] = '\0';
        U32 sceneCurrent = zUIIncrediblesGetSceneCurrent();

        S32 scene = counter == NULL ? sceneCurrent : counter->count;
        if (scene < 0 || scene >= (S32)zUIIncrediblesGetNumScenes())
            scene = 0;

        Scene* sceneCur = zUIIncrediblesGetScene(scene);
        baseText = sceneCur->levelDescription;
        running = baseText != NULL;
        return false;
    }
    return true;
}

#if DEBUG || RELEASE
void zUICustomDescriptionScroller::AddTweaks(const char* baseName)
{
    char buffer[128];

    sprintf(buffer, "%s|Text Scroll Speed", baseName);
    xTWEAK(buffer, &DESCRIPTION_SCROLLER_SPEED, 0.0f, 1000.0f, NULL, NULL, 0x2);
    sprintf(buffer, "%s|Text Scroll Extra Chars", baseName);
    xTWEAK(buffer, &DESCRIPTION_SCROLLER_FADE_CHARS, 0, 50, NULL, NULL, 0x2);
}
#endif

zUICustomMapScroller::zUICustomMapScroller(zUI* ui) : zUICustom(ui), counter(NULL)
{
    xASSERT_L(208, ui->asset->type == xStrHash("ui:image"));
}

void zUICustomMapScroller::Reset()
{
    counter = NULL;
    if (ui->asset->customWidget) {
        xBase* b = zSceneFindObject(ui->asset->customWidget);
        if (b && b->baseType == eBaseTypeCounter)
            counter = (xCounter*)b;
    }

    SignalEffect();
    currentX = destX;
    currentY = destY;
    currentSpeed = 0.0f;
    lastRunning = false;
}

bool zUICustomMapScroller::HandleEvent(xBase*, U32 toEvent, const F32*, xBase*, U32)
{
    switch (toEvent) {
    case eEventUISignalEffect:
        SignalEffect();
        return true;
    }

    return true;
}

void zUICustomMapScroller::PreUpdate(F32 dt)
{
    if (currentX != destX || currentY != destY) {
        if (!lastRunning) {
            ui->Signal(eEventUISignalMapStart);
            lastRunning = true;
        }

        F32 dx = destX - currentX, dy = destY - currentY;
        F32 distance = xsqrt(dx * dx + dy * dy);

        if (distance < MAP_SCROLLER_SLOW_DISTANCE) {
            currentSpeed -= MAP_SCROLLER_ACCELERATION * dt;
            if (currentSpeed < MAP_SCROLLER_SLOW_SPEED)
                currentSpeed = MAP_SCROLLER_SLOW_SPEED;
        } else {
            currentSpeed += MAP_SCROLLER_ACCELERATION * dt;
            if (currentSpeed > MAP_SCROLLER_MAX_SPEED)
                currentSpeed = MAP_SCROLLER_MAX_SPEED;
        }

        F32 maxMove = currentSpeed * dt;
        if (distance < maxMove) {
            currentX = destX;
            currentY = destY;
        } else {
            F32 angle = xatan2(dy, dx);
            currentX += maxMove * icos(angle);
            currentY += maxMove * isin(angle);

            xASSERT_L(279, currentX >= -5.0f && currentX <= 5.0f);
            xASSERT_L(280, currentY >= -5.0f && currentY <= 5.0f);
        }
    } else {
        if (lastRunning) {
            ui->Signal(eEventUISignalMapEnd);
            lastRunning = false;
        }
        currentSpeed = 0.0f;
    }
}

#if DEBUG || RELEASE
void zUICustomMapScroller::AddTweaks(const char* baseName)
{
    char buffer[128];

    sprintf(buffer, "%s|Scroll Acceleration", baseName);
    xTWEAK(buffer, &MAP_SCROLLER_ACCELERATION, 0.0f, 1000.0f, NULL, NULL, 0x2);
    sprintf(buffer, "%s|Max Scroll Speed", baseName);
    xTWEAK(buffer, &MAP_SCROLLER_MAX_SPEED, 0.0f, 1000.0f, NULL, NULL, 0x2);
    sprintf(buffer, "%s|Slow Scroll Speed", baseName);
    xTWEAK(buffer, &MAP_SCROLLER_SLOW_SPEED, 0.0f, 1000.0f, NULL, NULL, 0x2);
    sprintf(buffer, "%s|Slow Distance", baseName);
    xTWEAK(buffer, &MAP_SCROLLER_SLOW_DISTANCE, 0.0f, 1.0f, NULL, NULL, 0x2);
}
#endif

void zUICustomMapScroller::PreApplyMotionFrame(zUIMotionFrame* frame)
{
    frame->offsetU = currentX;
    frame->offsetV = currentY;
}

void zUICustomMapScroller::SignalEffect()
{
}

zUICustomNumberScroller::zUICustomNumberScroller(zUI* ui) : zUICustom(ui), current(0), timeLeft(0.0f)
{
    xASSERT_L(342, ui->asset->type == xStrHash("ui:text"));
}

void zUICustomNumberScroller::Reset()
{
    current = 0;
    timeLeft = 0.0f;
}

void zUICustomNumberScroller::PreUpdate(F32 dt)
{
    static S32 numbers[] = {
        120970, 111073, 70573, 12676, 10780, 10303, 11804, 30647, 72446, 91076,
        210779, 111473, 11234, 71336, 50278, 40881, 62282, 42879, 100579,
    };

    if (timeLeft <= 0.0f) {
        char* bufferIterator = buffer;
        for (S32 i = 0; i < NUMBER_SCROLLER_ELEMENTS_PER_LINE; i++) {
            sprintf(bufferIterator, "%s%06d", i == 0 ? "" : " ", numbers[(current + i) % (S32)ARRAY_LENGTH(numbers)]);
            bufferIterator += i == 0 ? 6 : 7;
        }

        if (++current >= (S32)ARRAY_LENGTH(numbers))
            current = 0;

        timeLeft = NUMBER_SCROLLER_SWITCH_TIME;
    } else
        timeLeft -= dt;

    zUIText* text = (zUIText*)ui;
    text->SetText(buffer);
}

#if DEBUG || RELEASE
void zUICustomNumberScroller::AddTweaks(const char* baseName)
{
    char buffer[128];

    sprintf(buffer, "%s|Scroll Switch Time", baseName);
    xTWEAK(buffer, &NUMBER_SCROLLER_SWITCH_TIME, 0.0001f, 1000.0f, NULL, NULL, 0x2);
}
#endif

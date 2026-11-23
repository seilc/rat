#include "xCommon.h"
#include "zHud.h"

#include "containers.h"
#include "xCam.h"
#include "xHud.h"
#include "xPtankPool.h"
#include "xserializer.h"
#include "zGame.h"
#include "zGlobals.h"
#include "zPlayer.h"
#include "zScene.h"

namespace {

void EventHandlerFunc(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                      U32 toParamWidgetID); // UNUSED

} // namespace

namespace zHud {

void hud_system::Startup()
{
}

void hud_system::Shutdown()
{
}

void hud_system::init()
{
    last_paused = false;
    hide_count = 0;
    xhud::init();

    for (hud_element* element = elements; element; element = element->next()) {
        element->init();
    }
}

void hud_system::update(F32 dt)
{
    if (!last_paused && zGameIsPaused()) {
        hide();
        last_paused = true;
    }
    else if (last_paused && !zGameIsPaused()) {
        show();
        last_paused = false;
    }

    for (hud_element* element = elements; element; element = element->next()) {
        element->update(dt);
    }

    xhud::update(dt);
}

void hud_system::render()
{
    if (globals.cmgr) {
        return;
    }

    xhud::render();

    for (hud_element* e = elements; e; e = e->_next) {
        if (globals.enableHUD)
            e->render();
    }
}

void hud_system::show()
{
    hide_count--;
    if (hide_count < 0) {
        hide_count = 0;
    }

    if (hide_count == 0) {
        for (hud_element* element = elements; element; element = element->next()) {
            element->show();
        }
    }
}

void hud_system::hide()
{
    hide_count++;
    if (hide_count == 1) {
        for (hud_element* element = elements; element; element = element->next()) {
            element->hide();
        }
    }
}

void hud_system::destroy()
{
    for (hud_element* element = elements; element; element = element->next()) {
        element->destroy();
    }
    elements = NULL;
    xhud::destroy();
}

hud_system& hud_system::instance()
{
    static hud_system _instance;
    return _instance;
}

#if DEBUG || RELEASE
void hud_system::add_tweaks()
{
    for (hud_element* element = elements; element; element = element->next()) {
        element->add_tweaks();
    }
}
#endif

} // namespace zHud

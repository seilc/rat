#ifndef ZHUD_H
#define ZHUD_H

#include "xBase.h"

namespace zHud {

class hud_element : public xBase
{
    friend class hud_system;

    hud_element* _next;

public:
    virtual void init(); // UNUSED
    inline virtual void setup(); // UNUSED
    inline virtual void update(F32); // UNUSED
    inline virtual void destroy(); // UNUSED
    virtual void show() = 0;
    virtual void hide() = 0;
    inline virtual void handle_event(xBase*, U32, const F32*, xBase*, U32); // UNUSED
#if DEBUG || RELEASE
    inline virtual void add_tweaks(); // UNUSED
#endif
    inline virtual void render(); // UNUSED

    hud_element* next()
    {
        return _next;
    }
};

class hud_system
{
    hud_element* elements;
    S8 hide_count;
    bool last_paused;

    hud_system() {}

public:
    static void Startup();
    static void Shutdown();
    void init();
    void update(F32 dt);
    void render();
    void show();
    void hide();
    void add(hud_element* element); // UNUSED
    void destroy();
    static hud_system& instance();

#if DEBUG || RELEASE
    virtual void add_tweaks();
#endif
};

} // namespace zHud

#endif

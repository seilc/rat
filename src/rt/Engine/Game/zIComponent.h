#ifndef ZICOMPONENT_H
#define ZICOMPONENT_H

#include "xBase.h"
#include "zCompParameters.h"

struct xScene;

template <class T>
class zIComponent
{
public:
    T* owner;

    virtual S32 Initialize();
    virtual void Attached() = 0;
    virtual void Detached() = 0;
#ifdef RELEASE
    virtual void Reset() = 0;
#endif
    virtual void AllAttached() = 0;
    virtual void SetParameters(const zCompParameters* parameters);
    virtual void Update(xScene* xscn, F32 dt) = 0;
    virtual void PreUpdate(xScene* xscn, F32 dt);
    virtual void PostUpdate(xScene* xscn, F32 dt);
    virtual void DebugRender();
    virtual bool SystemEvent(xBase* from, xBase* to, U32 to_event, const F32* to_param, xBase* to_param_widget, U32 to_param_widget_id);
};

#endif

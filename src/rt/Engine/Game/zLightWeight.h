#ifndef ZLIGHTWEIGHT_H
#define ZLIGHTWEIGHT_H

#include "xMemMgr.h"

class zLightweightSystemBase
{
public:
    virtual void SceneSetup() {}
    virtual void Reset() {}
    virtual void Update(F32) {}
    virtual void Render() {}
    virtual void PostRender2d() const {}
    virtual void PostRender3d() const {}
    virtual void Exit() {}
};

#define MAX_LIGHTWEIGHT_SYSTEMS 10

class zLightweightManager
{
public:
    static void Setup();
    static void Exit();
    static void Reset();
    static void Update(F32 dt);
    static void Render();
    static void PostRender2d();
    static void PostRender3d();
    static void SceneSetup();
    static void RegisterLightweight(zLightweightSystemBase* newSystem);

    static S32 systemCount;
    static zLightweightSystemBase* system[MAX_LIGHTWEIGHT_SYSTEMS];
};

template <class T, S32 size>
class zLightweightSystem : public zLightweightSystemBase
{
    T object[size];
    zLightweightSystem** system;
    U32 current_id;

public:
    zLightweightSystem(zLightweightSystem** system) : system(system)
    {
        current_id = size;
        zLightweightManager::RegisterLightweight(this);
        T::Setup();
    }

    void Exit()
    {
        for (S32 i = 0; i < size; i++) {
            if (object[i].IsUsed()) {
                object[i].Reset();
            }
            object[i].Destroy();
            object[i].SetUsed(false);
        }
        T::Exit();
        *system = NULL;
    }

    void Reset()
    {
        for (S32 i = 0; i < size; i++) {
            object[i].Reset();
            object[i].SetUsed(false);
        }
    }

    T* GetIndex(U32 index)
    {
        xASSERT_L(138, index<size);
        return &object[index];
    }

    void Update(F32 dt)
    {
        T::StaticUpdate(dt);
        for (S32 i = 0; i < size; i++) {
            if (object[i].IsUsed()) {
                object[i].Update(dt);
            }
        }
    }

    void Render()
    {
        T::RenderStart();
        for (S32 i = 0; i < size; i++) {
            if (object[i].IsUsed()) {
                object[i].Render();
            }
        }
        T::RenderEnd();
    }

    void PostRender2d() const
    {
        T::PostRenderStart();
        for (S32 i = 0; i < size; i++) {
            if (object[i].IsUsed()) {
                object[i].PostRender2d();
            }
        }
        T::PostRenderEnd();
    }

    void PostRender3d() const
    {
        T::PostRenderStart();
        for (S32 i = 0; i < size; i++) {
            if (object[i].IsUsed()) {
                object[i].PostRender3d();
            }
        }
        T::PostRenderEnd();
    }

    void SceneSetup()
    {
        T::SceneSetup();
    }
};

template <class T, S32 size>
class zLightweight
{
    bool used;
    U32 id;

public:
    static zLightweightSystem<T, size>*& GetSystem()
    {
        static zLightweightSystem<T, size>* system = NULL;
        if (system == NULL) {
            system = new (eMemStaticType_0, eMemMgrTag_Lightweight) zLightweightSystem<T, size>(&system);
        }
        return system;
    }

    zLightweight(bool used) : used(used) {}

    virtual void Render() const {}
    virtual void PostRender2d() const {}
    virtual void PostRender3d() const {}
    virtual void Update(F32) {}
    virtual void Destroy() {}
    virtual void Reset() {}

    static void StaticUpdate(F32) {}
    static void RenderStart() {}
    static void RenderEnd() {}
    static void PostRenderStart() {}
    static void PostRenderEnd() {}
    static void SceneSetup() {}
    static void Exit() { GetSystem() = NULL; }

    static T* GetIndex(U32 index)
    {
        return GetSystem()->GetIndex(index);
    }

    bool IsUsed() const { return used; }

    void SetUsed(bool value)
    {
        used = value;
    }
};

#endif

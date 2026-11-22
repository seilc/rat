#ifndef ZCOMPONENTPOOL_H
#define ZCOMPONENTPOOL_H

#include "xDebug.h"
#include "zCompParameters.h"

template <class T, class I, class O>
class zComponentPool
{
public:
    static void Initialize(S32 Size);
    static void DeInitialize();
    static I* AttachComponent(O* owner, zCompParameters* parameters);
    static S32 GetPoolSize();
    static S32 GetPoolUsage();
    static S32 GetMemUsage();
    static S32 GetMemUsagePerComponent();
    static zComponentPool* GetComponentPool();

private:
    zComponentPool();

    S32 FindNextUnused();

    S32 extraComponentMemUsage;
    S32 poolSize;
    T* components;
};

template <class T, class I, class O>
inline I* zComponentPool<T, I, O>::AttachComponent(O* owner, zCompParameters* parameters)
{
    xASSERT_L(DEBUG ? 101 : 96, owner);

    zComponentPool* componentPool = GetComponentPool();

    if (componentPool->components == NULL) {
        xFAIL_MSG_L(DEBUG ? 110 : 105, "This Component Pool is not Initialized. The pool has no data!");
        return NULL;
    }

    S32 nextFreeSpace = componentPool->FindNextUnused();
    if (nextFreeSpace == -1) {
        xFAIL_MSG_L(DEBUG ? 117 : 112, "There is no component free. Not enough space in the pool. Please increase the pool size.");
        return NULL;
    }

    I* CurComponent = &componentPool->components[nextFreeSpace];
    CurComponent->Attach(owner, parameters);
    return CurComponent;
}

template <class T, class I, class O>
inline zComponentPool<T, I, O>::zComponentPool()
{
    components = NULL;
    poolSize = 0;
    extraComponentMemUsage = 0;
}

template <class T, class I, class O>
inline zComponentPool<T, I, O>* zComponentPool<T, I, O>::GetComponentPool()
{
    static zComponentPool _componentPool;
    return &_componentPool;
}

template <class T, class I, class O>
inline S32 zComponentPool<T, I, O>::FindNextUnused()
{
    for (S32 i = 0; i < poolSize; i++) {
        if (components[i].owner == NULL) {
            return i;
        }
    }
    return -1;
}

#endif

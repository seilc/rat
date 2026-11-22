#include "xCommon.h"
#include "zLightWeight.h"

S32 zLightweightManager::systemCount;
zLightweightSystemBase* zLightweightManager::system[MAX_LIGHTWEIGHT_SYSTEMS];

void zLightweightManager::Setup()
{
    systemCount = 0;
}

void zLightweightManager::Exit()
{
    for (S32 i = 0; i < systemCount; i++)
        system[i]->Exit();
}

void zLightweightManager::Reset()
{
    for (S32 i = 0; i < systemCount; i++)
        system[i]->Reset();
}

void zLightweightManager::Update(F32 dt)
{
    for (S32 i = 0; i < systemCount; i++)
        system[i]->Update(dt);
}

void zLightweightManager::Render()
{
    for (S32 i = 0; i < systemCount; i++)
        system[i]->Render();
}

void zLightweightManager::PostRender2d()
{
    for (S32 i = 0; i < systemCount; i++)
        system[i]->PostRender2d();
}

void zLightweightManager::PostRender3d()
{
    for (S32 i = 0; i < systemCount; i++)
        system[i]->PostRender3d();
}

void zLightweightManager::SceneSetup()
{
    for (S32 i = 0; i < systemCount; i++)
        system[i]->SceneSetup();
}

void zLightweightManager::RegisterLightweight(zLightweightSystemBase* newSystem)
{
    xASSERT_FMT_L(62, systemCount < MAX_LIGHTWEIGHT_SYSTEMS, "Too many lightweight systems.  Increase lightweight limit.");
    system[systemCount] = newSystem;
    systemCount++;
}

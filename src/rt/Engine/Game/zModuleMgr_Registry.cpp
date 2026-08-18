#include "zModuleMgr_Registry.h"

#if DEBUG
#include "zModuleExample.h"
#endif

void zModuleMgr_Registry_Startup()
{
#if DEBUG
    ModuleExample::zModuleExample_Startup();
#endif
}

void zModuleMgr_Registry_Shutdown()
{
#if DEBUG
    ModuleExample::zModuleExample_Shutdown();
#endif
}

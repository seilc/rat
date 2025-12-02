#include "zModuleMgr_Registry.h"

#ifdef DEBUG
#include "zModuleExample.h"
#endif

void zModuleMgr_Registry_Startup()
{
#ifdef DEBUG
    ModuleExample::zModuleExample_Startup();
#endif
}

void zModuleMgr_Registry_Shutdown()
{
#ifdef DEBUG
    ModuleExample::zModuleExample_Shutdown();
#endif
}

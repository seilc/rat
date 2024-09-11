#ifndef __OS_H__
#define __OS_H__

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_CACHED_REGION_PREFIX 0x8000
#define OS_UNCACHED_REGION_PREFIX 0xC000
#define OS_PHYSICAL_MASK 0x3FFF

#define OS_BASE_CACHED (OS_CACHED_REGION_PREFIX << 16)
#define OS_BASE_UNCACHED (OS_UNCACHED_REGION_PREFIX << 16)

typedef s64 OSTime;
typedef u32 OSTick;

u32 __OSBusClock AT_ADDRESS(OS_BASE_CACHED | 0x00F8);
u32 __OSCoreClock AT_ADDRESS(OS_BASE_CACHED | 0x00FC);
#define OS_BUS_CLOCK __OSBusClock
#define OS_CORE_CLOCK __OSCoreClock
#define OS_TIMER_CLOCK (OS_BUS_CLOCK/4)

#define OSSecondsToTicks(sec) ((sec) * OS_TIMER_CLOCK)

void OSReport(const char* msg, ...);

#ifdef __cplusplus
}
#endif

#endif

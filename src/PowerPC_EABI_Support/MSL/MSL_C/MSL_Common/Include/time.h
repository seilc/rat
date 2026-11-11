#ifndef _MSL_TIME_H
#define _MSL_TIME_H

#include <size_t.h>

typedef long time_t;

#ifdef __cplusplus
extern "C" {
#endif

extern char* ctime(const time_t*);

#ifdef __cplusplus
}
#endif

#endif

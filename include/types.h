#ifndef TYPES_H
#define TYPES_H

#if defined(DEBUG) || defined(RELEASE)
#define DEBUGRELEASE
#endif

#ifdef S8
#undef S8
#endif

#ifdef S16
#undef S16
#endif

#ifdef S32
#undef S32
#endif

#ifdef S64
#undef S64
#endif

#ifdef U8
#undef U8
#endif

#ifdef U16
#undef U16
#endif

#ifdef U32
#undef U32
#endif

#ifdef U64
#undef U64
#endif

#ifdef F32
#undef F32
#endif

#ifdef F64
#undef F64
#endif

typedef signed char S8;
typedef signed short S16;
typedef signed int S32;
typedef signed long long S64;
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef float F32;
typedef double F64;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

#ifndef NULL
#define NULL 0L
#endif

#ifdef __MWERKS__
#define ALIGN(a) __attribute__ ((aligned(a)))
#else
#define ALIGN(a)
#endif

#define FORCEENUMSIZEINT ((S32)((~((U32)0))>>1))

#ifdef DEBUG
#define __DATE__ "Jan 11 2006"
#define __TIME__ "00:29:14"
#else
#define __DATE__ "Jan 18 2006"
#define __TIME__ "15:52:40"
#endif

#endif

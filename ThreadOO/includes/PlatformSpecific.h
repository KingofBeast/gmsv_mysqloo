#ifndef _PLATFORM_SPECIFIC_H_
#define _PLATFORM_SPECIFIC_H_

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif LINUX
#include <pthread.h>
#else
#error Unhandled Platform!
#endif

#endif //_PLATFORM_SPECIFIC_H_
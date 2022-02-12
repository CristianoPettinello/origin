#ifndef __UTILS_GLOBAL_H__
#define __UTILS_GLOBAL_H__

#ifdef UNIX
#define UTILS_LIBRARYSHARED_EXPORT
#else
#if defined(UTILS_LIBRARY)
#define UTILS_LIBRARYSHARED_EXPORT __declspec(dllexport)
#else
#define UTILS_LIBRARYSHARED_EXPORT __declspec(dllimport)
#endif
#endif

#endif // __UTILS_GLOBAL_H__
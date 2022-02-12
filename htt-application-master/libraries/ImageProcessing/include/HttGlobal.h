#ifndef __HTT_GLOBAL_H__
#define __HTT_GLOBAL_H__

#ifdef UNIX
#define HTT_LIBRARYSHARED_EXPORT
#else
#if defined(HTT_LIBRARY)
#define HTT_LIBRARYSHARED_EXPORT __declspec(dllexport)
#else
#define HTT_LIBRARYSHARED_EXPORT __declspec(dllimport)
#endif
#endif

#endif // __HTT_GLOBAL_H__
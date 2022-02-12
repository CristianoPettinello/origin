//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2019   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __HTT_DRIVER_GLOBAL_H__
#define __HTT_DRIVER_GLOBAL_H__

#ifdef UNIX
#define USB_DRIVER_LIBRARYSHARED_EXPORT
#else
#if defined(USB_DRIVER_LIBRARY)
#define USB_DRIVER_LIBRARYSHARED_EXPORT __declspec(dllexport)
#else
#define USB_DRIVER_LIBRARYSHARED_EXPORT __declspec(dllimport)
#endif
#endif

#endif // __HTT_DRIVER_GLOBAL_H__

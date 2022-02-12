//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __TIMER_H__
#define __TIMER_H__

#include "HttGlobal.h"
#include <time.h>

namespace Nidek {
namespace Libraries {
namespace HTT {

class Timer
{
public:
    HTT_LIBRARYSHARED_EXPORT void start();
    HTT_LIBRARYSHARED_EXPORT float stop();

private:
    timespec timeDifference(timespec startTime, timespec stopTime);

private:
    timespec m_startTime;
};

} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __TIMER_H__

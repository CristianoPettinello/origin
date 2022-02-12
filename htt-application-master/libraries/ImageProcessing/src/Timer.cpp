//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "Timer.h"

#include <iostream>
#include <time.h>

using namespace Nidek::Libraries::HTT;
using namespace std;

#ifndef UNIX
#include <windows.h>
static int my_clock_gettime(int, struct timespec* tv)
{
    static int initialized = 0;
    static LARGE_INTEGER freq, startCount;
    static struct timespec tv_start;
    LARGE_INTEGER curCount;
    time_t sec_part;
    long nsec_part;

    if (!initialized)
    {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&startCount);
        timespec_get(&tv_start, TIME_UTC);
        initialized = 1;
    }

    QueryPerformanceCounter(&curCount);

    curCount.QuadPart -= startCount.QuadPart;
    sec_part = curCount.QuadPart / freq.QuadPart;
    nsec_part = (long)((curCount.QuadPart - (sec_part * freq.QuadPart)) * 1000000000UL / freq.QuadPart);

    tv->tv_sec = tv_start.tv_sec + sec_part;
    tv->tv_nsec = tv_start.tv_nsec + nsec_part;
    if (tv->tv_nsec >= 1000000000UL)
    {
        tv->tv_sec += 1;
        tv->tv_nsec -= 1000000000UL;
    }
    return 0;
}
#endif

void Timer::start()
{
#ifdef UNIX
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &m_startTime);
#else
    my_clock_gettime(0, &m_startTime);
#endif
}

float Timer::stop()
{
    timespec stopTime;
#ifdef UNIX
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stopTime);
    // cout << "Elapsed time: " << (float)timeDifference(m_startTime, stopTime).tv_sec +
    // (float)timeDifference(m_startTime, stopTime).tv_nsec / 1000000000 << " s" << endl;

    return (float)timeDifference(m_startTime, stopTime).tv_sec +
           (float)timeDifference(m_startTime, stopTime).tv_nsec / 1000000000;
#else
    my_clock_gettime(0, &stopTime);
    return (float)timeDifference(m_startTime, stopTime).tv_sec +
           (float)timeDifference(m_startTime, stopTime).tv_nsec / 1000000000;
#endif
}

timespec Timer::timeDifference(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else
    {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

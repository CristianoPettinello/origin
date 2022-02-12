//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "Log.h"
#ifdef UNIX
#include <log4cpp/PropertyConfigurator.hh>
#endif

using namespace std;
using namespace Nidek::Libraries::Utils;

shared_ptr<Log> Log::getInstance()
{
    static weak_ptr<Log> _instance;
    shared_ptr<Log> instance = _instance.lock();
    if (!instance)
    {
        instance.reset(new Log());
        _instance = instance;
    }

    return instance;
}

Log::Log()
#ifdef UNIX
    : logger(log4cpp::Category::getRoot())
#endif
{
}

void Log::setProprieties(string initFileName)
{
#ifdef UNIX
    log4cpp::PropertyConfigurator::configure(initFileName);
#endif
}

void Log::debug(string str)
{
#ifdef UNIX
    logger.debug(str);
#else
    cout << now() << " [DEBUG] " << str << endl;
#endif
}
void Log::info(string str)
{
#ifdef UNIX
    logger.info(str);
#else
    cout << now() << " [INFO ] " << str << endl;
#endif
}
void Log::warn(string str)
{
#ifdef UNIX
    logger.warn(str);
#else
    cout << now() << " [WARN ] " << str << endl;
#endif
}
void Log::error(const char* file, int line, string str)
{
#ifdef UNIX
    stringstream s;
    s << "[" << file << ":" << line << "] " << str;
    logger.error(s.str());
#else
    cout << now() << " [ERROR] " << str << endl;
#endif
}

#ifndef UNIX
#include <ctime>
#include <iostream>
string Log::now()
{
    time_t rawtime;
    struct tm timeinfo;
    char buffer[80];

    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);

    strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", &timeinfo);
    string str(buffer);
    return str;
}
#endif
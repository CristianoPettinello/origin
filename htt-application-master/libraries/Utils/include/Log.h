//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __LOG_H__
#define __LOG_H__

#ifdef UNIX
#include <log4cpp/Category.hh>
#else
#include <iostream>
#include <string>
#endif
#include <memory>

#include "UtilsGlobal.h"

using namespace std;

namespace Nidek {
namespace Libraries {
namespace Utils {
///
/// \brief Log class.
///
/// This class provides a logger component to manage the logs using log4cpp library
///
class Log
{
private:
    Log();

public:
#ifdef UNIX
    log4cpp::Category& logger; ///< Component used to manage the logs
#endif

public:
    /// Static access method.
    /// Will be created a new instance only the first time.
    ///
    /// \return A shared pointer to the unique instance of this class
    ///
    UTILS_LIBRARYSHARED_EXPORT static shared_ptr<Log> getInstance();

    /// Set proprieties using an external file
    ///
    /// \param [in] initFileName    Input file with the proprieties used to configure the logger
    ///
    UTILS_LIBRARYSHARED_EXPORT void setProprieties(string initFileName);

    UTILS_LIBRARYSHARED_EXPORT void debug(string str);
    UTILS_LIBRARYSHARED_EXPORT void info(string str);
    UTILS_LIBRARYSHARED_EXPORT void warn(string str);
    UTILS_LIBRARYSHARED_EXPORT void error(const char* file, int line, string str);

private:
#ifndef UNIX
    string now();
#endif
};
} // namespace Utils
} // namespace Libraries
} // namespace Nidek

#endif // __LOG_H__
//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "Log.h"
#include "UtilsGlobal.h"
#include "json/json.h"
#include <string>

using namespace std;
using namespace Nidek::Libraries::Utils;

namespace Nidek {
namespace Libraries {
namespace Utils {
///
/// \brief Settings class.
///
/// This class contains the settings of the algorithm
///
class Settings
{
private:
    shared_ptr<Log> log;
    Settings();

public:
    /// Static access method.
    /// Will be created a new instance only the first time.
    ///
    /// \return A shared pointer to the unique instance of this class
    ///
    UTILS_LIBRARYSHARED_EXPORT static shared_ptr<Settings> getInstance();

    UTILS_LIBRARYSHARED_EXPORT void loadSettings(const string& file);
    UTILS_LIBRARYSHARED_EXPORT void loadCalibration(const string& file);
    UTILS_LIBRARYSHARED_EXPORT void loadHwBoard(const string& file);

    /// Saves roots object to settings file
    ///
    /// \param [out] file    Json file where save setting
    ///
    UTILS_LIBRARYSHARED_EXPORT void saveSettings(string& file);
    UTILS_LIBRARYSHARED_EXPORT void saveCalibration(const string& file);

    /// Function used to visualize the settings
    ///
    UTILS_LIBRARYSHARED_EXPORT void printSettings();
    UTILS_LIBRARYSHARED_EXPORT void printCalibration();

    // Returns the Json object
    UTILS_LIBRARYSHARED_EXPORT Json::Value& getSettings();
    UTILS_LIBRARYSHARED_EXPORT Json::Value& getCalibration();
    UTILS_LIBRARYSHARED_EXPORT Json::Value& getHwBoard();

    // Returns the Json object as string
    UTILS_LIBRARYSHARED_EXPORT string getSettingsString();
    UTILS_LIBRARYSHARED_EXPORT string getCalibrationString();

    // Update Json object
    UTILS_LIBRARYSHARED_EXPORT void updateSettings(const Json::Value& value);
    UTILS_LIBRARYSHARED_EXPORT void updateCalibration(const Json::Value& value);

private:
    /// Loads setting from file and save into a Json object
    ///
    /// \param [in] file    Source Json file where load settings
    ///
    void load(const string& file, Json::Value* obj);

    /// Saves roots object to settings file
    ///
    /// \param [out] file    Json file where save setting
    ///
    void save(const string& file, const Json::Value& obj);

    /// Function used to visualize the settings
    ///
    void print(const Json::Value& obj);

    string getString(const Json::Value& obj);

protected:
    Json::Value root;
    Json::Value calibration;
    Json::Value hwBoard;
};
} // namespace Utils
} // namespace Libraries
} // namespace Nidek

#endif // __SETTINGS_H__
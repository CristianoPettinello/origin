//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "Settings.h"
#include "Utils.h"
#include <fstream>

using namespace Nidek::Libraries::Utils;

static const string tag = "[SETT] ";

shared_ptr<Settings> Settings::getInstance()
{
    static weak_ptr<Settings> _instance;
    shared_ptr<Settings> instance = _instance.lock();
    if (!instance)
    {
        instance.reset(new Settings());
        _instance = instance;
    }
    return instance;
}

Settings::Settings() : log(Log::getInstance())
{
}

void Settings::loadSettings(const string& file)
{
    // log->debug(tag + "Settings::loadSettings()");
    load(file, &root);
}

void Settings::loadCalibration(const string& file)
{
    // log->debug(tag + "Settings::loadCalibration()");
    load(file, &calibration);
}

void Settings::loadHwBoard(const string& file)
{
    load(file, &hwBoard);
}

void Settings::saveSettings(string& file)
{
    log->debug(tag + "Settings::saveSettings()");
    save(file, root);
}

void Settings::saveCalibration(const string& file)
{
    log->debug(tag + "Settings::saveCalibration()");
    save(file, calibration);
}

void Settings::printSettings()
{
    log->debug(tag + "Settings::printSettings()");
    print(root);
}

void Settings::printCalibration()
{
    log->debug(tag + "Settings::printCalibration()");
    print(calibration);
}

Json::Value& Settings::getSettings()
{
    return root;
}

Json::Value& Settings::getCalibration()
{
    return calibration;
}

Json::Value& Settings::getHwBoard()
{
    return hwBoard;
}

string Settings::getSettingsString()
{
    return getString(root);
}

string Settings::getCalibrationString()
{
    return getString(calibration);
}

void Settings::updateSettings(const Json::Value& value)
{
    root = value;
}

void Settings::updateCalibration(const Json::Value& value)
{
    calibration = value;
}

void Settings::load(const string& file, Json::Value* obj)
{
    ifstream ifs;
    ifs.open(file.c_str(), ifstream::in);
    M_Assert(ifs.good(), string("Missing " + file + " file").c_str());
    Json::CharReaderBuilder rbuilder;
    string errs;
    bool parsingSuccessful = Json::parseFromStream(rbuilder, ifs, obj, &errs);
    if (!parsingSuccessful)
    {
        // report to the user the failure and their locations in the document.
        stringstream str;
        str << tag << "Failed to parse configuration\n" << errs;
        log->error(__FILE__, __LINE__, str.str());
        return;
    }
}

void Settings::save(const string& file, const Json::Value& obj)
{
    std::ofstream ofs;
    ofs.open(file.c_str(), ifstream::out);
    Json::StreamWriterBuilder wbuilder;
    std::unique_ptr<Json::StreamWriter> writer(wbuilder.newStreamWriter());
    writer->write(obj, &ofs);
}

void Settings::print(const Json::Value& obj)
{
    Json::StreamWriterBuilder wbuilder;
    stringstream str;
    str << Json::writeString(wbuilder, obj) << endl;
    log->info(str.str());
}

string Settings::getString(const Json::Value& obj)
{
    Json::StreamWriterBuilder wbuilder;
    stringstream str;
    str << Json::writeString(wbuilder, obj) << endl;
    return str.str();
}

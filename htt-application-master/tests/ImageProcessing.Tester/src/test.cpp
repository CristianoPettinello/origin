#include "Htt.h"
#include "HttProcessType.h"
#include "Log.h"
#include "Settings.h"
#include "Statistics.h"
#include "Utils.h"
#include <fstream>  /* ifstream */
#include <iostream> /* cout */
#include <sstream>  /* stringstream */
#include <unistd.h> /* getopt */

using namespace std;
using namespace Nidek::Libraries::HTT;
using namespace Nidek::Libraries::Utils;

static const string tag = "[MAIN] ";

const string settingsFile = "./build/conf/settings.json";
const string calibrationFile = "./build/conf/calibration.json";
const string logProperties = "./build/conf/log4cpp.properties";
const string defaultFolderToProcess = "../../tests/ImageProcessing.Tester/images/";
const string defaultTestFolderToProcess = "../../tests/TestSets/";

void doHelp(char* app)
{
    stringstream s;
    s << "HTT Image Processing Library\n"
      << "Usage: " << app << " [OPTION] ...\n"
      << "Example: " << app << " -h\n"
      << "\n"
      << "  -h       Print out this help\n"
      << "\n"
      << " (default) Lens Fitting mode\n"
      << "  -C N     Calibration mode (0: grid, 1: frame)\n"
      << "\n"
      << "Optional parameters:\n"
      << "  -f PATH  Specifies the path to the folder containing the images to be processed         [current: "
      << defaultFolderToProcess << "]\n"
      << "  -F PATH  Specifies the path to the single image to be processed                         [only for lens "
         "fitting mode]\n";
    cout << s.str() << endl;
}

void checkSystemCalibrated()
{
    Json::Value st = Settings::getInstance()->getCalibration()["scheimpflug_transformation"];
    M_Assert(st != Json::objectValue, "System is not calibrated!");
}

string validateFilePath(string file, const string& defaultFile, shared_ptr<Log> log)
{
    ifstream f(file.c_str());
    if (file.empty())
    {
        log->debug(tag + "Missing file...");
        file = defaultFile;
    } else if (!f.good())
    {
        log->debug(tag + file + " not exist");
        file = defaultFile;
    }
    log->debug(tag + file + " will be used");
    return file;
}

int main(int argc, char* argv[])
{
    string folderToProcess;
    string filename;
    int calibrationType;

    HttProcessType httType = HttProcessType::lensFitting;

    int c;
    while ((c = getopt(argc, argv, "C:f:F:h")) != -1)
    {
        switch (c)
        {
        case 'C':
        {
            httType = HttProcessType::calibration;
            calibrationType = stoi(optarg);
            break;
        }
        case 'f':
        {
            folderToProcess = optarg;
            continue;
        }
        case 'F':
        {
            filename = optarg;
            continue;
        }
        case '?': // error
        case 'h': // help
        default:
            doHelp(argv[0]);
            return 1;
        }
    }

    // FIXME: log and settings are not exposed
    // Load log4cpp proprieties
    shared_ptr<Log> log = Log::getInstance();
    log->setProprieties(logProperties);

    // Load settings from file and create json object
    shared_ptr<Settings> settings = Settings::getInstance();
    settings->loadSettings(settingsFile);
    settings->loadCalibration(calibrationFile);

    switch (httType)
    {
    case HttProcessType::calibration:
    {
        log->info(tag + "Starting calibration procedure");
        if (calibrationType == HttCalibrationType::Grid)
        {
            shared_ptr<Htt> htt(new Htt());

            Json::Value calibration = settings->getCalibration()["scheimpflug_transformation_calibration"];
            string inputImage = calibration["grid"]["image"].asString();
            log->info(tag + "Load image " + inputImage);

            shared_ptr<Image<uint8_t>> img = loadImage((char*)inputImage.c_str());

            Htt::CalibrationGridResults result = htt->calibrateGrid(img);
            htt->printReturnCode();

            if (result.valid)
                settings->saveCalibration(calibrationFile);

        } else if (calibrationType == HttCalibrationType::Frame)
        {
            checkSystemCalibrated();
            shared_ptr<Htt> htt(new Htt());

            // Load the input calibration image
            Json::Value calibration = settings->getCalibration()["scheimpflug_transformation_calibration"];
            string currentCalibrationFrame = calibration["current"].asString();
            string inputImage = calibration["frames"][currentCalibrationFrame]["image"].asString();
            log->info(tag + "Load image " + inputImage);
            shared_ptr<Image<uint8_t>> img = loadImage((char*)inputImage.c_str());

            // Extract R and H from filename
            // i.e. Frame_15_LED=0600_R=-15_H=+15_05.bmp
            // R=-15 and H=15 (double values)
            double deltaN = 0, H = 0;
            string fName = inputImage.substr(inputImage.find_last_of('/') + 1);
            parseFilename(fName, deltaN, H);

            Htt::AccurateCalibrationResults res = htt->accurateCalibration(img, deltaN, H, fName);
            htt->printReturnCode();

            if (res.valid)
                settings->saveCalibration(calibrationFile);
        } else
        {
            M_Assert(false, "Calibration type not supported");
        }

        break;
    }
    case HttProcessType::lensFitting:
    {
        log->info(tag + "Starting lens fitting procedure");

        checkSystemCalibrated();
        shared_ptr<Htt> htt(new Htt());

        static Json::Value jsonSettings = settings->getSettings();
        static Json::Value testJson = jsonSettings["test"];
        int repetitions = testJson["repetitions"].asInt();
        shared_ptr<Statistics> stats(new Statistics(testJson["outputFolder"].asString(),
                                                    testJson["statisticsFilename"].asString(), repetitions));

        if (jsonSettings["debug"].asBool() || jsonSettings["optimization"]["debug"].asBool() ||
            testJson["storeImages"].asBool())
            log->warn(tag + "Image saving enabled, possible increase in execution time");

        if (filename != "")
        {
            // Load png image from inputImage path
            log->info(tag + "Load image " + filename);
            shared_ptr<Image<uint8_t>> img = loadImage((char*)filename.c_str());

            double deltaN = 0, H = 0;
            string fName = filename.substr(filename.find_last_of('/') + 1);
            parseFilename(fName, deltaN, H);

            Htt::LensFittingResults result =
                htt->lensFitting(img, deltaN, H, getModelType(jsonSettings["model"].asString()));
            htt->printReturnCode();
        } else
        {
            string folder = validateFilePath(folderToProcess, defaultFolderToProcess, log);
            htt->processFolder(folder, stats);
            stats->close();
        }

        log->info(tag + "Lens fitting procedure terminated");
        break;
    }
    default:
        break;
    }
    return 0;
}
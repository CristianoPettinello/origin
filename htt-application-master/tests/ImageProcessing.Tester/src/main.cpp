//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "Htt.h"
#include "HttPrivate.h"
#include "Statistics.h"
#include "Tester.h"
#include "Utils.h"
#include <fstream>  /* ifstream */
#include <unistd.h> /* getopt */

using namespace std;
using namespace Nidek::Libraries::HTT;
using namespace Nidek::Libraries::Utils;

static const string tag = "[MAIN] ";

const string defaultSettingsFile = "./build/conf/settings.json";
string defaultCalibrationFile = "./build/conf/calibration.json";
const string defaultLogProperties = "./build/conf/log4cpp.properties";
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
      << "  -M       Measure mode\n"
      << "  -V       Validation mode\n"
      << "  -T N     Testing mode (0: cross, 1: square, 2: repetitions, 3: grid calibration, 4: validation (?), 5: "
         "comparative), 6: synthetic images generation, 7: inverse transform from image to object plane\n"
      << "  -T 7 -F <PATH to csv file>   Inverse transform from image to object plane\n"
      << "\n"
      << "  -f PATH  Specify the path to the folder containing the images to be processed         [current: "
      << defaultFolderToProcess << "]\n"
      << "  -l PATH  Specify an alternative log4cpp configuration file to load                    [current: "
      << defaultLogProperties << "]\n"
      << "  -s PATH  Specify an alternative json file containing the settings params to load      [current: "
      << defaultSettingsFile << "]\n"
      << "  -c PATH  Specify an alternative json file containing the calibration params to load   [current: "
      << defaultCalibrationFile << "]\n";
    cout << s.str() << endl;
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

void checkSystemCalibrated()
{
    Json::Value st = Settings::getInstance()->getCalibration()["scheimpflug_transformation"];
    M_Assert(st != Json::objectValue, "System is not calibrated!");
}

int main(int argc, char* argv[])
{
    string logProperties = defaultLogProperties;
    string settingsFile;
    string calibrationFile;
    string folderToProcess;
    string inputFile;
    int testType;
    int calibrationType;

    HttProcessType httType = HttProcessType::lensFitting;

    // c: calibration, l: log4cpp.properties, s: settings.json, t: test, f: folder, v: validation
    int c;
    while ((c = getopt(argc, argv, "C:f:F:l:Ms:T:Vh")) != -1)
    {
        switch (c)
        {
        case 'c':
        {
            calibrationFile = optarg;
            continue;
        }
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
            inputFile = optarg;
            continue;
        }
        case 'l':
        {
            logProperties = optarg;
            continue;
        }
        case 'M':
        {
            httType = HttProcessType::measure;
            break;
        }
        case 's':
        {
            settingsFile = optarg;
            continue;
        }
        case 'T':
        {
            httType = HttProcessType::testing;
            testType = stoi(optarg);
            break;
        }
        case 'V':
        {
            httType = HttProcessType::validation;
            break;
        }
        case '?': // error
        case 'h': // help
        default:
            doHelp(argv[0]);
            return 1;
        }
    }

    // Load log4cpp proprieties
    shared_ptr<Log> log = Log::getInstance();
    log->setProprieties(logProperties);

    // Load settings from file and create json object
    shared_ptr<Settings> settings = Settings::getInstance();
    settings->loadSettings(validateFilePath(settingsFile, defaultSettingsFile, log));
    settings->loadCalibration(validateFilePath(calibrationFile, defaultCalibrationFile, log));

    switch (httType)
    {
    case HttProcessType::calibration:
    {
        log->info(tag + "Starting calibration procedure");
        if (calibrationType == HttCalibrationType::Grid)
        {
            shared_ptr<HttPrivate> htt(new HttPrivate());

            Json::Value calibration = settings->getCalibration()["scheimpflug_transformation_calibration"];
            string inputImage = calibration["grid"]["image"].asString();
            log->info(tag + "Load image " + inputImage);

            shared_ptr<Image<uint8_t>> img = loadImage((char*)inputImage.c_str());

            HttPrivate::CalibrationGridResults res = htt->calibrateGrid(img);
            if (res.valid)
            {
                // Save the Scheimpflug Transforms' parameters into a specific json file
                settings->saveCalibration(defaultCalibrationFile);
                // settings->print();

                string folder = settings->getSettings()["test"]["outputFolder"].asCString();
                ofstream outCsvFile(folder + "/gridObjPoints.csv");
                outCsvFile << "index,x [pixel],y [pixel]\n";
                for (int i = 0; i < res.imgPlaneIntersectionPoints.size(); ++i)
                {
                    outCsvFile << i << ", " << res.imgPlaneIntersectionPoints[i].first << ", "
                               << res.imgPlaneIntersectionPoints[i].second << "\n";
                }
                outCsvFile.close();
            }
            htt->printReturnCode();

        } else if (calibrationType == HttCalibrationType::Frame)
        {
            checkSystemCalibrated();
            shared_ptr<HttPrivate> htt(new HttPrivate());

            // Load the input calibration image
            Json::Value calibration = settings->getCalibration()["scheimpflug_transformation_calibration"];
            string currentCalibrationFrame = calibration["current"].asString();
            string inputImage = calibration["frames"][currentCalibrationFrame]["image"].asString();
            log->info(tag + "Load image " + inputImage);
            shared_ptr<Image<uint8_t>> img = loadImage((char*)inputImage.c_str());

            // Extract R and H from filename
            // i.e. Frame_15_LED=0600_R=-15_H=+15_05.bmp
            // R=-15 and H=15 (double values)
            double R = 0, H = 0;
            string filename = inputImage.substr(inputImage.find_last_of('/') + 1);
            parseFilename(filename, R, H);

            HttPrivate::processArgs args;
            args.R = R;
            args.H = H;
            args.inputImage = img;
            args.repetitions = 1;
            args.processType = HttProcessType::calibration;
            args.imageLabel = filename.substr(0, filename.find_last_of('.'));
            args.modelType = ModelType::MiniBevelModelType;

            // Enable Scheimpflug Transform calibration
            int maxRetries = settings->getSettings()["scheimpflug_transformation_calibration"]["maxRetries"].asInt();
            bool validCalibration = false;
            for (int count = 0; count < maxRetries; ++count)
            {
                shared_ptr<HttPrivate::ProcessResults> resProcess = htt->process(args);
                if (!resProcess->valid)
                {
                    log->debug(tag + "htt process failed");
                    continue;
                }

                vector<float> imgX;
                vector<float> imgY;
                for (int i = 0; i < (int)resProcess->optimizationOutput->bevel->imgX.size(); ++i)
                {
                    imgX.push_back((float)resProcess->optimizationOutput->bevel->imgX[i]);
                    imgY.push_back((float)resProcess->optimizationOutput->bevel->imgY[i]);
                }

                HttPrivate::OptimizeScheimpflugTransformResults res =
                    htt->optimizeScheimpflugTransform(img, imgX, imgY, R, H);
                if (res.valid)
                {
                    validCalibration = true;
                    break;
                }
            }

            if (validCalibration)
            {
                // Save the Scheimpflug Transforms' parameters into a specific json file
                settings->saveCalibration(defaultCalibrationFile);
                // settings->print();
            }
            htt->printReturnCode();
        } else if (calibrationType == HttCalibrationType::MultiFrames)
        {
            checkSystemCalibrated();
            shared_ptr<Htt> htt(new Htt());

            vector<shared_ptr<Image<uint8_t>>> inputImageSet;
            vector<double> deltaNSet;
            vector<double> HSet;
            vector<string> filenameSet;

            Json::Value calibration = settings->getCalibration()["scheimpflug_transformation_calibration"];
            string folder = calibration["multiFramesFolder"].asString();
            htt->initMultiFramesCalibration(folder, inputImageSet, deltaNSet, HSet, filenameSet);

            Htt::AccurateCalibrationResults results =
                htt->accurateCalibration(inputImageSet, deltaNSet, HSet, filenameSet);

            if (results.valid)
            {
                // Save the Scheimpflug Transforms' parameters into a specific json file
                settings->saveCalibration(defaultCalibrationFile);
                // settings->print();
            }

            htt->printReturnCode();

        } else
        {
            M_Assert(false, "Calibration type not supported");
        }

        break;
    }
    case HttProcessType::validation:
    {
        log->info(tag + "Starting validation procedure");

        checkSystemCalibrated();

        shared_ptr<Tester> tester(new Tester(defaultSettingsFile));

        string folder = validateFilePath(folderToProcess, defaultTestFolderToProcess, log);

        // TestSet_08: (9 Images) M801 (MiniBevelModel) with LED = 0010 and different angles (H and DeltaN)
        tester->validationTests(folder, "TestSet_08", "LTT12-M801_3_KIT_1", 1);
        tester->validationTests(folder, "TestSet_08", "LTT12-M801_3_KIT_1", 51);

        // TestSet_08: (21 Images) M804 (MiniBevelModel) with LED = 0010 and different angles (H and DeltaN)
        tester->validationTests(folder, "TestSet_09", "LTT12-M804_3_KIT_1", 1);
        tester->validationTests(folder, "TestSet_09", "LTT12-M804_3_KIT_1", 51);
        break;
    }
    case HttProcessType::lensFitting:
    {
        log->info(tag + "Starting lens fitting procedure");

        checkSystemCalibrated();

        shared_ptr<HttPrivate> htt(new HttPrivate());

        static Json::Value jsonSettings = settings->getSettings();
        static Json::Value testJson = jsonSettings["test"];
        int repetitions = testJson["repetitions"].asInt();
        shared_ptr<Statistics> stats(new Statistics(testJson["outputFolder"].asString(),
                                                    testJson["statisticsFilename"].asString(), repetitions));

        if (jsonSettings["debug"].asBool() || jsonSettings["optimization"]["debug"].asBool() ||
            testJson["storeImages"].asBool())
            log->warn(tag + "Image saving enabled, possible increase in execution time");

        string folder = validateFilePath(folderToProcess, defaultFolderToProcess, log);
        htt->processFolder(folder, stats);
        stats->close();

        log->info(tag + "Lens fitting procedure terminated");
        break;
    }
    case HttProcessType::testing:
    {
        log->info(tag + "Starting test procedure");
        Tester::TestType type = (Tester::TestType)testType;

        shared_ptr<Tester> tester(new Tester(defaultSettingsFile));

        switch (type)
        {
        case Tester::TestType::Cross:
        case Tester::TestType::Square:
        {
            checkSystemCalibrated();
            tester->testScheimpflug(type);
        }
        break;
        case Tester::TestType::Repetitions:
        {
            checkSystemCalibrated();
            string folder = validateFilePath(folderToProcess, defaultTestFolderToProcess, log);

            /**
             * Discarded images with the led intensity lower than L0010
             */
            // TestSet_01: (7 Images) Frame 09 (T-Model) with different LED intensities
            tester->repetitionTests(folder, "TestSet_01", "T_BEVEL");

            // TestSet_02: (7 Images) M805 from Kit1 (T-Model) with different LED intensities
            tester->repetitionTests(folder, "TestSet_02", "T_BEVEL");

            // TestSet_03: (9 Images) Frame 07 (Plastic frame, MiniBevel) with different LED intensities, H = 7.0°,
            // DeltaN = 0.0°
            tester->repetitionTests(folder, "TestSet_03", "MINI_BEVEL");

            // TestSet_04: (145 Images) M801 from Kit1 (MiniBevel) with different LED intensities and angles
            tester->repetitionTests(folder, "TestSet_04", "MINI_BEVEL");

            // TestSet_05: (86) M808 from Kit 1 (Custom Bevel) with different LED intensities and angles
            tester->repetitionTests(folder, "TestSet_05", "CUSTOM_BEVEL");

            // TestSet_06: (84 Images) M809 from Kit 1 (Custom Bevel) with different LED intensities and angles
            tester->repetitionTests(folder, "TestSet_06", "CUSTOM_BEVEL");

            // TestSet_07: (10 Images) MiniBevel Frames: Frame 01, Frame 03, Frame 06, Frame 07, Frame 19, M801, M804
            tester->repetitionTests(folder, "TestSet_07", "MINI_BEVEL");

            // TestSet_08: (9 Images) M801 (MiniBevelModel) with LED = 0010 and different angles (H and DeltaN)
            tester->repetitionTests(folder, "TestSet_08", "MINI_BEVEL");

            // TestSet_09: (21 Images) M804 (MiniBevelModel) with LED = 0010 and different angles (H and DeltaN)
            tester->repetitionTests(folder, "TestSet_09", "MINI_BEVEL");

            // TestSet_10: (10 Images) Frame 01 (MiniBevel?) with different LED intensities, H = 7.0°, DeltaN = 0.0°
            tester->repetitionTests(folder, "TestSet_10", "MINI_BEVEL");

            // TestSet_11: (7 Images) Frame 03 (MiniBevel) with different LED intensities, H = 7.0°, DeltaN = 0.0°
            tester->repetitionTests(folder, "TestSet_11", "MINI_BEVEL");

            // TestSet_12: (11 Images) Frame 06 (MiniBevel) with different LED intensities, H = 7.0°, DeltaN =
            // 0.0, 3.0°
            tester->repetitionTests(folder, "TestSet_12", "MINI_BEVEL");

            // TestSet_13: (6 Images) Frame 19 (MiniBevel) with different LED intensities, H = 7.0°, DeltaN = 0.0°
            tester->repetitionTests(folder, "TestSet_13", "MINI_BEVEL");
        }
        break;
        case Tester::TestType::GridCalibration:
        {
            tester->calibrationGrid();
        }
        break;
        case Tester::TestType::Comparative:
        {
            Json::Value jsonCalibration = settings->getCalibration();

            // Replace current "scheimpflug_transformation" object with nominal value
            jsonCalibration["scheimpflug_transformation_bkp"] = jsonCalibration["scheimpflug_transformation"];
            jsonCalibration["scheimpflug_transformation"] = jsonCalibration["nominal_scheimpflug_transformation"];
            settings->updateCalibration(jsonCalibration);
            settings->saveCalibration(defaultCalibrationFile);

            tester->comparativeTest();

            jsonCalibration = settings->getCalibration();
            jsonCalibration["scheimpflug_transformation"] = jsonCalibration["scheimpflug_transformation_bkp"];
            jsonCalibration.removeMember("scheimpflug_transformation_bkp");
            settings->updateCalibration(jsonCalibration);
            settings->saveCalibration(defaultCalibrationFile);
        }
        break;
        case Tester::TestType::SyntheticImagesGeneration:
        {
            tester->generateSyntheticImages();
            break;
        }
        case Tester::TestType::InversTransform:
        {
            if (inputFile.empty())
            {
                log->warn(tag + "Missing CSV input file");
                break;
            }

            tester->inverseTransform(inputFile);
            break;
        }
        default:
            break;
        }
        break;
    }
    case HttProcessType::measure:
    {
        log->info(tag + "Starting measure procedure");

        // checkSystemCalibrated();

        // Json::Value jsonCalibration = settings->getCalibration();
        // // Replace current "scheimpflug_transformation" object with nominal value
        // jsonCalibration["scheimpflug_transformation_bkp"] = jsonCalibration["scheimpflug_transformation"];
        // jsonCalibration["scheimpflug_transformation"] = jsonCalibration["nominal_scheimpflug_transformation"];
        // settings->updateCalibration(jsonCalibration);
        // settings->saveCalibration(defaultCalibrationFile);

        shared_ptr<HttPrivate> htt(new HttPrivate());

        static Json::Value jsonSettings = settings->getSettings();
        static Json::Value testJson = jsonSettings["test"];
        int repetitions = testJson["repetitions"].asInt();
        shared_ptr<Statistics> stats(new Statistics(testJson["outputFolder"].asString(),
                                                    testJson["statisticsFilename"].asString(), repetitions));

        if (jsonSettings["debug"].asBool() || jsonSettings["optimization"]["debug"].asBool() ||
            testJson["storeImages"].asBool())
            log->warn(tag + "Image saving enabled, possible increase in execution time");

        string folder = validateFilePath(folderToProcess, defaultFolderToProcess, log);
        // htt->processFolder(folder, stats, false, true);  // Used to measure synthetic images
        htt->processFolder(folder, stats, false, false);
        stats->close();

        // // Replace current "scheimpflug_transformation" object with nominal value
        // jsonCalibration = settings->getCalibration();
        // jsonCalibration["scheimpflug_transformation"] = jsonCalibration["scheimpflug_transformation_bkp"];
        // jsonCalibration.removeMember("scheimpflug_transformation_bkp");
        // settings->updateCalibration(jsonCalibration);
        // settings->saveCalibration(defaultCalibrationFile);

        log->info(tag + "Measure procedure terminated");
        break;
    }
    default:
        break;
    }
    return 0;
}

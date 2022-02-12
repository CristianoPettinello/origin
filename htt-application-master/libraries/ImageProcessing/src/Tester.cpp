#include "Tester.h"
#include "HttPrivate.h"
#include "Image.h"
#include "ScheimpflugTransform.h"
#include "Utils.h"
#include <fstream>
#include <iomanip>
#include <math.h>

using namespace Nidek::Libraries::HTT;
using namespace std;

static const string tag = "[TEST] ";

Tester::Tester(string settingsFile)
    : m_settingsFile(settingsFile),
      m_settings(Settings::getInstance()),
      m_log(Log::getInstance())
{
}

void Tester::testScheimpflug(TestType type, double R /* = 0 */, double H /* = 0 */)
{
    Json::Value jsonSettings = m_settings->getSettings();
    Json::Value jsonCalibration = m_settings->getCalibration();
    int width = jsonSettings["image"]["width"].asInt();
    int height = jsonSettings["image"]["height"].asInt();
    int halfWidth = width / 2;
    int halfHeight = height / 2;

    shared_ptr<Image<uint8_t>> imgPlane(new Image<uint8_t>(width, height, 1));
    shared_ptr<Image<uint8_t>> objPlane(new Image<uint8_t>(width, height, 1));

    uint16_t stride = imgPlane->getStride();

    vector<float> objX; // pixel
    vector<float> objY;
    vector<int> imgX;
    vector<int> imgY;
    vector<int> newObjX;
    vector<int> newObjY;

    shared_ptr<ScheimpflugTransform> st(new ScheimpflugTransform());
    st->init(jsonCalibration["scheimpflug_transformation"]);
    st->setStylusOrientation(R, H);

    int size;
    string label;
    string folder = jsonSettings["test"]["outputFolder"].asCString();

    switch (type)
    {
    case TestType::Cross:
    {
        label = "Cross";
        float xi = -0.5;              // mm
        float step = 1.0 / halfWidth; // mm
        int start = width / 4;
        int end = start + halfWidth;
        for (int i = start; i < end; ++i)
        {
            objX.push_back(xi);
            objY.push_back(0);
            objX.push_back(0);
            objY.push_back(xi);
            xi += step;
        }
    }
    break;
    case TestType::Square:
    {
        label = "Square";
        float xi = -1;                // mm
        float step = 2.0 / halfWidth; // mm
        int start = width / 4;
        int end = start + halfWidth;
        float startf = (start - halfWidth) * step;
        float endf = (end - halfWidth) * step;
        for (int i = start; i < end; ++i)
        {
            objX.push_back(xi);
            objY.push_back(startf);
            objX.push_back(xi);
            objY.push_back(endf);
            objX.push_back(startf);
            objY.push_back(xi);
            objX.push_back(endf);
            objY.push_back(xi);
            xi += step;
        }
    }
    break;
    default:
        break;
    }

    size = objX.size();
    imgX.resize(size);
    imgY.resize(size);

    // Transform the square from object to image plane
    for (int i = 0; i < (int)objX.size(); ++i)
    {
        double xMm = (double)objX[i];
        double yMm = (double)objY[i];
        double srcObjPoint[3] = {xMm, yMm, 0.0};
        double dstImgPoint[3] = {0.0, 0.0, 0.0};
        st->objectToImage(srcObjPoint, dstImgPoint);

        int xPixel = (int)dstImgPoint[0];
        int yPixel = (int)dstImgPoint[1];

        if (jsonSettings["debug"].asBool())
        {
            stringstream s;
            s << tag << "(" << objX[i] << "," << objY[i] << ") ---> (" << xPixel << "," << yPixel << ")";
            m_log->debug(s.str());
        }

        if (xPixel >= 0 && xPixel < width && yPixel >= 0 && yPixel < height)
        {
            imgX.push_back(xPixel);
            imgY.push_back(yPixel);
        }
    }

    size = imgX.size();

    uint8_t* imgPlanePtr = imgPlane->getData();
    for (auto i = 0; i < size; ++i)
    {
        int index = imgX[i] + imgY[i] * stride;
        imgPlanePtr[index] = 255;
    }
    saveImage(folder, label + "ImgPlane.png", imgPlane);

    newObjX.resize(size);
    newObjY.resize(size);

    uint8_t* objPlanePtr = objPlane->getData();
    for (auto i = 0; i < size; ++i)
    {
        double srcImgPoint[3] = {(double)imgX[i], (double)imgY[i], 0.0};
        double dstObjPoint[3] = {0.0, 0.0, 0.0};
        st->imageToObject(srcImgPoint, dstObjPoint);

        int xPixel = st->mmToPixel(dstObjPoint[0]);
        int yPixel = st->mmToPixel(dstObjPoint[1]);
        xPixel += halfWidth;
        yPixel += halfHeight;

        if (xPixel >= 0 && xPixel < width && yPixel >= 0 && yPixel < height)
        {
            int index = xPixel + yPixel * stride;
            objPlanePtr[index] = 255;
        }
    }
    saveImage(folder, label + "ObjPlane.png", objPlane);
}

static string now()
{
    char buffer[80];
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%d-%m-%Y_%H%M%S", timeinfo);
    string str(buffer);
    return str;
}

void Tester::repetitionTests(string path, string folder, string modelType)
{
    shared_ptr<HttPrivate> htt(new HttPrivate());

    Json::Value jsonSettings = m_settings->getSettings();
    Json::Value jsonCalibration = m_settings->getCalibration();
    Json::Value testJson = jsonSettings["test"];
    int repetitions = testJson["repetitions"].asInt();

    string outFolder = testJson["outputFolder"].asString() + "/" + folder + "_" + now();
    shared_ptr<Statistics> stats(new Statistics(outFolder, testJson["statisticsFilename"].asString(), repetitions));

    // Update setting json file
    testJson["preOutputFolder"] = testJson["outputFolder"].asString();
    testJson["outputFolder"] = outFolder;
    jsonSettings["test"] = testJson;
    jsonSettings["model"] = modelType;
    m_settings->updateSettings(jsonSettings);
    m_settings->saveSettings(m_settingsFile);

    shared_ptr<HttPrivate::ProcessFolderResults> res = htt->processFolder(path + folder + "/", stats);
    stats->close();

    stringstream s;
    s << tag << folder << " results:\n";
    s << "\t\t\tImages      : " << res->nImages << "\n";
    s << "\t\t\tSuccess     : " << res->counters[ReturnCode::LENSFITTING_SUCCESSFUL] << "\n";
    s << "\t\t\tMax Retries : " << res->counters[ReturnCode::LENSFITTING_FAILED_MAX_RETRIES_REACHED] << "\n";
    s << "\t\t\tIntersection: " << res->counters[ReturnCode::LENSFITTING_FAILED_BEVELFRAME_INTERSECTION] << "\n";
    s << "\t\t\tPSO boundary: " << res->counters[ReturnCode::LENSFITTING_FAILED_PSO_BOUNDARY_REACHED] << "\n";
    m_log->info(s.str());

    // Revert setting json file
    jsonSettings = m_settings->getSettings();
    testJson = jsonSettings["test"];
    testJson["outputFolder"] = testJson["preOutputFolder"].asString();
    testJson.removeMember("preOutputFolder");
    jsonSettings["test"] = testJson;
    m_settings->updateSettings(jsonSettings);
    m_settings->saveSettings(m_settingsFile);
}

void Tester::validationTests(string path, string folder, string frame, int repetitions)
{
    shared_ptr<HttPrivate> htt(new HttPrivate());

    Json::Value jsonSettings = m_settings->getSettings();
    Json::Value jsonCalibration = m_settings->getCalibration();
    Json::Value testJson = jsonSettings["test"];
    string outFolder =
        testJson["outputFolder"].asString() + "/" + folder + "_rep=" + to_string(repetitions) + "_" + now();

    // Update setting json file
    testJson["preOutputFolder"] = testJson["outputFolder"].asString();
    testJson["outputFolder"] = outFolder;
    testJson["preRepetitions"] = testJson["repetitions"].asUInt();
    testJson["repetitions"] = repetitions;
    jsonSettings["test"] = testJson;
    m_settings->updateSettings(jsonSettings);
    m_settings->saveSettings(m_settingsFile);

    // int repetitions = testJson["repetitions"].asInt();
    shared_ptr<Statistics> stats(new Statistics(outFolder, testJson["statisticsFilename"].asString(), repetitions));

    shared_ptr<HttPrivate::ProcessFolderResults> res = htt->processFolder(path + folder + "/", stats, true);
    stats->close();

    // Revert setting json file
    jsonSettings = m_settings->getSettings();
    testJson = jsonSettings["test"];
    testJson["outputFolder"] = testJson["preOutputFolder"].asString();
    testJson["repetitions"] = testJson["preRepetitions"].asUInt();
    testJson.removeMember("preOutputFolder");
    testJson.removeMember("preRepetitions");
    jsonSettings["test"] = testJson;
    m_settings->updateSettings(jsonSettings);
    m_settings->saveSettings(m_settingsFile);

    Json::Value f = jsonCalibration["scheimpflug_transformation_calibration"]["frames"][frame];
    double realB = f["B"].asDouble();
    double realM = f["M"].asDouble();

    // Threshold in micron
    double thresholdB =
        repetitions > 1 ? testJson["thresholdBWithRepetitions"].asDouble() : testJson["thresholdB"].asDouble();
    double thresholdM =
        repetitions > 1 ? testJson["thresholdMWithRepetitions"].asDouble() : testJson["thresholdM"].asDouble();

    // Read stats from stats.csv file
    ifstream csvFile;
    string statsFile = outFolder + "/stats.csv";
    csvFile.open(statsFile.c_str());

    // Read the Data from the file
    vector<string> row;
    string line, column;

    // skip the 1st line
    getline(csvFile, line);

    int successes = 0;

    // read an entire row and store it in a string variable 'line'
    while (getline(csvFile, line))
    {
        row.clear();

        // read every column data of a row and store it in a string variable, 'column'
        stringstream srow(line);
        while (getline(srow, column, ';'))
        {
            // add all the column data of a row to a vector
            row.push_back(column);
        }

        double B = stod(row[1]);
        double M = stod(row[4]);
        double errB = fabs(B - realB) * 1e3;
        double errM = fabs(M - realM) * 1e3;

        stringstream s;
        s << tag << row[0] << "\n";
        s << "\t\t\t\t\tB: " << B << " realB: " << realB << "  err: " << errB << " [um]\n";
        s << "\t\t\t\t\tM: " << M << " realM: " << realM << "  err: " << errM << " [um]";
        m_log->info(s.str());

        if (errB > thresholdB || errM > thresholdM)
        {
            m_log->info(tag + "Validation test failed\n");
        } else
        {
            successes++;
            m_log->info(tag + "Validation test succeed\n");
        }
    }

    stringstream s;
    s << tag << folder << " results:\n";
    s << "\t\t\tImages      : " << res->nImages << "\n";
    s << "\t\t\tSuccess     : " << successes;
    m_log->info(s.str());
}

void Tester::calibrationGrid()
{
    shared_ptr<HttPrivate> htt(new HttPrivate());

    Json::Value jsonSettings = m_settings->getSettings();
    Json::Value jsonCalibration = m_settings->getCalibration();
    string inputImage = jsonCalibration["scheimpflug_transformation_calibration"]["grid"].asString();
    shared_ptr<Image<uint8_t>> img = loadImage((char*)inputImage.c_str());

    // Create an output filestream object

    Json::Value testJson = jsonSettings["test"];
    string outFolder = testJson["outputFolder"].asString();
    string statsFile = outFolder + "/calibrationGridTest_" + now() + ".csv";
    // Make parent directories as needed
    stringstream s;
    s << "mkdir -p " << outFolder;
    system(s.str().c_str());
    ofstream csvFile(statsFile);

    s.str("");
    s << "id;loss;tt;theta;alpha;mag;delta;beta;cx0;cy0\n";
    csvFile << s.str();

    int rep = testJson["repetitions"].asInt();
    for (int i = 0; i < rep; ++i)
    {

        // Avoid to save the calibrated params into the json file
        shared_ptr<HttPrivate::CalibrationGridResults> res =
            make_shared<HttPrivate::CalibrationGridResults>(htt->calibrateGrid(img, false));
        if (res->valid)
        {
            shared_ptr<ScheimpflugTransform::Params> p = res->scheimpflugTransformParams;
            float loss = res->lossValue;
            s.str("");
            s << i << ";" << loss << ";" << p->tt << ";" << p->theta << ";" << p->alpha << ";" << p->magnification
              << ";" << p->delta << ";" << p->beta << ";" << p->cx0 << ";" << p->cy0 << "\n";
            csvFile << s.str();
        }
    }

    csvFile.close();
}

void Tester::comparativeTest()
{
    Json::Value jsonSettings = m_settings->getSettings();
    Json::Value jsonCalibration = m_settings->getCalibration();

    vector<float> objX;
    vector<float> objY;
    // vector<int> imgX;
    // vector<int> imgY;
    // vector<int> roundImgX;
    // vector<int> roundImgY;

    // Read object points from csv file
    ifstream csvFile;
    string inputFile = jsonSettings["test"]["comparativeTestInputFile"].asString();
    csvFile.open(inputFile.c_str());

    // Read the Data from the file
    vector<string> row;
    string line, column;

    // skip the 1st line
    // getline(csvFile, line);

    // read an entire row and store it in a string variable 'line'
    while (getline(csvFile, line))
    {
        row.clear();

        // read every column data of a row and store it in a string variable, 'column'
        stringstream srow(line);
        while (getline(srow, column, ',')) // FIXME: check the column separator used ',' or ';'
        {
            // add all the column data of a row to a vector
            row.push_back(column);
        }

        objX.push_back(stof(row[0]));
        objY.push_back(stof(row[1]));
        // objX.push_back(round(stof(row[0]) * 100.0) / 100.0);
        // objY.push_back(round(stof(row[1]) * 100.0) / 100.0);
    }
    csvFile.close();

    int size = objX.size();
    // imgX.resize(size);
    // imgY.resize(size);
    // roundImgX.resize(size);
    // roundImgY.resize(size);

    double deltaN[3] = {-5.0, 0.0, 5.0};
    double H[3] = {0.0, 7.0, 14.0};
    double delta[3] = {-2.0, 0.0, 2.0};
    // double deltaN[1] = {0};
    // double H[1] = {0};
    // double delta[1] = {0};

    shared_ptr<ScheimpflugTransform> st(new ScheimpflugTransform());

    string folder = jsonSettings["test"]["outputFolder"].asCString();
    string resultsFile;

    for (int deltaIndex = 0; deltaIndex < 3; ++deltaIndex)
    {
        Json::Value params = jsonCalibration["scheimpflug_transformation"];
        params["delta"] = delta[deltaIndex];
        st->init(params);
        for (int deltaNIndex = 0; deltaNIndex < 3; ++deltaNIndex)
        {
            for (int hIndex = 0; hIndex < 3; ++hIndex)
            {
                {
                    stringstream ss;
                    ss << "--------------------\n";
                    ss << "deltaN:" << deltaN[deltaNIndex] << " H:" << H[hIndex] << " delta:" << delta[deltaIndex];
                    m_log->info(tag + ss.str());
                }

                st->setStylusOrientation(deltaN[deltaNIndex], H[hIndex]);

                ostringstream ostr;
                ostr << fixed << setprecision(1);
                ostr << "deltaN=" << deltaN[deltaNIndex] << "_H=" << H[hIndex] << "_delta=" << delta[deltaIndex] << "_"
                     << ".csv";
                cout << ostr.str() << endl;
                string filename = folder + "/" + ostr.str();
                ofstream outCsvFile(filename);
                if (outCsvFile.is_open())
                    cout << ">>>>>>>>>>>>>>>>> OPEN" << endl;

                stringstream ss;
                ss << "objX [mm],objY [mm],imgX [pixel],imgY [pixel],roundImgX [pixel],roundImgY [pixel],newObjX "
                      "[mm],newObjY [mm],deltaX [um],deltaY "
                      "[um],newRoundObjX [mm],newRoundObjY [mm],"
                      "deltaRoundX [um],deltaRoundY [um]\n";
                outCsvFile << ss.str();

                // Transform from object to image plane and vice versa
                for (int i = 0; i < size; ++i)
                {
                    double xMm = round((double)objX[i] * 1e6) * 1e-6;
                    double yMm = round((double)objY[i] * 1e6) * 1e-6;
                    double srcObjPoint[3] = {xMm, yMm, 0.0};
                    double dstImgPoint[3] = {0.0, 0.0, 0.0};
                    double roundDstImgPoint[3] = {0.0, 0.0, 0.0};
                    st->objectToImage(srcObjPoint, dstImgPoint, false);
                    st->objectToImage(srcObjPoint, roundDstImgPoint, true);

                    // Coordinates in the image plane without the rounding
                    double xPixel = dstImgPoint[0];
                    double yPixel = dstImgPoint[1];

                    // imgX.push_back(xPixel);
                    // imgY.push_back(yPixel);

                    // Coordinates in the image plane with the rounding
                    double roundXPixel = roundDstImgPoint[0];
                    double roundYPixel = roundDstImgPoint[1];

                    // roundImgX.push_back(roundXPixel);
                    // roundImgY.push_back(roundYPixel);

                    // Remapping the points into object plane into image plane
                    double dstObjPoint[3] = {0.0, 0.0, 0.0};
                    double roundDstObjPoint[3] = {0.0, 0.0, 0.0};
                    st->imageToObject(dstImgPoint, dstObjPoint);
                    st->imageToObject(roundDstImgPoint, roundDstObjPoint);

                    double newXMm = round(dstObjPoint[0] * 1e6) * 1e-6;
                    double newYMm = round(dstObjPoint[1] * 1e6) * 1e-6;
                    double newRoundXMm = round(roundDstObjPoint[0] * 1e6) * 1e-6;
                    double newRoundYMm = round(roundDstObjPoint[1] * 1e6) * 1e-6;

                    double deltaX = (xMm - newXMm) * 1000;
                    double deltaY = (yMm - newYMm) * 1000;

                    double deltaRoundX = (xMm - newRoundXMm) * 1000;
                    double deltaRoundY = (yMm - newRoundYMm) * 1000;

                    // if (jsonSettings["debug"].asBool())
                    // {
                    //     stringstream ss;
                    //     ss << tag << "(" << objX[i] << "," << objY[i] << ") ---> (" << xPixel << "," <<
                    //     yPixel
                    //        << ") round: (" << roundXPixel << "," << roundYPixel << ")";
                    //     m_log->debug(ss.str());
                    // }
                    outCsvFile << xMm << "," << yMm << "," << xPixel << "," << yPixel << "," << roundXPixel << ","
                               << roundYPixel << "," << newXMm << "," << newYMm << "," << deltaX << "," << deltaY << ","
                               << newRoundXMm << "," << newRoundYMm << "," << deltaRoundX << "," << deltaRoundY << "\n";
                }
                outCsvFile.close();
            }
        }
    }
}

void Tester::generateSyntheticImages()
{
    Json::Value jsonSettings = m_settings->getSettings();
    Json::Value jsonCalibration = m_settings->getCalibration();

    vector<float> objX;
    vector<float> objY;

    // Read object points from csv file
    ifstream csvFile;
    string inputFile = jsonSettings["test"]["generetionSyntheticImagesInputFile"].asString();
    string bevel = inputFile.substr(inputFile.find_last_of("/") + 1);
    // cout << ">>> " << bevel << endl;
    bevel = bevel.substr(0, bevel.find_first_of(".csv"));
    // cout << ">>> " << bevel << endl;
    csvFile.open(inputFile.c_str());

    // Read the Data from the file
    vector<string> row;
    string line, column;

    // read an entire row and store it in a string variable 'line'
    while (getline(csvFile, line))
    {
        row.clear();

        // read every column data of a row and store it in a string variable, 'column'
        stringstream srow(line);
        while (getline(srow, column, ',')) // FIXME: check the column separator used ',' or ';'
        {
            // add all the column data of a row to a vector
            row.push_back(column);
        }

        objX.push_back(stof(row[0]));
        objY.push_back(stof(row[1]));
    }
    csvFile.close();

    int size = objX.size();

    string folder = jsonSettings["test"]["outputFolder"].asCString();
    string resultsFile;

    int deltaNSize = 9;
    int hSize = 3;

    float deltaN[] = {-12.0, -9.0, -6.0, -3.0, 0.0, 3.0, 6.0, 9.0, 12.0};
    float H[] = {7.0, 11.0, 15.0};

    shared_ptr<ScheimpflugTransform> st(new ScheimpflugTransform());
    Json::Value params = jsonCalibration["nominal_scheimpflug_transformation"];
    st->init(params);

    for (int deltaNIndex = 0; deltaNIndex < deltaNSize; ++deltaNIndex)
    {
        for (int hIndex = 0; hIndex < hSize; ++hIndex)
        {
            {
                stringstream ss;
                ss << "--------------------\n";
                ss << "deltaN:" << deltaN[deltaNIndex] << " H:" << H[hIndex];
                m_log->info(tag + ss.str());
            }

            st->setStylusOrientation(deltaN[deltaNIndex], H[hIndex]);

            ostringstream ostr;
            ostr << bevel;
            ostr << fixed << setprecision(1) << showpos;
            ostr << "_H=" << setfill('0') << setw(2) << H[hIndex];
            ostr << "_deltaN=" << setfill('0') << setw(2) << deltaN[deltaNIndex];
            ostr << ".png";
            cout << ostr.str() << endl;
            // string filename = folder + "/" + ostr.str();
            string filename = ostr.str();

            vector<double> imgX;
            vector<double> imgY;
            vector<int> roundImgX;
            vector<int> roundImgY;

            // Transform from object to image plane and vice versa
            for (int i = 0; i < size; ++i)
            {
                double xMm = round((double)objX[i] * 1e6) * 1e-6;
                double yMm = round((double)objY[i] * 1e6) * 1e-6;
                double srcObjPoint[3] = {xMm, yMm, 0.0};
                double dstImgPoint[3] = {0.0, 0.0, 0.0};
                double roundDstImgPoint[3] = {0.0, 0.0, 0.0};
                st->objectToImage(srcObjPoint, dstImgPoint, false);
                st->objectToImage(srcObjPoint, roundDstImgPoint, true);

                // Coordinates in the image plane without the rounding
                double xPixel = dstImgPoint[0];
                double yPixel = dstImgPoint[1];

                imgX.push_back(xPixel);
                imgY.push_back(yPixel);

                // Coordinates in the image plane with the rounding
                double roundXPixel = roundDstImgPoint[0];
                double roundYPixel = roundDstImgPoint[1];

                roundImgX.push_back((int)roundXPixel);
                roundImgY.push_back((int)roundYPixel);

                {
                    stringstream ss;
                    // ss << xMm << "," << yMm << "," << xPixel << "," << yPixel << "," << roundXPixel << ","
                    //    << roundYPixel;
                    ss << "(" << xMm << "," << yMm << ") -> (" << roundXPixel << "," << roundYPixel << ")";
                    cout << ss.str() << endl;
                }
            }

            shared_ptr<Image<uint8_t>> image(new Image<uint8_t>(560, 560, 1));
            uint8_t* pImg = image->getData();
            uint16_t stride = image->getStride(); // / sizeof(uint8_t);

            // for (int i = 0; i < roundImgX.size(); ++i)
            // {
            //     int index = roundImgX[i] + stride * roundImgY[i];
            //     pImg[index] = 255;
            // }

            // L1
            {
                // y = m*x + q
                float xA = roundImgX[0];
                float yA = roundImgY[0];
                float xB = roundImgX[1];
                float yB = roundImgY[1];
                float m = (yB - yA) / (xB - xA);
                float q = yA - m * xA;

                for (int y = yB; y <= yA; ++y)
                {
                    int x = (int)roundf((y - q) / m + 0.5);
                    int index = x + stride * y;
                    pImg[index] = 255;
                }
            }

            // L2
            {
                // y = m*x + q
                float xA = roundImgX[1];
                float yA = roundImgY[1];
                float xB = roundImgX[2];
                float yB = roundImgY[2];
                float m = (yB - yA) / (xB - xA);
                float q = yA - m * xA;

                for (int y = yB; y <= yA; ++y)
                {
                    int x = (int)roundf((y - q) / m + 0.5);
                    int index = x + stride * y;
                    pImg[index] = 255;
                }
            }

            // L3
            {
                // y = m*x + q
                float xA = roundImgX[2];
                float yA = roundImgY[2];
                float xB = roundImgX[3];
                float yB = roundImgY[3];
                float m = (yB - yA) / (xB - xA);
                float q = yA - m * xA;

                for (int y = yB; y <= yA; ++y)
                {
                    int x = (int)roundf((y - q) / m + 0.5);
                    int index = x + stride * y;
                    pImg[index] = 255;
                }
            }

            // L4
            {
                // y = m*x + q
                float xA = roundImgX[3];
                float yA = roundImgY[3];
                float xB = roundImgX[4];
                float yB = roundImgY[4];
                float m = (yB - yA) / (xB - xA);
                float q = yA - m * xA;

                for (int y = yB; y <= yA; ++y)
                {
                    int x = (int)roundf((y - q) / m + 0.5);
                    int index = x + stride * y;
                    pImg[index] = 255;
                }
            }

#if 0
            // Used to generate smoothed images 
            shared_ptr<ImagePreprocessing> imgPreProc = shared_ptr<ImagePreprocessing>(new ImagePreprocessing());
            shared_ptr<Image<uint8_t>> filteredImage = imgPreProc->lowPassFilter1(image, 4);

            saveImage(folder, filename, filteredImage);
#endif
            saveImage(folder, filename, image);
        }
    }
}

void Tester::inverseTransform(string inputFile)
{
    cout << "1" << endl;
    Json::Value jsonSettings = m_settings->getSettings();
    Json::Value jsonCalibration = m_settings->getCalibration();

    vector<int> imgX;
    vector<int> imgY;
    vector<float> objX;
    vector<float> objY;

    // Read image points from csv file
    ifstream csvFile;
    csvFile.open(inputFile.c_str());

    // Read the Data from the file
    vector<string> row;
    string line, column;

    // read an entire row and store it in a string variable 'line'
    while (getline(csvFile, line))
    {
        row.clear();

        // read every column data of a row and store it in a string variable, 'column'
        stringstream srow(line);
        while (getline(srow, column, ',')) // FIXME: check the column separator used ',' or ';'
        {
            // add all the column data of a row to a vector
            row.push_back(column);
        }

        imgX.push_back(stof(row[0]));
        imgY.push_back(stof(row[1]));
    }
    csvFile.close();
    cout << "2" << endl;

    double R = 0, H = 0;
    string filename = inputFile.substr(inputFile.find_last_of('/') + 1);
    // cout << filename << endl;
    parseFilename(filename, R, H);

    // Init Scheimpflug transform
    shared_ptr<ScheimpflugTransform> st(new ScheimpflugTransform());
    st->init(jsonCalibration["scheimpflug_transformation"]);
    st->setStylusOrientation(R, H);

    cout << "R: " << R << " H: " << H << endl;

    int size = imgX.size();
    // Transform from object to image plane and vice versa
    for (int i = 0; i < size; ++i)
    {
        double srcImgPoint[3] = {(double)imgX[i], (double)imgY[i], 0.0};
        double dstObjPoint[3] = {0.0, 0.0, 0.0};
        st->imageToObject(srcImgPoint, dstObjPoint);

        objX.push_back((float)dstObjPoint[0]);
        objY.push_back((float)dstObjPoint[1]);
    }

    string outFolder = jsonSettings["test"]["outputFolder"].asCString();
    stringstream s;
    s << "mkdir -p " << outFolder;
    system(s.str().c_str());

    string filenameObj = outFolder + "/" + filename.substr(0, filename.find_last_of(".")) + "_obj.csv";
    ofstream outCsvFile(filenameObj);
    if (outCsvFile.is_open())
        cout << ">>>>>>>>>>>>>>>>> OPEN" << endl;
    else
        cout << ">>>>>>>>>>>>>>>>> ERROR" << endl;

    cout << filenameObj << endl;

    for (int i = 0; i < size; ++i)
    {
        stringstream ss;
        ss << objX[i] << "," << objY[i] << "\n";
        outCsvFile << ss.str();
    }
    outCsvFile.close();
    cout << "3" << endl;
}
#include "Htt.h"
#include "HttPrivate.h"
#include "Log.h"
#include "Settings.h"
#include <map>

using namespace std;
using namespace Nidek::Libraries::HTT;

Htt::Htt() : m_settings(Settings::getInstance()), m_log(Log::getInstance())
{
    // Instance Htt library
    m_httPrivate = make_shared<HttPrivate>(HttPrivate());

    Json::Value root = m_settings->getCalibration();
    if (root.isMember("scheimpflug_transformation") && root["scheimpflug_transformation"] != Json::objectValue)
    {
        m_httPrivate->init(0, 0);
    }
}

Htt::CalibrationGridResults Htt::calibrateGrid(shared_ptr<Image<uint8_t>> inputImage, bool saveValues /* = true*/)
{
    HttPrivate::CalibrationGridResults r = m_httPrivate->calibrateGrid(inputImage, saveValues);

    Htt::CalibrationGridResults results;
    results.valid = r.valid;
    results.lossValue = r.lossValue;
    results.imgPlaneGridLines = r.imgPlaneGridLines;
    results.imgPlaneIntersectionPoints = r.imgPlaneIntersectionPoints;
    results.objPlaneGrid = r.objPlaneGrid;
    results.objPlaneIntersectionPoints = r.objPlaneIntersectionPoints;
    results.returnCode = r.returnCode;
    results.gridErrors = r.gridErrors;

    if (r.valid)
    {
        map<string, double> m;
        auto p = r.scheimpflugTransformParams;
        m.insert(pair<string, double>("alpha", p->alpha));
        m.insert(pair<string, double>("beta", p->beta));
        m.insert(pair<string, double>("cx0", p->cx0));
        m.insert(pair<string, double>("cy0", p->cy0));
        m.insert(pair<string, double>("delta", p->delta));
        m.insert(pair<string, double>("magnification", p->magnification));
        m.insert(pair<string, double>("theta", p->theta));
        m.insert(pair<string, double>("tt", p->tt));
        m.insert(pair<string, double>("p1", p->p1));
        m.insert(pair<string, double>("p2", p->p2));
        m.insert(pair<string, double>("phi", p->phi));
        results.scheimpflugTransformParams = m;

        // m_settings->printCalibration();
    }

    return results;
}

Htt::AccurateCalibrationResults Htt::accurateCalibration(shared_ptr<Image<uint8_t>> inputImage, const double& deltaN,
                                                         const double& H, const string& filename)
{
    //-------------------------------------------------------------------------
    // Note: this is the original calibration based on a single bevel of the
    // calibration tool.
    //-------------------------------------------------------------------------

    HttPrivate::processArgs args;
    args.R = deltaN;
    args.H = H;
    args.inputImage = inputImage;
    args.repetitions = 1;
    args.processType = HttProcessType::calibration;
    args.imageLabel = filename.substr(0, filename.find_last_of('.'));
    args.modelType = ModelType::MiniBevelModelType;

    // Enable Scheimpflug Transform calibration
    int maxRetries = m_settings->getSettings()["scheimpflug_transformation_calibration"]["maxRetries"].asInt();
    bool validCalibration = false;
    HttPrivate::OptimizeScheimpflugTransformResults res;
    for (int count = 0; count < maxRetries; ++count)
    {
        shared_ptr<HttPrivate::ProcessResults> resProcess = m_httPrivate->process(args);
        if (!resProcess->valid)
        {
            // log->debug(tag + "htt process failed");
            continue;
        }

        vector<float> imgX;
        vector<float> imgY;
        for (int i = 0; i < (int)resProcess->optimizationOutput->bevel->imgX.size(); ++i)
        {
            imgX.push_back((float)resProcess->optimizationOutput->bevel->imgX[i]);
            imgY.push_back((float)resProcess->optimizationOutput->bevel->imgY[i]);
        }

        res = m_httPrivate->optimizeScheimpflugTransform(inputImage, imgX, imgY, deltaN, H);
        if (res.valid)
        {
            validCalibration = true;
            break;
        }
    }

    Htt::AccurateCalibrationResults results;
    results.valid = validCalibration;
    if (res.valid)
    {
        map<string, float> m;
        auto p = res.optimizationOutput->params;
        m.insert(pair<string, float>("alpha", p->alpha));
        m.insert(pair<string, float>("beta", p->beta));
        m.insert(pair<string, float>("cx0", p->cx0));
        m.insert(pair<string, float>("cy0", p->cy0));
        m.insert(pair<string, float>("delta", p->delta));
        m.insert(pair<string, float>("magnification", p->magnification));
        m.insert(pair<string, float>("theta", p->theta));
        m.insert(pair<string, float>("tt", p->tt));
        m.insert(pair<string, float>("p1", p->p1));
        m.insert(pair<string, float>("p2", p->p2));
        m.insert(pair<string, float>("phi", p->phi));
        results.scheimpflugTransformParams = m;

        // m_settings->printCalibration();
    }

    return results;
}

Htt::AccurateCalibrationResults Htt::accurateCalibration(vector<shared_ptr<Image<uint8_t>>>& inputImageSet,
                                                         const vector<double>& deltaN, const vector<double>& H,
                                                         const vector<string>& filenameSet)
{
    //-------------------------------------------------------------------------
    // Note: this is the calibration based on multiple images of a bevel of the
    // calibration tool.
    //-------------------------------------------------------------------------

    // ADG-MULTI: FIXME: is this required?
    // m_settings->loadSettings(m_settingFile);
    // m_settings->loadCalibration(m_calibrationFile);

    HttPrivate::processArgs args;
    args.repetitions = 1;
    args.processType = HttProcessType::calibration;
    args.modelType = ModelType::MiniBevelModelType;

    // Enable Scheimpflug Transform calibration
    int maxRetries = m_settings->getSettings()["scheimpflug_transformation_calibration"]["maxRetries"].asInt();
    bool validCalibration = false;

    HttPrivate::OptimizeScheimpflugTransformResults res;

    // Prepare some working vectors.
    vector<shared_ptr<Image<uint8_t>>> imageSet;
    vector<shared_ptr<vector<float>>> imgXSet, imgYSet;
    vector<double> deltaNSet, HSet;
    vector<shared_ptr<Profile<int>>> profileSet;

    // Try the processing a certain number of times.
    for (int count = 0; count < maxRetries; ++count)
    {
        // Make sure that the sets are empty.
        imageSet.clear();
        imgXSet.clear();
        imgYSet.clear();
        deltaNSet.clear();
        HSet.clear();
        profileSet.clear();

        // Process each input image in order to have the best bevel
        const int nImages = inputImageSet.size();
        for (int k = 0; k < nImages; ++k)
        {
            // Get the current Image
            shared_ptr<Image<uint8_t>> currentImage = inputImageSet[k];

            // Set parameters for processing the image.
            args.R = deltaN[k];
            args.H = H[k];
            args.inputImage = currentImage;
            args.imageLabel = filenameSet[k].substr(0, filenameSet[k].find_last_of('.'));

            // Extract the profile for the k-th image.
            shared_ptr<HttPrivate::ProcessResults> resProcess = m_httPrivate->process(args);

            // If the processing was valid, then we can insert all the information in the
            // structures that are passed to the optimizer.
            if (resProcess->valid)
            {
                imageSet.push_back(currentImage);
                deltaNSet.push_back(deltaN[k]);
                HSet.push_back(H[k]);
                profileSet.push_back(resProcess->profile);

                shared_ptr<vector<float>> imgX(new vector<float>());
                shared_ptr<vector<float>> imgY(new vector<float>());

                for (int i = 0; i < (int)resProcess->optimizationOutput->bevel->imgX.size(); ++i)
                {
                    imgX->push_back((float)resProcess->optimizationOutput->bevel->imgX[i]);
                    imgY->push_back((float)resProcess->optimizationOutput->bevel->imgY[i]);
                }

                imgXSet.push_back(imgX);
                imgYSet.push_back(imgY);
            }
        }

        int imageSetSize = imageSet.size();

        // Print the number of valid images.
        cout << "Valid images: " << imageSetSize << "/" << inputImageSet.size() << endl;

        if (imageSetSize)
        {
            // Scheimpflug optimization
            res = m_httPrivate->optimizeScheimpflugTransform(imageSet, imgXSet, imgYSet, deltaNSet, HSet, profileSet);

            cout << "res.valid: " << res.valid << endl;
            if (res.valid)
            {

                validCalibration = true;
                break;
            }
        }

        break; // FIXME
    }

    Htt::AccurateCalibrationResults results;
    results.valid = validCalibration;
    if (res.valid)
    {
        map<string, float> m;
        auto p = res.optimizationOutput->params;
        m.insert(pair<string, float>("alpha", p->alpha));
        m.insert(pair<string, float>("beta", p->beta));
        m.insert(pair<string, float>("cx0", p->cx0));
        m.insert(pair<string, float>("cy0", p->cy0));
        m.insert(pair<string, float>("delta", p->delta));
        m.insert(pair<string, float>("magnification", p->magnification));
        m.insert(pair<string, float>("theta", p->theta));
        m.insert(pair<string, float>("tt", p->tt));
        m.insert(pair<string, float>("p1", p->p1));
        m.insert(pair<string, float>("p2", p->p2));
        m.insert(pair<string, float>("phi", p->phi));
        results.scheimpflugTransformParams = m;

        // Save calibration file
        // FIXME: insert the following line
        // m_settings->saveCalibration(m_calibrationFile);
        // m_settings->printCalibration();
    }

    return results;
}

Htt::LensFittingResults Htt::bevelFitting(shared_ptr<Image<uint8_t>> inputImage, const double& deltaN, const double& H,
                                          ModelType modelType, bool checkIntersections)
{
    Htt::LensFittingResults r;

    HttPrivate::processArgs args;
    args.R = deltaN;
    args.H = H;
    args.inputImage = inputImage;
    args.processType = checkIntersections ? HttProcessType::lensFitting : HttProcessType::validation;
    args.modelType = modelType;
    shared_ptr<HttPrivate::ProcessResults> res = m_httPrivate->process(args);

    r.valid = res->valid;
    r.inputImage = inputImage;
    r.bevelImage = res->bevelImage;

    if (res->valid)
    {
        shared_ptr<Profile<int>> bevel = res->optimizationOutput->bevel;
        for (int i = 0; i < (int)bevel->x.size(); ++i)
            r.imgBevel.push_back(pair<int, int>(bevel->x[i], bevel->y[i]));
        for (int i = 0; i < (int)bevel->imgX.size(); ++i)
            r.imgModelPoints.push_back(pair<int, int>(bevel->imgX[i], bevel->imgY[i]));
        for (int i = 0; i < (int)bevel->distanceImgX.size(); ++i)
            r.imgDistancePoints.push_back(pair<int, int>(bevel->distanceImgX[i], bevel->distanceImgY[i]));

        shared_ptr<Profile<int>> profile = res->profile;
        for (int i = 0; i < (int)profile->x.size(); ++i)
            r.imgProfile.push_back(pair<int, int>(profile->x[i], profile->y[i]));
        for (int i = 0; i < (int)profile->ancorImgX.size(); ++i)
            r.imgMaximaPoints.push_back(pair<int, int>(profile->ancorImgX[i], profile->ancorImgY[i]));

        r.objPlaneImage = m_httPrivate->generateImageObjPlane(inputImage);
        r.objBevelImage = res->objBevelImage;

        int size = (int)profile->x.size();
        vector<int> objX;
        vector<int> objY;
        objX.reserve(size);
        objY.reserve(size);
        m_httPrivate->convertPointsFromImgToObjPlane(profile->x, profile->y, objX, objY);
        for (int i = 0; i < (int)objX.size(); ++i)
            r.objProfile.push_back(pair<int, int>(objX[i], objY[i]));

        size = (int)profile->ancorImgX.size();
        objX.clear();
        objY.clear();
        objX.reserve(size);
        objY.reserve(size);
        m_httPrivate->convertPointsFromImgToObjPlane(profile->ancorImgX, profile->ancorImgY, objX, objY);
        for (int i = 0; i < (int)objX.size(); ++i)
            r.objMaximaPoints.push_back(pair<int, int>(objX[i], objY[i]));

        size = (int)bevel->x.size();
        objX.clear();
        objY.clear();
        objX.reserve(size);
        objY.reserve(size);
        m_httPrivate->convertPointsFromImgToObjPlane(bevel->x, bevel->y, objX, objY);
        for (int i = 0; i < (int)objX.size(); ++i)
            r.objBevel.push_back(pair<int, int>(objX[i], objY[i]));

        size = (int)bevel->imgX.size();
        objX.clear();
        objY.clear();
        objX.reserve(size);
        objY.reserve(size);
        m_httPrivate->convertPointsFromImgToObjPlane(bevel->imgX, bevel->imgY, objX, objY);
        for (int i = 0; i < (int)objX.size(); ++i)
            r.objModelPoints.push_back(pair<int, int>(objX[i], objY[i]));

        size = (int)bevel->distanceImgX.size();
        objX.clear();
        objY.clear();
        objX.reserve(size);
        objY.reserve(size);
        m_httPrivate->convertPointsFromImgToObjPlane(bevel->distanceImgX, bevel->distanceImgY, objX, objY);
        for (int i = 0; i < (int)objX.size(); ++i)
            r.objDistancePoints.push_back(pair<int, int>(objX[i], objY[i]));

        r.measures = res->optimizationOutput->measures->m;
        r.modelType = res->optimizationOutput->modelType;

        Json::Value stc = m_settings->getCalibration()["scheimpflug_transformation_calibration"];
        Json::Value frame = stc["frames"][stc["current"].asString()];
        // FIXME: improve for bevels different from MiniBevel
        double realB = frame["B"].asDouble();
        double realM = frame["M"].asDouble();
        r.realMeasures.insert(make_pair("B", realB));
        r.realMeasures.insert(make_pair("M", realM));
        r.errors.insert(make_pair("B", (realB - r.measures["B"]) * 1000.0));
        r.errors.insert(make_pair("M", (realM - r.measures["M"]) * 1000.0));
    }

    return r;
}

Htt::LensFittingResults Htt::lensValidation(shared_ptr<Image<uint8_t>> inputImage, const double& deltaN,
                                            const double& H, ModelType modelType)
{
    return bevelFitting(inputImage, deltaN, H, modelType, false);
}

Htt::ProfileDetectionResults Htt::profileDetection(shared_ptr<Image<uint8_t>> inputImage, const double& deltaN,
                                                   const double& H, ModelType modelType)
{
    Htt::ProfileDetectionResults r;
    HttPrivate::processArgs args;
    args.R = deltaN;
    args.H = H;
    args.inputImage = inputImage;
    args.processType = HttProcessType::lensFitting;
    args.modelType = modelType;
    shared_ptr<HttPrivate::ProcessResults> res = m_httPrivate->process(args, true);

    r.valid = res->valid;
    if (res->valid)
    {
        shared_ptr<Profile<int>> profile = res->profile;
        for (int i = 0; i < (int)profile->x.size(); ++i)
            r.imgProfile.push_back(pair<int, int>(profile->x[i], profile->y[i]));
    }
    return r;
}

Htt::LensFittingResults Htt::lensFitting(shared_ptr<Image<uint8_t>> inputImage, const double& deltaN, const double& H,
                                         ModelType modelType)
{
    return bevelFitting(inputImage, deltaN, H, modelType, true);
}

void Htt::processFolder(const string& folder, shared_ptr<Statistics> stats /* = nullptr*/)
{
    m_httPrivate->processFolder(folder, stats);
}

int Htt::printReturnCode()
{
    return m_httPrivate->printReturnCode();
}

double Htt::measureOnObjectPlane(double x1, double y1, double x2, double y2)
{
    return m_httPrivate->measureOnObjectPlane(x1, y1, x2, y2);
}

string Htt::getCalibrationParams()
{
    // XXX: check me!
    // m_settings->loadCalibration(m_calibrationFile);
    return m_settings->getCalibrationString();
}

map<string, bool> Htt::checkCalibratedParams()
{
    map<string, bool> result;
    result = m_httPrivate->checkCalibratedParamsWithNominalRanges(true);
    return result;
}

vector<pair<int, int>> Htt::getMinimumImagingFoV(const int& x, const int& y, const double& width, const double& height)
{
    double halfWidth = width * 0.5;
    double halfHeight = height * 0.5;

    vector<pair<int, int>> res;
    double Xmm = m_httPrivate->convertPixelToMm((double)x);
    double Ymm = m_httPrivate->convertPixelToMm((double)y);
    cout << Xmm << " " << Ymm << " " << halfWidth << " " << halfHeight << endl;

    res.push_back(pair<int, int>(m_httPrivate->convertMmToPixel(Xmm - halfWidth),
                                 m_httPrivate->convertMmToPixel(Ymm + halfHeight)));
    res.push_back(pair<int, int>(m_httPrivate->convertMmToPixel(Xmm + halfWidth),
                                 m_httPrivate->convertMmToPixel(Ymm + halfHeight)));
    res.push_back(pair<int, int>(m_httPrivate->convertMmToPixel(Xmm - halfWidth),
                                 m_httPrivate->convertMmToPixel(Ymm - halfHeight)));
    res.push_back(pair<int, int>(m_httPrivate->convertMmToPixel(Xmm + halfWidth),
                                 m_httPrivate->convertMmToPixel(Ymm - halfHeight)));
    return res;
}

vector<pair<int, int>> Htt::getReferenceSquare(const double& width, const double& height)
{
    // Initialize the Scheimpflug Transform
    m_httPrivate->init(0, 0, true); // Initialize using calibration mode

    double halfWidth = width * 0.5;
    double halfHeight = height * 0.5;

    vector<float> objX;
    vector<float> objY;
    objX.push_back((-halfWidth));
    objY.push_back((halfHeight));

    objX.push_back((halfWidth));
    objY.push_back((halfHeight));

    objX.push_back((-halfWidth));
    objY.push_back((-halfHeight));

    objX.push_back((halfWidth));
    objY.push_back((-halfHeight));

    int size = (int)objX.size();
    vector<int> imgX;
    vector<int> imgY;
    imgX.reserve(size);
    imgY.reserve(size);
    m_httPrivate->convertPointsFromObjToImgPlane(objX, objY, imgX, imgY);

    vector<pair<int, int>> squarePoints;
    for (int i = 0; i < imgX.size(); ++i)
    {
        // cout << "x: " << imgX[i] << " y: " << imgY[i] << endl;
        squarePoints.push_back(pair<int, int>(imgX[i], imgY[i]));
    }

    return squarePoints;
}

void Htt::initMultiFramesCalibration(const string& folder, vector<shared_ptr<Image<uint8_t>>>& inputImageSet,
                                     vector<double>& deltaNSet, vector<double>& HSet, vector<string>& filenameSet)
{
    m_httPrivate->initMultiFramesCalibration(folder, inputImageSet, deltaNSet, HSet, filenameSet);
}

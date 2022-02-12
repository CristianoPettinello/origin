//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "HttPrivate.h"
#include "Pso.h"
#include "Utils.h"
#ifdef UNIX
#include <dirent.h>
#endif
#include "Hough.h"
#include "math.h"
#include <fstream> /* ofstream */
#include <iomanip>

using namespace Nidek::Libraries::HTT;

#ifdef UNIX
#define sprintf_s(buf, ...) sprintf((buf), __VA_ARGS__)
#endif

static const string tag = "[HTT ] ";

const int colorWhite[] = {255, 255, 255};
const int colorYellow[] = {255, 255, 0};
const int colorRed[] = {255, 0, 0};
const int colorGreen[] = {0, 255, 0};
const int colorPurple[] = {255, 0, 255};
const int colorBlu[] = {0, 0, 255};

HttPrivate::ProcessFolderResults::ProcessFolderResults() : nImages(0)
{
    counters.insert(make_pair(ReturnCode::LENSFITTING_SUCCESSFUL, 0));
    counters.insert(make_pair(ReturnCode::LENSFITTING_FAILED_MAX_RETRIES_REACHED, 0));
    counters.insert(make_pair(ReturnCode::LENSFITTING_FAILED_BEVELFRAME_INTERSECTION, 0));
    counters.insert(make_pair(ReturnCode::LENSFITTING_FAILED_PSO_BOUNDARY_REACHED, 0));
}

HttPrivate::HttPrivate()
    : log(Log::getInstance()),
      settings(Settings::getInstance()),
      imgPreProc(new ImagePreprocessing()),
      optimizationEngine(new OptimizationEngine()),
      m_st(new ScheimpflugTransform()),
      m_rcode(ReturnCode::UNINITIALIZED)
{
    m_debug = settings->getSettings()["debug"].asBool();
}

shared_ptr<HttPrivate::ProcessResults> HttPrivate::process(processArgs& args, bool disableOptimization /*= false*/)
{
    M_Assert(args.repetitions > 0, "Process repetitions must be greather than zero");

    Json::Value jsonSettings = settings->getSettings();
    Json::Value jsonCalibration = settings->getCalibration();

    shared_ptr<HttPrivate::ProcessResults> res(new HttPrivate::ProcessResults);
    res->valid = false;

    if (args.R == 0 || args.H == 0)
    {
        char str[100];
        sprintf_s(str, "Check Stylus Orientation    deltaN:%.1f H:%.1f", args.R, args.H);
        log->info(tag + str);
    }

    // Check R/H boundaries
    double R_min = jsonCalibration["htt"]["deltaN"]["min"].asDouble();
    double R_max = jsonCalibration["htt"]["deltaN"]["max"].asDouble();
    double H_min = jsonCalibration["htt"]["H"]["min"].asDouble();
    double H_max = jsonCalibration["htt"]["H"]["max"].asDouble();
    if (args.R < R_min || args.R > R_max)
    {
        stringstream s;
        s << tag << "deltaN:" << args.R << " value out of boundary [" << R_min << "," << R_max << "]";
        log->warn(s.str());
    }
    if (args.H < H_min || args.H > H_max)
    {
        stringstream s;
        s << tag << "H:" << args.H << " value out of boundary [" << H_min << "," << H_max << "]";
        log->warn(s.str());
    }

    // Itialize the Scheimpflug Transform
    init(args.R, args.H, args.processType == HttProcessType::calibration);

    // Instance preprocessing and optimization time
    float preprocessingTime = 0;
    // float currOptimizationTime = 0;
    float initOptimizationTime = 0;
    float optimizationTime = 0;

    // Preprocessing
    timer.start();
    shared_ptr<Profile<int>> profileCleaned =
        preProcess(args.inputImage, args.imageLabel, args.processType, args.modelType);
    preprocessingTime = timer.stop();

    if (!profileCleaned)
        return res;

    if (args.processType == HttProcessType::lensFitting && !jsonSettings["optimization"]["enabled"].asBool() ||
        disableOptimization)
    {
        log->warn(tag + "Optimization disabled");
        res->valid = true;
        res->profile = profileCleaned;
        return res;
    }

    Pso<float>::Input psoInput;

    timer.start();
    shared_ptr<Model> model =
        optimizationEngine->initModel(m_st, profileCleaned, args.modelType, args.processType, psoInput);
    initOptimizationTime = timer.stop();

    // Handle the case when the model was not initialized (e.g. upperMax or lowerMax not found).
    if (model == nullptr)
    {
        log->warn(tag + "Model was not initialized");
        m_rcode = ReturnCode::LENSFITTING_FAILED_MODEL_NOT_INITIALIZED;
        return res;
    }

    shared_ptr<Image<uint8_t>> bevelImg = nullptr;
    shared_ptr<Image<uint8_t>> objFrameImg = nullptr;
    shared_ptr<Image<uint8_t>> objBevelImg = nullptr;
    shared_ptr<OptimizationEngine::Output> output = nullptr;
    int maxRetries = jsonSettings["optimization"]["maxRetries"].asInt();

    for (int i = 0; i < args.repetitions; ++i)
    {
        if (m_debug)
        {
            stringstream str;
            str << tag << "optimizeModel [" << i + 1 << "/" << args.repetitions << "]";
            log->debug(str.str());
        }
        timer.start();
        output = optimizationEngine->optimizeModel(model, psoInput, profileCleaned, maxRetries);
        float currOptimizationTime = timer.stop();
        optimizationTime += currOptimizationTime;

        if (output->measures != nullptr)
        {
            // Save process results in the statistics vectors
            ModelType modelType = model->modelType();

            if ((args.processType == HttProcessType::lensFitting || args.processType == HttProcessType::validation) &&
                args.stats)
                if (modelType == MiniBevelModelType)
                    args.stats->push_back(output->measures->m["B*"], output->measures->m["B"], output->measures->m["M"],
                                          0.0, output->loss, output->iteration, preprocessingTime,
                                          currOptimizationTime);
                else if (modelType == MiniBevelModelExtType)
                {
                    args.stats->push_back(output->measures->m["B*"], output->measures->m["B"], output->measures->m["M"],
                                          output->measures->m["beta"], output->loss, output->iteration,
                                          preprocessingTime, currOptimizationTime);
                }
        }

        bevelImg = generateBevelImage(args.inputImage, output->bevel, profileCleaned);
        objBevelImg = generateBevelImageOnObjectPlane(args.inputImage, output->bevel, profileCleaned);

        // Save image on disk
        if (jsonSettings["test"]["storeImages"].asBool())
        {
            {
                char file[256];
                sprintf_s(file, "%s_objplan_%03d.png", args.imageLabel.c_str(), i);
                saveImage(jsonSettings["test"]["outputFolder"].asString(), file, objBevelImg);
            }

            {
                char file[256];
                sprintf_s(file, "%s_%03d.png", args.imageLabel.c_str(), i);
                saveImage(jsonSettings["test"]["outputFolder"].asString(), file, bevelImg);
            }
        }
    }

    // Save optimizeModel return code
    m_rcode = output->rcode;

    log->info(tag + "Performance:");
    optimizationTime /= args.repetitions;
    optimizationTime += initOptimizationTime;
    float totalProcessingTime = preprocessingTime + optimizationTime;
    char str[100];
    sprintf_s(str, "\tpreprocessing time:    %.3f s", preprocessingTime);
    log->info(tag + str);

    sprintf_s(str, "\toptimization time:     %.3f s", optimizationTime);
    log->info(tag + str);

    sprintf_s(str, "\tTotal processing time: %.3f s", totalProcessingTime);
    log->info(tag + str);

    if (m_rcode == ReturnCode::LENSFITTING_SUCCESSFUL || m_rcode == ReturnCode::LENSFITTING_FAILED_PSO_BOUNDARY_REACHED)
        res->valid = true;
    else
        res->valid = false;
    res->bevelImage = bevelImg;
    res->optimizationOutput = output;
    res->preprocessingTime = preprocessingTime;
    res->optimizationTime = optimizationTime;
    res->profile = profileCleaned;
    res->objBevelImage = objBevelImg;

    return res;
}

HttPrivate::OptimizeScheimpflugTransformResults
HttPrivate::optimizeScheimpflugTransform(shared_ptr<Image<uint8_t>> inputImage, const vector<float>& imgX,
                                         const vector<float>& imgY, double R /* = 0*/
                                         ,
                                         double H /* = 0*/)
{
    HttPrivate::OptimizeScheimpflugTransformResults res;

    if (m_debug)
        log->debug(tag + "Optimize Scheimpflug Transformation");
    // Initialize the Scheimpflug Transform
    init(R, H);

    // Preprocessing
    shared_ptr<Profile<int>> profileCleaned =
        preProcess(inputImage, "", HttProcessType::calibration, ModelType::MiniBevelModelType);

    if (!profileCleaned)
    {
        res.valid = false;
        return res;
    } else
    {
        m_rcode = ReturnCode::PREPROCESS_SUCCESSFUL;
    }

    shared_ptr<OptimizationEngine::ScheimpflugTransformOutput> output =
        optimizationEngine->optimizeScheimpflugTransform(m_st, profileCleaned, imgX, imgY);
    res.valid = output->validCalibration;
    m_rcode = output->rcode;

    if (output->validCalibration)
    {
        map<string, bool> validation = checkCalibratedParamsWithNominalRanges();
        {
            stringstream ss;
            ss << left << boolalpha;
            ss << setw(13) << validation["alpha"] << setw(13) << validation["beta"] << setw(13) << validation["cx0"]
               << setw(13) << validation["cy0"] << setw(13) << validation["delta"] << setw(13) << validation["p1"]
               << setw(13) << validation["p2"] << setw(13) << validation["phi"] << setw(13) << validation["theta"]
               << setw(13) << validation["tt"];
            log->info(tag + ss.str());
        }
    }

    // FIXME: 2020-12-02
    // Re-initialize the Scheimpflug transformation with the new calibrated parameters and not the
    // nominal ones.
    init(R, H);

    if (output->validCalibration)
    {
        res.optimizationOutput = output;
        if (output != nullptr && output->bevel != nullptr)
            res.bevelImage = generateBevelImage(inputImage, output->bevel, profileCleaned);
    }

    return res;
}

void OptimizationEngine::printMetrics(shared_ptr<vector<shared_ptr<MiniBevelModel>>> modelSet, double B_star,
                                      double M_star, double angle1_star, double angle2_star, int kTraining)
{
    //
    double avgB = 0.0, avgM = 0.0, avgAngle1 = 0.0, avgAngle2 = 0.0;
    double avgBerr = 0.0, avgMerr = 0.0, varBerr = 0.0, varMerr = 0.0;
    double maxAbsBerr = 0.0, maxAbsMerr = 0.0;

    int nImages = modelSet->size();

    double trainAvgB = 0.0, trainAvgM = 0.0, trainAvgAngle1 = 0.0, trainAvgAngle2 = 0.0;
    double trainAvgBerr = 0.0, trainAvgMerr = 0.0, trainVarBerr = 0.0, trainVarMerr = 0.0;
    double trainMaxAbsBerr = 0.0, trainMaxAbsMerr = 0.0;

    double valAvgB = 0.0, valAvgM = 0.0, valAvgAngle1 = 0.0, valAvgAngle2 = 0.0;
    double valAvgBerr = 0.0, valAvgMerr = 0.0, valVarBerr = 0.0, valVarMerr = 0.0;
    double valMaxAbsBerr = 0.0, valMaxAbsMerr = 0.0;

    for (int k = 0; k < nImages; ++k)
    {
        // Get the k-th Scheimpflug model which was initialized with the
        // acquisition angles of the k-th image.
        shared_ptr<MiniBevelModel> model = modelSet->at(k);
        model->computeObjPoints();
        double B, M, angle1, angle2;
        model->getTestMeasures(B, M, angle1, angle2);

        // All dataset statistics
        avgB += B;
        avgM += M;
        avgAngle1 += angle1;
        avgAngle2 += angle2;

        avgBerr += B_star - B;
        avgMerr += M_star - M;
        varBerr += (B_star - B) * (B_star - B);
        varMerr += (M_star - M) * (M_star - M);

        double absBerr = fabs(B_star - B);
        double absMerr = fabs(M_star - M);

        if (absBerr > maxAbsBerr)
            maxAbsBerr = absBerr;
        if (absMerr > maxAbsMerr)
            maxAbsMerr = absMerr;

        if (k < kTraining)
        {
            // Training set
            trainAvgB += B;
            trainAvgM += M;
            trainAvgAngle1 += angle1;
            trainAvgAngle2 += angle2;

            trainAvgBerr += B_star - B;
            trainAvgMerr += M_star - M;
            trainVarBerr += (B_star - B) * (B_star - B);
            trainVarMerr += (M_star - M) * (M_star - M);

            double absBerr = fabs(B_star - B);
            double absMerr = fabs(M_star - M);

            if (absBerr > trainMaxAbsBerr)
                trainMaxAbsBerr = absBerr;
            if (absMerr > trainMaxAbsMerr)
                trainMaxAbsMerr = absMerr;
        }
#if 0
        else
        {
            // Validation set
            valAvgB += B;
            valAvgM += M;
            valAvgAngle1 += angle1;
            valAvgAngle2 += angle2;

            valAvgBerr += B_star - B;
            valAvgMerr += M_star - M;
            valVarBerr += (B_star - B) * (B_star - B);
            valVarMerr += (M_star - M) * (M_star - M);

            double absBerr = fabs(B_star - B);
            double absMerr = fabs(M_star - M);

            if (absBerr > valMaxAbsBerr)
                valMaxAbsBerr = absBerr;
            if (absMerr > valMaxAbsMerr)
                valMaxAbsMerr = absMerr;
        }
#endif

        /*{
            stringstream s;
            s << tag << "B: " << B << " M: " << M << " a1: " << angle1 << " (" << angle1/3.14*180
              << "°) a2: " << angle2 << " (" << angle2/3.14*180 << "°)"
              << " Berr: " << B_star - B << " Merr: " << M_star - M;
            log->info(s.str());
            s.str("");

            s << tag << "-> DeltaN: " << nominalR << " Estimated DeltaN: " << R;
            log->info(s.str());
        }*/
    }

    //  FIXME controllare
    avgBerr /= nImages;
    avgMerr /= nImages;

    if (nImages > 1)
    {
        varBerr /= (nImages - 1);
        varMerr /= (nImages - 1);
        varBerr -= avgBerr * avgBerr;
        varMerr -= avgMerr * avgMerr;
    }

    avgB /= nImages;
    avgM /= nImages;
    avgAngle1 /= nImages;
    avgAngle2 /= nImages;

    // Training set
    if (kTraining > 0)
    {
        trainAvgBerr /= kTraining;
        trainAvgMerr /= kTraining;

        if (kTraining > 1)
        {
            trainVarBerr /= (kTraining - 1);
            trainVarMerr /= (kTraining - 1);
            trainVarBerr -= trainAvgBerr * trainAvgBerr;
            trainVarMerr -= trainAvgMerr * trainAvgMerr;
        }

        trainAvgB /= kTraining;
        trainAvgM /= kTraining;
        trainAvgAngle1 /= kTraining;
        trainAvgAngle2 /= kTraining;
    }

#if 0
    // Validation set
    int kValidation = nImages - kTraining;
    if (kValidation > 0)
    {
        valAvgBerr /= kValidation;
        valAvgMerr /= kValidation;

        if (kValidation > 1)
        {
            valVarBerr /= (kValidation - 1);
            valVarMerr /= (kValidation - 1);
            valVarBerr -= valAvgBerr * valAvgBerr;
            valVarMerr -= valAvgMerr * valAvgMerr;
        }

        valAvgB /= kValidation;
        valAvgM /= kValidation;
        valAvgAngle1 /= kValidation;
        valAvgAngle2 /= kValidation;
    }
#endif

    {
        log->info(tag + "Optimization results [ALL] ---------------");
        stringstream s;

        s << tag << "Avg B:      " << avgB << " Exp: " << B_star << " Err: " << B_star - avgB;
        log->info(s.str());
        s.str("");
        s << tag << "Avg M:      " << avgM << " Exp: " << M_star << " Err: " << M_star - avgM;
        log->info(s.str());
        s.str("");
        s << tag << "Avg Angle1: " << avgAngle1 / 3.14 * 180.0 << " Exp: " << angle1_star / 3.14 * 180.0;
        log->info(s.str());
        s.str("");
        s << tag << "Avg Angle2: " << avgAngle2 / 3.14 * 180.0 << " Exp: " << angle2_star / 3.14 * 180.0;
        log->info(s.str());
        s.str("");

        s << tag << "AvgBerr: " << avgBerr << " (" << varBerr << ")";
        log->info(s.str());
        s.str("");
        s << tag << "AvgMerr: " << avgMerr << " (" << varMerr << ")";
        log->info(s.str());
        s.str("");
        s << tag << "MaxAbs(Berr): " << maxAbsBerr;
        log->info(s.str());
        s.str("");
        s << tag << "MaxAbs(Merr): " << maxAbsMerr;
        log->info(s.str());
        s.str("");

        log->info(tag + "Optimization results [TRAINING] ---------------");
        s << tag << "Avg B:      " << trainAvgB << " Exp: " << B_star << " Err: " << B_star - trainAvgB;
        log->info(s.str());
        s.str("");
        s << tag << "Avg M:      " << trainAvgM << " Exp: " << M_star << " Err: " << M_star - trainAvgM;
        log->info(s.str());
        s.str("");
        s << tag << "Avg Angle1: " << trainAvgAngle1 / 3.14 * 180.0 << " Exp: " << angle1_star / 3.14 * 180.0;
        log->info(s.str());
        s.str("");
        s << tag << "Avg Angle2: " << trainAvgAngle2 / 3.14 * 180.0 << " Exp: " << angle2_star / 3.14 * 180.0;
        log->info(s.str());
        s.str("");

        s << tag << "AvgBerr: " << trainAvgBerr << " (" << trainVarBerr << ")";
        log->info(s.str());
        s.str("");
        s << tag << "AvgMerr: " << trainAvgMerr << " (" << trainVarMerr << ")";
        log->info(s.str());
        s.str("");
        s << tag << "MaxAbs(Berr): " << trainMaxAbsBerr;
        log->info(s.str());
        s.str("");
        s << tag << "MaxAbs(Merr): " << trainMaxAbsMerr;
        log->info(s.str());
        s.str("");

#if 0
        log->info(tag + "Optimization results [VALIDATION] ---------------");
        s << tag << "Avg B:      " << valAvgB << " Exp: " << B_star << " Err: " << B_star - valAvgB;
        log->info(s.str());
        s.str("");
        s << tag << "Avg M:      " << valAvgM << " Exp: " << M_star << " Err: " << M_star - valAvgM;
        log->info(s.str());
        s.str("");
        s << tag << "Avg Angle1: " << valAvgAngle1 / 3.14 * 180.0 << " Exp'ed: " << angle1_star / 3.14 * 180.0;
        log->info(s.str());
        s.str("");
        s << tag << "Avg Angle2: " << valAvgAngle2 / 3.14 * 180.0 << " Exp'ed: " << angle2_star / 3.14 * 180.0;
        log->info(s.str());
        s.str("");

        s << tag << "AvgBerr: " << valAvgBerr << " (" << valVarBerr << ")";
        log->info(s.str());
        s.str("");
        s << tag << "AvgMerr: " << valAvgMerr << " (" << valVarMerr << ")";
        log->info(s.str());
        s.str("");
        s << tag << "MaxAbs(Berr): " << valMaxAbsBerr;
        log->info(s.str());
        s.str("");
        s << tag << "MaxAbs(Merr): " << valMaxAbsMerr;
        log->info(s.str());
        s.str("");
#endif
    }
}

HttPrivate::OptimizeScheimpflugTransformResults HttPrivate::optimizeScheimpflugTransform(
    vector<shared_ptr<Image<uint8_t>>>& imageSet, const vector<shared_ptr<vector<float>>>& imgXSet,
    const vector<shared_ptr<vector<float>>>& imgYSet, const vector<double>& deltaNSet, const vector<double>& HSet,
    vector<shared_ptr<Profile<int>>>& profileSet)
{
    HttPrivate::OptimizeScheimpflugTransformResults res;

    if (m_debug)
        log->debug(tag + "Optimize Scheimpflug Transformation");

    // Create sets of Scheimpflug transforms and profiles, one for each image.
    vector<shared_ptr<ScheimpflugTransform>> stSet;
    // vector<shared_ptr<Profile<int>>> profileSet;

    const int nImages = imageSet.size();
    for (int k = 0; k < nImages; ++k)
    {
        // Create the Scheimpflug transform for the k-th image.
        shared_ptr<ScheimpflugTransform> st(new ScheimpflugTransform());

        // Initialize the Scheimpflug Transform with the angles of the
        // k-th image.
        init(st, deltaNSet[k], HSet[k]);

        // st->setNominalStylusOrientation(deltaNSet[k], HSet[k]); // FIXME experimental
        // st->setStylusOrientation(deltaNSet[k], HSet[k]); // FIXME redundant
        // Preprocessing: observe that this operation is only necessary for having
        // a valid profile in the model. The actual profile was computed before.
        // FIXME: we may pass the pre-computed profile in the method signature and
        //        not recomputing the profile here.
        // shared_ptr<Profile<int>> profileCleaned = preProcess(imageSet[k], "",
        //                            HttProcessType::calibration,
        //                            ModelType::MiniBevelModelType);

        /* if (!profileCleaned)
         {
             cout << "You got a serious problem...." << endl;
             res.valid = false;
             // FIXME this should not fail
             return res;
         }
         else*/
        {
            // profileSet.push_back(profileCleaned);
            stSet.push_back(st);
            m_rcode = ReturnCode::PREPROCESS_SUCCESSFUL;
        }
    }

    // ADG-Multi: I believe that the passed profile is not relevant for the optimization, it is only used for
    // having a well-defined model. Hence, I pass the first profile
    shared_ptr<OptimizationEngine::ScheimpflugTransformOutput> output =
        optimizationEngine->optimizeScheimpflugTransform(stSet, profileSet, imgXSet, imgYSet);

    if (output->validCalibration)
    {
        map<string, bool> validation = checkCalibratedParamsWithNominalRanges();
        {
            stringstream ss;
            ss << left << boolalpha;
            ss << setw(13) << validation["alpha"] << setw(13) << validation["beta"] << setw(13) << validation["cx0"]
               << setw(13) << validation["cy0"] << setw(13) << validation["delta"] << setw(13) << validation["p1"]
               << setw(13) << validation["p2"] << setw(13) << validation["phi"] << setw(13) << validation["theta"]
               << setw(13) << validation["tt"];
            log->info(tag + ss.str());
        }
    }

    res.valid = output->validCalibration;
    m_rcode = output->rcode;

    // FIXME: qui in realtà possiamo prendere solo una deele immagini di riferimento, ad esempio la prima
    // valida.

    // FIXME: 2020-12-02
    // Re-initialize the Scheimpflug transformation with the new calibrated parameters and not the
    // nominal ones.

    // FIXME: here we are using the old code. Let us initialize m_st with the parameters valid
    // for the first image.
    init(deltaNSet[0], HSet[0]);

    if (output->validCalibration)
    {
        res.optimizationOutput = output;
        if (output != nullptr && output->bevel != nullptr)
            res.bevelImage = generateBevelImage(imageSet[0], output->bevel, profileSet[0]);
    }

    return res;
}

HttPrivate::CalibrationGridResults HttPrivate::calibrateGrid(shared_ptr<Image<uint8_t>> inputImage,
                                                             bool saveValues /*= true*/)
{
    Json::Value settingsJson = settings->getSettings();
    Json::Value calibrationJson = settings->getCalibration();

    HttPrivate::CalibrationGridResults res;

    shared_ptr<Image<uint8_t>> singleChannelImage = nullptr;

    if (inputImage->getNChannels() > 1)
    {
        if (m_debug)
            log->debug(tag + "Extract singleChannel from input image");
        uint8_t channel = settingsJson["channel_section"]["selectedChannel"].asUInt();

        // Get the selected channel
        singleChannelImage = imgPreProc->getChannel(inputImage, channel);
    } else
    {
        // We already have a single channel image. Since the singleChannelImage is later
        // converted to floating point we are sure that inputImage won't be changed and
        // we do not need a deep copy.
        singleChannelImage = inputImage;
    }

    // Conversion to float
    shared_ptr<Image<float>> floatSingleChannelImage = imageToFloat(singleChannelImage);

    // Normalization (not strictly necessary)
    imgPreProc->normalize(floatSingleChannelImage);

    // Difference of Gaussian
    float sigma1 = settingsJson["scheimpflug_transformation_chart"]["dog_sigma1"].asFloat();
    float sigma2 = settingsJson["scheimpflug_transformation_chart"]["dog_sigma2"].asFloat();
    float threshold = settingsJson["scheimpflug_transformation_chart"]["dog_threshold"].asFloat();

    // TODO: remove test code
    // string foldert = settingsJson["test"]["outputFolder"].asString();
    // cout << foldert << endl;
    // saveImage(foldert, "test0.png", floatSingleChannelImage);

    shared_ptr<Image<float>> dog = imgPreProc->differenceOfGaussian(floatSingleChannelImage, sigma1, sigma2);

    // TODO: remove test code
    // saveImage(foldert, "test1.png", dog);

    // Normalization of the difference of gaussians
    imgPreProc->normalize(dog);

    // TODO: remove test code
    // saveImage(foldert, "test2.png", dog);

    shared_ptr<Image<float>> dogNormalized(new Image<float>(*dog.get()));

    // Binarization the normalized dog
    // imgPreProc->binarize(dog, threshold);
    if (!imgPreProc->adaptiveBinarize(dog, threshold))
    {
        m_rcode = ReturnCode::CALIBRATION_FAILED_GRID;
        res.returnCode = m_rcode;
        return res;
    }

    // TODO: remove test code
    // cout << "Threshold = " << threshold << endl;
    // saveImage(foldert, "test3.png", dog);

    // Hough transform
    shared_ptr<Hough> HT(new Hough());
    shared_ptr<Image<float>> hough = HT->transform(dog);
    // Normalization of the hough transform
    imgPreProc->normalize(hough);

    // TODO: remove test code
    // cout << "Threshold = " << threshold << endl;
    // saveImage(foldert, "test4.png", hough);

    // Procedure:
    // - getPeaks
    // - getLines
    // - getIntersectionPoints
    float houghThreshold = settingsJson["scheimpflug_transformation_chart"]["hough_threshold"].asFloat();
    int houghPeaks = settingsJson["scheimpflug_transformation_chart"]["hough_peaks"].asUInt();
    // HT->getPeaks(houghThreshold, houghPeaks);
    HT->adaptiveGetPeaks(houghThreshold, houghPeaks);
    vector<pair<pair<int, int>, pair<int, int>>> lines = HT->getLines();

    vector<int> imgX;
    vector<int> imgY;
    HT->getIntersectionPoints(imgX, imgY);

    /**
     * Improvment for correct the peaks detection obtain with Hough transformation
     *
     * For each point of intersection:
     * - Take the 2 lines that define it.
     * - Construct an all-zero kernel with the 2 drawn lines.
     * - We filter a portion of the image (dog) centered at the point of intersection with the kernel.
     * - On the filtered image we have a maximum at the point where the kernel overlaps the grid lines and then we have
     * the correction offset of the intersection point.
     */

    int nLines = lines.size();
    int startH = 0;
    int stopH = (nLines >> 1) - 1;
    int startV = stopH + 1;
    int stopV = nLines - 1;

    int nPoints = imgX.size();
    int halfNPoints = nPoints >> 1;

    int width = inputImage->getWidth();
    int height = inputImage->getHeight();

    for (int p = 0; p < nPoints; ++p)
    {
        if (imgX[p] < 0 || imgX[p] > width || imgY[p] < 0 || imgY[p] > height)
        {
#if 0
            stringstream ss;
            ss << "(" << imgX[p] << "," << imgY[p] << ") out of boundary, skip optimization";
            log->debug(tag + ss.str());
            continue;
#else
            m_rcode = ReturnCode::CALIBRATION_FAILED_LINE_DETECTION;
            res.returnCode = m_rcode;
            return res;
#endif
        }

        // cout << p << ": (" << imgX[p] << "," << imgY[p] << ")";
        int hIndex = 0;
        int vIndex = 0;

        if (p == nPoints - 1)
        {
            // Point 28 (central)
            int cIndex = (stopH - startH) >> 1;
            hIndex = startH + cIndex; // r3
            vIndex = startV + cIndex; // r10
        } else
        {
            if (p < halfNPoints)
            {
                // Points from 0 to 13
                if (p % 2)
                {
                    // odd value
                    hIndex = stopH; // r6
                    vIndex = (p >> 1) + startV;
                } else
                {
                    // even value
                    hIndex = startH; // r0
                    vIndex = (p >> 1) + startV;
                }
            } else
            {
                // Points from 14 to 27
                int index = p - halfNPoints; // from 0 to 13
                if (index % 2)
                {
                    // odd value
                    hIndex = (index >> 1);
                    vIndex = stopV; // r13
                } else
                {
                    // even value
                    hIndex = (index >> 1);
                    vIndex = startV; // r7
                }
            }
        }

        // cout << p << ": r" << hIndex << " + r" << vIndex << endl;
        // cout << "------" << endl;
        // for (auto i = 0; i < lines.size(); ++i)
        // {
        //     auto it = &lines[i];
        //     cout << i << " (" << it->first.first << "," << it->first.second << ") (" << it->second.first << ","
        //          << it->second.second << ")" << endl;
        // }
        // cout << "------" << endl;

        // Horizontal line
        // y = m*x + q
        float xA = lines[hIndex].first.first;
        float yA = lines[hIndex].first.second;
        float xB = lines[hIndex].second.first;
        float yB = lines[hIndex].second.second;
        float m1 = (yB - yA) / (xB - xA);

        // Vertical line
        // x = m*y + q
        float xC = lines[vIndex].first.first;
        float yC = lines[vIndex].first.second;
        float xD = lines[vIndex].second.first;
        float yD = lines[vIndex].second.second;
        float m2 = (xD - xC) / (yD - yC);

        // Kernel 21x21, the central point is the intersection of the two lines
        int stride = 21;
        const int kernelWidth = 21;
        const int kernelHeight = 21;
        const int kernelSize = kernelWidth * kernelHeight;
        float matrix[kernelSize];
        memset(matrix, 0, kernelSize * sizeof(float));

        // X0(0,0) -> (11,11)
        int x0 = 10;
        int y0 = 10;
        int count = 0;

        for (int i = 0; i < kernelWidth; ++i)
        {
            float yf = m1 * (i - x0);
            int y = (int)floor(yf + 0.5) + y0;
            if (y >= 0 && y < kernelHeight)
                matrix[i + y * stride] = 1;
        }

        for (int i = 0; i < kernelHeight; ++i)
        {
            float xf = m2 * (i - y0);
            int x = (int)floor(xf + 0.5) + x0;
            if (x >= 0 && x < kernelWidth)
                matrix[x + i * stride] = 1;
        }

        for (int i = 0; i < kernelSize; ++i)
        {
            if (matrix[i] > 0)
                ++count;
        }

        if (count)
        {
            for (int i = 0; i < kernelSize; ++i)
            {

                matrix[i] /= count;
            }
        }

        // cout << "---- kernel ----" << endl;
        // for (int i = 0; i < 21; ++i)
        // {
        //     int offset = i * stride;
        //     for (int j = 0; j < 21; ++j)
        //         cout << matrix[j + offset] << " ";
        //     cout << endl;
        // }
        // cout << "---- ------ ----" << endl;

        int roiWidth = 30;
        int roiHeight = 30;
        int roiX0 = imgX[p] - (roiWidth >> 1);
        int roiY0 = imgY[p] - (roiHeight >> 1);

        // cout << ">>>>>>> " << p << " imgX[p]: " << imgX[p] << " imgY[p]: " << imgY[p] << " roiX0: " << roiX0
        //      << " roiY0: " << roiY0 << endl;

        if (roiX0 < 0 || roiY0 < 0)
        {
#if 0
            stringstream ss;
            ss << "roi (" << roiX0 << "," << roiY0 << ") out of boundary, skip filtering";
            log->debug(tag + ss.str());
            continue;
#else
            m_rcode = ReturnCode::CALIBRATION_FAILED_LINE_DETECTION;
            res.returnCode = m_rcode;
            return res;
#endif
        }

        shared_ptr<Image<float>> dogFiltered =
            imgPreProc->filter(dogNormalized, matrix, kernelWidth, kernelHeight, roiX0, roiY0, roiWidth, roiHeight);
        // string fName = "test3f_" + to_string(p) + ".png";
        // saveImage(foldert, fName, dogFiltered);

        float* pDogFiltered = dogFiltered->getData();
        int dogFilteredStride = dogFiltered->getStride() / sizeof(float);

        {
            // cout << p << ": (" << imgX[p] << "," << imgY[p] << ") " << endl;
            // Find the max value in a window 5x5 centred in (imgX[i],imgY[i])
            float maxValue = 0;
            int maxX = imgX[p];
            int maxY = imgY[p];
            for (int yi = -5; yi <= 5; ++yi)
            {
                for (int xi = -5; xi <= 5; ++xi)
                {
                    // cout << imgX[p] + xi << " " << imgY[p] + yi << " "
                    //      << ((imgX[p] + xi) + (imgY[p] + yi) * dogFilteredStride) << endl;
                    float currValue = pDogFiltered[(imgX[p] + xi) + (imgY[p] + yi) * dogFilteredStride];
                    if (currValue > maxValue)
                    {
                        maxValue = currValue;
                        maxX = imgX[p] + xi;
                        maxY = imgY[p] + yi;
                    }
                }
            }
            cout << p << ": (" << imgX[p] << "," << imgY[p] << ")";
            imgX[p] = maxX;
            imgY[p] = maxY;
            cout << " new: (" << imgX[p] << "," << imgY[p] << ")" << endl;
        }
    }

    // Update line
    int indexOffset = (stopH + 1) * 2;
    for (int l = startH; l <= stopH; ++l)
    {
        int a = l * 2 + indexOffset;
        int b = a + 1;
        // y = m*x + q
        // cout << a << " " << b << " " << imgX[a] << " " << imgY[a] << " " << imgX[b] << " " << imgY[b] << endl;
        float m = ((float)(imgY[b] - imgY[a]) / ((float)(imgX[b] - imgX[a])));
        float q = (float)imgY[a] - m * (float)imgX[a];
        // cout << "m:" << m << " q:" << q << endl;
        // x1=0   -> y1=q
        // x2=560 -> y2=m*x2+q
        float x1 = 0;
        float y1 = q;
        if (y1 > height)
        {
            // x=(y-q)/m
            y1 = height;
            x1 = (y1 - q) / m;
        }
        float x2 = width;
        float y2 = (x2 * m) + q;
        if (y2 < 0)
        {
            // x=-q/m
            y2 = 0;
            x2 = -q / m;
        }
        // cout << l << ": (" << x1 << "," << y1 << ") (" << x2 << "," << y2 << ")" << endl;
        lines[l].first.first = (int)floor(x1 + 0.5);
        lines[l].first.second = (int)floor(y1 + 0.5);
        lines[l].second.first = (int)floor(x2 + 0.5);
        lines[l].second.second = (int)floor(y2 + 0.5);
    }

    // cout << "----------------------" << endl;

    indexOffset = -startV;
    for (int l = startV; l <= stopV; ++l)
    {
        int a = (l + indexOffset) * 2;
        int b = a + 1;
        // x = m*y + q
        // cout << a << " " << b << " " << imgX[a] << " " << imgY[a] << " " << imgX[b] << " " << imgY[b] << endl;
        float m = ((float)(imgX[b] - imgX[a]) / ((float)(imgY[b] - imgY[a])));
        float q = (float)imgX[a] - m * (float)imgY[a];
        // cout << "m:" << m << " q:" << q << endl;
        // y1=560 -> x1=m*y1+q
        float y1 = height;
        float x1 = (y1 * m) + q;
        if (x1 < 0)
        {
            x1 = 0;
            y1 = -q / m;
        }
        // y2=0 -> x2=q
        float y2 = 0;
        float x2 = q;
        if (x2 > width)
        {
            x2 = width;
            y2 = (x2 - q) / m;
        }
        // cout << l << ": (" << x1 << "," << y1 << ") (" << x2 << "," << y2 << ")" << endl;
        lines[l].first.first = (int)floor(x1 + 0.5);
        lines[l].first.second = (int)floor(y1 + 0.5);
        lines[l].second.first = (int)floor(x2 + 0.5);
        lines[l].second.second = (int)floor(y2 + 0.5);
    }

    // TODO: remove test code
    // Center
    // cout << "Center point " << imgX[imgX.size() - 1] << ", " << imgY[imgY.size() - 1] << endl;
    // imgPreProc->binarize(hough, 0.65);

    // Save image on disk
    bool saveImages = settingsJson["test"]["storeImages"].asBool();
    if (saveImages)
    {
        string folder = settingsJson["test"]["outputFolder"].asString();
        // shared_ptr<Image<uint8_t>> calibrationGrid = imgPreProc->toUint8(dog);
        shared_ptr<Image<uint8_t>> calibrationGrid(new Image<uint8_t>(*inputImage));
        drawProfileImgPlane(calibrationGrid, imgX, imgY, colorRed, 3);
        saveImage(folder, "calibrationGrid.png", calibrationGrid);

        if (m_debug)
        {
            saveImage(folder, "dog.png", dog);
            saveImage(folder, "hough.png", hough);

            shared_ptr<Image<float>> gridImage(new Image<float>(inputImage->getWidth(), inputImage->getHeight(), 1));
            traceCalibrationGrid(gridImage, lines);
            saveImage(folder, "grid.png", gridImage);
        }
    }

    // ############################################################################################################

    // Initialize the Scheimpflug Transform
    double R = calibrationJson["scheimpflug_transformation_calibration"]["grid"]["deltaN"].asDouble(); // 0;
    // FIXME - H:12.8 in FM1; H:0.0 in FM2
    double H = calibrationJson["scheimpflug_transformation_calibration"]["grid"]["H"].asDouble();
    init(R, H, true); // true -> Initialize using calibration mode
    shared_ptr<OptimizationEngine::CalibrationGridOutput> output =
        optimizationEngine->optimizeCalibrationGrid(m_st, imgX, imgY, saveValues);
    m_rcode = output->rcode;

    if (output->validCalibration)
    {
        map<string, bool> validation = checkCalibratedParamsWithNominalRanges();
        {
            stringstream ss;
            ss << left << boolalpha;
            ss << setw(13) << validation["alpha"] << setw(13) << validation["beta"] << setw(13) << validation["cx0"]
               << setw(13) << validation["cy0"] << setw(13) << validation["delta"] << setw(13) << validation["p1"]
               << setw(13) << validation["p2"] << setw(13) << validation["phi"] << setw(13) << validation["theta"]
               << setw(13) << validation["tt"];
            log->info(tag + ss.str());
        }
    }

    // Re-initialize the Scheimpflug transformation with the new calibrated parameters and not the
    // nominal ones.
    init(R, H, false); // true -> Initialize using calibration mode

    int p1 = (startV + (stopV - startV) / 2) * 2;
    int p2 = p1 + 1;

    double aImgPoint[3] = {(double)imgX[p1], (double)imgY[p1], 0.0};
    double aObjPoint[3] = {0.0, 0.0, 0.0};
    m_st->imageToObject(aImgPoint, aObjPoint);

    double bImgPoint[3] = {(double)imgX[p2], (double)imgY[p2], 0.0};
    double bObjPoint[3] = {0.0, 0.0, 0.0};
    m_st->imageToObject(bImgPoint, bObjPoint);

    double deltaY = bObjPoint[1] - aObjPoint[1];
    double deltaX = bObjPoint[0] - aObjPoint[0];
    double L = sqrt((deltaX * deltaX) + (deltaY * deltaY));
    double estH = -(H + m_st->radToDeg(atanf(deltaY / deltaX)));
    {
        stringstream ss;
        ss << "Estimate H: " << estH << " deltaY:" << deltaY << " deltaX:" << deltaX << " L: " << L;
        log->debug(tag + ss.str());
    }

    init(R, estH, false); // true -> Initialize using calibration mode

    if (saveImages)
    {
        string folder = settingsJson["test"]["outputFolder"].asString();
        shared_ptr<Image<uint8_t>> gridCalibrated = generateImageObjPlane(inputImage);
        drawProfileObjPlane(gridCalibrated, imgX, imgY, colorRed, 3);
        saveImage(folder, "gridCalibrated.png", gridCalibrated);
    }

    // FIXME: calculation of the length of the sides of the square and diagolnals
    if (m_debug && output->validCalibration)
    {
        // L1 - r0 - P0-P12
        double aImg[3] = {(double)imgX[0], (double)imgY[0], 0};
        double bImg[3] = {(double)imgX[12], (double)imgY[12], 0};
        double aObj[3] = {0.0, 0.0, 0.0};
        double bObj[3] = {0.0, 0.0, 0.0};
        m_st->imageToObject(aImg, aObj);
        m_st->imageToObject(bImg, bObj);
        double deltaY = bObj[1] - aObj[1];
        double deltaX = bObj[0] - aObj[0];
        double L1 = sqrt((deltaX * deltaX) + (deltaY * deltaY));
        // cout << deltaY << " " << deltaX << " " << L1 << endl;

        // L2 - r13 - P12-13
        aImg[0] = imgX[12];
        aImg[1] = imgY[12];
        bImg[0] = imgX[13];
        bImg[1] = imgY[13];
        aObj[0] = 0.0;
        aObj[1] = 0.0;
        bObj[0] = 0.0;
        bObj[1] = 0.0;
        m_st->imageToObject(aImg, aObj);
        m_st->imageToObject(bImg, bObj);
        deltaY = bObj[1] - aObj[1];
        deltaX = bObj[0] - aObj[0];
        double L2 = sqrt((deltaX * deltaX) + (deltaY * deltaY));

        // L3 - r6 - P26-P27
        aImg[0] = imgX[26];
        aImg[1] = imgY[26];
        bImg[0] = imgX[27];
        bImg[1] = imgY[27];
        aObj[0] = 0.0;
        aObj[1] = 0.0;
        bObj[0] = 0.0;
        bObj[1] = 0.0;
        m_st->imageToObject(aImg, aObj);
        m_st->imageToObject(bImg, bObj);
        deltaY = bObj[1] - aObj[1];
        deltaX = bObj[0] - aObj[0];
        double L3 = sqrt((deltaX * deltaX) + (deltaY * deltaY));

        // L4 - r7 - P0-P1
        aImg[0] = imgX[0];
        aImg[1] = imgY[0];
        bImg[0] = imgX[1];
        bImg[1] = imgY[1];
        aObj[0] = 0.0;
        aObj[1] = 0.0;
        bObj[0] = 0.0;
        bObj[1] = 0.0;
        m_st->imageToObject(aImg, aObj);
        m_st->imageToObject(bImg, bObj);
        deltaY = bObj[1] - aObj[1];
        deltaX = bObj[0] - aObj[0];
        double L4 = sqrt((deltaX * deltaX) + (deltaY * deltaY));

        float realL = 3.0;
        float horizontalErrorLThreshold = 16.0;
        float vericalErrorLThreshold = 10.0;
        float err1 = (L1 - realL) * 1000;
        res.gridErrors.push_back(err1);
        if (fabs(err1) > horizontalErrorLThreshold)
        {
            stringstream strs;
            strs << "L1 err: " << err1 << " [um]";
            log->warn(tag + strs.str());
        }
        float err2 = (L2 - realL) * 1000;
        ;
        res.gridErrors.push_back(err2);
        if (fabs(err2) > vericalErrorLThreshold)
        {
            stringstream strs;
            strs << "L2 err: " << err2 << " [um]";
            log->warn(tag + strs.str());
        }
        float err3 = (L3 - realL) * 1000;
        ;
        res.gridErrors.push_back(err3);
        if (fabs(err3) > horizontalErrorLThreshold)
        {
            stringstream strs;
            strs << "L3 err: " << err3 << " [um]";
            log->warn(tag + strs.str());
        }
        float err4 = (L4 - realL) * 1000;
        ;
        res.gridErrors.push_back(err4);
        if (fabs(err4) > vericalErrorLThreshold)
        {
            stringstream strs;
            strs << "L4 err: " << err4 << " [um]";
            log->warn(tag + strs.str());
        }

        // D1 - P0-P13
        aImg[0] = imgX[0];
        aImg[1] = imgY[0];
        bImg[0] = imgX[13];
        bImg[1] = imgY[13];
        aObj[0] = 0.0;
        aObj[1] = 0.0;
        bObj[0] = 0.0;
        bObj[1] = 0.0;
        m_st->imageToObject(aImg, aObj);
        m_st->imageToObject(bImg, bObj);
        deltaY = bObj[1] - aObj[1];
        deltaX = bObj[0] - aObj[0];
        double D1 = sqrt((deltaX * deltaX) + (deltaY * deltaY));

        // D2 - P1-P12
        aImg[0] = imgX[1];
        aImg[1] = imgY[1];
        bImg[0] = imgX[12];
        bImg[1] = imgY[12];
        aObj[0] = 0.0;
        aObj[1] = 0.0;
        bObj[0] = 0.0;
        bObj[1] = 0.0;
        m_st->imageToObject(aImg, aObj);
        m_st->imageToObject(bImg, bObj);
        deltaY = bObj[1] - aObj[1];
        deltaX = bObj[0] - aObj[0];
        double D2 = sqrt((deltaX * deltaX) + (deltaY * deltaY));

        float realD = realL * sqrtf(2);
        float thresholdErrorD = 18.86796; // sqrt(10^2+16^2)
        float errD1 = (D1 - realD) * 1000;
        res.gridErrors.push_back(errD1);
        if (fabs(errD1) > thresholdErrorD)
        {
            stringstream strs;
            strs << "D1 err " << errD1;
            log->warn(tag + strs.str());
        }

        float errD2 = (D2 - realD) * 1000;
        res.gridErrors.push_back(errD2);
        if (fabs(errD2) > thresholdErrorD)
        {
            stringstream strs;
            strs << "D2 err " << errD2;
            log->warn(tag + strs.str());
        }

        {
            stringstream ss;
            ss << left << setw(13) << "errL1 and errL3 must be lesser than 16um";
            log->debug(tag + ss.str());
            ss.str("");
            ss << left << setw(13) << "errL2 and errL4 must be lesser than 10um";
            log->debug(tag + ss.str());
            ss.str("");
            ss << left << setw(13) << "errL1 [um]" << setw(13) << "errL2 [um]" << setw(13) << "errL3 [um]" << setw(13)
               << "errL4 [um]" << setw(13) << "errD1 [um]" << setw(13) << "errD2 [um]";
            log->debug(tag + ss.str());
            ss.str("");
            ss << left << setw(13) << err1 << setw(13) << err2 << setw(13) << err3 << setw(13) << err4 << setw(13)
               << errD1 << setw(13) << errD2;
            log->debug(tag + ss.str());

            ss.str("");
            ss << "centerX: " << output->params->cx0 << " (" << (280 - output->params->cx0)
               << ") centerY: " << output->params->cy0 << " (" << (280 - output->params->cy0) << ")";
            log->debug(tag + ss.str());
        }
    }

    // HttPrivate::CalibrationGridResults res;
    res.valid = output->validCalibration;
    res.scheimpflugTransformParams = output->params;
    res.lossValue = output->loss;
    res.imgPlaneGridLines = lines;
    res.objPlaneGrid = generateImageObjPlane(inputImage);
    // int halfWidth = inputImage->getWidth() * 0.5;
    // int halfHeight = inputImage->getHeight() * 0.5;
    for (int i = 0; i < (int)imgX.size(); ++i)
    {
        res.imgPlaneIntersectionPoints.push_back(pair<int, int>(imgX[i], imgY[i]));

        int halfWidth = res.objPlaneGrid->getWidth() * 0.5;
        int halfHeight = res.objPlaneGrid->getHeight() * 0.5;

        // Json::Value st = settings->getCalibration()["scheimpflug_transformation"];
        // cout << "cx0: " << st["cx0"].asDouble() << "cy0: " << st["cy0"].asDouble() << endl;

        // FIXME
        double srcImgPoint[3] = {(double)imgX[i], (double)imgY[i], 0.0};
        double dstObjPoint[3] = {0.0, 0.0, 0.0};
        m_st->imageToObject(srcImgPoint, dstObjPoint);
        int xPixel = (int)floor(m_st->mmToPixel(dstObjPoint[0]) + 0.5);
        int yPixel = (int)floor(m_st->mmToPixel(dstObjPoint[1]) + 0.5);

        xPixel += halfWidth;  // - 5;  // st["cx0"].asDouble(); // halfWidth;
        yPixel += halfHeight; // - 5; // st["cy0"].asDouble(); // halfHeight;
        res.objPlaneIntersectionPoints.push_back(pair<int, int>(xPixel, yPixel));
    }
    m_rcode = output->rcode;
    res.returnCode = m_rcode;
    return res;
}

shared_ptr<Image<uint8_t>> HttPrivate::generateBevelImageOnObjectPlane(shared_ptr<Image<uint8_t>> inputImage,
                                                                       shared_ptr<Profile<int>> bevel,
                                                                       shared_ptr<Profile<int>> profile)
{
    shared_ptr<Image<uint8_t>> objImage = nullptr;
    if (inputImage->getNChannels() == 1)
    {
        uint8_t* pImg = inputImage->getData();
        uint16_t inputHeight = inputImage->getHeight();
        uint16_t inputWidth = inputImage->getWidth();
        uint16_t inputStride = inputImage->getStride();

        shared_ptr<Image<uint8_t>> newImg(new Image<uint8_t>(inputWidth, inputHeight, 3));
        uint8_t* pNewImg = newImg->getData();
        uint16_t newStride = newImg->getStride();

        // IMAGE
        for (auto y = 0; y < inputHeight; ++y)
        {
            int startAddress = y * inputStride;
            int newStartAddress = y * newStride;
            for (auto x = 0; x < inputWidth; ++x)
            {
                int index = x * 3 + newStartAddress;
                int val = pImg[x + startAddress];
                pNewImg[index] = val;
                pNewImg[index + 1] = val;
                pNewImg[index + 2] = val;
            }
        }
        objImage = generateImageObjPlane(newImg);
    } else
        objImage = generateImageObjPlane(inputImage);

    if (profile != nullptr)
    {
        // PROFILE: white line
        drawProfileObjPlane(objImage, profile->x, profile->y, colorBlu);

        // MAXIMA POINTS: yellow points
        drawProfileObjPlane(objImage, profile->ancorImgX, profile->ancorImgY, colorYellow, 3);
    }

    if (bevel != nullptr)
    {
        // BEVEL: red line
        drawProfileObjPlane(objImage, bevel->x, bevel->y, colorRed);

        // MODEL POINT: green points
        drawProfileObjPlane(objImage, bevel->imgX, bevel->imgY, colorGreen, 3);

        // DISTANCE POINTS: purple points
        drawProfileObjPlane(objImage, bevel->distanceImgX, bevel->distanceImgY, colorPurple, 5);
    }

    return objImage;
}

shared_ptr<Image<uint8_t>> HttPrivate::generateBevelImage(shared_ptr<Image<uint8_t>> inputImage,
                                                          shared_ptr<Profile<int>> bevel,
                                                          shared_ptr<Profile<int>> profile)
{
    shared_ptr<Image<uint8_t>> bevelImg(new Image<uint8_t>(*inputImage));
    if (bevelImg->getNChannels() == 1)
    {
        uint8_t* pImg = bevelImg->getData();
        uint16_t inputHeight = bevelImg->getHeight();
        uint16_t inputWidth = bevelImg->getWidth();
        uint16_t inputStride = bevelImg->getStride();

        shared_ptr<Image<uint8_t>> newImg(new Image<uint8_t>(inputWidth, inputHeight, 3));
        uint8_t* pNewImg = newImg->getData();
        uint16_t newStride = newImg->getStride();

        // IMAGE
        for (auto y = 0; y < inputHeight; ++y)
        {
            int startAddress = y * inputStride;
            int newStartAddress = y * newStride;
            for (auto x = 0; x < inputWidth; ++x)
            {
                int index = x * 3 + newStartAddress;
                int val = pImg[x + startAddress];
                pNewImg[index] = val;
                pNewImg[index + 1] = val;
                pNewImg[index + 2] = val;
            }
        }

        bevelImg = newImg;
    }

    if (bevel != nullptr)
    {
        // Bevel profile in red
        drawProfileImgPlane(bevelImg, bevel->x, bevel->y, colorRed);

        // Model points onto the image plane in green
        drawProfileImgPlane(bevelImg, bevel->imgX, bevel->imgY, colorGreen, 3);

        // Model points onto the image plane in pink
        drawProfileImgPlane(bevelImg, bevel->distanceImgX, bevel->distanceImgY, colorPurple, 5);
    }

    if (profile != nullptr)
    {
        // Frame profile in white
        drawProfileImgPlane(bevelImg, profile->x, profile->y, colorBlu);

        // Highlits the 2 maxima over the profile
        drawProfileImgPlane(bevelImg, profile->ancorImgX, profile->ancorImgY, colorYellow, 3);
    }

    return bevelImg;
}

shared_ptr<Image<uint8_t>> HttPrivate::generateImageObjPlane(shared_ptr<Image<uint8_t>> inputImage)
{
    uint8_t* pImg = inputImage->getData();
    uint8_t nChannels = inputImage->getNChannels();
    uint16_t height = inputImage->getHeight();
    uint16_t width = inputImage->getWidth();
    uint16_t stride = inputImage->getStride();
    // int halfWidth = width / 2;
    // int halfHeight = height / 2;

    // cout << "generateImageObjPlane " << halfHeight << " " << halfWidth << endl;

    Json::Value st = settings->getCalibration()["scheimpflug_transformation"];
    double cx0 = st["cx0"].asDouble();
    double cy0 = st["cy0"].asDouble();

    double srcImgPoint[3] = {cx0, cy0, 0.0};
    double dstObjPoint[3] = {0.0, 0.0, 0.0};
    m_st->imageToObject(srcImgPoint, dstObjPoint);
    // cout << dstObjPoint[0] << " " << dstObjPoint[1] << endl;

    int dstWidth = (int)floor(m_st->mmToPixel(5.5) + 0.5);
    int dstHeight = (int)floor(m_st->mmToPixel(5.5) + 0.5);

    int halfWidth = dstWidth / 2;
    int halfHeight = dstHeight / 2;

    shared_ptr<Image<uint8_t>> newImg(new Image<uint8_t>(dstWidth, dstHeight, nChannels));
    uint8_t* pNewImg = newImg->getData();
    uint16_t newStride = newImg->getStride();

    // IMAGE
    if (nChannels == 1)
    {

        for (auto y = 0; y < dstHeight; ++y)
        {
            int startAddress = y * newStride;
            for (auto x = 0; x < dstWidth; ++x)
            {
                double xMm = m_st->pixelToMm((double)x - halfWidth);
                double yMm = m_st->pixelToMm((double)y - halfHeight);

                double srcObjPoint[3] = {xMm, yMm, 0.0};
                double dstImgPoint[3] = {0.0, 0.0, 0.0};
                m_st->objectToImage(srcObjPoint, dstImgPoint, false);

                int xPixel = (int)floor(dstImgPoint[0] + 0.5);
                int yPixel = (int)floor(dstImgPoint[1] + 0.5);

                if (xPixel >= 0 && xPixel < width && yPixel >= 0 && yPixel < height)
                {
                    int i = startAddress + x * nChannels;
                    int index = xPixel * nChannels;
                    index += yPixel * stride;
                    pNewImg[i] = pImg[index];
                }
            }
        }
    } else //(nChannels == 3)
    {

        for (auto y = 0; y < dstHeight; ++y)
        {
            int startAddress = y * newStride;
            for (auto x = 0; x < dstWidth; ++x)
            {
                double xMm = m_st->pixelToMm((double)x - halfWidth);
                double yMm = m_st->pixelToMm((double)y - halfHeight);

                double srcObjPoint[3] = {xMm, yMm, 0.0};
                double dstImgPoint[3] = {0.0, 0.0, 0.0};
                m_st->objectToImage(srcObjPoint, dstImgPoint, false);

                int xPixel = (int)floor(dstImgPoint[0] + 0.5);
                int yPixel = (int)floor(dstImgPoint[1] + 0.5);

                if (xPixel >= 0 && xPixel < width && yPixel >= 0 && yPixel < height)
                {
                    int i = startAddress + x * nChannels;
                    int index = xPixel * nChannels;
                    index += yPixel * stride;
                    pNewImg[i] = pImg[index];
                    pNewImg[i + 1] = pImg[index + 1];
                    pNewImg[i + 2] = pImg[index + 2];
                }
            }
        }
    }

    return newImg;
}

void HttPrivate::convertPointsFromImgToObjPlane(const vector<int>& imgX, const vector<int>& imgY, vector<int>& objX,
                                                vector<int>& objY)
{
    int size = (int)imgX.size();
    for (int i = 0; i < size; ++i)
    {
        double srcImgPoint[3] = {(double)imgX[i], (double)imgY[i], 0.0};
        double dstObjPoint[3] = {0.0, 0.0, 0.0};
        m_st->imageToObject(srcImgPoint, dstObjPoint);

        int xPixel = (int)floor(m_st->mmToPixel(dstObjPoint[0]) + 0.5);
        int yPixel = (int)floor(m_st->mmToPixel(dstObjPoint[1]) + 0.5);

        int dstWidth = (int)floor(m_st->mmToPixel(5.5) + 0.5);
        int dstHeight = (int)floor(m_st->mmToPixel(5.5) + 0.5);

        int halfWidth = dstWidth / 2;
        int halfHeight = dstHeight / 2;

        xPixel += halfWidth;
        yPixel += halfHeight;

        objX.push_back(xPixel);
        objY.push_back(yPixel);
    }
}

void HttPrivate::convertPointsFromObjToImgPlane(const vector<float>& objX, const vector<float>& objY, vector<int>& imgX,
                                                vector<int>& imgY)
{
    int size = (int)objX.size();
    for (int i = 0; i < size; ++i)
    {
        double srcObjPoint[] = {objX[i], objY[i], 0.0};
        double dstImgPoint[] = {0.0, 0.0, 0.0};
        m_st->objectToImage(srcObjPoint, dstImgPoint);

        imgX.push_back((float)dstImgPoint[0]);
        imgY.push_back((float)dstImgPoint[1]);
    }
}

void HttPrivate::drawProfileObjPlane(shared_ptr<Image<uint8_t>> image, vector<int> x, vector<int> y, const int color[],
                                     int pixelSize /* = 1 */)
{
    int nChannels = image->getNChannels();
    int stride = image->getStride();
    int width = image->getWidth();
    int height = image->getHeight();
    int halfWidth = width / 2;
    int halfHeight = height / 2;
    uint8_t* pImg = image->getData();

    /* Json::Value st = settings->getCalibration()["scheimpflug_transformation"];
     double cx0 = st["cx0"].asDouble();
     double cy0 = st["cy0"].asDouble();

     double srcImgPoint[3] = {194.0, 212.0, 0.0};
     double dstObjPoint[3] = {0.0, 0.0, 0.0};
     m_st->imageToObject(srcImgPoint, dstObjPoint);
     cout << dstObjPoint[0] << " " << dstObjPoint[1] << endl;

     int xPixel = (int)floor(m_st->mmToPixel(dstObjPoint[0]) + 0.5);
     int yPixel = (int)floor(m_st->mmToPixel(dstObjPoint[1]) + 0.5);
     xPixel += halfWidth;
     yPixel += halfHeight;
     cout << xPixel << " " << yPixel << endl;

     double xMm = m_st->pixelToMm((double)xPixel - halfWidth);
     double yMm = m_st->pixelToMm((double)yPixel - halfHeight);
     cout << xMm << " " << yMm << endl;

     // double srcObjPoint[3] = {dstObjPoint[0], dstObjPoint[1], 0.0};
     double srcObjPoint[3] = {xMm, yMm, 0.0};
     double dstImgPoint[3] = {0.0, 0.0, 0.0};
     m_st->objectToImage(srcObjPoint, dstImgPoint, false);
     cout << dstImgPoint[0] << " " << dstImgPoint[1] << endl;

     xPixel = (int)floor(dstImgPoint[0] + 0.5);
     yPixel = (int)floor(dstImgPoint[1] + 0.5);
     cout << xPixel << " " << yPixel << endl;

     cout << "drawProfileObjPlane " << halfHeight << " " << halfWidth << endl;*/

    int size = (int)x.size();

    for (int i = 0; i < size; ++i)
    {
        double srcImgPoint[3] = {(double)x[i], (double)y[i], 0.0};
        double dstObjPoint[3] = {0.0, 0.0, 0.0};
        m_st->imageToObject(srcImgPoint, dstObjPoint);

        // int xPixel = (int)m_st->mmToPixel(dstObjPoint[0]);
        // int yPixel = (int)m_st->mmToPixel(dstObjPoint[1]);
        int xPixel = (int)floor(m_st->mmToPixel(dstObjPoint[0]) + 0.5);
        int yPixel = (int)floor(m_st->mmToPixel(dstObjPoint[1]) + 0.5);

        xPixel += halfWidth;
        yPixel += halfHeight;

        if (pixelSize > 1)
        {
            if (pixelSize % 2 == 0)
                pixelSize += 1;
            for (int n = -(pixelSize / 2); n < (pixelSize / 2) + 1; ++n)
            {
                for (int m = -(pixelSize / 2); m < (pixelSize / 2) + 1; ++m)
                {
                    if (xPixel + n >= 0 && xPixel + n < width && yPixel + m >= 0 && yPixel + m < height)
                    {
                        int index = xPixel * nChannels;
                        index += yPixel * stride;
                        index += nChannels * n;
                        index += stride * m;
                        pImg[index] = color[0];
                        pImg[index + 1] = color[1];
                        pImg[index + 2] = color[2];
                    }
                }
            }
        } else
        {
            if (xPixel >= 0 && xPixel < width && yPixel >= 0 && yPixel < height)
            {
                int index = xPixel * nChannels;
                index += yPixel * stride;
                pImg[index] = color[0];
                pImg[index + 1] = color[1];
                pImg[index + 2] = color[2];
            }
        }
    }
}

void HttPrivate::drawProfileImgPlane(shared_ptr<Image<uint8_t>> image, vector<int> vx, vector<int> vy,
                                     const int color[], int pixelSize /* = 1 */)
{
    int nChannels = image->getNChannels();
    int stride = image->getStride();
    int width = image->getWidth();
    int height = image->getHeight();
    uint8_t* pImg = image->getData();

    int size = (int)vx.size();
    for (int i = 0; i < size; ++i)
    {
        int x = vx[i];
        int y = vy[i];

        if (pixelSize > 1)
        {
            if (pixelSize % 2 == 0)
                pixelSize += 1;
            for (int n = -(pixelSize / 2); n < (pixelSize / 2) + 1; ++n)
            {
                for (int m = -(pixelSize / 2); m < (pixelSize / 2) + 1; ++m)
                {
                    if (x + n >= 0 && x + n < width && y + m >= 0 && y + m < height)
                    {
                        int index = x * nChannels;
                        index += y * stride;
                        index += nChannels * n;
                        index += stride * m;
                        pImg[index] = color[0];
                        pImg[index + 1] = color[1];
                        pImg[index + 2] = color[2];
                    }
                }
            }
        } else
        {
            if (x >= 0 && x < width && y >= 0 && y < height)
            {
                int index = x * nChannels;
                index += y * stride;
                pImg[index] = color[0];
                pImg[index + 1] = color[1];
                pImg[index + 2] = color[2];
            }
        }
    }
}

void HttPrivate::init(double R, double H, bool calibration /*= false*/)
{
    if (m_debug)
    {
        stringstream str;
        str << "ScheimpflugTransform init deltaN:" << R << " H:" << H;
        log->debug(tag + str.str());
    }

    // Get settings
    Json::Value jsonCalibration = settings->getCalibration();

    // Configure the transformation parameters and initialize the variables
    if (calibration)
    {
        log->info(tag + "Init calibration mode");
        m_st->init(jsonCalibration["nominal_scheimpflug_transformation"]);
    } else
    {
        log->info(tag + "Init lens fitting or validation mode");
        m_st->init(jsonCalibration["scheimpflug_transformation"]);
    }

    // Initialize the variables that depend on the orientation of the stylus
    m_st->setStylusOrientation(R, H);
}

void HttPrivate::init(shared_ptr<ScheimpflugTransform> st, double R, double H, bool calibration /*= false*/)
{
    if (m_debug)
    {
        stringstream str;
        str << "ScheimpflugTransform init deltaN:" << R << " H:" << H;
        log->debug(tag + str.str());
    }

    // Get settings
    Json::Value jsonCalibration = settings->getCalibration();

    // Configure the transformation parameters and initialize the variables
    if (calibration)
    {
        log->info(tag + "Init calibration mode");
        st->init(jsonCalibration["nominal_scheimpflug_transformation"]);
    } else
    {
        log->info(tag + "Init lens fitting or validation mode");
        st->init(jsonCalibration["scheimpflug_transformation"]);
    }
    // Initialize the variables that depend on the orientation of the stylus
    st->setStylusOrientation(R, H);
}

int HttPrivate::printReturnCode()
{
    string ret;
    switch (m_rcode)
    {
    case ReturnCode::PREPROCESS_SUCCESSFUL:
        ret = "Pre-process successful";
        break;
    case ReturnCode::PREPROCESS_FAILED_IMAGE_NOT_AVAILABLE:
        ret = "Pre-process failed: image not available";
        break;
    case ReturnCode::PREPROCESS_FAILED_WRONG_IMAGE_SIZE:
        ret = "Pre-process failed: wrong image size";
        break;
    case ReturnCode::PREPROCESS_FAILED_WRONG_IMAGE_CHANNEL:
        ret = "Pre-process failed: wrong image channels number";
        break;
    case ReturnCode::PREPROCESS_FAILED_PROFILE_DETECTION:
        ret = "Pre-process failed: profile detection failed";
        break;
    case ReturnCode::CALIBRATION_SUCCESSFUL:
        ret = "Calibration successful";
        break;
    case ReturnCode::CALIBRATION_FAILED_CONVERGENCE:
        ret = "Calibration failed: convergence not reached";
        break;
    case ReturnCode::CALIBRATION_FAILED_GRID:
        ret = "Calibration failed: grid not detected";
        break;
    case ReturnCode::CALIBRATION_FAILED_LINE_DETECTION:
        ret = "Calibration failed: line detection failed";
        break;
    case ReturnCode::CALIBRATION_FAILED_SEARCH_SPACE:
        ret = "Calibration failed: search space boundary reached";
        break;
    case ReturnCode::CALIBRATION_FAILED_PSO_INIT:
        ret = "Calibration failed: PSO initialization failed";
        break;
    case ReturnCode::LENSFITTING_SUCCESSFUL:
        ret = "Lens fitting successful";
        break;
    case ReturnCode::LENSFITTING_FAILED_MODEL_NOT_INITIALIZED:
        ret = "Lens fitting failed: model not initialized";
        break;
    case ReturnCode::LENSFITTING_FAILED_MAX_RETRIES_REACHED:
        ret = "Lens fitting failed: max retries reached";
        break;
    case ReturnCode::LENSFITTING_FAILED_BEVELFRAME_INTERSECTION:
        ret = "Lens fitting failed: bevel-frame intersection";
        break;
    case ReturnCode::LENSFITTING_FAILED_PSO_BOUNDARY_REACHED:
        ret = "Lens fitting failed: PSO baundary reached";
        break;
    default:
        ret = "Undefined return code";
        break;
    }
    int rcode = m_rcode;
    m_rcode = ReturnCode::UNINITIALIZED;
    stringstream s;
    s << tag << "(" << rcode << ") " << ret;
    log->info(s.str());
    return rcode;
}

double HttPrivate::measureOnObjectPlane(double x1, double y1, double x2, double y2)
{
    double X1_X2 = m_st->pixelToMm(x1) - m_st->pixelToMm(x2);
    double Y1_Y2 = m_st->pixelToMm(y1) - m_st->pixelToMm(y2);
    double ret = sqrt(X1_X2 * X1_X2 + Y1_Y2 * Y1_Y2);
    return ret;
}

double HttPrivate::convertPixelToMm(double m)
{
    return m_st->pixelToMm(m);
}

double HttPrivate::convertMmToPixel(double m)
{
    return m_st->mmToPixel(m);
}

shared_ptr<Profile<int>> HttPrivate::preProcess(shared_ptr<Image<uint8_t>> inputImage, string imageLabel,
                                                HttProcessType processType, ModelType modelType)
{
    if (m_debug)
    {
        stringstream s;
        s << tag << "Preprocessing (type:" << processType << ")";
        log->debug(s.str());
    }

    Json::Value settingsJson = settings->getSettings();

    int width = settingsJson["image"]["width"].asUInt();
    int height = settingsJson["image"]["height"].asUInt();
    int nChannels = settingsJson["image"]["nChannels"].asUInt();

    if (!inputImage)
    {
        m_rcode = ReturnCode::PREPROCESS_FAILED_IMAGE_NOT_AVAILABLE;
        return nullptr;
    }
    if (inputImage->getNChannels() != nChannels)
    {
        m_rcode = ReturnCode::PREPROCESS_FAILED_WRONG_IMAGE_CHANNEL;
        return nullptr;
    }
    if (inputImage->getWidth() != width || inputImage->getHeight() != height)
    {
        m_rcode = ReturnCode::PREPROCESS_FAILED_WRONG_IMAGE_SIZE;
        return nullptr;
    }

    shared_ptr<Image<uint8_t>> singleChannelImage = nullptr;
    if (m_debug)
        log->debug(tag + "Extract singleChannel from input image");
    uint8_t channel = settingsJson["channel_section"]["selectedChannel"].asUInt();
    singleChannelImage = imgPreProc->getChannel(inputImage, channel);

    shared_ptr<Image<uint8_t>> floatSingleChannelInputImage = nullptr;
    shared_ptr<Image<uint8_t>> medianFilterImage = nullptr;
    if (settingsJson["test"]["medianBlurFilter"].asBool())
    {
        // Median Blur filter
        if (m_debug)
            log->debug(tag + "Median filter on 1-channel data");
        medianFilterImage = imgPreProc->medianBlurFilter(singleChannelImage, 4);
        floatSingleChannelInputImage = medianFilterImage;
    } else
        floatSingleChannelInputImage = singleChannelImage;

    // Convert it to float and normalize
    if (m_debug)
        log->debug(tag + "Convert it to float and normalize");
    shared_ptr<Image<float>> floatSingleChannelImage = imageToFloat(floatSingleChannelInputImage);

    shared_ptr<Image<float>> floatLPF1Image = nullptr;
    if (settingsJson["test"]["lowPassFilter"].asBool())
    {
        if (m_debug)
            log->debug(tag + "Low-pass filter on 1-channel data");
        uint8_t radius = 5;
        floatLPF1Image = imgPreProc->lowPassFilter1(floatSingleChannelImage, radius);
    } else
    {
        floatLPF1Image = floatSingleChannelImage;
    }

    // Apply the Sobel filter to the selected channel and normalize
    if (m_debug)
        log->debug(tag + "Apply the Sobel filter to the selected channel and normalize");
    // shared_ptr<Image<float>> sobelFilterImage = floatLPF1Image;//imgPreProc->sobelFilter(floatLPF1Image);
    shared_ptr<Image<float>> sobelFilterImage = imgPreProc->sobelFilter(floatLPF1Image);

    // ADG
    // saveImage("./", imageLabel + "_lowPass.png", floatLPF1Image);
    // saveImage("./", imageLabel + "_Sobel.png", sobelFilterImage);

    // Thresholding
    if (m_debug)
        log->debug(tag + "Thresholding");

    ImagePreprocessing::ThresholdArgs<float> thresholdArgs;
    thresholdArgs.lowPassImage = floatLPF1Image;
    thresholdArgs.sobelImage = sobelFilterImage;
    thresholdArgs.threshold = settingsJson["thresholding"]["threshold"].asFloat();
    thresholdArgs.searchWidth = settingsJson["thresholding"]["searchWidth"].asUInt();
    thresholdArgs.searchOffset = settingsJson["thresholding"]["searchOffset"].asUInt();
    thresholdArgs.leftBoundary = settingsJson["thresholding"]["leftBoundary"].asUInt();
    thresholdArgs.rightBoundary = settingsJson["thresholding"]["rightBoundary"].asUInt();
    thresholdArgs.upperBoundary = settingsJson["thresholding"]["upperBoundary"].asUInt();
    thresholdArgs.lowerBoundary = settingsJson["thresholding"]["lowerBoundary"].asUInt();
    thresholdArgs.maxIterations = settingsJson["thresholding"]["maxIterations"].asUInt();
    thresholdArgs.profileOffset = settingsJson["thresholding"]["profileOffset"].asInt();

    thresholdArgs.enableStylusMask = settingsJson["thresholding"]["enableStylusMask"].asBool();
    thresholdArgs.stylusMaskX1 = settingsJson["thresholding"]["stylusMaskX1"].asUInt();
    thresholdArgs.stylusMaskY1 = settingsJson["thresholding"]["stylusMaskY1"].asUInt();
    thresholdArgs.stylusMaskX2 = settingsJson["thresholding"]["stylusMaskX2"].asUInt();
    thresholdArgs.stylusMaskY2 = settingsJson["thresholding"]["stylusMaskY2"].asUInt();

    thresholdArgs.referenceOffsetFromStylus = settingsJson["thresholding"]["referenceOffsetFromStylus"].asUInt();
    thresholdArgs.referenceHalfHeight = settingsJson["thresholding"]["referenceHalfHeight"].asUInt();
    thresholdArgs.referenceWidth = settingsJson["thresholding"]["referenceWidth"].asUInt();

    shared_ptr<Profile<int>> profile = imgPreProc->threshold(thresholdArgs);

    // FIXME - NBr
    {
        string folder = settings->getSettings()["test"]["outputFolder"].asCString();
        ofstream outCsvFile(folder + "/profile_" + imageLabel + ".csv");
        // outCsvFile << "index,x [pixel],y [pixel]\n";
        for (int i = 0; i < profile->x.size(); ++i)
        {
            outCsvFile << profile->x[i] << ", " << profile->y[i] << ", 0\n";
        }
        outCsvFile.close();
    }

    if (!profile)
    {
        m_rcode = ReturnCode::PREPROCESS_FAILED_PROFILE_DETECTION;
        return nullptr;
    }

    // Remove isolated points (applied twice for more filtering)
    if (m_debug)
        log->debug(tag + "Remove isolated points");
    float distanceThreshold = settingsJson["profile_cleanup"]["distanceThreshold"].asFloat();
    int windowSize = settingsJson["profile_cleanup"]["windowSize"].asInt();
    int counterThreshold = settingsJson["profile_cleanup"]["counterThreshold"].asInt();
    shared_ptr<Profile<int>> profileClean1 =
        imgPreProc->removeIsolatedPoints(profile, distanceThreshold, windowSize, counterThreshold);
    shared_ptr<Profile<int>> profileClean2 =
        imgPreProc->removeIsolatedPoints(profileClean1, distanceThreshold, windowSize, counterThreshold);

    int filteringHalfWindowSize = settingsJson["profile_cleanup"]["filteringHalfWindowSize"].asInt();

    if (processType == HttProcessType::lensFitting &&
        (modelType != ModelType::MiniBevelModelType || modelType != ModelType::MiniBevelModelExtType))
        profileClean2 = imgPreProc->filterProfile(profileClean2, filteringHalfWindowSize, height);
    else // lensFitting & MiniBevel or other process types
        profileClean2 = imgPreProc->filterProfileWithInterpolation(profileClean2, filteringHalfWindowSize);

    // FIXME - NBr
    {
        string folder = settings->getSettings()["test"]["outputFolder"].asCString();
        ofstream outCsvFile(folder + "/profileCleand_" + imageLabel + ".csv");
        // outCsvFile << "index,x [pixel],y [pixel]\n";
        for (int i = 0; i < profileClean2->x.size(); ++i)
        {
            outCsvFile << profileClean2->x[i] << ", " << profileClean2->y[i] << ", 0\n";
        }
        outCsvFile.close();
    }

    // Create images with profiles for debug purposes
    if (m_debug)
    {
        string folder = settingsJson["test"]["outputFolder"].asString();
        shared_ptr<Image<float>> profCleanedImg(new Image<float>(*floatLPF1Image));
        float* pProfCleanedImg = profCleanedImg->getData();
        uint16_t stride = profCleanedImg->getStride() / sizeof(float);
        uint8_t nChs = profCleanedImg->getNChannels();
        for (unsigned int i = 0; i < profile->x.size(); i++)
        {
            pProfCleanedImg[profile->x[i] * nChs + profile->y[i] * stride] = 1;
        }
        if (!imageLabel.empty())
            saveImage(folder, imageLabel + "_profile.png", profCleanedImg);

        shared_ptr<Image<float>> profCleaned2Img(new Image<float>(*floatLPF1Image));
        float* pProfCleaned2Img = profCleaned2Img->getData();
        // uint16_t stride2 = profCleaned2Img->getStride();
        // uint8_t nChannels2 = profCleaned2Img->getNChannels();
        for (unsigned int i = 0; i < profileClean2->x.size(); i++)
        {
            pProfCleaned2Img[profileClean2->x[i] * nChs + profileClean2->y[i] * stride] = 1;
        }
        if (!imageLabel.empty())
            saveImage(folder, imageLabel + "_profileCleaned.png", profCleaned2Img);

        if (!imageLabel.empty() && medianFilterImage)
            saveImage(folder, imageLabel + "_medianFilter.png", medianFilterImage);
    }

    m_rcode = ReturnCode::PREPROCESS_SUCCESSFUL;
    return profileClean2;
}

shared_ptr<Profile<int>> HttPrivate::preProcessTest(shared_ptr<Image<uint8_t>> inputImage, string imageLabel,
                                                    HttProcessType processType, ModelType modelType)
{
    Json::Value settingsJson = settings->getSettings();

    shared_ptr<Image<uint8_t>> singleChannelImage = nullptr;
    if (m_debug)
        log->debug(tag + "Extract singleChannel from input image");
    uint8_t channel = settingsJson["channel_section"]["selectedChannel"].asUInt();
    singleChannelImage = imgPreProc->getChannel(inputImage, channel);

    // Convert it to float and normalize
    if (m_debug)
        log->debug(tag + "Convert it to float and normalize");
    shared_ptr<Image<float>> floatSingleChannelImage = imageToFloat(singleChannelImage);

    shared_ptr<Profile<int>> profile = imgPreProc->thresholdTest(floatSingleChannelImage);

    if (!profile)
    {
        m_rcode = ReturnCode::PREPROCESS_FAILED_PROFILE_DETECTION;
        return nullptr;
    }

    // Create images with profiles for debug purposes
    if (m_debug)
    {
        string folder = settingsJson["test"]["outputFolder"].asString();
        shared_ptr<Image<float>> profCleanedImg(new Image<float>(*floatSingleChannelImage));
        float* pProfCleanedImg = profCleanedImg->getData();
        uint16_t stride = profCleanedImg->getStride() / sizeof(float);
        uint8_t nChs = profCleanedImg->getNChannels();
        for (unsigned int i = 0; i < profile->x.size(); i++)
        {
            pProfCleanedImg[profile->x[i] * nChs + profile->y[i] * stride] = 1;
        }
        if (!imageLabel.empty())
            saveImage(folder, imageLabel + "_profile.png", profCleanedImg);
    }

    m_rcode = ReturnCode::PREPROCESS_SUCCESSFUL;
    return profile;
}

shared_ptr<HttPrivate::ProcessFolderResults> HttPrivate::processFolder(const string& folder,
                                                                       shared_ptr<Statistics> stats,
                                                                       bool validation /* = false */,
                                                                       bool deltaNVariable /* = false */)
{
    shared_ptr<ProcessFolderResults> results(new ProcessFolderResults());

#ifdef UNIX
    if (auto dir = opendir(folder.c_str()))
    {
        Json::Value settingsJson = settings->getSettings();
        while (auto f = readdir(dir))
        {
            if (!f->d_name || f->d_name[0] == '.') // Exclude "." and ".."
                continue;
            if (f->d_type == DT_DIR) // Recursion into sub-folder
                // processFolder(folder + f->d_name + "/", stats);
                continue;

            if (f->d_type == DT_REG)
            {

                string d_name = f->d_name;
                string fName = d_name.substr(0, d_name.find_last_of('.'));

                // Extract R and H from filename
                // i.e. Frame_15_LED=0600_R=-15_H=+15_05.bmp
                // R=-15 and H=15 (double values)
                double R = 0, H = 0;
                parseFilename(f->d_name, R, H);

                string inputImage = folder + f->d_name;

                // Load png image from inputImage path
                log->info(tag + "Load image " + inputImage);
                shared_ptr<Image<uint8_t>> img = loadImage((char*)inputImage.c_str());

                HttPrivate::processArgs args;
                args.R = R;
                args.H = H;
                args.inputImage = img;
                args.stats = stats;
                args.modelType = getModelType(settingsJson["model"].asString());
                if (validation)
                    args.processType = HttProcessType::validation;

                if (settingsJson["debug"].asBool())
                    log->debug(tag + "Test repetitions enabled");

                // Overwrite input args
                args.repetitions = settingsJson["test"]["repetitions"].asInt();
                args.imageLabel = fName;

// FIXME
#if 0
                // if (stats)
                //     stats->start(fName.c_str());
                // shared_ptr<HttPrivate::ProcessResults> res = process(args);
                // results->nImages++;
                // results->counters[m_rcode]++;
                // if (stats)
                //     stats->stop();
#else
                shared_ptr<HttPrivate::ProcessResults> res;
                if (deltaNVariable)
                {
                    res = processWithDeltaNVariable(args);
                } else
                {
                    if (stats)
                        stats->start(fName.c_str());
                    res = process(args);
                    results->nImages++;
                    results->counters[m_rcode]++;
                    if (stats)
                        stats->stop();
                }
#endif

                printReturnCode();
                cout << endl;
            }
        }
        closedir(dir);
    }
#endif

    return results;
}

map<string, bool> HttPrivate::checkCalibratedParamsWithNominalRanges(bool checkBoundariesReached /*= false*/)
{
    map<string, bool> result;

    Json::Value root = settings->getCalibration();
    Json::Value p = root["scheimpflug_transformation"];
    Json::Value range = root["nominal_range_scheimpflug_transformation"];

    const int size = 10;
    const string params[size] = {"alpha", "beta", "cx0", "cy0", "delta", "p1", "p2", "phi", "theta", "tt"};
    for (int i = 0; i < size; ++i)
    {
        float val = p[params[i]].asFloat();
        float min = range[params[i]]["min"].asFloat();
        float max = range[params[i]]["max"].asFloat();
        bool check;
        if (checkBoundariesReached)
            check = (val == min || val == max) ? true : false;
        else
            check = (val < min || val > max) ? false : true;
        // cout << params[i] << " val:" << val << " min:" << min << " max:" << max << " check:" << to_string(check)
        //      << endl;
        result.insert(pair<string, bool>(params[i], check));
    }

    return result;
}

void HttPrivate::imageToObject(const double* srcImgPoint, double* dstObjPoint)
{
    m_st->imageToObject(srcImgPoint, dstObjPoint);
}

void HttPrivate::initMultiFramesCalibration(const string& folder, vector<shared_ptr<Image<uint8_t>>>& inputImageSet,
                                            vector<double>& deltaNSet, vector<double>& HSet,
                                            vector<string>& filenameSet)
{
#ifdef UNIX
    if (auto dir = opendir(folder.c_str()))
    {
        Json::Value settingsJson = settings->getSettings();
        while (auto f = readdir(dir))
        {
            if (!f->d_name || f->d_name[0] == '.') // Exclude "." and ".."
                continue;
            if (f->d_type == DT_DIR) // Recursion into sub-folder disable
                // processFolder(folder + f->d_name + "/", stats);
                continue;

            if (f->d_type == DT_REG)
            {

                string d_name = f->d_name;
                string fName = d_name.substr(0, d_name.find_last_of('.'));

                // Extract R and H from filename
                // i.e. Frame_15_LED=0600_R=-15_H=+15_05.bmp
                // R=-15 and H=15 (double values)
                double R = 0, H = 0;
                parseFilename(f->d_name, R, H);

                string inputImage = folder + f->d_name;

                // Load png image from inputImage path
                log->info(tag + "Load image " + inputImage);
                shared_ptr<Image<uint8_t>> img = loadImage((char*)inputImage.c_str());

                inputImageSet.push_back(img);
                deltaNSet.push_back(R);
                HSet.push_back(H);
                filenameSet.push_back(fName);
            }
        }
    }
#endif
}

shared_ptr<HttPrivate::ProcessResults> HttPrivate::processWithDeltaNVariable(processArgs& args)
{
    M_Assert(args.repetitions > 0, "Process repetitions must be greather than zero");

    Json::Value jsonSettings = settings->getSettings();
    Json::Value jsonCalibration = settings->getCalibration();

    shared_ptr<HttPrivate::ProcessResults> res(new HttPrivate::ProcessResults);
    res->valid = false;

    if (args.R == 0 || args.H == 0)
    {
        char str[100];
        sprintf_s(str, "Check Stylus Orientation    deltaN:%.1f H:%.1f", args.R, args.H);
        log->info(tag + str);
    }

    // Check R/H boundaries
    double R_min = jsonCalibration["htt"]["deltaN"]["min"].asDouble();
    double R_max = jsonCalibration["htt"]["deltaN"]["max"].asDouble();
    double H_min = jsonCalibration["htt"]["H"]["min"].asDouble();
    double H_max = jsonCalibration["htt"]["H"]["max"].asDouble();
    if (args.R < R_min || args.R > R_max)
    {
        stringstream s;
        s << tag << "deltaN:" << args.R << " value out of boundary [" << R_min << "," << R_max << "]";
        log->warn(s.str());
    }
    if (args.H < H_min || args.H > H_max)
    {
        stringstream s;
        s << tag << "H:" << args.H << " value out of boundary [" << H_min << "," << H_max << "]";
        log->warn(s.str());
    }

    // Instance preprocessing and optimization time
    float preprocessingTime = 0;
    // float currOptimizationTime = 0;
    float initOptimizationTime = 0;
    float optimizationTime = 0;

    // Preprocessing
    shared_ptr<Profile<int>> profileCleaned = nullptr;
    timer.start();
    if (jsonSettings["test"]["processSyntheticImages"].asBool())
        profileCleaned = preProcessTest(args.inputImage, args.imageLabel, args.processType, args.modelType);
    else
        profileCleaned = preProcess(args.inputImage, args.imageLabel, args.processType, args.modelType);
    preprocessingTime = timer.stop();

    if (!profileCleaned)
        return res;

    if (args.processType == HttProcessType::lensFitting && !jsonSettings["optimization"]["enabled"].asBool())
    {
        log->warn(tag + "Optimization disabled");
        return res;
    }

    Pso<float>::Input psoInput;

    float deltaNRange[] = {-10.0, -5.0, -2.0, -1.0, 0.0, 1.0, 2.0, 5.0, 10.0};
    for (int i = 0; i < 9; ++i)
    {
        float deltaN = args.R + deltaNRange[i];
        stringstream val;
        // val << setprecision(1) << internal << setfill('0') << setw(5) << showpos << fixed << deltaN;
        // string filename = args.imageLabel + "_newDeltaN=" + val.str();
        // val << setprecision(1) << internal << setfill('0') << setw(5) << showpos << fixed << deltaNRange[i];
        val << showpos << deltaNRange[i];
        string filename = args.imageLabel + "_deltaNOffset=" + val.str();

        // Itialize the Scheimpflug Transform
        init(deltaN, args.H, args.processType == HttProcessType::calibration);

        timer.start();
        shared_ptr<Model> model =
            optimizationEngine->initModel(m_st, profileCleaned, args.modelType, args.processType, psoInput);
        initOptimizationTime = timer.stop();

        // Handle the case when the model was not initialized (e.g. upperMax or lowerMax not found).
        if (model == nullptr)
        {
            log->warn(tag + "Model was not initialized");
            m_rcode = ReturnCode::LENSFITTING_FAILED_MODEL_NOT_INITIALIZED;
            return res;
        }

        if (args.stats)
            args.stats->start(filename.c_str());

        shared_ptr<Image<uint8_t>> bevelImg = nullptr;
        shared_ptr<Image<uint8_t>> objFrameImg = nullptr;
        shared_ptr<Image<uint8_t>> objBevelImg = nullptr;
        shared_ptr<OptimizationEngine::Output> output = nullptr;
        int maxRetries = jsonSettings["optimization"]["maxRetries"].asInt();

        for (int i = 0; i < args.repetitions; ++i)
        {
            if (m_debug)
            {
                stringstream str;
                str << tag << "optimizeModel [" << i + 1 << "/" << args.repetitions << "]";
                log->debug(str.str());
            }
            timer.start();
            output = optimizationEngine->optimizeModel(model, psoInput, profileCleaned, maxRetries);
            float currOptimizationTime = timer.stop();
            optimizationTime += currOptimizationTime;

            if (output->measures != nullptr)
            {
                // Save process results in the statistics vectors
                ModelType modelType = model->modelType();

                if ((args.processType == HttProcessType::lensFitting ||
                     args.processType == HttProcessType::validation) &&
                    args.stats)
                    if (modelType == MiniBevelModelType)
                        args.stats->push_back(output->measures->m["B*"], output->measures->m["B"],
                                              output->measures->m["M"], 0.0, output->loss, output->iteration,
                                              preprocessingTime, currOptimizationTime);
                    else if (modelType == MiniBevelModelExtType)
                    {
                        args.stats->push_back(output->measures->m["B*"], output->measures->m["B"],
                                              output->measures->m["M"], output->measures->m["beta"], output->loss,
                                              output->iteration, preprocessingTime, currOptimizationTime);
                    }
            }

            bevelImg = generateBevelImage(args.inputImage, output->bevel, profileCleaned);
            objBevelImg = generateBevelImageOnObjectPlane(args.inputImage, output->bevel, profileCleaned);

            // Save image on disk
            if (jsonSettings["test"]["storeImages"].asBool())
            {
                {
                    char file[256];
                    sprintf_s(file, "%s_objplan_%03d.png", filename.c_str(), i);
                    saveImage(jsonSettings["test"]["outputFolder"].asString(), file, objBevelImg);
                }

                {
                    char file[256];
                    sprintf_s(file, "%s_%03d.png", filename.c_str(), i);
                    saveImage(jsonSettings["test"]["outputFolder"].asString(), file, bevelImg);
                }
            }
        }

        // Save optimizeModel return code
        m_rcode = output->rcode;

        log->info(tag + "Performance:");
        optimizationTime /= args.repetitions;
        optimizationTime += initOptimizationTime;
        float totalProcessingTime = preprocessingTime + optimizationTime;
        char str[100];
        sprintf_s(str, "\tpreprocessing time:    %.3f s", preprocessingTime);
        log->info(tag + str);

        sprintf_s(str, "\toptimization time:     %.3f s", optimizationTime);
        log->info(tag + str);

        sprintf_s(str, "\tTotal processing time: %.3f s", totalProcessingTime);
        log->info(tag + str);

        if (m_rcode == ReturnCode::LENSFITTING_SUCCESSFUL ||
            m_rcode == ReturnCode::LENSFITTING_FAILED_PSO_BOUNDARY_REACHED)
            res->valid = true;
        else
            res->valid = false;
        res->bevelImage = bevelImg;
        res->optimizationOutput = output;
        res->preprocessingTime = preprocessingTime;
        res->optimizationTime = optimizationTime;
        res->profile = profileCleaned;
        res->objBevelImage = objBevelImg;

        if (args.stats)
            args.stats->stop();
    }

    return res;
}

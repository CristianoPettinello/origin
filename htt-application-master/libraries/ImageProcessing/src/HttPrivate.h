//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __HTT_PRIVATE_H__
#define __HTT_PRIVATE_H__

#include "HttGlobal.h"
#include "HttProcessType.h"
#include "ImagePreprocessing.h"
#include "Log.h"
#include "OptimizationEngine.h"
#include "ReturnCode.h"
#include "Settings.h"
#include "Statistics.h"
#include "Timer.h"
#include "json/json.h"
#include <functional>

using namespace std;
using namespace Nidek::Libraries::Utils;

namespace Nidek {
namespace Libraries {
namespace HTT {
///
/// \brief HttPrivate class.
///
/// This class is the main entry point of the library
///
class HttPrivate
{
private:
    shared_ptr<Log> log;
    shared_ptr<Settings> settings;
    shared_ptr<ImagePreprocessing> imgPreProc;
    shared_ptr<OptimizationEngine> optimizationEngine;
    shared_ptr<ScheimpflugTransform> m_st;
    Timer timer; /// Timer for measuring the processing performance
    bool m_debug;
    ReturnCode m_rcode;

public:
    class ProcessFolderResults
    {
    public:
        int nImages;
        map<ReturnCode, int> counters;

    public:
        ProcessFolderResults();
    };

    struct ProcessResults
    {
        bool valid;
        shared_ptr<Image<uint8_t>> bevelImage;
        shared_ptr<OptimizationEngine::Output> optimizationOutput;
        shared_ptr<Profile<int>> profile;
        shared_ptr<Image<uint8_t>> objFrameImage;
        shared_ptr<Image<uint8_t>> objBevelImage;
        float preprocessingTime;
        float optimizationTime;
    };
    struct OptimizeScheimpflugTransformResults
    {
        bool valid = false;
        shared_ptr<Image<uint8_t>> bevelImage;
        shared_ptr<OptimizationEngine::ScheimpflugTransformOutput> optimizationOutput;
    };
    struct CalibrationGridResults
    {
        bool valid = false;
        float lossValue;
        shared_ptr<ScheimpflugTransform::Params> scheimpflugTransformParams;
        vector<pair<pair<int, int>, pair<int, int>>> imgPlaneGridLines;
        vector<pair<int, int>> imgPlaneIntersectionPoints;
        shared_ptr<Image<uint8_t>> objPlaneGrid;
        vector<pair<int, int>> objPlaneIntersectionPoints;
        vector<float> gridErrors;
        ReturnCode returnCode;
    };
    struct processArgs
    {
        double H = 0;
        double R = 0;
        int repetitions = 1;
        HttProcessType processType = lensFitting;
        string imageLabel;
        shared_ptr<Image<uint8_t>> inputImage;
        shared_ptr<Statistics> stats;
        ModelType modelType;
    };

    ///
    /// Constructor.
    ///
    HttPrivate();

    ///
    /// Processes a single image.
    ///
    /// \param [in] inputImage  Shared pointer to input image
    /// \param [in] R           (optional) Stylus R angle
    /// \param [in] H           (optional) Stylus H angle
    ///
    /// \return A shared pointer of the output image to the sobel filter
    ///
    shared_ptr<HttPrivate::ProcessResults> process(processArgs& args, bool disableOptimization = false);

    ///
    /// Process an entire folder and its subfolders
    ///
    /// \param [in] folder      Starting folder path
    /// \param [in] stats       Shared pointer to a statistics object
    ///
    shared_ptr<HttPrivate::ProcessFolderResults> processFolder(const string& folder, shared_ptr<Statistics> stats,
                                                               bool validation = false, bool deltaNVariable = false);

    ///
    /// Optimize the Scheimpflug Transform parameters using a single image.
    ///
    /// \param [in] inputImage  Shared pointer to input image
    /// \param [in] bevelParams Array with the bevel parameters
    /// \param [in] R           (optional) Stylus R angle
    /// \param [in] H           (optional) Stylus H angle
    ///
    /// \return A structure with the optimization results
    ///
    OptimizeScheimpflugTransformResults optimizeScheimpflugTransform(shared_ptr<Image<uint8_t>> inputImage,
                                                                     const vector<float>& imgX,
                                                                     const vector<float>& imgY, double R = 0,
                                                                     double H = 0);

    ///
    /// Optimize the Scheimpflug Transform parameters using many images.
    ///
    /// \param [in] imageSet  Shared pointer to input image
    /// \param [in] imageXSet Set of array with the bevel points (X coordinates)
    /// \param [in] imageYSet Set of array with the bevel points (Y coordinates)
    /// \param [in] deltaNSet DeltaN angle for each image
    /// \param [in] HSet      H angle for each image
    /// \param [in] profileSet Set of profile, one for each image.
    ///
    /// \return A structure with the optimization results
    ///
    OptimizeScheimpflugTransformResults optimizeScheimpflugTransform(vector<shared_ptr<Image<uint8_t>>>& imageSet,
                                                                     const vector<shared_ptr<vector<float>>>& imgXSet,
                                                                     const vector<shared_ptr<vector<float>>>& imgYSet,
                                                                     const vector<double>& deltaNSet,
                                                                     const vector<double>& HSet,
                                                                     vector<shared_ptr<Profile<int>>>& profileSet);

    CalibrationGridResults calibrateGrid(shared_ptr<Image<uint8_t>> inputImage, bool saveValues = true);

    ///
    /// TODO: Initialization the Scheimpflug Transform
    ///
    void init(double R, double H, bool calibration = false);

    void init(shared_ptr<ScheimpflugTransform> st, double R, double H, bool calibration = false);

    ///
    /// TODO: Returns a string to explain the relative return code
    ///
    int printReturnCode();

    // FIXME:
    double measureOnObjectPlane(double x1, double y1, double x2, double y2);

    // Convert pixel to mm using the Scheimpflug transformation
    double convertPixelToMm(double m);

    // Convert mm to pixel using the Scheimpflug transformation
    double convertMmToPixel(double m);

    // Converts the input image from image plane to object plane
    shared_ptr<Image<uint8_t>> generateImageObjPlane(shared_ptr<Image<uint8_t>> inputImage);

    // Convert points from image plane to object plane
    void convertPointsFromImgToObjPlane(const vector<int>& imgX, const vector<int>& imgY, vector<int>& objX,
                                        vector<int>& objY);

    // Convert points from object plane to image plane
    void convertPointsFromObjToImgPlane(const vector<float>& objX, const vector<float>& objY, vector<int>& imgX,
                                        vector<int>& imgY);

    // Function used to check if the calibration parameters are within the expected range of nominal values
    map<string, bool> checkCalibratedParamsWithNominalRanges(bool checkBoundariesReached = false);

    // Converts coordinates on the image plane to coordinates on the object plane
    void imageToObject(const double* srcImgPoint, double* dstObjPoint);

    ///
    /// Process an entire folder and its subfolders
    ///
    /// \param [in] folder      Starting folder path
    /// \param [in] stats       Shared pointer to a statistics object
    ///
    void initMultiFramesCalibration(const string& folder, vector<shared_ptr<Image<uint8_t>>>& inputImageSet,
                                    vector<double>& deltaNSet, vector<double>& HSet, vector<string>& filenameSet);

    shared_ptr<HttPrivate::ProcessResults> processWithDeltaNVariable(processArgs& args);

private:
    ///
    /// TODO: Pre-process
    ///
    shared_ptr<Profile<int>> preProcess(shared_ptr<Image<uint8_t>> inputImage, string imageLabel,
                                        HttProcessType processType, ModelType modelType);
    shared_ptr<Profile<int>> preProcessTest(shared_ptr<Image<uint8_t>> inputImage, string imageLabel,
                                            HttProcessType processType, ModelType modelType);
    ///
    /// TODO: Converts the image, the profile and the bevel from image plane to object plane
    ///
    shared_ptr<Image<uint8_t>> generateBevelImageOnObjectPlane(shared_ptr<Image<uint8_t>> inputImage,
                                                               shared_ptr<Profile<int>> bevel,
                                                               shared_ptr<Profile<int>> profile);
    ///
    /// TODO: Bevel image
    ///
    shared_ptr<Image<uint8_t>> generateBevelImage(shared_ptr<Image<uint8_t>> inputImage, shared_ptr<Profile<int>> bevel,
                                                  shared_ptr<Profile<int>> profile);

    void drawProfileObjPlane(shared_ptr<Image<uint8_t>> image, vector<int> x, vector<int> y, const int color[],
                             int pixelSize = 1);

    void drawProfileImgPlane(shared_ptr<Image<uint8_t>> image, vector<int> x, vector<int> y, const int color[],
                             int pixelSize = 1);
};
} // namespace HTT
} // namespace Libraries
} // namespace Nidek
#endif // __HTT_PRIVATE_H__

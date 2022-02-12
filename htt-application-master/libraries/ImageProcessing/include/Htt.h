//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __HTT_H__
#define __HTT_H__

#include "HttGlobal.h"
#include "Image.h"
#include "Log.h"
#include "ModelType.h"
#include "ReturnCode.h"
#include "Settings.h"

#include <map>
#include <memory>
#include <vector>

using namespace std;
using namespace Nidek::Libraries::Utils;

namespace Nidek {
namespace Libraries {
namespace HTT {

// Forward declaration
class HttPrivate;
class Statistics;

///
/// \brief Htt class.
///
/// This class is the main entry point of the Htt Processing Library
///
class Htt
{
private:
    shared_ptr<HttPrivate> m_httPrivate;
    shared_ptr<Settings> m_settings;
    shared_ptr<Log> m_log;

public:
    struct CalibrationGridResults
    {
        /// Boolean flag indicating whether the calibration is successful or not.
        bool valid;
        /// Optimum value for the fitness function returned by the PSO.
        float lossValue;
        /// Scheimpflug parameters that identify the calibrated transformation.
        map<string, double> scheimpflugTransformParams;
        /// Set of lines identifying the calibration grid, obtained through the Hough transform in the image plane.
        vector<pair<pair<int, int>, pair<int, int>>> imgPlaneGridLines;
        /// Set of points identified by the intersections of the grid lines, plus the center of the grid, in the image
        /// plane.
        vector<pair<int, int>> imgPlaneIntersectionPoints;
        /// The grid image represented on the object plane mapped with the inverse Scheimpflug transformation.
        shared_ptr<Image<uint8_t>> objPlaneGrid;
        /// Set of points identified by the intersections of the grid lines, plus the center of the grid, in the object
        /// plane.
        vector<pair<int, int>> objPlaneIntersectionPoints;
        /// The grid reconstruction errors
        vector<float> gridErrors;
        /// Return code used to identify the state of the HTT library
        ReturnCode returnCode;
    };

    struct LensFittingResults
    {
        /// Boolean flag indicating whether the lens fitting is successful or not.
        bool valid;
        /// Shared pointer to the input image.
        shared_ptr<Image<uint8_t>> inputImage;
        /// The input image with the processing results superimposed (e.g. frame profile in white, bevel profile in red,
        /// model points in green).
        shared_ptr<Image<uint8_t>> bevelImage;
        /// Set of points representing the line detected on the frame illuminated by the projected slit light.
        vector<pair<int, int>> imgProfile;
        /// The two reference points initially determined on the frame profile from which the PSO search space is
        /// initialized.
        vector<pair<int, int>> imgMaximaPoints;
        /// Set of points representing the line segments associated to the bevel model fit to the frame profile.
        vector<pair<int, int>> imgBevel;
        /// Set of anchor points representing the model (5 points for the Mini bevel, 6 points for the T model, and 7
        /// points for the Custom bevel).
        vector<pair<int, int>> imgModelPoints;
        /// Two points used to compute the distance “D” between the center of the bevel and the estimated outer shoulder
        /// of the frame.
        vector<pair<int, int>> imgDistancePoints;
        /// Shared pointer to the input image remapped onto the object plane using the inverse Scheimpflug
        /// Transformation.
        shared_ptr<Image<uint8_t>> objPlaneImage;
        /// The computed image with the processing results (e.g. frame profile in white, bevel profile in red, model
        /// points in green), remapped onto the object plane.
        shared_ptr<Image<uint8_t>> objBevelImage;
        /// Set of points representing the line detected on the frame illuminated by the projected slit light, remapped
        /// onto the object plane.
        vector<pair<int, int>> objProfile;
        /// The two reference points initially determined on the frame profile from which the PSO search space is
        /// initialized, remapped onto the object plane.
        vector<pair<int, int>> objMaximaPoints;
        /// Set of points representing the line segments associated to the bevel model fit to the frame profile,
        /// remapped onto the object plane.
        vector<pair<int, int>> objBevel;
        /// Set of anchor points representing the model (5 points for the Mini bevel, 6 points for the T model, and 7
        /// points for the Custom bevel), remapped onto the object plane.
        vector<pair<int, int>> objModelPoints;
        /// Two points used to compute the distance “D” between the center of the bevel and the estimated outer shoulder
        /// of the frame, remapped onto the object plane.
        vector<pair<int, int>> objDistancePoints;
        /// Set of measures (in the object plane) representing the model.
        map<string, float> measures;
        /// Calibrated measures of the LTT12-8000 frame bevels used to validate the accurate calibration procedure.
        /// (Information available only during validation).
        map<string, float> realMeasures;
        /// Measures error between the computed measures and the calibrated ones. (Information available only during
        /// validation).
        map<string, int> errors;
        /// Model type used for the fitting (i.e. “MINI_BEVEL”, “T_BEVEL”, or “CUSTOM_BEVEL”).
        ModelType modelType;
    };

    struct AccurateCalibrationResults
    {
        /// Boolean flag indicating whether the calibration is successful or not.
        bool valid;
        /// Scheimpflug parameters that identify the calibrated system.
        map<string, float> scheimpflugTransformParams;
        /// The resut of the lens fitting after the accurate calibration
        LensFittingResults lensValidationResults;
    };

    struct ProfileDetectionResults
    {
        /// Boolean flag indicating whether the profile detection is successful or not.
        bool valid;
        /// Set of points representing the line detected on the frame illuminated by the projected slit light.
        vector<pair<int, int>> imgProfile;
    };

    ///
    /// Constructs a new instance of the Htt class using the given parameters.
    ///
    HTT_LIBRARYSHARED_EXPORT Htt();

    ///
    /// Function used to calibrate the system starting from the given grid image.
    ///
    /// \param  [in] inputImage     Shared pointer to the grid image used for the calibration.
    /// \param  [in] saveValues     Optional flag used to enable/disable the saving of the Scheimpflug parameters in
    /// the JSON file. Enabled by default.
    ///
    /// \return Structure with the calibration results (e.g. the Scheimpflug paramters, the grid lines and the
    /// intersection points detected).
    ///
    HTT_LIBRARYSHARED_EXPORT CalibrationGridResults calibrateGrid(shared_ptr<Image<uint8_t>> inputImage,
                                                                  bool saveValues = true);

    ///
    /// Function used to perform an accurate calibration starting from from a single image of the LTT12-8000 calibration
    /// tool.
    ///
    /// \param  [in] inputImage     Shared pointer to the frame image used for the accurate calibration.
    /// \param  [in] deltaN         The angle between the normal plane to the frame plane and the stylus, in degrees.
    /// \param  [in] H              The angle of rotation of the sylus around the "h axis" (i.e. rotation of the bevel
    /// image in the camera sensor), in degrees.
    /// \param  [in] filename       Image filename used as a prefix name for the images generated and saved inside the
    /// function resulting from the computation.
    ///
    /// \return Structure with the accurate calibration results (i.e. the flag indicating the results of the computation
    /// and the Scheimpflug paramters).
    ///
    HTT_LIBRARYSHARED_EXPORT AccurateCalibrationResults accurateCalibration(shared_ptr<Image<uint8_t>> inputImage,
                                                                            const double& deltaN, const double& H,
                                                                            const string& filename);

    ///
    /// Function used to perform an accurate calibration starting from from images of the LTT12-8000 calibration tool.
    ///
    /// \param  [in] inputImageSet  Shared pointers to the frame images used for the accurate calibration.
    /// \param  [in] deltaN         Angles between the normal plane to the frame plane and the stylus, in degrees for
    /// each image.
    /// \param  [in] H              Angles of rotation of the sylus around the "h axis" (i.e. rotation of the bevel
    /// image in the camera sensor), in degrees for each image.
    /// \param  [in] filename       Image filenames used as a prefix name for the images generated and saved inside the
    /// function resulting from the computation.
    ///
    /// \return Structure with the accurate calibration results (i.e. the flag indicating the results of the computation
    /// and the Scheimpflug paramters).
    ///
    HTT_LIBRARYSHARED_EXPORT AccurateCalibrationResults
    accurateCalibration(vector<shared_ptr<Image<uint8_t>>>& inputImageSet, const vector<double>& deltaN,
                        const vector<double>& H, const vector<string>& filenameSet);

    ///
    /// Funtion used to fit a bevel model to the detected frame profile in a calibrated system.
    ///
    /// \param  [in] inputImage     Shared pointer to the frame image of which you want to find the bevel model
    /// \param  [in] deltaN         The angle between the normal plane to the frame plane and the stylus, in degrees.
    /// \param  [in] H              The angle of rotation of the sylus around the "h axis" (i.e. rotation of the bevel
    /// image in the camera sensor), in degrees.
    /// \param  [in] modelType      Model type used for the fitting.
    ///
    /// \return Struct with the lens fitting results (e.g. the frame profile detected and the bevel model constituted by
    /// line segments which is fot to the frame profile).
    ///
    HTT_LIBRARYSHARED_EXPORT LensFittingResults lensFitting(shared_ptr<Image<uint8_t>> inputImage, const double& deltaN,
                                                            const double& H, ModelType modelType);

    HTT_LIBRARYSHARED_EXPORT LensFittingResults lensValidation(shared_ptr<Image<uint8_t>> inputImage,
                                                               const double& deltaN, const double& H,
                                                               ModelType modelType);

    HTT_LIBRARYSHARED_EXPORT ProfileDetectionResults profileDetection(shared_ptr<Image<uint8_t>> inputImage,
                                                                      const double& deltaN, const double& H,
                                                                      ModelType modelType);

    ///
    /// Function used to process all PNG images in the given directory.
    ///
    /// \param  [in] folder     Folder path with images to be processed.
    /// \param  [in] stats      Optional shared pointer to the statistics, used for collect information of the processed
    /// images. If stats == nullPtr, statistics are not computed.
    ///
    void processFolder(const string& folder, shared_ptr<Statistics> stats = nullptr);

    ///
    /// Returns a string to explain the relative return code.
    ///
    HTT_LIBRARYSHARED_EXPORT int printReturnCode();

    ///
    /// Function use to compute the distance (in the object plane) between two anchor points (x1,y1) and (x2,y2).
    ///
    /// \param  [in] x1     X coordinate of the first anchor point
    /// \param  [in] y1     Y coordinate of the first anchor point
    /// \param  [in] x2     X coordinate of the second anchor point
    /// \param  [in] y2     Y coordinate of the second anchor point
    ///
    /// \return The measure of the distance in mm.
    ///
    HTT_LIBRARYSHARED_EXPORT double measureOnObjectPlane(double x1, double y1, double x2, double y2);

    ///
    /// Function used to returns the string representing the parameters used during calibration procedure.
    ///
    /// \return  A string containing the JSON parameters.
    ///
    HTT_LIBRARYSHARED_EXPORT string getCalibrationParams();

    ///
    /// Function used to check if the parameters of the Scheimpflug transformation are within the admissible ranges.
    ///
    /// \return A map with pairs (parameter, result).
    ///
    HTT_LIBRARYSHARED_EXPORT map<string, bool> checkCalibratedParams();

    ///
    /// Function used to return the minimum Imaging Field of View
    ///
    /// \param [in] x       X coordinate of the center point of the Field of View
    /// \param [in] y       Y coordinate of the center point of the Field of View
    /// \param [in] width   Width of the minimum Imaging Field of View
    /// \param [in] height  Height of the minimum Imaging Field of View
    ///
    /// \return Coordinates that rapresent the Field of View to visualize
    ///
    HTT_LIBRARYSHARED_EXPORT vector<pair<int, int>> getMinimumImagingFoV(const int& x, const int& y,
                                                                         const double& width, const double& height);

    ///
    /// Function used to return the reference square of the grid in the image plane
    ///
    /// \param [in] width   Width of the reference square
    /// \param [in] height  Height of the reference square
    ///
    /// \return Coordinates that rapresent the reference square
    ///
    HTT_LIBRARYSHARED_EXPORT vector<pair<int, int>> getReferenceSquare(const double& width, const double& height);

    void initMultiFramesCalibration(const string& folder, vector<shared_ptr<Image<uint8_t>>>& inputImageSet,
                                    vector<double>& deltaNSet, vector<double>& HSet, vector<string>& filenameSet);

private:
    LensFittingResults bevelFitting(shared_ptr<Image<uint8_t>> inputImage, const double& deltaN, const double& H,
                                    ModelType modelType, bool checkIntersections);
};

} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __HTT_H__

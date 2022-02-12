//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __MODEL_H__
#define __MODEL_H__

#include <map>
#include <memory>
#include <vector>

#include "Image.h"
#include "Log.h"
#include "ModelType.h"
#include "Profile.h"
#include "ScheimpflugTransform.h"

using namespace std;

namespace Nidek {
namespace Libraries {
namespace HTT {

class Model
{
public:
    class Measures
    {
    public:
        map<string, float> m;
    };

public:
    ///
    /// \brief Constructor
    ///
    /// This is the base abstract class representing a bevel model.
    ///
    /// \param st [in]   Shared pointer to the ScheimpflugTransform object.
    ///
    explicit Model(shared_ptr<ScheimpflugTransform> st);

    ///
    /// \brief Destructor
    ///
    virtual ~Model();
    shared_ptr<ScheimpflugTransform> scheimpflugTransform();
    ///
    /// \brief Set the loss field and the profile in the Model.
    ///
    /// Set the loss field and the profile which were computed outside the model.
    /// - The loss field is the weight associated to each pixel in the image
    ///   and is used by the optimizer.
    /// - The profile is the bevel profile detected by thresholding on the image.
    ///
    /// \param lossField             [in]  Shared pointer to the loss field.
    /// \param profile               [in]  Shared pointer to the detected profile.
    /// \param checkIntersections    [in]  Flag enabling the checking of intersections between bevel and profile.
    /// \param intersectionThreshold [in]  Threshold on the number of points above which an intersection is detected.
    ///
    void setLossFieldAndProfile(shared_ptr<Image<float>> lossField, shared_ptr<Profile<int>> profile,
                                bool checkIntersections, int intersectionThreshold, vector<double>& weight);

    ///
    /// \brief Compute the loss function for the current bevel.
    ///
    /// Compute the value of the loss function for the bevel model.
    ///
    /// \return the value of the loss function for the current bevel.
    ///
    float mse();

    ///
    /// \brief get the R angle
    ///
    /// \return the R angle in degrees.
    ///
    double getR();

    ///
    /// \brief get the H angle
    ///
    /// \return the H angle in degrees.
    ///
    double getH();

    ///
    /// \brief Get the measures of the bevel.
    ///
    /// Get the bevel measures. This function is pure virtual and is overridden
    /// derived models.
    ///
    /// \return A shared pointer of a Measure object.
    ///
    virtual shared_ptr<Model::Measures> getMeasures() = 0;

    virtual ModelType modelType() = 0;

    ///
    /// \brief Compute the bevel defining points in the image plane.
    ///
    /// Compute the bevel defining poins in the image plane starting from the
    /// coordinates of the points in the object plane using the Scheimpflug
    /// transformation. This function changes the values in m_imgX and m_imgY.
    ///
    /// \param objX [in] Bevel defining points X coordinates in the object plane.
    /// \param objY [in] Bevel defining points Y coordinates in the object plane.
    ///
    void computeImgPoints(const vector<double>& objX, const vector<double>& objY);

    // Compute object points
    virtual void computeObjPoints();

    // Recalculate the image points given the model points on the image plane
    void recomputeImgPoints();

    ///
    /// \brief Print the coordinates of the bevel anchor points in the image plane.
    ///
    void printImgPoints();

    ///
    /// \brief Print the coordinates of the bevel anchor points in the object plane.
    ///
    void printObjPoints();

    void printBevel();
    virtual void printMeasures() = 0;

    ///
    /// \brief Converts coordinates from the object plane to the image plane.
    ///
    /// Converts coordinates on the object plane to coordinates on the image plane.
    ///
    /// \params srcObjPoint [in] Array with 3 elements with the anchor points defined in the object plane.
    /// \params srcObjPoint [out] Array with 3 elements with the anchor points defined in the image plane.
    ///
    void objectToImage(double* srcObjPoint, double* dstImgPoint);

    ///
    /// \brief Converts coordinates from the image plane to the object plane.
    ///
    /// Converts coordinates on the image plane to coordinates on the object plane
    ///
    /// \params srcObjPoint [in] Array with 3 elements with the anchor points defined in the image plane.
    /// \params srcObjPoint [out] Array with 3 elements with the anchor points defined in the object plane.
    ///
    void imageToObject(double* srcImgPoint, double* dstObjPoint);

    ///
    /// \brief Convert pixel to mm.
    ///
    /// \param v [in] Value in pixel to convert to mm.
    ///
    /// \return value in mm.
    ///
    double pixelToMm(double v);

    ///
    /// \brief Convert mm to pixel.
    ///
    /// \param v [in] Value in mm to convert to pixels.
    ///
    /// \return value in pixel.
    ///
    double mmToPixel(double v);

    ///
    /// \brief Get the Bevel representation.
    ///
    /// \return the bevel points.
    ///
    shared_ptr<Profile<int>> getBevel();

    virtual bool finalizeModel(const float* v, const int* lastImgPointProfile,
                               shared_ptr<Profile<int>> profile = nullptr) = 0;

    ///
    /// \brief Set the debug flag.
    ///
    /// \param debug [in]  New value for the debug flag.
    ///
    void setDebug(bool debug);

    ///
    /// \brief Set the parameters that define the bevel.
    ///
    /// \param v [in] Vector with the parameters.
    /// \param offset [in] Offset in mm used to adapt the profile to the bevel
    ///
    /// \return True if the bevel is valid with respect to the profile, false otherwise.
    ///
    virtual bool setFreeParams(const float* v, const double* offset = nullptr) = 0;

protected:
    ///
    /// \brief Compute the bevel profile.
    ///
    /// Determine the bevel points on the image given
    /// the model, and check if the constraints are met.
    ///
    /// \return True if the bevel is valid with respect to the profile, false otherwise.
    ///
    bool computeConstrainedBevelProfile();
    bool computeConstrainedTBevelProfile();
    bool computeFinalBevelProfile();

    ///
    /// \brief Get the number of intersections points between the bevel and the profile.
    ///
    /// \return the number of intersection points.
    ///
    int getIntersectionCounter();

protected:
    shared_ptr<Log> log;

    // Object handling the Scheimpflug transformation.
    shared_ptr<ScheimpflugTransform> m_st;

    // Loss field
    shared_ptr<Image<float>> m_lossField;
    int m_lossFieldWidth;
    int m_lossFieldHeight;
    int m_lossFieldSpan;
    float** m_lossFieldRowPtr;

    // Frame profile detected by thresholding
    shared_ptr<Profile<int>> m_profile;

    // Bevel
    shared_ptr<Profile<int>> m_bevel;
    int* m_bevelX;
    int* m_bevelY;
    int m_bevelN;

    // Model points onto the image plane
    vector<float> m_imgX;
    vector<float> m_imgY;

    // Coordinates of the points used to compute the distance D onto the image plane
    vector<float> m_distanceImgX;
    vector<float> m_distanceImgY;

    // Model points onto the object plane
    vector<double> m_objX;
    vector<double> m_objY;

    // Constraints
    int* m_leftBoundaryX;

    int m_intersectionCounter;
    bool m_checkIntersections;
    int m_intersectionThreshold;

    bool m_debug;

    float m_mse;

    // Vector with the weights for each segments used by the loss function
    vector<double> m_weight;
};

} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __MODEL_H__

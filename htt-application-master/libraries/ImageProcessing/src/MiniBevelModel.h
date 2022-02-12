//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __MINI_BEVEL_MODEL_H__
#define __MINI_BEVEL_MODEL_H__

#include "Model.h"
#include <math.h>
#include <memory>
#include <vector>

namespace Nidek {
namespace Libraries {
namespace HTT {

class MiniBevelModel : public Model
{
public:
    // Constructor
    // R and H represent the stylus orientation in degrees
    explicit MiniBevelModel(shared_ptr<ScheimpflugTransform> st, double bevelAngle = 55.0);

    // Set the model free parameters.
    // v: Vector with models free parameters
    virtual bool setFreeParams(const float* v, const double* offset = nullptr) override;

    // Get the bevel measures.
    virtual shared_ptr<Model::Measures> getMeasures() override;

    // Get the model type.
    virtual ModelType modelType() override;

    // Finalize the model
    virtual bool finalizeModel(const float* v, const int* lastImgPointProfile, shared_ptr<Profile<int>> profile = nullptr) override;

    // Set the model from image points instead of object points
    void setFromImgPoints(const vector<float>& imgX, const vector<float>& imgY);
    void getTestMeasures(double& B, double& M, double& angle1, double& angle2);
    virtual void computeObjPoints() override;
    virtual void printMeasures() override;

private:
    // Compute other parameters
    void computeOtherParams();

private:
    // Coordinates of the bevel model on the object plane
    double m_A;
    double m_B;
    double m_Bstar;
    double m_C;
    double m_D;
    double m_M;
    double m_alpha;
    double m_delta;
    double m_tanDelta;
    double m_xm;
    double m_ym;
};

} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __MINI_BEVEL_MODEL_H__

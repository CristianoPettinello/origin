//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __CUSTOM_BEVEL_MODEL_H__
#define __CUSTOM_BEVEL_MODEL_H__

#include "Model.h"
#include "ScheimpflugTransform.h"

namespace Nidek {
namespace Libraries {
namespace HTT {

class CustomBevelModel : public Model
{
public:
    // Constructor
    explicit CustomBevelModel(shared_ptr<ScheimpflugTransform> st);

    // Set the model free parameters.
    // v: Vector with models free parameters
    virtual bool setFreeParams(const float* v, const double* offset = nullptr) override;

    // Get the bevel measures.
    virtual shared_ptr<Model::Measures> getMeasures() override;

    // Get the model type.
    virtual ModelType modelType() override;

    // Finalize the model
    virtual bool finalizeModel(const float* v, const int* lastImgPointProfile, shared_ptr<Profile<int>> profile = nullptr) override;

    virtual void printMeasures() override;

private:
    // Compute other parameters
    void computeOtherParams();

private:
    // Coordinates of the bevel model on the object plane
    double m_A; // Frame front shoulder length
    double m_C; // Frame rear shoulder length
    double m_D; // measure 1 - Bevel height (front)
    double m_E; // measure 2 - Bevel apex width
    double m_F; // measure 3 - Bevel height (rear)
    double m_G; // measure 4 - Step width
    double m_M; // distance p2-p5
    double m_B; // distance p2-pm
    double m_S; // distance p1-p2
    double m_alpha;
    double m_xm; // x position of point m
    double m_ym; // y position of point m
    double m_tanDelta1;
    double m_tanDelta2;
};
} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __CUSTOM_BEVEL_MODEL_H__

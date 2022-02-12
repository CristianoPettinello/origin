//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __T_BEVEL_MODEL_H__
#define __T_BEVEL_MODEL_H__

#include "Model.h"
#include "ScheimpflugTransform.h"

namespace Nidek {
namespace Libraries {
namespace HTT {

class TBevelModel : public Model
{
public:
    // Constructor
    explicit TBevelModel(shared_ptr<ScheimpflugTransform> st);

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
    void computeOtherParams(const double* offset);

private:
    // Coordinates of the bevel model on the object plane
    double m_A; // Frame front shoulder length
    double m_C; // Frame rear shoulder length
    double m_B; // Bevel height
    double m_M; // Bevel width
    double m_D; // Frame rear shoulder length + half bevel width
    double m_alpha;
    double m_xm;
    double m_ym;
};
} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __T_BEVEL_MODEL_H__

//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __OPTIMIZATION_ENGINE_H__
#define __OPTIMIZATION_ENGINE_H__

#include <memory>
#include <vector>

#include "HttProcessType.h"
#include "Image.h"
#include "Log.h"
#include "MiniBevelModel.h"
#include "Model.h"
#include "ModelType.h"
#include "Profile.h"
#include "Pso.h"
#include "ReturnCode.h"

using namespace std;

namespace Nidek {
namespace Libraries {
namespace HTT {

class OptimizationEngine
{
public:
    class Output
    {
    public:
        // Bevel
        shared_ptr<Profile<int>> bevel;

        // Measures
        shared_ptr<Model::Measures> measures;
        // Value of the loss function after the optimization
        float loss;

        // PSO iteration at which the optimum solution was found
        int iteration;

        // Model type
        ModelType modelType;

        // HTT angle w.r.t. the normal in degrees
        double R;

        // HTT's H angle in degrees
        double H;

        ReturnCode rcode;
    };

    class ScheimpflugTransformOutput
    {
    public:
        // Bevel resulting from the optimization
        shared_ptr<Profile<int>> bevel;

        // Optimum transform parameters
        shared_ptr<ScheimpflugTransform::Params> params;

        // Bevel measures
        shared_ptr<Model::Measures> measures;

        // Output of the loss function for the returned parms after the optimization
        float loss;

        // Iteration at which the optimum solution was found.
        int iteration;

        // Flag which is true if the calibration was successful, false otherwise.
        bool validCalibration;

        // Model type
        ModelType modelType;

        // HTT angle w.r.t. the normal in degrees
        double R;

        // HTT's H angle in degrees
        double H;

        ReturnCode rcode;
    };

    class CalibrationGridOutput
    {
    public:
        // Optimum transform parameters
        shared_ptr<ScheimpflugTransform::Params> params;

        // Output of the loss function for the returned parms after the optimization
        float loss;

        // Iteration at which the optimum solution was found.
        int iteration;

        // Flag which is true if the calibration was successful, false otherwise.
        bool validCalibration;

        // Calibration grid point in the objec plain
        shared_ptr<Profile<int>> intersectionPoints;

        ReturnCode rcode;
    };

public:
    // Default constructor
    OptimizationEngine();

    // Initialize the model
    shared_ptr<Model> initModel(shared_ptr<ScheimpflugTransform> st, shared_ptr<Profile<int>> profile,
                                ModelType modelType, HttProcessType processType, Pso<float>::Input& psoInput);

    // Optimize the model
    shared_ptr<OptimizationEngine::Output> optimizeModel(shared_ptr<Model> model, Pso<float>::Input& psoInput,
                                                         shared_ptr<Profile<int>> profile, const int maxRetries);

    // Optimize the Scheimpflug Transform parameters
    shared_ptr<OptimizationEngine::ScheimpflugTransformOutput>
    optimizeScheimpflugTransform(shared_ptr<ScheimpflugTransform> st, shared_ptr<Profile<int>> profile,
                                 const vector<float>& imgX, const vector<float>& imgY);

    shared_ptr<OptimizationEngine::ScheimpflugTransformOutput> optimizeScheimpflugTransform(
        vector<shared_ptr<ScheimpflugTransform>>& stSet, vector<shared_ptr<Profile<int>>>& profileSet,
        const vector<shared_ptr<vector<float>>>& imgXSet, const vector<shared_ptr<vector<float>>>& imgYSet);

    void printMetrics(shared_ptr<vector<shared_ptr<MiniBevelModel>>> modelSet, double B_star, double M_star,
                      double angle1_star, double angle2_star, int kTraining);

    // Optimize the Scheimpflug Transform parameters using the calibration grid
    shared_ptr<OptimizationEngine::CalibrationGridOutput> optimizeCalibrationGrid(shared_ptr<ScheimpflugTransform> st,
                                                                                  const vector<int>& imgX,
                                                                                  const vector<int>& imgY,
                                                                                  bool saveValues);

private:
    void precomputeLossField(shared_ptr<Profile<int>> profile);

    // Get the min and max value in vector v
    void getMinMax(const vector<int>& v, int& min, int& max);

    // Calculate the two maxima given a profile profile
    bool getSearchSpace(shared_ptr<Profile<int>> profile, int& upperMaxX, int& upperMaxY, int& lowerMaxX,
                        int& lowerMaxY, int& minimumX, int& minimumY, ModelType modelType);

    shared_ptr<Image<float>> lossFieldNormalize(shared_ptr<Image<float>> srcImage);

private:
    shared_ptr<Log> log;
    bool m_debug;
    int m_width;
    int m_height;
    int m_lossFieldMargin;
    int m_lossFieldRightOffset;
    int m_lossFieldLeftOffset;
    int m_leftGradientWidth;

    shared_ptr<Image<float>> m_lossField;
};
} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __OPTIMIZATION_ENGINE_H__

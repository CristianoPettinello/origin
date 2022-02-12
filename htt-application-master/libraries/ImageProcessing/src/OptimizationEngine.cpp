//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "OptimizationEngine.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdio.h>

#include "CustomBevelModel.h"
#include "Log.h"
#include "MiniBevelModel.h"
#include "MiniBevelModelExt.h"
#include "ModelType.h"
#include "Pso.h"
#include "Settings.h"
#include "TBevelModel.h"
#include "Utils.h"
#include "json/json.h"
#include <assert.h>

using namespace Nidek::Libraries::HTT;

#ifdef UNIX
#define sprintf_s(buf, ...) sprintf((buf), __VA_ARGS__)
#endif

static const string tag = "[OPTI] ";

OptimizationEngine::OptimizationEngine()
    : log(Log::getInstance()),
      m_debug(false),
      m_width(0),
      m_height(0),
      m_lossFieldMargin(0),
      m_lossFieldRightOffset(0),
      m_lossFieldLeftOffset(0),
      m_leftGradientWidth(0)
{
}

void OptimizationEngine::getMinMax(const vector<int>& v, int& min, int& max)
{
    const int N = (int)v.size();
    min = v[0];
    max = min;
    for (int n = 1; n < N; ++n)
    {
        int val = v[n];
        if (min > val)
            min = val;

        if (max < val)
            max = val;
    }
}

#if 0
bool OptimizationEngine::getSearchSpace(shared_ptr<Profile<int>> profile, int& upperMaxX, int& upperMaxY,
                                        int& lowerMaxX, int& lowerMaxY, int& minimumX, int& minimumY)
{
    // Find minimum and maximum values of detected profile.
    int minX = 0, maxX = 0;
    getMinMax(profile->x, minX, maxX);

    int minY = 0, maxY = 0;
    getMinMax(profile->y, minY, maxY);

    int midY = (minY + maxY) >> 1;
    /*if (m_debug)
    {
        stringstream s;
        s << tag << "minY: " << minY << " maxY: " << maxY << " midY: " << midY;
        log->debug(s.str());
    }*/

    // Find the Y position of the two maxima
    int halfHeight = midY;
    int N = (int)profile->x.size();


    // FIXME - test con derivata prima
    int n = 0;
    int m = N - 1;

    const int offset = 10; // 20;
    double threshold = 5.0 / offset;
    double dx = 0.0;
    double dx2 = dx;

    int watchdog = 0;
    while (fabs(dx - dx2) < 0.2)
    {
        if (++watchdog > 6)
            break;

        //for (int i = 0; i < N - offset; ++i)
        for (int i = 200; i >= offset; --i)
        {
            // VERSIONE orig
            int deltaX = profile->x[i] - profile->x[i - offset];
            int deltaY = profile->y[i] - profile->y[i - offset];
            if (deltaY != 0)
                dx = (double)deltaX / deltaY;

             //cout << "   " << deltaX << " " << deltaY << " " << profile->x[i] << " " << dx << endl;
             //cout << "   " << dx << " " << threshold << endl;
            if (dx <= -threshold)
            {
                int index = i - offset * 0.5;
                upperMaxX = profile->x[index];
                upperMaxY = profile->y[index];
                n = index;

                // FIXME: index - offset will be lower than 0
                int minDeltaIndex = index - offset;
                int maxDeltaIndex = index + offset;
                if (minDeltaIndex < 0)
                    minDeltaIndex = 0;
                if (maxDeltaIndex > N)
                    maxDeltaIndex = N;
                int deltaXtmp = profile->x[minDeltaIndex] - profile->x[maxDeltaIndex];
                int deltaYtmp = profile->y[minDeltaIndex] - profile->y[maxDeltaIndex];
                // double dx2 = 0;

                if (deltaYtmp != 0) // && watchdog == 1)
                    dx2 = (double)deltaXtmp / deltaYtmp;

                // cout << dx << " " << dx2 << " " << threshold << " " << upperMaxX << " " << upperMaxY
                //      << "  C: " << fabs(dx - dx2) << endl;
                break;
            }
        }
        if (dx2 > dx)
            threshold -= 0.05;
        else if (dx2 < dx)
            threshold += 0.05;
    }
    cout << "upperMax: (" << upperMaxX << "," << upperMaxY << ") n: " << n << endl;

    // FIXME
    threshold = 5.0 / offset;
    dx = 0.0;
    dx2 = 0.0;
    watchdog = 0;
    while (fabs(dx - dx2) < 0.2)
    {
        if (++watchdog > 6)
            break;
        for (int i = N - 1; i >= offset; --i)
        {
            int deltaX = profile->x[i] - profile->x[i - offset];
            int deltaY = profile->y[i] - profile->y[i - offset];
            // cout << "   " << deltaX << " " << deltaY << " " << profile->x[i] << " " << dx << " " << threshold <<
            // endl;
            if (deltaY != 0)
                dx = (double)deltaX / deltaY;

            if (dx >= threshold)
            {
                int index = i - offset * 0.5;
                lowerMaxX = profile->x[index];
                lowerMaxY = profile->y[index];
                m = index;

                // // FIXME: index - offset will be greater than N-1
                // int deltaXtmp = profile->x[index + offset] - profile->x[index - offset];
                // int deltaYtmp = profile->y[index + offset] - profile->y[index - offset];

                // FIXME: index - offset will be greater than N-1
                int minDeltaIndex = index - offset;
                int maxDeltaIndex = index + offset;
                if (minDeltaIndex < 0)
                    minDeltaIndex = 0;
                if (maxDeltaIndex > N - 1)
                    maxDeltaIndex = N - 1;
                int deltaXtmp = profile->x[maxDeltaIndex] - profile->x[minDeltaIndex];
                int deltaYtmp = profile->y[maxDeltaIndex] - profile->y[minDeltaIndex];

                if (deltaYtmp != 0) // && watchdog == 1)
                    dx2 = (double)deltaXtmp / deltaYtmp;

                // cout << dx << " " << dx2 << " " << threshold << " " << lowerMaxX << " " << lowerMaxY
                //      << "  C: " << fabs(dx - dx2) << endl;
                break;
            }
        }
        if (dx2 > dx)
            threshold += 0.05;
        else if (dx2 < dx)
            threshold -= 0.05;
    }
    cout << "lowerMax: (" << lowerMaxX << "," << lowerMaxY << ") m: " << m << endl;

    // Search the minimun between the two maxima
    minimumX = upperMaxX;
    minimumY = upperMaxY;
    for (int i = n; i < m; ++i)
    {
        int x = profile->x[i];
        int y = profile->y[i];

        if (x < minimumX)
        {
            minimumX = x;
            minimumY = y;
        }
    }

    return true;
}

#else
bool OptimizationEngine::getSearchSpace(shared_ptr<Profile<int>> profile, int& upperMaxX, int& upperMaxY,
                                        int& lowerMaxX, int& lowerMaxY, int& minimumX, int& minimumY,
                                        ModelType modelType)
{
    // Find minimum and maximum values of detected profile.
    int minX = 0, maxX = 0;
    getMinMax(profile->x, minX, maxX);

    int minY = 0, maxY = 0;
    getMinMax(profile->y, minY, maxY);

    /*if (m_debug)
    {
        int midY = (minY + maxY) >> 1;
        stringstream s;
        s << tag << "minY: " << minY << " maxY: " << maxY << " midY: " << midY;
        log->debug(s.str());
    }*/

    // Find the Y position of the two maxima
    int N = (int)profile->x.size();
    // FIXME: NBr
    int halfN = (N + 1) >> 1;

    /*int Nref = 0;
    for (int n = 0; n < N; ++n)
    {
        if (profile->y[n] > m_height)
        {
            Nref = n > 1 ? n - 1 : 0;
            break;
        }
    }*/

    int n = 0;
    int m = N - 1;

    Json::Value searchSpace = Settings::getInstance()->getSettings()["search_space"];

    // 0: MiniBevel, 1: TBevel, 2: CustomBevel, 3: MiniBevelExt
    int modelIndex = 0;
    switch (modelType)
    {
    case MiniBevelModelType:
        modelIndex = 0;
        break;
    case TBevelModelType:
        modelIndex = 1;
        break;
    case CustomBevelModelType:
        modelIndex = 2;
        break;
    case MiniBevelModelExtType:
        modelIndex = 3;
        break;
    default:
        M_Assert(false, "Wrong Model selected");
        log->error(__FILE__, __LINE__, "Wrong Model selected");
        return false;
    }
    int offset = searchSpace[modelIndex]["referencePoints"]["offset"].asInt();
    double upThreshold = searchSpace[modelIndex]["referencePoints"]["upThreshold"].asDouble();
    double loThreshold = searchSpace[modelIndex]["referencePoints"]["loThreshold"].asDouble();
    double upThresholdRef = searchSpace[modelIndex]["referencePoints"]["upThresholdRef"].asDouble();
    double loThresholdRef = searchSpace[modelIndex]["referencePoints"]["loThresholdRef"].asDouble();

    cout << offset << " " << upThreshold << endl;

    // Step used when the candidate points are not found
    double upThresholdStep = upThreshold / 10;
    double loThresholdStep = loThreshold / 10;

    if (m_debug)
    {
        stringstream s;
        s << tag << "upThreshold: " << upThreshold << "  upThresholdRef: " << upThresholdRef;
        log->debug(s.str());
        s.str("");
        s << tag << "loThreshold: " << loThreshold << "  loThresholdRef: " << loThresholdRef;
        log->debug(s.str());
    }

    double dx = 0.0;

    int watchdog = 0;
    bool loopCondition = true;
    bool candidatePointFound = false;

    const int maxWatchdog = 6;

    while (loopCondition)
    {
        if (++watchdog > maxWatchdog)
        {
            log->warn(tag + "Whatchdog (upperMax) activated");
            loopCondition = false;
            break;
        }

        // int xRef = -1;
        // int iRef = 0;
        int index = 0;

        // FIXME: NBr
        // for (int i = 0; i < N - offset; ++i)
        for (int i = 0; i < halfN - offset; ++i)
        {
            double deltaX = (double)(profile->x[i + offset] - profile->x[i]);
            double deltaY = (double)(profile->y[i + offset] - profile->y[i]);
            if (fabs(deltaY) >= 1.0)
            {
                dx = deltaX / deltaY;

                if (dx <= -upThreshold)
                {
                    index = i + offset / 2;
                    upperMaxX = profile->x[index];
                    upperMaxY = profile->y[index];
                    n = index;
                    // xRef = profile->x[i + offset];
                    // iRef = i + offset;

                    if (m_debug)
                    {
                        stringstream s;
                        s << tag << "dx/dy (fw): " << dx << " @ " << n << " (" << upperMaxX << "," << upperMaxY << ")";
                        log->debug(s.str());
                    }

                    candidatePointFound = true;
                    break;
                }
            }
        }

        // Handle the case when a candidate point was not found
        if (!candidatePointFound)
        {
            // The candidate point for the upperMax was not found. Retry the detection using a smaller threshold
            upThreshold -= upThresholdStep;

            if (m_debug)
            {
                stringstream s;
                s << tag << "Set new upThreshold to: " << upThreshold;
                log->debug(s.str());
            }

            continue;
        }

        // // check that there aren't points higher than xRef
        // loopCondition = false;
        // for (int i = iRef; i < Nref; ++i)
        // {
        //     if (xRef < profile->x[i])
        //     {
        //         cout << "Found a wrong point" << endl;
        //         loopCondition = true;
        //         break;
        //     }
        // }

        // if (loopCondition == false)
        // {
        // Moves upward
        for (int i = index; i >= offset; --i)
        {
            double deltaX = (double)(profile->x[i - offset] - profile->x[i]);
            double deltaY = (double)(profile->y[i - offset] - profile->y[i]);
            if (fabs(deltaY) >= 1.0)
            {
                dx = deltaX / deltaY;

                if (m_debug)
                {
                    stringstream s;
                    s << tag << "dx/dy (bw): " << dx << " dx/dy (ref): " << upThresholdRef << " @ " << i - offset / 2;
                    log->debug(s.str());
                }

                if (dx >= -upThresholdRef)
                {
                    int idx = i - offset / 2;
                    upperMaxX = profile->x[idx];
                    upperMaxY = profile->y[idx];
                    n = idx;

                    // FIXME: NBr
                    if (m_debug)
                    {
                        stringstream s;
                        s << tag << "dx/dy (bw): " << dx << " @ " << n << " (" << upperMaxX << "," << upperMaxY << ")";
                        log->debug(s.str());
                    }

                    break;
                }
            }
            // }

            // FIXME: NBr
            loopCondition = false;
        }
    }

    // Check if the candidate upperMax was found
    if (!candidatePointFound)
    {
        log->warn(tag + "Candidate upperMax not found.");
        return false;
    }

    if (m_debug)
    {
        stringstream s;
        s << tag << "upperMax: (" << upperMaxX << "," << upperMaxY << ") n: " << n;
        log->debug(s.str());
    }

    cout << endl;

    /////////////////--------------------------------------------------------------

    dx = 0.0;
    loopCondition = true;
    candidatePointFound = false;
    watchdog = 0;
    while (loopCondition)
    {
        if (++watchdog > maxWatchdog)
        {
            log->warn(tag + "Whatchdog (lowerMax) activated");
            loopCondition = false;
            break;
        }

        // int xRef = -1;
        // int iRef = 0;
        int index = 0;
        // FIXME: NBr
        // cout << "halfN: " << halfN << " " << profile->x[halfN] << "," << profile->y[halfN] << endl;
        // for (int i = N - 1; i >= offset; --i)
        for (int i = N - 1; i >= halfN - offset; --i)
        {
            double deltaX = (double)(profile->x[i] - profile->x[i - offset]);
            double deltaY = (double)(profile->y[i] - profile->y[i - offset]);
            // cout << "i: " << i << " deltaX:" << deltaX << " deltaY:" << deltaY << endl;
            if (fabs(deltaY) >= 1.0)
            {
                dx = deltaX / deltaY;

                // cout << "dx: " << dx << endl;

                if (dx >= loThreshold)
                {
                    index = i - offset / 2;
                    lowerMaxX = profile->x[index];
                    lowerMaxY = profile->y[index];
                    m = index;
                    // xRef = profile->x[i - offset];
                    // iRef = i - offset;

                    if (m_debug)
                    {
                        stringstream s;
                        s << tag << "dx/dy (bw): " << dx << " @ " << m << " (" << lowerMaxX << "," << lowerMaxY << ")";
                        log->debug(s.str());
                    }

                    candidatePointFound = true;
                    break;
                }
            }
        }
        // Handle the case when a candidate point was not found
        if (candidatePointFound == false)
        {
            // The candidate point for the lowerMax was not found
            loThreshold -= loThresholdStep;

            if (m_debug)
            {
                stringstream s;
                s << tag << "Set new loThreshold to: " << loThreshold << " " << watchdog;
                log->debug(s.str());
            }

            continue;
        }

        // FIXME: NBr
        // loopCondition == false;
        // continue;

        // check that there aren't points higher than xRef
        // loopCondition = false;
        /*for(int i = iRef; i < Nref; ++i)
        {
            if(xRef < profile->x[i])
            {
                cout << "Found a wrong point";
                loopCondition = true;
                break;
            }
        }*/

        // FIXME: NBr
        // if (loopCondition == false)
        // {
        // Moves upward

        for (int i = index; i < N - offset; ++i)
        // for (int i = index; i < N - offset2; ++i)
        // for (int i = index; i >= halfN; --i)
        {
            double deltaX = (double)(profile->x[i] - profile->x[i + offset]);
            double deltaY = (double)(profile->y[i] - profile->y[i + offset]);
            // cout << "i: " << i << " deltaX:" << deltaX << " deltaY:" << deltaY << endl;
            if (fabs(deltaY) >= 1.0)
            {
                dx = deltaX / deltaY;
                // cout << "dx/dy: " << dx << " dx/dy (ref): " << loThresholdRef << " @ " << i + offset2 / 2 << endl;
                if (m_debug)
                {
                    stringstream s;
                    s << tag << "dx/dy (fw): " << dx << " dx/dy (ref): " << loThresholdRef << " @ " << i + offset / 2;
                    log->debug(s.str());
                }

                if (dx >= loThresholdRef)
                {
                    int index = i + offset / 2;
                    lowerMaxX = profile->x[index];
                    lowerMaxY = profile->y[index];
                    m = index;

                    stringstream s;
                    s << tag << "dx/dy (fw): " << dx << " dx/dy (ref): " << loThresholdRef << " @ " << index << " ("
                      << lowerMaxX << "," << lowerMaxY << ")";
                    log->debug(s.str());

                    break;
                }
            }
        }
        // }
        loopCondition = false;
    }

    if (candidatePointFound == false)
    {
        log->warn(tag + "Candidate lowerMax not found.");
        return false;
    }

    // cout << "lowerMax: (" << lowerMaxX << "," << lowerMaxY << ") m: " << m << endl;

    if (m_debug)
    {
        stringstream s;
        s << tag << "lowerMax: (" << lowerMaxX << "," << lowerMaxY << ") m: " << m;
        log->debug(s.str());
    }

    // Search the minimun between the two maxima
    minimumX = upperMaxX;
    minimumY = upperMaxY;
    for (int i = n; i < m; ++i)
    {
        int x = profile->x[i];
        int y = profile->y[i];

        if (x < minimumX)
        {
            minimumX = x;
            minimumY = y;
        }
    }

    return true;
}

#endif

shared_ptr<Image<float>> OptimizationEngine::lossFieldNormalize(shared_ptr<Image<float>> srcImage)
{
    // single channel only
    float* pSrcImage = srcImage->getData();
    uint16_t width = srcImage->getWidth();
    uint16_t height = srcImage->getHeight();
    shared_ptr<Image<float>> dstImage(new Image<float>(width, height, srcImage->getNChannels()));
    float* pDstImage = dstImage->getData();

    uint16_t strideSrcImage = srcImage->getStride() / sizeof(float);
    uint16_t strideDstImage = dstImage->getStride() / sizeof(float);

    // Get min and max
    float minValue = pSrcImage[0];
    float maxValue = pSrcImage[0];

    for (uint16_t y = 0; y < height; ++y)
    {
        const int rowIndex = y * strideSrcImage;
        for (uint32_t x = 0; x < width; ++x)
        {
            float v = pSrcImage[x + rowIndex];
            if (minValue > v)
                minValue = v;
            if (maxValue < v)
                maxValue = v;
        }
    }

    float threshold = 120;

    if (minValue < maxValue)
    {
        float normFactor = 1.0f / (threshold);
        for (uint16_t y = 0; y < height; ++y)
        {
            uint32_t srcRowStartAddress = y * strideSrcImage;
            uint32_t dstRowStartAddress = y * strideDstImage;

            for (uint32_t x = 0; x < width; ++x)
            {
                float value = pSrcImage[x + srcRowStartAddress];
                if (value > threshold)
                    value = threshold;

                pDstImage[x + dstRowStartAddress] = value * normFactor;
            }
        }
    }

    return dstImage;
}

void OptimizationEngine::precomputeLossField(shared_ptr<Profile<int>> profile)
{
    // Get the max and the min of profile.x
    int minX = 0, maxX = 0;
    getMinMax(profile->x, minX, maxX);

    // Set the boundaries outside which the loss field has maximum value.
    int rightX = maxX + m_lossFieldRightOffset;
    int leftX = minX - m_lossFieldLeftOffset;

    // Check against image boundaries.
    if (rightX >= m_width)
        rightX = m_width - 1;
    if (leftX < 0)
        leftX = 0;

    // Define some default loss values.
    const float hiLoss = 548.0f; // Loss in the prohibited region
    const float hiLoss2 = hiLoss * hiLoss;
    const float lowLoss = 10.0f; // Loss in the admitted region

    // Pre-initialize the loss field with the maximum loss value
    float* ptr = m_lossField->getData();
    int height = m_lossField->getHeight();
    int stride = m_lossField->getStride() / sizeof(float);

    for (int n = height * stride - 1; n >= 0; --n)
        *ptr++ = hiLoss2;

    int N = (int)profile->x.size();
    vector<bool> i(height, 0);

    ptr = m_lossField->getData();

    for (int n = N - 1; n >= 0; --n)
    {
        int x = profile->x[n] + m_lossFieldMargin;
        int y = profile->y[n];

        i[y] = true;

        float* localPtr = &ptr[x + y * stride];
        float* localPtr2 = localPtr;

        // Right gradient
        int xRange = rightX - x;
        const float ib = lowLoss / (float)xRange;

        float value = ib;
        for (int j = 0; j < xRange; ++j)
        {
            *localPtr++ = value * value;
            value += ib;
        }

        // Left gradient
        xRange = m_leftGradientWidth;
        const float ib2 = lowLoss / (float)xRange;

        value = ib2;
        for (int j = xRange - 1; j >= 0; --j)
        {
            *localPtr2-- = value * value;
            value += ib2;
        }
    }

    for (int y = height - 1; y >= 0; --y)
    {
        if (i[y] == false)
        {
            float* localPtr = &ptr[leftX + y * stride];
            int xRange = rightX - leftX;
            for (int i = 0; i < xRange; ++i)
                *localPtr++ = 0;
        }
    }
}

struct costFunctionArgs
{
    shared_ptr<MiniBevelModel> model;
    shared_ptr<ScheimpflugTransform> st;
    double B_star;
    double M_star;
    double angle1_start;
    double angle2_start;
};

struct costFunctionMultiArgs
{
    shared_ptr<vector<shared_ptr<MiniBevelModel>>> modelSet;
    double B_star;
    double M_star;
    double angle1_start;
    double angle2_start;
    int flag; // TEST FLAG
    int kTraining;
};

struct costFCalibrationGridArgs
{
    shared_ptr<ScheimpflugTransform> st;
    vector<int> X;
    vector<int> Y;
    double H;
    double deltaN;
    float* nominalV;
    float lambda;
};

void costF(float* res, float* X, int nDims, int nParticles, void* args)
{
    ModelType modelType = ((Model*)args)->modelType();
    Model* model = nullptr;

    switch (modelType)
    {
    case MiniBevelModelType:
        model = (MiniBevelModel*)args;
        break;
    case MiniBevelModelExtType:
        model = (MiniBevelModelExt*)args;
        break;
    case TBevelModelType:
        model = (TBevelModel*)args;
        break;
    case CustomBevelModelType:
        model = (CustomBevelModel*)args;
        break;
    }

    // X is a nParticles x nDims matrix with the current positions of particles
    // res is the output value of the fitness function and is a vector with nParticles elements
    int index = 0;
    for (int s = 0; s < nParticles; ++s)
    {
        // Set the model parameter for the current particle. This call create the
        // associated bevel to X.
        model->setFreeParams(&X[index]);
        index += nDims;

        // Compute the loss value for the current bevel. The loss is substantially a
        // function of the lossField values summed up along the path of the current
        // bevel.
        // The PSO will use the computed res values in order to update the new test position
        // of each particle.
        res[s] = model->mse();
    }
}

void costFScheimpflug(float* res, float* X, int nDims, int nParticles, void* args)
{
    shared_ptr<MiniBevelModel> model = ((costFunctionArgs*)args)->model;
    shared_ptr<ScheimpflugTransform> st = ((costFunctionArgs*)args)->st;

    // double B_star =((costFunctionArgs*)args)->B_star;
    double M_star = ((costFunctionArgs*)args)->M_star;
    double angle1_start = ((costFunctionArgs*)args)->angle1_start;
    double angle2_start = ((costFunctionArgs*)args)->angle2_start;

    // X is a nParticles x nDims matrix with the current positions of particles
    // res is the output value of the fitness function and is a vector with nParticles elements

    // Reset res.
    memset(res, 0, sizeof(float) * nParticles);

    for (int s = 0; s < nParticles; ++s)
    {
        int index = s * nDims;
        // Set the new scheimpflug transformation with the one represented by
        // the current particle.
        st->init(&X[index]);

        // The model was already specified using the image points. We need to invoke
        // computeObjPoints() in order to ge the corresponding object points using the
        // new transform set above. Observe that during this optimization the image points
        // of the bevel are always the same and only the object points vary depending on the
        // transformation under test.
        model->computeObjPoints();

        // Once that we have the points in the object plane we can get the measures
        // of the bevel on that plane.
        double B, M, angle1, angle2;
        model->getTestMeasures(B, M, angle1, angle2);

        // The cost function is a quadratic loss depending on the difference of the
        // mini bevel base (M) and angles (angle1, angle2).
        res[s] = (float)((M_star - M) * (M_star - M) + (angle1 - angle1_start) * (angle1 - angle1_start) +
                         (angle2 - angle2_start) * (angle2 - angle2_start));
    }
}

void costFScheimpflugMulti_V3(float* res, float* X, int nDims, int nParticles, void* args)
{
    shared_ptr<vector<shared_ptr<MiniBevelModel>>> modelSet = ((costFunctionMultiArgs*)args)->modelSet;

    // double B_star =((costFunctionArgs*)args)->B_star;
    double M_star = ((costFunctionMultiArgs*)args)->M_star;
    double angle1_star = ((costFunctionMultiArgs*)args)->angle1_start;
    double angle2_star = ((costFunctionMultiArgs*)args)->angle2_start;
    int flag = ((costFunctionMultiArgs*)args)->flag;
    int kTraining = ((costFunctionMultiArgs*)args)->kTraining;

    // X is a nParticles x nDims matrix with the current positions of particles
    // res is the output value of the fitness function and is a vector with nParticles elements

    // Get the number of models.
    int nModels = modelSet->size();

    // Reset res.
    memset(res, 0, sizeof(float) * nParticles);

    if (flag == 0) // Scheimpflug Transform optimization.
    {
        // =============================================================
        // Here we estimate the Scheimpflug Transform parameters,
        // not the actual angles.
        // =============================================================

        // FIXME: we perform the Scheimpflug calibration using only the first 5 images.
        //
        int kModels = kTraining; // nModels;

        for (int s = 0; s < nParticles; ++s)
        {
            int index = s * nDims;

            // Get the average measures of the bevels.
            //
            double avgB = 0.0, avgM = 0.0, avgAngle1 = 0.0, avgAngle2 = 0.0;
            float maxErr = 0.0;
            for (int k = 0; k < nModels; ++k)
            {
                // Get the k-th model.
                shared_ptr<MiniBevelModel> model = (*modelSet)[k];

                // Get the nominal value of R and H. These will be used later on.
                // Nota che non prendiamo gli angoli nominali perchè questa ottimizzazione deve
                // essere reiterata. Il modello avrà gli angoli R e H modificati in base allo step
                // successivo.

                // double R = model->scheimpflugTransform()->getR();  // FIXME moved inside the init function
                // double H = model->scheimpflugTransform()->getH();  // FIXME moved inside the init function

                // Set the new Scheimpflug Transform parameter.
                // E' importante invocare l'init prima del setStylusOrientation perchè
                // l'impostazione degli angoli usa delle informazioni calcolate dalla
                // procedura di init.
                model->scheimpflugTransform()->init(&X[index]);

                // Since we have changed the Scheimpflug parameters, we need to set the stylus angles.
                // model->scheimpflugTransform()->setStylusOrientation(R, H); // FIXME moved inside the init function

                // The model was already specified using the image points. We need to invoke
                // computeObjPoints() in order to get the corresponding object points using the
                // new transform set above. Observe that during this optimization the image points
                // of the bevel are always the same and only the object points vary depending on the
                // transformation under test.
                model->computeObjPoints();

                // Once that we have the points in the object plane we can get the measures
                // of the bevel on that plane.
                double B, M, angle1, angle2;
                model->getTestMeasures(B, M, angle1, angle2);

                if (k < kModels)
                {
                    avgB += B;
                    avgM += M;
                    avgAngle1 += angle1;
                    avgAngle2 += angle2;

                    // ottimizziamo il fabs(errore)?
                    float err = fabs(M_star - M); // + fabs(avgAngle2 - angle2_star) +fabs(avgAngle1 - angle1_star);
                    // float err = (M_star - M)*(M_star - M) + (avgAngle2 - angle2_star)*(avgAngle2 - angle2_star)
                    //        + (avgAngle1 - angle1_star) *(avgAngle1 - angle1_star);
                    if (maxErr < err)
                        maxErr = err;
                }
            }
            avgB /= kModels;
            avgM /= kModels;
            avgAngle1 /= kModels;
            avgAngle2 /= kModels;

            // Let assume that the average measures are the best knowledge we have for the
            // observed bevel.
            res[s] = (float)((M_star - avgM) * (M_star - avgM) + (avgAngle1 - angle1_star) * (avgAngle1 - angle1_star) +
                             (avgAngle2 - angle2_star) * (avgAngle2 - angle2_star));
            // res[s] = maxErr;

            // res[s] = maxErr + (float)((M_star - avgM) * (M_star - avgM) + (avgAngle1 - angle1_star) * (avgAngle1 -
            // angle1_star) +
            //                 (avgAngle2 - angle2_star) * (avgAngle2 - angle2_star));
        }
    }
}

void costFCalibrationGrid(float* res, float* X, int nDims, int nParticles, void* args)
{
    shared_ptr<ScheimpflugTransform> st = ((costFCalibrationGridArgs*)args)->st;
    vector<int> imgX = ((costFCalibrationGridArgs*)args)->X;
    vector<int> imgY = ((costFCalibrationGridArgs*)args)->Y;
    double H = ((costFCalibrationGridArgs*)args)->H;
    double deltaN = ((costFCalibrationGridArgs*)args)->deltaN;
    float* nominalV = ((costFCalibrationGridArgs*)args)->nominalV;
    float lambda = ((costFCalibrationGridArgs*)args)->lambda;

    const int N = imgX.size();

    vector<double> objX;
    vector<double> objY;
    objX.reserve(N);
    objY.reserve(N);

    // The Hugh transform finds 29 points, 7 for each side of the 3x3 square
    // and the central point. The optimization try to minimize the error
    // in measuring the length of 14 lines (7 horizontal and 7 vertical).
    // and the length of the two diagonals.
    int nLines = (N - 1) / 2;

    // int offset = nLines - 1;
    // int minV = 0;
    // int maxV = minV + offset;

    // X is a nParticles x nDims matrix with the current positions of particles
    // res is the output value of the fitness function and is a vector with nParticles elements

    const float realL = 3.0;
    const float realD = realL * sqrt(2.0);

    // Reset res.
    memset(res, 0, sizeof(float) * nParticles);

    // offset used to detect the points C and D
    int offset = ((nLines >> 1) - 1) * 2;

    for (int s = 0; s < nParticles; ++s)
    {
        int index = s * nDims;
        // Set the new scheimpflug transformation with the one represented by
        // the current particle.
        st->init(&X[index]);

        // FIXME: once that the st is initialized we need to reset the angles.
        // st->setStylusOrientation(deltaN, H); // FIXME redundat

        // Reset the vectors with the points in the object plan
        objX.clear();
        objY.clear();

        // Compute object points
        for (int n = 0; n < N; ++n)
        {
            // Z coord will be always zero.
            double srcImgPoint[] = {(double)imgX[n], (double)imgY[n], 0.0};
            double dstObjPoint[] = {0.0, 0.0, 0.0};

            // We need to have points within the image boundaries.
            st->imageToObject(srcImgPoint, dstObjPoint);

            float x = (float)dstObjPoint[0];
            float y = (float)dstObjPoint[1];

            // Store the points in the image plane
            objX.push_back(x);
            objY.push_back(y);
        }

        // TEST: cost of distance from nominal solution.

        float err = 0.0f;

        if (lambda > 0)
        {
            float errN = 0.0f;
            for (int i = 0; i < 10; ++i)
            {
                errN += (nominalV[i] - X[index + i]) * (nominalV[i] - X[index + i]);
            }
            errN *= lambda;
            err = errN;
        }

        float xA = objX[0]; // objX[0 * 2];
        float yA = objY[0]; // objY[0 * 2];

        float xB = objX[1]; // objX[0 * 2 + 1];
        float yB = objY[1]; // objY[0 * 2 + 1];

        float xC = objX[offset + 1]; // objX[(nLines - 1) * 2 + 1];
        float yC = objY[offset + 1]; // objY[(nLines - 1) * 2 + 1];

        float xD = objX[offset]; // objX[(nLines - 1) * 2];
        float yD = objY[offset]; // objY[(nLines - 1) * 2];

#if 0
        // Errors along all sides of the grids (18)
        for (int i = 0; i < nLines; ++i)
        {
            float deltaX = objX[i * 2 + 1] - objX[i * 2];
            float deltaY = objY[i * 2 + 1] - objY[i * 2];
            float errL = sqrt(deltaX * deltaX + deltaY * deltaY) - realL;
            err += errL * errL;
        }
#else
        // Errors along all sides of the square (4)

        float errL1 = sqrt((xA - xB) * (xA - xB) + (yA - yB) * (yA - yB)) - realL;
        float errL2 = sqrt((xC - xB) * (xC - xB) + (yC - yB) * (yC - yB)) - realL;
        float errL3 = sqrt((xC - xD) * (xC - xD) + (yC - yD) * (yC - yD)) - realL;
        float errL4 = sqrt((xA - xD) * (xA - xD) + (yA - yD) * (yA - yD)) - realL;
        err += errL1 * errL1 + errL2 * errL2 + errL3 * errL3 + errL4 * errL4;
#endif

        // Errors along the two diagonals of the square
        float D1 = sqrt((xA - xC) * (xA - xC) + (yA - yC) * (yA - yC));
        float D2 = sqrt((xB - xD) * (xB - xD) + (yB - yD) * (yB - yD));

        err += (D1 - realD) * (D1 - realD) + (D2 - realD) * (D2 - realD);

        res[s] = err;
    }
}

shared_ptr<Model> OptimizationEngine::initModel(shared_ptr<ScheimpflugTransform> st, shared_ptr<Profile<int>> profile,
                                                ModelType modelType, HttProcessType processType,
                                                Pso<float>::Input& psoInput)
{
    // Get settings
    shared_ptr<Settings> settings = Settings::getInstance();
    Json::Value settingsJson = settings->getSettings();
    m_debug = settingsJson["optimization"]["debug"].asBool();
    if (m_debug)
        log->debug(tag + "Init model");

    // FIXME
    // 0: MiniBevel, 1: TBevel, 2: CustomBevel, 3: MiniBevelExt
    int modelIndex = 0;
    switch (modelType)
    {
    case MiniBevelModelType:
        modelIndex = 0;
        break;
    case TBevelModelType:
        modelIndex = 1;
        break;
    case CustomBevelModelType:
        modelIndex = 2;
        break;
    case MiniBevelModelExtType:
        modelIndex = 3;
        break;
    default:
        M_Assert(false, "Wrong Model selected");
        log->error(__FILE__, __LINE__, "Wrong Model selected");
        return nullptr;
    }

    int winSizeX = settingsJson["search_space"][modelIndex]["winSizeX"].asInt();
    int winSizeY = settingsJson["search_space"][modelIndex]["winSizeY"].asInt();

    int intersectionThreshold = settingsJson["optimization"]["intersectionThreshold"].asInt();
    bool checkIntersections = false;
    switch (processType)
    {
    case HttProcessType::lensFitting:
        m_lossFieldMargin = settingsJson["optimization"]["lossFieldMargin"].asInt();
        checkIntersections = settingsJson["optimization"]["checkIntersections"].asBool();
        psoInput.nIterations = settingsJson["pso"][modelIndex]["nIterations"].asInt();
        psoInput.nParticles = settingsJson["pso"][modelIndex]["nParticles"].asInt();
        break;
    case HttProcessType::calibration:
        m_lossFieldMargin = settingsJson["optimization"]["lossFieldMarginCalibration"].asInt();
        checkIntersections = settingsJson["optimization"]["checkIntersectionsCalibration"].asBool();
        psoInput.nIterations = settingsJson["scheimpflug_transformation_calibration"]["pso"]["nIterations"].asInt();
        psoInput.nParticles = settingsJson["scheimpflug_transformation_calibration"]["pso"]["nParticles"].asInt();
        break;
    case HttProcessType::validation:
        m_lossFieldMargin = settingsJson["optimization"]["lossFieldMarginValidation"].asInt();
        checkIntersections = settingsJson["optimization"]["checkIntersectionsValidation"].asBool();
        psoInput.nIterations = settingsJson["pso"][modelIndex]["nIterations"].asInt();
        psoInput.nParticles = settingsJson["pso"][modelIndex]["nParticles"].asInt();
        break;
    default:
        log->error(__FILE__, __LINE__, "Invalid processType (OptimizationEngine::optimizeModel)");
        assert(false);
        break;
    }

    m_lossFieldRightOffset = settingsJson["optimization"]["lossFieldRightOffset"].asInt();
    m_lossFieldLeftOffset = settingsJson["optimization"]["lossFieldLeftOffset"].asInt();
    m_leftGradientWidth = settingsJson["optimization"]["leftGradientWidth"].asInt();

    // Get size of input image
    m_width = settingsJson["image"]["width"].asInt();
    m_height = settingsJson["image"]["height"].asInt();

    // Allocate the loss field object.
    m_lossField = make_shared<Image<float>>(Image<float>(m_width, m_height, 1));

    // Check if profile has at least one element
    M_Assert(profile->y.size() != 0, "Empty profile");

    // Precompute the loss field given the profile.
    precomputeLossField(profile);

    if (m_debug)
    {
        // Normalize the image.
        shared_ptr<Image<float>> normLossField = lossFieldNormalize(m_lossField);

        if (saveImage(".", "LossField.png", normLossField) == false)
            log->error(__FILE__, __LINE__, "Error on saveImage");
    }

    // Get the search space
    int upperMaxX = 0;
    int upperMaxY = 0;
    int lowerMaxX = 0;
    int lowerMaxY = 0;
    // FIXME only for TModel
    int minimumX = 0;
    int minimumY = 0;
    bool ssRes = getSearchSpace(profile, upperMaxX, upperMaxY, lowerMaxX, lowerMaxY, minimumX, minimumY, modelType);

    // Handle the case when the point of maximum are not found
    if (ssRes == false)
        return nullptr;

    // Save the two maxima over the profile
    profile->ancorImgX.push_back(upperMaxX);
    profile->ancorImgX.push_back(lowerMaxX);
    profile->ancorImgY.push_back(upperMaxY);
    profile->ancorImgY.push_back(lowerMaxY);

    float minMMultiplier = settingsJson["search_space"][modelIndex]["minMMultiplier"].asFloat();
    float maxMMultiplier = settingsJson["search_space"][modelIndex]["maxMMultiplier"].asFloat();
    float minAlpha = settingsJson["search_space"][modelIndex]["minAlpha"].asFloat();
    float maxAlpha = settingsJson["search_space"][modelIndex]["maxAlpha"].asFloat();
    // FIXME TModel
    float minBMultiplier = settingsJson["search_space"][modelIndex]["minBMultiplier"].asFloat();
    float maxBMultiplier = settingsJson["search_space"][modelIndex]["maxBMultiplier"].asFloat();
    // FIXME CustonModel
    float minS = settingsJson["search_space"][modelIndex]["minS"].asFloat();
    float maxS = settingsJson["search_space"][modelIndex]["maxS"].asFloat();
    float minE = settingsJson["search_space"][modelIndex]["minE"].asFloat();
    float maxE = settingsJson["search_space"][modelIndex]["maxE"].asFloat();
    // FIXME MiniBevelModelExt
    float deltaBeta = settingsJson["search_space"][modelIndex]["deltaBeta"].asFloat();

    vector<double> weight;
    weight.clear();
    Json::Value weightArray = settingsJson["search_space"][modelIndex]["weight"];
    for (int i = 0; i < (int)weightArray.size(); ++i)
        weight.push_back(weightArray[i].asDouble());

    double M =
        sqrt((lowerMaxY - upperMaxY) * (lowerMaxY - upperMaxY) + (lowerMaxX - upperMaxX) * (lowerMaxX - upperMaxX));
    double alpha = atan2(lowerMaxX - upperMaxX, lowerMaxY - upperMaxY);

    double minM = M * minMMultiplier;
    double maxM = M * maxMMultiplier;
    minAlpha += (float)alpha;
    maxAlpha += (float)alpha;

    // FIXME TModel
    int dMax = 0;
    for (int i = upperMaxY; i < lowerMaxY; ++i)
    {
        // x = m*y + q
        double m = tan(alpha);
        double q = upperMaxX - m * upperMaxY;
        int x = (int)(m * i + q);

        auto it = find(profile->y.begin(), profile->y.end(), i);
        if (it != profile->y.end())
        {
            int index = (int)distance(profile->y.begin(), it);
            int d = x - profile->x[index];
            // cout << "(" << x << "," << i << ") -> " << d << endl;
            if (d > dMax)
                dMax = d;
        }
    }
    double B = dMax;
    double minB = B * minBMultiplier;
    double maxB = B * maxBMultiplier;

    if (m_debug)
    {
        stringstream s;
        s << tag << "M (min, max): " << M << " (" << minM << ", " << maxM << ")";
        log->debug(s.str());
        s.str("");
        s << tag << "alphaRad (min, max): " << alpha << " (" << minAlpha << ", " << maxAlpha << ")";
        log->debug(s.str());
        s.str("");
        s << tag << "alphaDeg (min, max): " << alpha / 3.14 * 180 << " (" << minAlpha / 3.14 * 180 << ", "
          << maxAlpha / 3.14 * 180 << ")";
        log->debug(s.str());
        s.str("");
        s << tag << "B (min, max): " << B << " (" << minB << ", " << maxB << ")";
        log->debug(s.str());
    }

    // Istantiate the model given its type.
    shared_ptr<Model> model = nullptr;
    vector<float> lb; // Optimization lower bound of variables
    vector<float> ub; // Optimization upper bound of variables

    // Pso<float>::Input input;
    psoInput.w0i = settingsJson["pso"][modelIndex]["w0i"].asFloat();
    psoInput.w0f = settingsJson["pso"][modelIndex]["w0f"].asFloat();
    psoInput.cp = settingsJson["pso"][modelIndex]["cp"].asFloat();
    psoInput.cg = settingsJson["pso"][modelIndex]["cg"].asFloat();
    // FIXME
    // psoInput.nIterations = settingsJson["pso"][modelIndex]["nIterations"].asInt();
    // psoInput.nParticles = settingsJson["pso"][modelIndex]["nParticles"].asInt();
    psoInput.debug = settingsJson["pso"][modelIndex]["debug"].asBool();

    if (m_debug)
    {
        char str[100];

        log->debug(tag + "Search space parameters:");

        sprintf_s(str, "   (upperMaxX, upperMaxY):  (%d, %d) pixel", upperMaxX, upperMaxY);
        log->debug(tag + str);
        sprintf_s(str, "   (lowerMaxX, lowerMaxY):  (%d, %d) pixel", lowerMaxX, lowerMaxY);
        log->debug(tag + str);
        sprintf_s(str, "   (minimumX, minimumY):  (%d, %d) pixel", minimumX, minimumY);
        log->debug(tag + str);
        sprintf_s(str, "   (minM, maxM):            (%.0f, %.0f) pixel", minM, maxM);
        log->debug(tag + str);
        sprintf_s(str, "   (minAlpha, maxAlpha):    (%.2f, %.2f) rad", minAlpha, maxAlpha);
        log->debug(tag + str);
        sprintf_s(str, "   (winSizeX, winSizeY):    (%d, %d) pixel", winSizeX, winSizeY);
        log->debug(tag + str);

        //----------------------------------------------------------------------
        log->debug(tag + "PSO parameters:");

        sprintf_s(str, "   nIterations: %d", psoInput.nIterations);
        log->debug(tag + str);
        sprintf_s(str, "   nParticles:  %d", psoInput.nParticles);
        log->debug(tag + str);

        sprintf_s(str, "   w0i:         %.4f", psoInput.w0i);
        log->debug(tag + str);
        sprintf_s(str, "   w0f:         %.4f", psoInput.w0f);
        log->debug(tag + str);
        sprintf_s(str, "   cp:          %.4f", psoInput.cp);
        log->debug(tag + str);
        sprintf_s(str, "   cg:          %.4f", psoInput.cg);
        log->debug(tag + str);
    }

    switch (modelType)
    {
    case MiniBevelModelType:
    {
        // Create the model initialized with the given Scheimpflug Transformation
        Json::Value stcJson = settings->getCalibration()["scheimpflug_transformation_calibration"];
        Json::Value frame = stcJson["frames"][stcJson["current"].asString()];

        Json::Value settingsJson = settings->getSettings();
        float angle = settingsJson["pso"][0]["angle"].asDouble();
        double bevelAngle = (processType == HttProcessType::validation) ? frame["angle1"].asDouble() : angle;

        model = make_shared<MiniBevelModel>(st, bevelAngle);
        model->setDebug(m_debug);

#if 0 // FIXME
        double srcImgPoint[3] = {(double)upperMaxX, (double)upperMaxY, 0.0};
        double dstObjPoint[3] = {0.0, 0.0, 0.0};
        model->imageToObject(srcImgPoint, dstObjPoint);

        float minX1mmT = (float)(dstObjPoint[0] - model->pixelToMm(winSizeX));
        float maxX1mmT = (float)(dstObjPoint[0] + model->pixelToMm(winSizeX));

        float minY1mmT = (float)(dstObjPoint[1] - model->pixelToMm(winSizeY));
        float maxY1mmT = (float)(dstObjPoint[1] + model->pixelToMm(winSizeY));

        if (m_debug)
        {
            stringstream s;
            s << tag << "minX T " << minX1mmT << " " << maxX1mmT;
            log->debug(s.str());
            s.str("");
            s << tag << "minY T " << minY1mmT << " " << maxY1mmT;
            log->debug(s.str());
        }
#endif

        double srcImgPoint_A[3] = {(double)(upperMaxX + winSizeX), (double)(upperMaxY + winSizeY), 0.0};
        double dstObjPoint_A[3] = {0.0, 0.0, 0.0};
        model->imageToObject(srcImgPoint_A, dstObjPoint_A);
        double srcImgPoint_B[3] = {(double)(upperMaxX - winSizeX), (double)(upperMaxY - winSizeY), 0.0};
        double dstObjPoint_B[3] = {0.0, 0.0, 0.0};
        model->imageToObject(srcImgPoint_B, dstObjPoint_B);

        float minX1mm = (float)(dstObjPoint_A[0]);
        float maxX1mm = (float)(dstObjPoint_B[0]);

        float minY1mm = (float)(dstObjPoint_A[1]);
        float maxY1mm = (float)(dstObjPoint_B[1]);

        if (m_debug)
        {
            float Mmm = (float)model->pixelToMm(M);
            stringstream s;
            s << tag << "Initial M estimate [mm] " << Mmm;
            log->debug(s.str());
        }

        float minMmm = (float)model->pixelToMm(minM);
        float maxMmm = (float)model->pixelToMm(maxM);

        // Define the search space
        psoInput.lb.resize(4);
        psoInput.ub.resize(4);

        psoInput.lb[0] = minX1mm;
        psoInput.ub[0] = maxX1mm;
        psoInput.lb[1] = minY1mm;
        psoInput.ub[1] = maxY1mm;
        psoInput.lb[2] = minAlpha;
        psoInput.ub[2] = maxAlpha;
        psoInput.lb[3] = minMmm;
        psoInput.ub[3] = maxMmm;

        // Set the cost function
        psoInput.costF = costF;
    }
    break;

    case MiniBevelModelExtType:
    {
        // Create the model initialized with the given Scheimpflug Transformation
        Json::Value stcJson = settings->getCalibration()["scheimpflug_transformation_calibration"];
        Json::Value frame = stcJson["frames"][stcJson["current"].asString()];

        Json::Value settingsJson = settings->getSettings();
        float angle = settingsJson["pso"][3]["angle"].asDouble();
        double bevelAngle = (processType == HttProcessType::validation) ? frame["angle1"].asDouble() : angle;

        model = make_shared<MiniBevelModelExt>(st, bevelAngle);
        model->setDebug(m_debug);

        double srcImgPoint_A[3] = {(double)(upperMaxX + winSizeX), (double)(upperMaxY + winSizeY), 0.0};
        double dstObjPoint_A[3] = {0.0, 0.0, 0.0};
        model->imageToObject(srcImgPoint_A, dstObjPoint_A);
        double srcImgPoint_B[3] = {(double)(upperMaxX - winSizeX), (double)(upperMaxY - winSizeY), 0.0};
        double dstObjPoint_B[3] = {0.0, 0.0, 0.0};
        model->imageToObject(srcImgPoint_B, dstObjPoint_B);

        float minX1mm = (float)(dstObjPoint_A[0]);
        float maxX1mm = (float)(dstObjPoint_B[0]);

        float minY1mm = (float)(dstObjPoint_A[1]);
        float maxY1mm = (float)(dstObjPoint_B[1]);

        if (m_debug)
        {
            float Mmm = (float)model->pixelToMm(M);
            stringstream s;
            s << tag << "Initial M estimate [mm] " << Mmm;
            log->debug(s.str());
        }

        float minMmm = (float)model->pixelToMm(minM);
        float maxMmm = (float)model->pixelToMm(maxM);

        // Define the search space
        psoInput.lb.resize(5);
        psoInput.ub.resize(5);

        psoInput.lb[0] = minX1mm;
        psoInput.ub[0] = maxX1mm;
        psoInput.lb[1] = minY1mm;
        psoInput.ub[1] = maxY1mm;
        psoInput.lb[2] = minAlpha;
        psoInput.ub[2] = maxAlpha;
        psoInput.lb[3] = minMmm;
        psoInput.ub[3] = maxMmm;

        psoInput.lb[4] = bevelAngle - deltaBeta;
        psoInput.ub[4] = bevelAngle + deltaBeta;

        // Set the cost function
        psoInput.costF = costF;
    }
    break;

    case TBevelModelType:
        // FIXME TModel
        {
            model = make_shared<TBevelModel>(st);
            model->setDebug(m_debug);

            double srcImgPoint_A[3] = {(double)(upperMaxX + winSizeX), (double)(upperMaxY + winSizeY), 0.0};
            double dstObjPoint_A[3] = {0.0, 0.0, 0.0};
            model->imageToObject(srcImgPoint_A, dstObjPoint_A);
            double srcImgPoint_B[3] = {(double)(upperMaxX - winSizeX), (double)(upperMaxY - winSizeY), 0.0};
            double dstObjPoint_B[3] = {0.0, 0.0, 0.0};
            model->imageToObject(srcImgPoint_B, dstObjPoint_B);

            float minX1mm = (float)(dstObjPoint_A[0]);
            float maxX1mm = (float)(dstObjPoint_B[0]);

            float minY1mm = (float)(dstObjPoint_A[1]);
            float maxY1mm = (float)(dstObjPoint_B[1]);

            if (m_debug)
            {
                float Mmm = (float)model->pixelToMm(M);
                float Bmm = (float)model->pixelToMm(B);
                stringstream s;
                s << tag << "Initial M estimate [mm] " << Mmm << " B estimate [mm] " << Bmm;
                log->debug(s.str());
            }

            float minMmm = (float)model->pixelToMm(minM);
            float maxMmm = (float)model->pixelToMm(maxM);

            float minBmm = (float)model->pixelToMm(minB);
            float maxBmm = (float)model->pixelToMm(maxB);

            // Define the search space //FIXME hard-coded value for the nuber of the FreeParams
            psoInput.lb.resize(5);
            psoInput.ub.resize(5);
            psoInput.lb[0] = minX1mm;
            psoInput.ub[0] = maxX1mm;
            psoInput.lb[1] = minY1mm;
            psoInput.ub[1] = maxY1mm;
            psoInput.lb[2] = minAlpha;
            psoInput.ub[2] = maxAlpha;
            psoInput.lb[3] = minMmm;
            psoInput.ub[3] = maxMmm;
            psoInput.lb[4] = minBmm;
            psoInput.ub[4] = maxBmm;

            // Set the cost function
            psoInput.costF = costF;
        }
        break;

    case CustomBevelModelType:
        // FIXME CustomModel
        {
            model = make_shared<CustomBevelModel>(st);
            model->setDebug(m_debug);

            double srcImgPoint_A[3] = {(double)(upperMaxX + winSizeX), (double)(upperMaxY + winSizeY), 0.0};
            double dstObjPoint_A[3] = {0.0, 0.0, 0.0};
            model->imageToObject(srcImgPoint_A, dstObjPoint_A);
            double srcImgPoint_B[3] = {(double)(upperMaxX - winSizeX), (double)(upperMaxY - winSizeY), 0.0};
            double dstObjPoint_B[3] = {0.0, 0.0, 0.0};
            model->imageToObject(srcImgPoint_B, dstObjPoint_B);

            float minX1mm = (float)(dstObjPoint_A[0]);
            float maxX1mm = (float)(dstObjPoint_B[0]);
            float minY1mm = (float)(dstObjPoint_A[1]);
            float maxY1mm = (float)(dstObjPoint_B[1]);

            float minMmm = (float)model->pixelToMm(minM);
            float maxMmm = (float)model->pixelToMm(maxM);

            float minSmm = (float)model->pixelToMm(minS);
            float maxSmm = (float)model->pixelToMm(maxS);

            float minEmm = (float)model->pixelToMm(minE);
            float maxEmm = (float)model->pixelToMm(maxE);

            double srcImgPoint_C[3] = {(double)(lowerMaxX + winSizeX), (double)(lowerMaxY + winSizeY), 0.0};
            double dstObjPoint_C[3] = {0.0, 0.0, 0.0};
            model->imageToObject(srcImgPoint_C, dstObjPoint_C);
            double srcImgPoint_D[3] = {(double)(lowerMaxX - winSizeX), (double)(lowerMaxY - winSizeY), 0.0};
            double dstObjPoint_D[3] = {0.0, 0.0, 0.0};
            model->imageToObject(srcImgPoint_D, dstObjPoint_D);

            float minX5mm = (float)(dstObjPoint_C[0]);
            float maxX5mm = (float)(dstObjPoint_D[0]);
            float minY5mm = (float)(dstObjPoint_C[1]);
            float maxY5mm = (float)(dstObjPoint_D[1]);

            // Define the search space
            psoInput.lb.resize(8);
            psoInput.ub.resize(8);

            // Order: x1,y1,x5,y5,alpha,M,S,E
            psoInput.lb[0] = minX1mm;
            psoInput.ub[0] = maxX1mm;
            psoInput.lb[1] = minY1mm;
            psoInput.ub[1] = maxY1mm;
            psoInput.lb[2] = minX5mm;
            psoInput.ub[2] = maxX5mm;
            psoInput.lb[3] = minY5mm;
            psoInput.ub[3] = maxY5mm;
            psoInput.lb[4] = minAlpha;
            psoInput.ub[4] = maxAlpha;
            psoInput.lb[5] = minMmm;
            psoInput.ub[5] = maxMmm;
            psoInput.lb[6] = minSmm;
            psoInput.ub[6] = maxSmm;
            psoInput.lb[7] = minEmm;
            psoInput.ub[7] = maxEmm;

            // Set the cost function
            psoInput.costF = costF;
        }
        break;

    default:
        M_Assert(false, "Wrong Model selected");
        log->error(__FILE__, __LINE__, "Wrong Model selected");
        break;
    }

    // Set the loss field and the profile in the model.
    model->setLossFieldAndProfile(m_lossField, profile, checkIntersections, intersectionThreshold, weight);

    return model;
}

shared_ptr<OptimizationEngine::Output> OptimizationEngine::optimizeModel(shared_ptr<Model> model,
                                                                         Pso<float>::Input& input,
                                                                         shared_ptr<Profile<int>> profile,
                                                                         const int maxRetries)
{
    if (m_debug)
        log->debug(tag + "Optimize model");

    // Optimization
    bool validSolution = false;

    Pso<float> pso;
    pso.setArgs(model.get());
    Pso<float>::Output output;

    // Initialize OptimizationEngine::Output values
    shared_ptr<Profile<int>> bevel = nullptr;
    shared_ptr<Model::Measures> measures = nullptr;
    int iteration = input.nIterations;
    float loss = 1e10;

    int counter = 0;
    Pso<float>::ReturnCode res;
    while (!validSolution && counter < maxRetries)
    {
        // Call the PSO
        res = pso.optimize(input, output);

        if (res == Pso<float>::ReturnCode::SUCCESS || res == Pso<float>::ReturnCode::BOUNDARY_REACHED)
        {
            const int size = (int)output.optimumX.size();
            float* optModel = new float[size];
            for (int j = 0; j < size; ++j)
                optModel[j] = (float)output.optimumX[j];

            // Anchor point
            double srcImgPoint3[3] = {(double)output.optimumX[0], (double)output.optimumX[1], 0.0};
            double dstObjPoint3[3] = {0.0, 0.0, 0.0};
            model->objectToImage(srcImgPoint3, dstObjPoint3);

            if (m_debug)
            {
                stringstream s;
                s << tag << "upperMaxOpt [pixel] " << dstObjPoint3[0] << " " << dstObjPoint3[1];
                log->debug(s.str());
                s.str("");
                s << tag << "OPTIMUM: ";

                for (int j = 0; j < size; ++j)
                    s << output.optimumX[j] << " ";

                log->debug(s.str());
            }

            // Set the optimal model parameters found by the PSO and get the corresponding bevel.
            int pos = (int)profile->x.size();
            int lastImgPointProfile[] = {profile->x[pos - 1], profile->y[pos - 1]};

            validSolution = model->finalizeModel(optModel, lastImgPointProfile, profile);

            if (optModel)
            {
                delete[] optModel;
                optModel = nullptr;
            }

            if (m_debug)
            {
                model->printImgPoints();
                model->printObjPoints();
            }

            model->printMeasures();

            if (validSolution)
            {
                bevel = model->getBevel();
                measures = model->getMeasures();
                loss = output.loss;
                iteration = output.optimumIteration;
                break;
            }
        } else
        {
            // log->error(__FILE__, __LINE__, "PSO failed");
            stringstream s;
            s << tag << "PSO Return code: " << res;
            log->warn(s.str());
        }

        // Increment the counter of retries.
        ++counter;
    }

    shared_ptr<OptimizationEngine::Output> out(new OptimizationEngine::Output());
    out->bevel = bevel;
    out->measures = measures;
    out->loss = loss;
    out->iteration = iteration;
    out->modelType = model->modelType();
    out->R = model->getR();
    out->H = model->getH();

    if (counter >= maxRetries)
    {
        if (res == Pso<float>::ReturnCode::SUCCESS || res == Pso<float>::ReturnCode::BOUNDARY_REACHED)
            if (validSolution)
                out->rcode = ReturnCode::LENSFITTING_FAILED_PSO_BOUNDARY_REACHED;
            else
                out->rcode = ReturnCode::LENSFITTING_FAILED_BEVELFRAME_INTERSECTION;
        else
            out->rcode = ReturnCode::LENSFITTING_FAILED_MAX_RETRIES_REACHED;
    } else
    {
        out->rcode = ReturnCode::LENSFITTING_SUCCESSFUL;
    }

    return out;
}

shared_ptr<OptimizationEngine::ScheimpflugTransformOutput>
OptimizationEngine::optimizeScheimpflugTransform(shared_ptr<ScheimpflugTransform> st, shared_ptr<Profile<int>> profile,
                                                 const vector<float>& imgX, const vector<float>& imgY)
{
    // Get settings
    shared_ptr<Settings> settings = Settings::getInstance();
    Json::Value settingsJson = settings->getSettings();
    Json::Value calibrationJson = settings->getCalibration();

    int maxRetries = 1; // settingsJson["scheimpflug_transformation_calibration"]["maxRetries"].asInt();
    bool checkIntersections = settingsJson["optimization"]["checkIntersectionsCalibration"].asBool();
    int intersectionThreshold = settingsJson["optimization"]["intersectionThreshold"].asInt();
    m_lossFieldMargin = settingsJson["optimization"]["lossFieldMargin"].asInt();
    m_lossFieldRightOffset = settingsJson["optimization"]["lossFieldRightOffset"].asInt();
    m_lossFieldLeftOffset = settingsJson["optimization"]["lossFieldLeftOffset"].asInt();

    // Get size of input image
    m_width = settingsJson["image"]["width"].asInt();
    m_height = settingsJson["image"]["height"].asInt();

    // Allocate the loss field object.
    m_lossField = make_shared<Image<float>>(Image<float>(m_width, m_height, 1));

    // Check if profile has at least one element
    M_Assert(profile->y.size() != 0, "Empty profile");

    // Precompute the loss field given the profile.
    precomputeLossField(profile);

    if (m_debug)
    {
        // Normalize the image.
        shared_ptr<Image<float>> normLossField = lossFieldNormalize(m_lossField);

        if (saveImage(".", "LossField.png", normLossField) == false)
            log->error(__FILE__, __LINE__, "Error on saveImage");
    }

    Pso<float>::Input input;
    input.w0i = settingsJson["scheimpflug_transformation_calibration"]["pso"]["w0i"].asFloat();
    input.w0f = settingsJson["scheimpflug_transformation_calibration"]["pso"]["w0f"].asFloat();
    input.cp = settingsJson["scheimpflug_transformation_calibration"]["pso"]["cp"].asFloat();
    input.cg = settingsJson["scheimpflug_transformation_calibration"]["pso"]["cg"].asFloat();
    input.nIterations = settingsJson["scheimpflug_transformation_calibration"]["pso"]["nIterations"].asInt();
    input.nParticles = settingsJson["scheimpflug_transformation_calibration"]["pso"]["nParticles"].asInt();
    input.debug = settingsJson["scheimpflug_transformation_calibration"]["pso"]["debug"].asBool();

    // Create the model initialized with the given Scheimpflug Transformation
    shared_ptr<MiniBevelModel> model(new MiniBevelModel(st));

    vector<double> weight;
    weight.clear();
    Json::Value weightArray = settingsJson["search_space"][0]["weight"];
    for (int i = 0; i < (int)weightArray.size(); ++i)
        weight.push_back(weightArray[i].asDouble());

    // Set the loss field and the profile in the model.
    model->setLossFieldAndProfile(m_lossField, profile, checkIntersections, intersectionThreshold, weight);

    // Usually the model is defined in the object plane but here we need to have it
    // defined  directly in the image plane
    model->setFromImgPoints(imgX, imgY);
    model->printImgPoints();

    // Define the search space
    vector<float> lb; // Optimization lower bound of variables
    vector<float> ub; // Optimization upper bound of variables
    input.lb.resize(10);
    input.ub.resize(10);

    Json::Value sps = calibrationJson["search_space_scheimpflug"];
    // {"alpha", "beta", "cx0", "cy0", "delta", "p1", "p2", "phi", "theta", "tt"}
    input.lb[0] = sps["alpha"]["min"].asFloat();
    input.ub[0] = sps["alpha"]["max"].asFloat();

    input.lb[1] = sps["beta"]["min"].asFloat();
    input.ub[1] = sps["beta"]["max"].asFloat();

    input.lb[2] = sps["cx0"]["min"].asFloat();
    input.ub[2] = sps["cx0"]["max"].asFloat();

    input.lb[3] = sps["cy0"]["min"].asFloat();
    input.ub[3] = sps["cy0"]["max"].asFloat();

    input.lb[4] = sps["delta"]["min"].asFloat();
    input.ub[4] = sps["delta"]["max"].asFloat();

    input.lb[5] = sps["p1"]["min"].asFloat();
    input.ub[5] = sps["p1"]["max"].asFloat();

    input.lb[6] = sps["p2"]["min"].asFloat();
    input.ub[6] = sps["p2"]["max"].asFloat();

    input.lb[7] = sps["phi"]["min"].asFloat();
    input.ub[7] = sps["phi"]["max"].asFloat();

    input.lb[8] = sps["theta"]["min"].asFloat();
    input.ub[8] = sps["theta"]["max"].asFloat();

    input.lb[9] = sps["tt"]["min"].asFloat();
    input.ub[9] = sps["tt"]["max"].asFloat();

    // Set the cost function
    input.costF = costFScheimpflug;

    // Optimization
    bool validSolution = false;

    // Get frame calibration params fron json object
    Json::Value calibration = calibrationJson["scheimpflug_transformation_calibration"];
    string currentCalibrationFrame = calibration["current"].asString();
    Json::Value frameParams = calibration["frames"][currentCalibrationFrame];

    Pso<float> pso;
    costFunctionArgs args = {model,
                             st,
                             frameParams["B"].asDouble(),
                             frameParams["M"].asDouble(),
                             st->degToRad(frameParams["angle1"].asDouble()),
                             st->degToRad(frameParams["angle2"].asDouble())};
    pso.setArgs((void*)&args);
    Pso<float>::Output output;

    // Initialize OptimizationEngine::ScheimpflugTransformOutput values
    shared_ptr<ScheimpflugTransform::Params> stParams = nullptr;
    std::shared_ptr<Profile<int>> bevel = nullptr;
    int iteration = input.nIterations;
    float loss = 1e10;

    int counter = 0;
    Pso<float>::ReturnCode res;
    while (!validSolution && counter < maxRetries)
    {
        // Call the PSO
        res = pso.optimize(input, output);

        if (res == Pso<float>::ReturnCode::SUCCESS || res == Pso<float>::ReturnCode::BOUNDARY_REACHED)
        {
            // FIXME: set cx0 and cy0 as input params of the pso
            float optModel[] = {(float)output.optimumX[0], (float)output.optimumX[1], (float)output.optimumX[2],
                                (float)output.optimumX[3], (float)output.optimumX[4], (float)output.optimumX[5],
                                (float)output.optimumX[6], (float)output.optimumX[7], (float)output.optimumX[8],
                                (float)output.optimumX[9]};

            st->init(optModel);
#if 1
            model->computeObjPoints();
#else
            model->recomputeImgPoints();
            validSolution = model->computeConstrainedBevelProfile();
            model->printImgPoints();
#endif

            double B, M, angle1, angle2;
            model->getTestMeasures(B, M, angle1, angle2);
            double error = sqrt(output.loss) / sqrt((frameParams["B"].asDouble() * frameParams["B"].asDouble()) +
                                                    (frameParams["M"].asDouble() * frameParams["M"].asDouble()));
            {
                stringstream s;
                s << tag << "B: " << B << " [mm] M: " << M << " [mm] a1: " << st->radToDeg(angle1)
                  << " [deg] a2: " << st->radToDeg(angle2) << " [deg]"
                  << " error: " << error;
                log->info(s.str());
            }
            double epsilon = settingsJson["scheimpflug_transformation_calibration"]["epsilon"].asDouble();
            validSolution = error > epsilon ? false : true;

            if (validSolution)
            {
                // log->info(tag + "Valid solution:");
                stParams = make_shared<ScheimpflugTransform::Params>(ScheimpflugTransform::Params());
                stParams->alpha = optModel[0];
                stParams->beta = optModel[1];
                stParams->cx0 = optModel[2];
                stParams->cy0 = optModel[3];
                stParams->delta = optModel[4];
                stParams->p1 = optModel[5];
                stParams->p2 = optModel[6];
                stParams->phi = optModel[7];
                stParams->theta = optModel[8];
                stParams->tt = optModel[9];
                stParams->magnification =
                    calibrationJson["nominal_scheimpflug_transformation"]["magnification"].asDouble();

                log->info(tag + "Valid solution:");
                {
                    stringstream s;
                    s << tag << left << setw(13) << "alpha" << setw(13) << "beta" << setw(13) << "cx0" << setw(13)
                      << "cy0" << setw(13) << "delta" << setw(13) << "p1" << setw(13) << "p2" << setw(13) << "phi"
                      << setw(13) << "theta" << setw(13) << "tt" << setw(13) << "loss";
                    log->info(s.str());
                    s.str("");
                    s << tag << left << setw(13) << optModel[0] << setw(13) << optModel[1] << setw(13) << optModel[2]
                      << setw(13) << optModel[3] << setw(13) << optModel[4] << setw(13) << optModel[5] << setw(13)
                      << optModel[6] << setw(13) << optModel[7] << setw(13) << optModel[8] << setw(13) << optModel[9]
                      << setw(13) << output.loss;
                    log->info(s.str());
                }

                // TODO: compare with nominal values

                // Update scheimpflug_transformation json object with the optimum values
                Json::Value st = calibrationJson["scheimpflug_transformation"];
                st["alpha"] = optModel[0];
                st["beta"] = optModel[1];
                st["cx0"] = optModel[2];
                st["cy0"] = optModel[3];
                st["delta"] = optModel[4];
                st["p1"] = optModel[5];
                st["p2"] = optModel[6];
                st["phi"] = optModel[7];
                st["theta"] = optModel[8];
                st["tt"] = optModel[9];

                Json::Value nst = calibrationJson["nominal_scheimpflug_transformation"];

                st["px"] = nst["px"];
                st["magnification"] = nst["magnification"];

                calibrationJson["scheimpflug_transformation"] = st;
                settings->updateCalibration(calibrationJson);

                loss = output.loss;
                iteration = output.optimumIteration;
                bevel = model->getBevel();
                break;
            }
        } else
        {
            // log->error(__FILE__, __LINE__, "PSO failed");
            stringstream s;
            s << tag << "PSO Return code: " << res;
            log->warn(s.str());
        }

        // Increment the counter of retries.
        ++counter;
    }

    shared_ptr<OptimizationEngine::ScheimpflugTransformOutput> out(
        new OptimizationEngine::ScheimpflugTransformOutput());

    out->params = stParams;
    out->bevel = bevel;
    out->iteration = iteration;
    out->loss = loss;
    out->validCalibration = validSolution;
    out->modelType = model->modelType();
    out->R = model->getR();
    out->H = model->getH();
    out->measures = model->getMeasures();

    if (counter >= maxRetries)
    {
        if (res == Pso<float>::ReturnCode::SUCCESS || res == Pso<float>::ReturnCode::BOUNDARY_REACHED)
            if (validSolution)
                out->rcode = ReturnCode::LENSFITTING_FAILED_PSO_BOUNDARY_REACHED;
            else
                out->rcode = ReturnCode::LENSFITTING_FAILED_BEVELFRAME_INTERSECTION;
        else
            out->rcode = ReturnCode::LENSFITTING_FAILED_MAX_RETRIES_REACHED;
    } else
    {
        out->rcode = ReturnCode::LENSFITTING_SUCCESSFUL;
    }

    return out;
}

shared_ptr<OptimizationEngine::ScheimpflugTransformOutput> OptimizationEngine::optimizeScheimpflugTransform(
    vector<shared_ptr<ScheimpflugTransform>>& stSet, vector<shared_ptr<Profile<int>>>& profileSet,
    const vector<shared_ptr<vector<float>>>& imgXSet, const vector<shared_ptr<vector<float>>>& imgYSet)
{
    // Get settings
    shared_ptr<Settings> settings = Settings::getInstance();
    Json::Value settingsJson = settings->getSettings();
    Json::Value calibrationJson = settings->getCalibration();

    int maxRetries = 1; // settingsJson["scheimpflug_transformation_calibration"]["maxRetries"].asInt();
    bool checkIntersections = settingsJson["optimization"]["checkIntersectionsCalibration"].asBool();
    int intersectionThreshold = settingsJson["optimization"]["intersectionThreshold"].asInt();
    m_lossFieldMargin = settingsJson["optimization"]["lossFieldMargin"].asInt();
    m_lossFieldRightOffset = settingsJson["optimization"]["lossFieldRightOffset"].asInt();
    m_lossFieldLeftOffset = settingsJson["optimization"]["lossFieldLeftOffset"].asInt();

    // Get size of input image
    m_width = settingsJson["image"]["width"].asInt();
    m_height = settingsJson["image"]["height"].asInt();

    // FIXME
    // Allocate the loss field object.
    m_lossField = make_shared<Image<float>>(Image<float>(m_width, m_height, 1));

    // FIXME: here we have a problem. We don't need the loss field, but
    shared_ptr<Profile<int>> profile = profileSet[0];

    // ------------------------------------
    // ADG-Multi: note that we are using a specific profile, which is one among the many available.
    //            I believe that this is not relevant for the optimization but only for having a
    //            consistent model.
    // ------------------------------------

    // Check if profile has at least one element
    M_Assert(profile->y.size() != 0, "Empty profile");

    // Precompute the loss field given the profile.
    precomputeLossField(profile);

    if (m_debug)
    {
        // Normalize the image.
        shared_ptr<Image<float>> normLossField = lossFieldNormalize(m_lossField);

        if (saveImage(".", "LossField.png", normLossField) == false)
            log->error(__FILE__, __LINE__, "Error on saveImage");
    }

    Pso<float>::Input input;
    input.w0i = settingsJson["scheimpflug_transformation_calibration"]["pso"]["w0i"].asFloat();
    input.w0f = settingsJson["scheimpflug_transformation_calibration"]["pso"]["w0f"].asFloat();
    input.cp = settingsJson["scheimpflug_transformation_calibration"]["pso"]["cp"].asFloat();
    input.cg = settingsJson["scheimpflug_transformation_calibration"]["pso"]["cg"].asFloat();
    input.nIterations = 50;   // settingsJson["scheimpflug_transformation_calibration"]["pso"]["nIterations"].asInt();
    input.nParticles = 10000; // settingsJson["scheimpflug_transformation_calibration"]["pso"]["nParticles"].asInt();
    input.debug = settingsJson["scheimpflug_transformation_calibration"]["pso"]["debug"].asBool();

    // ADG-Multi: create a model vector. Each model handles one image.
    shared_ptr<vector<shared_ptr<MiniBevelModel>>> modelSet(new vector<shared_ptr<MiniBevelModel>>());

    int nImages = imgXSet.size();
    for (int k = 0; k < nImages; ++k)
    {
        // Get the k-th Scheimpflug model which was initialized with the
        // acquisition angles of the k-th image.
        shared_ptr<ScheimpflugTransform> st = stSet[k];

        // Create the model for the k-th image.
        shared_ptr<MiniBevelModel> model(new MiniBevelModel(st));

        // FIXME: precompute the loss field for each profile.
        vector<double> weight;
        weight.clear();
        Json::Value weightArray = settingsJson["search_space"][0]["weight"];
        for (int i = 0; i < (int)weightArray.size(); ++i)
            weight.push_back(weightArray[i].asDouble());

        // Set the loss field and the profile in the model.
        // FIXME: we are using the wrong lossField.
        model->setLossFieldAndProfile(m_lossField, profileSet[k], checkIntersections, intersectionThreshold, weight);

        // -------------------------------------------
        // ADG-Multi: this is the relevant part. We use precomputed imgX and imgY inside the optimization loop.
        //
        // -------------------------------------------

        // Usually the model is defined in the object plane but here we need to have it
        // defined  directly in the image plane

        // ADG-Multi: is this necessary?
        model->setFromImgPoints(*(imgXSet[k]), *(imgYSet[k]));
        model->printImgPoints();

        // Push the model in the vector
        modelSet->push_back(model);
    }

    // Define the search space
    input.lb.resize(10);
    input.ub.resize(10);

    Json::Value sps = calibrationJson["search_space_scheimpflug"];
    input.lb[0] = sps["alpha"]["min"].asFloat();
    input.ub[0] = sps["alpha"]["max"].asFloat();

    input.lb[1] = sps["beta"]["min"].asFloat();
    input.ub[1] = sps["beta"]["max"].asFloat();

    input.lb[2] = sps["cx0"]["min"].asFloat();
    input.ub[2] = sps["cx0"]["max"].asFloat();

    input.lb[3] = sps["cy0"]["min"].asFloat();
    input.ub[3] = sps["cy0"]["max"].asFloat();

    input.lb[4] = sps["delta"]["min"].asFloat();
    input.ub[4] = sps["delta"]["max"].asFloat();

    input.lb[5] = sps["p1"]["min"].asFloat();
    input.ub[5] = sps["p1"]["max"].asFloat();

    input.lb[6] = sps["p2"]["min"].asFloat();
    input.ub[6] = sps["p2"]["max"].asFloat();

    input.lb[7] = sps["phi"]["min"].asFloat();
    input.ub[7] = sps["phi"]["max"].asFloat();

    input.lb[8] = sps["theta"]["min"].asFloat();
    input.ub[8] = sps["theta"]["max"].asFloat();

    input.lb[9] = sps["tt"]["min"].asFloat();
    input.ub[9] = sps["tt"]["max"].asFloat();

    input.costF = costFScheimpflugMulti_V3;

    // Optimization
    bool validSolution = false;

    // Get frame calibration params fron json object
    Json::Value calibration = calibrationJson["scheimpflug_transformation_calibration"];
    string currentCalibrationFrame = calibration["current"].asString();
    Json::Value frameParams = calibration["frames"][currentCalibrationFrame];

    Pso<float> pso;
    double B_star = frameParams["B"].asDouble();
    double M_star = frameParams["M"].asDouble();
    double angle1_star = stSet[0]->degToRad(frameParams["angle1"].asDouble());
    double angle2_star = stSet[0]->degToRad(frameParams["angle2"].asDouble());

    int kTraining = nImages; // 21; // FIXME

    costFunctionMultiArgs args = {
        modelSet, frameParams["B"].asDouble(), frameParams["M"].asDouble(), angle1_star, angle2_star, 0, kTraining};

    pso.setArgs((void*)&args);
    Pso<float>::Output output;

    // Initialize OptimizationEngine::ScheimpflugTransformOutput values
    shared_ptr<ScheimpflugTransform::Params> stParams = nullptr;
    std::shared_ptr<Profile<int>> bevel = nullptr;
    int iteration = input.nIterations;
    float loss = 1e10;

    float optModel[10];

    int counter = 0;
    Pso<float>::ReturnCode res;
    while (!validSolution && counter < maxRetries)
    {
        printMetrics(modelSet, B_star, M_star, angle1_star, angle2_star, kTraining);

        //--------------------------------------------------------------
        // Optimize Scheimpflug parameters given the current angles
        //--------------------------------------------------------------

        // This flag is used for choosing which type of optimization has to be performed.
        args.flag = 0;

        bool loop = true;
        while (loop)
        {
            res = pso.optimize(input, output);

            // FIXME insert the boundary code.
            bool solutionValidation;

            if (!settingsJson["scheimpflug_transformation_calibration"]["permitBoundarySolutions"].asBool())
            {
                solutionValidation = (res == Pso<float>::ReturnCode::SUCCESS);
                log->info(tag + "Boundary solutions not permitted");
            } else
            {
                solutionValidation =
                    (res == Pso<float>::ReturnCode::SUCCESS || res == Pso<float>::ReturnCode::BOUNDARY_REACHED);
                log->info(tag + "Boundary solutions permitted");
            }

            if (solutionValidation)
                loop = false;
            else
                cout << "WARNING ------> LOOP STEP 1: it = " << endl;
        }
        cout << "\t\t\tSTEP 1) "
             << " loss: " << output.loss << endl;

        // Set each model with the optimum Scheimpflug transform.
        optModel[0] = (float)output.optimumX[0];
        optModel[1] = (float)output.optimumX[1];
        optModel[2] = (float)output.optimumX[2];
        optModel[3] = (float)output.optimumX[3];

        optModel[4] = (float)output.optimumX[4];
        optModel[5] = (float)output.optimumX[5];
        optModel[6] = (float)output.optimumX[6];
        optModel[7] = (float)output.optimumX[7];
        optModel[8] = (float)output.optimumX[8];
        optModel[9] = (float)output.optimumX[9];

        for (int s = 0; s < nImages; ++s)
        {
            // Get the model.
            shared_ptr<MiniBevelModel> model = (*modelSet)[s];

            // Get the value of R and H used for the computation.
            double R = model->scheimpflugTransform()->getR();
            double H = model->scheimpflugTransform()->getH();

            // Initializes the Scheimpflug transform with the optimal model
            // just computed.
            model->scheimpflugTransform()->init(optModel);

            // Reinitialize the angles. This is necessary because we have changed the
            // model parameters.
            // model->scheimpflugTransform()->setStylusOrientation(R, H); // FIXME redundat
        }

        // Print metrics after optimization
        printMetrics(modelSet, B_star, M_star, angle1_star, angle2_star, kTraining);

        // Se tutto va bene, dovrei avere in optModel i parametri ottimi della trasformazione
        // Scheimpflug e in optAngles, gli angoli ottimi.
        cout << "Optimum Scheimpflug parameters:" << endl;
        for (int h = 0; h < 10; ++h)
            cout << "OptParam(" << h << ") = " << optModel[h] << endl;

        double epsilon = settingsJson["scheimpflug_transformation_calibration"]["epsilon"].asDouble();
        cout << "EPSILON " << epsilon << endl;

        // double B, M, angle1, angle2;
        // model->getTestMeasures(B, M, angle1, angle2);
        // double error = sqrt(output.loss) / sqrt((frameParams["B"].asDouble() * frameParams["B"].asDouble()) +
        //                                        (frameParams["M"].asDouble() * frameParams["M"].asDouble()));

        // validSolution = error > epsilon ? false : true;

        // FIXME for testing
        validSolution = true;

        if (validSolution)
        {
            log->info(tag + "Valid solution:");
            stParams = make_shared<ScheimpflugTransform::Params>(ScheimpflugTransform::Params());
            stParams->alpha = optModel[0];
            stParams->beta = optModel[1];
            stParams->cx0 = optModel[2];
            stParams->cy0 = optModel[3];
            stParams->delta = optModel[4];
            stParams->p1 = optModel[5];
            stParams->p2 = optModel[6];
            stParams->phi = optModel[7];
            stParams->theta = optModel[8];
            stParams->tt = optModel[9];
            stParams->magnification = calibrationJson["nominal_scheimpflug_transformation"]["magnification"].asDouble();

            log->info(tag + "Valid solution:");
            {
                stringstream s;
                s << tag << left << setw(13) << "alpha" << setw(13) << "beta" << setw(13) << "cx0" << setw(13) << "cy0"
                  << setw(13) << "delta" << setw(13) << "p1" << setw(13) << "p2" << setw(13) << "phi" << setw(13)
                  << "theta" << setw(13) << "tt" << setw(13) << "loss";
                log->info(s.str());
                s.str("");
                s << tag << left << setw(13) << optModel[0] << setw(13) << optModel[1] << setw(13) << optModel[2]
                  << setw(13) << optModel[3] << setw(13) << optModel[4] << setw(13) << optModel[5] << setw(13)
                  << optModel[6] << setw(13) << optModel[7] << setw(13) << optModel[8] << setw(13) << optModel[9]
                  << setw(13) << output.loss;
                log->info(s.str());
            }

            // Update scheimpflug_transformation json object with the optimum values
            Json::Value st = calibrationJson["scheimpflug_transformation"];
            st["alpha"] = optModel[0];
            st["beta"] = optModel[1];
            st["cx0"] = optModel[2];
            st["cy0"] = optModel[3];
            st["delta"] = optModel[4];
            st["p1"] = optModel[5];
            st["p2"] = optModel[6];
            st["phi"] = optModel[7];
            st["theta"] = optModel[8];
            st["tt"] = optModel[9];

            Json::Value nst = calibrationJson["nominal_scheimpflug_transformation"];

            st["px"] = nst["px"];
            st["magnification"] = nst["magnification"];

            calibrationJson["scheimpflug_transformation"] = st;
            settings->updateCalibration(calibrationJson);

            loss = output.loss;
            iteration = output.optimumIteration;

            // FIXME only first bevel
            bevel = (*modelSet)[0]->getBevel();
            break;
        }
        // else
        {
            // log->error(__FILE__, __LINE__, "PSO failed");
            stringstream s;
            s << tag << "PSO Return code: " << res;
            log->warn(s.str());
        }

        // Increment the counter of retries.
        ++counter;
    }

    shared_ptr<OptimizationEngine::ScheimpflugTransformOutput> out(
        new OptimizationEngine::ScheimpflugTransformOutput());

    out->params = stParams;
    out->bevel = bevel;
    out->iteration = iteration;
    out->loss = loss;
    out->validCalibration = validSolution;
    out->modelType = (*modelSet)[0]->modelType();
    out->R = (*modelSet)[0]->getR();
    out->H = (*modelSet)[0]->getH();

    if (counter >= maxRetries)
    {
        if (res == Pso<float>::ReturnCode::SUCCESS || res == Pso<float>::ReturnCode::BOUNDARY_REACHED)
            if (validSolution)
                out->rcode = ReturnCode::LENSFITTING_FAILED_PSO_BOUNDARY_REACHED;
            else
                out->rcode = ReturnCode::LENSFITTING_FAILED_BEVELFRAME_INTERSECTION;
        else
            out->rcode = ReturnCode::LENSFITTING_FAILED_MAX_RETRIES_REACHED;
    } else
    {
        out->rcode = ReturnCode::LENSFITTING_SUCCESSFUL;
    }

    return out;
}

shared_ptr<OptimizationEngine::CalibrationGridOutput>
OptimizationEngine::optimizeCalibrationGrid(shared_ptr<ScheimpflugTransform> st, const vector<int>& imgX,
                                            const vector<int>& imgY, bool saveValues)
{
    shared_ptr<Settings> settings = Settings::getInstance();
    Json::Value settingsJson = settings->getSettings();
    Json::Value calibrationJson = settings->getCalibration();

    Pso<float>::Input input;
    input.w0i = settingsJson["scheimpflug_transformation_calibration"]["pso"]["w0i"].asFloat();
    input.w0f = settingsJson["scheimpflug_transformation_calibration"]["pso"]["w0f"].asFloat();
    input.cp = settingsJson["scheimpflug_transformation_calibration"]["pso"]["cp"].asFloat();
    input.cg = settingsJson["scheimpflug_transformation_calibration"]["pso"]["cg"].asFloat();
    input.nIterations = settingsJson["scheimpflug_transformation_calibration"]["pso"]["nIterations"].asInt();
    input.nParticles = settingsJson["scheimpflug_transformation_calibration"]["pso"]["nParticles"].asInt();
    input.debug = settingsJson["scheimpflug_transformation_calibration"]["pso"]["debug"].asBool();

    // Get the angle of calibration
    Json::Value grid = calibrationJson["scheimpflug_transformation_calibration"]["grid"];
    double H = grid["H"].asDouble();
    double deltaN = grid["deltaN"].asDouble();
    float lambda = grid["lambda"].asFloat();

    // Define the search space which comprises the following 10 parameters:
    // tt, theta, alpha, p1, p2, delta, beta, phi, cx0, and cy0

    // Reserve the space for the lower and upper boundaries used during optimization
    // for the 10 parameters
    input.lb.resize(10);
    input.ub.resize(10);

    // Parameters' boundaries are loaded from the json structure.
    Json::Value sps = calibrationJson["search_space_scheimpflug"];

    input.lb[0] = sps["alpha"]["min"].asFloat();
    input.ub[0] = sps["alpha"]["max"].asFloat();

    input.lb[1] = sps["beta"]["min"].asFloat();
    input.ub[1] = sps["beta"]["max"].asFloat();

    input.lb[2] = sps["cx0"]["min"].asFloat();
    input.ub[2] = sps["cx0"]["max"].asFloat();

    input.lb[3] = sps["cy0"]["min"].asFloat();
    input.ub[3] = sps["cy0"]["max"].asFloat();

    input.lb[4] = sps["delta"]["min"].asFloat();
    input.ub[4] = sps["delta"]["max"].asFloat();

    input.lb[5] = sps["p1"]["min"].asFloat();
    input.ub[5] = sps["p1"]["max"].asFloat();

    input.lb[6] = sps["p2"]["min"].asFloat();
    input.ub[6] = sps["p2"]["max"].asFloat();

    input.lb[7] = sps["phi"]["min"].asFloat();
    input.ub[7] = sps["phi"]["max"].asFloat();

    input.lb[8] = sps["theta"]["min"].asFloat();
    input.ub[8] = sps["theta"]["max"].asFloat();

    input.lb[9] = sps["tt"]["min"].asFloat();
    input.ub[9] = sps["tt"]["max"].asFloat();

    // Nominal parameter loaded from the json structure.
    Json::Value nst = calibrationJson["nominal_scheimpflug_transformation"];
    float nominalV[10] = {nst["alpha"].asFloat(), nst["beta"].asFloat(), nst["cx0"].asFloat(), nst["cy0"].asFloat(),
                          nst["delta"].asFloat(), nst["p1"].asFloat(),   nst["p2"].asFloat(),  nst["phi"].asFloat(),
                          nst["theta"].asFloat(), nst["tt"].asFloat()};

    // Set the cost function
    input.costF = costFCalibrationGrid;

    // Optimization
    bool validSolution = false;

    Pso<float> pso;
    costFCalibrationGridArgs args = {st, imgX, imgY, H, deltaN, nominalV, lambda};
    pso.setArgs((void*)&args);
    Pso<float>::Output output;

    shared_ptr<ScheimpflugTransform::Params> stParams = nullptr;
    int iteration = input.nIterations;
    float loss = 1e10;

    int counter = 0;
    Pso<float>::ReturnCode res;

    // int maxRetries = 1;
    int maxRetries = settingsJson["scheimpflug_transformation_calibration"]["maxRetries"].asInt();

    float errorGrid = settingsJson["scheimpflug_transformation_calibration"]["epsilonGrid"].asFloat();

    // FIXME: compute initial optimum value using nominal values
    //--------------
    // {
    //     float resN = 0.0f;
    //     costFCalibrationGrid(&resN, nominalV, 10, 1, (void*)&args);
    //     cout << "resN: " << resN << endl;
    // }
    //--------------

    while (!validSolution && counter < maxRetries)
    {
        // Call the PSO
        res = pso.optimize(input, output);

        bool solutionValidation;

        if (!settingsJson["scheimpflug_transformation_calibration"]["permitBoundarySolutions"].asBool())
        {
            solutionValidation = (res == Pso<float>::ReturnCode::SUCCESS);
            log->info(tag + "Boundary solutions not permitted");
        } else
        {
            solutionValidation =
                (res == Pso<float>::ReturnCode::SUCCESS || res == Pso<float>::ReturnCode::BOUNDARY_REACHED);
            log->info(tag + "Boundary solutions permitted");
        }

        if (solutionValidation)
        {
            // Get the optimal model from the PSO output.
            float optModel[] = {(float)output.optimumX[0], (float)output.optimumX[1], (float)output.optimumX[2],
                                (float)output.optimumX[3], (float)output.optimumX[4], (float)output.optimumX[5],
                                (float)output.optimumX[6], (float)output.optimumX[7], (float)output.optimumX[8],
                                (float)output.optimumX[9]};

            // Initialize the Scheimpflug transformation with the optimal model found.
            st->init(optModel);

            // st->init(calibrationJson["nominal_scheimpflug_transformation"]);

            float error = output.loss;
            validSolution = error < errorGrid;

            // Note: cx0 and cy0 detected from the Hough transform are the last coordinates
            // in imgX and imgY vectors. If we want to substitute the optimized values with
            // the detected values, we need to use the following variables
            double gridCx0 = (double)imgX[imgX.size() - 1];
            double gridCy0 = (double)imgY[imgY.size() - 1];
            {
                stringstream s;
                s << tag << "Grid center: (" << gridCx0 << ", " << gridCy0 << ")";
                log->info(s.str());
            }

            if (validSolution)
            {
                stParams = make_shared<ScheimpflugTransform::Params>(ScheimpflugTransform::Params());
                stParams->alpha = optModel[0];
                stParams->beta = optModel[1];
                stParams->cx0 = optModel[2];
                stParams->cy0 = optModel[3];
                stParams->delta = optModel[4];
                stParams->p1 = optModel[5];
                stParams->p2 = optModel[6];
                stParams->phi = optModel[7];
                stParams->theta = optModel[8];
                stParams->tt = optModel[9];

                // magnification is not optimized
                stParams->magnification =
                    calibrationJson["nominal_scheimpflug_transformation"]["magnification"].asDouble();

                log->info(tag + "Valid solution:");
                {
                    stringstream s;
                    s << tag << left << setw(13) << "alpha" << setw(13) << "beta" << setw(13) << "cx0" << setw(13)
                      << "cy0" << setw(13) << "delta" << setw(13) << "p1" << setw(13) << "p2" << setw(13) << "phi"
                      << setw(13) << "theta" << setw(13) << "tt" << setw(13) << "loss";
                    log->info(s.str());
                    s.str("");
                    s << tag << left << setw(13) << optModel[0] << setw(13) << optModel[1] << setw(13) << optModel[2]
                      << setw(13) << optModel[3] << setw(13) << optModel[4] << setw(13) << optModel[5] << setw(13)
                      << optModel[6] << setw(13) << optModel[7] << setw(13) << optModel[8] << setw(13) << optModel[9]
                      << setw(13) << output.loss;
                    log->info(s.str());
                }

                if (saveValues)
                {
                    // Update scheimpflug_transformation json object with the optimum values
                    Json::Value st = calibrationJson["scheimpflug_transformation"];
                    st["alpha"] = optModel[0];
                    st["beta"] = optModel[1];
                    st["cx0"] = optModel[2];
                    st["cy0"] = optModel[3];
                    st["delta"] = optModel[4];
                    st["p1"] = optModel[5];
                    st["p2"] = optModel[6];
                    st["phi"] = optModel[7];
                    st["theta"] = optModel[8];
                    st["tt"] = optModel[9];

                    Json::Value nst = calibrationJson["nominal_scheimpflug_transformation"];

                    st["px"] = nst["px"];
                    st["magnification"] = nst["magnification"];

                    calibrationJson["scheimpflug_transformation"] = st;
                    settings->updateCalibration(calibrationJson);
                    // settings->printCalibration();
                }

                loss = output.loss;
                iteration = output.optimumIteration;
            }
        } else
        {
            log->warn(tag + "PSO failed");
        }

        // Increment the counter of retries.
        ++counter;
    }

    shared_ptr<OptimizationEngine::CalibrationGridOutput> out(new CalibrationGridOutput());
    out->validCalibration = validSolution;
    out->params = stParams;
    out->loss = loss;
    out->iteration = iteration;

    if (validSolution)
    {
        out->rcode = ReturnCode::CALIBRATION_SUCCESSFUL;
    } else
    {
        if (counter >= maxRetries)
        {
            if (res != Pso<float>::ReturnCode::SUCCESS)
            {
                switch (res)
                {
                case Pso<float>::ReturnCode::BOUNDARY_REACHED:
                    out->rcode = ReturnCode::CALIBRATION_FAILED_SEARCH_SPACE;
                    break;
                default:
                    out->rcode = ReturnCode::CALIBRATION_FAILED_PSO_INIT;
                }
            } else
                out->rcode = ReturnCode::CALIBRATION_FAILED_CONVERGENCE;
        }
    }

    return out;
}

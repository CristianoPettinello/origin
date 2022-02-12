//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "MiniBevelModelExt.h"
#include "Log.h"
#include "Utils.h"
#include <cstring>
#include <iostream>
#include <math.h>

using namespace Nidek::Libraries::HTT;

static const string tag = "[MBME] ";

#define SHOULDER_LENGTH 120 // 80
#define MINIBEVEL_POINTS 5
#define DISTANCE_POINTS 2

MiniBevelModelExt::MiniBevelModelExt(shared_ptr<ScheimpflugTransform> st, double bevelAngle /* = 55.0 */) : Model(st)
{
    m_A = m_st->pixelToMm(SHOULDER_LENGTH);
    m_B = 0.0;
    m_Bstar = 0.0;
    m_C = m_A;
    m_D = 0.0;
    m_xm = 0.0;
    m_ym = 0.0;

    // Set the adjacent bevel angle to a predefined value in the object plane.
    {
        stringstream s;
        s << tag << "MiniBevelModelExt initial angle: " << bevelAngle << "°";
        log->info(s.str());
    }
    m_delta = (90.0 - bevelAngle) / 180.0 * 3.141592;

    m_tanDelta = tan(m_delta) * 0.5;
    m_M = 0.0;
    m_alpha = 0.0;

    m_objX.resize(MINIBEVEL_POINTS);
    m_objY.resize(MINIBEVEL_POINTS);

    m_imgX.resize(MINIBEVEL_POINTS);
    m_imgY.resize(MINIBEVEL_POINTS);

    m_distanceImgX.resize(DISTANCE_POINTS);
    m_distanceImgY.resize(DISTANCE_POINTS);
}

bool MiniBevelModelExt::setFreeParams(const float* v, const double* /* offset */)
{
    double* objXPtr = m_objX.data();
    double* objYPtr = m_objY.data();

    // Set the model's free parameters
    objXPtr[1] = (double)v[0];
    objYPtr[1] = (double)v[1];
    m_alpha = (double)v[2];
    m_M = (double)v[3];
    m_delta = (90.0 - (double)v[4]) / 180.0 * 3.141592;
    m_tanDelta = tan(m_delta) * 0.5;

    // Compute other parameters
    computeOtherParams();

    return computeConstrainedBevelProfile();
}

void MiniBevelModelExt::computeOtherParams()
{
    double* objXPtr = m_objX.data();
    double* objYPtr = m_objY.data();

    double sin_alpha = sin(m_alpha);
    double cos_alpha = cos(m_alpha);

    double m_AsinAlpha = m_A * sin_alpha;
    double m_AcosAlpha = m_A * cos_alpha;

    objXPtr[0] = objXPtr[1] - m_AsinAlpha;
    objYPtr[0] = objYPtr[1] + m_AcosAlpha;

    objXPtr[3] = objXPtr[1] + m_M * sin_alpha;
    objYPtr[3] = objYPtr[1] - m_M * cos_alpha;

    objXPtr[4] = objXPtr[3] + m_AsinAlpha; // this is because m_A = m_C
    objYPtr[4] = objYPtr[3] - m_AcosAlpha; // this is because m_A = m_C

    // Get the middle point
    m_xm = (objXPtr[1] + objXPtr[3]) * 0.5;
    m_ym = (objYPtr[1] + objYPtr[3]) * 0.5;

    // B is the height of the triangle
    // delta = 90°-55°=35° angle between the cathetus and the base of the triangle
    // B = M/2 * tan(delta) [m_tanDelta = tan(delta)/2 for optimization]
    m_B = m_tanDelta * m_M;
    objXPtr[2] = m_xm + m_B * cos_alpha;
    objYPtr[2] = m_ym + m_B * sin_alpha;

    // Compute the points in the image plane.
    computeImgPoints(m_objX, m_objY);
}

shared_ptr<Model::Measures> MiniBevelModelExt::getMeasures()
{
    shared_ptr<Model::Measures> measures(new Model::Measures());
    measures->m.insert(make_pair("A", (float)m_A));
    measures->m.insert(make_pair("B", (float)m_B));
    measures->m.insert(make_pair("C", (float)m_C));
    measures->m.insert(make_pair("D", (float)m_D));
    measures->m.insert(make_pair("M", (float)m_M));
    measures->m.insert(make_pair("B*", (float)m_Bstar));

    double beta = (90.0 - m_delta / 3.141592 * 180.0) * 2.0;
    measures->m.insert(make_pair("beta", (float)beta));

    return measures;
}

ModelType MiniBevelModelExt::modelType()
{
    return MiniBevelModelExtType;
}

bool MiniBevelModelExt::finalizeModel(const float* v, const int* lastImgPointProfile, shared_ptr<Profile<int>> profile)
{
    bool validSolution = setFreeParams(v);

    if (validSolution)
    {
        m_distanceImgX.clear();
        m_distanceImgY.clear();

        double srcObjPoint[] = {m_xm, m_ym, 0.0};
        double dstImgPoint[] = {0.0, 0.0, 0.0};
        m_st->objectToImage(srcObjPoint, dstImgPoint);

        m_distanceImgX.push_back((float)dstImgPoint[0]);
        m_distanceImgY.push_back((float)dstImgPoint[1]);

        double srcImgPoint[] = {(double)lastImgPointProfile[0], (double)lastImgPointProfile[1], 0.0};
        double dstObjPoint[] = {0.0, 0.0, 0.0};
        m_st->imageToObject(srcImgPoint, dstObjPoint);

        double X3_4 = m_objX[3] - m_objX[4];
        double Y3_4 = m_objY[3] - m_objY[4];
        double X3Y4 = m_objX[3] * m_objY[4];
        double X4Y3 = m_objX[4] * m_objY[3];
        double m1 = Y3_4 / X3_4;
        double m2 = -1.0 / m1;
        double q1 = ((X3Y4 - X4Y3) / X3_4);
        double q2 = dstObjPoint[1] - (m2 * dstObjPoint[0]);

        double Xb, Yb;
        if (fabs(X3_4) > 1e-6)
        // if (fabs(X3_4) > 0.2)
        {
            // log->debug(tag + "solution y=mx+q");
            Xb = (q2 - q1) / (m1 - m2);
            Yb = (m1 * Xb) + q1;
        } else
        {
            Yb = (dstObjPoint[0] + (dstObjPoint[1] * m1) - (q1 * m2)) / (m1 - m2);
            Xb = m2 * (q1 - Yb);
        }

        float Xb_xm = (float)(Xb - m_xm);
        float Yb_ym = (float)(Yb - m_ym);
        m_D = sqrt((Xb_xm * Xb_xm) + (Yb_ym * Yb_ym));

        double srcObjPointB[] = {Xb, Yb, 0.0};
        double dstImgPointB[] = {0.0, 0.0, 0.0};
        m_st->objectToImage(srcObjPointB, dstImgPointB);

        m_distanceImgX.push_back((float)dstImgPointB[0]);
        m_distanceImgY.push_back((float)dstImgPointB[1]);

        {
            // dstImgPoint[0] -> m_xM on img plane
            // dstImgPoint[1] -> m_yM on img plane

            // y = mx + q;
            double deltaX = dstImgPoint[0] - m_imgX[2];
            double deltaY = dstImgPoint[1] - m_imgY[2];

            double m = deltaY / deltaX;
            double q = m_imgY[2] - m * m_imgX[2];

            // Search the intersection between the line and the profile.
            int N = profile->x.size();
            // cout << "Size N" << N << endl;
            double val = 1e10;
            double optX = 0.0, optY = 0.0;
            for (int n = 0; n < N; ++n)
            {
                double x = profile->x[n];
                double y = profile->y[n];

                double v = fabs(m * x + q - y);
                // cout << "x, y, v: " << x << y << v << endl;
                if (v < val)
                {
                    optX = x;
                    optY = y;
                    val = v;
                }
            }

            double srcImgPoint[] = {optX, optY, 0.0};
            double dstObjPoint[] = {0.0, 0.0, 0.0};
            m_st->imageToObject(srcImgPoint, dstObjPoint);

            double vertX = dstObjPoint[0];
            double vertY = dstObjPoint[1];
            m_Bstar = sqrt((vertX - m_xm) * (vertX - m_xm) + (vertY - m_ym) * (vertY - m_ym));

            cout << "Distance: " << m_Bstar << endl;
        }
    }

    if (m_debug)
    {
        stringstream s;
        s << tag << "Intersection points: " << getIntersectionCounter();
        log->debug(s.str());
    }

    return validSolution;
}

void MiniBevelModelExt::setFromImgPoints(const vector<float>& imgX, const vector<float>& imgY)
{
    // Usually the model is defined in the object plane and the scheimpflug transformation
    // maps it on the image plane. For the calibration of the Scheimpflug transformation
    // this order is reversed and we need to set the model points in the image plane and get the
    // corresponding points in the object plane. After that, we can proceed with the computation
    // of the bevel profile. Once that these operations are performed we have a consistent model defined
    // both in the object and image plane and with its bevel representation.
    m_imgX = imgX;
    m_imgY = imgY;

    computeObjPoints();
    computeConstrainedBevelProfile();
}

void MiniBevelModelExt::getTestMeasures(double& B, double& M, double& angle1, double& angle2)
{
    M = m_M;
    B = m_B;

    M_Assert(m_objX[2] != m_objX[1], "m_objX[2] == m_objX[1] detected");
    M_Assert(m_objX[2] != m_objX[3], "m_objX[2] == m_objX[3] detected");

    double m1 = (m_objY[2] - m_objY[1]) / (m_objX[2] - m_objX[1]);
    double m2 = (m_objY[2] - m_ym) / (m_objX[2] - m_xm);
    angle1 = fabs(atan2((m1 - m2), (1 + m1 * m2)));

    double m3 = (m_objY[2] - m_objY[3]) / (m_objX[2] - m_objX[3]);
    angle2 = fabs(atan2((m2 - m3), (1 + m3 * m2)));
}

void MiniBevelModelExt::computeObjPoints()
{
    Model::computeObjPoints();

    m_M = sqrt((m_objX[1] - m_objX[3]) * (m_objX[1] - m_objX[3]) + (m_objY[1] - m_objY[3]) * (m_objY[1] - m_objY[3]));
    m_xm = (m_objX[1] + m_objX[3]) / 2;
    m_ym = (m_objY[1] + m_objY[3]) / 2;
    m_B = sqrt((m_xm - m_objX[2]) * (m_xm - m_objX[2]) + (m_ym - m_objY[2]) * (m_ym - m_objY[2]));
    m_alpha = atan2(-m_objX[1] + m_objX[3], m_objY[1] - m_objY[3]);
}

void MiniBevelModelExt::printMeasures()
{
    stringstream s;
    s << tag << "Bevel Measures [mm]:";
    log->info(s.str());
    s.str("");
    double beta = (90.0 - m_delta / 3.141592 * 180.0) * 2.0;
    s << tag << "   B*: " << m_Bstar << "   B: " << m_B << " M: " << m_M << " D: " << m_D << " beta: " << beta << "°";
    log->info(s.str());
}

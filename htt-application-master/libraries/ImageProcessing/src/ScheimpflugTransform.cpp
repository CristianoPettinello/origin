//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "ScheimpflugTransform.h"

#include <cstring>
#include <iostream>
#include <math.h>

using namespace std;
using namespace Nidek::Libraries::HTT;

#define ENABLE_OPTIMIZATION
static const string tag = "[SCHT] ";

ScheimpflugTransform::ScheimpflugTransform() : log(Log::getInstance())
{
}

void ScheimpflugTransform::init(Json::Value& params)
{
    m_tt = params["tt"].asDouble();
    m_p1 = params["p1"].asDouble();
    m_p2 = params["p2"].asDouble();
    m_px = params["px"].asDouble();

    m_theta = degToRad(params["theta"].asDouble());
    m_phi = degToRad(params["phi"].asDouble());

    m_alpha = degToRad(params["alpha"].asDouble());
    m_beta = degToRad(params["beta"].asDouble());

    m_magnification = params["magnification"].asDouble();
    m_delta = degToRad(params["delta"].asDouble());

    m_cx0 = params["cx0"].asInt();
    m_cy0 = params["cy0"].asInt();

    // Initialize the variables that depends only on the
    // Scheimpflug optics.
    initializeScheimpflugModel();
}

void ScheimpflugTransform::init(float* params)
{
    // This function is used only during the calibration of the Scheimpflug Transform
    // parameters.

    m_alpha = degToRad((double)params[0]);
    m_beta = degToRad((double)params[1]);
    m_cx0 = (int)params[2];
    m_cy0 = (int)params[3];
    m_delta = degToRad((double)params[4]);
    m_p1 = (double)params[5];
    m_p2 = (double)params[6];
    m_phi = degToRad((double)params[7]);
    m_theta = degToRad((double)params[8]);
    m_tt = (double)params[9];

    // Initialize the variables that depends only on the
    // Scheimpflug optics.
    initializeScheimpflugModel();

    double R = getR();
    double H = getH();

    setStylusOrientation(R, H);
}

void ScheimpflugTransform::initializeScheimpflugModel()
{
    m_iPx = 1.0 / m_px;

    double RY_theta[9];
    double RX_phi[9];
    double RX_beta[9];
    double RY_alpha[9];

    RY(RY_theta, m_theta);
    RX(RX_phi, m_phi);
    RX(RX_beta, m_beta);
    RY(RY_alpha, m_alpha);

    // Compute Rtf and Rab
    double Rtf[9], Rab[9];
    matrixProduct(Rtf, RY_theta, RX_phi);
    matrixProduct(Rab, RX_beta, RY_alpha);

    double vr[3];
    double z[3] = {0.0, 0.0, 1.0};

    // Compute vr
    vectorProduct(vr, Rtf, z);

    // Compute S, P1 and P2
    // double S[3] = {m_tt * vr[0], m_tt * vr[1], m_tt * vr[2]};
    m_S[0] = m_tt * vr[0];
    m_S[1] = m_tt * vr[1];
    m_S[2] = m_tt * vr[2];

    // double P1[3] = {m_p1 * vr[0], m_p1 * vr[1], m_p1 * vr[2]};
    m_P1[0] = m_p1 * vr[0];
    m_P1[1] = m_p1 * vr[1];
    m_P1[2] = m_p1 * vr[2];

    // double P2[3] = {m_p2 * vr[0], m_p2 * vr[1], m_p2 * vr[2]};
    m_P2[0] = m_p2 * vr[0];
    m_P2[1] = m_p2 * vr[1];
    m_P2[2] = m_p2 * vr[2];

    // Vector perpendicular to the sensor plane
    // double nS[3];

    matrixProduct(m_RtfRab, Rtf, Rab);
    vectorProduct(m_nS, m_RtfRab, z);

    transpose(m_T, m_RtfRab);

    // Optimization
    for (int j = 0; j < 9; ++j)
        m_T[j] *= m_iPx;

    RY(m_RY_delta, m_delta);
}

void ScheimpflugTransform::setStylusOrientation(double R, double H)
{
    // Store the current stylus orientation in radians
    m_R = degToRad(-R);
    m_H = degToRad(H);

    // double tmpM[9];
    double vg[3] = {0.0, 0.0, 0.0};
    double z[3] = {0.0, 0.0, 1.0};
    double w[3] = {10.0, 0.0, 1.0}; // BRT v9

    RZ(m_RZ_H, m_H);
    RY(m_RY_R, m_R);
    RZ(m_RZ_mH, -m_H);
    // RY(m_RY_mR, -m_R);

    RY(m_RY_R_delta, m_R + m_delta);      // BRT v9
    RY(m_RY_m_R_delta, -(m_R + m_delta)); // BRT v9

    // matrixProduct(tmpM, m_RY_R, m_RY_delta);
    matrixProduct(m_RY_R_x_RY_delta, m_RY_R, m_RY_delta);
    vectorProduct(vg, m_RY_R_x_RY_delta, z); // Compute vg

// Compute C
#if 0 
    vectorProduct(m_C, m_RY_R, z);
    // m_C[0] -= z[0]; // can be commented because z[0] = 0
    // m_C[1] -= z[1]; // can be commented because z[1] = 0
    m_C[2] -= z[2];
#else
    // FIXME: BRT rev9
    double m_ConlyR[3];
    vectorProduct(m_ConlyR, m_RY_R, z);
    // vectorProduct(m_ConlyR, m_RY_mR, z); // FIXME: RY(-R)
    // m_ConlyR[0] -= z[0]; // can be commented because z[0] = 0
    // m_ConlyR[1] -= z[1]; // can be commented because z[1] = 0
    m_ConlyR[2] -= z[2];

    vectorProduct(m_C, m_RY_delta, w);
    m_C[0] += m_ConlyR[0] - w[0];
    m_C[1] += m_ConlyR[1]; // - w[1]; // can be commented because w[1] = 0
    m_C[2] += m_ConlyR[2] - w[2];
#endif

    // Compute B
    double B[3] = {0.0, 0.0, 0.0};
    // B[0] = 0.0;
    // B[1] = 0.0;
    B[2] = (vg[2] * m_C[2] + vg[0] * m_C[0]) / vg[2];

    double d0 = B[0] - m_C[0];
    double d1 = B[1] - m_C[1];
    double d2 = B[2] - m_C[2];

    m_dBC = sqrt(d0 * d0 + d1 * d1 + d2 * d2);

    // Other precomputed values
    m_v1 = m_dBC * sign(m_C[0]);
    m_v3 = cos(m_R + m_delta);
    m_v2 = 1.0 / m_v3;
    m_v4 = (m_nS[0] * m_S[0] + m_nS[1] * m_S[1] + m_nS[2] * m_S[2]) -
           (m_nS[0] * m_P2[0] + m_nS[1] * m_P2[1] + m_nS[2] * m_P2[2]);
    m_v5 = tan(m_R + m_delta);
    m_v6 = -(m_P1[2] + m_v5 * m_P1[0]);
}

#ifndef ENABLE_OPTIMIZATION
void ScheimpflugTransform::objectToImage(const double* srcImgPoint, double* dstImgPoint,
                                         bool enableRounding /* = true */)
{
    double M0[3];
    memcpy(M0, srcImgPoint, 3 * sizeof(double));

#if 1
    double M[3];
    // vectorProduct(M, m_RZ_H, M0);
    vectorProduct(M, m_RZ_mH, M0); // FIXME: RZ(-H)  instead of RZ(H)

    // M[0] = M[0] / cos(m_R + m_delta) + m_dBC * sign(m_C[0]); // FIXME
    M[0] = M[0] / cos(m_R + m_delta) - m_dBC * sign(m_C[0]);
#else
    // FIXME - BRT v9
    double M1[3];
    vectorProduct(M1, m_RZ_mH, M0);

    M1[0] = M1[0] / cos(m_R + m_delta) + m_dBC * sign(m_C[0]);

    double M[3];
    vectorProduct(M, m_RY_R_delta, M1);
#endif

    double vt[3];
    vt[0] = m_P1[0] - M[0];
    vt[1] = m_P1[1] - M[1];
    vt[2] = m_P1[2] - M[2];

    double norm = vt[0] * vt[0] + vt[1] * vt[1] + vt[2] * vt[2];

    if (norm > 0.0)
    {
        double iNorm = 1.0 / sqrt(norm);
        vt[0] *= iNorm;
        vt[1] *= iNorm;
        vt[2] *= iNorm;
    }

    double lam = ((m_nS[0] * m_S[0] + m_nS[1] * m_S[1] + m_nS[2] * m_S[2]) -
                  (m_nS[0] * m_P2[0] + m_nS[1] * m_P2[1] + m_nS[2] * m_P2[2])) /
                 (m_nS[0] * vt[0] + m_nS[1] * vt[1] + m_nS[2] * vt[2]);

    double MS[3];
    MS[0] = m_P2[0] + vt[0] * lam;
    MS[1] = m_P2[1] + vt[1] * lam;
    MS[2] = m_P2[2] + vt[2] * lam;

    double MSc[3];
    MSc[0] = MS[0] - m_S[0];
    MSc[1] = MS[1] - m_S[1];
    MSc[2] = MS[2] - m_S[2];

    double* MSp = dstImgPoint;
    vectorProduct(MSp, m_T, MSc);

    // MSp[0] *= m_iPx;
    // MSp[1] *= m_iPx;
    // MSp[2] *= m_iPx;

    // Correct center and convert to int.
    MSp[0] += m_cx0;
    MSp[1] += m_cy0;

    if (enableRounding)
    {
        MSp[0] = round(MSp[0]);
        MSp[1] = round(MSp[1]);
        MSp[2] = round(MSp[2]);
    }
}
#else

void ScheimpflugTransform::objectToImage(const double* srcImgPoint, double* dstImgPoint,
                                         bool enableRounding /* = true */)
{
    // double M[3];

    // Original code
    // vectorProduct(M, m_RZ_H, srcImgPoint);  // BRT v7
    // vectorProduct(M, m_RZ_mH, srcImgPoint); // BRT v9

    // Optimized code
    // const double s0 = srcImgPoint[0];
    // const double s1 = srcImgPoint[1];
    // const double s2 = srcImgPoint[2];
    // vectorProduct2(M, m_RZ_H, srcImgPoint[0], srcImgPoint[1], srcImgPoint[2]); // BRT v7
    // vectorProduct2(M, m_RZ_mH, srcImgPoint[0], srcImgPoint[1], srcImgPoint[2]); // BRT v9

    // Assumes srcImgPoint[2] = v2 = 0
    const double srcImgPoint0 = srcImgPoint[0];
    const double srcImgPoint1 = srcImgPoint[1];

#if 0
    // FIXME - BRT v7
    const double M0 = m_RZ_H[0] * srcImgPoint0 + m_RZ_H[1] * srcImgPoint1 /*+ m_RZ_H[2] * srcImgPoint[2]*/;
    const double M1 = m_RZ_H[3] * srcImgPoint0 + m_RZ_H[4] * srcImgPoint1 /*+ m_RZ_H[5] * srcImgPoint[2]*/;
    const double M2 = m_RZ_H[6] * srcImgPoint0 + m_RZ_H[7] * srcImgPoint1 /*+ m_RZ_H[8] * srcImgPoint[2]*/;
#else
    // FIXME - BRT v9
    const double M0 = m_RZ_mH[0] * srcImgPoint0 + m_RZ_mH[1] * srcImgPoint1 /*+ m_RZ_mH[2] * srcImgPoint[2]*/;
    const double M1 = m_RZ_mH[3] * srcImgPoint0 + m_RZ_mH[4] * srcImgPoint1 /*+ m_RZ_mH[5] * srcImgPoint[2]*/;
    const double M2 = m_RZ_mH[6] * srcImgPoint0 + m_RZ_mH[7] * srcImgPoint1 /*+ m_RZ_mH[8] * srcImgPoint[2]*/;
#endif

    // Original code
    // // M[0] = M[0] / cos(m_R + m_delta) + m_dBC * sign(m_C[0]);
    // M[0] = M[0] / cos(m_R + m_delta) - m_dBC * sign(m_C[0]);
    // vectorProduct(M, m_RY_R_delta, M);

#if 1
    // FIXME - BRT v7

    // Optimized code
    // const double vt0 = m_P1[0] - (M0 * m_v2 + m_v1);
    const double vt0 = m_P1[0] - (M0 * m_v2 - m_v1); // FIXME
    const double vt1 = m_P1[1] - M1;
    const double vt2 = m_P1[2] - M2;
#else
    // FIXME - BRT v9

    // Optimized code
    M0 = M0 * m_v2 + m_v1;
    double w[3];
    vectorProduct2(w, m_RY_R_delta, M0, M1, M2);

    const double vt0 = m_P1[0] - w[0];
    const double vt1 = m_P1[1] - w[1];
    const double vt2 = m_P1[2] - w[2];
#endif

    // Original Code: Note it seems that the normalization is not necessary
    // double norm = vt0 * vt0 + vt1 * vt1 + vt2 * vt2;

    /*if(norm > 0.0)
    {
        double iNorm = 1.0 / sqrt(norm);
        vt0 *= iNorm;
        vt1 *= iNorm;
        vt2 *= iNorm;
    }*/

    // double lam = ((m_nS[0] * m_S[0] + m_nS[1] * m_S[1] + m_nS[2] * m_S[2]) -
    //             (m_nS[0] * m_P2[0] + m_nS[1] * m_P2[1] + m_nS[2] * m_P2[2])) /
    //             (m_nS[0] * vt0 + m_nS[1] * vt1 + m_nS[2] * vt2);
    const double lam = m_v4 / (m_nS[0] * vt0 + m_nS[1] * vt1 + m_nS[2] * vt2);

    // double MS[3];
    // double MS0 = m_P2[0] + vt0 * lam;
    // double MS1 = m_P2[1] + vt1 * lam;
    // double MS2 = m_P2[2] + vt2 * lam;

    // Original Code
    // double MSc[3];
    // MSc[0] = MS0 - m_S[0];
    // MSc[1] = MS1 - m_S[1];
    // MSc[2] = MS2 - m_S[2];

    // Optimized code
    // double MSc0 = MS0 - m_S[0];
    // double MSc1 = MS1 - m_S[1];
    // double MSc2 = MS2 - m_S[2];

    const double MSc0 = m_P2[0] + vt0 * lam - m_S[0];
    const double MSc1 = m_P2[1] + vt1 * lam - m_S[1];
    const double MSc2 = m_P2[2] + vt2 * lam - m_S[2];

    double* MSp = dstImgPoint;
    // vectorProduct(MSp, m_T, MSc); // Original code
    // vectorProduct2(MSp, m_T, MSc0, MSc1, MSc2); // Optimized code

    // Assumes MSp[2] = 0
    MSp[0] = m_T[0] * MSc0 + m_T[1] * MSc1 + m_T[2] * MSc2;
    MSp[1] = m_T[3] * MSc0 + m_T[4] * MSc1 + m_T[5] * MSc2;
    MSp[2] = 0.0; // m_T[6] * MSc0 + m_T[7] * MSc1 + m_T[8] * MSc2;

    // MSp[0] *= m_iPx;
    // MSp[1] *= m_iPx;
    // MSp[2] *= m_iPx; // Optimized code: this should be 0

    // Correct center and convert to int.
    MSp[0] += m_cx0;
    MSp[1] += m_cy0;

    if (enableRounding)
    {
        MSp[0] = round(MSp[0]);
        MSp[1] = round(MSp[1]);
        // MSp[2] = round(MSp[2]); // this should be 0
    }
}

void ScheimpflugTransform::objectToImageWithRounding(const double* srcImgPoint, double* dstImgPoint)
{
    // Assumes srcImgPoint[2] = v2 = 0
    const double srcImgPoint0 = srcImgPoint[0];
    const double srcImgPoint1 = srcImgPoint[1];

#if 0
    // BRT v7
    const double M0 = m_RZ_H[0] * srcImgPoint0 + m_RZ_H[1] * srcImgPoint1 /*+ m_RZ_H[2] * srcImgPoint[2]*/;
    const double M1 = m_RZ_H[3] * srcImgPoint0 + m_RZ_H[4] * srcImgPoint1 /*+ m_RZ_H[5] * srcImgPoint[2]*/;
    const double M2 = m_RZ_H[6] * srcImgPoint0 + m_RZ_H[7] * srcImgPoint1 /*+ m_RZ_H[8] * srcImgPoint[2]*/;
#else
    // BRT v9
    const double M0 = m_RZ_mH[0] * srcImgPoint0 + m_RZ_mH[1] * srcImgPoint1 /*+ m_RZ_mH[2] * srcImgPoint[2]*/;
    const double M1 = m_RZ_mH[3] * srcImgPoint0 + m_RZ_mH[4] * srcImgPoint1 /*+ m_RZ_mH[5] * srcImgPoint[2]*/;
    const double M2 = m_RZ_mH[6] * srcImgPoint0 + m_RZ_mH[7] * srcImgPoint1 /*+ m_RZ_mH[8] * srcImgPoint[2]*/;
#endif

#if 1
    // FIXME - BRT v7
    // const double vt0 = m_P1[0] - (M0 * m_v2 + m_v1); // FIXME
    const double vt0 = m_P1[0] - (M0 * m_v2 - m_v1);
    const double vt1 = m_P1[1] - M1;
    const double vt2 = m_P1[2] - M2;
#else
    // FIXME - BRT v9

    // Optimized code
    M0 = M0 * m_v2 + m_v1;
    double w[3];
    vectorProduct2(w, m_RY_R_delta, M0, M1, M2);

    const double vt0 = m_P1[0] - w[0];
    const double vt1 = m_P1[1] - w[1];
    const double vt2 = m_P1[2] - w[2];
#endif

    const double lam = m_v4 / (m_nS[0] * vt0 + m_nS[1] * vt1 + m_nS[2] * vt2);

    const double MSc0 = m_P2[0] + vt0 * lam - m_S[0];
    const double MSc1 = m_P2[1] + vt1 * lam - m_S[1];
    const double MSc2 = m_P2[2] + vt2 * lam - m_S[2];

    dstImgPoint[0] = round((m_T[0] * MSc0 + m_T[1] * MSc1 + m_T[2] * MSc2) /** m_iPx*/ + m_cx0);
    dstImgPoint[1] = round((m_T[3] * MSc0 + m_T[4] * MSc1 + m_T[5] * MSc2) /** m_iPx */ + m_cy0);
    dstImgPoint[2] = 0.0; // m_T[6] * MSc0 + m_T[7] * MSc1 + m_T[8] * MSc2;
}

#endif

void ScheimpflugTransform::objectToImage(const double* objX, const double* objY, float* imgX, float* imgY, int N,
                                         bool enableRounding /*= true*/)
{

    for (int n = 0; n < N; ++n)
    {
        double objX0 = objX[n];
        double objX1 = objY[n];

        // Assumes srcImgPoint[2] = v2 = 0
#if 1
        // FIMXE - BRT v7 - RZ(-H) instead of RZ(H)
        const double M0 = m_RZ_mH[0] * objX0 + m_RZ_mH[1] * objX1 /*+ m_RZ_mH[2] * srcImgPoint[2]*/;
        const double M1 = m_RZ_mH[3] * objX0 + m_RZ_mH[4] * objX1 /*+ m_RZ_mH[5] * srcImgPoint[2]*/;
        const double M2 = m_RZ_mH[6] * objX0 + m_RZ_mH[7] * objX1 /*+ m_RZ_mH[8] * srcImgPoint[2]*/;

        // Optimized code
        // const double vt0 = m_P1[0] - (M0 * m_v2 + m_v1);
        const double vt0 = m_P1[0] - (M0 * m_v2 - m_v1); // FIXME
        const double vt1 = m_P1[1] - M1;
        const double vt2 = m_P1[2] - M2;
#else
        // FIMXE - BRT v9
        double M0 = m_RZ_mH[0] * objX0 + m_RZ_mH[1] * objX1 /*+ m_RZ_mH[2] * srcImgPoint[2]*/;
        const double M1 = m_RZ_mH[3] * objX0 + m_RZ_mH[4] * objX1 /*+ m_RZ_mH[5] * srcImgPoint[2]*/;
        const double M2 = m_RZ_mH[6] * objX0 + m_RZ_mH[7] * objX1 /*+ m_RZ_mH[8] * srcImgPoint[2]*/;

        M0 = M0 * m_v2 + m_v1;
        double w[3];
        vectorProduct2(w, m_RY_R_delta, M0, M1, M2);
        const double vt0 = m_P1[0] - w[0];
        const double vt1 = m_P1[1] - w[1];
        const double vt2 = m_P1[2] - w[2];
#endif
        const double lam = m_v4 / (m_nS[0] * vt0 + m_nS[1] * vt1 + m_nS[2] * vt2);

        const double MSc0 = m_P2[0] + vt0 * lam - m_S[0];
        const double MSc1 = m_P2[1] + vt1 * lam - m_S[1];
        const double MSc2 = m_P2[2] + vt2 * lam - m_S[2];

        // double* MSp = dstImgPoint;

        // Assumes MSp[2] = 0

        double MSp0 = m_T[0] * MSc0 + m_T[1] * MSc1 + m_T[2] * MSc2;
        double MSp1 = m_T[3] * MSc0 + m_T[4] * MSc1 + m_T[5] * MSc2;
        // MSp[2] = 0.0;//m_T[6] * MSc0 + m_T[7] * MSc1 + m_T[8] * MSc2;

        // MSp0 *= m_iPx;
        // MSp1 *= m_iPx;

        // Correct center and convert to int.
        MSp0 += m_cx0;
        MSp1 += m_cy0;

        if (enableRounding)
        {
            MSp0 = round(MSp0);
            MSp1 = round(MSp1);
        }

        imgX[n] = (float)MSp0;
        imgY[n] = (float)MSp1;
    }
}

void ScheimpflugTransform::objectToImageWithRounding(const double* objX, const double* objY, float* imgX, float* imgY,
                                                     int N)
{
#if 0
    // FIXME - BRT v7
    const double m_RZ_H0 = m_RZ_H[0];
    const double m_RZ_H3 = m_RZ_H[3];
    const double m_RZ_H6 = m_RZ_H[6];
    const double m_RZ_H1 = m_RZ_H[1];
    const double m_RZ_H4 = m_RZ_H[4];
    const double m_RZ_H7 = m_RZ_H[7];
#else
    // FIXME - BRT v9
    const double m_RZ_mH0 = m_RZ_mH[0];
    const double m_RZ_mH3 = m_RZ_mH[3];
    const double m_RZ_mH6 = m_RZ_mH[6];
    const double m_RZ_mH1 = m_RZ_mH[1];
    const double m_RZ_mH4 = m_RZ_mH[4];
    const double m_RZ_mH7 = m_RZ_mH[7];
#endif

    for (int n = 0; n < N; ++n)
    {
        const double objX0 = objX[n];
        const double objX1 = objY[n];

        // Assumes srcImgPoint[2] = v2 = 0
#if 1
        // FIXME - BRT v7 - RZ(-H) instead of RZ(H)
        const double M0 = m_RZ_mH0 * objX0 + m_RZ_mH1 * objX1;
        // const double M1 = m_RZ_mH3 * objX0 + m_RZ_mH4 * objX1;
        // const double M2 = m_RZ_mH6 * objX0 + m_RZ_mH7 * objX1;

        // Optimized code
        // const double vt0 = m_P1[0] - (M0 * m_v2 + m_v1); // FIXME
        const double vt0 = m_P1[0] - (M0 * m_v2 - m_v1);
        // const double vt1 = m_P1[1] - M1;
        // const double vt2 = m_P1[2] - M2;
        const double vt1 = m_P1[1] - (m_RZ_mH3 * objX0 + m_RZ_mH4 * objX1);
        const double vt2 = m_P1[2] - (m_RZ_mH6 * objX0 + m_RZ_mH7 * objX1);
#else
        // FIXME - BRT v9
        double M0 = m_RZ_mH0 * objX0 + m_RZ_mH1 * objX1;
        const double M1 = m_RZ_mH3 * objX0 + m_RZ_mH4 * objX1;
        const double M2 = m_RZ_mH6 * objX0 + m_RZ_mH7 * objX1;

        // Optimized code
        M0 = M0 * m_v2 + m_v1;
        double w[3];
        vectorProduct2(w, m_RY_R_delta, M0, M1, M2);

        const double vt0 = m_P1[0] - w[0];
        const double vt1 = m_P1[1] - w[1];
        const double vt2 = m_P1[2] - w[2];
#endif

        const double lam = m_v4 / (m_nS[0] * vt0 + m_nS[1] * vt1 + m_nS[2] * vt2);

        const double MSc0 = m_P2[0] + vt0 * lam - m_S[0];
        const double MSc1 = m_P2[1] + vt1 * lam - m_S[1];
        const double MSc2 = m_P2[2] + vt2 * lam - m_S[2];

        imgX[n] = (float)round((m_T[0] * MSc0 + m_T[1] * MSc1 + m_T[2] * MSc2) /** m_iPx */ + m_cx0);
        imgY[n] = (float)round((m_T[3] * MSc0 + m_T[4] * MSc1 + m_T[5] * MSc2) /** m_iPx */ + m_cy0);
    }
}

#ifndef ENABLE_OPTIMIZATION

void ScheimpflugTransform::imageToObject(const double* srcImgPoint, double* dstObjPoint)
{
    double PScenter[3];
    PScenter[0] = m_cx0;
    PScenter[1] = m_cy0;
    PScenter[2] = 0.0;

    double PSens[3];
    PSens[0] = (srcImgPoint[0] - PScenter[0]) * m_px;
    PSens[1] = (srcImgPoint[1] - PScenter[1]) * m_px;
    PSens[2] = (srcImgPoint[2] - PScenter[2]) * m_px;

    double PSens2[3];
    vectorProduct(PSens2, m_RtfRab, PSens);
    PSens2[0] += m_S[0];
    PSens2[1] += m_S[1];
    PSens2[2] += m_S[2];

    double vt[3];
    vt[0] = PSens2[0] - m_P2[0];
    vt[1] = PSens2[1] - m_P2[1];
    vt[2] = PSens2[2] - m_P2[2];

    double norm = vt[0] * vt[0] + vt[1] * vt[1] + vt[2] * vt[2];

    if (norm > 0.0)
    {
        double iNorm = 1.0 / sqrt(norm);
        vt[0] *= iNorm;
        vt[1] *= iNorm;
        vt[2] *= iNorm;
    }

    // Intersection points onto the plane xy

#if 1
    // FIXME - BRT v7
    double lam = -m_P1[2] / vt[2];
#else
    // FIXME - BRT v9
    // lam = -(P1[2,0]+np.tan((R+delta)/180*np.pi)*P1[0,0])/(vt[2,n]+np.tan((R+delta)/180*np.pi)*vt[0,n])
    double lam = m_v6 / (vt[2] + m_v5 * vt[0]);
#endif
    double* PO = dstObjPoint;

    PO[0] = m_P1[0] + vt[0] * lam;
    PO[1] = m_P1[1] + vt[1] * lam;
    PO[2] = m_P1[2] + vt[2] * lam;

#if 1
    // FIXME - BRT v7

    // Projection
    // PO[0] = PO[0] * cos(m_R + m_delta) - m_dBC * sign(m_C[0]);
    // PO[0] = (PO[0] - m_dBC * sign(m_C[0])) * cos(m_R + m_delta); // FIXME
    PO[0] = (PO[0] + m_dBC * sign(m_C[0])) * cos(m_R + m_delta); // NEW FIXME

    // Rotation H
    double P_0_R[3];
    // vectorProduct(P_0_R, m_RZ_mH, PO);
    vectorProduct(P_0_R, m_RZ_H, PO); // FIXME: RZ(H) instead of RZ(-H)
    PO[0] = P_0_R[0];
    PO[1] = P_0_R[1];
    PO[2] = P_0_R[2];
#else
    // FIXME - BRT v10

    // PO = RY(-(R + delta)) @ PO # new
    double PO_new[3];
    vectorProduct(PO_new, m_RY_m_R_delta, PO);

    // Projection
    // PO[0] = (PO[0] - m_dBC * sign(m_C[0])) * cos(m_R + m_delta);
    PO_new[0] = (PO_new[0] - m_dBC * sign(m_C[0])) * cos(m_R + m_delta);

    // Rotation H
    double P_0_R[3];
    vectorProduct(P_0_R, m_RZ_H, PO_new);
    PO[0] = P_0_R[0];
    PO[1] = P_0_R[1];
    PO[2] = P_0_R[2];
#endif
}

#else

void ScheimpflugTransform::imageToObject(const double* srcImgPoint, double* dstObjPoint)
{
    // double PScenter[3];
    // PScenter[0] = m_cx0;
    // PScenter[1] = m_cy0;
    // PScenter[2] = 0.0;

    double PSens[3];
    // PSens[0] = (srcImgPoint[0] - PScenter[0]) * m_px;
    // PSens[1] = (srcImgPoint[1] - PScenter[1]) * m_px;
    // PSens[2] = (srcImgPoint[2] - PScenter[2]) * m_px;
    PSens[0] = (srcImgPoint[0] - m_cx0) * m_px;
    PSens[1] = (srcImgPoint[1] - m_cy0) * m_px;
    PSens[2] = srcImgPoint[2] * m_px;

    double PSens2[3];
    vectorProduct(PSens2, m_RtfRab, PSens);
    PSens2[0] += m_S[0];
    PSens2[1] += m_S[1];
    PSens2[2] += m_S[2];

    double vt[3];
    vt[0] = PSens2[0] - m_P2[0];
    vt[1] = PSens2[1] - m_P2[1];
    vt[2] = PSens2[2] - m_P2[2];

    // Optimization: it seems that normalization is not necessary
    /*double norm = vt[0] * vt[0] + vt[1] * vt[1] + vt[2] * vt[2];

    if(norm > 0.0)
    {
        double iNorm = 1.0 / sqrt(norm);
        vt[0] *= iNorm;
        vt[1] *= iNorm;
        vt[2] *= iNorm;
    }*/

    // Intersection points onto the plane xy

#if 1
    // FIXME - BRT v7
    double lam = -m_P1[2] / vt[2];
#else
    // FIXME - BRT v9
    // lam = -(P1[2,0]+np.tan((R+delta)/180*np.pi)*P1[0,0])/(vt[2,n]+np.tan((R+delta)/180*np.pi)*vt[0,n])
    double lam = m_v6 / (vt[2] + m_v5 * vt[0]);
#endif
    double* PO = dstObjPoint;

    PO[0] = m_P1[0] + vt[0] * lam;
    PO[1] = m_P1[1] + vt[1] * lam;
    PO[2] = m_P1[2] + vt[2] * lam;

#if 0
    // FIXME - BRT v9
    // PO = RY(-(R + delta)) @ PO # new
    double PO_new[3];
    vectorProduct(PO_new, m_RY_m_R_delta, PO);
#endif

    // Projection

    // Original code
    // PO[0] = PO[0] * cos(m_R + m_delta) - m_dBC * sign(m_C[0]);

#if 1
    // FIXME - BRT v7

    // Optimized code
    // PO[0] = PO[0] * m_v3 - m_v1;
    // PO[0] = (PO[0] - m_v1) * m_v3; // FIXME
    PO[0] = (PO[0] + m_v1) * m_v3; // NEW FIXME

    // Rotation H
    double P_0_R[3];
    // vectorProduct(P_0_R, m_RZ_mH, PO);
    vectorProduct(P_0_R, m_RZ_H, PO); // FIXME: RZ(H) instead of RZ(-H)
    PO[0] = P_0_R[0];
    PO[1] = P_0_R[1];
    PO[2] = P_0_R[2];
#else
    // FIXME - BRT v9

    // Optimized code
    // PO[0] = (PO[0] - m_v1) * m_v3;
    PO_new[0] = (PO_new[0] - m_v1) * m_v3;

    // Rotation H
    double P_0_R[3];
    vectorProduct(P_0_R, m_RZ_H, PO_new);
    PO[0] = P_0_R[0];
    PO[1] = P_0_R[1];
    PO[2] = P_0_R[2];
#endif
}

#endif

double ScheimpflugTransform::pixelToMm(double v)
{
    return v * m_px / fabs(m_magnification);
}

double ScheimpflugTransform::mmToPixel(double v)
{
    return v / m_px * fabs(m_magnification);
}

double ScheimpflugTransform::degToRad(double v)
{
    return v * 3.14159265 / 180.0;
}

double ScheimpflugTransform::radToDeg(double v)
{
    return v * 180.0 / 3.14159265;
}

double ScheimpflugTransform::sign(double v)
{
    if (v < 0.0)
        return -1.0;
    else if (v > 0.0)
        return 1.0;
    else
        return 0.0;
}

void ScheimpflugTransform::matrixProduct(double* C, const double* A, const double* B)
{
    C[0] = A[0] * B[0] + A[1] * B[3] + A[2] * B[6];
    C[1] = A[0] * B[1] + A[1] * B[4] + A[2] * B[7];
    C[2] = A[0] * B[2] + A[1] * B[5] + A[2] * B[8];

    C[3] = A[3] * B[0] + A[4] * B[3] + A[5] * B[6];
    C[4] = A[3] * B[1] + A[4] * B[4] + A[5] * B[7];
    C[5] = A[3] * B[2] + A[4] * B[5] + A[5] * B[8];

    C[6] = A[6] * B[0] + A[7] * B[3] + A[8] * B[6];
    C[7] = A[6] * B[1] + A[7] * B[4] + A[8] * B[7];
    C[8] = A[6] * B[2] + A[7] * B[5] + A[8] * B[8];
}

inline void ScheimpflugTransform::vectorProduct(double* w, const double* A, const double* v)
{
    w[0] = A[0] * v[0] + A[1] * v[1] + A[2] * v[2];
    w[1] = A[3] * v[0] + A[4] * v[1] + A[5] * v[2];
    w[2] = A[6] * v[0] + A[7] * v[1] + A[8] * v[2];
}

inline void ScheimpflugTransform::vectorProduct2(double* w, const double* A, const double& v0, const double& v1,
                                                 const double& v2)
{
    // double *Aptr = A;
    w[0] = A[0] * v0 + A[1] * v1 + A[2] * v2;
    w[1] = A[3] * v0 + A[4] * v1 + A[5] * v2;
    w[2] = A[6] * v0 + A[7] * v1 + A[8] * v2;
}

void ScheimpflugTransform::RX(double* R, double theta)
{
    const double ct = cos(theta);
    const double st = sin(theta);

    R[0] = 1.0;
    R[1] = 0.0;
    R[2] = 0.0;
    R[3] = 0.0;
    R[4] = ct;
    R[5] = -st;
    R[6] = 0.0;
    R[7] = st;
    R[8] = ct;
}

void ScheimpflugTransform::RY(double* R, double theta)
{
    const double ct = cos(theta);
    const double st = sin(theta);

    R[0] = ct;
    R[1] = 0.0;
    R[2] = st;
    R[3] = 0.0;
    R[4] = 1.0;
    R[5] = 0.0;
    R[6] = -st;
    R[7] = 0.0;
    R[8] = ct;
}

void ScheimpflugTransform::RZ(double* R, double theta)
{
    const double ct = cos(theta);
    const double st = sin(theta);

    R[0] = ct;
    R[1] = -st;
    R[2] = 0.0;
    R[3] = st;
    R[4] = ct;
    R[5] = 0.0;
    R[6] = 0.0;
    R[7] = 0.0;
    R[8] = 1.0;
}

void ScheimpflugTransform::transpose(double* T, double* M)
{
    T[0] = M[0];
    T[1] = M[3];
    T[2] = M[6];

    T[3] = M[1];
    T[4] = M[4];
    T[5] = M[7];

    T[6] = M[2];
    T[7] = M[5];
    T[8] = M[8];
}

void ScheimpflugTransform::printVector(double* v, char* name)
{
    log->debug(tag + name);
    stringstream s;
    s << tag << v[0] << " " << v[1] << " " << v[2];
    log->debug(s.str());
}

void ScheimpflugTransform::printMatrix(double* R, char* name)
{
    log->debug(tag + name);
    stringstream s;
    s << tag << R[0] << " " << R[1] << " " << R[2];
    log->debug(s.str());
    s.str("");
    s << tag << R[3] << " " << R[4] << " " << R[5];
    log->debug(s.str());
    s.str("");
    s << tag << R[6] << " " << R[7] << " " << R[8];
    log->debug(s.str());
}

double ScheimpflugTransform::getR()
{
    return radToDeg(-m_R);
}

double ScheimpflugTransform::getH()
{
    return radToDeg(m_H);
}

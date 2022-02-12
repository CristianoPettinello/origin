//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __SCHEIMPFLUG_TRANSFORM_H__
#define __SCHEIMPFLUG_TRANSFORM_H__

#include "Log.h"
#include "json/json.h"

using namespace Nidek::Libraries::Utils;

namespace Nidek {
namespace Libraries {
namespace HTT {

class ScheimpflugTransform
{
public:
    class Params
    {
    public:
        float tt;
        float theta;
        float alpha;
        float beta;
        float magnification;
        float delta;
        float cx0;
        float cy0;

        // FM2
        float phi;
        float p1;
        float p2;
    };

public:
    // Constructor
    ScheimpflugTransform();

    // Converts coordinates on the object plane to coordinates on the image plane
    void objectToImage(const double* srcObjPoint, double* dstImgPoint, bool enableRounding = true);
    void objectToImageWithRounding(const double* srcObjPoint, double* dstImgPoint);

    void objectToImage(const double* objX, const double* objY, float* imgX, float* imgY, int N,
                       bool enableRounding = true);
    void objectToImageWithRounding(const double* objX, const double* objY, float* imgX, float* imgY, int N);

    // Converts coordinates on the image plane to coordinates on the object plane
    void imageToObject(const double* srcImgPoint, double* dstObjPoint);

    // Set the HTT stylus orientation
    void setStylusOrientation(double R, double H);

    // Convert pixel to mm
    double pixelToMm(double v);

    // Convert mm to pixel
    double mmToPixel(double v);

    // Configure the transformation parameters and initialize the variables
    void init(Json::Value& params);
    void init(float* params);

    // Transform degrees to radians
    double degToRad(double v);

    // Transform radians to degrees
    double radToDeg(double v);

    // Get the R angle
    double getR();

    // Get the H angle
    double getH();

private:
    // Initialize the transform by precomputing all the required matrixes.
    void initializeScheimpflugModel();

    // Sign function
    inline double sign(double v);

    // Transpose a 3x3 matrix
    void transpose(double* T, double* M);

    // Rotations
    inline void RX(double* R, double theta);
    inline void RY(double* R, double theta);
    inline void RZ(double* R, double theta);

    // C = A * B (with C, B, A 3x3 matrixes)
    inline void matrixProduct(double* C, const double* A, const double* B);

    // w = A * v (A: 3x3 matrix, v and w 3x1 vectors)
    inline void vectorProduct(double* w, const double* A, const double* v);

    // w = A * v (A: 3x3 matrix, v and w 3x1 vectors)
    inline void vectorProduct2(double* w, const double* A, const double& v0, const double& v1, const double& v2);

    // Print out the elements of a 3x3 matrix
    void printMatrix(double* R, char* name);

    // Print out the elements of a 3x1 vector
    void printVector(double* v, char* name);

private:
    shared_ptr<Log> log;

    // Total track of the imaging path (axial length from the center of
    // the object space to the center of the imaging space) in mm.
    double m_tt;

    // Nodal point distance of the object space from the system origin C of the object plane
    double m_p1;

    // Nodal point distance of the imaging space from the system origin C of the object plane
    double m_p2;

    // Pixel size in mm
    double m_px;

    // Center of the optics
    int m_cx0;
    int m_cy0;

    double m_theta;
    double m_phi;

    double m_alpha;
    double m_beta;

    double m_magnification;

    double m_delta;

    // Stylus orientation in radians
    double m_R;
    double m_H;

    double m_RtfRab[9];
    double m_RZ_H[9];
    double m_RZ_mH[9];
    double m_RY_R[9];
    // double m_RY_mR[9];
    double m_RY_R_x_RY_delta[9];
    double m_RY_delta[9];

    double m_RY_R_delta[9];   // RY(R + delta)
    double m_RY_m_R_delta[9]; // RY(-(R + delta))

    double m_nS[3];
    double m_P1[3];
    double m_P2[3];
    double m_S[3];
    double m_C[3];
    double m_dBC;
    double m_T[9];

    double m_iPx;

    // precomputed values
    double m_v1;
    double m_v2;
    double m_v3;
    double m_v4;
    double m_v5;
    double m_v6;
};

} // namespace HTT
} // namespace Libraries
} // namespace Nidek

#endif // __SCHEIMPFLUG_TRANSFORM_H__

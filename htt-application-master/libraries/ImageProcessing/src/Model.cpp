//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include "Model.h"

#include <cstring>
#include <iostream>
#include <math.h>

using namespace Nidek::Libraries::HTT;
using namespace std;

// This is the maximum length of the profile. This value should depend on the
// size of the image. Currently, it should be 400. However, we set a bigger
// value.
#define MAX_PROFILE_LENGTH 1200

static const string tag = "[MODL] ";

Model::Model(shared_ptr<ScheimpflugTransform> st)
    : log(Log::getInstance()),
      m_st(st),
      m_lossField(nullptr),
      m_lossFieldWidth(0),
      m_lossFieldHeight(0),
      m_lossFieldSpan(0),
      m_lossFieldRowPtr(nullptr),
      m_profile(nullptr),
      m_bevel(nullptr),
      m_bevelX(nullptr),
      m_bevelY(nullptr),
      m_bevelN(0),
      m_leftBoundaryX(nullptr),
      m_intersectionCounter(0),
      m_checkIntersections(false),
      m_intersectionThreshold(0),
      m_debug(false)
{
    m_bevelX = new int[MAX_PROFILE_LENGTH];
    m_bevelY = new int[MAX_PROFILE_LENGTH];
}

Model::~Model()
{
    if (m_bevelX)
    {
        delete[] m_bevelX;
        m_bevelX = nullptr;
    }

    if (m_bevelY)
    {
        delete[] m_bevelY;
        m_bevelY = nullptr;
    }

    if (m_leftBoundaryX)
    {
        delete[] m_leftBoundaryX;
        m_leftBoundaryX = nullptr;
    }

    if (m_lossFieldRowPtr)
    {
        delete[] m_lossFieldRowPtr;
        m_lossFieldRowPtr = nullptr;
    }
}

shared_ptr<ScheimpflugTransform> Model::scheimpflugTransform()
{
    return m_st;
}

void Model::setLossFieldAndProfile(shared_ptr<Image<float>> lossField, shared_ptr<Profile<int>> profile,
                                   bool checkIntersections, int intersectionThreshold, vector<double>& weight)
{
    m_lossField = lossField;
    m_lossFieldWidth = lossField->getWidth();
    m_lossFieldHeight = lossField->getHeight();
    m_lossFieldSpan = lossField->getStride() / sizeof(float);

    m_weight.clear();
    m_weight = weight;

    // Optimization for fast access to lossField rows
    float* lossFieldPtr = lossField->getData();
    m_lossFieldRowPtr = new float*[m_lossFieldHeight];

    for (int y = 0; y < m_lossFieldHeight; ++y)
        m_lossFieldRowPtr[y] = &lossFieldPtr[y * m_lossFieldSpan];

    m_profile = profile;

    // This variable is used for enabling the check of intersections between
    // the profile and the bevel.
    m_checkIntersections = checkIntersections;
    m_intersectionThreshold = intersectionThreshold;

    // Precompute some quantities that are used for speeding-up the processing.
    const int imageHeight = m_lossField->getHeight();

    // For each row, m_leftBoundaryX contains the X position at which the profile
    // was possibly detected. This position is corrected with leftboundaryOffset
    // and during bevel displacement is used for checking when the bevel is intersecting
    // the profile. In rows where the profile was not detected, m_leftBoundaryX is set to 0.
    if (m_leftBoundaryX)
    {
        delete[] m_leftBoundaryX;
        m_leftBoundaryX = nullptr;
    }

    m_leftBoundaryX = new int[imageHeight];
    memset(m_leftBoundaryX, 0, sizeof(int) * imageHeight);

    int N = (int)profile->x.size();

    int* yPtr = profile->y.data();
    int* xPtr = profile->x.data();
    for (int n = N - 1; n >= 0; --n)
        m_leftBoundaryX[*yPtr++] = *xPtr++;
}

shared_ptr<Profile<int>> Model::getBevel()
{
    computeFinalBevelProfile();

    shared_ptr<Profile<int>> intBevel(new Profile<int>());
    auto N = m_bevelN;
    for (auto n = 0; n < N; ++n)
    {
        intBevel->x.push_back(m_bevelX[n]);
        intBevel->y.push_back(m_bevelY[n]);
    }

    // Save into the bevel profile also the model points onto the image plane
    N = (int)m_imgX.size();
    for (auto n = 0; n < N; ++n)
    {
        intBevel->imgX.push_back((int)m_imgX[n]);
        intBevel->imgY.push_back((int)m_imgY[n]);
    }

    N = (int)m_distanceImgX.size();
    for (auto n = 0; n < N; ++n)
    {
        intBevel->distanceImgX.push_back((int)m_distanceImgX[n]);
        intBevel->distanceImgY.push_back((int)m_distanceImgY[n]);
    }

    return intBevel;
}

bool Model::computeConstrainedBevelProfile()
{
    // The bevel profile is build by connecting the points
    // that define the bevel model onto the image plane
    // These points are contained in m_imgX and m_imgY

    // Counter counting how many pixels are intersecting the profile.
    int intersectionCounter = 0;
    bool valid = true;

    // Get pointers to profile coordinates and its boundary
    // int* xV = m_bevelX;   // Not required for the PSO
    // int* yV = m_bevelY;   // Not required for the PSO
    const int* leftBoundaryXPtr = m_leftBoundaryX;

    // m_imgX contains the X coordinates of the bevel model in the image plane.
    // Note: it is expected that the model is well defined and therefore m_imgX.size() > 1,
    //       i.e. it has at least one segment.
    const int N = (int)m_imgX.size() - 1;
    int cnt = 0;

    float lossValue = 0.0f;
    const float** lossFieldRowPtr = (const float**)m_lossFieldRowPtr;

    // Local variables for speeding up
    const int lossFieldWidth = m_lossFieldWidth;
    const int lossFieldHeight = m_lossFieldHeight;

    for (int n = 0; n < N; ++n)
    {
        // Consider a segment from (xi, yi) to (xj, yj)
        const float xi = m_imgX[n];
        const float yi = m_imgY[n];

        const float xj = m_imgX[n + 1];
        const float yj = m_imgY[n + 1];

        // float xj; // = m_imgX[n + 1];
        // float yj; // = m_imgY[n + 1];

        const float L = xj - xi; // must be xj > xi
        const float K = yj - yi;
        const float absL = fabs(L);
        const float absK = fabs(K);

        if (absL >= absK)
        {
            // Excursion on the X axis is greater than the one on the Y axis. The inner
            // loop connects (xi, yi) to (xj, yj) with a segment iterating on x.
            const float J = K / L;
            const int endX = (int)fabs(L);

            float Jx = yi;
            int absX = (int)xi;

            // Optimization for reducing the range of the for loop below.
            const int xinit = absX >= 0 ? 0 : -absX;
            const int xend = absX + endX < lossFieldWidth ? endX : lossFieldWidth - absX;
            absX += xinit;

            float segmentLoss = 0.0f;

            // for(int x = 0; x < endX ; ++x)
            for (int x = xinit; x < xend; ++x)
            {
                int y = (int)Jx;

                if (/*absX >= 0 && absX < lossFieldWidth && */ y >= 0 && y < lossFieldHeight)
                {
                    // Store the current segment point in m_bevelX and m_bevelY, then update the
                    // point counter.
                    //*xV++ = absX; // Not required for the PSO
                    //*yV++ = y;    // Not required for the PSO
                    ++cnt;

                    segmentLoss += (lossFieldRowPtr[y])[absX];

                    // Check if there is an interesection between the profile and the bevel.
                    if (leftBoundaryXPtr[y] >= absX)
                        ++intersectionCounter;
                }

                Jx += J; // Increment the new absolute value of y.
                ++absX;  // Increment the new absolute value of x.
            }

            lossValue += segmentLoss * (float)m_weight[n];
        } else
        {
            // Excursion on the Y axis is greater than the one on the X axis. The inner
            // loop connects (xi, yi) to (xj, yj) with a segment iterating on y.
            const float J = L / K;

            const int endY = static_cast<int>(fabs(K));

            float Jy = xi;

            int yyi = (int)yi;

            // Optimization for reducing the range of the for loop below.
            const int yinit = yyi >= 0 ? 0 : -yyi;
            const int yend = yyi + endY < lossFieldHeight ? endY : lossFieldHeight - yyi - 1;
            yyi += yinit; // Update yyi to the correct point
            // Update starting X coordinate relative to Y=0
            Jy += J * ((float)yinit);

            float segmentLoss = 0.0f;
            // for(int y = 0; y < endY; ++y)
            for (int y = yinit; y < yend; ++y)
            {
                int x = (int)Jy;

                if (x >= 0 && x < lossFieldWidth /*&& yyi >= 0 && yyi < lossFieldHeight*/)
                {
                    // Store the current segment point in m_bevelX and m_bevelY, then update the
                    // point counter.
                    //*xV++ = x;   // Not required for the PSO
                    //*yV++ = yyi; // Not required for the PSO
                    ++cnt;

                    segmentLoss += (lossFieldRowPtr[yyi])[x];

                    // Check if there is an interesection between the profile and the bevel.
                    if (leftBoundaryXPtr[yyi] >= x)
                        ++intersectionCounter;
                }

                Jy += J; // Increment the new absolute value of x
                ++yyi;   // Increment the new absolute value of y.
            }

            lossValue += segmentLoss * (float)m_weight[n];
        }
    }

    // Update m_bevelN with the actual number of points inserted in
    // m_bevelX and m_bevelY.
    m_bevelN = cnt;

    // Store the intersection counter.
    m_intersectionCounter = m_checkIntersections ? intersectionCounter : 0;

    // This code is compatible with the previous version but doesn not set valid=false inside
    // left boundary check. It is very likely that valid is not used anymore outside this class.
    if (m_intersectionCounter > m_intersectionThreshold)
        valid = false;

    int profileN = (int)m_profile->x.size();

    lossValue /= m_bevelN;
    if (profileN > m_bevelN)
        lossValue += (float)((profileN - m_bevelN));

    // Weights intersection points.
    m_mse = lossValue + m_intersectionCounter;

    return valid;
}

bool Model::computeConstrainedTBevelProfile()
{
    // The bevel profile is build by connecting the points
    // that define the bevel model onto the image plane
    // These points are contained in m_imgX and m_imgY

    // Counter counting how many pixels are intersecting the profile.
    int intersectionCounter = 0;
    bool valid = true;

    // Get pointers to profile coordinates and its boundary
    // int* xV = m_bevelX;   // Not required for the PSO
    // int* yV = m_bevelY;   // Not required for the PSO
    const int* leftBoundaryXPtr = m_leftBoundaryX;

    // m_imgX contains the X coordinates of the bevel model in the image plane.
    // Note: it is expected that the model is well defined and therefore m_imgX.size() > 1,
    //       i.e. it has at least one segment.
    const int N = (int)m_imgX.size() - 1;
    int cnt = 0;

    float lossValue = 0.0f;
    const float** lossFieldRowPtr = (const float**)m_lossFieldRowPtr;

    // Local variables for speeding up
    const int lossFieldWidth = m_lossFieldWidth;
    const int lossFieldHeight = m_lossFieldHeight;

    int offset = (int)(m_imgY[3] - m_imgY[4]);

    for (int n = 0; n < N; ++n)
    {
        // Consider a segment from (xi, yi) to (xj, yj)
        const float xi = m_imgX[n];
        const float yi = m_imgY[n];

        const float xj = m_imgX[n + 1];
        const float yj = m_imgY[n + 1];

        const float L = xj - xi; // must be xj > xi
        const float K = yj - yi;

        if (L >= K)
        {
            // Excursion on the X axis is greater than the one on the Y axis. The inner
            // loop connects (xi, yi) to (xj, yj) with a segment iterating on x.
            const float J = K / L;
            const int endX = (int)fabs(L);

            float Jx = yi;
            int absX = (int)xi;

            // Optimization for reducing the range of the for loop below.
            const int xinit = absX >= 0 ? 0 : -absX;
            const int xend = absX + endX < lossFieldWidth ? endX : lossFieldWidth - absX;
            absX += xinit;

            float segmentLoss = 0.0f;

            for (int x = xinit; x < xend; ++x)
            {
                int y = (int)Jx;

                if (y >= 0 && y < lossFieldHeight)
                {
                    // Store the current segment point in m_bevelX and m_bevelY, then update the
                    // point counter.
                    //*xV++ = absX; // Not required for the PSO
                    //*yV++ = y;    // Not required for the PSO
                    ++cnt;

                    // FIXME
                    if (n == 3)
                    {
                        /*int yRef = y - offset;
                        if (yRef < lossFieldHeight)
                        {
                            segmentLoss += (lossFieldRowPtr[yRef])[absX];
                            if (leftBoundaryXPtr[yRef] >= absX)
                                ++intersectionCounter;
                        }*/
                    }
                    /*  else if( n == 4)
                      {
                          float l = (lossFieldRowPtr[y])[absX];
                          if(l < 0.00001f)
                              l = 30.0f;

                          segmentLoss += l;//(lossFieldRowPtr[y])[absX];

                          if (leftBoundaryXPtr[y] >= absX)
                              ++intersectionCounter;
                      }*/
                    else
                    {
                        segmentLoss += (lossFieldRowPtr[y])[absX];

                        if (leftBoundaryXPtr[y] >= absX)
                            ++intersectionCounter;
                    }
                    // double srcImgPoint[] = {(double)lastImgPointProfile[0], (double)lastImgPointProfile[1], 0.0};
                    // double dstObjPoint[] = {0.0, 0.0, 0.0};
                    // m_st->imageToObject(srcImgPoint, dstObjPoint);
                }

                Jx += J; // Increment the new absolute value of y.
                ++absX;  // Increment the new absolute value of x.
            }

            lossValue += segmentLoss * (float)m_weight[n];
        } else
        {
            // Excursion on the Y axis is greater than the one on the X axis. The inner
            // loop connects (xi, yi) to (xj, yj) with a segment iterating on y.
            const float J = L / K;

            const int endY = static_cast<int>(fabs(K));

            float Jy = xi;

            int yyi = (int)yi;

            // Optimization for reducing the range of the for loop below.
            const int yinit = yyi >= 0 ? 0 : -yyi;
            const int yend = yyi + endY < lossFieldHeight ? endY : lossFieldHeight - yyi - 1;
            yyi += yinit; // Update yyi to the correct point

            float segmentLoss = 0.0f;
            // for(int y = 0; y < endY; ++y)
            for (int y = yinit; y < yend; ++y)
            {
                int x = (int)Jy;

                if (x >= 0 && x < lossFieldWidth /*&& yyi >= 0 && yyi < lossFieldHeight*/)
                {
                    // Store the current segment point in m_bevelX and m_bevelY, then update the
                    // point counter.
                    //*xV++ = x;   // Not required for the PSO
                    //*yV++ = yyi; // Not required for the PSO
                    ++cnt;

                    // segmentLoss += (lossFieldRowPtr[yyi])[x];

                    // Check if there is an interesection between the profile and the bevel.
                    // if (leftBoundaryXPtr[yyi] >= x)
                    //     ++intersectionCounter;

                    // FIXME
                    if (n == 3)
                    {
                        /*  int yRef = yyi - offset;
                          if (yRef < lossFieldHeight)
                          {
                              segmentLoss += (lossFieldRowPtr[yRef])[x];
                              if (leftBoundaryXPtr[yRef] >= x)
                                  ++intersectionCounter;
                          }*/
                    }
                    /* else if( n == 4)
                     {
                         float l = (lossFieldRowPtr[yyi])[x];
                         if(l < 0.00001f)
                             l = 30.0f;

                         segmentLoss += l;//(lossFieldRowPtr[y])[absX];

                         if (leftBoundaryXPtr[yyi] >= x)
                             ++intersectionCounter;
                     }*/
                    else if (n == 1)
                    {
                        int yRef = yyi;

                        // if(yyi > 200)
                        //    yRef -= offset;
                        // else
                        // yRef -= 20;
                        if (yRef < lossFieldHeight)
                        {
                            segmentLoss += (lossFieldRowPtr[yRef])[x];
                            if (leftBoundaryXPtr[yRef] >= x)
                                ++intersectionCounter;
                        }
                    } else if (n == 2)
                    {
                        int yRef = yyi;

                        if (yyi > 200)
                            yRef -= offset;
                        else
                            yRef += 0;

                        if (yRef < lossFieldHeight)
                        {
                            // FIXME: PATCH to avoid segmentation fault with
                            // LTT12-M805_pos=L110_H=+07.0_deltaN=+00.0_LED=0010_000.png
                            if (yRef < 0)
                                yRef = 0;
                            segmentLoss += (lossFieldRowPtr[yRef])[x];
                            if (leftBoundaryXPtr[yRef] >= x)
                                ++intersectionCounter;
                        }
                    } else
                    {
                        segmentLoss += (lossFieldRowPtr[yyi])[x];

                        if (leftBoundaryXPtr[yyi] >= x)
                            ++intersectionCounter;
                    }
                }

                Jy += J; // Increment the new absolute value of x
                ++yyi;   // Increment the new absolute value of y.
            }

            lossValue += segmentLoss * (float)m_weight[n];
        }
    }

    // Update m_bevelN with the actual number of points inserted in
    // m_bevelX and m_bevelY.
    m_bevelN = cnt;

    // Store the intersection counter.
    m_intersectionCounter = m_checkIntersections ? intersectionCounter : 0;

    // This code is compatible with the previous version but doesn not set valid=false inside
    // left boundary check. It is very likely that valid is not used anymore outside this class.
    if (m_intersectionCounter > m_intersectionThreshold)
        valid = false;

    int profileN = (int)m_profile->x.size();
    if (profileN > m_bevelN)
        lossValue += (float)((profileN - m_bevelN) * 1e3);

    // Weights intersection points.
    lossValue += m_intersectionCounter;
    m_mse = lossValue / N;

    return valid;
}

bool Model::computeFinalBevelProfile()
{
    // The bevel profile is build by connecting the points
    // that define the bevel model onto the image plane
    // These points are contained in m_imgX and m_imgY

    // Counter counting how many pixels are intersecting the profile.
    int intersectionCounter = 0;
    bool valid = true;

    // Get pointers to profile coordinates and its boundary
    int* xV = m_bevelX;
    int* yV = m_bevelY;
    const int* leftBoundaryXPtr = m_leftBoundaryX;

    // m_imgX contains the X coordinates of the bevel model in the image plane.
    // Note: it is expected that the model is well defined and therefore m_imgX.size() > 1,
    //       i.e. it has at least one segment.
    const int N = (int)m_imgX.size() - 1;
    int cnt = 0;

    float lossValue = 0.0f;
    const float** lossFieldRowPtr = (const float**)m_lossFieldRowPtr;

    for (int n = 0; n < N; ++n)
    {
        // Consider a segment from (xi, yi) to (xj, yj)
        const float xi = m_imgX[n];
        const float yi = m_imgY[n];

        const float xj = m_imgX[n + 1];
        const float yj = m_imgY[n + 1];

        const float L = xj - xi; // must be xj > xi
        const float K = yj - yi;

        const float absL = fabs(L);
        const float absK = fabs(K);

        // Local variables for speeding up
        const int lossFieldWidth = m_lossFieldWidth;
        const int lossFieldHeight = m_lossFieldHeight;

        if (absL >= absK)
        {
            // Excursion on the X axis is greater than the one on the Y axis. The inner
            // loop connects (xi, yi) to (xj, yj) with a segment iterating on x.
            const float J = K / L;
            const int endX = (int)fabs(L);

            float Jx = yi; // double type seems to be aster than float
            int absX = (int)xi;

            // Optimization for reducing the range of the for loop below.
            const int xinit = absX >= 0 ? 0 : -absX;
            const int xend = absX + endX < lossFieldWidth ? endX : lossFieldWidth - absX;
            absX += xinit;

            // for(int x = 0; x < endX ; ++x)
            for (int x = xinit; x < xend; ++x)
            {
                int y = (int)Jx;

                if (/*absX >= 0 && absX < lossFieldWidth && */ y >= 0 && y < lossFieldHeight)
                {
                    // Store the current segment point in m_bevelX and m_bevelY, then update the
                    // point counter.
                    *xV++ = absX;
                    *yV++ = y;
                    ++cnt;

                    lossValue += (lossFieldRowPtr[y])[absX];

                    // Check if there is an interesection between the profile and the bevel.
                    if (leftBoundaryXPtr[y] >= absX)
                        ++intersectionCounter;
                }

                Jx += J; // Increment the new absolute value of y.
                ++absX;  // Increment the new absolute value of x.
            }
        } else
        {
            // Excursion on the Y axis is greater than the one on the X axis. The inner
            // loop connects (xi, yi) to (xj, yj) with a segment iterating on y.
            const float J = L / K;
            const int endY = static_cast<int>(fabs(K));

            float Jy = xi;
            int yyi = (int)yi;

            // Optimization for reducing the range of the for loop below.
            const int yinit = yyi >= 0 ? 0 : -yyi;
            const int yend = yyi + endY < lossFieldHeight ? endY : lossFieldHeight - yyi - 1;
            yyi += yinit; // Update yyi to the correct point

            // Update starting X coordinate relative to Y=0
            Jy += J * ((float)yinit);

            // for(int y = 0; y < endY; ++y)
            for (int y = yinit; y < yend; ++y)
            {
                int x = (int)Jy;

                if (x >= 0 && x < lossFieldWidth /*&& yyi >= 0 && yyi < lossFieldHeight*/)
                {
                    // Store the current segment point in m_bevelX and m_bevelY, then update the
                    // point counter.
                    *xV++ = x;
                    *yV++ = yyi;
                    ++cnt;

                    lossValue += (lossFieldRowPtr[yyi])[x];

                    // Check if there is an interesection between the profile and the bevel.
                    if (leftBoundaryXPtr[yyi] >= x)
                        ++intersectionCounter;
                }

                Jy += J; // Increment the new absolute value of x
                ++yyi;   // Increment the new absolute value of y.
            }
        }
    }

    // Update m_bevelN with the actual number of points inserted in
    // m_bevelX and m_bevelY.
    m_bevelN = cnt;

    // Store the intersection counter.
    m_intersectionCounter = m_checkIntersections ? intersectionCounter : 0;

    // This code is compatible with the previous version but doesn not set valid=false inside
    // left boundary check. It is very likely that valid is not used anymore outside this class.
    if (m_intersectionCounter > m_intersectionThreshold)
        valid = false;

    int profileN = (int)m_profile->x.size();
    if (profileN > m_bevelN)
        lossValue += (float)((profileN - m_bevelN) * 1e3);

    // Weights intersection points.
    lossValue += m_intersectionCounter;
    m_mse = lossValue / N;

    return valid;
}

int Model::getIntersectionCounter()
{
    return m_intersectionCounter;
}

void Model::setDebug(bool debug)
{
    m_debug = debug;
}

void Model::recomputeImgPoints()
{
    computeImgPoints(m_objX, m_objY);
}

void Model::computeImgPoints(const vector<double>& objX, const vector<double>& objY)
{
    // Apply the Scheimpflug Transformation to each vertex in the model
    const int N = (int)objX.size();

    // ScheimpflugTransform* st = m_st.get();

    float* imgX = m_imgX.data();
    float* imgY = m_imgY.data();
    const double* objXPtr = objX.data();
    const double* objYPtr = objY.data();

    // m_st->objectToImage(objXPtr, objYPtr, imgX, imgY, N, true);
    m_st->objectToImageWithRounding(objXPtr, objYPtr, imgX, imgY, N);

    /* for(int n = 0; n < N; ++n)
     {
         // Z coord will be always zero.
         const double srcObjPoint[] = { objXPtr[n], objYPtr[n], 0.0};
         double dstImgPoint[] = { 0.0, 0.0, 0.0 };

         // We need to have points within the image boundaries.
         //m_st->objectToImage(srcObjPoint, dstImgPoint, true);
         st->objectToImage(srcObjPoint, dstImgPoint, true);

         // Store the points in the image plane
         //m_imgX[n] = (float)dstImgPoint[0];
         //m_imgY[n] = (float)dstImgPoint[1];
         imgX[n] = (float)dstImgPoint[0];
         imgY[n] = (float)dstImgPoint[1];
     }*/
}

void Model::computeObjPoints()
{
    m_objX.clear();
    m_objY.clear();

    const int N = (int)m_imgX.size();

    for (int n = 0; n < N; ++n)
    {
        // Z coord will be always zero.
        double srcImgPoint[] = {m_imgX[n], m_imgY[n], 0.0};
        double dstObjPoint[] = {0.0, 0.0, 0.0};

        // We need to have points within the image boundaries.
        m_st->imageToObject(srcImgPoint, dstObjPoint);

        float x = (float)dstObjPoint[0];
        float y = (float)dstObjPoint[1];

        // Store the points in the image plane
        m_objX.push_back(x);
        m_objY.push_back(y);
    }
}

float Model::mse()
{
    // This function requires that m_bevelX and m_bevelY have
    // values within the image boundaries. So, no additional check is
    // done.
    if (m_bevelN == 0)
        return 1e10;

    return m_mse;
}

void Model::printImgPoints()
{
    log->info(tag + "Image Points (pixels):");
    const auto L = m_imgX.size();
    stringstream s;
    s << tag << "\t";
    for (unsigned int i = 0; i < L; ++i)
        s << "(" << m_imgX[i] << "," << m_imgY[i] << ") ";
    log->info(s.str());
}

void Model::printObjPoints()
{
    log->info(tag + "Object Points (mm):");
    const auto L = m_objX.size();
    stringstream s;
    s << tag << "\t";
    for (unsigned int i = 0; i < L; ++i)
        s << "(" << m_objX[i] << "," << m_objY[i] << ") ";
    log->info(s.str());
}

void Model::printBevel()
{
    stringstream s;
    log->info(tag + "Bevel Points (mm):");
    for (unsigned int i = 0; i < m_bevel->x.size(); ++i)
    {
        s << tag << "\t" << m_bevel->x[i] << " " << m_bevel->y[i];
        log->info(tag + s.str());
        s.str("");
    }
}

void Model::objectToImage(double* srcObjPoint, double* dstImgPoint)
{
    m_st->objectToImage(srcObjPoint, dstImgPoint);
}

void Model::imageToObject(double* srcImgPoint, double* dstObjPoint)
{
    m_st->imageToObject(srcImgPoint, dstObjPoint);
}

double Model::pixelToMm(double v)
{
    return m_st->pixelToMm(v);
}

double Model::mmToPixel(double v)
{
    return m_st->mmToPixel(v);
}

double Model::getR()
{
    return m_st->getR();
}

double Model::getH()
{
    return m_st->getH();
}

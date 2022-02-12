//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __HOUGH_H__
#define __HOUGH_H__

#include "Image.h"
#include "Log.h"
#include <memory> // shared_ptr
#include <vector>

using namespace std;
using namespace Nidek::Libraries::Utils;

namespace Nidek {
namespace Libraries {
namespace HTT {
class Hough
{
private:
    shared_ptr<Log> m_log;
    bool m_debug;
    int m_srcImgWidth;
    int m_srcImgHeight;
    shared_ptr<Image<float>> m_houghImage;
    vector<pair<int, int>> m_peaks;

    vector<pair<pair<int, int>, pair<int, int>>> m_lines;
    // vector<pair<int, int>> m_intersectionPoints;
    double m_thetaStep;
    vector<double> m_theta;
    double m_rhoStep;
    vector<double> m_rho;

public:
    Hough(/* args */);
    ~Hough();

    shared_ptr<Image<float>> transform(shared_ptr<Image<float>> srcImage);
    void getPeaks(float threshold, int nPeaks);
    void adaptiveGetPeaks(float startingThreshold, int nPeaks);
    vector<pair<pair<int, int>, pair<int, int>>> getLines();
    void getIntersectionPoints(vector<int>& imgX, vector<int>& imgY);

private:
    void savePeak(float& cx, float& cy, int& r, int& t, float* pImage, const uint16_t& width, const uint16_t& height,
                  const uint16_t& stride);
};

} // namespace HTT
} // namespace Libraries
} // namespace Nidek
#endif // __HOUGH_H__
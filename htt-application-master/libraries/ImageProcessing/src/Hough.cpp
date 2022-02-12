#include "Hough.h"
#include "Settings.h"
#include "Utils.h"
#include <algorithm>
#include <iostream>
#include <math.h>
#include <time.h>

using namespace std;
using namespace Nidek::Libraries::HTT;

#define DEG2RAD 0.017453293f

static const string tag = "[HOUG] ";

bool distanceAbsCmp(pair<int, int> a, pair<int, int> b)
{
    return (abs(a.second) < abs(b.second));
}

bool distanceCmp(pair<int, int> a, pair<int, int> b)
{
    return (a.second > b.second);
}

Hough::Hough(/* args */) : m_log(Log::getInstance()), m_srcImgWidth(0), m_srcImgHeight(0), m_houghImage(nullptr)
{
    m_debug = Settings::getInstance()->getSettings()["scheimpflug_transformation_chart"]["debug"].asBool();
}

Hough::~Hough()
{
}

// FIXME: remove all comments not necessary

shared_ptr<Image<float>> Hough::transform(shared_ptr<Image<float>> srcImage)
{
    // get source image size
    float* pImage = srcImage->getData();
    m_srcImgWidth = srcImage->getWidth();
    m_srcImgHeight = srcImage->getHeight();
    const uint16_t stride = srcImage->getStride() / sizeof(float);

    int centerX = m_srcImgWidth * 0.5;
    int centerY = m_srcImgHeight * 0.5;

    srand((unsigned int)time(NULL));

    // TEST
    // cout << "Hough: Width: " << m_srcImgWidth << " " << m_srcImgHeight << endl;

    // TEST
    m_thetaStep = 0.2; // 0.33f;
    float theta = 15.0f;
    while (theta < 195.0f)
    {
        m_theta.push_back(theta);
        theta += m_thetaStep;
    }
    int maxTheta = m_theta.size();

    int doubleHeight = (int)(sqrt(2) * max(m_srcImgHeight, m_srcImgWidth));
    m_rhoStep = 0.2; // 0.33f;
    float rho = 0.0f;
    while (rho < doubleHeight)
    {
        m_rho.push_back(rho);
        rho += m_rhoStep;
    }
    int maxRho = m_rho.size();
    int houghHeight = maxRho * 0.5;

    // cout << "Hough: maxTheta: " << maxTheta << " maxRho: " << maxRho << endl;

    m_houghImage = make_shared<Image<float>>(Image<float>(maxTheta, maxRho, 1));
    float* houghMap = m_houghImage->getData();

    // scanning through each (x,y) pixel of the image
    for (int y = 0; y < m_srcImgHeight; ++y)
    {
        uint32_t startAddress = y * stride;
        for (int x = 0; x < m_srcImgWidth; ++x)
        {
            // if a pixel is black, skip it.
            if (pImage[x + startAddress] > 0.5)
            {
                // We are drawing some Sine waves.
                // It may vary from -90 to +90 degrees.
                // for (int theta = 0; theta < maxTheta; ++theta)
                for (int t = 0; t < maxTheta - 1; ++t)
                {
                    double theta = m_theta[t];
                    // respective radius value is computed
                    double rad = (float)theta * DEG2RAD;

                    // FIXME - controllare cosa succede!!!!
                    // int rho = (int)(((float)(x - centerX) * cos(rad)) + ((float)(y - centerY) * sin(rad)));
                    float rho = ((float)(x - centerX) * cos(rad)) + ((float)(y - centerY) * sin(rad));
                    // int rhoIndex = (int)(rho / m_rhoStep + 0.5);
                    int rhoIndex = (int)((rho + ((float)rand() / RAND_MAX)) / m_rhoStep);

                    // if (t == 500)
                    // {
                    //     cout << "x: " << x << " theta: " << theta << " cos(rad): " << cos(rad) << " y: " << y
                    //          << " sin(rad): " << sin(rad) << " index: " << rhoIndex << endl;
                    // }

                    // FIXME:
                    if (rhoIndex != 0)
                    {
                        // get rid of negative value
                        // rho += houghHeight;
                        rhoIndex += houghHeight;

                        // if the radious value is between
                        // 1 and twice the houghHeight
                        // if ((rho > 0) && (rho <= doubleHeight))
                        if ((rhoIndex >= 0) && (rhoIndex < maxRho))
                        {
                            // houghMap[theta + maxTheta * rho]++;
                            houghMap[t + maxTheta * rhoIndex]++;

                            // FIXME
                            // if (t == 500 && (rhoIndex >= 1585) && (rhoIndex < 1600))
                            // {
                            //     cout << "x: " << x << " theta: " << theta << " cos(rad): " << cos(rad) << " y: " << y
                            //          << " sin(rad): " << sin(rad) << " rho: " << rho << " index: " << rhoIndex <<
                            //          endl;
                            // }
                        }
                    }
                }
            }
        }
    }
    return m_houghImage;
}

// Driver function to sort the vector elements by second element of pairs
static bool sortbyX(const pair<pair<int, int>, pair<int, int>>& a, const pair<pair<int, int>, pair<int, int>>& b)
{
    return (a.first.first < b.first.first);
}
static bool sortbyY(const pair<pair<int, int>, pair<int, int>>& a, const pair<pair<int, int>, pair<int, int>>& b)
{
    return (a.first.second > b.first.second);
}

#if 0
void Hough::getPeaks(float threshold, int nPeaks)
{
    cout << "   <<<<<<<<<<<<<< getPeaks >>>>>>>>>>>>>>" << endl;
    float* pImage = m_houghImage->getData();
    const uint16_t width = m_houghImage->getWidth();
    const uint16_t height = m_houghImage->getHeight();
    const uint16_t stride = m_houghImage->getStride() / sizeof(float);

    int halfHeight = height >> 1;
    // For each cycle, the max number of peaks is equal to nPeaks / 4
    int maxPeaks = nPeaks * 0.25; // FIX THIS

    cout << "nPeaks " << nPeaks << endl;
    cout << "maxPeaks " << maxPeaks << endl;

    int p = 0;
    float avgDeltaY = 0;
    bool centralPeakFound = false;

    // rho (x axis) - theta (y axis)
    // limits the search space for the theta axis in these two bands [0-30]° and [75-105]°

    int stripeWidth = (int)(30.0 / m_thetaStep); // FIXME Hardcoded value

    // Theta: [0-30]° Rho: [0-height/2]
    for (int r = halfHeight; r >= 0; --r)
    {
        uint32_t startAddress = r * stride;
        for (int t = 1; t <= stripeWidth; ++t)
        {
            float* pPtr = &pImage[t + startAddress];
            if (*pPtr >= threshold)
            {
                if (p >= maxPeaks)
                {
                    avgDeltaY /= maxPeaks - 1;
                    // cout << "#### " << (fabs(m_peaks[0].second - halfHeight) * 0.5) << " " << avgDeltaY << endl;
                    // if ((fabs(m_peaksY[0] - halfHeight) * 0.5) < avgDeltaY)
                    if (fabs(m_peaks[0].second - halfHeight) < (avgDeltaY * 0.5))
                        centralPeakFound = true;
                    else
                    {
                        r = -1;
                        break;
                    }
                }

                float cx = 0.0f, cy = 0.0f;
                savePeak(cx, cy, r, t, pImage, width, height, stride);
                if (centralPeakFound)
                {
                    r = -1;
                    break;
                }

                if (p > 0)
                {
                    // cout << "   " << (float)abs(m_peaks[p].second - m_peaks[p - 1].second) << endl;
                    // avgDeltaY += (float)abs(m_peaksY[p] - m_peaksY[p - 1]);
                    avgDeltaY += (float)abs(m_peaks[p].second - m_peaks[p - 1].second);
                    // cout << ">> " << avgDeltaY << endl;
                }

                p++;
            }
        }
    }

    p = 0;
    int currMaxPeaks = centralPeakFound ? maxPeaks : maxPeaks + 1;

    // Theta: [0-30]° Rho: [height/2 - height]
    for (int r = halfHeight; r < height; ++r)
    {
        uint32_t startAddress = r * stride;
        for (int t = 1; t <= stripeWidth; ++t)
        {
            float* pPtr = &pImage[t + startAddress];
            if (*pPtr >= threshold)
            {
                if (p >= currMaxPeaks)
                {
                    if (centralPeakFound)
                    {
                        r = height;
                        break;
                    }
                    centralPeakFound = true;
                }

                float cx = 0.0f, cy = 0.0f;
                savePeak(cx, cy, r, t, pImage, width, height, stride);
                p++;
            }
        }
    }

    // Theta: [75-105]° Rho: [0 - height/2]
    cout << "AAAAAAAAAAAAAAAA" << endl;
    // Hard- coded values
    // int A = (int)(75.0 / m_thetaStep);
    // int B = (int)(105.0 / m_thetaStep);
    int A = (int)(27.0 / m_thetaStep);
    int B = (int)(75.0 / m_thetaStep);

    cout << "A: " << A << endl;
    cout << "B: " << B << endl;
    int index = maxPeaks * 2 + 1;
    centralPeakFound = false;
    avgDeltaY = 0;
    p = 0;
    for (int r = halfHeight; r >= 0; --r)
    {
        uint32_t startAddress = r * stride;
        for (int t = A; t <= B; ++t)
        {
            float* pPtr = &pImage[t + startAddress];
            if (*pPtr >= threshold)
            {
                cout << ".";
                if (p >= maxPeaks)
                {
                    avgDeltaY /= maxPeaks - 1;
                    // cout << "#### " << (fabs(m_peaks[index].second - halfHeight) * 0.5) << " " << avgDeltaY << endl;
                    // if ((fabs(m_peaksY[index] - halfHeight) * 0.5) < avgDeltaY)
                    // cout << m_peaks[index].second << " " << halfHeight << endl;
                    if (fabs(m_peaks[index].second - halfHeight) < (avgDeltaY * 0.5))
                        centralPeakFound = true;
                    else
                    {
                        r = -1;
                        break;
                    }
                }

                float cx = 0.0f, cy = 0.0f;
                savePeak(cx, cy, r, t, pImage, width, height, stride);
                if (centralPeakFound)
                {
                    r = -1;
                    break;
                }

                if (p > 0)
                {
                    // cout << "   " << (float)abs(m_peaks[index + p].second - m_peaks[index + p - 1].second) << endl;
                    // avgDeltaY += (float)abs(m_peaksY[index + p] - m_peaksY[index + p - 1]);
                    avgDeltaY += (float)abs(m_peaks[index + p].second - m_peaks[index + p - 1].second);
                    // cout << ">> " << avgDeltaY << " p:" << p << endl;
                }

                p++;
            }
        }
    }

    p = 0;
    currMaxPeaks = centralPeakFound ? maxPeaks : maxPeaks + 1;

    // Theta: [75-105]° Rho: [height/2 - height]
    for (int r = halfHeight; r < height; ++r)
    {
        uint32_t startAddress = r * stride;
        for (int t = A; t <= B; ++t)
        {
            float* pPtr = &pImage[t + startAddress];
            if (*pPtr >= threshold)
            {
                if (p >= currMaxPeaks)
                {
                    if (centralPeakFound)
                    {
                        r = height;
                        break;
                    }

                    centralPeakFound = true;
                }

                float cx = 0.0f, cy = 0.0f;
                savePeak(cx, cy, r, t, pImage, width, height, stride);
                ++p;
            }
        }
    }

    // cout << "Peaks: " << m_peaks.size() << endl;

    // cout << "########################################" << endl;
    // for (auto it = m_peaks.begin(); it < m_peaks.end(); ++it)
    // {
    //     cout << "(" << it->first << "," << it->second << ")" << endl;
    // }
    // cout << "########################################" << endl;

    // // sort vector
    // int length = nPeaks * 0.5;
    // sort(m_peaks.begin(), m_peaks.begin() + length - 1, sortbysec);
    // sort(m_peaks.begin() + length, m_peaks.end(), sortbysec);

    // cout << "########################################" << endl;
    // for (auto it = m_peaks.begin(); it < m_peaks.end(); ++it)
    // {
    //     cout << "   (" << it->first << "," << it->second << ")" << endl;
    // }
    // cout << "########################################" << endl;
}

#else

void Hough::getPeaks(float threshold, int nPeaks)
{
    // cout << "   <<<<<<<<<<<<<< getPeaks >>>>>>>>>>>>>>" << endl;
    float* pImage = m_houghImage->getData();
    const uint16_t width = m_houghImage->getWidth();
    const uint16_t height = m_houghImage->getHeight();
    const uint16_t stride = m_houghImage->getStride() / sizeof(float);

    int N = nPeaks >> 1;
    int halfHeight = height >> 1;
    // For each cycle, the max number of peaks is equal to nPeaks / 4

    // rho (x axis) - theta (y axis)
    // limits the search space for the theta axis in these two bands [0-90]° and [91-180]°

    vector<pair<int, int>> peak;
    vector<pair<int, int>> distance;

    int t1 = (int)((0.0) / m_thetaStep);
    int t2 = (int)((90.0) / m_thetaStep);

    int peakCounter = 0;
    for (int r = height - 1; r >= 0; --r)
    {
        int rowIndex = r * stride;
        for (int t = t1; t <= t2; ++t)
        {
            float* pPtr = &pImage[t + rowIndex];

            if (*pPtr >= threshold)
            {
                // cout << ">>>>> " << *pPtr << " r: " << r << " t: " << t << endl;
                float cx = 0.0f, cy = 0.0f;
                savePeak(cx, cy, r, t, pImage, width, height, stride);

                // int d = abs((int)cy - halfHeight);
                int d = (int)cy - halfHeight; // Save distance with sign
                peak.push_back(pair<int, int>((int)cx, (int)cy));
                distance.push_back(pair<int, int>(peakCounter++, d));
            }
        }
    }

    int peakSize = peak.size();

    // cout << "peaks.size: " << peakSize << endl;
    // cout << "HalfHeight:" << halfHeight << endl;
    // cout << "N: " << N << endl;

    M_Assert(peakSize >= N, "Few peaks found");

    // Sort using abs value in order to obtain first N or N+1 values
    sort(distance.begin(), distance.end(), distanceAbsCmp);

    int halfPoints = N >> 1;
    int upperPoints = halfPoints;
    int lowerPoints = halfPoints;

    if (peakSize > N)
    {
        // More peaks than necessary, so we sort the first N+1 distance values considering the sign
        sort(distance.begin(), distance.begin() + N + 1, distanceCmp);
        // for (int i = 0; i < N + 1; ++i)
        // {
        //    cout << i << ": d:" << distance[i].second << endl;
        // }

        // Find one of the two center points that has the minimum distance
        if (abs(distance[halfPoints].second) < abs(distance[halfPoints + 1].second))
        {
            if (distance[halfPoints].second < 0)
                upperPoints += 1;
            else
                lowerPoints += 1;
        } else
        {
            if (distance[halfPoints + 1].second < 0)
                upperPoints += 1;
            else
                lowerPoints += 1;
        }
    } else
    {
        sort(distance.begin(), distance.end(), distanceCmp);
        // for (int i = 0; i < N; ++i)
        // {
        //    cout << i << ": d:" << distance[i].second << endl;
        // }
        if (distance[halfPoints].second < 0)
            upperPoints += 1;
        else
            lowerPoints += 1;
    }

    // cout << "up: " << upperPoints << " down:" << lowerPoints << endl;

    // Remove from distance vector unnecessary lowerPoints values in order to obtain only the required number
    {
        int count = 0;
        vector<std::pair<int, int>>::iterator it = distance.begin();
        while (it->second >= 0)
        {
            ++count;
            ++it;
        }

        // cout << "count: " << count << endl;

        int len = count - lowerPoints;
        for (int i = 0; i < len; ++i)
            distance.erase(distance.begin() + i);

        // TODO: remove tests
        // cout << "-----" << endl;
        // for (int i = 0; i < N; ++i)
        // {
        //     cout << i << ": d:" << distance[i].second << endl;
        // }
    }

    int upperCount = 0;
    int lowerCount = 0;
    peakCounter = 0;
    for (int tt = 0; tt < distance.size(); ++tt)
    {
        int index = distance[tt].first;
        bool upperPeak = peak[index].second < halfHeight;

        if (upperPeak && upperCount < upperPoints)
        {
            // TODO: remove tests
            cout << "D: " << distance[tt].second << " P: " << peak[index].first << ", " << peak[index].second << " UP"
                 << endl;
            m_peaks.push_back(pair<int, int>(peak[index].first, peak[index].second));
            ++upperCount;
            ++peakCounter;
        }

        if (!upperPeak && lowerCount < lowerPoints)
        {
            // TODO: remove tests
            // cout << "D: " << distance[tt].second << " P: " << peak[index].first << ", " << peak[index].second
            //      << "  DOWN" << endl;
            m_peaks.push_back(pair<int, int>(peak[index].first, peak[index].second));
            ++lowerCount;
            ++peakCounter;
        }
        if (peakCounter >= N)
            break;
    }
    // -----------

    peak.clear();
    distance.clear();

    t1 = (int)((91.0) / m_thetaStep);
    t2 = (int)((180.0) / m_thetaStep);

    // TODO: remove tests
    // cout << "T1 " << t1 << endl;
    // cout << "T2 " << t2 << endl;

    peakCounter = 0;
    upperCount = 0;
    lowerCount = 0;
    for (int r = height - 1; r >= 0; --r)
    {
        int rowIndex = r * stride;
        for (int t = t1; t <= t2; ++t)
        {
            float* pPtr = &pImage[t + rowIndex];

            if (*pPtr >= threshold)
            {
                float cx = 0.0f, cy = 0.0f;
                savePeak(cx, cy, r, t, pImage, width, height, stride);

                // int d = abs((int)cy - halfHeight);
                int d = (int)cy - halfHeight;
                peak.push_back(pair<int, int>((int)cx, (int)cy));
                distance.push_back(pair<int, int>(peakCounter++, d));
                // cout << ">>>>> " << *pPtr << " r: " << r << " t: " << t << " c: " << peakCounter << " d: " << d <<
                // endl;
            }
        }
    }

    peakSize = peak.size();
    // cout << "peaks.size: " << peakSize << endl;s

    M_Assert(peakSize >= N, "Few peaks found");

    // Sort using abs value in order to obtain first N values
    sort(distance.begin(), distance.end(), distanceAbsCmp);

    upperPoints = halfPoints;
    lowerPoints = halfPoints;

    if (peakSize > N)
    {
        // More peaks than necessary, so we sort the first N+1 distance values considering the sign
        sort(distance.begin(), distance.begin() + N + 1, distanceCmp);
        // for (int i = 0; i < N + 1; ++i)
        // {
        //     cout << i << ": d:" << distance[i].second << endl;
        // }

        // Find one of the two center points that has the minimum distance
        if (abs(distance[halfPoints].second) < abs(distance[halfPoints + 1].second))
        {
            if (distance[halfPoints].second < 0)
                upperPoints += 1;
            else
                lowerPoints += 1;
        } else
        {
            if (distance[halfPoints + 1].second < 0)
                upperPoints += 1;
            else
                lowerPoints += 1;
        }
    } else
    {
        sort(distance.begin(), distance.end(), distanceCmp);
        // for (int i = 0; i < N; ++i)
        // {
        //     cout << i << ": d:" << distance[i].second << endl;
        // }
        if (distance[halfPoints].second < 0)
            upperPoints += 1;
        else
            lowerPoints += 1;
    }

    // cout << "up: " << upperPoints << " down:" << lowerPoints << endl;

    // Remove from distance vector unnecessary lowerPoints values in order to obtain only the required number
    {
        int count = 0;
        vector<std::pair<int, int>>::iterator it = distance.begin();
        while (it->second >= 0)
        {
            ++count;
            ++it;
        }

        // cout << "count: " << count << endl;

        int len = count - lowerPoints;
        for (int i = 0; i < len; ++i)
            distance.erase(distance.begin() + i);

        // cout << "-----" << endl;
        // for (int i = 0; i < N; ++i)
        // {
        //     cout << i << ": d:" << distance[i].second << endl;
        // }
    }

    peakCounter = 0;
    for (int tt = 0; tt < distance.size(); ++tt)
    {
        int index = distance[tt].first;
        bool upperPeak = peak[index].second < halfHeight;

        if (upperPeak && upperCount < upperPoints)
        {
            // cout << "D: " << distance[tt].second << " P: " << peak[index].first << ", " << peak[index].second << "
            // UP"
            //      << endl;
            m_peaks.push_back(pair<int, int>(peak[index].first, peak[index].second));
            ++upperCount;
            ++peakCounter;
        }

        if (!upperPeak && lowerCount < lowerPoints)
        {
            // cout << "D: " << distance[tt].second << " P: " << peak[index].first << ", " << peak[index].second
            //      << "  DOWN" << endl;
            m_peaks.push_back(pair<int, int>(peak[index].first, peak[index].second));
            ++lowerCount;
            ++peakCounter;
        }

        if (peakCounter >= N)
            break;
    }

    /*for(int tt = 0; tt < N; ++tt)
    {
        int index = distance[tt].first;
        cout << "D: " << distance[tt].second << " P: " << peak[index].first << ", " << peak[index].second << endl;
        // Get the first 9
        m_peaks.push_back(pair<int, int>(peak[index].first, peak[index].second));
    }*/

    for (int i = 0; i < m_peaks.size(); ++i)
    {
        int x = m_peaks[i].first;
        int y = m_peaks[i].second;

        pImage[x + y * stride] = 1.0;
    }
}

#endif

void Hough::adaptiveGetPeaks(float startingThreshold, int nPeaks)
{
    // cout << "<<<<<<<<<<<<<< getPeaks >>>>>>>>>>>>>>" << endl;
    float* pImage = m_houghImage->getData();
    const uint16_t width = m_houghImage->getWidth();
    const uint16_t height = m_houghImage->getHeight();
    const uint16_t stride = m_houghImage->getStride() / sizeof(float);

    int N = nPeaks >> 1;
    int halfHeight = height >> 1;
    int halfPoints = N >> 1;

    // cout << "N: " << N << endl;
    // cout << "halfHeight:" << halfHeight << endl;
    // cout << "halfPoints:" << halfPoints << endl;

    // rho (x axis) - theta (y axis)
    // limits the search space for the theta axis in these two bands [0-90]° and [91-180]°

    // Horizontal lines
    if (m_debug)
    {
        m_log->debug(tag + " --- Horizontal lines --- ");
    }

    vector<pair<int, int>> peak;
    vector<pair<int, int>> distance;

    int t1 = (int)((0.0) / m_thetaStep);
    int t2 = (int)((90.0) / m_thetaStep);

    int peakCounter = 0;
    int upperPoints;
    int lowerPoints;

    bool detectd = false;
    float threshold = startingThreshold;
    float step = 0.05;
    while (!detectd && threshold >= 0.1)
    {
        // cout << endl;
        // peakCounter = 0;
        for (int r = height - 1; r >= 0; --r)
        {
            int rowIndex = r * stride;
            for (int t = t1; t <= t2; ++t)
            {
                float* pPtr = &pImage[t + rowIndex];

                if (*pPtr >= threshold)
                {
                    float cx = 0.0f, cy = 0.0f;
                    savePeak(cx, cy, r, t, pImage, width, height, stride);

                    // int d = abs((int)cy - halfHeight);
                    int d = (int)cy - halfHeight; // Save distance with sign
                    peak.push_back(pair<int, int>((int)cx, (int)cy));
                    distance.push_back(pair<int, int>(peakCounter++, d));
                    if (m_debug)
                    {
                        stringstream ss;
                        ss << ">>>>> " << *pPtr << " r: " << r << " t: " << t << " c: " << peakCounter << " d: " << d;
                        m_log->debug(tag + ss.str());
                    }
                }
            }
        }
        if (m_debug)
        {
            stringstream ss;
            ss << "Threshold: " << threshold << " peakCounter: " << peakCounter;
            m_log->debug(tag + ss.str());
        }

        if (peakCounter < N)
        {
            threshold -= step;
            continue;
        }

        upperPoints = halfPoints;
        lowerPoints = halfPoints;

        // Sort using abs value in order to obtain first N or N+1 values
        sort(distance.begin(), distance.end(), distanceAbsCmp);

        if (peakCounter > N)
        {
            // More peaks than necessary, so we sort the first N+1 distance values considering the sign
            sort(distance.begin(), distance.begin() + N + 1, distanceCmp);
            if (m_debug)
            {
                stringstream ss;
                ss << peakCounter << " Peaks detected:" << endl;
                for (int i = 0; i < N + 1; ++i)
                {
                    ss << "\t" << i << ": d:" << distance[i].second << endl;
                }
                m_log->debug(tag + ss.str());
            }

            // Find one of the two center points that has the minimum distance
            if (abs(distance[halfPoints].second) < abs(distance[halfPoints + 1].second))
            {
                if (distance[halfPoints].second < 0)
                    upperPoints += 1;
                else
                    lowerPoints += 1;
            } else
            {
                if (distance[halfPoints + 1].second < 0)
                    upperPoints += 1;
                else
                    lowerPoints += 1;
            }
        } else
        {
            sort(distance.begin(), distance.end(), distanceCmp);
            if (m_debug)
            {
                stringstream ss;
                ss << peakCounter << " Peaks detected:" << endl;
                for (int i = 0; i < N; ++i)
                {
                    ss << "\t" << i << ": d:" << distance[i].second << endl;
                }
                m_log->debug(tag + ss.str());
            }
            if (distance[halfPoints].second < 0)
                upperPoints += 1;
            else
                lowerPoints += 1;
        }

        if (m_debug)
        {
            stringstream ss;
            ss << "up: " << upperPoints << " (-) down:" << lowerPoints << " (+)";
            m_log->debug(tag + ss.str());
        }

        int currUpperPoints = 0;
        int currLowerPoints = 0;

        for (vector<std::pair<int, int>>::iterator it = distance.begin(); it != distance.end(); ++it)
        {
            if (it->second >= 0)
                ++currLowerPoints;
            else
                ++currUpperPoints;
        }

        if (m_debug)
        {
            stringstream ss;
            ss << "currUpperPoints: " << currUpperPoints << " currLowerPoints: " << currLowerPoints;
            m_log->debug(tag + ss.str());
        }

        if (currLowerPoints < lowerPoints || currUpperPoints < upperPoints)
        {
            threshold -= step;
            continue;
        } else
        {
            // Remove from distance vector unnecessary lowerPoints values in order to obtain only the required
            // number
            int count = 0;
            vector<std::pair<int, int>>::iterator iter = distance.begin();
            while (iter->second >= 0)
            {
                ++count;
                ++iter;
            }
            int len = count - lowerPoints;
            for (int i = 0; i < len; ++i)
                // distance.erase(distance.begin() + i);
                // FIXME: NBr
                distance.erase(distance.begin());

            if (len > 0 && m_debug)
            {
                stringstream ss;
                ss << "Removed not necessary points:" << endl;
                for (int i = 0; i < N; ++i)
                {
                    ss << "\t" << i << ": d:" << distance[i].second << endl;
                }
                m_log->debug(tag + ss.str());
            }

            detectd = true;

            // if (m_debug)
            {
                stringstream ss;
                ss << ">>>> (H) Hough threshold detected: " << threshold << " <<<<";
                m_log->info(tag + ss.str());
            }
        }

        threshold -= step;
    }

    int upperCount = 0;
    int lowerCount = 0;
    peakCounter = 0;
    for (int tt = 0; tt < distance.size(); ++tt)
    {
        int index = distance[tt].first;
        bool upperPeak = peak[index].second < halfHeight;

        if (upperPeak && upperCount < upperPoints)
        {
            if (m_debug)
            {
                stringstream ss;
                ss << "D: " << distance[tt].second << " P: " << peak[index].first << ", " << peak[index].second
                   << " UP";
                m_log->debug(tag + ss.str());
            }
            m_peaks.push_back(pair<int, int>(peak[index].first, peak[index].second));
            ++upperCount;
            ++peakCounter;
        }

        if (!upperPeak && lowerCount < lowerPoints)
        {
            if (m_debug)
            {
                stringstream ss;
                ss << "D: " << distance[tt].second << " P: " << peak[index].first << ", " << peak[index].second
                   << " DOWN";
                m_log->debug(tag + ss.str());
            }
            m_peaks.push_back(pair<int, int>(peak[index].first, peak[index].second));
            ++lowerCount;
            ++peakCounter;
        }
        if (peakCounter >= N)
            break;
    }

    // Vertical lines
    if (m_debug)
    {
        m_log->debug(tag + " --- Vertical lines --- ");
    }

    peak.clear();
    distance.clear();

    t1 = (int)((91.0) / m_thetaStep);
    t2 = (int)((180.0) / m_thetaStep);

    peakCounter = 0;
    upperCount = 0;
    lowerCount = 0;

    upperPoints = halfPoints;
    lowerPoints = halfPoints;

    detectd = false;
    threshold = startingThreshold;
    // step = 0.05;
    while (!detectd && threshold >= 0.1)
    {
        // cout << endl;

        // peakCounter = 0;
        for (int r = height - 1; r >= 0; --r)
        {
            int rowIndex = r * stride;
            for (int t = t1; t <= t2; ++t)
            {
                float* pPtr = &pImage[t + rowIndex];

                if (*pPtr >= threshold)
                {
                    float cx = 0.0f, cy = 0.0f;
                    savePeak(cx, cy, r, t, pImage, width, height, stride);

                    // int d = abs((int)cy - halfHeight);
                    int d = (int)cy - halfHeight; // Save distance with sign
                    peak.push_back(pair<int, int>((int)cx, (int)cy));
                    distance.push_back(pair<int, int>(peakCounter++, d));
                    if (m_debug)
                    {
                        stringstream ss;
                        ss << ">>>>> " << *pPtr << " r: " << r << " t: " << t << " c: " << peakCounter << " d: " << d;
                        m_log->debug(tag + ss.str());
                    }
                }
            }
        }
        if (m_debug)
        {
            stringstream ss;
            ss << "Threshold: " << threshold << " peakCounter: " << peakCounter;
            m_log->debug(tag + ss.str());
        }

        if (peakCounter < N)
        {
            threshold -= step;
            continue;
        }

        upperPoints = halfPoints;
        lowerPoints = halfPoints;

        // Sort using abs value in order to obtain first N or N+1 values
        sort(distance.begin(), distance.end(), distanceAbsCmp);

        if (peakCounter > N)
        {
            // More peaks than necessary, so we sort the first N+1 distance values considering the sign
            sort(distance.begin(), distance.begin() + N + 1, distanceCmp);
            if (m_debug)
            {
                stringstream ss;
                ss << peakCounter << " Peaks detected:" << endl;
                for (int i = 0; i < N + 1; ++i)
                {
                    ss << "\t" << i << ": d:" << distance[i].second << endl;
                }
                m_log->debug(tag + ss.str());
            }

            // Find one of the two center points that has the minimum distance
            if (abs(distance[halfPoints].second) < abs(distance[halfPoints + 1].second))
            {
                if (distance[halfPoints].second < 0)
                    upperPoints += 1;
                else
                    lowerPoints += 1;
            } else
            {
                if (distance[halfPoints + 1].second < 0)
                    upperPoints += 1;
                else
                    lowerPoints += 1;
            }
        } else
        {
            sort(distance.begin(), distance.end(), distanceCmp);
            if (m_debug)
            {
                stringstream ss;
                ss << peakCounter << " Peaks detected:" << endl;
                for (int i = 0; i < N; ++i)
                {
                    ss << "\t" << i << ": d:" << distance[i].second << endl;
                }
                m_log->debug(tag + ss.str());
            }
            if (distance[halfPoints].second < 0)
                upperPoints += 1;
            else
                lowerPoints += 1;
        }

        if (m_debug)
        {
            stringstream ss;
            ss << "up: " << upperPoints << " (-) down:" << lowerPoints << " (+)";
            m_log->debug(tag + ss.str());
        }

        int currUpperPoints = 0;
        int currLowerPoints = 0;

        for (vector<std::pair<int, int>>::iterator it = distance.begin(); it != distance.end(); ++it)
        {
            if (it->second >= 0)
                ++currLowerPoints;
            else
                ++currUpperPoints;
        }

        if (m_debug)
        {
            stringstream ss;
            ss << "currUpperPoints: " << currUpperPoints << " currLowerPoints: " << currLowerPoints;
            m_log->debug(tag + ss.str());
        }

        if (currLowerPoints < lowerPoints || currUpperPoints < upperPoints)
        {
            threshold -= step;
            continue;
        } else
        {
            // Remove from distance vector unnecessary lowerPoints values in order to obtain only the required
            // number

            int count = 0;
            vector<std::pair<int, int>>::iterator iter = distance.begin();
            while (iter->second >= 0)
            {
                ++count;
                ++iter;
            }
            int len = count - lowerPoints;
            for (int i = 0; i < len; ++i)
                // distance.erase(distance.begin() + i);
                // FIXME: NBr
                distance.erase(distance.begin());

            if (len > 0 && m_debug)
            {
                stringstream ss;
                ss << "Removed not necessary points:" << endl;
                for (int i = 0; i < N; ++i)
                {
                    ss << "\t" << i << ": d:" << distance[i].second << endl;
                }
                m_log->debug(tag + ss.str());
            }

            detectd = true;

            // if (m_debug)
            {
                stringstream ss;
                ss << ">>>> (V) Hough threshold detected: " << threshold << " <<<<";
                m_log->info(tag + ss.str());
            }
        }

        threshold -= step;
    }

    peakCounter = 0;
    for (int tt = 0; tt < distance.size(); ++tt)
    {
        int index = distance[tt].first;
        bool upperPeak = peak[index].second < halfHeight;

        if (upperPeak && upperCount < upperPoints)
        {
            if (m_debug)
            {
                stringstream ss;
                ss << "D: " << distance[tt].second << " P: " << peak[index].first << ", " << peak[index].second
                   << " UP ";
                m_log->debug(tag + ss.str());
            }
            m_peaks.push_back(pair<int, int>(peak[index].first, peak[index].second));
            ++upperCount;
            ++peakCounter;
        }

        if (!upperPeak && lowerCount < lowerPoints)
        {
            if (m_debug)
            {
                stringstream ss;
                ss << "D: " << distance[tt].second << " P: " << peak[index].first << ", " << peak[index].second
                   << " DOWN ";
                m_log->debug(tag + ss.str());
            }
            m_peaks.push_back(pair<int, int>(peak[index].first, peak[index].second));
            ++lowerCount;
            ++peakCounter;
        }

        if (peakCounter >= N)
            break;
    }

    for (int i = 0; i < m_peaks.size(); ++i)
    {
        int x = m_peaks[i].first;
        int y = m_peaks[i].second;

        pImage[x + y * stride] = 1.0;
    }
}

vector<pair<pair<int, int>, pair<int, int>>> Hough::getLines()
{
    // cout << "   <<<<<<<<<<<<<< getLines >>>>>>>>>>>>>>" << endl;
    const uint16_t height = (int)((double)m_houghImage->getHeight() * m_rhoStep + 0.5);

    // for (int i = 0; i < (int)m_peaks.size(); ++i)
    for (auto it = m_peaks.begin(); it != m_peaks.end(); ++it)
    {
        // int cx = m_peaksX[i];
        // int cy = m_peaksY[i];
        int cx = it->first;
        int cy = (int)((double)it->second * m_rhoStep + 0.5);

        double theta = m_theta[cx]; // m_theta[cx] + 0.5;
        // cout << cx << " " << cy << " " << theta << endl;

        int x1, y1, x2, y2;
        x1 = y1 = x2 = y2 = 0;

        int minCX = (int)((45.0 - 15) / m_thetaStep);
        int maxCX = (int)((135.0 - 15) / m_thetaStep);

        if (cx >= minCX && cx <= maxCX)
        {
            // y = (r - x cos(t)) / sin(t)
            x1 = 1;
            y1 = (int)floor(((double)(cy - (height / 2)) - ((x1 - (m_srcImgWidth / 2)) * cos(theta * DEG2RAD))) /
                                sin(theta * DEG2RAD) +
                            (m_srcImgWidth / 2) + 0.5);
            if (y1 >= m_srcImgHeight)
            {
                // cout << "ORIG - x1:" << x1 << " y1:" << y1 << endl;
                y1 = m_srcImgHeight - 2;
                x1 = (int)floor(((double)(cy - (height / 2)) - ((y1 - (m_srcImgHeight / 2)) * sin(theta * DEG2RAD))) /
                                    cos(theta * DEG2RAD) +
                                (m_srcImgWidth / 2) + 0.5);
                // cout << "NEW - x1:" << x1 << " y1:" << y1 << endl;
            }

            x2 = m_srcImgWidth - 2;
            y2 = (int)floor(((double)(cy - (height / 2)) - ((x2 - (m_srcImgWidth / 2)) * cos(theta * DEG2RAD))) /
                                sin(theta * DEG2RAD) +
                            (m_srcImgWidth / 2) + 0.5);
            if (y2 <= 0)
            {
                // cout << "ORIG - x2:" << x2 << " y2:" << y2 << endl;
                y2 = 1;
                x2 = (int)floor(((double)(cy - (height / 2)) - ((y2 - (m_srcImgHeight / 2)) * sin(theta * DEG2RAD))) /
                                    cos(theta * DEG2RAD) +
                                (m_srcImgWidth / 2) + 0.5);
                // cout << "NEW - x2:" << x2 << " y2:" << y2 << endl;
            }
            // cout << cx << " " << cy << " " << theta << " A " << endl;
            // cout << "x1:" << x1 << " y1:" << y1 << " x2:" << x2 << " y2:" << y2 << endl;
        } else
        {
            // x = (r - y sin(t)) / cos(t);
            y1 = 1;
            x1 = (int)floor(((double)(cy - (height / 2)) - ((y1 - (m_srcImgHeight / 2)) * sin(theta * DEG2RAD))) /
                                cos(theta * DEG2RAD) +
                            (m_srcImgWidth / 2) + 0.5);
            y2 = m_srcImgWidth - 2;
            x2 = (int)floor(((double)(cy - (height / 2)) - ((y2 - (m_srcImgHeight / 2)) * sin(theta * DEG2RAD))) /
                                cos(theta * DEG2RAD) +
                            (m_srcImgWidth / 2) + 0.5);

            // cout << cx << " " << cy << " " << theta << " B " << endl;
            // cout << "x1:" << x1 << " y1:" << y1 << " x2:" << x2 << " y2:" << y2 << endl;
        }

        // y1 += 1;
        // y2 += 1;

        m_lines.push_back(pair<pair<int, int>, pair<int, int>>(pair<int, int>(x1, y1), pair<int, int>(x2, y2)));
    }

    // Sort lines
    int lines = m_peaks.size() * 0.5;
    sort(m_lines.begin(), m_lines.begin() + lines, sortbyY);
    sort(m_lines.begin() + lines, m_lines.end(), sortbyX);

    // int tmp = 0;
    // for (auto it = m_lines.begin(); it != m_lines.end(); ++it)
    //{
    //    cout << tmp++ << " (" << it->first.first << "," << it->first.second << ") (" << it->second.first << ","
    //         << it->second.second << ")" << endl;
    //}

    // cout << "Lines: " << m_lines.size() << endl;

    return m_lines;
}

void Hough::getIntersectionPoints(vector<int>& imgX, vector<int>& imgY)
{
    // TEST
    // cout << "   <<<<<<<<<<<<<< getIntersectionPoints >>>>>>>>>>>>>>" << endl;
    int nLines = (int)m_lines.size();
    int maxSize = nLines * 2 + 1;

    // cout << "nLines: " << nLines << endl;
    // cout << "maxSize: " << maxSize << endl;

    imgX.reserve(maxSize);
    imgY.reserve(maxSize);

    // Vertical   lines [0 - nLines/2-1]
    // Horizontal lines [nLines/2 - nLines]

    // Select the points of intersection with the first and last vertical line
    int startV = 0;
    int stopV = nLines * 0.5 - 1;
    int startH = stopV + 1;
    int stopH = nLines - 1;

    // cout << startV << " " << stopV << " " << startH << " " << stopH << endl;

    float xA = m_lines[startV].first.first;
    float yA = m_lines[startV].first.second;
    float xB = m_lines[startV].second.first;
    float yB = m_lines[startV].second.second;
    float m1 = (yB - yA) / (xB - xA);
    float q1 = yA - m1 * xA;
    // cout << "r" << startV << ": (" << xA << "," << yA << ") (" << xB << "," << yB << ") m1: " << m1 << " q1: " <<
    // q1
    //     << endl;

    xA = m_lines[stopV].first.first;
    yA = m_lines[stopV].first.second;
    xB = m_lines[stopV].second.first;
    yB = m_lines[stopV].second.second;
    float m2 = (yB - yA) / (xB - xA);
    float q2 = yA - m2 * xA;
    // cout << "r" << stopV << ": (" << xA << "," << yA << ")(" << xB << "," << yB << ") m2 : " << m2 << " q2: " <<
    // q2
    //     << endl;

    // cout << "--------- H LINES -------------" << endl;

#if 1
    for (int i = startH; i <= stopH; ++i)
    {

        float xC = m_lines[i].first.first;
        float yC = m_lines[i].first.second;
        float xD = m_lines[i].second.first;
        float yD = m_lines[i].second.second;
#if 0
        float m3 = (yD - yC) / (xD - xC);
        float q3 = yC - m3 * xC;
        //cout << "r" << i << ": (" << xC << "," << yC << ") (" << xD << "," << yD << ") m3: " << m3 << " q3: " << q3
        //     << endl;

        int x1 = (q1 - q3) / (m3 - m1);
        int y1 = m3 * x1 + q3;
        imgX.push_back(x1);
        imgY.push_back(y1);
        //cout << "   1:(" << x1 << "," << y1 << ")" << endl;

        int x2 = (q2 - q3) / (m3 - m2);
        int y2 = m3 * x2 + q3;
        imgX.push_back(x2);
        imgY.push_back(y2);
        //cout << "   2:(" << x2 << "," << y2 << ")" << endl;
#else
        // x = m * y + q
        float m3 = (xD - xC) / (yD - yC);
        float q3 = xC - m3 * yC;

        // cout << "A r" << i << ": (" << xC << "," << yC << ") (" << xD << "," << yD << ") m3: " << m3 << " q3: "
        // << q3
        //     << endl;

        float roundError = 0.0f;

        float y1f = (m1 * q3 + q1) / (1.0f - m1 * m3);
        int y1 = (int)floor(y1f + roundError);
        int x1 = (int)floor(m3 * y1f + q3 + roundError);

        float y2f = (m2 * q3 + q2) / (1.0f - m2 * m3);
        int y2 = (int)floor(y2f + roundError);
        int x2 = (int)floor(m3 * y2f + q3 + roundError);

        imgX.push_back(x1);
        imgY.push_back(y1);
        // cout << "   1:(" << x1 << "," << y1 << ")" << endl;

        imgX.push_back(x2);
        imgY.push_back(y2);
        // cout << "   2:(" << x2 << "," << y2 << ")" << endl;
#endif
    }
    // cout << "\n" << endl;
#endif

#if 1
    // Select the points of intersection with the first and last horizontal line
    // Consider the equation x = y * m + q
    xA = m_lines[startH].first.first;
    yA = m_lines[startH].first.second;
    xB = m_lines[startH].second.first;
    yB = m_lines[startH].second.second;

    m1 = (xB - xA) / (yB - yA);
    q1 = xA - m1 * yA;
    // cout << "r" << startH << ": (" << xA << "," << yA << ") (" << xB << "," << yB << ") m1: " << m1 << " q1: " <<
    // q1
    //     << endl;

    xA = m_lines[stopH].first.first;
    yA = m_lines[stopH].first.second;
    xB = m_lines[stopH].second.first;
    yB = m_lines[stopH].second.second;
    m2 = (xB - xA) / (yB - yA);
    q2 = xA - m2 * yA;
    // cout << "r" << stopH << ": (" << xA << "," << yA << ") (" << xB << "," << yB << ") m2: " << m2 << " q2: " <<
    // q2
    //     << endl;

    // cout << "--------- V LINES -------------" << endl;

    for (int i = startV; i <= stopV; ++i)
    {
        float xC = m_lines[i].first.first;
        float yC = m_lines[i].first.second;
        float xD = m_lines[i].second.first;
        float yD = m_lines[i].second.second;
        float m3 = (xD - xC) / (yD - yC);
        float q3 = xC - m3 * yC;

        // cout << "r" << i << ": (" << xC << "," << yC << ") (" << xD << "," << yD << ") m3: " << m3 << " q3: " <<
        // q3
        //     << endl;
        float roundError = 0.0f;
        float y1f = (q1 - q3) / (m3 - m1);
        int y1 = (int)floor(y1f + roundError);
        int x1 = (int)floor(m3 * y1f + q3 + roundError);

        imgX.push_back(x1);
        imgY.push_back(y1);
        // cout << "   1:(" << x1 << "," << y1 << ")" << endl;

        float y2f = (q2 - q3) / (m3 - m2);
        int y2 = (int)floor(y2f + roundError);
        int x2 = (int)floor(m3 * y2f + q3 + roundError);

        imgX.push_back(x2);
        imgY.push_back(y2);
        // cout << "   2:(" << x2 << "," << y2 << ")" << endl;
    }
    // cout << "\n" << endl;
#endif

    // FIXME this does not work

    // Select the central point
    int centerH = startH + (stopH - startH) * 0.5;
    xA = m_lines[centerH].first.first;
    yA = m_lines[centerH].first.second;
    xB = m_lines[centerH].second.first;
    yB = m_lines[centerH].second.second;
    // m1 = (yB - yA) / (xB - xA);
    // q1 = yA - m1 * xA;
    m1 = (xB - xA) / (yB - yA);
    q1 = xA - m1 * yA;
    // cout << "r" << centerH << ": (" << xA << "," << yA << ") (" << xB << "," << yB << ") m1: " << m1 << " q1: "
    // << q1
    //     << endl;

    int centerV = startV + (stopV - startV) * 0.5;
    xA = m_lines[centerV].first.first;
    yA = m_lines[centerV].first.second;
    xB = m_lines[centerV].second.first;
    yB = m_lines[centerV].second.second;
    // m2 = (yB - yA) / (xB - xA);
    // q2 = yA - m2 * xA;
    m2 = (xB - xA) / (yB - yA);
    q2 = xA - m2 * yA;
    // cout << "r" << centerV << ": (" << xA << "," << yA << ") (" << xB << "," << yB << ") m2: " << m2 << " q2: "
    // << q2
    //     << endl;

    float roundError = 0.0f;
    float y3f = (q2 - q1) / (m1 - m2);
    int y3 = (int)floor(y3f + roundError);
    int x3 = (int)floor(m1 * y3f + q1 + roundError);

    // int y3 = (m1 * q2 + q1) / (1 - m1 * m2);
    // int x3 = m2 * y3 + q2;

    imgX.push_back(x3);
    imgY.push_back(y3);
    // cout << "   1:(" << x3 << "," << y3 << ")" << endl;
    // cout << endl;

    // cout << "Intersection points: " << imgX.size() << endl;
}

void Hough::savePeak(float& cx, float& cy, int& r, int& t, float* pImage, const uint16_t& width, const uint16_t& height,
                     const uint16_t& stride)
{
    float count = 0.0f;

    // Max
    // float currMax = pImage[r * stride + t];
    // cx = t;
    // cy = r;

    // Consider a 10x20 window centered in r, t.
    for (int ly = -10; ly <= 10; ++ly)
    {
        for (int lx = -5; lx <= 5; ++lx)
        {
            // Check that the current pixel is not outside the image.
            if ((ly + r >= 0 && ly + r < height) && (lx + t >= 0 && lx + t < width))
            {
                float currPtr = pImage[((r + ly) * stride) + (t + lx)];

                // Max
                // if (currPtr > currMax)
                //{
                //    cx = t + lx;
                //    cy = r + ly;
                //    currMax = currPtr;
                //}

                // if (currPtr >)
                /*{
                    cx += (t + lx) * currPtr * currPtr;
                    cy += (r + ly) * currPtr * currPtr;
                    count += currPtr * currPtr;
                }*/

                // Update the centroid.
                cx += (t + lx) * currPtr;
                cy += (r + ly) * currPtr;
                count += currPtr;
            }
        }
    }

    // (cx, cy) is the computed centroid for the 8x8 window centered at r, t
    cx /= count;
    cy /= count;

    // // Compensate rounding to integer operations
    cx -= 0.5;
    cy -= 0.5;

    // m_peaks.push_back(pair<int, int>(cx, cy));

    // Mask
    int wx = (int)(8.0 / m_thetaStep);
    int wy = (int)(8.0 / m_rhoStep);
    for (int ly = -wy; ly <= wy; ++ly)
    {
        for (int lx = -wx; lx <= wx; ++lx)
        {
            if ((ly + cy >= 0 && ly + cy < height) && (lx + cx >= 0 && lx + cx < width))
                pImage[(((int)cy + ly) * stride) + (int)(cx + lx)] = 0;
        }
    }
}

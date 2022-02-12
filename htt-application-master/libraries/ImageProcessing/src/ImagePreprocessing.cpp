//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include <algorithm> /* min_element, max_element, sort */
#include <assert.h>  /* assert */
#include <cstring>
#include <iostream>
#include <math.h> /* sqrt */

#include "ImagePreprocessing.h"
#include "Settings.h"
#include "Utils.h"

using namespace Nidek::Libraries::HTT;

static const string tag = "[IMGP] ";

ImagePreprocessing::ImagePreprocessing() : m_log(Log::getInstance())
{
    m_debug = Settings::getInstance()->getSettings()["scheimpflug_transformation_chart"]["debug"].asBool();
}

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::getChannel(shared_ptr<Image<T>> srcImage, uint8_t selectedChannel)
{
    const uint8_t srcNChannles = srcImage->getNChannels();
    if (srcNChannles == 1)
    {
        return srcImage;
    } else
    {
        const uint16_t height = srcImage->getHeight();
        const uint16_t width = srcImage->getWidth();

        // Create single channel image
        shared_ptr<Image<T>> singleChannelImage(new Image<T>(width, height, 1));

        // Copy selected channel from srcImage into singleChannelImage
        const uint16_t strideSrcImage = srcImage->getStride() / sizeof(T);
        const uint16_t strideSingleChannelImage = singleChannelImage->getStride() / sizeof(T);

        auto pSrcImage = srcImage->getData();
        auto pSingleChannelImage = singleChannelImage->getData();

        for (uint16_t y = 0; y < height; ++y)
        {
            // uint32_t srcRowStartAddress = y * strideSrcImage;
            // uint32_t singleChannelRowStartAddress = y * strideSingleChannelImage;

            auto pSrcImageRow = &pSrcImage[selectedChannel + y * strideSrcImage];
            auto pSingleChannelImageRow = &pSingleChannelImage[y * strideSingleChannelImage];
            for (uint32_t x = 0; x < width; ++x)
            {
                // auto value = pSrcImage[(x * srcNChannles + selectedChannel) + srcRowStartAddress];
                // pSingleChannelImage[x + singleChannelRowStartAddress] = value;
                *pSingleChannelImageRow++ = *pSrcImageRow;
                pSrcImageRow += srcNChannles;
            }
        }

        return singleChannelImage;
    }
}

template shared_ptr<Image<uint8_t>> ImagePreprocessing::getChannel(shared_ptr<Image<uint8_t>> srcImage,
                                                                   uint8_t selectedChannel);
template shared_ptr<Image<float>> ImagePreprocessing::getChannel(shared_ptr<Image<float>> srcImage,
                                                                 uint8_t selectedChannel);

void ImagePreprocessing::normalize(shared_ptr<Image<float>> srcDstImage)
{
    float* ptr = srcDstImage->getData();
    int width = srcDstImage->getWidth();
    int height = srcDstImage->getHeight();
    int stride = srcDstImage->getStride() / sizeof(float);

    float minValue = 1e10, maxValue = -1e10;

    // Find min and maximum
    for (int y = 0; y < height; ++y)
    {
        float* rowPtr = &ptr[y * stride];
        for (int x = 0; x < width; ++x)
        {
            if (*rowPtr < minValue)
                minValue = *rowPtr;

            if (*rowPtr > maxValue)
                maxValue = *rowPtr;

            ++rowPtr;
        }
    }

    // cout << "Minvalue: " << minValue << endl;
    // cout << "Maxvalue: " << maxValue << endl;
    // cout << "Stride: " << stride << endl;

    // Normalize
    if (maxValue > minValue)
    {
        float K = 1.0f / (maxValue - minValue);

        for (int y = 0; y < height; ++y)
        {
            float* rowPtr = &ptr[y * stride];
            for (int x = 0; x < width; ++x)
            {
                *rowPtr = (*rowPtr - minValue) * K;
                ++rowPtr;
            }
        }
    }
}

void ImagePreprocessing::binarize(shared_ptr<Image<float>> srcDstImage, float threshold)
{
    float* ptr = srcDstImage->getData();
    int width = srcDstImage->getWidth();
    int height = srcDstImage->getHeight();
    int stride = srcDstImage->getStride() / sizeof(float);

    for (int y = 0; y < height; ++y)
    {
        float* rowPtr = &ptr[y * stride];
        for (int x = 0; x < width; ++x)
        {
            if (*rowPtr >= threshold)
                *rowPtr = 1.0f;
            else
                *rowPtr = 0.0f;

            ++rowPtr;
        }
    }
}

bool ImagePreprocessing::adaptiveBinarize(shared_ptr<Image<float>> srcDstImage, float startingThreshold)
{
    float* ptr = srcDstImage->getData();
    int width = srcDstImage->getWidth();
    int height = srcDstImage->getHeight();
    int stride = srcDstImage->getStride() / sizeof(float);

    float count = width * height;
    float threshold = startingThreshold;
    float step = 0.05;
    bool detected = false;
    while (!detected && threshold >= 0.1)
    {
        if (m_debug)
        {
            stringstream ss;
            ss << "Threshold: " << threshold;
            m_log->debug(tag + ss.str());
        }
        int whitePixels = 0;
        for (int y = 0; y < height; ++y)
        {
            float* rowPtr = &ptr[y * stride];
            for (int x = 0; x < width; ++x)
            {
                if (*rowPtr >= threshold)
                    ++whitePixels;

                ++rowPtr;
            }
        }

        // Check 0.65 <= m <= 0.15
        float m = (float)whitePixels / count;
        if (m_debug)
        {
            stringstream ss;
            ss << "mean: " << m;
            m_log->debug(tag + ss.str());
        }
        if (m >= 0.065 && m <= 0.200) // FIXME: 0.15 or 0.2
            detected = true;
        else
            threshold -= step;
    }

    if (!detected)
    {
        m_log->error(__FILE__, __LINE__, tag + "Failure on DoG threshold detection");
        return false;
    }

    m_log->info(tag + "DoG threshold detected " + to_string(threshold));

    for (int y = 0; y < height; ++y)
    {
        float* rowPtr = &ptr[y * stride];
        for (int x = 0; x < width; ++x)
        {
            if (*rowPtr >= threshold)
                *rowPtr = 1.0f;
            else
                *rowPtr = 0.0f;

            ++rowPtr;
        }
    }

    return true;
}

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::filterByRow(shared_ptr<Image<T>> srcImage, float kernel[3])
{
    assert(srcImage->getNChannels() == 1);

    uint16_t width = srcImage->getWidth();
    uint16_t height = srcImage->getHeight();
    uint16_t stride = srcImage->getStride() / sizeof(T);
    shared_ptr<Image<T>> filteredImage(new Image<T>(width, height, 1));
    T* pSrcImage = srcImage->getData();
    T* pFilteredImage = filteredImage->getData();

    /*
            for (uint32_t y = 0; y < height; ++y)
            {
                    uint32_t startAddress = y * stride;
                    for (uint16_t x = 1; x < width - 1; ++x)
                    {
                            float vp = pSrcImage[(x - 1) + startAddress] * kernel[0];
                            float vc = pSrcImage[x + startAddress] * kernel[1];
                            float vn = pSrcImage[(x + 1) + startAddress] * kernel[2];
                            pFilteredImage[x + startAddress] = (T)(vp + vc + vn);
                    }
            }
    */
    // ADG optimized code
    const int widthm1 = width - 1;
    for (uint32_t y = 0; y < height; ++y)
    {
        uint32_t startAddress = y * stride;

        T* vpPtr = &pSrcImage[startAddress];
        T* vcPtr = vpPtr + 1;
        T* vnPtr = vcPtr + 1;

        T* pFilteredImageRow = &pFilteredImage[startAddress];
        for (uint16_t x = 1; x < widthm1; ++x)
        {
            const float vp = *vpPtr * kernel[0];
            const float vc = *vcPtr * kernel[1];
            const float vn = *vnPtr * kernel[2];
            pFilteredImageRow[x] = (T)(vp + vc + vn);

            vpPtr = vcPtr;
            vcPtr = vnPtr;
            ++vnPtr;
        }
    }

    return filteredImage;
}
template shared_ptr<Image<uint8_t>> ImagePreprocessing::filterByRow(shared_ptr<Image<uint8_t>> srcImage,
                                                                    float kernel[3]);
template shared_ptr<Image<float>> ImagePreprocessing::filterByRow(shared_ptr<Image<float>> srcImage, float kernel[3]);

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::filterByRowR(shared_ptr<Image<T>> srcImage, uint8_t radius)
{
    assert(srcImage->getNChannels() == 1);
    float C = 1.0f / (float)radius;

    uint16_t width = srcImage->getWidth();
    uint16_t height = srcImage->getHeight();
    uint16_t stride = srcImage->getStride() / sizeof(T);
    shared_ptr<Image<T>> filteredImage(new Image<T>(width, height, 1));
    T* pSrcImage = srcImage->getData();
    T* pFilteredImage = filteredImage->getData();
    T** vPtr = new T*[radius];

    const int refWidth = width - radius / 2;

    for (uint32_t y = 0; y < height; ++y)
    {
        uint32_t startAddress = y * stride;
        vPtr[0] = &pSrcImage[startAddress];
        for (auto i = 1; i < radius; ++i)
            vPtr[i] = vPtr[i - 1] + 1;

        // for (uint16_t x = radius/2; x < width - radius/2; ++x)
        for (uint16_t x = radius / 2; x < refWidth; ++x)
        {
            float value = 0;
            for (auto j = 0; j < radius; ++j)
                value += *vPtr[j]++;
            pFilteredImage[x + startAddress] = (T)(value * C);
        }
    }

    if (vPtr)
    {
        delete[] vPtr;
        vPtr = nullptr;
    }

    return filteredImage;
}
template shared_ptr<Image<uint8_t>> ImagePreprocessing::filterByRowR(shared_ptr<Image<uint8_t>> srcImage,
                                                                     uint8_t radius);
template shared_ptr<Image<float>> ImagePreprocessing::filterByRowR(shared_ptr<Image<float>> srcImage, uint8_t radius);

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::filterByColumn(shared_ptr<Image<T>> srcImage, float kernel[3])
{
    assert(srcImage->getNChannels() == 1);

    uint16_t width = srcImage->getWidth();
    uint16_t height = srcImage->getHeight();
    uint16_t stride = srcImage->getStride() / sizeof(T);
    shared_ptr<Image<T>> filteredImage(new Image<T>(width, height, 1));
    T* pSrcImage = srcImage->getData();
    T* pFilteredImage = filteredImage->getData();

    /*
            for (uint32_t x = 0; x < width; ++x)
            {
                    for (uint16_t y = 1; y < height - 1; ++y)
                    {
                            uint32_t startAddress = (uint32_t)y * stride;
                            float vp = pSrcImage[x + (startAddress - stride)] * kernel[0];
                            float vc = pSrcImage[x + startAddress] * kernel[1];
                            float vn = pSrcImage[x + (startAddress + stride)] * kernel[2];
                            pFilteredImage[x + startAddress] = (T)(vp + vc + vn);
                    }
            }
    */

    const float kernel0 = kernel[0];
    const float kernel1 = kernel[1];
    const float kernel2 = kernel[2];

    // ADG optimized code
    for (uint32_t x = 0; x < width; ++x)
    {
        uint32_t startAddress = stride;
        const int heightm1 = height - 1;

        T* vpPtr = &pSrcImage[x + (startAddress - stride)];
        T* vcPtr = &pSrcImage[x + startAddress];
        T* vnPtr = &pSrcImage[x + (startAddress + stride)];

        T* pFilteredImagePtr = &pFilteredImage[x + startAddress];

        for (uint16_t y = 1; y < heightm1; ++y)
        {
            const float vp = *vpPtr * kernel0;
            const float vc = *vcPtr * kernel1;
            const float vn = *vnPtr * kernel2;
            *pFilteredImagePtr = (T)(vp + vc + vn);

            startAddress += stride;
            vpPtr = vcPtr;
            vcPtr = vnPtr;
            vnPtr += stride;
            pFilteredImagePtr += stride;
        }
    }

    return filteredImage;
}

template shared_ptr<Image<uint8_t>> ImagePreprocessing::filterByColumn(shared_ptr<Image<uint8_t>> srcImage,
                                                                       float kernel[3]);
template shared_ptr<Image<float>> ImagePreprocessing::filterByColumn(shared_ptr<Image<float>> srcImage,
                                                                     float kernel[3]);

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::filterByColumnR(shared_ptr<Image<T>> srcImage, uint8_t radius)
{
    assert(srcImage->getNChannels() == 1);

    uint16_t width = srcImage->getWidth();
    uint16_t height = srcImage->getHeight();
    uint16_t stride = srcImage->getStride() / sizeof(T);
    shared_ptr<Image<T>> filteredImage(new Image<T>(width, height, 1));
    T* pSrcImage = srcImage->getData();
    T* pFilteredImage = filteredImage->getData();
    T** vPtr = new T*[radius];

    const float C = 1.0f / (float)radius;
    const int radiusm1 = radius - 1;

    for (uint32_t x = 0; x < width; ++x)
    {
        uint32_t startAddress = 0;
        const int heightEnd = height - radius / 2;
        vPtr[0] = &pSrcImage[x + startAddress];
        for (auto i = 1; i < radius; ++i)
            vPtr[i] = vPtr[i - 1] + stride;

        // T* pFilteredImagePtr = &pFilteredImage[x + startAddress];

        for (uint16_t y = radius / 2; y < heightEnd; ++y)
        {
            float value = 0;
            for (auto j = 0; j < radius; ++j)
                value += *vPtr[j];

            // FIXME
            // pFilteredImage[x + startAddress] = (T)(value * C);
            pFilteredImage[x + startAddress + (radiusm1 / 2) * stride] = (T)(value * C); // NBr

            startAddress += stride;
            for (auto j = 0; j < radiusm1; ++j)
                vPtr[j] = vPtr[j + 1];

            vPtr[radius - 1] += stride;
            // pFilteredImagePtr += stride;
        }
    }

    if (vPtr)
    {
        delete[] vPtr;
        vPtr = nullptr;
    }

    return filteredImage;
}

template shared_ptr<Image<uint8_t>> ImagePreprocessing::filterByColumnR(shared_ptr<Image<uint8_t>> srcImage,
                                                                        uint8_t radius);
template shared_ptr<Image<float>> ImagePreprocessing::filterByColumnR(shared_ptr<Image<float>> srcImage,
                                                                      uint8_t radius);

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::filterGaussianByRow(shared_ptr<Image<T>> srcImage, const float* kernel,
                                                             int radius)
{
    int N = (radius - 1) >> 1;
    const uint16_t width = srcImage->getWidth();
    const uint16_t height = srcImage->getHeight();
    const uint16_t stride = srcImage->getStride() / sizeof(T);
    shared_ptr<Image<T>> G(new Image<T>(width, height, 1));

    T* pSrcImage = srcImage->getData();
    T* pG = G->getData();

    const int widthmN = width - N;
    for (uint32_t y = 0; y < height; ++y)
    {
        uint32_t startAddress = y * stride;
        T* vPtr = &pSrcImage[startAddress];
        T* pGPtr = &pG[startAddress];

        for (uint16_t x = 0; x < N; ++x)
        {
            float v = 0.0f;
            for (int i = -x; i <= N; ++i)
            {
                v += vPtr[x + i] * kernel[radius - 1 - i - N];
            }
            // Normalizes the part of kernel used during filtering
            pGPtr[x] = (T)(v * radius / (N + 1 + x));
        }

        for (uint16_t x = N; x < widthmN; ++x)
        {
            float v = 0.0f;
            for (int i = -N; i <= N; ++i)
            {
                v += vPtr[x + i] * kernel[radius - 1 - i - N];
            }

            pGPtr[x] = (T)(v);
        }

        for (uint16_t x = widthmN; x < width; ++x)
        {
            float v = 0.0f;
            int rightB = width - 1 - x;
            for (int i = -N; i <= rightB; ++i)
            {
                v += vPtr[x + i] * kernel[radius - 1 - i - N];
            }

            pGPtr[x] = (T)(v * radius / (N + 2 + rightB));
        }
    }

    return G;
}

template shared_ptr<Image<uint8_t>> ImagePreprocessing::filterGaussianByRow(shared_ptr<Image<uint8_t>> srcImage,
                                                                            const float* kernel, int radius);
template shared_ptr<Image<float>> ImagePreprocessing::filterGaussianByRow(shared_ptr<Image<float>> srcImage,
                                                                          const float* kernel, int radius);

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::filterGaussianByColumn(shared_ptr<Image<T>> srcImage, const float* kernel,
                                                                int radius)
{
    int N = (radius - 1) >> 1;
    const uint16_t width = srcImage->getWidth();
    const uint16_t height = srcImage->getHeight();
    const uint16_t stride = srcImage->getStride() / sizeof(T);
    shared_ptr<Image<T>> G(new Image<T>(width, height, 1));

    T* pSrcImage = srcImage->getData();
    T* pG = G->getData();

    const int heightmN = height - N;
    for (uint32_t x = 0; x < width; ++x)
    {
        T* vPtr = &pSrcImage[x];
        T* pGPtr = &pG[x];

        for (uint16_t y = 0; y < N; ++y)
        {
            float v = 0.0f;
            for (int i = -y; i <= N; ++i)
            {
                v += vPtr[(y + i) * stride] * kernel[radius - 1 - i - N];
            }
            // Normalizes the part of kernel used during filtering
            pGPtr[y * stride] = (T)(v * radius / (N + y + 1));
        }

        for (uint16_t y = N; y < heightmN; ++y)
        {
            float v = 0.0f;
            for (int i = -N; i <= N; ++i)
            {
                v += vPtr[(y + i) * stride] * kernel[radius - 1 - i - N];
            }
            pGPtr[y * stride] = (T)(v);
        }

        for (uint16_t y = heightmN; y < height; ++y)
        {
            float v = 0.0f;
            int bottomB = height - 1 - y;
            for (int i = -N; i <= bottomB; ++i)
            {
                v += vPtr[(y + i) * stride] * kernel[radius - 1 - i - N];
            }
            pGPtr[y * stride] = (T)(v * radius / (N + 2 + bottomB));
        }
    }

    return G;
}
template shared_ptr<Image<uint8_t>> ImagePreprocessing::filterGaussianByColumn(shared_ptr<Image<uint8_t>> srcImage,
                                                                               const float* kernel, int radius);
template shared_ptr<Image<float>> ImagePreprocessing::filterGaussianByColumn(shared_ptr<Image<float>> srcImage,
                                                                             const float* kernel, int radius);

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::lowPassFilter1(shared_ptr<Image<T>> srcImage, uint8_t radius)
{
    M_Assert(srcImage->getNChannels() == 1, "The number of channels of the input image is greather than one");
    shared_ptr<Image<T>> filByRow = this->filterByRowR(srcImage, radius);
    shared_ptr<Image<T>> filByRowAndColumn = this->filterByColumnR(filByRow, radius);

    return filByRowAndColumn;
}

template shared_ptr<Image<uint8_t>> ImagePreprocessing::lowPassFilter1(shared_ptr<Image<uint8_t>> srcImage,
                                                                       uint8_t radius);
template shared_ptr<Image<float>> ImagePreprocessing::lowPassFilter1(shared_ptr<Image<float>> srcImage, uint8_t radius);

template <typename T> shared_ptr<Image<T>> ImagePreprocessing::sobelFilter(shared_ptr<Image<T>> srcImage)
{
    assert(srcImage->getNChannels() == 1);

    float kernel_1[3] = {1, 0, -1}; // Kx_1 && Ky_2
    float kernel_2[3] = {1, 2, 1};  // Kx_2 && Ky_1

    shared_ptr<Image<T>> filByRow1 = this->filterByRow(srcImage, kernel_1);
    shared_ptr<Image<T>> sobelGx = this->filterByColumn(filByRow1, kernel_2);

    shared_ptr<Image<T>> filByRow2 = this->filterByRow(srcImage, kernel_2);
    shared_ptr<Image<T>> sobelGy = this->filterByColumn(filByRow2, kernel_1);

    uint16_t width = srcImage->getWidth();
    uint16_t height = srcImage->getHeight();
    uint16_t stride = srcImage->getStride() / sizeof(T);
    shared_ptr<Image<T>> G(new Image<T>(width, height, 1));
    T* pGx = sobelGx->getData();
    T* pGy = sobelGy->getData();
    T* pG = G->getData();

    for (uint16_t y = 0; y < height; ++y)
    {
        const uint32_t startAddress = y * stride;

        T* pGxPtr = &pGx[startAddress];
        T* pGyPtr = &pGy[startAddress];
        T* pGPtr = &pG[startAddress];

        for (uint16_t x = 0; x < stride; ++x)
        {
            // Original code
            // float gx = pGx[x + startAddress] * 0.33333f;
            // float gy = pGy[x + startAddress] * 0.33333f;
            // pG[x + startAddress] = (T)(sqrt((gx * gx) + (gy * gy)));

            // float gx = pGxPtr[x] * 0.33333f;
            // float gy = pGyPtr[x] * 0.33333f;
            const float gx = *pGxPtr++; // * 0.33333f;
            const float gy = *pGyPtr++; // * 0.33333f;

            pGPtr[x] = (T)sqrt((gx * gx + gy * gy) * 0.11111f);

            // TODO: If it will be necessary we could find the min and max of pG
            //      and calculate the normalized version as:
            //		pG* = (pG - min(pG)) /(max-min)
        }
    }

    return G;
}

template shared_ptr<Image<uint8_t>> ImagePreprocessing::sobelFilter(shared_ptr<Image<uint8_t>> srcImage);
template shared_ptr<Image<float>> ImagePreprocessing::sobelFilter(shared_ptr<Image<float>> srcImage);

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::gaussianFilter(shared_ptr<Image<T>> srcImage, float sigma, int radius)
{
    M_Assert(srcImage->getNChannels() == 1, " Source image must be a single channel image");

    int N = (radius - 1) >> 1;
    const float q = 2.0f * sigma * sigma;

    float p, sum = 0.0;
    float* kernel = new float[radius];

    // Generating 1-D kernel
    for (int i = -N; i <= N; ++i)
    {
        p = (float)(i * i);
        kernel[i + N] = exp(-p / q);
        sum += kernel[i + N];
    }

    // Normaliaze the kernel
    for (int i = 0; i < radius; ++i)
    {
        kernel[i] /= sum;
        // cout << kernel[i] << " ";
    }
    // cout << endl;

    shared_ptr<Image<T>> filByRow = filterGaussianByRow(srcImage, kernel, radius);
    shared_ptr<Image<T>> blur = filterGaussianByColumn(filByRow, kernel, radius);

    if (kernel)
        delete[] kernel;

    return blur;
}
template shared_ptr<Image<uint8_t>> ImagePreprocessing::gaussianFilter(shared_ptr<Image<uint8_t>> srcImage, float sigma,
                                                                       int N);
template shared_ptr<Image<float>> ImagePreprocessing::gaussianFilter(shared_ptr<Image<float>> srcImage, float sigma,
                                                                     int N);

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::differenceOfGaussian(shared_ptr<Image<T>> srcImage, float sigma1, float sigma2)
{

    int radius = 37; // ceil(6 * max(sigma1, sigma2));
    if (radius % 2 == 0)
        ++radius;

    shared_ptr<Image<T>> blur1 = gaussianFilter(srcImage, sigma1, radius);
    shared_ptr<Image<T>> blur2 = gaussianFilter(srcImage, sigma2, radius);

    // int N1 = ceil(6 * sigma1);
    // int N2 = ceil(6 * sigma2);
    // if (N1 % 2 == 0)
    //     ++N1;
    // if (N2 % 2 == 0)
    //     ++N2;
    // cout << "N1:" << N1 << " N2:" << N2 << endl;
    // shared_ptr<Image<T>> blur1 = gaussianFilter(srcImage, sigma1, N1);
    // shared_ptr<Image<T>> blur2 = gaussianFilter(srcImage, sigma2, N2);

    // Debug::saveImage(".", "blur1.png", blur1);
    // Debug::saveImage(".", "blur2.png", blur2);

    const uint16_t width = blur1->getWidth();
    const uint16_t height = blur1->getHeight();
    const uint16_t stride = blur1->getStride() / sizeof(T);
    shared_ptr<Image<T>> dog(new Image<T>(width, height, 1));

    // cout << "blur1: " << blur1->getWidth() << " " << blur1->getHeight() << " " << blur1->getStride() << endl;
    // cout << "blur2: " << blur2->getWidth() << " " << blur2->getHeight() << " " << blur2->getStride() << endl;
    // cout << "dog: " << dog->getWidth() << " " << dog->getHeight() << " " << dog->getStride() << endl;

    T* pDog = dog->getData();
    T* pBlur1 = blur1->getData();
    T* pBlur2 = blur2->getData();

    memset(pDog, 0, width * stride * sizeof(T));

    for (uint32_t y = 0; y < height; ++y)
    {
        uint32_t startAddress = y * stride;
        for (uint16_t x = 0; x < width; ++x)
        {
            uint32_t index = x + startAddress;
            T diff = pBlur1[index] - pBlur2[index];
            pDog[index] = diff;
        }
    }

    return dog;
}
template shared_ptr<Image<uint8_t>> ImagePreprocessing::differenceOfGaussian(shared_ptr<Image<uint8_t>> srcImage,
                                                                             float sigma1, float sigma2);
template shared_ptr<Image<float>> ImagePreprocessing::differenceOfGaussian(shared_ptr<Image<float>> srcImage,
                                                                           float sigma1, float sigma2);

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::filter(shared_ptr<Image<T>> srcImage, float* kernel, int nx, int ny,
                                                int roiX0 /* = - 1*/, int roiY0 /* = - 1*/, int roiWidth /* = - 1*/,
                                                int roiHeight /* = - 1*/)
{
    uint16_t width = srcImage->getWidth();
    uint16_t height = srcImage->getHeight();
    uint16_t srcStride = srcImage->getStride() / sizeof(T);
    T* pSrcImage = srcImage->getData();

    int offsetX = (nx - 1) >> 1;
    int offsetY = (ny - 1) >> 1;

    // Make a copy of the image by adding an external frame
    shared_ptr<Image<T>> newImage(new Image<T>(width + nx, height + ny, srcImage->getNChannels()));
    uint16_t newStride = newImage->getStride() / sizeof(T);
    T* pNewImage = newImage->getData();
    for (uint32_t y = 0; y < height; ++y)
    {
        uint32_t startAddressSrcImage = y * srcStride;
        uint32_t startAddressNewImage = (y + offsetY) * newStride;
        for (uint16_t x = 0; x < width; ++x)
        {
            pNewImage[(x + offsetX) + startAddressNewImage] = pSrcImage[x + startAddressSrcImage];
        }
    }
    // saveImage("results", "testFilter.png", newImage);

    shared_ptr<Image<T>> dstImage(new Image<T>(width, height, 1));
    uint16_t dstStride = dstImage->getStride() / sizeof(T);
    T* pDstImage = dstImage->getData();

    int leftX, rightX, topY, bottomY;

    if (roiHeight < 0 || roiWidth < 0 || roiX0 < 0 || roiY0 < 0)
    {
        leftX = 0;
        rightX = width;
        topY = 0;
        bottomY = height;
    } else
    {
        leftX = roiX0;

        if (leftX >= width - roiWidth)
            leftX = width - roiWidth;

        rightX = leftX + roiWidth;

        topY = roiY0;

        if (topY >= height - roiHeight)
            topY = height - roiHeight;

        bottomY = topY + roiHeight;

        // cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> leftX:" << leftX << " rightX:" << rightX << " topY:" << topY
        //      << " bottomY:" << bottomY << endl;
    }

    for (int y = topY; y < bottomY; ++y)
    {
        uint32_t startAddressNewImage = (y + offsetY) * newStride;
        uint32_t startAddressDstImage = y * dstStride;
        for (uint16_t x = leftX; x < rightX; ++x)
        {
            uint32_t srcIndex = (x + offsetX) + startAddressNewImage;
            T* ptrSrc = &pNewImage[srcIndex];
            uint32_t dstIndex = x + startAddressDstImage;
            T* ptrDst = &pDstImage[dstIndex];
            *ptrDst = 0;

            for (int yk = -offsetY; yk <= offsetY; ++yk)
            {
                for (int xk = -offsetX; xk <= offsetX; ++xk)
                {
                    *ptrDst +=
                        ptrSrc[xk + yk * newStride] * kernel[(nx - 1 - xk - offsetX) + (ny - 1 - yk - offsetY) * ny];
                }
            }
        }
    }
    return dstImage;
}
template shared_ptr<Image<uint8_t>> ImagePreprocessing::filter(shared_ptr<Image<uint8_t>> srcImage, float* kernel,
                                                               int nx, int ny, int roiX0 /* = - 1*/,
                                                               int roiY0 /* = - 1*/, int roiWidth /* = - 1*/,
                                                               int roiHeight /* = - 1*/);
template shared_ptr<Image<float>> ImagePreprocessing::filter(shared_ptr<Image<float>> srcImage, float* kernel, int nx,
                                                             int ny, int roiX0 /* = - 1*/, int roiY0 /* = - 1*/,
                                                             int roiWidth /* = - 1*/, int roiHeight /* = - 1*/);

template <typename T>
shared_ptr<Image<T>> ImagePreprocessing::medianBlurFilter(shared_ptr<Image<T>> srcImage, int radius)
{
    uint16_t width = srcImage->getWidth();
    uint16_t height = srcImage->getHeight();
    uint16_t srcStride = srcImage->getStride() / sizeof(T);
    T* pSrcImage = srcImage->getData();

    int offsetX = radius;
    int offsetY = radius;

    // Make a copy of the image by adding an external frame
    shared_ptr<Image<T>> newImage(
        new Image<T>(width + 2 * radius + 1, height + 2 * radius + 1, srcImage->getNChannels()));
    uint16_t newStride = newImage->getStride() / sizeof(T);
    T* pNewImage = newImage->getData();
    for (uint32_t y = 0; y < height; ++y)
    {
        uint32_t startAddressSrcImage = y * srcStride;
        uint32_t startAddressNewImage = (y + offsetY) * newStride;
        for (uint16_t x = 0; x < width; ++x)
        {
            pNewImage[(x + offsetX) + startAddressNewImage] = pSrcImage[x + startAddressSrcImage];
        }
    }

    shared_ptr<Image<T>> dstImage(new Image<T>(width, height, 1));
    uint16_t dstStride = dstImage->getStride() / sizeof(T);
    T* pDstImage = dstImage->getData();

    int size = ((radius << 1) + 1) * ((radius << 1) + 1);
    T* v = new T[size];
    // the size is always odd, so the center point is equal to (size-1)/2
    int centralIndex = (int)(size >> 1);

    for (uint32_t y = 0; y < height; ++y)
    {
        uint32_t startAddressNewImage = (y + offsetY) * newStride;
        uint32_t startAddressDstImage = y * dstStride;
        for (uint16_t x = 0; x < width; ++x)
        {
            uint32_t srcIndex = (x + offsetX) + startAddressNewImage;
            T* ptrSrc = &pNewImage[srcIndex];
            uint32_t dstIndex = x + startAddressDstImage;
            T* ptrDst = &pDstImage[dstIndex];

            int index = 0;
            for (int yk = -offsetY; yk <= offsetY; ++yk)
            {
                const int startAddress = yk * newStride;
                for (int xk = -offsetX; xk <= offsetX; ++xk)
                {
                    v[index++] = ptrSrc[xk + startAddress];
                }
            }

            sort(v, v + size);
            // v[centraIndex] = median(v)
            *ptrDst = v[centralIndex];
        }
    }

    if (v)
        delete[] v;

    return dstImage;
}
template shared_ptr<Image<uint8_t>> ImagePreprocessing::medianBlurFilter(shared_ptr<Image<uint8_t>> srcImage,
                                                                         int radius);
template shared_ptr<Image<float>> ImagePreprocessing::medianBlurFilter(shared_ptr<Image<float>> srcImage, int radius);

template <typename T> shared_ptr<Profile<int>> ImagePreprocessing::threshold(const ThresholdArgs<T>& args)
{
    uint16_t width = args.lowPassImage->getWidth();
    uint16_t height = args.lowPassImage->getHeight();
    uint16_t stride = args.lowPassImage->getStride() / sizeof(T);
    T* pLowPassImage = args.lowPassImage->getData();
    T* pSobelImage = args.sobelImage->getData();

    shared_ptr<Profile<int>> profile(new Profile<int>());
    profile->reserve(width);

    // uint16_t sobelHeight = args.sobelImage->getHeight();
    // uint16_t sobelWidth = args.sobelImage->getWidth();

    // cout << sobelWidth << endl;

    // For each row, we start from the right side of the image and
    // go backward. When the intensity is higher than the chosen
    // threshold, the right boundary rw is set.
    // The maximum value on the search interval [rw-search_width, rw]
    // is then used for determining the position of the maximum, along
    // that row. This is considered as a raw point of the profile on that
    // particular raw. Observe that if we set the search_width to zero,
    // then then we fall back onto the previous version of the thresholding
    // algorithm.

    T threshold = args.threshold;
    const int searchWidth = args.searchWidth;
    const int searchOffset = args.searchOffset;
    int upperBoundary = args.upperBoundary;
    int lowerBoundary = args.lowerBoundary;

    int leftBoundary = args.leftBoundary;
    int rightBoundary = args.rightBoundary;

    // The Sobel image is 2 pixels smaller both in width and height than the
    // lowPassImage.
    if (upperBoundary <= 0)
        upperBoundary = 1;

    // FIXME: bug!!!
    // if (lowerBoundary >= 0)
    if (lowerBoundary <= 0)
        lowerBoundary = 1;

    if (rightBoundary <= 0)
        rightBoundary = 1;

    if (leftBoundary <= 0)
        leftBoundary = 1;

    int maxIterations = args.maxIterations;
    const int profileOffset = args.profileOffset;

    int stylusMaskX1 = args.stylusMaskX1;
    int stylusMaskY1 = args.stylusMaskY1;
    int stylusMaskX2 = args.stylusMaskX2;
    int stylusMaskY2 = args.stylusMaskY2;
    float step = (float)threshold / 255;

    // Mask the stylus
    if (args.enableStylusMask)
    {
        // Check correctnes of values
        if (stylusMaskX1 >= 0 && stylusMaskX1 < width && stylusMaskX2 >= 0 && stylusMaskX2 < width &&
            stylusMaskY1 >= 0 && stylusMaskY1 < height && stylusMaskY2 >= 0 && stylusMaskY2 < height)
        {
            // Masking
            for (int h = stylusMaskY1; h <= stylusMaskY2; ++h)
            {
                int startAddress = h * stride;
                for (int w = stylusMaskX1; w <= stylusMaskX2; ++w)
                    pSobelImage[w + startAddress] = (T)0.0f;
            }
        }
    }

    // Find a reference position of the profile in the central part of the image
    int stylusOffset = args.referenceOffsetFromStylus;
    int leftBoundary2 = stylusMaskX1 - stylusOffset;

    int referenceX = 0;
    int counter = 0;

    int initH = (height >> 1) - abs(args.referenceHalfHeight);
    int endH = (height >> 1) + abs(args.referenceHalfHeight);

    if (initH < 0)
        initH = 0;

    if (endH > height)
        endH = height;

    for (uint16_t h = initH; h < endH; ++h)
    {
        uint32_t startAddress = h * stride;
        int rw = -1;
        int iteration = 1;
        float currThreshold = threshold;
        while (rw < 0 && iteration < maxIterations)
        {
            for (uint16_t w = stylusMaskX1; w >= leftBoundary2; --w)
            {
                T R = pSobelImage[w + startAddress];

                if (R > currThreshold)
                {
                    rw = w;
                    break;
                }
            }
            currThreshold -= step;
            ++iteration;
        }

        // Find the maximum within the given search range L
        if (rw >= 0)
        {
            rw += searchOffset;
            rw = (rw >= width) ? width - 1 : rw;
            int lw = rw - searchWidth;
            lw = (lw <= leftBoundary) ? leftBoundary : lw;
            T maxValue = pLowPassImage[lw + startAddress];
            int max_w = lw;
            for (int curRw = lw; curRw <= rw; ++curRw)
            {
                if (pLowPassImage[curRw + startAddress] > maxValue)
                {
                    maxValue = pLowPassImage[curRw + startAddress];
                    max_w = curRw;
                }
            }
            referenceX += max_w;
            ++counter;
        }
    }

    if (counter > 0)
        referenceX /= counter;

    // Thresholding
    vector<int> x, y;

    int refRw = referenceX - args.referenceWidth;
    for (uint16_t h = height >> 1; h >= upperBoundary; --h)
    {
        uint32_t startAddress = h * stride;
        int rw = -1;
        int iteration = 1;
        float currThreshold = threshold;
        // while (rw < 0 && iteration < maxIterations && rw < refRw)
        // cout << "refRw " << refRw << endl;
        while (iteration < maxIterations && rw < refRw)
        {
            for (int w = width - 1 - rightBoundary; w >= leftBoundary; --w)
            {
                T R = pSobelImage[w + startAddress];
                // cout << w << " " << h << endl;
                if (R > currThreshold)
                {
                    rw = w;
                    break;
                }
            }

            currThreshold -= step;
            ++iteration;
        }

        // Find the maximum within the given search range L
        if (rw >= refRw)
        {
            rw += searchOffset;
            rw = (rw >= width) ? width - 1 : rw;

            int lw = rw - searchWidth;
            lw = (lw <= leftBoundary) ? leftBoundary : lw;
            T maxValue = pLowPassImage[lw + startAddress];
            int max_w = lw;

            for (int curRw = lw; curRw <= rw; ++curRw)
            {
                if (pLowPassImage[curRw + startAddress] > maxValue)
                {
                    maxValue = pLowPassImage[curRw + startAddress];
                    max_w = curRw;
                }
            }
            refRw = max_w - args.referenceWidth;

            x.push_back(max_w + profileOffset);
            y.push_back(h);
        }
    }

    for (int k = (int)x.size() - 1; k >= 0; --k)
    {
        profile->x.push_back(x[k]);
        profile->y.push_back(y[k]);
    }

    // refRw = referenceX - 10;
    refRw = referenceX - args.referenceWidth;
    for (uint16_t h = (height >> 1) + 1; h < height - lowerBoundary; ++h)
    {
        uint32_t startAddress = h * stride;
        int rw = -1;
        int iteration = 1;
        float currThreshold = threshold;
        // float step = 1.0f / 255;
        while (iteration < maxIterations && rw < refRw)
        {
            for (uint16_t w = width - 1 - rightBoundary; w >= leftBoundary; --w)
            {
                T R = pSobelImage[w + startAddress];

                if (R > currThreshold)
                {
                    rw = w;
                    break;
                }
            }
            currThreshold -= step;
            ++iteration;
        }

        // Find the maximum within the given search range L
        if (rw >= refRw)
        {
            rw += searchOffset;
            rw = (rw >= width) ? width - 1 : rw;
            int lw = rw - searchWidth;
            lw = (lw <= leftBoundary) ? leftBoundary : lw;
            T maxValue = pLowPassImage[lw + startAddress];
            int max_w = lw;
            for (int curRw = lw; curRw <= rw; ++curRw)
            {
                if (pLowPassImage[curRw + startAddress] > maxValue)
                {
                    maxValue = pLowPassImage[curRw + startAddress];
                    max_w = curRw;
                }
            }
            refRw = max_w - args.referenceWidth;
            profile->x.push_back(max_w + profileOffset);
            profile->y.push_back(h);
        }
    }

    return profile;
}

template shared_ptr<Profile<int>> ImagePreprocessing::threshold(const ThresholdArgs<uint8_t>& args);
template shared_ptr<Profile<int>> ImagePreprocessing::threshold(const ThresholdArgs<float>& args);

template <typename T> shared_ptr<Profile<int>> ImagePreprocessing::thresholdTest(shared_ptr<Image<T>> inputImage)
{
    uint16_t width = inputImage->getWidth();
    uint16_t height = inputImage->getHeight();
    uint16_t stride = inputImage->getStride() / sizeof(T);
    T* pImage = inputImage->getData();

    shared_ptr<Profile<int>> profile(new Profile<int>());
    profile->reserve(width);

    for (uint16_t y = 0; y < height; ++y)
    {
        uint32_t startAddress = y * stride;
        for (int x = width; x > 0; --x)
        {
            if (pImage[x + startAddress] > 0)
            {
                profile->x.push_back(x);
                profile->y.push_back(y);
                break;
            }
        }
    }

    return profile;
}
template shared_ptr<Profile<int>> ImagePreprocessing::thresholdTest(shared_ptr<Image<uint8_t>> inputImage);
template shared_ptr<Profile<int>> ImagePreprocessing::thresholdTest(shared_ptr<Image<float>> inputImage);

shared_ptr<Profile<int>> ImagePreprocessing::removeIsolatedPoints(shared_ptr<Profile<int>> srcPath,
                                                                  float distanceThreshold, int windowSize,
                                                                  int counterThreshold)
{
    int L = (int)srcPath->x.size();
    shared_ptr<Profile<int>> profile(new Profile<int>());
    profile->reserve(L);
    float distanceThresholdSquared = distanceThreshold * distanceThreshold;

    for (int l = 0; l < L; ++l)
    {
        int counter = 0;
        int currentX = srcPath->x[l];
        int currentY = srcPath->y[l];

        // The current value of x in l is compared with values
        // in an interval [-window_size, window_size]. We count the
        // number of elements that are within distance_threshold.
        for (int w = -windowSize; w <= windowSize; ++w)
        {
            int t = w + l;
            if (t >= 0 && t < L)
            {
                // distance = abs(float(x[t] - current_x))
                const float dx = float(srcPath->x[t] - currentX);
                const float dy = float(srcPath->y[t] - currentY);
                float distance = dx * dx + dy * dy;
                if (distance < distanceThresholdSquared)
                    counter += 1;
            }
        }

        if (counter >= counterThreshold)
        {
            profile->x.push_back(currentX);
            profile->y.push_back(currentY);
        }
    }

    return profile;
}

shared_ptr<Profile<int>> ImagePreprocessing::filterProfileWithInterpolation(shared_ptr<Profile<int>> profile, int W)
{
    const int N = (int)profile->x.size();

    shared_ptr<Profile<int>> ret(new Profile<int>());

    for (int n = 0; n < N; ++n)
    {
        int cnt = 0;
        int sumX = 0;
        int sumY = 0;

        const int initJ = n - W;
        const int endJ = n + W;

        // for(int j = n - W; j <= n + W; ++j)
        for (int j = initJ; j <= endJ; ++j)
        {
            if (j >= 0 && j < N)
            {
                ++cnt;
                sumX += profile->x[j];
                sumY += profile->y[j];
            }
        }

        int valueX = (int)((float)sumX / (float)cnt);
        int valueY = (int)((float)sumY / (float)cnt);
        ret->x.push_back(valueX);
        ret->y.push_back(valueY);
    }

    return ret;
}

shared_ptr<Profile<int>> ImagePreprocessing::filterProfile(shared_ptr<Profile<int>> profile, int W,
                                                           const int& maxHeight)
{
    vector<int> v;
    v.assign(maxHeight, 0);

    const int N = (int)profile->x.size();

    for (int n = 0; n < N; ++n)
    {
        v[profile->y[n]] = 1;
    }

    shared_ptr<Profile<int>> ret(new Profile<int>());

    for (int n = 0; n < N; ++n)
    {

        int sumX = 0;
        int sumY = 0;

        const int initJ = n - W;
        const int endJ = n + W;

        int currentX = profile->x[n];
        float sumW = 0;

        for (int j = initJ; j <= endJ; ++j)
        {
            if (j >= 0 && j < N)
            {
                float d = (float)fabs((double)(currentX - profile->x[j]) / 20.0);
                float w = 1.0f - d;
                if (w < 0)
                    w = 0;
                sumW += w;
                sumX += (int)(profile->x[j] * w);
                sumY += (int)(profile->y[j] * w);
            }
        }

        int valueX = (int)((float)sumX / sumW);
        int valueY = (int)((float)sumY / sumW);

        if (v[valueY] == 1)
        {
            ret->x.push_back(valueX);
            ret->y.push_back(valueY);
        }
    }

    return ret;
}

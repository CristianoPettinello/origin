//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#include <stdlib.h> // malloc
#include <string.h> // memcpy

#include "Image.h"

#define PADDING 4

using namespace Nidek::Libraries::Utils;

template class Nidek::Libraries::Utils::Image<uint8_t>;
template class Nidek::Libraries::Utils::Image<float>;

template <class T>
Image<T>::Image(uint16_t width, uint16_t height, uint8_t nChannels)
    : data(nullptr),
      m_width(width),
      m_height(height),
      m_nChannels(nChannels)
{
    // Compute the number of bytes that one needs to add to the address
    // in the first pixel of a row in order to go to the address of the
    // first pixel of the next row
    m_stride = m_width * m_nChannels * sizeof(T);
    uint8_t modulo = m_stride % PADDING;
    if (modulo > 0)
    {
        m_stride += PADDING - modulo;
    }
    uint32_t size = m_height * m_stride;
    // data = (T*)malloc(size);
    // data = (T *)calloc(size, sizeof(T));
    data = (T*)calloc(size, sizeof(char));
}

template <class T> Image<T>::Image(const Image<T>& other)
{
    m_width = other.m_width;
    m_height = other.m_height;
    m_nChannels = other.m_nChannels;
    uint32_t stride = m_width * m_nChannels * sizeof(T);
    uint8_t modulo = stride % PADDING;
    if (modulo > 0)
    {
        stride += PADDING - modulo;
    }
    m_stride = stride;
    uint32_t size = m_height * m_stride;
    // data = (T *)calloc(size, sizeof(T));
    data = (T*)calloc(size, sizeof(char));

    // Original code
    /*for (int h = 0; h < m_height; ++h)
    {
            T *dstPointer = &data[h * m_stride / sizeof(T)];
            T *srcPointer = &other.data[h * other.m_stride / sizeof(T)];
            memcpy(dstPointer, srcPointer, sizeof(T)*other.m_width*other.m_nChannels);
    }*/

    int srcStrideInT = m_stride / sizeof(T);
    int dstStrideInT = other.m_stride / sizeof(T);
    int srcIndex = 0;
    int dstIndex = 0;
    for (int h = 0; h < m_height; ++h)
    {
        T* dstPointer = &data[srcIndex];
        T* srcPointer = &other.data[dstIndex];
        memcpy(dstPointer, srcPointer, sizeof(T) * other.m_width * other.m_nChannels);

        srcIndex += srcStrideInT;
        dstIndex += dstStrideInT;
    }
}

template <class T> Image<T>::~Image()
{
    if (data)
    {
        free(data);
        data = nullptr;
    }
}

template <class T> T* Image<T>::getData()
{
    return data;
}

template <class T> uint16_t Image<T>::getWidth()
{
    return m_width;
}

template <class T> uint16_t Image<T>::getHeight()
{
    return m_height;
}

template <class T> uint8_t Image<T>::getNChannels()
{
    return m_nChannels;
}

template <class T> uint16_t Image<T>::getStride()
{
    return m_stride;
}

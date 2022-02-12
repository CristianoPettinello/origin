//
//          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
//  be copied or disclosed  except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
//

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "UtilsGlobal.h"
#include <stdint.h> // uint8_t, ...

namespace Nidek {
namespace Libraries {
namespace Utils {
///
/// \brief Image class.
///
/// This class is a specialized buffer representing an image in the HTT processing algorithm.
/// Since the algorithm works both with uint8_t and float data types, the class should be modelled as a template object.
/// The class public interface should implement the methods for retrieving image width, height, stride,
/// number of channels, and a pointer to the raw buffer.
///
/// \tparam T Data type to work with: uint8_t and float are supported.
///
template <class T> class Image
{
private:
    T* data;             ///< Pointer to row data
    uint16_t m_width;    ///< Width of image in units of pixels
    uint16_t m_height;   ///< Height of image in units of pixels
    uint8_t m_nChannels; ///< Number of channels
    uint16_t m_stride;   ///< Length of the line in bytes

public:
    ///
    /// Constructor used to creates a black image with the specified size.
    ///
    /// \param [in] width 		Image width in pixel.
    /// \param [in] height 		Image height in pixel.
    /// \param [in] nChannels	Number of channels in the image, i.e RGB (three channels) or monochromatic (single
    /// channel).
    ///
    UTILS_LIBRARYSHARED_EXPORT Image<T>(uint16_t width, uint16_t height, uint8_t nChannels);

    ///
    /// Copy constructor. Save a copy of the input image.
    ///
    /// \param [in] other		Shared pointer of the image to be saved as a copy
    ///
    UTILS_LIBRARYSHARED_EXPORT Image<T>(const Image<T>& other);

    ///
    /// Desctructor used to free the memory allocated with the constructor.
    ///
    UTILS_LIBRARYSHARED_EXPORT ~Image();

public:
    /// \return A pointer to row data.
    UTILS_LIBRARYSHARED_EXPORT T* getData();

    /// \return An integer that rapresents the width of the image in units of pixels.
    UTILS_LIBRARYSHARED_EXPORT uint16_t getWidth();

    /// \return An integer that rapresents the height of the image in units of pixels.
    UTILS_LIBRARYSHARED_EXPORT uint16_t getHeight();

    /// \return Number of channels in the image.
    UTILS_LIBRARYSHARED_EXPORT uint8_t getNChannels();

    /// \return The actual length of the line in bytes.
    UTILS_LIBRARYSHARED_EXPORT uint16_t getStride();
};
} // namespace Utils
} // namespace Libraries
} // namespace Nidek

#endif // __IMAGE_H__
